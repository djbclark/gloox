/*
  Copyright (c) 2004-2005 by Jakob Schroeter <js@camaya.net>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/


#ifndef FEEDER_H__
#define FEEDER_H__

#include "pollhandler.h"
#include "infohandler.h"
#include "resulthandler.h"

#include "../jlib/connectionlistener.h"
#include "../jlib/iqhandler.h"
#include "../jlib/rosterlistener.h"
#include "../jlib/rosterhelper.h"

#include "../common/common.h"

#include <iksemel.h>

#include <map>
#include <string>
using namespace std;

class JClient;

/**
 * This is the main class of the Feeder.
 * @author Jakob Schroeter <js@camaya.net>
 */
class Feeder : public ConnectionListener, IqHandler, RosterListener
{
  public:
    /**
     * Constructor.
     * Creates a new Feeder, registers IqHandler, ConnectionListener and
     * RosterListener and connects to the XMPP server.
     * @param id The full JID.
     * @param password The password to use for authentication.
     * @param debug Turn debug of the jabber client on. Default: true
     * @param port The port to connect to. Default: 5222
     */
    Feeder( const string& id, const string& password, bool debug = true, int port = 5222 );

    /**
     * Virtual destructor.
     */
    virtual ~Feeder();

    /**
     * Initiates the connection to the XMPP server.
     */
    void connect();

    /**
     * Use this function to push data. If a Worker is available, data is pushed.
     * @param data is passed to it for processing
     * @return Returns false if no Worker is available. True otherwise.
     */
    bool push( const char* data );

    /**
     * Using this method you can register an object as poll handler. This object is
     * polled for data to be sent to an available worker.
     * @param ph The object derived from PollHandler.
     */
    void registerPollHandler( PollHandler* ph );

    /**
     * Using this method you can register an object as info handler. The methods
     * reimplemented from get called on various events. See the documentation of 
     * the InfoHandler interface for more information.
     * @param ih The object derived from InfoHandlerFeeder.
     */
    void registerInfoHandler( InfoHandlerFeeder* ih );

    /**
     * Using this method you can register an object as ResultHandler.
     * @param rh The object derived from ResultHandler.
     */
    void registerResultHandler( ResultHandler* rh );

    // reimplemented from RosterListener
    virtual void itemAdded( const string& jid );

    // reimplemented from RosterListener
    virtual void itemRemoved( const string& jid );

    // reimplemented from RosterListener
    virtual void roster( RosterHelper::RosterMap roster );

    // reimplemented from RosterListener
    virtual void itemChanged( const string& jid, int status, const char* msg );

    // reimplemented from RosterListener
    virtual bool subscriptionRequest( const string& jid, const char* msg );

    // reimplemented from IqHandler.
    virtual void handleIq( const char* xmlns, ikspak* pak );

    // reimplemented from ConnectionListener.
    virtual void onConnect();

    // reimplemented from ConnectionListener.
    virtual void onDisconnect();

  private:
    struct workerInfo
    {
      string jid;
      bool worker;
    };

    void sendData( const string& jid );

    JClient* c;
    PollHandler* m_pollHandler;
    InfoHandlerFeeder* m_infoHandler;
    ResultHandler* m_resultHandler;

    bool m_poll;
    bool m_debug;

};

#endif // FEEDER_H__
