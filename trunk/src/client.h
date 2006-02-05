/*
  Copyright (c) 2004-2005 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef CLIENT_H__
#define CLIENT_H__

#include "bareclient.h"
#include "iqhandler.h"

#include <string>

namespace gloox
{

  class RosterManager;
  class Disco;
  class Stanza;

  /**
   * @brief This class implements a full-featured Jabber Client.
   *
   * It supports @ref sasl_auth as well as TLS (Encryption), which can be
   * switched on/off separately. They are used automatically if the server supports them.
   *
   * To use, create a new Client instance and feed it connection credentials, either in the Constructor or
   * afterwards using the setters. You should then register packet handlers implementing the corresponding
   * Interfaces (ConnectionListener, PresenceHandler, MessageHandler, IqHandler, SubscriptionHandler),
   * and call @ref connect() to establish the connection to the server.<br>
   *
   * @note While the MessageHandler interface is still available (and will be in future versions)
   * it is now recommended to use the new @link gloox::MessageSession MessageSession @endlink for any
   * serious messaging.
   *
   * Simple usage example:
   * @code
   * using namespace gloox;
   *
   * void TestProg::doIt()
   * {
   *   Client* j = new Client( "user@server/resource", "password" );
   *   j->registerPresenceHandler( this );
   *   j->setVersion( "TestProg", "1.0" );
   *   j->setIdentity( "client", "bot" );
   *   j->setAutoPresence( true );
   *   j->setInitialPriority( 5 );
   *   j->connect();
   * }
   *
   * virtual void TestProg::presenceHandler( Stanza *stanza )
   * {
   *   // handle incoming presence packets here
   * }
   * @endcode
   *
   * However, you can skip the presence handling stuff if you make use of the RosterManager.
   *
   * By default, the library handles a few (incoming) IQ namespaces on the application's behalf. These
   * include:
   * @li jabber:iq:roster: by default the server-side roster is fetched and handled. Use
   * @ref rosterManager() and @ref RosterManager to interact with the Roster.
   * @li JEP-0092 (Software Version): If no version is specified, a name of "based on gloox" with
   * gloox's current version is announced.
   * @li JEP-0030 (Service Discovery): All supported/available services are announced. No items are
   * returned.
   * @note By default a priority of -1 is sent along with the initial presence. That means no message
   * stanzas will be received (from compliant servers). Use @ref setInitialPriority() to set a different
   * value. Also, no initial presence is sent which is usually required for a client to show up as
   * 'online' in their contact's contact list. Use setAutoPresence() to enable automatic sending of
   * initial presence.
   *
   * @section sasl_auth SASL Authentication
   *
   * Besides the simple, IQ-based authentication (JEP-0078), gloox supports several SASL (Simple
   * Authentication and Security Layer, RFC 2222) authentication mechanisms.
   * @li DIGEST-MD5: This mechanism is preferred over all other mechanisms if username and password are
   * provided to the Client instance. It is secure even without TLS encryption.
   * @li PLAIN: This mechanism is used if DIGEST-MD5 is not available. It is @b not secure without
   * encryption.
   * @li ANONYMOUS This mechanism is used if neither username nor password are set. The server generates
   * random, temporary username and resource and may restrict available services.
   * @li EXTERNAL This mechanism is currently only available if a client certificate and private key
   * are provided. The server tries to figure out who the client is by external means -- for instance,
   * using the provided certificate or even the IP address. (The restriction to certificate/key
   * availability is likely to be lifted in the future.)
   *
   * Of course, all these mechanisms are not tried unless the server offers them.
   *
   * @author Jakob Schroeter <js@camaya.net>
   */
  class GLOOX_API Client : public BareClient
  {
    public:

      friend class Parser;

      /**
       * Constructs a new Client which can be used for account registration only.
       * SASL and TLS are on by default. The port will be determined by looking up SRV records.
       * Alternatively, you can set the port explicitly by calling @ref setPort().
       * @param server The server to connect to.
       */
      Client( const std::string& server );

      /**
       * Constructs a new Client.
       * SASL and TLS are on by default. This should be the default constructor for most use cases.
       * The server address will be taken from the JID. The actual host will be resolved using SRV
       * records. The domain part of the JID is used as a fallback in case no SRV record is found, or
       * you can set the server address separately by calling @ref setServer().
       * @param jid A full Jabber ID used for connecting to the server.
       * @param password The password used for authentication.
       * @param port The port to connect to. The default of -1 means to look up the port via DNS SRV.
       */
      Client( const JID& jid, const std::string& password, int port = -1 );

      /**
       * Constructs a new Client.
       * SASL and TLS are on by default.
       * The actual host will be resolved using SRV records. The server value is used as a fallback
       * in case no SRV record is found.
       * @param username The username/local part of the JID.
       * @param resource The resource part of the JID.
       * @param password The password to use for authentication.
       * @param server The Jabber ID'S server part and the host name to connect to. If those are different
       * for your setup, use the second constructor instead.
       * @param port The port to connect to. The default of -1 means to look up the port via DNS SRV.
       */
      Client( const std::string& username, const std::string& password,
              const std::string& server, const std::string& resource, int port = -1 );

      /**
       * Virtual destructor.
       */
      virtual ~Client();

      /**
       * Disables automatic handling of disco queries.
       * There is currently no way to re-enable disco query-handling.
       * @note This disables the browsing capabilities because
       * both use the same @c Disco object.
       */
      void disableDisco();

      /**
       * Disables the automatic roster management.
       * You have to keep track of incoming presence yourself if
       * you want to have a roster.
       */
      void disableRoster();

      /**
       * This function gives access to the @c RosterManager object.
       * @return A pointer to the RosterManager.
       */
      RosterManager* rosterManager();

      /**
       * This function gives access to the @c Disco object.
       * @return A pointer to the Disco object.
       */
      Disco* disco();

    protected:
      /**
       * Called by ???
       */
      virtual void connected();

    private:
      void init();

      RosterManager *m_rosterManager;
      Disco *m_disco;

      bool m_manageRoster;
      bool m_handleDisco;

  };

}

#endif // CLIENT_H__
