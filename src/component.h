/*
  Copyright (c) 2005-2007 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/



#ifndef COMPONENT_H__
#define COMPONENT_H__

#include "clientbase.h"

#include <string>

namespace gloox
{

  /**
   * @brief This is an implementation of a basic jabber Component.
   *
   * It's using XEP-0114 (Jabber Component Protocol) to authenticate with a server.
   *
   * @author Jakob Schroeter <js@camaya.net>
   * @since 0.3
   */
  class GLOOX_API Component : public ClientBase
  {
    public:
      /**
       * Constructs a new Component.
       * @param ns The namespace that qualifies the stream. Either @b jabber:component:accept or
       * @b jabber:component:connect. See XEP-0114 for details.
       * @param server The server to connect to.
       * @param component The component's hostname. FQDN.
       * @param password The component's password.
       * @param port The port to connect to. The default of 5347 is the default port of the router
       * in jabberd2.
       */
      Component( const std::string& ns, const std::string& server,
                 const std::string& component, const std::string& password, int port = 5347 );

      /**
       * Virtual Destructor.
       */
      virtual ~Component() {}

      /**
       * Disconnects from the server.
       */
      void disconnect();

    protected:
      virtual void handleStartNode();
      virtual bool handleNormalNode( Stanza *stanza );
      virtual bool checkStreamVersion( const std::string& /*version*/ ) { return true; }
      virtual void disconnect( ConnectionError reason = ConnUserDisconnected )
        { ClientBase::disconnect( reason ); }

    private:
      // reimplemented from ClientBase
      virtual void rosterFilled() {}

  };

}

#endif // COMPONENT_H__
