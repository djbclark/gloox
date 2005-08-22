/*
  Copyright (c) 2005 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
*/


#ifndef REGISTRATION_H__
#define REGISTRATION_H__

#include "iqhandler.h"
#include "registrationhandler.h"

#include <string>
#include <map>

namespace gloox
{

  class ClientBase;
  class Stanza;

  /**
   * This class is an implementation of JEP-0077 (In-Band Registration).
   * Usage is very straight-forward.<br>
   * Derive your object from @ref RegistrationHandler and implement the
   * virtual functions offered by that interface. Then use it like this:
   * @code
   * Registration* r = new Registration( "example.org" );
   * r->registerRegistrationHandler( this );
   * r->createAccount( ... );
   * @endcode
   * Check @c tests/register_test.cpp for an example.
   * @author Jakob Schroeter <js@camaya.net>
   * @since 0.2
   */
  class Registration : public IqHandler
  {
    public:

      /**
       * Holds all the possible fields a server may require for registration according
       * to Section 14.1, JEP-0077.
       */
      struct fieldStruct
      {
        std::string username;
        std::string nick;
        std::string password;
        std::string name;
        std::string first;
        std::string last;
        std::string email;
        std::string address;
        std::string city;
        std::string state;
        std::string zip;
        std::string phone;
        std::string url;
        std::string date;
        std::string misc;
        std::string text;
      };

      /**
       * The possible fields of a JEP-0077 account registration.
       */
      enum fieldEnum
      {
        FIELD_USERNAME =     1,
        FIELD_NICK     =     2,
        FIELD_PASSWORD =     4,
        FIELD_NAME     =     8,
        FIELD_FIRST    =    16,
        FIELD_LAST     =    32,
        FIELD_EMAIL    =    64,
        FIELD_ADDRESS  =   128,
        FIELD_CITY     =   256,
        FIELD_STATE    =   512,
        FIELD_ZIP      =  1024,
        FIELD_PHONE    =  2048,
        FIELD_URL      =  4096,
        FIELD_DATE     =  8192,
        FIELD_MISC     = 16384,
        FIELD_TEXT     = 32768
      };

      /**
       * Constructor.
       * Use this constructor if you already have an established connection to a server. The
       * Client must be in a connected and authenticated state, i.e. STATE_AUTHENTICATED.
       * A @c Registration object vcreated with this constructor can only be used to change
       * the account's password or to un-register an account.
       * @param parent The ClientBase which is used.
       */
      Registration( ClientBase *parent );

      /**
       * Destructor.
       */
      virtual ~Registration();

      /**
       * Use this function to request the registration fields the server requires.
       * The required fields are returned asynchronously to the object registered as
       * @ref RegistrationHandler by calling @ref RegistrationHandler::handleRegistrationFields().
       */
      void fetchRegistrationFields();

      /**
       * Attempts to register an account with the given credentials. Only the fields OR'ed in
       * @c fields will eb sent. This can only be called with an unauthenticated parent (@ref Client).
       * @note It is recommended to use @ref fetchRegistrationFields to find out which fields the
       * server requires.
       * @param fields The fields to use to generate the registration request. OR'ed @ref fieldEnum values.
       * @param values The struct contains the values which shall be used for the registration.
       */
      void createAccount( int fields, fieldStruct values );

      /**
       * Tells the server to remove the currently authenticated account from the server.
       */
      void removeAccount();

      /**
       * Tells the server to change the password for the current account.
       * @param password The new password.
       */
      void changePassword( const std::string& password );

      /**
       * Registers the given @c rh as RegistrationHandler. Only one handler is possibel at a time.
       * @param rh The RegistrationHandler to register.
       */
      void registerRegistrationHandler( RegistrationHandler *rh );

      /**
       * Un-registers the given RegistrationHandler.
       * @param rh The RegistrationHandler to un-register.
       */
      void removeRegistrationHandler( RegistrationHandler *rh );

      // reimplemented from IqHandler
      virtual bool handleIq( const Stanza& stanza );

      // reimplemented from IqHandler
      virtual bool handleIqID( const Stanza& stanza, int context );

    private:
      enum IdType
      {
        FETCH_REGISTRATION_FIELDS,
        CREATE_ACCOUNT,
        REMOVE_ACCOUNT,
        CHANGE_PASSWORD
      };

      ClientBase *m_parent;
      RegistrationHandler *m_registrationHandler;
  };

};

#endif // REGISTRATION_H__
