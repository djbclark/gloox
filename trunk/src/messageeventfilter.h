/*
  Copyright (c) 2005 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef MESSAGEVENTFILTER_H__
#define MESSAGEVENTFILTER_H__

#include "messagesession.h"
#include "messagehandler.h"

namespace gloox
{

  class Tag;
  class MessageEventHandler;

  /**
   * @brief This class adds Message Event (JEP-0022) support to a MessageSession.
   *
   * This implementation of Message Events is fully transparent to the user of the class.
   * If the remote entity does not request message events, MessageEventFilter will not send
   * any, even if the user requests it. (This is required by the protocol specification.)
   *
   * @author Jakob Schroeter <js@camaya.net>
   * @since 0.8
   */
  class MessageEventFilter
  {
    public:
      /**
       * Contstructs a new Message Event filter for a MessageSession.
       * You should use the newly created decorator and forget about the old
       * MessageSession.
       * @param parent The MessageSession to decorate.
       * @param defaultEvents Bit-wise ORed MessageEventType's which shall be requested
       * for every message sent. Default: all.
       */
      MessageEventFilter( MessageSession *parent,
                             int defaultEvents = MESSAGE_EVENT_OFFLINE | MESSAGE_EVENT_DELIVERED
                                               | MESSAGE_EVENT_DISPLAYED | MESSAGE_EVENT_COMPOSING );

      /**
       * Virtual destructor.
       */
      virtual ~MessageEventFilter();

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
      void raiseMessageEvent( MessageEventType event );

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
       * @copydoc MessageSession::registerMessageHandler().
       * @param mh The MessageHandler to register.
       */
      void registerMessageHandler( MessageHandler *mh );

      /**
       * @copydoc MessageSession::removeMessageHandler().
       */
      void removeMessageHandler();

      /**
       * Adds Message Event request tags to the given Tag.
       * @param tag The tag to decorate.
       */
      virtual void decorate( Tag *tag );

      // reimplemented from MessageHandler
      virtual void handleMessage( Stanza *stanza );

    private:
      MessageSession *m_parent;
      MessageEventHandler *m_messageEventHandler;
      std::string m_lastID;
      int m_requestedEvents;
      int m_defaultEvents;
      MessageEventType m_lastSent;

  };

}

#endif // MESSAGEVENTFILTER_H__
