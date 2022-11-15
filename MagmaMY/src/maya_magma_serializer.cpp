// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <assert.h>
#include <boost/regex.hpp>

#include "frantic/convert/tstring.hpp"
#include "frantic/magma/maya/maya_magma_description.hpp"
#include "frantic/magma/maya/maya_magma_exception.hpp"
#include "frantic/magma/maya/maya_magma_serializer.hpp"
#include <frantic/logging/logging_level.hpp>
#include <frantic/strings/utf8.hpp>

#include <frantic/maya/convert.hpp>

namespace frantic {
namespace magma {
namespace maya {

using namespace frantic::magma::maya::desc;

namespace {

// Convert the given string into serialized form, removing any characters that are reserved
frantic::tstring sanitizeString( const frantic::tstring& str, bool isAscii, bool removeReserved = true ) {
    std::stringstream streamer;
    bool removedCharacters = false;
    for( frantic::tstring::const_iterator iter = str.begin(); iter != str.end(); ++iter ) {

        char current = *iter;
        switch( current ) {
            // Characters used for our serialization structure
        case ',':
        case '(':
        case ')':
        case '[':
        case ']':
        case '{':
        case '}':
        case '<':
        case '>':
            if( removeReserved )
                removedCharacters = true;
            else
                streamer << current;
            break;

            // Ascii escape characters
        case '\\':
            if( isAscii )
                streamer << "\\\\";
            else
                streamer << current;
            break;
        case '\r':
            if( isAscii )
                streamer << "\\r";
            else
                streamer << current;
            break;
        case '\n':
            if( isAscii )
                streamer << "\\n";
            else
                streamer << current;
            break;
        case '\"':
            if( isAscii )
                streamer << "\\\"";
            else
                streamer << current;
            break;
        case '\t':
            if( isAscii )
                streamer << "\\t";
            else
                streamer << current;
            break;
        case '\v':
            if( isAscii )
                streamer << "\\v";
            else
                streamer << current;
            break;

        default:
            streamer << current;
            break;
        }
    }

    frantic::tstring result = frantic::strings::to_tstring( streamer.str() );
    if( removedCharacters ) {
        MGlobal::displayWarning( MString( "Magma node description \"" ) + str.c_str() +
                                 ( "\" contains characters that were not saved" ) );
    }
    return result;
}
} // namespace

void maya_magma_standard_serializer::visit( const desc::maya_magma_desc_input_socket* e ) {
    assert( e != NULL );

    m_out << "{" << e->get_desc_node_id() << "," << e->get_desc_socket_index() << ","
          << frantic::strings::to_utf8( e->get_maya_enum_attr_name() );
    std::vector<frantic::tstring>::const_iterator cit;
    const std::vector<frantic::tstring> inputSocketMayaAttrNames = e->get_input_socket_maya_attr_names();
    for( cit = inputSocketMayaAttrNames.begin(); cit != inputSocketMayaAttrNames.end(); cit++ )
        m_out << "," << frantic::strings::to_utf8( *cit );
    m_out << "}";
}

void maya_magma_standard_serializer::visit( const desc::maya_magma_desc_node* e ) {
    assert( e != NULL );

    // Sanitize node name
    frantic::tstring nodeName = sanitizeString( e->get_node_name(), m_isAscii );

    m_out << "<" << e->get_desc_node_id() << "," << frantic::strings::to_utf8( e->get_node_type() ) << "," << e->get_x()
          << "," << e->get_y() << "," << frantic::strings::to_utf8( nodeName ) << "," << e->get_parent() << ">";

    std::vector<frantic::tstring> propertyNames = e->get_node_property_names();
    std::vector<frantic::tstring>::const_iterator cit;
    for( cit = propertyNames.begin(); cit != propertyNames.end(); cit++ )
        m_out << "(" << e->get_desc_node_id() << "," << frantic::strings::to_utf8( *cit ) << ","
              << frantic::strings::to_utf8( e->get_node_property_maya_attr_name( *cit ) ) << ")";

    std::map<desc_index_type, maya_magma_desc_input_socket>::const_iterator citInputSocket;
    const std::map<desc_index_type, maya_magma_desc_input_socket>& inputSocketMayaAttrs =
        e->get_node_input_socket_maya_attr();
    for( citInputSocket = inputSocketMayaAttrs.begin(); citInputSocket != inputSocketMayaAttrs.end(); citInputSocket++ )
        citInputSocket->second.accept_serializer( *this );
}

void maya_magma_standard_serializer::visit( const desc::maya_magma_desc_connection* e ) {
    assert( e != NULL );
    m_out << '[' << e->get_src_desc_node_id() << "," << e->get_src_socket_index() << "," << e->get_dst_desc_node_id()
          << "," << e->get_dst_socket_index() << "]";
}

void maya_magma_standard_serializer::visit( const desc::maya_magma_desc* e ) {
    assert( e != NULL );

    const std::map<desc_id, maya_magma_desc_node> nodes = e->get_nodes();
    const std::vector<maya_magma_desc_connection> connections = e->get_connections();

    std::map<desc_id, maya_magma_desc_node>::const_iterator citNode;
    for( citNode = nodes.begin(); citNode != nodes.end(); citNode++ )
        citNode->second.accept_serializer( *this );

    std::vector<maya_magma_desc_connection>::const_iterator citConnection;
    for( citConnection = connections.begin(); citConnection != connections.end(); citConnection++ )
        ( *citConnection ).accept_serializer( *this );
}

////////////////////////////////////////////////////////////////////////////////

void maya_magma_standard_deserializer::visit( desc::maya_magma_desc_input_socket* e ) {
    // TODO use boost::regex instead
    // {m_descNodeID, inputSocketIndex, enumAttrName, inputSocketAttrName0, inputSocketAttrName1, inputSocketAttrName2,
    // ...}
    assert( e != NULL );
    frantic::tstring sbuffer( frantic::convert::utf8_istream_to_tstring( m_in, m_length ) );

    std::size_t startIndex;
    std::size_t endIndex;

    startIndex = 1;
    endIndex = sbuffer.find( ',', startIndex );
    e->set_desc_node_id( boost::lexical_cast<desc_id>( sbuffer.substr( startIndex, endIndex - startIndex ) ) );
    startIndex = endIndex + 1;

    endIndex = sbuffer.find( ',', startIndex );
    e->set_desc_socket_index(
        boost::lexical_cast<desc_index_type>( sbuffer.substr( startIndex, endIndex - startIndex ) ) );
    startIndex = endIndex + 1;

    endIndex = sbuffer.find( ',', startIndex );
    if( endIndex == std::string::npos ) {
        e->set_maya_enum_attr_name( sbuffer.substr( startIndex, sbuffer.size() - startIndex - 1 ) );
    } else {
        e->set_maya_enum_attr_name( sbuffer.substr( startIndex, endIndex - startIndex ) );
        startIndex = endIndex + 1;

        endIndex = sbuffer.find( ',', startIndex );
        while( endIndex != std::string::npos ) {
            e->push_maya_attr_name( sbuffer.substr( startIndex, endIndex - startIndex ) );
            startIndex = endIndex + 1;
            endIndex = sbuffer.find( ',', startIndex );
        }

        // push the last maya attr name
        e->push_maya_attr_name( sbuffer.substr( startIndex, sbuffer.size() - startIndex - 1 ) );
    }
}

void maya_magma_standard_deserializer::visit( desc::maya_magma_desc_node* e ) {
    frantic::tstring sbuffer( frantic::convert::utf8_istream_to_tstring( m_in, m_length ) );

    bool ok = false;
    if( sbuffer.size() >= 2 ) {
        const int nodeType = 1;
        const int nodeProperty = 2;
        int type = -1;
        if( sbuffer[0] == '<' && sbuffer[sbuffer.size() - 1] == '>' ) {
            type = nodeType;
        } else if( sbuffer[0] == '(' && sbuffer[sbuffer.size() - 1] == ')' ) {
            type = nodeProperty;
        }

        // This was changed from the regex implementation since it was not doing what I expected it to do, particularly
        // in handling special characters like spaces Matches (<[e1],[e2],[e3],...>)
        std::vector<frantic::tstring> matches;
        frantic::tstring currentToken;
        frantic::tstring::const_iterator beginning = sbuffer.begin() + 1;
        frantic::tstring::const_iterator ending = sbuffer.end() - 1;
        for( frantic::tstring::const_iterator iter = beginning; iter != ending; ++iter ) {
            char current = *iter;
            if( current == ',' ) {
                matches.push_back( currentToken );
                currentToken.clear();
            } else {
                currentToken += current;
            }
        }
        matches.push_back( currentToken ); // Add the last element

        if( type == nodeType ) {
            if( matches.size() >= 4 ) {
                e->set_desc_node_id( boost::lexical_cast<desc_id>( matches[0] ) );
                e->set_node_type( matches[1] );
                int x = boost::lexical_cast<int>( matches[2] );
                int y = boost::lexical_cast<int>( matches[3] );
                e->set_position( x, y );
            }

            if( matches.size() >= 5 ) {
                // convert to proper string
                frantic::tstring nodeName( matches[4] );
                e->set_node_name( nodeName );
            }

            if( matches.size() >= 6 ) {
                // set the parent node
                int id = boost::lexical_cast<int>( matches[5] );
                e->set_parent( id );
            }

        } else if( type == nodeProperty ) {
            if( matches.size() >= 3 ) {
                e->set_desc_node_id( boost::lexical_cast<desc_id>( matches[0] ) );
                e->set_node_property_maya_attr_name( matches[1], matches[2] );
            }
        }
    }
}

void maya_magma_standard_deserializer::visit( desc::maya_magma_desc_connection* e ) {
    std::string sbuffer = frantic::strings::to_utf8( frantic::convert::utf8_istream_to_tstring( m_in, m_length ) );

    boost::regex connectExp( "^\\[\\s*(\\S+?)\\s*,\\s*(\\S+)?\\s*,\\s*(\\S+?)\\s*,\\s*(\\S+?)\\s*\\]$" );
    boost::match_results<std::string::const_iterator> what;

    if( boost::regex_search( sbuffer, what, connectExp, boost::match_default ) ) {
        e->set_src_desc_node_id( boost::lexical_cast<desc_id>( frantic::tstring( what[1].first, what[1].second ) ) );
        e->set_src_socket_index(
            boost::lexical_cast<desc_index_type>( frantic::tstring( what[2].first, what[2].second ) ) );
        e->set_dst_desc_node_id( boost::lexical_cast<desc_id>( frantic::tstring( what[3].first, what[3].second ) ) );
        e->set_dst_socket_index(
            boost::lexical_cast<desc_index_type>( frantic::tstring( what[4].first, what[4].second ) ) );
    }
}

void maya_magma_standard_deserializer::visit( desc::maya_magma_desc* e ) {
    assert( e != NULL );

    std::string sbuffer( frantic::strings::to_utf8( frantic::convert::utf8_istream_to_tstring( m_in, m_length ) ) );

    // map the desc_id in the stream to current one, key: id in the stream, value: new-desc id
    // the reason I am doing this because it might be a case that the desc id in the stream will be assigned different
    // value when we insert them into current maya_magma_desc. for example, where maya_magma_desc already have a node
    // with desc_id 0, but, in the stream, there also has a node with desc_id 0. However, the stream desc id has a
    // different meaning than the node already been created maya_magma_desc. so we have to cope with this case. the
    // solution is to introduce `descIDMaps` this also enforce the nodeID information has to present before actal detail
    // informations
    std::map<desc_id, desc_id> descIDMaps;

    std::size_t startIndex = 0;
    std::size_t endIndex;

    ////////////// 5/26/15 - PropertyQuery Pre-2.4 Legacy Load Hack ////////////////////
    // This can be removed when it is no longer necessary to support maya files Pre-2.4

    // Keep a list of the ids of PropertyQuery nodes
    std::vector<desc_id> propertyQueryNodeIds;
    std::vector<desc_id> noiseNodeIds;
    std::vector<desc_id> vecNoiseNodeIds;
    std::vector<desc_id> nearestPointNodeIds;
    std::vector<desc_id> intersectRayNodeIds;

    ////////////// End Hack ////////////////////////////////////////////////////////////

    while( startIndex < sbuffer.size() ) {
        char startCh = sbuffer[startIndex];
        char endCh;

        switch( startCh ) {
        case '<':
            endCh = '>';
            break;
        case '(':
            endCh = ')';
            break;
        case '[':
            endCh = ']';
            break;
        case '{':
            endCh = '}';
            break;
        default:
            throw maya_magma_exception(
                frantic::strings::to_tstring( "maya_magma_standard_deserializer::visit invalid format brackets " +
                                              boost::lexical_cast<std::string>( startCh ) ) );
        }

        endIndex = sbuffer.find( endCh, startIndex );
        std::istringstream istream;
        istream.str( sbuffer.substr( startIndex, endIndex - startIndex + 1 ) ); // include delimiter
        maya_magma_standard_deserializer tempDeserializer( istream, endIndex - startIndex + 1 );

        if( startCh == '<' ) {
            // <desc-id, nodeType, x, y, user description, parent ID>
            maya_magma_desc_node tempNode;
            tempNode.accept_deserializer( tempDeserializer );

            desc_id targetDescID( -1 );
            desc_id streamDescID = tempNode.get_desc_node_id();
            std::map<desc_id, desc_id>::iterator it = descIDMaps.find( streamDescID );
            if( it == descIDMaps.end() ) {
                // the new stream desc id
                targetDescID = e->set_node( tempNode );
                descIDMaps[streamDescID] = targetDescID;

                ////////////// 5/26/15 - PropertyQuery Pre-2.4 Legacy Load Hack ////////////////////
                // This can be removed when it is no longer necessary to support maya files Pre-2.4
                if( tempNode.get_node_type() == _T( "PropertyQuery" ) ) {
                    propertyQueryNodeIds.push_back( tempNode.get_desc_node_id() );
                } else if( tempNode.get_node_type() == _T( "Noise" ) ) {
                    noiseNodeIds.push_back( tempNode.get_desc_node_id() );
                } else if( tempNode.get_node_type() == _T( "VecNoise" ) ) {
                    vecNoiseNodeIds.push_back( tempNode.get_desc_node_id() );
                } else if( tempNode.get_node_type() == _T( "IntersectRay" ) ) {
                    intersectRayNodeIds.push_back( tempNode.get_desc_node_id() );
                } else if( tempNode.get_node_type() == _T( "NearestPoint" ) ) {
                    nearestPointNodeIds.push_back( tempNode.get_desc_node_id() );
                }
                ////////////// End Hack ////////////////////////////////////////////////////////////

            } else {
                // user can only define one nodeType for per desc-id otherwise throw a
                throw maya_magma_exception( _T( "maya_magma_standard_deserializer::visit duplicate node_type " ) +
                                            tempNode.get_node_type() );
            }
        } else if( startCh == '(' ) {
            // (desc-id,propertyName,MayaProperty)
            maya_magma_desc_node tempNode;
            tempNode.accept_deserializer( tempDeserializer );

            desc_id targetDescID( -1 );
            desc_id streamDescID = tempNode.get_desc_node_id();
            std::map<desc_id, desc_id>::iterator it = descIDMaps.find( streamDescID );
            if( it == descIDMaps.end() ) {
                throw maya_magma_exception( _T( "maya_magma_standard_deserializer::visit unknown desc id " ) +
                                            frantic::convert::to_tstring( streamDescID ) );
            } else {
                targetDescID = it->second;
            }
            std::vector<frantic::tstring> streamPropertyNames = tempNode.get_node_property_names();
            assert( streamPropertyNames.size() == 1 );
            e->set_node_property_maya_attr_name( targetDescID, streamPropertyNames[0],
                                                 tempNode.get_node_property_maya_attr_name( streamPropertyNames[0] ) );
        } else if( startCh == '{' ) {
            // {m_descNodeID, inputSocketIndex, enumAttrName}
            // {m_descNodeID, inputSocketIndex, enumAttrName, inputSocketAttrName0}
            // {m_descNodeID, inputSocketIndex, enumAttrName, inputSocketAttrName0, inputSocketAttrName1,
            // inputSocketAttrName2, ...}
            maya_magma_desc_input_socket tempInputSocket;
            tempInputSocket.accept_deserializer( tempDeserializer );

            desc_id targetDescID( -1 );
            desc_id streamDescID = tempInputSocket.get_desc_node_id();
            std::map<desc_id, desc_id>::iterator it = descIDMaps.find( streamDescID );
            if( it == descIDMaps.end() ) {
                throw maya_magma_exception( _T( "maya_magma_standard_deserializer::visit unknown desc id " ) +
                                            frantic::convert::to_tstring( streamDescID ) );
            } else {
                targetDescID = it->second;
            }

            e->set_node_input_socket_maya_enum_attr_name( targetDescID, tempInputSocket.get_desc_socket_index(),
                                                          tempInputSocket.get_maya_enum_attr_name() );
            const std::vector<frantic::tstring>& inputSocketMayaAttrNames =
                tempInputSocket.get_input_socket_maya_attr_names();

            std::vector<frantic::tstring>::const_iterator cit;
            for( cit = inputSocketMayaAttrNames.begin(); cit != inputSocketMayaAttrNames.end(); cit++ )
                e->push_node_input_socket_maya_attr_name( targetDescID, tempInputSocket.get_desc_socket_index(), *cit );
        } else if( startCh == '[' ) {
            // [src-id,src-socketIndex,dst-id,dst-SocketIndex]
            maya_magma_desc_connection tempConnection;
            tempConnection.accept_deserializer( tempDeserializer );

            desc_id srcStreamDescNodeID = tempConnection.get_src_desc_node_id();
            desc_id dstStreamDescNodeID = tempConnection.get_dst_desc_node_id();

            desc_id srcTargetDescNodeID;
            desc_id dstTargetDescNodeID;

            std::map<desc_id, desc_id>::iterator it;
            it = descIDMaps.find( srcStreamDescNodeID );
            if( it == descIDMaps.end() )
                throw maya_magma_exception( _T( "maya_magma_standard_deserializer::visit stream desc node id:" ) +
                                            frantic::convert::to_tstring( srcStreamDescNodeID ) +
                                            _T( " not found in descMap" ) );
            else
                srcTargetDescNodeID = it->second;

            it = descIDMaps.find( dstStreamDescNodeID );
            if( it == descIDMaps.end() )
                throw maya_magma_exception( _T( "maya_magma_standard_deserializer::visit stream desc node id:" ) +
                                            frantic::convert::to_tstring( dstStreamDescNodeID ) +
                                            _T( " not found in descMap" ) );
            else
                dstTargetDescNodeID = it->second;
            e->create_connection( srcTargetDescNodeID, tempConnection.get_src_socket_index(), dstTargetDescNodeID,
                                  tempConnection.get_dst_socket_index() );
        }
        startIndex = endIndex + 1;
    }

    ////////////// 5/26/15 - PropertyQuery Pre-2.4 Legacy Load Hack ////////////////////
    // This can be removed when it is no longer necessary to support maya files Pre-2.4

    // We now go through each PropertyQuery node and make sure it has metadata

    std::vector<desc_id>::iterator it = propertyQueryNodeIds.begin();
    for( it; it != propertyQueryNodeIds.end(); ++it ) {
        try {
            e->get_desc_node_input_socket_maya_enum_attr_name( *it, 1 );
        } catch( maya_magma_exception& /*e*/ ) {
            e->set_node_input_socket_maya_enum_attr_name(
                *it, 1,
                _T( "PropertyQuery_" ) + boost::lexical_cast<frantic::tstring>( *it ) + _T("_InputSocket1_enum" ) );
            e->push_node_input_socket_maya_attr_name(
                *it, 1,
                _T( "PropertyQuery_" ) + boost::lexical_cast<frantic::tstring>( *it ) + _T( "_InputSocket1_int" ) );
        }
    }

    // 2.6

    it = intersectRayNodeIds.begin();
    for( it; it != intersectRayNodeIds.end(); ++it ) {
        try {
            e->get_desc_node_input_socket_maya_enum_attr_name( *it, 3 );
        } catch( maya_magma_exception& /*e*/ ) {
            e->set_node_input_socket_maya_enum_attr_name(
                *it, 1,
                _T( "IntersectRay_" ) + boost::lexical_cast<frantic::tstring>( *it ) + _T( "_InputSocket1_enum" ) );
            e->push_node_input_socket_maya_attr_name(
                *it, 1,
                _T( "IntersectRay_" ) + boost::lexical_cast<frantic::tstring>( *it ) + _T( "_InputSocket1_vector3f" ) );
            e->set_node_input_socket_maya_enum_attr_name(
                *it, 2,
                _T( "IntersectRay_" ) + boost::lexical_cast<frantic::tstring>( *it ) + _T( "_InputSocket2_enum" ) );
            e->push_node_input_socket_maya_attr_name(
                *it, 2,
                _T( "IntersectRay_" ) + boost::lexical_cast<frantic::tstring>( *it ) + _T( "_InputSocket2_vector3f" ) );
            e->set_node_input_socket_maya_enum_attr_name(
                *it, 3,
                _T( "IntersectRay_" ) + boost::lexical_cast<frantic::tstring>( *it ) + _T( "_InputSocket3_enum" ) );
            e->push_node_input_socket_maya_attr_name(
                *it, 3,
                _T( "IntersectRay_" ) + boost::lexical_cast<frantic::tstring>( *it ) + _T( "_InputSocket3_bool" ) );
        }
    }

    it = nearestPointNodeIds.begin();
    for( it; it != nearestPointNodeIds.end(); ++it ) {
        try {
            e->get_desc_node_input_socket_maya_enum_attr_name( *it, 2 );
        } catch( maya_magma_exception& /*e*/ ) {
            e->set_node_input_socket_maya_enum_attr_name(
                *it, 1,
                _T( "IntersectRay_" ) + boost::lexical_cast<frantic::tstring>( *it ) + _T( "_InputSocket1_enum" ) );
            e->push_node_input_socket_maya_attr_name(
                *it, 1,
                _T( "IntersectRay_" ) + boost::lexical_cast<frantic::tstring>( *it ) + _T( "_InputSocket1_vector3f" ) );
            e->set_node_input_socket_maya_enum_attr_name(
                *it, 2,
                _T( "NearestPoint_" ) + boost::lexical_cast<frantic::tstring>( *it ) + _T( "_InputSocket2_enum" ) );
            e->push_node_input_socket_maya_attr_name(
                *it, 2,
                _T( "NearestPoint_" ) + boost::lexical_cast<frantic::tstring>( *it ) + _T( "_InputSocket2_bool" ) );
        }
    }

    it = noiseNodeIds.begin();
    for( it; it != noiseNodeIds.end(); ++it ) {
        try {
            e->get_desc_node_input_socket_maya_enum_attr_name( *it, 0 );
        } catch( maya_magma_exception& /*e*/ ) {
            e->set_node_input_socket_maya_enum_attr_name(
                *it, 0, _T( "Noise_" ) + boost::lexical_cast<frantic::tstring>( *it ) + _T( "_InputSocket0_enum" ) );
            e->push_node_input_socket_maya_attr_name( *it, 0,
                                                      _T( "Noise_" ) + boost::lexical_cast<frantic::tstring>( *it ) +
                                                          _T( "_InputSocket0_vector3f" ) );
            e->push_node_input_socket_maya_attr_name(
                *it, 0, _T( "Noise_" ) + boost::lexical_cast<frantic::tstring>( *it ) + _T( "_InputSocket0_float" ) );
            e->set_node_input_socket_maya_enum_attr_name(
                *it, 1, _T( "Noise_" ) + boost::lexical_cast<frantic::tstring>( *it ) + _T( "_InputSocket1_enum" ) );
            e->push_node_input_socket_maya_attr_name(
                *it, 1, _T( "Noise_" ) + boost::lexical_cast<frantic::tstring>( *it ) + _T( "_InputSocket1_float" ) );
        }
    }

    it = vecNoiseNodeIds.begin();
    for( it; it != vecNoiseNodeIds.end(); ++it ) {
        try {
            e->get_desc_node_input_socket_maya_enum_attr_name( *it, 0 );
        } catch( maya_magma_exception& /*e*/ ) {
            e->set_node_input_socket_maya_enum_attr_name(
                *it, 0, _T( "VecNoise_" ) + boost::lexical_cast<frantic::tstring>( *it ) + _T( "_InputSocket0_enum" ) );
            e->push_node_input_socket_maya_attr_name( *it, 0,
                                                      _T( "VecNoise_" ) + boost::lexical_cast<frantic::tstring>( *it ) +
                                                          _T( "_InputSocket0_vector3f" ) );
            e->push_node_input_socket_maya_attr_name( *it, 0,
                                                      _T( "VecNoise_" ) + boost::lexical_cast<frantic::tstring>( *it ) +
                                                          _T( "_InputSocket0_float" ) );
            e->set_node_input_socket_maya_enum_attr_name(
                *it, 1, _T( "VecNoise_" ) + boost::lexical_cast<frantic::tstring>( *it ) + _T( "_InputSocket1_enum" ) );
            e->push_node_input_socket_maya_attr_name( *it, 1,
                                                      _T( "VecNoise_" ) + boost::lexical_cast<frantic::tstring>( *it ) +
                                                          _T( "_InputSocket1_float" ) );
        }
    }

    ////////////// End Hack ////////////////////////////////////////////////////////////
}

} // namespace maya
} // namespace magma
} // namespace frantic
