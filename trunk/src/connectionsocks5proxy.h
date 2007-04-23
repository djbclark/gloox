/*
  Copyright (c) 2004-2007 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef CONNECTIONSOCKS5PROXY_H__
#define CONNECTIONSOCKS5PROXY_H__

#include "gloox.h"
#include "connectionbase.h"
#include "logsink.h"

#include <string>

namespace gloox
{

  /**
   * @brief This is an implementation of a simple SOCKS5 Proxying connection (RFC 1928 + RFC 1929).
   *
   * To use with a SOCKS5 proxy:
   *
   * @code
   * Client *c = new Client( ... );
   * c->setConnectionImpl(
   *       new ConnectionSOCKS5Proxy( c,
   *                                new ConnectionTCP( c->logInstance(),
   *                                                   proxyHost, proxyPort ),
   *                                c->logInstance(), xmppHost, xmppPort ) );
   * @endcode
   *
   * Make sure to pass the proxy host/port to the transport connection (ConnectionTCP in this case),
   * and the XMPP host/port to the proxy connection.
   *
   * The reason why ConnectionSOCKS5Proxy doesn't manage its own ConnectionTCP is that it allows it
   * to be used with other transports (like IPv6 or chained HTTP/SOCKS5 proxies).
   *
   * @note This class is also used by the SOCKS5 bytestreams implementation (with slightly different
   * semantics).
   *
   * @author Jakob Schroeter <js@camaya.net>
   * @since 0.9
   */
  class GLOOX_API ConnectionSOCKS5Proxy : public ConnectionBase, ConnectionDataHandler
  {
    public:
      /**
       * Constructs a new ConnectionSOCKS5Proxy object.
       * @param cdh An ConnectionDataHandler-derived object that will handle incoming data.
       * @param connection A transport connection. It should be configured to connect to
       * the proxy host and port, @b not to the (XMPP) host. ConnectionSOCKS5Proxy will own the
       * transport connection and delete it in its destructor.
       * @param logInstance The log target. Obtain it from ClientBase::logInstance().
       * @param server A server to connect to. This is the XMPP server's address, @b not the proxy.
       * @param port The proxy's port to connect to. This is the (XMPP) server's port, @b not the proxy's.
       * The default of -1 means that SRV records will be used to find out about the actual host:port.
       * @param ip Indicates whether @c server is an IP address (true) or a host name (false).
       */
      ConnectionSOCKS5Proxy( ConnectionDataHandler *cdh, ConnectionBase *connection,
                             const LogSink& logInstance,
                             const std::string& server, int port = -1, bool ip = false );

      /**
       * Virtual destructor
       */
      virtual ~ConnectionSOCKS5Proxy();

      // reimplemented from ConnectionBase
      virtual ConnectionError connect();

      // reimplemented from ConnectionBase
      virtual ConnectionError recv( int timeout = -1 );

      // reimplemented from ConnectionBase
      virtual bool send( const std::string& data );

      // reimplemented from ConnectionBase
      virtual ConnectionError receive();

      // reimplemented from ConnectionBase
      virtual void disconnect();

      // reimplemented from ConnectionBase
      virtual void cleanup();

      // reimplemented from ConnectionBase
      virtual void getStatistics( int &totalIn, int &totalOut );

      // reimplemented from ConnectionDataHandler
      virtual void handleReceivedData( const std::string& data );

      // reimplemented from ConnectionDataHandler
      virtual void handleConnect();

      // reimplemented from ConnectionDataHandler
      virtual void handleDisconnect( ConnectionError reason );

      // reimplemented from ConnectionDataHandler
      virtual ConnectionSOCKS5Proxy* newInstance() const;

      /**
       * Sets the server to proxy to.
       * @param host The server hostname (IP address).
       * @param port The server port. The default of -1 means that SRV records will be used
       * to find out about the actual host:port.
       * @param ip Indicates whether @c host is an IP address (true) or a host name (false).
       */
      void setServer( const std::string& host, unsigned short port = -1, bool ip )
        { m_server = host; m_port = port; m_ip = ip; }

      /**
       * Sets proxy authorization credentials.
       * @param user The user name to use for proxy authorization.
       * @param password The password to use for proxy authorization.
       */
      void setProxyAuth( const std::string& user, const std::string& password )
        { m_proxyUser = user; m_proxyPassword = password; }

      /**
       * Sets the underlying transport connection. A possibly existing connection will be deleted.
       * @param connection The ConnectionBase to replace the current connection, if any.
       */
      void setConnection( ConnectionBase* connection );

   private:
     enum Socks5State
     {
       S5StateDisconnected,
       S5StateConnecting,
       S5StateNegotiating,
       S5StateAuthenticating,
       S5StateConnected
     };

      ConnectionSOCKS5Proxy &operator= ( const ConnectionSOCKS5Proxy& );
      void negotiate();

      ConnectionBase* m_connection;
      const LogSink& m_logInstance;

      Socks5State m_s5state;

      std::string m_proxyUser;
      std::string m_proxyPassword;
      std::string m_proxyHandshakeBuffer;
      bool m_ip;

  };

}

#endif // CONNECTIONSOCKS5PROXY_H__
