/*
  Copyright (c) 2004-2007 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#include "clientbase.h"
#include "rostermanager.h"
#include "disco.h"
#include "rosteritem.h"
#include "rosterlistener.h"
#include "privatexml.h"


namespace gloox
{

  RosterManager::RosterManager( ClientBase *parent )
    : m_rosterListener( 0 ), m_parent( parent ), m_privateXML( 0 ),
      m_syncSubscribeReq( false )
  {
    if( m_parent )
    {
      m_parent->registerIqHandler( this, XMLNS_ROSTER );
      m_parent->registerPresenceHandler( this );
      m_parent->registerSubscriptionHandler( this );

      m_self = new RosterItem( m_parent->jid().bare() );

      m_privateXML = new PrivateXML( m_parent );
    }
  }

  RosterManager::~RosterManager()
  {
    if( m_parent )
    {
      m_parent->removeIqHandler( XMLNS_ROSTER );
      m_parent->removePresenceHandler( this );
      m_parent->removeSubscriptionHandler( this );
      delete m_self;
      delete m_privateXML;
    }

    Roster::iterator it = m_roster.begin();
    for( ; it != m_roster.end(); ++it )
      delete (*it).second;
    m_roster.clear();
  }

  Roster* RosterManager::roster()
  {
    return &m_roster;
  }

  void RosterManager::fill()
  {
    m_privateXML->requestXML( "roster", XMLNS_ROSTER_DELIMITER, this );

    Tag *iq = new Tag( "iq" );
    iq->addAttribute( "type", "get" );
    iq->addAttribute( "id", m_parent->getID() );
    Tag *q = new Tag( iq, "query" );
    q->addAttribute( "xmlns", XMLNS_ROSTER );
    m_parent->send( iq );
  }

  bool RosterManager::handleIq( IQ* iq )
  {
    if( iq->subtype() == IQ::IqTypeResult ) // initial roster
    {
      extractItems( iq, false );

      if( m_rosterListener )
        m_rosterListener->handleRoster( m_roster );

      m_parent->rosterFilled();

      return true;
    }
    else if( iq->subtype() == IQ::IqTypeSet ) // roster item push
    {
      extractItems( iq, true );

      Tag *re = new Tag( "iq" );
      re->addAttribute( "id", iq->id() );
      re->addAttribute( "type", "result" );
      m_parent->send( re );

      return true;
    }
    else if( iq->subtype() == IQ::IqTypeError )
    {
      if( m_rosterListener )
        m_rosterListener->handleRosterError( iq );
    }

    return false;
  }

  void RosterManager::handleIqID( IQ* /*iq*/, int /*context*/ )
  {
  }

  void RosterManager::handlePresence( Presence* presence )
  {
    if( presence->subtype() == Presence::PresenceError )
      return;

    StringList caps;
    const Tag::TagList& l = presence->children();
    Tag::TagList::const_iterator it_c = l.begin();
    for( ; it_c != l.end(); ++it_c )
    {
      if( (*it_c)->name() == "c" )
      {
        std::string cap;
        cap.append( (*it_c)->findAttribute( "node" ).c_str() );
        cap.append( "#" );
        cap.append( (*it_c)->findAttribute( "ver" ).c_str() );
        if( (*it_c)->findAttribute( "ext" ).size ())
        {
          cap.append( "#" );
          cap.append( (*it_c)->findAttribute( "ext" ).c_str() );
        }
        caps.push_back( cap );
      }
    }

    Roster::iterator it = m_roster.find( presence->from().bare() );
    if( it != m_roster.end() )
    {
      if( presence->presence() == Presence::PresenceUnavailable )
        (*it).second->removeResource( presence->from().resource() );
      else
      {
        (*it).second->setPresence( presence->from().resource(), presence->presence() );
        (*it).second->setStatus( presence->from().resource(), presence->status() );
        (*it).second->setPriority( presence->from().resource(), presence->priority() );
  //       (*it).second->setCaps ( caps );
      }

      if( m_rosterListener )
        m_rosterListener->handleRosterPresence( (*(*it).second), presence->from().resource(),
                                                presence->presence(), presence->status() );
    }
    else if( presence->from().bare() == m_self->jid() )
    {
      if( presence->presence() == Presence::PresenceUnavailable )
        m_self->removeResource( presence->from().resource() );
      else
      {
        m_self->setPresence( presence->from().resource(), presence->presence() );
        m_self->setStatus( presence->from().resource(), presence->status() );
        m_self->setPriority( presence->from().resource(), presence->priority() );
  //       (*it).second->setCaps ( caps );
      }

      if( m_rosterListener )
        m_rosterListener->handleSelfPresence( *m_self, presence->from().resource(),
                                               presence->presence(), presence->status() );
    }
    else
    {
      if( m_rosterListener )
        m_rosterListener->handleNonrosterPresence( presence );
    }
  }

  void RosterManager::subscribe( const JID& jid, const std::string& name,
                                 const StringList& groups, const std::string& msg )
  {
    if( jid.empty() )
      return;

    add( jid, name, groups );

    Tag *s = new Tag( "presence" );
    s->addAttribute( "type", "subscribe" );
    s->addAttribute( "to", jid.bare() );
    s->addAttribute( "from", m_parent->jid().full() );
    if( !msg.empty() )
      new Tag( s, "status", msg );

    m_parent->send( s );
  }


  void RosterManager::add( const JID& jid, const std::string& name, const StringList& groups )
  {
    if( jid.empty() )
      return;

    const std::string& id = m_parent->getID();

    Tag *iq = new Tag( "iq" );
    iq->addAttribute( "type", "set" );
    iq->addAttribute( "id", id );
    Tag *q = new Tag( iq, "query" );
    q->addAttribute( "xmlns", XMLNS_ROSTER );
    Tag *i = new Tag( q, "item" );
    i->addAttribute( "jid", jid.bare() );
    if( !name.empty() )
      i->addAttribute( "name", name );

    if( groups.size() != 0 )
    {
      StringList::const_iterator it = groups.begin();
      for( ; it != groups.end(); ++it )
        new Tag( i, "group", (*it) );
    }

    m_parent->send( iq );
  }

  void RosterManager::unsubscribe( const JID& jid, const std::string& msg )
  {
    Tag *s = new Tag( "presence" );
    s->addAttribute( "type", "unsubscribe" );
    s->addAttribute( "from", m_parent->jid().bare() );
    s->addAttribute( "to", jid.bare() );
    if( !msg.empty() )
      new Tag( s, "status", msg );

    m_parent->send( s );

  }

  void RosterManager::cancel( const JID& jid, const std::string& msg )
  {
    Tag *s = new Tag( "presence" );
    s->addAttribute( "type", "unsubscribed" );
    s->addAttribute( "from", m_parent->jid().bare() );
    s->addAttribute( "to", jid.bare() );
    if( !msg.empty() )
      new Tag( s, "status", msg );

    m_parent->send( s );

  }

  void RosterManager::remove( const JID& jid )
  {
    const std::string& id = m_parent->getID();

    Tag *iq = new Tag( "iq" );
    iq->addAttribute( "type", "set" );
    iq->addAttribute( "id", id );
    Tag *q = new Tag( iq, "query" );
    q->addAttribute( "xmlns", XMLNS_ROSTER );
    Tag *i = new Tag( q, "item" );
    i->addAttribute( "jid", jid.bare() );
    i->addAttribute( "subscription", "remove" );

    m_parent->send( iq );
  }

  void RosterManager::synchronize()
  {
    Roster::const_iterator it = m_roster.begin();
    for( ; it != m_roster.end(); ++it )
    {
      if( (*it).second->changed() )
      {
        const std::string& id = m_parent->getID();

        Tag *iq = new Tag( "iq" );
        iq->addAttribute( "type", "set" );
        iq->addAttribute( "id", id );
        Tag *q = new Tag( iq, "query" );
        q->addAttribute( "xmlns", XMLNS_ROSTER );
        Tag *i = new Tag( q, "item" );
        i->addAttribute( "jid", (*it).second->jid() );
        if( !(*it).second->name().empty() )
          i->addAttribute( "name", (*it).second->name() );

        if( (*it).second->groups().size() != 0 )
        {
          StringList::const_iterator g_it = (*it).second->groups().begin();
          for( ; g_it != (*it).second->groups().end(); ++g_it )
            new Tag( i, "group", (*g_it) );
        }

        m_parent->send( iq );
      }
    }
  }

  void RosterManager::ackSubscriptionRequest( const JID& to, bool ack )
  {
    Tag *p = new Tag( "presence" );
    if( ack )
      p->addAttribute( "type", "subscribed" );
    else
      p->addAttribute( "type", "unsubscribed" );

    p->addAttribute( "from", m_parent->jid().bare() );
    p->addAttribute( "to", to.bare() );
    m_parent->send( p );
  }

  void RosterManager::handleSubscription( Subscription* s10n )
  {
    if( !m_rosterListener )
      return;

    switch( s10n->subtype() )
    {
      case Subscription::S10nSubscribe:
      {
        bool answer = m_rosterListener->handleSubscriptionRequest( s10n->from(), s10n->status() );
        if( m_syncSubscribeReq )
        {
          ackSubscriptionRequest( s10n->from(), answer );
        }
        break;
      }
      case Subscription::S10nSubscribed:
      {
//         Tag *p = new Tag( "presence" );
//         p->addAttribute( "type", "subscribe" );
//         p->addAttribute( "from", m_parent->jid().bare() );
//         p->addAttribute( "to", stanza->from().bare() );
//         m_parent->send( p );

        m_rosterListener->handleItemSubscribed( s10n->from() );
        break;
      }

      case Subscription::S10nUnsubscribe:
      {
        Tag *p = new Tag( "presence" );
        p->addAttribute( "type", "unsubscribed" );
        p->addAttribute( "from", m_parent->jid().bare() );
        p->addAttribute( "to", s10n->from().bare() );
        m_parent->send( p );

        bool answer = m_rosterListener->handleUnsubscriptionRequest( s10n->from(), s10n->status() );
        if( m_syncSubscribeReq && answer )
          remove( s10n->from().bare() );
        break;
      }

      case Subscription::S10nUnsubscribed:
      {
//         Tag *p = new Tag( "presence" );
//         p->addAttribute( "type", "unsubscribe" );
//         p->addAttribute( "from", m_parent->jid().bare() );
//         p->addAttribute( "to", stanza->from().bare() );
//         m_parent->send( p );

        m_rosterListener->handleItemUnsubscribed( s10n->from() );
        break;
      }

      default:
        break;
    }
  }

  void RosterManager::registerRosterListener( RosterListener *rl, bool syncSubscribeReq )
  {
    m_syncSubscribeReq = syncSubscribeReq;
    m_rosterListener = rl;
  }

  void RosterManager::removeRosterListener()
  {
    m_syncSubscribeReq = false;
    m_rosterListener = 0;
  }

  void RosterManager::extractItems( Tag *tag, bool isPush )
  {
    Tag *t = tag->findChild( "query" );
    const Tag::TagList& l = t->children();
    Tag::TagList::const_iterator it = l.begin();
    for( ; it != l.end(); ++it )
    {
      if( (*it)->name() == "item" )
      {
        StringList gl;
        if( (*it)->hasChild( "group" ) )
        {
          const Tag::TagList& g = (*it)->children();
          Tag::TagList::const_iterator it_g = g.begin();
          for( ; it_g != g.end(); ++it_g )
          {
            gl.push_back( (*it_g)->cdata() );
          }
        }

        const JID& jid = (*it)->findAttribute( "jid" );
        Roster::iterator it_d = m_roster.find( jid.bare() );
        if( it_d != m_roster.end() )
        {
          (*it_d).second->setName( (*it)->findAttribute( "name" ) );
          const std::string& sub = (*it)->findAttribute( "subscription" );
          if( sub == "remove" )
          {
            delete (*it_d).second;
            m_roster.erase( it_d );
            if( m_rosterListener )
              m_rosterListener->handleItemRemoved( jid );
            continue;
          }
          const std::string& ask = (*it)->findAttribute( "ask" );
          bool a = false;
          if( !ask.empty() )
            a = true;
          (*it_d).second->setSubscription( sub, a );
          (*it_d).second->setGroups( gl );
          (*it_d).second->setSynchronized();

          if( isPush && m_rosterListener )
            m_rosterListener->handleItemUpdated( jid );
        }
        else
        {
          const std::string& sub = (*it)->findAttribute( "subscription" );
          if( sub == "remove" )
            continue;
          const std::string& name = (*it)->findAttribute( "name" );
          const std::string& ask = (*it)->findAttribute( "ask" );
          bool a = false;
          if( !ask.empty() )
            a = true;

          StringList caps;
          add( jid.bare(), name, gl, caps, sub, a );
          if( isPush && m_rosterListener )
            m_rosterListener->handleItemAdded( jid );
        }
      }
    }
  }

  void RosterManager::add( const std::string& jid, const std::string& name,
                           const StringList& groups, const StringList& caps,
                           const std::string& sub, bool ask )
  {
    if( m_roster.find( jid ) == m_roster.end() )
      m_roster[jid] = new RosterItem( jid, name );

    m_roster[jid]->setSubscription( sub, ask );
    m_roster[jid]->setGroups( groups );
//     m_roster[jid]->setCaps( caps );
    m_roster[jid]->setSynchronized();
  }

  void RosterManager::setDelimiter( const std::string& delimiter )
  {
    m_delimiter = delimiter;
    Tag *t = new Tag( "roster", m_delimiter );
    t->addAttribute( "xmlns", XMLNS_ROSTER_DELIMITER );
    m_privateXML->storeXML( t, this );
  }

  void RosterManager::handlePrivateXML( const std::string& /*tag*/, Tag *xml )
  {
    m_delimiter = xml->cdata();
  }

  void RosterManager::handlePrivateXMLResult( const std::string& /*uid*/, PrivateXMLResult /*result*/ )
  {
  }

  RosterItem* RosterManager::getRosterItem( const JID& jid )
  {
    Roster::const_iterator it = m_roster.find( jid.bare() );
    if( it != m_roster.end() )
      return (*it).second;
    else
      return 0;
  }

}
