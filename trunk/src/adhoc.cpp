/*
  Copyright (c) 2004-2007 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#include "adhoc.h"
#include "adhochandler.h"
#include "adhoccommandprovider.h"
#include "disco.h"
#include "error.h"
#include "discohandler.h"
#include "client.h"
#include "dataform.h"
#include "util.h"

namespace gloox
{

  static const char* cmdActionStringValues[] =
  {
    "execute", "cancel", "prev", "next", "complete"
  };

  static inline const std::string actionString( Adhoc::Command::Action action )
  {
    return util::lookup2( action, cmdActionStringValues );
  }

  static const char* cmdStatusStringValues[] =
  {
    "executing", "completed", "canceled"
  };

  static inline const std::string statusString( Adhoc::Command::Status status )
  {
    return util::lookup( status, cmdStatusStringValues );
  }

  static const char* cmdNoteStringValues[] =
  {
    "info", "warn", "error"
  };

  static inline const std::string noteString( Adhoc::Command::Severity sev )
  {
    return util::lookup( sev, cmdNoteStringValues );
  }

  // ---- Adhoc::Command::Note ----
  Tag* Adhoc::Command::Note::tag() const
  {
    if( m_note.empty() )
      return 0;

    Tag* n = new Tag( "note", m_note );
    n->addAttribute( TYPE, noteString( m_severity ) );
    return n;
  }
  // ---- ~Adhoc::Command::Note ----

  // ---- Adhoc::Command ----
  Adhoc::Command::Command( const std::string& node, Adhoc::Command::Action action )
    : StanzaExtension( ExtAdhocCommand ), m_node( node ), m_form( 0 ), m_action( action )
  {
  }

  Adhoc::Command::Command( const std::string& node, const std::string& sessionid,
                           Adhoc::Command::Action action )
    : StanzaExtension( ExtAdhocCommand ), m_node( node ), m_sessionid( sessionid ),
      m_form( 0 ), m_action( action )
  {
  }

  Adhoc::Command::Command( const Tag* tag )
    : StanzaExtension( ExtAdhocCommand ), m_actions( 0 )
  {
    if( !tag || tag->name() != "command" || tag->xmlns() != XMLNS_ADHOC_COMMANDS )
      return;

    m_node = tag->findAttribute( "node" );
    m_sessionid = tag->findAttribute( "sessionid" );
    m_action = (Action)util::lookup2( tag->findAttribute( "action" ), cmdActionStringValues );
    m_status = (Status)util::lookup( tag->findAttribute( "status" ), cmdStatusStringValues );

    Tag* a = tag->findChild( "actions" );
    if( a )
    {
      if( a->hasChild( "prev" ) )
        m_actions |= Previous;
      if( a->hasChild( "next" ) )
        m_actions |= Next;
      if( a->hasChild( "complete" ) )
        m_actions |= Complete;
    }

    const ConstTagList& l = tag->findTagList( "/command/note" );
    ConstTagList::const_iterator it = l.begin();
    for( ; it != l.end(); ++it )
    {
      Severity s = Info;
      if( (*it)->hasAttribute( TYPE ) )
        s = (Severity)util::lookup( tag->findAttribute( TYPE ), cmdNoteStringValues );
      m_notes.push_back( new Note( s, (*it)->cdata() ) );
    }
  }

  Adhoc::Command::~Command()
  {
    util::clearList( m_notes );
  }

  const std::string& Adhoc::Command::filterString() const
  {
    static const std::string filter = "/iq/command[@xmlns='" + XMLNS_ADHOC_COMMANDS + "']";
    return filter;
  }

  Tag* Adhoc::Command::tag() const
  {
    if( m_node.empty() )
      return 0;

    Tag* c = new Tag( "command" );
    c->setXmlns( XMLNS_ADHOC_COMMANDS );
    c->addAttribute( "node", m_node );
    if( m_action != InvalidAction )
      c->addAttribute( "action", actionString( m_action ) );
    if( m_status != InvalidStatus )
      c->addAttribute( "status", statusString( m_status ) );

    if( m_form && *m_form )
      c->addChild( m_form->tag() );

    NoteList::const_iterator it = m_notes.begin();
    for( ; it != m_notes.end(); ++it )
      c->addChild( (*it)->tag() );

    return c;
  }
  // ---- ~Adhoc::Command ----

  // ---- Adhoc ----
  Adhoc::Adhoc( ClientBase* parent )
    : m_parent( parent )
  {
    if( !m_parent || !m_parent->disco() )
      return;

    m_parent->registerIqHandler( this, XMLNS_ADHOC_COMMANDS );
    m_parent->disco()->addFeature( XMLNS_ADHOC_COMMANDS );
    m_parent->disco()->registerNodeHandler( this, XMLNS_ADHOC_COMMANDS );
    m_parent->disco()->registerNodeHandler( this, EmptyString );
    m_parent->registerStanzaExtension( new Adhoc::Command() );
  }

  Adhoc::~Adhoc()
  {
    if( !m_parent || !m_parent->disco() )
      return;

    m_parent->disco()->removeFeature( XMLNS_ADHOC_COMMANDS );
    m_parent->disco()->removeNodeHandler( this, XMLNS_ADHOC_COMMANDS );
    m_parent->disco()->removeNodeHandler( this, EmptyString );
    m_parent->removeIqHandler( this, XMLNS_ADHOC_COMMANDS );
  }

  StringList Adhoc::handleDiscoNodeFeatures( const JID& /*from*/, const std::string& /*node*/ )
  {
    StringList features;
    features.push_back( XMLNS_ADHOC_COMMANDS );
    return features;
//    return StringList( 1, XMLNS_ADHOC_COMMANDS );
  }

  Disco::ItemList Adhoc::handleDiscoNodeItems( const JID& /*from*/, const std::string& node )
  {
    Disco::ItemList l;
    if( node.empty() )
    {
      l.push_back( new Disco::Item( m_parent->jid(), XMLNS_ADHOC_COMMANDS, "Ad-Hoc Commands" ) );
    }
    else if( node == XMLNS_ADHOC_COMMANDS )
    {
      StringMap::const_iterator it = m_items.begin();
      for( ; it != m_items.end(); ++it )
      {
        l.push_back( new Disco::Item( m_parent->jid(), (*it).first, (*it).second ) );
      }
    }
    return l;
  }

  Disco::IdentityList Adhoc::handleDiscoNodeIdentities( const JID& /*from*/, const std::string& node )
  {
    Disco::IdentityList l;
    StringMap::const_iterator it = m_items.find( node );
    l.push_back( new Disco::Identity( "automation",
                               node == XMLNS_ADHOC_COMMANDS ? "command-list" : "command-node",
                               it == m_items.end() ? "Ad-Hoc Commands" : (*it).second ) );
    return l;
  }

  bool Adhoc::handleIq( const IQ& iq )
  {
    if( iq.subtype() != IQ::Set )
      return false;

    Tag* c = iq.query();
    if( c && c->name() == "command" )
    {
      const std::string& node = c->findAttribute( "node" );
      AdhocCommandProviderMap::const_iterator it = m_adhocCommandProviders.find( node );
      if( !node.empty() && ( it != m_adhocCommandProviders.end() ) )
      {
        (*it).second->handleAdhocCommand( node, c, iq.from(), iq.id() );
        return true;
      }
    }

    return false;
  }

  void Adhoc::handleIqID( const IQ& iq, int context )
  {
    if( context != Adhoc::Command::Execute || iq.subtype() != IQ::Result )
      return;

    AdhocTrackMap::iterator it = m_adhocTrackMap.find( iq.id() );
    if( it == m_adhocTrackMap.end() || (*it).second.context != context
        || (*it).second.remote != iq.from() )
      return;

    const Adhoc::Command* ac = static_cast<const Adhoc::Command*>( iq.findExtension( ExtAdhocCommand ) );
    if( !ac )
      return;

    m_adhocTrackMap.erase( it );
    (*it).second.ah->handleAdhocExecutionResult( iq.from(), *ac );
  }

  void Adhoc::registerAdhocCommandProvider( AdhocCommandProvider* acp, const std::string& command,
                                            const std::string& name )
  {
    if( !m_parent || !m_parent->disco() )
      return;

    m_parent->disco()->registerNodeHandler( this, command );
    m_adhocCommandProviders[command] = acp;
    m_items[command] = name;
  }

  void Adhoc::handleDiscoInfo( const JID& from, const Disco::Info& info, int context )
  {
    if( context != CheckAdhocSupport )
      return;

    AdhocTrackMap::iterator it = m_adhocTrackMap.begin();
    for( ; it != m_adhocTrackMap.end() && (*it).second.context != context
                                       && (*it).second.remote  != from; ++it )
      ;
    if( it == m_adhocTrackMap.end() )
      return;

    (*it).second.ah->handleAdhocSupport( from, info.hasFeature( XMLNS_ADHOC_COMMANDS ) );
    m_adhocTrackMap.erase( it );
  }

  void Adhoc::handleDiscoItems( const JID& from, const Disco::Items& items, int context )
  {
    if( context != FetchAdhocCommands )
      return;

    AdhocTrackMap::iterator it = m_adhocTrackMap.begin();
    for( ; it != m_adhocTrackMap.end(); ++it )
    {
      if( (*it).second.context == context && (*it).second.remote == from )
      {
        StringMap commands;
        const Disco::ItemList& l = items.items();
        Disco::ItemList::const_iterator it2 = l.begin();
        for( ; it2 != l.end(); ++it2 )
        {
          commands[(*it2)->node()] = (*it2)->name();
        }
        (*it).second.ah->handleAdhocCommands( from, commands );

        m_adhocTrackMap.erase( it );
        break;
      }
    }
  }

  void Adhoc::handleDiscoError( const JID& from, const Error& error, int context )
  {
    AdhocTrackMap::iterator it = m_adhocTrackMap.begin();
    for( ; it != m_adhocTrackMap.end(); ++it )
    {
      if( (*it).second.context == context && (*it).second.remote == from )
      {
        (*it).second.ah->handleAdhocError( from, error );

        m_adhocTrackMap.erase( it );
      }
    }
  }

  void Adhoc::checkSupport( const JID& remote, AdhocHandler* ah )
  {
    if( !remote || !ah || !m_parent || !m_parent->disco() )
      return;

    TrackStruct track;
    track.remote = remote;
    track.context = CheckAdhocSupport;
    track.ah = ah;
    const std::string& id = m_parent->getID();
    m_adhocTrackMap[id] = track;
    m_parent->disco()->getDiscoInfo( remote, EmptyString, this, CheckAdhocSupport, id );
  }

  void Adhoc::getCommands( const JID& remote, AdhocHandler* ah )
  {
    if( !remote || !ah || !m_parent || !m_parent->disco() )
      return;

    TrackStruct track;
    track.remote = remote;
    track.context = FetchAdhocCommands;
    track.ah = ah;
    const std::string& id = m_parent->getID();
    m_adhocTrackMap[id] = track;
    m_parent->disco()->getDiscoItems( remote, XMLNS_ADHOC_COMMANDS, this, FetchAdhocCommands, id );
  }

  void Adhoc::execute( const JID& remote, Adhoc::Command* command, AdhocHandler* ah )
  {
    if( !remote || !command || !m_parent || !ah )
      return;

    const std::string& id = m_parent->getID();
    IQ iq( IQ::Set, remote, id );
    iq.addExtension( command );

    TrackStruct track;
    track.remote = remote;
    track.context = ExecuteAdhocCommand;
    track.ah = ah;
    m_adhocTrackMap[id] = track;

    m_parent->send( iq, this, ExecuteAdhocCommand );
  }

  void Adhoc::removeAdhocCommandProvider( const std::string& command )
  {
    if( !m_parent || !m_parent->disco() )
      return;

    m_parent->disco()->removeNodeHandler( this, command );
    m_adhocCommandProviders.erase( command );
    m_items.erase( command );
  }

}
