/*
  Copyright (c) 2005-2007 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/



#ifdef _WIN32
# include "../config.h.win"
#elif defined( _WIN32_WCE )
# include "../config.h.win"
#else
# include "config.h"
#endif

#include "clientbase.h"
#include "connectionbase.h"
#include "tlsbase.h"
#include "compressionbase.h"
#include "connectiontcpclient.h"
#include "disco.h"
#include "messagesessionhandler.h"
#include "tag.h"
#include "iq.h"
#include "message.h"
#include "subscription.h"
#include "presence.h"
#include "connectionlistener.h"
#include "iqhandler.h"
#include "messagehandler.h"
#include "presencehandler.h"
#include "rosterlistener.h"
#include "subscriptionhandler.h"
#include "loghandler.h"
#include "taghandler.h"
#include "mucinvitationhandler.h"
#include "jid.h"
#include "base64.h"
#include "error.h"
#include "md5.h"
#include "util.h"
#include "tlsdefault.h"
#include "compressionzlib.h"
#include "stanzaextensionfactory.h"

#include <cstdlib>
#include <string>
#include <map>
#include <list>
#include <algorithm>
#include <cmath>

#ifndef _WIN32_WCE
# include <sstream>
# include <iomanip>
#endif

namespace gloox
{

  ClientBase::ClientBase( const std::string& ns, const std::string& server, int port )
    : m_connection( 0 ), m_encryption( 0 ), m_compression( 0 ), m_disco( 0 ), m_namespace( ns ),
      m_xmllang( "en" ), m_server( server ), m_compressionActive( false ), m_encryptionActive( false ),
      m_compress( true ), m_authed( false ), m_sasl( true ), m_tls( TLSOptional ), m_port( port ),
      m_availableSaslMechs( SaslMechAll ),
      m_statisticsHandler( 0 ), m_mucInvitationHandler( 0 ),
      m_messageSessionHandlerChat( 0 ), m_messageSessionHandlerGroupchat( 0 ),
      m_messageSessionHandlerHeadline( 0 ), m_messageSessionHandlerNormal( 0 ),
      m_parser( this ), m_seFactory( 0 ), m_authError( AuthErrorUndefined ),
      m_streamError( StreamErrorUndefined ), m_streamErrorAppCondition( 0 ),
      m_selectedSaslMech( SaslMechNone ), m_idCount( 0 ), m_autoMessageSession( false )
  {
    init();
  }

  ClientBase::ClientBase( const std::string& ns, const std::string& password,
                          const std::string& server, int port )
    : m_connection( 0 ), m_encryption( 0 ), m_compression( 0 ), m_disco( 0 ), m_namespace( ns ),
      m_password( password ),
      m_xmllang( "en" ), m_server( server ), m_compressionActive( false ), m_encryptionActive( false ),
      m_compress( true ), m_authed( false ), m_block( false ), m_sasl( true ), m_tls( TLSOptional ),
      m_port( port ), m_availableSaslMechs( SaslMechAll ),
      m_statisticsHandler( 0 ), m_mucInvitationHandler( 0 ),
      m_messageSessionHandlerChat( 0 ), m_messageSessionHandlerGroupchat( 0 ),
      m_messageSessionHandlerHeadline( 0 ), m_messageSessionHandlerNormal( 0 ),
      m_parser( this ), m_seFactory( 0 ), m_authError( AuthErrorUndefined ),
      m_streamError( StreamErrorUndefined ), m_streamErrorAppCondition( 0 ),
      m_selectedSaslMech( SaslMechNone ), m_idCount( 0 ), m_autoMessageSession( false )
  {
    init();
  }

  void ClientBase::init()
  {
    if( !m_disco )
    {
      m_disco = new Disco( this );
      m_disco->setVersion( "based on gloox", GLOOX_VERSION );
    }

    if( !m_seFactory )
      registerStanzaExtension( new Error() );

    m_streamError = StreamErrorUndefined;
    m_block = false;
    memset( &m_stats, 0, sizeof( m_stats ) );
    cleanup();
  }

  ClientBase::~ClientBase()
  {
    delete m_connection;
    delete m_encryption;
    delete m_compression;
    delete m_disco;
    delete m_seFactory;

    util::clearList( m_messageSessions );

    PresenceJidHandlerList::const_iterator it1 = m_presenceJidHandlers.begin();
    for( ; it1 != m_presenceJidHandlers.end(); ++it1 )
      delete (*it1).jid;
  }

  ConnectionError ClientBase::recv( int timeout )
  {
    if( !m_connection || m_connection->state() == StateDisconnected )
      return ConnNotConnected;

    return m_connection->recv( timeout );
  }

  bool ClientBase::connect( bool block )
  {
    if( m_server.empty() )
      return false;

    if( !m_connection )
      m_connection = new ConnectionTCPClient( this, m_logInstance, m_server, m_port );

    if( m_connection->state() >= StateConnecting )
      return true;

    if( !m_encryption )
      m_encryption = getDefaultEncryption();

    if( m_encryption )
    {
      m_encryption->setCACerts( m_cacerts );
      m_encryption->setClientCert( m_clientKey, m_clientCerts );
    }

    if( !m_compression )
      m_compression = getDefaultCompression();

    m_logInstance.dbg( LogAreaClassClientbase, "This is gloox " + GLOOX_VERSION + ", connecting..." );
    m_block = block;
    ConnectionError ret = m_connection->connect();
    return ret == ConnNoError;
  }

  void ClientBase::handleTag( Tag* tag )
  {
    if( !tag )
    {
      logInstance().dbg( LogAreaClassClientbase, "stream closed" );
      disconnect( ConnStreamClosed );
      return;
    }

    logInstance().dbg( LogAreaXmlIncoming, tag->xml() );
    ++m_stats.totalStanzasReceived;

    if( tag->name() == "stream" && tag->xmlns() == XMLNS_STREAM )
    {
      const std::string& version = tag->findAttribute( "version" );
      if( !checkStreamVersion( version ) )
      {
        logInstance().dbg( LogAreaClassClientbase, "This server is not XMPP-compliant"
            " (it does not send a 'version' attribute). Please fix it or try another one.\n" );
        disconnect( ConnStreamVersionError );
      }

      m_sid = tag->findAttribute( "id" );
      handleStartNode();
    }
    else if( tag->name() == "error" && tag->xmlns() == XMLNS_STREAM )
    {
      handleStreamError( tag );
      disconnect( ConnStreamError );
    }
    else
    {
      if( !handleNormalNode( tag ) )
      {
        if( tag->xmlns().empty() || tag->xmlns() == XMLNS_CLIENT )
        {
          if( tag->name() == "iq"  )
          {
            IQ iq( tag );
            m_seFactory->addExtensions( iq, tag );
            notifyIqHandlers( &iq );
            ++m_stats.iqStanzasReceived;
          }
          else if( tag->name() == "message" )
          {
            Message msg( tag );
            m_seFactory->addExtensions( msg, tag );
            notifyMessageHandlers( &msg );
            ++m_stats.messageStanzasReceived;
          }
          else if( tag->name() == "presence" )
          {
            const std::string& type = tag->findAttribute( TYPE );
            if( type == "subscribe"  || type == "unsubscribe"
                || type == "subscribed" || type == "unsubscribed" )
            {
              Subscription sub( tag );
              m_seFactory->addExtensions( sub, tag );
              notifySubscriptionHandlers( &sub );
              ++m_stats.s10nStanzasReceived;
            }
            else
            {
              Presence pres( tag );
              m_seFactory->addExtensions( pres, tag );
              notifyPresenceHandlers( &pres );
              ++m_stats.presenceStanzasReceived;
            }
          }
          else
            m_logInstance.err( LogAreaClassClientbase, "Received invalid stanza." );
        }
        else
        {
          notifyTagHandlers( tag );
        }
      }
    }

    if( m_statisticsHandler )
      m_statisticsHandler->handleStatistics( getStatistics() );
  }

  void ClientBase::handleCompressedData( const std::string& data )
  {
    if( m_encryption && m_encryptionActive )
      m_encryption->encrypt( data );
    else if( m_connection )
      m_connection->send( data );
    else
      m_logInstance.err( LogAreaClassClientbase, "Compression finished, but chain broken" );
  }

  void ClientBase::handleDecompressedData( const std::string& data )
  {
    parse( data );
  }

  void ClientBase::handleEncryptedData( const TLSBase* /*base*/, const std::string& data )
  {
    if( m_connection )
      m_connection->send( data );
    else
      m_logInstance.err( LogAreaClassClientbase, "Encryption finished, but chain broken" );
  }

  void ClientBase::handleDecryptedData( const TLSBase* /*base*/, const std::string& data )
  {
    if( m_compression && m_compressionActive )
      m_compression->decompress( data );
    else
      parse( data );
  }

  void ClientBase::handleHandshakeResult( const TLSBase* /*base*/, bool success, CertInfo &certinfo )
  {
    if( success )
    {
      if( !notifyOnTLSConnect( certinfo ) )
      {
        logInstance().err( LogAreaClassClientbase, "Server's certificate rejected!" );
        disconnect( ConnTlsFailed );
      }
      else
      {
        logInstance().dbg( LogAreaClassClientbase, "connection encryption active" );
        header();
      }
    }
    else
    {
      logInstance().err( LogAreaClassClientbase, "TLS handshake failed!" );
      disconnect( ConnTlsFailed );
    }
  }

  void ClientBase::handleReceivedData( const ConnectionBase* /*connection*/, const std::string& data )
  {
    if( m_encryption && m_encryptionActive )
      m_encryption->decrypt( data );
    else if( m_compression && m_compressionActive )
      m_compression->decompress( data );
    else
      parse( data );
  }

  void ClientBase::handleConnect( const ConnectionBase* /*connection*/ )
  {
    header();
    if( m_block && m_connection )
    {
      m_connection->receive();
    }
  }

  void ClientBase::handleDisconnect( const ConnectionBase* /*connection*/, ConnectionError reason )
  {
    if( m_connection )
      m_connection->cleanup();
    notifyOnDisconnect( reason );
  }

  void ClientBase::disconnect( ConnectionError reason )
  {
    if( m_connection && m_connection->state() >= StateConnecting )
    {
      if( reason != ConnTlsFailed )
        send( "</stream:stream>" );

      m_connection->disconnect();
      m_connection->cleanup();

      if( m_encryption )
        m_encryption->cleanup();

      m_encryptionActive = false;
      m_compressionActive = false;

      notifyOnDisconnect( reason );
    }
  }

  void ClientBase::parse( const std::string& data )
  {
    std::string copy = data;
    int i = 0;
    if( ( i = m_parser.feed( copy ) ) >= 0 )
    {
      const int len = 4 + (int)std::log10( i ? i : 1 ) + 1;
      char* tmp = new char[len];
      tmp[len-1] = '\0';
      sprintf( tmp, "%d", i );
      std::string error = "parse error (at pos ";
      error += tmp;
      error += "): ";
      m_logInstance.err( LogAreaClassClientbase, error + copy );
      Tag* e = new Tag( "stream:error" );
      new Tag( e, "restricted-xml", "xmlns", XMLNS_XMPP_STREAM );
      send( e );
      disconnect( ConnParseError );
      delete[] tmp;
    }
  }

  void ClientBase::header()
  {
    std::string head = "<?xml version='1.0' ?>";
    head += "<stream:stream to='" + m_jid.server() + "' xmlns='" + m_namespace + "' ";
    head += "xmlns:stream='http://etherx.jabber.org/streams'  xml:lang='" + m_xmllang + "' ";
    head += "version='" + XMPP_STREAM_VERSION_MAJOR + "." + XMPP_STREAM_VERSION_MINOR + "'>";
    send( head );
  }

  bool ClientBase::hasTls()
  {
#if defined( HAVE_GNUTLS ) || defined( HAVE_OPENSSL ) || defined( HAVE_WINTLS )
    return true;
#else
    return false;
#endif
  }

  void ClientBase::startTls()
  {
    send( new Tag( "starttls", XMLNS, XMLNS_STREAM_TLS ) );
  }

  void ClientBase::setServer( const std::string &server )
  {
    m_server = server;
    if( m_connection )
      m_connection->setServer( server );
  }

  void ClientBase::setClientCert( const std::string& clientKey, const std::string& clientCerts )
  {
    m_clientKey = clientKey;
    m_clientCerts = clientCerts;
  }

  void ClientBase::startSASL( SaslMechanism type )
  {
    m_selectedSaslMech = type;

    Tag* a = new Tag( "auth", XMLNS, XMLNS_STREAM_SASL );

    switch( type )
    {
      case SaslMechDigestMd5:
        a->addAttribute( "mechanism", "DIGEST-MD5" );
        break;
      case SaslMechPlain:
      {
        a->addAttribute( "mechanism", "PLAIN" );

        std::string tmp;
        if( m_authzid )
          tmp += m_authzid.bare();

        tmp += '\0';
        tmp += m_jid.username();
        tmp += '\0';
        tmp += m_password;
        a->setCData( Base64::encode64( tmp ) );
        break;
      }
      case SaslMechAnonymous:
        a->addAttribute( "mechanism", "ANONYMOUS" );
        a->setCData( getID() );
        break;
      case SaslMechExternal:
        a->addAttribute( "mechanism", "EXTERNAL" );
        a->setCData( Base64::encode64( m_authzid ? m_authzid.bare() : m_jid.bare() ) );
       break;
      case SaslMechGssapi:
      {
#ifdef _WIN32
        a->addAttribute( "mechanism", "GSSAPI" );
// The client calls GSS_Init_sec_context, passing in 0 for
// input_context_handle (initially) and a targ_name equal to output_name
// from GSS_Import_Name called with input_name_type of
// GSS_C_NT_HOSTBASED_SERVICE and input_name_string of
// "service@hostname" where "service" is the service name specified in
// the protocol's profile, and "hostname" is the fully qualified host
// name of the server.  The client then responds with the resulting
// output_token.
        std::string token;
        a->setCData( Base64::encode64( token ) );
//         etc... see gssapi-sasl-draft.txt
#else
        logInstance().err( LogAreaClassClientbase,
                    "GSSAPI is not supported on this platform. You should never see this." );
#endif
        break;
      }
      default:
        break;
    }

    send( a );
  }

  void ClientBase::processSASLChallenge( const std::string& challenge )
  {
    Tag* t = new Tag( "response", XMLNS, XMLNS_STREAM_SASL );

    const std::string& decoded = Base64::decode64( challenge );

    switch( m_selectedSaslMech )
    {
      case SaslMechDigestMd5:
      {
        if( !decoded.compare( 0, 7, "rspauth" ) )
          break;

        std::string realm;
        std::string::size_type end = 0;
        std::string::size_type pos = decoded.find( "realm=" );
        if( pos != std::string::npos )
        {
          end = decoded.find( '"', pos + 7 );
          realm = decoded.substr( pos + 7, end - ( pos + 7 ) );
        }
        else
          realm = m_jid.server();

        pos = decoded.find( "nonce=" );
        if( pos == std::string::npos )
          return;

        end = decoded.find( '"', pos + 7 );
        while( decoded[end-1] == '\\' )
          end = decoded.find( '"', end + 1 );
        std::string nonce = decoded.substr( pos + 7, end - ( pos + 7 ) );

        std::string cnonce;
#ifdef _WIN32_WCE
        char cn[4*8+1];
        for( int i = 0; i < 4; ++i )
          sprintf( cn + i*8, "%08x", rand() );
        cnonce.assign( cn, 4*8 );
#else
        std::ostringstream cn;
        for( int i = 0; i < 4; ++i )
          cn << std::hex << std::setw( 8 ) << std::setfill( '0' ) << rand();
        cnonce = cn.str();
#endif

        MD5 md5;
        md5.feed( m_jid.username() );
        md5.feed( ":" );
        md5.feed( realm );
        md5.feed( ":" );
        md5.feed( m_password );
        md5.finalize();
        const std::string& a1_h = md5.binary();
        md5.reset();
        md5.feed( a1_h );
        md5.feed( ":" );
        md5.feed( nonce );
        md5.feed( ":" );
        md5.feed( cnonce );
        md5.finalize();
        const std::string& a1  = md5.hex();
        md5.reset();
        md5.feed( "AUTHENTICATE:xmpp/" );
        md5.feed( m_jid.server() );
        md5.finalize();
        const std::string& a2 = md5.hex();
        md5.reset();
        md5.feed( a1 );
        md5.feed( ":" );
        md5.feed( nonce );
        md5.feed( ":00000001:" );
        md5.feed( cnonce );
        md5.feed( ":auth:" );
        md5.feed( a2 );
        md5.finalize();

        std::string response = "username=\"";
        response += m_jid.username();
        response += "\",realm=\"";
        response += realm;
        response += "\",nonce=\"";
        response += nonce;
        response += "\",cnonce=\"";
        response += cnonce;
        response += "\",nc=00000001,qop=auth,digest-uri=\"xmpp/";
        response += m_jid.server();
        response += "\",response=";
        response += md5.hex();
        response += ",charset=utf-8";

        if( m_authzid )
          response += ",authzid=" + m_authzid.bare();

        t->setCData( Base64::encode64( response ) );

        break;
      }
      case SaslMechGssapi:
#ifdef _WIN32
        // see gssapi-sasl-draft.txt
#else
        m_logInstance.err( LogAreaClassClientbase,
                           "Huh, received GSSAPI challenge?! This should have never happened!" );
#endif
        break;
      default:
        // should never happen.
        break;
    }

    send( t );
  }

  void ClientBase::processSASLError( Tag* tag )
  {
    if( tag->hasChild( "aborted" ) )
      m_authError = SaslAborted;
    else if( tag->hasChild( "incorrect-encoding" ) )
      m_authError = SaslIncorrectEncoding;
    else if( tag->hasChild( "invalid-authzid" ) )
      m_authError = SaslInvalidAuthzid;
    else if( tag->hasChild( "invalid-mechanism" ) )
      m_authError = SaslInvalidMechanism;
    else if( tag->hasChild( "malformed-request" ) )
      m_authError = SaslMalformedRequest;
    else if( tag->hasChild( "mechanism-too-weak" ) )
      m_authError = SaslMechanismTooWeak;
    else if( tag->hasChild( "not-authorized" ) )
      m_authError = SaslNotAuthorized;
    else if( tag->hasChild( "temporary-auth-failure" ) )
      m_authError = SaslTemporaryAuthFailure;
  }

  void ClientBase::send( const IQ& iq, IqHandler* ih, int context )
  {
    if( ( iq.subtype() == IQ::Set || iq.subtype() == IQ::Get ) && ih && !iq.id().empty() )
    {
      TrackStruct track;
      track.ih = ih;
      track.context = context;
      m_iqIDHandlers[iq.id()] = track;
    }

    send( iq );
  }

  void ClientBase::send( const IQ& iq )
  {
    ++m_stats.iqStanzasSent;
    Tag* tag = iq.tag();
    addFrom( tag );
    send( tag );
  }

  void ClientBase::send( const Message& msg )
  {
    ++m_stats.messageStanzasSent;
    Tag* tag = msg.tag();
    addFrom( tag );
    send( tag );
  }

  void ClientBase::send( const Subscription& sub )
  {
    ++m_stats.s10nStanzasSent;
    Tag* tag = sub.tag();
    addFrom( tag );
    send( tag );
  }

  void ClientBase::send( const Presence& pres )
  {
    ++m_stats.presenceStanzasSent;
    Tag* tag = pres.tag();
    addFrom( tag );
    send( tag );
  }

  void ClientBase::send( Tag* tag )
  {
    if( !tag )
      return;

    send( tag->xml() );

    ++m_stats.totalStanzasSent;

    if( m_statisticsHandler )
      m_statisticsHandler->handleStatistics( getStatistics() );

    delete tag;
  }

  void ClientBase::send( const std::string& xml )
  {
    if( m_connection && m_connection->state() == StateConnected )
    {
      if( m_compression && m_compressionActive )
        m_compression->compress( xml );
      else if( m_encryption && m_encryptionActive )
        m_encryption->encrypt( xml );
      else
        m_connection->send( xml );

      logInstance().dbg( LogAreaXmlOutgoing, xml );
    }
  }

  void ClientBase::addFrom( Tag* tag )
  {
    if( !tag || m_selectedResource.empty() )
      return;

    tag->addAttribute( "from", m_jid.bare() + '/' + m_selectedResource );
  }

  void ClientBase::registerStanzaExtension( StanzaExtension* ext )
  {
    if( !m_seFactory )
      m_seFactory = new StanzaExtensionFactory();

    m_seFactory->registerExtension( ext );
  }

  StatisticsStruct ClientBase::getStatistics()
  {
    if( m_connection )
      m_connection->getStatistics( m_stats.totalBytesReceived, m_stats.totalBytesSent );

    return m_stats;
  }

  ConnectionState ClientBase::state() const
  {
    return m_connection ? m_connection->state() : StateDisconnected;
  }

  void ClientBase::whitespacePing()
  {
    send( " " );
  }

  void ClientBase::xmppPing( const JID& to )
  {
    IQ iq( IQ::Get, to, getID(), XMLNS_XMPP_PING, "ping" );
    send( iq );
  }

  const std::string ClientBase::getID()
  {
#ifdef _WIN32_WCE
    char r[8+1];
    sprintf( r, "%08x", rand() );
    std::string ret( r, 8 );
    return std::string( "uid" ) + ret;
#else
    std::ostringstream oss;
    oss << ++m_idCount;
    return std::string( "uid" ) + oss.str();
#endif
  }

  bool ClientBase::checkStreamVersion( const std::string& version )
  {
    if( version.empty() )
      return false;

    int major = 0;
    int minor = 0;
    int myMajor = atoi( XMPP_STREAM_VERSION_MAJOR.c_str() );

    size_t dot = version.find( '.' );
    if( !version.empty() && dot && dot != std::string::npos )
    {
      major = atoi( version.substr( 0, dot ).c_str() );
      minor = atoi( version.substr( dot ).c_str() );
    }

    return myMajor >= major;
  }

  void ClientBase::setConnectionImpl( ConnectionBase* cb )
  {
    if( m_connection )
    {
      delete m_connection;
    }
    m_connection = cb;
  }

  void ClientBase::setEncryptionImpl( TLSBase* tb )
  {
    if( m_encryption )
    {
      delete m_encryption;
    }
    m_encryption = tb;
  }

  void ClientBase::setCompressionImpl( CompressionBase* cb )
  {
    if( m_compression )
    {
      delete m_compression;
    }
    m_compression = cb;
  }

  void ClientBase::handleStreamError( Tag* tag )
  {
    StreamError err = StreamErrorUndefined;
    const TagList& c = tag->children();
    TagList::const_iterator it = c.begin();
    for( ; it != c.end(); ++it )
    {
      const std::string& name = (*it)->name();
      if( name == "bad-format" )
        err = StreamErrorBadFormat;
      else if( name == "bad-namespace-prefix" )
        err = StreamErrorBadNamespacePrefix;
      else if( name == "conflict" )
        err = StreamErrorConflict;
      else if( name == "connection-timeout" )
        err = StreamErrorConnectionTimeout;
      else if( name == "host-gone" )
        err = StreamErrorHostGone;
      else if( name == "host-unknown" )
        err = StreamErrorHostUnknown;
      else if( name == "improper-addressing" )
        err = StreamErrorImproperAddressing;
      else if( name == "internal-server-error" )
        err = StreamErrorInternalServerError;
      else if( name == "invalid-from" )
        err = StreamErrorInvalidFrom;
      else if( name == "invalid-id" )
        err = StreamErrorInvalidId;
      else if( name == "invalid-namespace" )
        err = StreamErrorInvalidNamespace;
      else if( name == "invalid-xml" )
        err = StreamErrorInvalidXml;
      else if( name == "not-authorized" )
        err = StreamErrorNotAuthorized;
      else if( name == "policy-violation" )
        err = StreamErrorPolicyViolation;
      else if( name == "remote-connection-failed" )
        err = StreamErrorRemoteConnectionFailed;
      else if( name == "resource-constraint" )
        err = StreamErrorResourceConstraint;
      else if( name == "restricted-xml" )
        err = StreamErrorRestrictedXml;
      else if( name == "see-other-host" )
      {
        err = StreamErrorSeeOtherHost;
        m_streamErrorCData = tag->findChild( "see-other-host" )->cdata();
      }
      else if( name == "system-shutdown" )
        err = StreamErrorSystemShutdown;
      else if( name == "undefined-condition" )
        err = StreamErrorUndefinedCondition;
      else if( name == "unsupported-encoding" )
        err = StreamErrorUnsupportedEncoding;
      else if( name == "unsupported-stanza-type" )
        err = StreamErrorUnsupportedStanzaType;
      else if( name == "unsupported-version" )
        err = StreamErrorUnsupportedVersion;
      else if( name == "xml-not-well-formed" )
        err = StreamErrorXmlNotWellFormed;
      else if( name == "text" )
      {
        const std::string& lang = (*it)->findAttribute( "xml:lang" );
        if( !lang.empty() )
          m_streamErrorText[lang] = (*it)->cdata();
        else
          m_streamErrorText["default"] = (*it)->cdata();
      }
      else
        m_streamErrorAppCondition = (*it);

      if( err != StreamErrorUndefined && (*it)->hasAttribute( XMLNS, XMLNS_XMPP_STREAM ) )
        m_streamError = err;
    }
  }

  const std::string& ClientBase::streamErrorText( const std::string& lang ) const
  {
    StringMap::const_iterator it = m_streamErrorText.find( lang );
    return ( it != m_streamErrorText.end() ) ? (*it).second : EmptyString;
  }

  void ClientBase::registerMessageSessionHandler( MessageSessionHandler* msh, int types )
  {
    if( types & Message::Chat || types == 0 )
      m_messageSessionHandlerChat = msh;

    if( types & Message::Normal || types == 0 )
      m_messageSessionHandlerNormal = msh;

    if( types & Message::Groupchat || types == 0 )
      m_messageSessionHandlerGroupchat = msh;

    if( types & Message::Headline || types == 0 )
      m_messageSessionHandlerHeadline = msh;
  }

  void ClientBase::registerPresenceHandler( PresenceHandler* ph )
  {
    if( ph )
      m_presenceHandlers.push_back( ph );
  }

  void ClientBase::removePresenceHandler( PresenceHandler* ph )
  {
    if( ph )
      m_presenceHandlers.remove( ph );
  }

  void ClientBase::registerPresenceHandler( const JID& jid, PresenceHandler* ph )
  {
    if( ph && jid )
    {
      JidPresHandlerStruct jph;
      jph.jid = new JID( jid.bare() );
      jph.ph = ph;
      m_presenceJidHandlers.push_back( jph );
    }
  }

  void ClientBase::removePresenceHandler( const JID& jid, PresenceHandler* ph )
  {
    PresenceJidHandlerList::iterator t;
    PresenceJidHandlerList::iterator it = m_presenceJidHandlers.begin();
    while( it != m_presenceJidHandlers.end() )
    {
      t = it;
      ++it;
      if( ( !ph || (*t).ph == ph ) && (*t).jid->bare() == jid.bare() )
      {
        delete (*t).jid;
        m_presenceJidHandlers.erase( t );
      }
    }
  }

  void ClientBase::trackID( IqHandler* ih, const std::string& id, int context )
  {
    if( ih && !id.empty() )
    {
      TrackStruct track;
      track.ih = ih;
      track.context = context;
      m_iqIDHandlers[id] = track;
    }
  }

  void ClientBase::removeIDHandler( IqHandler* ih )
  {
    IqTrackMap::iterator t;
    IqTrackMap::iterator it = m_iqIDHandlers.begin();
    while( it != m_iqIDHandlers.end() )
    {
      t = it;
      ++it;
      if( ih == (*t).second.ih )
        m_iqIDHandlers.erase( t );
    }
  }

  void ClientBase::registerIqHandler( IqHandler* ih, const std::string& xmlns )
  {
    if( !ih || xmlns.empty() )
      return;

    typedef IqHandlerMap::const_iterator IQci;
    std::pair<IQci, IQci> g = m_iqNSHandlers.equal_range( xmlns );
    for( IQci it = g.first; it != g.second; ++it )
      if( (*it).second == ih )
        return;

    m_iqNSHandlers.insert( make_pair( xmlns, ih ) );
  }

  void ClientBase::removeIqHandler( IqHandler* ih, const std::string& xmlns )
  {
    if( !ih || xmlns.empty() )
      return;

    typedef IqHandlerMap::iterator IQi;
    std::pair<IQi, IQi> g = m_iqNSHandlers.equal_range( xmlns );
    IQi it_;
    IQi it = g.first;
    while( it != g.second )
    {
      if( (*it).second == ih )
      {
        m_iqNSHandlers.erase( it++ );
      }
      else
        ++it;
    }
  }

  void ClientBase::registerMessageSession( MessageSession* session )
  {
    if( session )
      m_messageSessions.push_back( session );
  }

  void ClientBase::disposeMessageSession( MessageSession* session )
  {
    if( !session )
      return;

    MessageSessionList::iterator it = std::find( m_messageSessions.begin(),
                                                 m_messageSessions.end(),
                                                 session );
    if( it != m_messageSessions.end() )
    {
      delete (*it);
      m_messageSessions.erase( it );
    }
  }

  void ClientBase::registerMessageHandler( MessageHandler* mh )
  {
    if( mh )
      m_messageHandlers.push_back( mh );
  }

  void ClientBase::removeMessageHandler( MessageHandler* mh )
  {
    if( mh )
      m_messageHandlers.remove( mh );
  }

  void ClientBase::registerSubscriptionHandler( SubscriptionHandler* sh )
  {
    if( sh )
      m_subscriptionHandlers.push_back( sh );
  }

  void ClientBase::removeSubscriptionHandler( SubscriptionHandler* sh )
  {
    if( sh )
      m_subscriptionHandlers.remove( sh );
  }

  void ClientBase::registerTagHandler( TagHandler* th, const std::string& tag, const std::string& xmlns )
  {
    if( th && !tag.empty() )
    {
      TagHandlerStruct ths;
      ths.tag = tag;
      ths.xmlns = xmlns;
      ths.th = th;
      m_tagHandlers.push_back( ths );
    }
  }

  void ClientBase::removeTagHandler( TagHandler* th, const std::string& tag, const std::string& xmlns )
  {
    if( th )
    {
      TagHandlerList::iterator it = m_tagHandlers.begin();
      for( ; it != m_tagHandlers.end(); ++it )
      {
        if( (*it).th == th && (*it).tag == tag && (*it).xmlns == xmlns )
          m_tagHandlers.erase( it );
      }
    }
  }

  void ClientBase::registerStatisticsHandler( StatisticsHandler* sh )
  {
    if( sh )
      m_statisticsHandler = sh;
  }

  void ClientBase::removeStatisticsHandler()
  {
    m_statisticsHandler = 0;
  }

  void ClientBase::registerMUCInvitationHandler( MUCInvitationHandler* mih )
  {
    if( mih )
    {
      m_mucInvitationHandler = mih;
      m_disco->addFeature( XMLNS_MUC );
    }
  }

  void ClientBase::removeMUCInvitationHandler()
  {
    m_mucInvitationHandler = 0;
    m_disco->removeFeature( XMLNS_MUC );
  }

  void ClientBase::registerConnectionListener( ConnectionListener* cl )
  {
    if( cl )
      m_connectionListeners.push_back( cl );
  }

  void ClientBase::removeConnectionListener( ConnectionListener* cl )
  {
    if( cl )
      m_connectionListeners.remove( cl );
  }

  void ClientBase::notifyOnConnect()
  {
    util::ForEach( m_connectionListeners, &ConnectionListener::onConnect );
  }

  void ClientBase::notifyOnDisconnect( ConnectionError e )
  {
    util::ForEach( m_connectionListeners, &ConnectionListener::onDisconnect, e );
    init();
  }

  bool ClientBase::notifyOnTLSConnect( const CertInfo& info )
  {
    ConnectionListenerList::const_iterator it = m_connectionListeners.begin();
    for( ; it != m_connectionListeners.end() && (*it)->onTLSConnect( info ); ++it )
      ;
    return m_stats.encryption = ( it == m_connectionListeners.end() );
  }

  void ClientBase::notifyOnResourceBindError( const Error* error )
  {
    util::ForEach( m_connectionListeners, &ConnectionListener::onResourceBindError, error );
  }

  void ClientBase::notifyOnResourceBind( const std::string& resource )
  {
    util::ForEach( m_connectionListeners, &ConnectionListener::onResourceBind, resource );
  }

  void ClientBase::notifyOnSessionCreateError( const Error* error )
  {
    util::ForEach( m_connectionListeners, &ConnectionListener::onSessionCreateError, error );
  }

  void ClientBase::notifyStreamEvent( StreamEvent event )
  {
    util::ForEach( m_connectionListeners, &ConnectionListener::onStreamEvent, event );
  }

  void ClientBase::notifyPresenceHandlers( Presence* pres )
  {
    bool match = false;
    PresenceJidHandlerList::const_iterator itj = m_presenceJidHandlers.begin();
    for( ; itj != m_presenceJidHandlers.end(); ++itj )
    {
      if( (*itj).jid->bare() == pres->from().bare() && (*itj).ph )
      {
        (*itj).ph->handlePresence( *pres );
        (*itj).ph->handlePresence( pres );
        match = true;
      }
    }
    if( match )
      return;

    // FIXME remove this for() for 1.1:
    PresenceHandlerList::const_iterator it = m_presenceHandlers.begin();
    for( ; it != m_presenceHandlers.end(); ++it )
    {
      (*it)->handlePresence( *pres );
      (*it)->handlePresence( pres );
    }
      // FIXME and reinstantiate this:
//     util::ForEach( m_presenceHandlers, &PresenceHandler::handlePresence, pres );
  }

  void ClientBase::notifySubscriptionHandlers( Subscription* s10n )
  {
    // FIXME remove this for() for 1.1:
    SubscriptionHandlerList::const_iterator it = m_subscriptionHandlers.begin();
    for( ; it != m_subscriptionHandlers.end(); ++it )
    {
      (*it)->handleSubscription( *s10n );
      (*it)->handleSubscription( s10n );
    }
      // FIXME and reinstantiate this:
//     util::ForEach( m_subscriptionHandlers, &SubscriptionHandler::handleSubscription, s10n );
  }

  void ClientBase::notifyIqHandlers( IQ* iq )
  {
    IqTrackMap::iterator it_id = m_iqIDHandlers.find( iq->id() );
    if( it_id != m_iqIDHandlers.end() && iq->subtype() & ( IQ::Result | IQ::Error ) )
    {
      (*it_id).second.ih->handleIqID( *iq, (*it_id).second.context );
      (*it_id).second.ih->handleIqID( iq, (*it_id).second.context ); // FIXME remove for 1.1
      m_iqIDHandlers.erase( it_id );
      return;
    }

    if( !iq->query() )
      return;

    bool res = false;

    typedef IqHandlerMap::const_iterator IQci;
    std::pair<IQci, IQci> g = m_iqNSHandlers.equal_range( iq->xmlns() );
    for( IQci it = g.first; it != g.second; ++it )
    {
      if( (*it).second->handleIq( *iq ) )
        res = true;
      if( (*it).second->handleIq( iq ) ) // FIXME remove for 1.1
        res = true;
    }

    if( !res && iq->subtype() & ( IQ::Get | IQ::Set ) )
    {
      IQ re( IQ::Error, iq->from(), iq->id() );
      re.addExtension( new Error( StanzaErrorTypeCancel, StanzaErrorServiceUnavailable ) );
      send( re );
    }
  }

  void ClientBase::notifyMessageHandlers( Message* msg )
  {
    Tag* m = msg->tag();
    if( m_mucInvitationHandler && m )
    {
      const Tag* x = m->findChild( "x", XMLNS, XMLNS_MUC_USER ); // FIXME !!!
      if( x && x->hasChild( "invite" ) )
      {
        const Tag* i = x->findChild( "invite" );
        JID invitee( i->findAttribute( "from" ) );

        const Tag * t = i->findChild( "reason" );
        const std::string& reason( t ? t->cdata() : EmptyString );

        t = x->findChild( "password" );
        const std::string& password( t ? t->cdata() : EmptyString );

        m_mucInvitationHandler->handleMUCInvitation( msg->from(), invitee,
                                                     reason, msg->body(), password,
                                                     i->hasChild( "continue" ) );
        return;
      }
    }
    delete m;

    MessageSessionList::const_iterator it1 = m_messageSessions.begin();
    for( ; it1 != m_messageSessions.end(); ++it1 )
    {
      if( (*it1)->target().full() == msg->from().full() &&
            ( msg->thread().empty() || (*it1)->threadID() == msg->thread() ) &&
// FIXME don't use '== 0' here
            ( (*it1)->types() & msg->subtype() || (*it1)->types() == 0 ) )
      {
        (*it1)->handleMessage( *msg );
        return;
      }
    }

    it1 = m_messageSessions.begin();
    for( ; it1 != m_messageSessions.end(); ++it1 )
    {
      if( (*it1)->target().bare() == msg->from().bare() &&
            ( msg->thread().empty() || (*it1)->threadID() == msg->thread() ) &&
// FIXME don't use '== 0' here
            ( (*it1)->types() & msg->subtype() || (*it1)->types() == 0 ) )
      {
        (*it1)->handleMessage( *msg );
        return;
      }
    }

    MessageSessionHandler* msHandler = 0;

    switch( msg->subtype() )
    {
      case Message::Chat:
        msHandler = m_messageSessionHandlerChat;
        break;
      case Message::Normal:
        msHandler = m_messageSessionHandlerNormal;
        break;
      case Message::Groupchat:
        msHandler = m_messageSessionHandlerGroupchat;
        break;
      case Message::Headline:
        msHandler = m_messageSessionHandlerHeadline;
        break;
      default:
        break;
    }

    if( msHandler )
    {
      MessageSession* session = new MessageSession( this, msg->from(), true, msg->subtype() );
      msHandler->handleMessageSession( session );
      session->handleMessage( *msg );
    }
    else
    {
      // FIXME remove this for() for 1.1:
      MessageHandlerList::const_iterator it = m_messageHandlers.begin();
      for( ; it != m_messageHandlers.end(); ++it )
      {
        (*it)->handleMessage( *msg );
        (*it)->handleMessage( msg );
      }
      // FIXME and reinstantiate this:
//       util::ForEach( m_messageHandlers, &MessageHandler::handleMessage, *msg );
//       util::ForEach( m_messageHandlers, &MessageHandler::handleMessage, msg ); // FIXME remove for 1.1
    }
  }

  void ClientBase::notifyTagHandlers( Tag* tag )
  {
    TagHandlerList::const_iterator it = m_tagHandlers.begin();
    for( ; it != m_tagHandlers.end(); ++it )
    {
      if( (*it).tag == tag->name() && tag->hasAttribute( XMLNS, (*it).xmlns ) )
        (*it).th->handleTag( tag );
    }
  }

  CompressionBase* ClientBase::getDefaultCompression()
  {
    if( !m_compress )
      return 0;

#ifdef HAVE_ZLIB
    CompressionBase* cmp = new CompressionZlib( this );
    if( cmp->init() )
      return cmp;

    delete cmp;
#endif
    return 0;
  }

  TLSBase* ClientBase::getDefaultEncryption()
  {
    if( m_tls == TLSDisabled || !hasTls() )
      return 0;

    TLSDefault* tls = new TLSDefault( this, m_server );
    if( tls->init() )
      return tls;
    else
    {
      delete tls;
      return 0;
    }
  }

}
