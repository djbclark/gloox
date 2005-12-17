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
#include "clientbase.h"
#include "tag.h"

namespace gloox
{

  MessageSession::MessageSession( ClientBase *parent, const JID& jid )
    : m_parent( parent ), m_target( jid ), m_messageHandler( 0 )
  {
    if( m_parent )
      m_parent->registerMessageHandler( m_target.full(), this );

    m_thread = "gloox" + m_parent->getID();
  }

  MessageSession::~MessageSession()
  {
    if( m_parent )
      m_parent->removeMessageHandler( m_target.full() );
  }

  void MessageSession::handleMessage( Stanza *stanza )
  {
    if( !m_messageHandler || stanza->body().empty() )
      return;
    else
      m_messageHandler->handleMessage( stanza );
  }

  void MessageSession::send( Tag *tag )
  {
    tag->addAttribute( "from", m_parent->jid().full() );
    new Tag( tag, "thread", m_thread );
    m_parent->send( tag );
  }

  void MessageSession::registerMessageHandler( MessageHandler *mh )
  {
    m_messageHandler = mh;
  }

  void MessageSession::removeMessageHandler()
  {
    m_messageHandler = 0;
  }

}
