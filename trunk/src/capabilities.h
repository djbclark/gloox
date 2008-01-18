/*
  Copyright (c) 2007-2008 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef CAPABILITIES_H__
#define CAPABILITIES_H__

#include "disconodehandler.h"
#include "stanzaextension.h"
#include "tag.h"

#include <string>

namespace gloox
{

  class Disco;
  class Tag;

  /**
   * @brief This is an implementation of XEP-0115 (Entity Capabilities).
   *
   * XEP Version: 1.5-15
   * @author Jakob Schroeter <js@camaya.net>
   * @since 1.0
   */
  class GLOOX_API Capabilities : public StanzaExtension, public DiscoNodeHandler
  {

    public:
      /**
       * Constructs a new object and fills it according to the parameters.
       * @param disco The current Client's Disco object.
       */
      Capabilities( Disco* disco );

      /**
       * Constructs a new object from the given Tag.
       * @param tag The Tag to parse.
       */
      Capabilities( const Tag* tag );

      /**
       * Virtual Destructor.
       */
      virtual ~Capabilities();

      /**
       * Returns the client's identifying node.
       * @return The node.
       */
      const std::string& node() const { return m_node; }

      /**
       * Sets the client's identifying node.
       * @param node The node.
       */
      void setNode( const std::string& node ) { m_node = node; }

      /**
       * Returns the client's identifying ver string.
       * @return The ver string.
       */
      const std::string ver() const;

      /**
       * Returns the application's advertised name.
       * @return The application's advertised name.
       */
      const std::string& n() const;

      /**
       * Returns the application's advertised version.
       * @return The application's advertised version.
       */
      const std::string& v() const;

      /**
       * Returns the application's advertised operating system.
       * @return The application's advertised operating system.
       */
      const std::string& os() const;

      // reimplemented from StanzaExtension
      virtual const std::string& filterString() const;

      // reimplemented from StanzaExtension
      virtual StanzaExtension* newInstance( const Tag* tag ) const
      {
        return new Capabilities( tag );
      }

      // reimplemented from StanzaExtension
      virtual Tag* tag() const;

      // reimplemented from DiscoNodeHandler
      virtual StringList handleDiscoNodeFeatures( const JID& from, const std::string& node );

      // reimplemented from DiscoNodeHandler
      virtual Disco::IdentityList handleDiscoNodeIdentities( const JID& from,
                                                             const std::string& node );

      // reimplemented from DiscoNodeHandler
      virtual Disco::ItemList handleDiscoNodeItems( const JID& from,
                                                    const std::string& node = EmptyString );

    private:
      Disco* m_disco;
      std::string m_node;
      std::string m_ver;
      std::string m_n;
      std::string m_v;
      std::string m_os;
      bool m_valid;
  };

}

#endif // CAPABILITIES_H__
