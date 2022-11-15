// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "stdafx.h"

#include "frantic/maya/convert.hpp"
#include "frantic/maya/parser_args.hpp"
#include "frantic/maya/selection.hpp"
#include <frantic/logging/logging_level.hpp>

#include "frantic/magma/maya/maya_magma_attr_manager.hpp"
#include "frantic/magma/maya/maya_magma_gui.hpp"
#include "frantic/magma/maya/maya_magma_info.hpp"
#include "frantic/magma/maya/maya_magma_info_factory.hpp"
#include "frantic/magma/maya/maya_magma_mel_connection.hpp"
#include "frantic/magma/maya/maya_magma_mel_window.hpp"

namespace frantic {
namespace magma {
namespace maya {
namespace mel {

const MString maya_magma_mel_create_connection::commandName = "create_magma_connection";

void* maya_magma_mel_create_connection::creator() { return new maya_magma_mel_create_connection; }

MStatus maya_magma_mel_create_connection::parseArgs( const MArgList& args,
                                                     frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                                                     MFnDependencyNode& depNode ) {
    MStatus outStatus = MS::kFailure;

    int srcDescID, srcSocketIndex;
    int dstDescID, dstSocketIndex;
    if( MS::kSuccess != frantic::maya::parser_args::get_arg( args, "-si", "-sourceID", srcDescID ) )
        throw maya_magma_exception( "maya_magma_mel_create_connection::parseArgs sourceID has not specified" );
    if( MS::kSuccess != frantic::maya::parser_args::get_arg( args, "-ss", "-sourceSocketIndex", srcSocketIndex ) )
        throw maya_magma_exception( "maya_magma_mel_create_connection::parseArgs sourceSocketIndex has not specified" );
    if( MS::kSuccess != frantic::maya::parser_args::get_arg( args, "-di", "-destinationID", dstDescID ) )
        throw maya_magma_exception( "maya_magma_mel_create_connection::parseArgs destinationID has not specified" );
    if( MS::kSuccess != frantic::maya::parser_args::get_arg( args, "-ds", "-destinationSocketIndex", dstSocketIndex ) )
        throw maya_magma_exception(
            "maya_magma_mel_create_connection::parseArgs destinationSocketIndex has not specified" );

    desc->create_connection( srcDescID, srcSocketIndex, dstDescID, dstSocketIndex );

    setResult( "Success" );
    outStatus = MS::kSuccess;

    maya_MagmaFLUX* magma = maya_magma_mel_window::findMagmaFLUXWidgetByNode( depNode );
    if( magma != NULL ) {
        magma->addConnection( srcDescID, srcSocketIndex, dstDescID, dstSocketIndex );
    }

    return outStatus;
}

////////////////////////////////////////////////////////////////////////////////

const MString maya_magma_mel_delete_connection::commandName = "delete_magma_connection";

void* maya_magma_mel_delete_connection::creator() { return new maya_magma_mel_delete_connection; }

MStatus maya_magma_mel_delete_connection::parseArgs( const MArgList& args,
                                                     frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                                                     MFnDependencyNode& depNode ) {
    MStatus outStatus = MS::kFailure;

    int srcDescID, srcSocketIndex;
    int dstDescID, dstSocketIndex;
    if( MS::kSuccess != frantic::maya::parser_args::get_arg( args, "-si", "-sourceID", srcDescID ) )
        throw maya_magma_exception( "maya_magma_mel_delete_connection::parseArgs sourceID was not specified" );
    if( MS::kSuccess != frantic::maya::parser_args::get_arg( args, "-ss", "-sourceSocketIndex", srcSocketIndex ) )
        throw maya_magma_exception( "maya_magma_mel_delete_connection::parseArgs sourceSocketIndex was not specified" );
    if( MS::kSuccess != frantic::maya::parser_args::get_arg( args, "-di", "-destinationID", dstDescID ) )
        throw maya_magma_exception( "maya_magma_mel_delete_connection::parseArgs destinationID was not specified" );
    if( MS::kSuccess != frantic::maya::parser_args::get_arg( args, "-ds", "-destinationSocketIndex", dstSocketIndex ) )
        throw maya_magma_exception(
            "maya_magma_mel_delete_connection::parseArgs destinationSocketIndex was not specified" );

    desc->delete_connection( srcDescID, srcSocketIndex, dstDescID, dstSocketIndex );

    setResult( "Success" );
    outStatus = MS::kSuccess;

    maya_MagmaFLUX* magma = maya_magma_mel_window::findMagmaFLUXWidgetByNode( depNode );
    if( magma != NULL ) {
        magma->removeConnection( srcDescID, srcSocketIndex, dstDescID, dstSocketIndex );
    }

    return outStatus;
}

////////////////////////////////////////////////////////////////////////////////

const MString maya_magma_mel_get_connections::commandName = "get_magma_connections";

void* maya_magma_mel_get_connections::creator() { return new maya_magma_mel_get_connections; }

MStatus maya_magma_mel_get_connections::parseArgs( const MArgList& args,
                                                   frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                                                   MFnDependencyNode& depNode ) {
    MStatus outStatus = MS::kFailure;
    FF_LOG( debug ) << "maya_magma_mel_get_connections:" << std::endl;

    int descID;
    if( MS::kSuccess == frantic::maya::parser_args::get_arg( args, "-i", "-id", descID ) ) {

        const std::map<desc::desc_id, desc::maya_magma_desc_node>& nodes = desc->get_nodes();
        std::map<desc::desc_id, desc::maya_magma_desc_node>::const_iterator iter = nodes.find( descID );
        if( iter == nodes.end() ) {
            throw maya_magma_exception( "maya_magma_mel_get_connections: no node of given ID" );
        }

        info::maya_magma_node_info magmaNodeInfo;
        try {
            magmaNodeInfo =
                factory::maya_magma_node_info_factory::create_node_infos( iter->second.get_node_type().c_str() );
        } catch( maya_magma_exception& e ) {
            // Invalid node type.  Sockets unknown?
            FF_LOG( debug ) << "maya_magma_mel_get_connections: warning: " << e.what() << std::endl;
            return outStatus;
        }

        unsigned int numInputs;
        if( desc->get_node_type( descID ) == _T("Mux") ) {
            frantic::tstring attribute = desc->get_maya_attr_name_from_node_property( descID, _T("NumInputs") );
            holder::property_variant_t attributeValue =
                attr::maya_magma_attr_manager::get_property_value_from_maya_attrs( attribute, depNode );
            numInputs = (unsigned int)boost::get<int>( attributeValue );
        } else {
            numInputs = (unsigned int)magmaNodeInfo.m_inputSocketInfos.size();
        }

        int srcSocketIndex;
        if( MS::kSuccess == frantic::maya::parser_args::get_arg( args, "-ss", "-sourceSocketIndex", srcSocketIndex ) ) {
            std::vector<desc::maya_magma_desc_connection> edges( desc->get_node_connections( descID, false, true ) );
            MIntArray result;
            for( std::vector<desc::maya_magma_desc_connection>::const_iterator iter = edges.begin();
                 iter != edges.end(); ++iter ) {
                int id = static_cast<int>( iter->get_src_desc_node_id() );
                int sockID = static_cast<int>( iter->get_src_socket_index() );
                if( id == descID && srcSocketIndex == sockID ) {

                    int dstID = static_cast<int>( iter->get_dst_desc_node_id() );
                    int dstSock = static_cast<int>( iter->get_dst_socket_index() );

                    result.append( dstID );
                    result.append( dstSock );
                }
            }
            setResult( result );

        } else {
            std::vector<desc::maya_magma_desc_connection> edges( desc->get_node_connections( descID, true, false ) );
            MIntArray result( 2 * numInputs, -1 );
            for( std::vector<desc::maya_magma_desc_connection>::const_iterator iter = edges.begin();
                 iter != edges.end(); ++iter ) {
                int id = static_cast<int>( iter->get_dst_desc_node_id() );
                if( id == descID ) {
                    int index = static_cast<int>( iter->get_dst_socket_index() );
                    result[2 * index + 0] = iter->get_src_desc_node_id();
                    result[2 * index + 1] = iter->get_src_socket_index();
                }
            }
            setResult( result );
        }
        outStatus = MS::kSuccess;
    }
    return outStatus;
}

////////////////////////////////////////////////////////////////////////////////

const MString maya_magma_mel_set_connection_properties::commandName = "set_magma_connection";

void* maya_magma_mel_set_connection_properties::creator() { return new maya_magma_mel_set_connection_properties; }

MStatus maya_magma_mel_set_connection_properties::parseArgs( const MArgList& args,
                                                             frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                                                             MFnDependencyNode& depNode ) {
    MStatus outStatus = MS::kFailure;
    FF_LOG( debug ) << "set_magma_connection:" << std::endl;

    int srcDescID, srcSocketIndex;
    int dstDescID, dstSocketIndex;
    if( MS::kSuccess != frantic::maya::parser_args::get_arg( args, "-si", "-sourceID", srcDescID ) )
        throw maya_magma_exception( "set_magma_connection::parseArgs sourceID was not specified" );
    if( MS::kSuccess != frantic::maya::parser_args::get_arg( args, "-ss", "-sourceSocketIndex", srcSocketIndex ) )
        throw maya_magma_exception( "set_magma_connection::parseArgs sourceSocketIndex was not specified" );
    if( MS::kSuccess != frantic::maya::parser_args::get_arg( args, "-di", "-destinationID", dstDescID ) )
        throw maya_magma_exception( "set_magma_connection::parseArgs destinationID was not specified" );
    if( MS::kSuccess != frantic::maya::parser_args::get_arg( args, "-ds", "-destinationSocketIndex", dstSocketIndex ) )
        throw maya_magma_exception( "set_magma_connection::parseArgs destinationSocketIndex was not specified" );

    int r, g, b, a;
    bool isSetR = false, isSetG = false, isSetB = false, isSetA = false;
    if( MS::kSuccess == frantic::maya::parser_args::get_arg( args, "-r", "-red", r ) ) {
        isSetR = true;
    }
    if( MS::kSuccess == frantic::maya::parser_args::get_arg( args, "-g", "-green", g ) ) {
        isSetG = true;
    }
    if( MS::kSuccess == frantic::maya::parser_args::get_arg( args, "-b", "-blue", b ) ) {
        isSetB = true;
    }
    if( MS::kSuccess == frantic::maya::parser_args::get_arg( args, "-a", "-alpha", a ) ) {
        isSetA = true;
    }

    maya_MagmaFLUX* magma = maya_magma_mel_window::findMagmaFLUXWidgetByNode( depNode );
    if( magma == NULL )
        return outStatus;

    unsigned char rgba[4];
    bool ok = magma->getConnectionColor( srcDescID, srcSocketIndex, dstDescID, dstSocketIndex, rgba );
    if( !ok )
        return outStatus;

    if( isSetR )
        rgba[0] = ( r < 0 ? 0 : ( r > 255 ? 255 : (unsigned char)r ) );
    if( isSetG )
        rgba[1] = ( g < 0 ? 0 : ( g > 255 ? 255 : (unsigned char)g ) );
    if( isSetB )
        rgba[2] = ( b < 0 ? 0 : ( b > 255 ? 255 : (unsigned char)b ) );
    if( isSetA )
        rgba[3] = ( a < 0 ? 0 : ( a > 255 ? 255 : (unsigned char)a ) );
    magma->setConnectionColor( srcDescID, srcSocketIndex, dstDescID, dstSocketIndex, rgba );

    outStatus = MS::kSuccess;
    return outStatus;
}

} // namespace mel
} // namespace maya
} // namespace magma
} // namespace frantic
