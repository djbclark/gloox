/*
  Copyright (c) 2005 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef PRIVACYLISTHANDLER_H__
#define PRIVACYLISTHANDLER_H__

#include "privacyitem.h"

#include <string>
#include <list>

namespace gloox
{

  /**
   * @brief A virtual interface that allows to retrieve Privacy Lists.
   *
   * @author Jakob Schroeter <js@camaya.net>
   * @since 0.3
   */
  class GLOOX_EXPORT PrivacyListHandler
  {
    public:

      /**
       * The possible results of an operation on a privacy list.
       */
      enum resultEnum
      {
        RESULT_STORE_SUCCESS,         /**< Storing was successful. */
        RESULT_ACTIVATE_SUCCESS,      /**< Activation was successful. */
        RESULT_DEFAULT_SUCCESS,       /**< Setting the default list was successful. */
        RESULT_REMOVE_SUCCESS,        /**< Removing a list was successful. */
        RESULT_REQUEST_NAMES_SUCCESS, /**< Requesting the list names was successful. */
        RESULT_REQUEST_LIST_SUCCESS,  /**< The list was requested successfully. */
        RESULT_CONFLICT,              /**< A conflict occurred when activating a list or setting the default
                                       * list. */
        RESULT_ITEM_NOT_FOUND,        /**< The requested list does not exist. */
        RESULT_BAD_REQUEST            /**< Bad request. */
      };

      /**
       * A list of PrivacyItems.
       */
      typedef std::list<PrivacyItem> PrivacyList;

      /**
       * Virtual Destructor.
       */
      virtual ~PrivacyListHandler() {};

      /**
       * Reimplement this function to retrieve the list of privacy list names after requesting it using
       * PrivacyManager::requestListNames().
       * @param active The name of the active list.
       * @param def The name of the default list.
       * @param lists All the lists.
       */
      virtual void handlePrivacyListNames( const std::string& active, const std::string& def,
                                           const StringList& lists ) = 0;

      /**
       * Reimplement this function to retrieve the content of a privacy list after requesting it using
       * PrivacyManager::requestList().
       * @param name The name of the list.
       * @param items A list of PrivacyItem's.
       */
      virtual void handlePrivacyList( const std::string& name, PrivacyList& items ) = 0;

      /**
       * Reimplement this function to be notified about new or changed lists.
       * @param name The name of the new or changed list.
       */
      virtual void handlePrivacyListChanged( const std::string& name ) = 0;

      /**
       * Reimplement this function to receive results of stores etc.
       * @param id The ID of the request, as returned by the initiating function.
       * @param result The result of an operation.
       */
      virtual void handlePrivacyListResult( const std::string& id, resultEnum result ) = 0;

  };

}

#endif // PRIVACYLISTHANDLER_H__
