/*
  Copyright (c) 2005 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef MESSAGESESSION_H__
#define MESSAGESESSION_H__

#include "messagehandler.h"
#include "jid.h"

#include <string>

namespace gloox
{

  class ClientBase;
  class Tag;
  class MessageEventFilter;
  class MessageEventHandler;

  /**
   * @brief An abstraction of a message session between any two entities.
   *
   * This is an alternative interface to XMPP messaging. The original interface, using the simple
   * MessageHandler-derived interface, is based on an all-or-nothing approach. Once registered with
   * ClientBase, a handler receives all message stanzas sent to this client and has to do any filtering
   * on its own.
   *
   * MessageSession adds an abstraction to a chat conversation. A MessageSession is responsible for
   * communicating with exactly one (full) JID. It is extensible by arbitrary decorators which,
   * for instance, offer Message Events support for messages sent through it. Message Events are
   * implemented in the MessageEventDecorator.
   *
   * You can still use the old MessageHandler in parallel, but messages will not be relayed to both
   * the generic MessageHandler and a MessageSession established for the sender's JID. The MessageSession
   * takes precedence.
   *
   * @author Jakob Schroeter <js@camaya.net>
   * @since 0.8
   */
  class GLOOX_EXPORT MessageSession : public MessageHandler
  {
    public:
      /**
       * Constructs a new MessageSession for the given JID.
       * It is recommended to supply a full JID, in other words, it should have set a resource.
       * No resource can lead to unexpected behavior. A thread ID is generated and sent along
       * with every message sent through this session.
       * @param parent The ClientBase to use for communication.
       * @param jid The remote contact's full JID.
       */
      MessageSession( ClientBase *parent, const JID& jid );

      /**
       * Virtual destructor.
       */
      virtual ~MessageSession();

      /**
       * This function clears the internal pointer to the MessageHandler and therefore
       * disables message delivery.
       */
      void removeMessageHandler();

      /**
       * Use this function to find out where is this session pointed at.
       * @return The receipient's JID.
       */
      const JID& target() const { return m_target; };

      /**
       * By default, a thread ID is sent with every message to identify
       * messages belonging together.
       * @returns The thread ID for this session.
       */
      const std::string& threadID() const { return m_thread; };

      /**
       * Use this function to associate a MessageHandler with this MessageSession.
       * The MessageHandler will receive all messages sent from this MessageSession's
       * remote contact.
       * @param mh The MessageHandler to register.
       */
      virtual void registerMessageHandler( MessageHandler *mh );

       /**
       * The MessageEventHandler registered here will receive Message Events according
       * to JEP-0022.
       * @param meh The MessageEventHandler to register.
        */
      void registerMessageEventHandler( MessageEventHandler *meh );

      /**
       * This function clears the internal pointer to the MessageEventHandler.
       * Message Events will not be delivered anymore after calling this function until another
       * MessageEventHandler is registered.
       */
      void removeMessageEventHandler();

      /**
       * A wrapper around ClientBase::send().
       * @param tag A Tag to send.
       */
      virtual void send( Tag *tag );

      /**
       * A convenience function
       */
      void send( const std::string& message, const std::string& subject );

      /**
       * Use this function to raise an event as defined in JEP-0022.
       * @note The Spec states that Message Events shall not be sent to an entity
       * which did not request them. Reasonable effort is taken in this function to
       * avoid spurious event sending. You should be safe to call this even if Message
       * Events were not requested by the remote entity. However,
       * calling raiseEvent( MESSAGE_EVENT_COMPOSING ) for every keystroke still is
       * discouraged. ;)
       * @param event The event to raise.
       */
      void raiseEvent( MessageEventType event );

      // reimplemented from MessageHandler
      virtual void handleMessage( Stanza *stanza );

    private:
      ClientBase *m_parent;
      MessageEventFilter *m_eventFilter;
      JID m_target;
      MessageHandler *m_messageHandler;
      std::string m_thread;

  };

}

#endif // MESSAGESESSION_H__
