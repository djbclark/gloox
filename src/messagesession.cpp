/*
  Copyright (c) 2005 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#include "messagesession.h"

#include "messageeventhandler.h"
#include "messageeventfilter.h"
#include "chatstatefilter.h"
#include "chatstatehandler.h"
#include "clientbase.h"
#include "client.h"
#include "disco.h"
#include "stanza.h"
#include "tag.h"

namespace gloox
{

  MessageSession::MessageSession( ClientBase *parent, const JID& jid )
    : m_parent( parent ), m_eventFilter( 0 ), m_stateFilter( 0 ), m_target( jid ),
      m_messageHandler( 0 ), m_enableMessageEvents( true ), m_enableChatStates( true )
  {
    if( m_parent )
      m_parent->registerMessageHandler( m_target.full(), this );

    m_thread = "gloox" + m_parent->getID();

    m_eventFilter = new MessageEventFilter( this );
    m_stateFilter = new ChatStateFilter( this );

    Client *c = dynamic_cast<Client *>( m_parent );
    if( c )
      c->disco()->addFeature( XMLNS_CHAT_STATES );

  }

  MessageSession::~MessageSession()
  {
    if( m_parent )
      m_parent->removeMessageHandler( m_target.full() );

    delete m_stateFilter;
    delete m_eventFilter;
  }

  void MessageSession::handleMessage( Stanza *stanza )
  {
    if( m_enableMessageEvents && m_eventFilter )
      m_eventFilter->filter( stanza );
    if( m_enableChatStates && m_stateFilter )
      m_stateFilter->filter( stanza );

    if( !m_messageHandler || stanza->body().empty() )
      return;
    else
      m_messageHandler->handleMessage( stanza );
  }

  void MessageSession::send( const std::string& message, const std::string& subject )
  {
    Tag *m = new Tag( "message" );
    m->addAttribute( "type", "chat" );
    new Tag( m, "body", message );
    if( !subject.empty() )
      new Tag( m, "subject", subject );

    m->addAttribute( "from", m_parent->jid().full() );
    m->addAttribute( "to", m_target.full() );
    new Tag( m, "thread", m_thread );

    if( m_eventFilter )
      m_eventFilter->decorate( m );

    if( m_stateFilter )
      m_stateFilter->decorate( m );

    m_parent->send( m );
  }

  void MessageSession::send( Tag *tag )
  {
    m_parent->send( tag );
  }

  void MessageSession::setFilter( int filters, bool enable )
  {
    if( filters & FilterMessageEvents )
    {
      if( enable )
        m_enableMessageEvents = true;
      else
        m_enableMessageEvents = false;
    }

    if( filters & FilterChatStates )
    {
      if( enable )
        m_enableChatStates = true;
      else
        m_enableChatStates = false;
    }
  }

  void MessageSession::raiseMessageEvent( MessageEventType event )
  {
    if( m_eventFilter )
    {
      m_eventFilter->raiseMessageEvent( event );
    }
  }

  void MessageSession::setChatState( ChatStateType state )
  {
    if( m_stateFilter )
    {
      m_stateFilter->setChatState( state );
    }
  }

  void MessageSession::registerMessageHandler( MessageHandler *mh )
  {
    m_messageHandler = mh;
  }

  void MessageSession::removeMessageHandler()
  {
    m_messageHandler = 0;
  }

  void MessageSession::registerMessageEventHandler( MessageEventHandler *meh )
  {
    if( m_eventFilter )
      m_eventFilter->registerMessageEventHandler( meh );
  }

  void MessageSession::removeMessageEventHandler()
  {
    if( m_eventFilter )
      m_eventFilter->removeMessageEventHandler();
  }

  void MessageSession::registerChatStateHandler( ChatStateHandler *csh )
  {
    if( m_stateFilter )
      m_stateFilter->registerChatStateHandler( csh );
  }

  void MessageSession::removeChatStateHandler()
  {
    if( m_stateFilter )
      m_stateFilter->removeChatStateHandler();
  }

}
