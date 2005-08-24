/*
  Copyright (c) 2004-2005 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warrenty.
*/



#ifndef PARSER_H__
#define PARSER_H__

#include "gloox.h"

#include <iksemel.h>

#include <string>

namespace gloox
{

  class ClientBase;
  class Stanza;

  /**
   * This class is an abstraction of libiksemel's XML parser.
   * @author Jakob Schroeter <js@camaya.net>
   * @since 0.4
   */
  class Parser
  {
    public:
      /**
       * Describes the return values of the parser.
       */
      enum ParserState
      {
        PARSER_OK,                     /**< Everything's alright. */
        PARSER_NOMEM,                  /**< Memory allcation error. */
        PARSER_BADXML,                 /**< XML parse error. */
      };

      /**
       * Constructs a new Parser object.
       * @param parent The object to send incoming Tags to.
       * @param ns The parser's namespace. Legacy, libiksemel-related.
       */
      Parser( ClientBase *parent, const std::string& ns );

      /**
       * Virtual destructor.
       */
      virtual ~Parser();

      /**
       * Use this function to feed the parser with more XML.
       * @param data Raw xml to parse.
       */
      ParserState feed( const std::string& data );

    private:
      void streamEvent( Stanza *stanza );

      iksparser *m_parser;
      ClientBase *m_parent;
      Stanza *m_current;
      Stanza *m_root;

      friend int cdataHook( Parser *parser, char *data, size_t len );
      friend int tagHook( Parser *parser, char *name, char **atts, int type );
  };

};

#endif // PARSER_H__
