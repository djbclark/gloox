/*
  Copyright (c) 2004-2005 by Jakob Schroeter <js@camaya.net>
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


#include "disco.h"
#include "discohandler.h"
#include "clientbase.h"
#include "disconodehandler.h"


namespace gloox
{

  Disco::Disco( ClientBase *parent )
    : m_parent( parent )
  {
    addFeature( XMLNS_VERSION );
    addFeature( XMLNS_DISCO_INFO );
    addFeature( XMLNS_DISCO_ITEMS );
    if( m_parent )
    {
      m_parent->registerIqHandler( this, XMLNS_DISCO_INFO );
      m_parent->registerIqHandler( this, XMLNS_DISCO_ITEMS );
      m_parent->registerIqHandler( this, XMLNS_VERSION );
    }
  }

  Disco::~Disco()
  {
    if( m_parent )
    {
      m_parent->removeIqHandler( XMLNS_DISCO_INFO );
      m_parent->removeIqHandler( XMLNS_DISCO_ITEMS );
      m_parent->removeIqHandler( XMLNS_VERSION );
    }
  }

  bool Disco::handleIq( Stanza *stanza )
  {
    switch( stanza->subtype() )
    {
      case STANZA_IQ_GET:
        if( stanza->xmlns() == XMLNS_VERSION )
        {
          Tag *iq = new Tag( "iq" );
          iq->addAttrib( "id", stanza->id() );
          iq->addAttrib( "from", m_parent->jid().full() );
          iq->addAttrib( "to", stanza->from().full() );
          Tag *query = new Tag( "query" );
          query->addAttrib( "xmlns", XMLNS_VERSION );
          Tag *name = new Tag( "name", m_versionName );
          Tag *version = new Tag( "version", m_versionVersion );
          query->addChild( name );
          query->addChild( version );
          iq->addChild( query );
          m_parent->send( iq );
        }
        else if( stanza->xmlns() == XMLNS_DISCO_INFO )
        {
          Tag *iq = new Tag( "iq" );
          iq->addAttrib( "id", stanza->id() );
          iq->addAttrib( "from", m_parent->jid().full() );
          iq->addAttrib( "to", stanza->from().full() );
          iq->addAttrib( "type", "result" );
          Tag *query = new Tag( "query" );
          iq->addChild( query );
          query->addAttrib( "xmlns", XMLNS_DISCO_INFO );

          Tag *q = stanza->findChild( "query" );
          const std::string node = q->findAttribute( "node" );
          if( !node.empty() )
          {
            DiscoNodeHandlerMap::const_iterator it = m_nodeHandlers.find( node );
            if( it != m_nodeHandlers.end() )
            {
              std::string name;
              DiscoNodeHandler::IdentityMap identities =
                  (*it).second->handleDiscoNodeIdentities( node, name );
              DiscoNodeHandler::IdentityMap::const_iterator im = identities.begin();
              for( im; im != identities.end(); im++ )
              {
                Tag *i = new Tag( "identity" );
                i->addAttrib( "category", (*im).first );
                i->addAttrib( "type", (*im).second );
                i->addAttrib( "name", name );
                query->addChild( i );
              }
              DiscoNodeHandler::FeatureList features = (*it).second->handleDiscoNodeFeatures( node );
              DiscoNodeHandler::FeatureList::const_iterator fi = features.begin();
              for( fi; fi != features.end(); fi++ )
              {
                Tag *f = new Tag( "feature" );
                f->addAttrib( "var", (*fi) );
                query->addChild( f );
              }
            }
          }
          else
          {
            Tag *i = new Tag( "identity" );
            i->addAttrib( "category", m_identityCategory );
            i->addAttrib( "type", m_identityType );
            i->addAttrib( "name", m_versionName );
            query->addChild( i );

            StringList::const_iterator it = m_features.begin();
            for( it; it != m_features.end(); ++it )
            {
              Tag *f = new Tag( "feature" );
              f->addAttrib( "var", (*it).c_str() );
              query->addChild( f );
            }
          }

          m_parent->send( iq );
        }
        else if( stanza->xmlns() == XMLNS_DISCO_ITEMS )
        {
          Tag *iq = new Tag( "iq" );
          iq->addAttrib( "id", stanza->id() );
          iq->addAttrib( "to", stanza->from().full() );
          iq->addAttrib( "from", m_parent->jid().full() );
          iq->addAttrib( "type", "result" );
          Tag *query = new Tag( "query" );
          query->addAttrib( "xmlns", XMLNS_DISCO_ITEMS );

          DiscoNodeHandler::ItemMap items;
          DiscoNodeHandlerMap::const_iterator it;
          Tag *q = stanza->findChild( "query" );
          const std::string node = q->findAttribute( "node" );
          if( !node.empty() )
          {
            it = m_nodeHandlers.find( node );
            if( it != m_nodeHandlers.end() )
            {
              items = (*it).second->handleDiscoNodeItems( node );
            }
          }
          else
          {
            it = m_nodeHandlers.begin();
            for( it; it != m_nodeHandlers.end(); it++ )
            {
              items = (*it).second->handleDiscoNodeItems( "" );
            }
          }

          if( items.size() )
          {
            DiscoNodeHandler::ItemMap::const_iterator it = items.begin();
            for( it; it != items.end(); it++ )
            {
              if( !(*it).first.empty() && !(*it).second.empty() )
              {
                Tag *i = new Tag( "item" );
                i->addAttrib( "jid",  m_parent->jid().full() );
                i->addAttrib( "node", (*it).first );
                i->addAttrib( "name", (*it).second );
                query->addChild( i );
              }
            }
          }

          iq->addChild( query );
          m_parent->send( iq );
        }
        return true;
        break;

      case IKS_TYPE_SET:
      {
        bool res = false;
        DiscoHandlerList::const_iterator it = m_discoHandlers.begin();
        for( it; it != m_discoHandlers.end(); it++ )
        {
          if( (*it)->handleDiscoSet( stanza->id(), stanza ) )
            res = true;
        }
        return res;
        break;
      }
    }
    return false;
  }

  bool Disco::handleIqID( Stanza *stanza, int context )
  {
    switch( stanza->subtype() )
    {
      case IKS_TYPE_RESULT:

        switch( context )
        {
          case GET_DISCO_INFO:
          {
            DiscoHandlerList::const_iterator it = m_discoHandlers.begin();
            for( it; it != m_discoHandlers.end(); it++ )
            {
              (*it)->handleDiscoInfoResult( stanza->id(), stanza );
            }
            break;
          }
          case GET_DISCO_ITEMS:
          {
            DiscoHandlerList::const_iterator it = m_discoHandlers.begin();
            for( it; it != m_discoHandlers.end(); it++ )
            {
              (*it)->handleDiscoItemsResult( stanza->id(), stanza );
            }
            break;
          }
        }
        break;

      case IKS_TYPE_ERROR:
        Tag *e = stanza->findChild( "error" );
        if( !e )
          return false;

        DiscoHandlerList::const_iterator it = m_discoHandlers.begin();
        for( it; it != m_discoHandlers.end(); it++ )
        {
          (*it)->handleDiscoError( stanza->id(), e->name() );
        }
        break;
    }

    return false;
  }

  void Disco::addFeature( const std::string& feature )
  {
    m_features.push_back( feature );
  }

  void Disco::getDiscoInfo( const std::string& to )
  {
    std::string id = m_parent->getID();

    Tag *iq = new Tag( "iq" );
    iq->addAttrib( "id", id );
    iq->addAttrib( "to", to );
    iq->addAttrib( "from", m_parent->jid().full() );
    iq->addAttrib( "type", "get" );
    Tag *q = new Tag( "query" );
    q->addAttrib( "xmlns", XMLNS_DISCO_INFO );
    iq->addChild( q );

    m_parent->trackID( this, id, GET_DISCO_INFO );
    m_parent->send( iq );
  }

  void Disco::getDiscoItems( const std::string& to )
  {
    std::string id = m_parent->getID();

    Tag *iq = new Tag( "iq" );
    iq->addAttrib( "id", id );
    iq->addAttrib( "to", to );
    iq->addAttrib( "from", m_parent->jid().full() );
    iq->addAttrib( "type", "get" );
    Tag *q = new Tag( "query" );
    q->addAttrib( "xmlns", XMLNS_DISCO_ITEMS );
    iq->addChild( q );

    m_parent->trackID( this, id, GET_DISCO_INFO );
    m_parent->send( iq );
  }

  void Disco::setVersion( const std::string& name, const std::string& version )
  {
    m_versionName = name;
    m_versionVersion = version;
  }

  void Disco::setIdentity( const std::string& category, const std::string& type )
  {
    m_identityCategory = category;
    m_identityType = type;
  }

  void Disco::registerDiscoHandler( DiscoHandler *dh )
  {
    m_discoHandlers.push_back( dh );
  }

  void Disco::removeDiscoHandler( DiscoHandler *dh )
  {
    m_discoHandlers.remove( dh );
  }

  void Disco::registerNodeHandler( DiscoNodeHandler *nh, const std::string& node )
  {
    m_nodeHandlers[node] = nh;
  }

  void Disco::removeNodeHandler( const std::string& node )
  {
    m_nodeHandlers.erase( node );
  }

};
