// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <boost/ref.hpp>

#include "frantic/convert/tstring.hpp"
#include "frantic/magma/maya/maya_magma_common.hpp"
#include "frantic/magma/maya/maya_magma_exception.hpp"
#include "frantic/magma/maya/maya_magma_serializable.hpp"
#include "frantic/magma/maya/maya_magma_serializer.hpp"

#include "frantic/magma/maya/maya_magma_description.hpp"

namespace frantic {
namespace magma {
namespace maya {
namespace desc {

void maya_magma_desc_input_socket::accept_serializer( maya_magma_serializer& serializer ) const {
    serializer.visit( this );
}

void maya_magma_desc_input_socket::accept_deserializer( maya_magma_deserializer& deserializer ) {
    deserializer.visit( this );
}

std::string maya_magma_desc_input_socket::debug() const {
    std::stringstream strOut;
    strOut << "{" << m_descNodeID << "," << m_descSocketIndex << ","
           << frantic::strings::to_string( m_enumMayaAttrName );
    std::vector<frantic::tstring>::const_iterator cit;
    for( cit = m_inputSocketMayaAttrNames.begin(); cit != m_inputSocketMayaAttrNames.end(); cit++ )
        strOut << "," << cit->c_str();
    strOut << "}";
    return strOut.str();
}

////////////////////////////////////////////////////////////////////////////////

std::vector<frantic::tstring> maya_magma_desc_node::get_node_property_names() const {
    std::vector<frantic::tstring> outPropertyNames;
    std::map<frantic::tstring, frantic::tstring>::const_iterator cit;
    for( cit = m_properties.begin(); cit != m_properties.end(); cit++ )
        outPropertyNames.push_back( cit->first );
    return outPropertyNames;
}

std::vector<desc_index_type> maya_magma_desc_node::get_node_socket_indexes() const {
    std::vector<desc_index_type> outSocketIndexes;
    std::map<desc_index_type, maya_magma_desc_input_socket>::const_iterator cit;
    for( cit = m_inputSockets.begin(); cit != m_inputSockets.end(); cit++ )
        outSocketIndexes.push_back( cit->first );
    return outSocketIndexes;
}

frantic::tstring maya_magma_desc_node::get_node_property_maya_attr_name( const frantic::tstring& propertyName ) const {
    std::map<frantic::tstring, frantic::tstring>::const_iterator cit = m_properties.find( propertyName );
    if( cit == m_properties.end() )
        throw maya_magma_exception(
            _T( "maya_magma_desc_node::get_node_property_maya_attr_name cannot find property name \"" ) + propertyName +
            _T( "\"" ) );
    return cit->second;
}

bool maya_magma_desc_node::has_node_property_name( const frantic::tstring& propertyName ) const {
    std::map<frantic::tstring, frantic::tstring>::const_iterator cit = m_properties.find( propertyName );
    if( cit == m_properties.end() )
        return false;
    return true;
}

void maya_magma_desc_node::remove_node_property( const frantic::tstring& propertyName ) {
    m_properties.erase( propertyName );
}

void maya_magma_desc_node::set_node_input_socket_maya_enum_attr_name( desc_index_type socketIndex,
                                                                      const frantic::tstring& mayaAttrName ) {
    std::map<desc_index_type, maya_magma_desc_input_socket>::iterator it;
    if( ( it = m_inputSockets.find( socketIndex ) ) == m_inputSockets.end() ) {
        m_inputSockets[socketIndex] = maya_magma_desc_input_socket( m_descNodeID, socketIndex, mayaAttrName );
    } else {
        m_inputSockets[socketIndex].set_maya_enum_attr_name( mayaAttrName );
    }
}

void maya_magma_desc_node::push_node_input_socket_maya_attr_name( desc_index_type socketIndex,
                                                                  const frantic::tstring& mayaAttrName ) {
    std::map<desc_index_type, maya_magma_desc_input_socket>::iterator it;
    if( ( it = m_inputSockets.find( socketIndex ) ) == m_inputSockets.end() ) {
        m_inputSockets[socketIndex] = maya_magma_desc_input_socket( m_descNodeID, socketIndex );
        m_inputSockets[socketIndex].push_maya_attr_name( mayaAttrName );
    } else {
        m_inputSockets[socketIndex].push_maya_attr_name( mayaAttrName );
    }
}

frantic::tstring maya_magma_desc_node::get_node_input_socket_maya_enum_attr_name( desc_index_type socketIndex ) const {
    std::map<desc_index_type, maya_magma_desc_input_socket>::const_iterator cit;
    if( ( cit = m_inputSockets.find( socketIndex ) ) == m_inputSockets.end() )
        throw maya_magma_exception(
            _T( "maya_magma_desc_node::get_node_input_socket_maya_enum_attr_name can not find socket index \"" ) +
            frantic::convert::to_tstring( socketIndex ) + _T( "\"" ) );
    return cit->second.get_maya_enum_attr_name();
}

std::vector<frantic::tstring>
maya_magma_desc_node::get_node_input_socket_maya_attr_names( desc_index_type socketIndex ) const {
    std::vector<frantic::tstring> outMayaAttrNames;
    std::map<desc_index_type, maya_magma_desc_input_socket>::const_iterator cit;
    if( ( cit = m_inputSockets.find( socketIndex ) ) != m_inputSockets.end() )
        outMayaAttrNames = cit->second.get_input_socket_maya_attr_names();
    return outMayaAttrNames;
}

void maya_magma_desc_node::accept_serializer( maya_magma_serializer& serializer ) const { serializer.visit( this ); }

void maya_magma_desc_node::accept_deserializer( maya_magma_deserializer& deserializer ) { deserializer.visit( this ); }

std::string maya_magma_desc_node::debug() const {
    std::ostringstream strOut;

    strOut << "<" << m_descNodeID << "," << frantic::strings::to_string( m_nodeType ) << ">";

    std::map<frantic::tstring, frantic::tstring>::const_iterator citP;
    for( citP = m_properties.begin(); citP != m_properties.end(); citP++ )
        strOut << "(" << m_descNodeID << "," << frantic::strings::to_string( citP->first ) << ","
               << frantic::strings::to_string( citP->second ) << ")";

    std::map<desc_index_type, maya_magma_desc_input_socket>::const_iterator citI;
    for( citI = m_inputSockets.begin(); citI != m_inputSockets.end(); citI++ )
        strOut << citI->second.debug();
    return strOut.str();
}

////////////////////////////////////////////////////////////////////////////////

void maya_magma_desc_connection::accept_serializer( maya_magma_serializer& serializer ) const {
    serializer.visit( this );
}

void maya_magma_desc_connection::accept_deserializer( maya_magma_deserializer& deserializer ) {
    deserializer.visit( this );
}

std::string maya_magma_desc_connection::debug() const {
    std::ostringstream strOut;
    strOut << _T( "src:" ) << m_srcDescNodeID << _T( ",srcSocketIndex:" ) << m_srcSocketIndex << _T( ",dst:" )
           << m_dstDescNodeID << _T( ",dstSocketIndex:" ) << m_dstSocketIndex;
    return strOut.str();
}

////////////////////////////////////////////////////////////////////////////////

desc_id maya_magma_desc::get_next_available_desc_id() const {
    desc_id outDescID( 0 );
    std::map<desc_id, maya_magma_desc_node>::const_iterator cit;
    for( cit = m_nodes.begin(); cit != m_nodes.end(); cit++ )
        if( cit->first == outDescID )
            outDescID++;
        else
            break;
    return outDescID;
}

maya_magma_desc_node* maya_magma_desc::find_desc_id( desc_id id ) {
    std::map<desc_id, maya_magma_desc_node>::iterator cit;
    if( ( cit = m_nodes.find( id ) ) == m_nodes.end() )
        throw maya_magma_exception( _T( "maya_magma_desc::get_desc_node_property_maya_attr_names " ) +
                                    convert::to_tstring( id ) + _T( " is invalid" ) );
    return &( cit->second );
}

const maya_magma_desc_node* maya_magma_desc::find_desc_id( desc_id id ) const {
    std::map<desc_id, maya_magma_desc_node>::const_iterator cit;
    if( ( cit = m_nodes.find( id ) ) == m_nodes.end() )
        throw maya_magma_exception( _T( "maya_magma_desc::get_desc_node_property_maya_attr_names " ) +
                                    convert::to_tstring( id ) + _T( " is invalid" ) );
    return &( cit->second );
}

bool maya_magma_desc::contains_node( desc_id id ) const {
    std::map<desc_id, maya_magma_desc_node>::const_iterator cit;
    if( ( cit = m_nodes.find( id ) ) == m_nodes.end() )
        return false;
    return true;
}

desc_id maya_magma_desc::create_node( const frantic::tstring& typeName, int xPos, int yPos ) {
    desc_id outID = get_next_available_desc_id();
    m_nodes[outID] = maya_magma_desc_node( outID );
    m_nodes[outID].set_node_type( typeName );
    m_nodes[outID].set_node_name( typeName );
    m_nodes[outID].set_position( xPos, yPos );
    return outID;
}

desc_id maya_magma_desc::set_node( const maya_magma_desc_node& copy ) {
    desc_id outID = copy.get_desc_node_id();
    m_nodes[outID] = copy;
    return outID;
}

void maya_magma_desc::delete_node( desc_id id ) {
    m_nodes.erase( id );

    // XXX see if possible to improve this
    // because once you erase a element in vector the allocation order will be effected
    std::vector<maya_magma_desc_connection> newConnections;
    std::vector<maya_magma_desc_connection>::iterator it;
    for( it = m_connections.begin(); it != m_connections.end(); it++ ) {
        bool isDeleted = false;
        if( it->get_dst_desc_node_id() == id || it->get_src_desc_node_id() == id )
            isDeleted = true;
        if( !isDeleted )
            newConnections.push_back( *it );
    }
    m_connections = newConnections;
}

void maya_magma_desc::set_node_position( desc_id id, int x, int y ) {
    maya_magma_desc_node* node = find_desc_id( id );
    assert( node != NULL );
    node->set_position( x, y );
}

void maya_magma_desc::get_node_position( desc_id id, int& outX, int& outY ) const {
    const maya_magma_desc_node* node = find_desc_id( id );
    assert( node != NULL );
    outX = node->get_x();
    outY = node->get_y();
}

frantic::tstring maya_magma_desc::get_node_type( desc_id id ) const {
    const maya_magma_desc_node* node = find_desc_id( id );
    assert( node != NULL );
    return node->get_node_type();
}

void maya_magma_desc::set_node_name( desc_id id, const frantic::tstring& name ) {
    maya_magma_desc_node* node = find_desc_id( id );
    assert( node != NULL );
    node->set_node_name( name );
}

frantic::tstring maya_magma_desc::get_node_name( desc_id id ) const {
    const maya_magma_desc_node* node = find_desc_id( id );
    assert( node != NULL );
    return node->get_node_name();
}

int maya_magma_desc::get_node_parent( desc_id id ) const {
    const maya_magma_desc_node* node = find_desc_id( id );
    assert( node != NULL );
    return node->get_parent();
}

void maya_magma_desc::set_node_parent( desc_id id, desc_id parent ) {
    maya_magma_desc_node* node = find_desc_id( id );
    assert( node != NULL );
    node->set_parent( parent );
}

std::vector<frantic::tstring> maya_magma_desc::get_desc_node_property_maya_attr_names( desc_id id ) const {
    std::vector<frantic::tstring> outPropertyMayaAttrNames;

    const maya_magma_desc_node* descNode = const_cast<maya_magma_desc*>( this )->find_desc_id( id );
    assert( descNode != NULL );
    std::vector<frantic::tstring> nodePropertyNames = descNode->get_node_property_names();
    std::vector<frantic::tstring>::iterator itPropertyName;
    for( itPropertyName = nodePropertyNames.begin(); itPropertyName != nodePropertyNames.end(); itPropertyName++ )
        outPropertyMayaAttrNames.push_back( descNode->get_node_property_maya_attr_name( *itPropertyName ) );
    return outPropertyMayaAttrNames;
}

std::vector<frantic::tstring> maya_magma_desc::get_desc_node_property_names( desc_id id ) const {
    std::vector<frantic::tstring> outPropertyMayaAttrNames;

    const maya_magma_desc_node* descNode = const_cast<maya_magma_desc*>( this )->find_desc_id( id );
    assert( descNode != NULL );
    std::vector<frantic::tstring> nodePropertyNames = descNode->get_node_property_names();
    std::vector<frantic::tstring>::iterator itPropertyName;
    for( itPropertyName = nodePropertyNames.begin(); itPropertyName != nodePropertyNames.end(); itPropertyName++ )
        outPropertyMayaAttrNames.push_back( *itPropertyName );
    return outPropertyMayaAttrNames;
}

frantic::tstring maya_magma_desc::get_maya_attr_name_from_node_property( desc_id id,
                                                                         const frantic::tstring& propertyName ) const {
    const maya_magma_desc_node* descNode = const_cast<maya_magma_desc*>( this )->find_desc_id( id );
    assert( descNode != NULL );
    return descNode->get_node_property_maya_attr_name( propertyName );
}

bool maya_magma_desc::has_node_property_name( desc_id id, const frantic::tstring& propertyName ) const {
    const maya_magma_desc_node* descNode = const_cast<maya_magma_desc*>( this )->find_desc_id( id );
    assert( descNode != NULL );
    return descNode->has_node_property_name( propertyName );
}

void maya_magma_desc::remove_node_property( desc_id id, const frantic::tstring& propertyName ) {
    maya_magma_desc_node* descNode = this->find_desc_id( id );
    assert( descNode != NULL );
    descNode->remove_node_property( propertyName );
}

std::vector<frantic::tstring> maya_magma_desc::get_desc_node_input_socket_maya_attr_names( desc_id id ) const {
    std::vector<frantic::tstring> outInputSocketMayaAttrNames;

    const maya_magma_desc_node* descNode = const_cast<maya_magma_desc*>( this )->find_desc_id( id );
    assert( descNode != NULL );
    std::vector<desc_index_type> nodeSocketIndexes = descNode->get_node_socket_indexes();
    std::vector<desc_index_type>::iterator itSocketIndex;
    for( itSocketIndex = nodeSocketIndexes.begin(); itSocketIndex != nodeSocketIndexes.end(); itSocketIndex++ ) {
        frantic::tstring mayaEnumAttrName = descNode->get_node_input_socket_maya_enum_attr_name( *itSocketIndex );
        std::vector<frantic::tstring> mayaAttrNames = descNode->get_node_input_socket_maya_attr_names( *itSocketIndex );

        outInputSocketMayaAttrNames.push_back( mayaEnumAttrName );
        outInputSocketMayaAttrNames.insert( outInputSocketMayaAttrNames.end(), mayaAttrNames.begin(),
                                            mayaAttrNames.end() );
    }

    return outInputSocketMayaAttrNames;
}

std::vector<frantic::tstring>
maya_magma_desc::get_desc_node_input_socket_maya_attr_names( desc_id id, desc_index_type index ) const {
    const maya_magma_desc_node* descNode = const_cast<maya_magma_desc*>( this )->find_desc_id( id );
    assert( descNode != NULL );

    std::vector<frantic::tstring> outInputSocketMayaAttrNames =
        descNode->get_node_input_socket_maya_attr_names( index );
    return outInputSocketMayaAttrNames;
}

std::vector<frantic::tstring> maya_magma_desc::get_desc_node_input_socket_names( desc_id id,
                                                                                 desc_index_type socketIndex ) const {
    std::vector<frantic::tstring> outInputSocketNames;

    const maya_magma_desc_node* descNode = const_cast<maya_magma_desc*>( this )->find_desc_id( id );
    assert( descNode != NULL );

    outInputSocketNames = descNode->get_node_input_socket_maya_attr_names( socketIndex );

    return outInputSocketNames;
}

frantic::tstring maya_magma_desc::get_desc_node_input_socket_maya_enum_attr_name( desc_id id,
                                                                                  desc_index_type socketIndex ) const {
    maya_magma_desc_node* node = const_cast<maya_magma_desc*>( this )->find_desc_id( id );
    assert( node != NULL );
    return node->get_node_input_socket_maya_enum_attr_name( socketIndex );
}

void maya_magma_desc::create_connection( desc_id srcID, desc_index_type srcSocketIndex, desc_id dstID,
                                         desc_index_type dstSocketIndex ) {
    if( srcID == dstID )
        throw maya_magma_exception(
            "maya_magma_desc::create_connection: A connection cannot have the same node source and destination." );

    // Make sure nodes are there
    bool foundSrc = false;
    bool foundDst = false;
    for( std::map<desc_id, maya_magma_desc_node>::const_iterator iter = m_nodes.begin(); iter != m_nodes.end();
         ++iter ) {
        if( iter->first == srcID )
            foundSrc = true;
        if( iter->first == dstID )
            foundDst = true;
        if( foundSrc && foundDst )
            break;
    }
    if( !foundSrc )
        throw maya_magma_exception( _T( "maya_magma_desc::create_connection: Node ID (" ) +
                                    boost::lexical_cast<frantic::tstring>( srcID ) +
                                    _T( ") does not exist. Cannot create connection." ) );
    if( !foundDst )
        throw maya_magma_exception( _T( "maya_magma_desc::create_connection: Node ID (" ) +
                                    boost::lexical_cast<frantic::tstring>( dstID ) +
                                    _T( ") does not exist. Cannot create connection." ) );

    maya_magma_desc_connection newConnection( srcID, srcSocketIndex, dstID, dstSocketIndex );

    // Make sure the connection is not there
    bool exists = false;
    for( std::vector<maya_magma_desc_connection>::const_iterator iter = m_connections.begin();
         iter != m_connections.end(); ++iter ) {
        if( *iter == newConnection ) {
            exists = true;
            break;
        }
    }

    if( !exists )
        m_connections.push_back( newConnection );
}

void maya_magma_desc::delete_connection( desc_id srcID, desc_index_type srcSocketIndex, desc_id dstID,
                                         desc_index_type dstSocketIndex ) {
    std::vector<maya_magma_desc_connection>::iterator it;
    maya_magma_desc_connection testConnection( srcID, srcSocketIndex, dstID, dstSocketIndex );
    for( it = m_connections.begin(); it != m_connections.end(); it++ )
        if( ( *it ) == testConnection )
            break;

    if( it != m_connections.end() )
        m_connections.erase( it );
}

std::vector<maya_magma_desc_connection> maya_magma_desc::get_node_connections( desc_id id, bool getInputs,
                                                                               bool getOutputs ) const {
    std::vector<maya_magma_desc_connection> edges;

    for( std::vector<maya_magma_desc_connection>::const_iterator iter = m_connections.begin();
         iter != m_connections.end(); ++iter ) {
        if( getInputs ) {
            if( iter->get_dst_desc_node_id() == id ) {
                edges.push_back( *iter );
            }
        }
        if( getOutputs ) {
            if( iter->get_src_desc_node_id() == id ) {
                edges.push_back( *iter );
            }
        }
    }

    return edges;
}

void maya_magma_desc::set_node_property_maya_attr_name( desc_id id, const frantic::tstring& propName,
                                                        const frantic::tstring& mayaAttrName ) {
    maya_magma_desc_node* descNode = find_desc_id( id );
    assert( descNode != NULL );
    descNode->set_node_property_maya_attr_name( propName, mayaAttrName );
}

void maya_magma_desc::set_node_input_socket_maya_enum_attr_name( desc_id id, desc_index_type socketIndex,
                                                                 const frantic::tstring& mayaAttrName ) {
    maya_magma_desc_node* descNode = find_desc_id( id );
    assert( descNode != NULL );
    descNode->set_node_input_socket_maya_enum_attr_name( socketIndex, mayaAttrName );
}

void maya_magma_desc::push_node_input_socket_maya_attr_name( desc_id id, desc_index_type socketIndex,
                                                             const frantic::tstring& mayaAttrName ) {
    maya_magma_desc_node* descNode = find_desc_id( id );
    assert( descNode != NULL );
    descNode->push_node_input_socket_maya_attr_name( socketIndex, mayaAttrName );
}

void maya_magma_desc::to_stream( std::ostream& out, bool isAscii ) const {
    boost::shared_ptr<maya_magma_serializer> serializer;
    if( m_serializer_type == MAYA_MAGMA_SERIALIZER_STANDARD )
        serializer = boost::make_shared<maya_magma_standard_serializer>( boost::ref( out ), isAscii );

    // call accept serializer to handle the job
    accept_serializer( *serializer );
}

void maya_magma_desc::from_stream( std::istream& in, std::size_t length ) {
    boost::shared_ptr<maya_magma_deserializer> deserializer;
    if( m_serializer_type == MAYA_MAGMA_SERIALIZER_STANDARD )
        deserializer = boost::make_shared<maya_magma_standard_deserializer>( boost::ref( in ), length );

    // call accept deserializer to handle the job
    accept_deserializer( *deserializer );
}

void maya_magma_desc::accept_serializer( maya_magma_serializer& serializer ) const { serializer.visit( this ); }

void maya_magma_desc::accept_deserializer( maya_magma_deserializer& deserializer ) { deserializer.visit( this ); }

std::string maya_magma_desc::debug() const {
    std::ostringstream strOut;
    std::map<desc_id, maya_magma_desc_node>::const_iterator citM;
    for( citM = m_nodes.begin(); citM != m_nodes.end(); citM++ )
        strOut << _T( "node: key:" ) << citM->first << _T( "," ) << citM->second.debug() << std::endl;
    std::vector<maya_magma_desc_connection>::const_iterator citV;
    for( citV = m_connections.begin(); citV != m_connections.end(); citV++ )
        strOut << _T( "connection: " ) << citV->debug() << std::endl;
    return strOut.str();
}

} // namespace desc
} // namespace maya
} // namespace magma
} // namespace frantic
