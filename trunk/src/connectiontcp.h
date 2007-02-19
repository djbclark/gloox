/*
  Copyright (c) 2004-2007 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef TCPCONNECTION_H__
#define TCPCONNECTION_H__

#include "gloox.h"
#include "connectionbase.h"
#include "logsink.h"

#include <string>

namespace gloox
{

  /**
   * @brief This is an implementation of a simple TCP connection.
   *
   * @author Jakob Schroeter <js@camaya.net>
   * @since 0.9
   */
  class GLOOX_API ConnectionTCP : public ConnectionBase
  {
    public:
      /**
       * Constructs a new ConnectionTCP object.
       * You should not need to use this function directly.
       * @param server A server to connect to.
       * @param port The port to connect to. The default of -1 means that SRV records will be used
       * to find out about the actual host:port.
       */
      ConnectionTCP( ConnectionDataHandler *cdh, const LogSink& logInstance,
                     const std::string& server, unsigned short port = -1 );

      /**
       * Virtual destructor
       */
      virtual ~ConnectionTCP();

      /**
       * Used to initiate the connection.
       * @return Returns the connection state.
       */
      virtual ConnectionError connect();

      /**
       * Use this periodically to receive data from the socket and to feed the parser.
       * @param timeout The timeout to use for select in microseconds. Default of -1 means blocking.
       * @return The state of the connection.
       */
      virtual ConnectionError recv( int timeout = -1 );

      /**
       * Use this function to send a string of data over the wire. The function returns only after
       * all data has been sent.
       * @param data The data to send.
       */
      virtual bool send( const std::string& data );

      /**
       * Use this function to put the connection into 'receive mode'.
       * @return Returns a value indicating the disconnection reason.
       */
      virtual ConnectionError receive();

      /**
       * Disconnects an established connection. NOOP if no active connection exists.
       * @param e A ConnectionError decribing why the connection is terminated. Well, its not really an
       * error here, but...
       */
      virtual void disconnect( ConnectionError e );

      /**
       * Gives access to the raw file descriptor of a connection. Use it wisely. Especially, you should not
       * ::recv() any data from it. There is no way to feed that back into the parser. You can
       * select()/poll() it and use ConnectionTCP::recv( -1 ) to fetch the data.
       * @return The file descriptor of the active connection, or -1 if no connection is established.
       */
      int fileDescriptor();

      /**
       * Returns current connection statistics.
       * @param totalIn The total number of bytes received.
       * @param totalOut The total number of bytes sent.
       */
      void getStatistics( int &totalIn, int &totalOut );

    private:
      ConnectionTCP &operator = ( const ConnectionTCP & );
      bool dataAvailable( int timeout = -1 );

      void cancel();
      void cleanup();

      const LogSink& m_logInstance;

      char *m_buf;
      std::string m_server;
      unsigned short m_port;
      int m_socket;
      int m_totalBytesIn;
      int m_totalBytesOut;
      const int m_bufsize;
      bool m_cancel;

  };

}

#endif // TCPCONNECTION_H__
