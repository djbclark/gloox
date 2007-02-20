/*
  Copyright (c) 2007 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/



#ifndef TLSHANDLER_H__
#define TLSHANDLER_H__

#include <string>

namespace gloox
{

  struct CertInfo;

  /**
   * @brief An interface that allows for interacting with TLS implementations derived from TLSBase.
   *
   * @author Jakob Schr�ter <js@camaya.net>
   * @since 0.9
   */
  class GLOOX_API TLSHandler
  {
    public:
      /**
       * Virtual Destructor.
       */
      virtual ~TLSHandler() {};

      /**
       *
       */
      virtual void handleEncryptedData( const std::string& data ) = 0;

      /**
       *
       */
      virtual void handleDecryptedData( const std::string& data ) = 0;

      /**
       *
       */
      virtual void handleHandshakeResult( bool success, CertInfo &certinfo ) = 0;

  };

}

#endif // TLSHANDLER_H__
