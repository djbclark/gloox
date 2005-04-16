/*
  iksemelmm -- c++ wrapper for iksemel xml/xmpp library

  Copyright (C) 2004 Igor Goryachieff <igor@jahber.org>
  Copyright (C) 2005 Jakob Schroeter <js@camaya.net>

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


#ifndef PARSER_H__
#define PARSER_H__

#include <iksemel.h>

/**
 * This namespace is used for the classes taken from Igor Goryachieff's Iksemelmm
 * wrapper around Iksemel. It was included for convenience reasons and to decrease
 * external dependencies of JLib.
 *
 */
namespace Iksemel
{
  /**
   * This class encapsulates the iks_parser from Iksemel.
   */
  class Parser
  {
    public:
      Parser();
      ~Parser();

      void init( iksparser* );

      int parse( char*, size_t, int );
      void reset();

      unsigned long bytes() const;
      unsigned long lines() const;
    protected:
      iksparser* P;
      bool inited;

    private:
      // for copying prevention
      Parser( const Parser& );
      Parser& operator=( const Parser& );
  };
}

#endif // PARSER_H__
