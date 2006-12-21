/*
  Copyright (c) 2006 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef OOB_H__
#define OOB_H__

#include "stanzaextension.h"

#include <string>

namespace gloox
{

  class Tag;

  /**
   * @brief This is an abstraction of a jabber:x:oob namespace element or a jabber:iq:oob namespace element.
   *
   * @author Jakob Schroeter <js@camaya.net>
   * @since 0.9
   */
  class GLOOX_API OOB : public StanzaExtension
  {
    public:
      /**
       * Constructs an empty OOB object.
       * @param xmlns The namespace of the OOB (either jabber:x:oob or jabber:iq:oob).
       */
      OOB( const std::string& xmlns );

      /**
       * Constructs an OOB object from the given Tag. To be recognized properly, the Tag should
       * either have a name 'x' of in the jabber:x:oob namespace, or a name of 'query' in the
       * jabber:iq:oob namespace.
       * @param tag The Tag to parse.
       */
      OOB( Tag *tag );

      /**
       * Virtual destructor.
       */
      virtual ~OOB();

      /**
       * Sets the out-of-band URL.
       * @param url The out-of-band URL.
       */
      void setUrl( const std::string& url ) { m_url = url; };

      /**
       * Let you retrieve the out-of-band URL.
       * @return The out-of-band URL.
       */
      const std::string& url() const { return m_url; };

      /**
       * Sets a descriptive text.
       * @param desc A string describing the URL's content.
       */
      void setDesc( const std::string& desc ) { m_desc = desc; };

      /**
       * Lets you retrieve the URL's description.
       * @return The URL's description.
       */
      const std::string& desc() const { return m_desc; };

      // reimplemented from StanzaExtension
      Tag* tag() const;

    private:
      std::string m_url;
      std::string m_desc;
      std::string m_xmlns;
  };

}

#endif // OOB_H__
