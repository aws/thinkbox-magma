// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "frantic/magma/maya/maya_magma_context_interface.hpp"
#include "frantic/magma/maya/maya_magma_modifier.hpp"
#include "frantic/magma/maya/maya_magma_singleton.hpp"
#include "frantic/magma/simple_compiler/magma_particle_istream.hpp"

#include "frantic/magma/maya/maya_magma_desc_mpxdata.hpp"
#include "frantic/magma/maya/maya_magma_description.hpp"
#include "frantic/magma/maya/maya_magma_holder.hpp"

#include "frantic/magma/maya/maya_magma_attr_manager.hpp"
#include "frantic/magma/maya/maya_magma_common.hpp"
#include "frantic/magma/maya/maya_magma_exception.hpp"

#include "frantic/convert/tstring.hpp"
#include "frantic/strings/tstring.hpp"

#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnPluginData.h>
#include <maya/MPlug.h>
#include <maya/MPxData.h>

#include <assert.h>

#include <map>
#include <memory>
#include <utility>

namespace frantic {
namespace magma {
namespace maya {
namespace modifier {

namespace detail {

void convert_desc_node_to_holder_node( desc::maya_magma_desc_ptr desc, holder::maya_magma_holder& mayaMagmaHolder,
                                       std::map<desc::desc_id, holder::magma_id>& idMaps ) {
    const std::map<desc::desc_id, desc::maya_magma_desc_node>& descNodes = desc->get_nodes();

    std::map<desc::desc_id, desc::maya_magma_desc_node>::const_iterator citNode;
    for( citNode = descNodes.begin(); citNode != descNodes.end(); citNode++ ) {
        // get the current descNode id
        desc::desc_id descNodeID = citNode->first;
        const desc::maya_magma_desc_node& descNode = citNode->second;

        // we try to find whether the current descNode id is already in idMaps
        if( idMaps.find( descNodeID ) == idMaps.end() ) {

            desc::desc_id parent = descNode.get_parent();

            // Certain nodes don't have to be added since Magma adds them automatically
            frantic::tstring nodeType = descNode.get_node_type();
            holder::magma_id holderNodeID;
            if( nodeType == _T("BLOPSocket") || nodeType == _T("Loop__Input") ) {
                holderNodeID = mayaMagmaHolder.get_BLOP_source_id( idMaps[parent] );

            } else if( nodeType == _T("BLOPOutput") || nodeType == _T("Loop__Output") ) {
                holderNodeID = mayaMagmaHolder.get_BLOP_sink_id( idMaps[parent] );

            } else {

                if( parent >= 0 )
                    mayaMagmaHolder.push_editable_BLOP( parent );

                // get the current desc node type
                holderNodeID = mayaMagmaHolder.create_node( descNode.get_node_type() );

                if( parent >= 0 )
                    mayaMagmaHolder.pop_editable_BLOP();
            }

            // throw a exception if a new holder node can't node be created
            if( holderNodeID == holder::kInvalidMagmaID )
                throw maya_magma_exception(
                    "convert_desc_node_to_holder_node could not create a node in mayaMagmaHolder" );

            // mapping the current desc node id to the newly created holder node id
            idMaps[descNodeID] = holderNodeID;

            FF_LOG( debug ) << "convert_desc_node_to_holder_node: Mapping between descNodeID:" << descNodeID
                            << ", mamgaNodeID:" << holderNodeID << std::endl;
        } else {
            // if the current descNode id is already in idMaps, there must be a error; because this can't happen
            // unless `convert_desc_node_holder_node` called more than once
            throw maya_magma_exception(
                "detail::convert_desc_node_holder_node duplicated desc ids appear in maya_magma_desc" );
        }
    }
}

void convert_desc_connection_to_holder_connection( desc::maya_magma_desc_ptr desc,
                                                   holder::maya_magma_holder& mayaMagmaHolder,
                                                   std::map<desc::desc_id, holder::magma_id>& idMaps ) {
    const std::vector<desc::maya_magma_desc_connection>& descConnections = desc->get_connections();

    std::vector<desc::maya_magma_desc_connection>::const_iterator citConnection;
    for( citConnection = descConnections.begin(); citConnection != descConnections.end(); citConnection++ ) {
        const desc::maya_magma_desc_connection& connection = *citConnection;

        desc::desc_id srcDescNodeID = connection.get_src_desc_node_id();
        desc::desc_id dstDescNodeID = connection.get_dst_desc_node_id();

        holder::magma_id srcHolderNodeID = holder::kInvalidMagmaID;
        holder::magma_id dstHolderNodeID = holder::kInvalidMagmaID;

        // get the corresponding src holder node id
        if( idMaps.find( srcDescNodeID ) == idMaps.end() ) {
            throw maya_magma_exception( _T( "detail::convert_desc_connection_to_holder_connection id (" ) +
                                        convert::to_tstring( srcDescNodeID ) + _T( ") is not found in idMaps" ) );
        } else {
            srcHolderNodeID = idMaps[srcDescNodeID];
        }

        // get the corresponding dst holder node id
        if( idMaps.find( dstDescNodeID ) == idMaps.end() ) {
            throw maya_magma_exception( _T( "detail::convert_desc_connection_to_holder_connection id (" ) +
                                        frantic::convert::to_tstring( dstDescNodeID ) +
                                        _T( ") is not found in idMaps" ) );
        } else {
            dstHolderNodeID = idMaps[dstDescNodeID];
        }

        holder::index_type srcHolderSocketIndex = connection.get_src_socket_index();
        holder::index_type dstHolderSocketIndex = connection.get_dst_socket_index();

        FF_LOG( debug ) << "convert_desc_connection_to_holder_connection: set_node_input(" << srcHolderNodeID << ","
                        << srcHolderSocketIndex << "," << dstHolderNodeID << "," << dstHolderSocketIndex << ")"
                        << std::endl;
        mayaMagmaHolder.set_node_input( srcHolderNodeID, srcHolderSocketIndex, dstHolderNodeID, dstHolderSocketIndex );
    }
}

void set_holder_default_input_value( desc::maya_magma_desc_ptr desc, holder::maya_magma_holder& mayaMagmaHolder,
                                     std::map<desc::desc_id, holder::magma_id>& idMaps,
                                     const MFnDependencyNode& depNode ) {
    const std::map<desc::desc_id, desc::maya_magma_desc_node>& descNodes = desc->get_nodes();

    std::map<desc::desc_id, desc::maya_magma_desc_node>::const_iterator citNode;
    for( citNode = descNodes.begin(); citNode != descNodes.end(); citNode++ ) {
        desc::desc_id descNodeID = citNode->first;

        // get the corresponding magma node id (holder node id)
        holder::magma_id holderNodeID = holder::kInvalidMagmaID;
        if( idMaps.find( descNodeID ) == idMaps.end() ) {
            throw maya_magma_exception(
                _T( "detail::set_holder_default_input_value no corresponding magma id found for desc id (" ) +
                frantic::convert::to_tstring( descNodeID ) + _T( ")" ) );
        } else {
            holderNodeID = idMaps[descNodeID];
        }

        // get the current node's indexes
        std::vector<desc::desc_index_type> descSocketIndexes = citNode->second.get_node_socket_indexes();

        std::vector<desc::desc_index_type>::const_iterator citSocketIndex;
        for( citSocketIndex = descSocketIndexes.begin(); citSocketIndex != descSocketIndexes.end(); citSocketIndex++ ) {
            // because for each input socket, two types of maya attribute will be created
            // 1) a maya enum attr for user selecting current data type (int, float, vector3f)
            // 2) the actual maya attributes (int maya attribute, float maya attribute, vector3f maya attribute)
            // we have to pass these two types of information to `get_input_socket_value_from_maya_attrs` in order
            // to get the value from maya attribute

            frantic::tstring mayaEnumAttrName =
                citNode->second.get_node_input_socket_maya_enum_attr_name( *citSocketIndex );
            std::vector<frantic::tstring> mayaAttrNames =
                citNode->second.get_node_input_socket_maya_attr_names( *citSocketIndex );

            // get the socket input value from maya_magma_attr_manager
            holder::input_socket_variant_t value =
                attr::maya_magma_attr_manager::get_input_socket_value_from_maya_attrs( mayaEnumAttrName, mayaAttrNames,
                                                                                       depNode );
            FF_LOG( debug ) << "set_holder_default_input_value: set_node_input_default_value(" << holderNodeID << ","
                            << *citSocketIndex << "," << value << ")" << std::endl;

            // set the input value on the magma node, throw a exception if it fails
            if( !mayaMagmaHolder.set_node_input_default_value( holderNodeID, *citSocketIndex, value ) ) {
                throw maya_magma_exception(
                    _T( "detail::set_holder_default_input_value could not set input socket default value for magma-id:" ) +
                    convert::to_tstring( holderNodeID ) + _T( " inputSocketIndex: " ) +
                    convert::to_tstring( *citSocketIndex ) + _T( " with value:" ) + convert::to_tstring( value ) );
            }
        }
    }
}

void set_holder_property( desc::maya_magma_desc_ptr desc, holder::maya_magma_holder& mayaMagmaHolder,
                          std::map<desc::desc_id, holder::magma_id>& idMaps, const MFnDependencyNode& depNode ) {
    const std::map<desc::desc_id, desc::maya_magma_desc_node>& descNodes = desc->get_nodes();

    std::map<desc::desc_id, desc::maya_magma_desc_node>::const_iterator citNode;
    for( citNode = descNodes.begin(); citNode != descNodes.end(); citNode++ ) {
        desc::desc_id descNodeID = citNode->first;
        std::vector<frantic::tstring> propertyNames = citNode->second.get_node_property_names();

        // get the corresponding magma node id based on current desc node id if callers has
        // called `convert_desc_node_holder_node` before, we should be able to find the corresponding holder node id
        holder::magma_id holderNodeID = holder::kInvalidMagmaID;
        if( idMaps.find( descNodeID ) == idMaps.end() ) {
            throw maya_magma_exception(
                _T( "detail::set_holder_property no corresponding magma id found for desc id (" ) +
                frantic::convert::to_tstring( descNodeID ) + _T( ")" ) );
        } else {
            holderNodeID = idMaps[descNodeID];
        }

        // iterate through all property name in the current node
        std::vector<frantic::tstring>::const_iterator cit;
        for( cit = propertyNames.begin(); cit != propertyNames.end(); cit++ ) {
            // get the corresponding maya attribute name based on the current property name
            frantic::tstring propertyMayaAttributeName = citNode->second.get_node_property_maya_attr_name( *cit );

            // get the value of that maya attribute
            holder::property_variant_t value =
                attr::maya_magma_attr_manager::get_property_value_from_maya_attrs( propertyMayaAttributeName, depNode );
            FF_LOG( debug ) << "set_holder_property: holderNodeID:" << holderNodeID << " propertyName:" << *cit
                            << " value:" << value << std::endl;

            // Special properties that are handled differently from usual magma properties
            if( citNode->second.get_node_type() == _T("Mux") && *cit == _T("NumInputs") ) {
                int numInputs = boost::get<int>( value );
                if( numInputs < 3 )
                    // throw magma_exception()
                    //	<< magma_exception::node_id( holderNodeID )
                    //	<< magma_exception::property_name( "NumInputs" )
                    //	<< magma_exception::error_name( _T("detail::set_holder_property error: Number of inputs property
                    //for mux node must be >= 3.") );
                    throw maya_magma_exception(
                        _T( "detail::set_holder_property could not set property value for magmaID " ) +
                        convert::to_tstring( holderNodeID ) + _T( " property: \"" ) + *cit + _T( "\" with value:" ) +
                        convert::to_tstring( value ) + _T( ".  Values must be >= 3." ) );
                mayaMagmaHolder.set_num_node_inputs( holderNodeID, numInputs );
            }

            // Loop Node properties
            else if( citNode->second.get_node_type() == _T("Loop") && *cit == _T("outputMask") ) {
                // don't add it
            }

            // All other normal magma properties
            else {
                // call holder to set that node property value, throw a exception if it fails
                if( !mayaMagmaHolder.get_node_property_readonly( holderNodeID, *cit ) ) {
                    if( !mayaMagmaHolder.set_node_property( holderNodeID, *cit, value ) ) {
                        throw maya_magma_exception(
                            _T( "detail::set_holder_property could not set property value for magmaID " ) +
                            convert::to_tstring( holderNodeID ) + _T( " property: \"" ) + *cit +
                            _T( "\" with value:" ) + convert::to_tstring( value ) );
                    }
                } else {
                    FF_LOG( debug ) << "set_holder_property: property is readonly and ignored" << std::endl;
                }
            }
        }
    }
}

void convert_desc_to_holder( desc::maya_magma_desc_ptr desc, holder::maya_magma_holder& mayaMagmaHolder,
                             const MFnDependencyNode& depNode ) {
    // note here, magma node id is exact the same thing as holder node id;

    // this idMaps is not necessary but important to understand why I create them here
    // because if developers forget to clear mayaMagmaHolder before they recompiling current maya magma, there might be
    // a case where mayaMagmaHolder already has some nodes and these nodes are not created from the current desc nodes.
    // Therefore, I created a `idMaps` to map between current desc node id and the new created holder node id you may
    // see `convert_desc_node_to_holder_node` for more details
    std::map<desc::desc_id, holder::magma_id> idMaps;

    // desc nodes to holder nodes
    convert_desc_node_to_holder_node( desc, mayaMagmaHolder, idMaps );

    // desc property
    set_holder_property( desc, mayaMagmaHolder, idMaps, depNode );

    // desc default input value
    set_holder_default_input_value( desc, mayaMagmaHolder, idMaps, depNode );

    // desc connections to holder connections
    convert_desc_connection_to_holder_connection( desc, mayaMagmaHolder, idMaps );
}

} // namespace detail

void maya_magma_error_callback( const frantic::magma::magma_exception& e ) {
    FF_LOG( stats ) << "DEBUG: maya_magma_error_callback" << std::endl;
    throw e;
}

maya_magma_modifier::maya_magma_modifier( MObject& obj, const frantic::tstring& mayaMagmaPlugName,
                                          const frantic::graphics::transform4f& worldTransform,
                                          const frantic::graphics::transform4f& cameraTransform )
    : m_desc()
    , m_surShapeNode( obj )
    , m_worldTransform( worldTransform )
    , m_cameraTransform( cameraTransform ) {
    MStatus status;
    MFnDependencyNode depNode( obj, &status );
    throw_maya_magma_exception_if_not_success(
        status, _T( "maya_magma_modifier::maya_magma_modifier could not get dependency node from MObject" ) );

    MPlug plug = depNode.findPlug( mayaMagmaPlugName.c_str(), &status );
    throw_maya_magma_exception_if_not_success(
        status, _T( "maya_magma_modifier::maya_magma_modifier could not MPlug from MFnDependencyNode" ) );

    MObject mayaMagmaDataObj;
    status = plug.getValue( mayaMagmaDataObj );
    throw_maya_magma_exception_if_not_success(
        status, _T( "maya_magma_modifier::maya_magma_modifier could not MPlug's value" ) );

    MFnPluginData pluginData( mayaMagmaDataObj, &status );
    throw_maya_magma_exception_if_not_success(
        status, _T( "maya_magma_modifier::maya_magma_modifier could not construct MFnPluginData" ) );

    MPxData* data = pluginData.data( &status );
    throw_maya_magma_exception_if_not_success(
        status, _T( "maya_magma_modifier::maya_magma_modifier could not get MPxData from MFnPluginData" ) );

    const maya_magma_desc_mpxdata* pMayaMagmaMPxData = static_cast<const maya_magma_desc_mpxdata*>( data );
    assert( pMayaMagmaMPxData != NULL );

    m_desc = pMayaMagmaMPxData->get_maya_magma_desc();
}

maya_magma_modifier::~maya_magma_modifier() {}

maya_magma_modifier::particle_istream_ptr
maya_magma_modifier::get_modified_particle_istream( particle_istream_ptr inStream ) const {
    if( inStream->particle_count() == 0 ) {
        FF_LOG( debug ) << "inStream is empty; maya_magma_modifier ignores it" << std::endl;
        return inStream;
    }

    std::unique_ptr<magma_interface> mayaMagmaSingleton = maya_magma_singleton::get_instance().create_magma_instance();
    // this statement passes the ownship of maya_interface_impl in mayaMagmaSingleton to the mayaMagmaHolder
    holder::maya_magma_holder mayaMagmaHolder( std::move( mayaMagmaSingleton ) );

    // call clear before (DO NOT call this after compiling magma)
    mayaMagmaHolder.clear();

    // TODO get rid of this ?
    MStatus status;
    MFnDependencyNode depNode( m_surShapeNode, &status );
    throw_maya_magma_exception_if_not_success(
        status, _T( "maya_magma_modifier::get_modified_particle_istream could not get depNode from MObject" ) );

    // convert desc to mayaMagmaHolder
    detail::convert_desc_to_holder( m_desc, mayaMagmaHolder, depNode );

    // please remember get the current magma_interface from mayaMagmaHolder
    boost::shared_ptr<frantic::magma::magma_interface> magmaInterface( mayaMagmaHolder.get_magma_interface() );
    boost::shared_ptr<maya_magma_context_interface> mayaMagmaContextInterface(
        new maya_magma_context_interface( m_worldTransform, m_cameraTransform ) );

    particle_istream_ptr outStream;
    outStream = simple_compiler::apply_simple_compiler_expression( inStream, magmaInterface, mayaMagmaContextInterface,
                                                                   maya_magma_error_callback );

    return outStream;
}

} // namespace modifier
} // namespace maya
} // namespace magma
} // namespace frantic
