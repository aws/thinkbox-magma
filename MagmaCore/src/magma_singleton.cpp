// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/magma_singleton.hpp>

#include <frantic/magma/nodes/magma_blop_node.hpp>
#include <frantic/magma/nodes/magma_input_channel_node.hpp>
#include <frantic/magma/nodes/magma_loop_node.hpp>
#include <frantic/magma/nodes/magma_output_node.hpp>
#include <frantic/magma/nodes/magma_standard_operators.hpp>

#include <frantic/graphics/quat4f.hpp>

#include <boost/integer.hpp>

#include <memory>

namespace frantic {
namespace magma {

std::map<frantic::tstring, magma_data_type> magma_singleton::s_dataTypes;

magma_singleton::magma_singleton( bool defineStandardTypes ) {
    define_data_type<float>( _T("Float") );
    define_data_type<int>( _T("Int") );
    define_data_type<frantic::graphics::vector3f>( _T("Vec3") );
    define_data_type<frantic::graphics::vector3>( _T("IVec3") );
    define_data_type<frantic::graphics::quat4f>( _T("Quat") );

    typedef boost::int_t<sizeof( bool ) * CHAR_BIT>::fast bool_int_type;
    define_data_type<bool_int_type>( _T("Bool") );
    // define_data_type<int>( _T("Bool") ); //Old method used in earlier versions.

    if( !defineStandardTypes )
        return;

    define_node_type<nodes::magma_blop_node>();
    define_node_type<nodes::magma_blop_input_node>();
    define_node_type<nodes::magma_blop_output_node>();
    define_node_type<nodes::magma_loop_node>();
    define_node_type<nodes::magma_loop_inputs_node>();
    define_node_type<nodes::magma_loop_outputs_node>();
    define_node_type<nodes::magma_input_channel_node>();
    define_node_type<nodes::magma_output_node>();

    nodes::define_standard_operators( *this );
}

extern std::unique_ptr<magma_interface> create_magma_instance();

std::unique_ptr<magma_interface> magma_singleton::create_magma_instance_impl() {
    return frantic::magma::create_magma_instance();
}

std::unique_ptr<magma_interface> magma_singleton::create_magma_instance() {
    std::unique_ptr<magma_interface> result = create_magma_instance_impl();
    result->set_singleton( *this );
    return result;
}

std::size_t magma_singleton::get_num_node_types() const { return m_nodeTypes.size(); }

const frantic::tstring& magma_singleton::get_node_type_name( std::size_t i ) const {
    return m_nodeTypes[i]->get_name();
}

const magma_node_type* magma_singleton::get_named_node_type( const frantic::tstring& name ) const {
    std::map<frantic::tstring, std::size_t>::const_iterator it = m_nodeTypeMap.find( name );
    if( it == m_nodeTypeMap.end() )
        return NULL;
    return m_nodeTypes[it->second].get();
}

const magma_data_type* magma_singleton::get_named_data_type( const frantic::tstring& name ) {
    std::map<frantic::tstring, magma_data_type>::const_iterator it = s_dataTypes.find( name );
    if( it == s_dataTypes.end() )
        return NULL;
    return &it->second;
}

const magma_data_type* magma_singleton::get_matching_data_type( frantic::channels::data_type_t type,
                                                                std::size_t arity ) {
    for( std::map<frantic::tstring, magma_data_type>::const_iterator it = s_dataTypes.begin(),
                                                                     itEnd = s_dataTypes.end();
         it != itEnd; ++it ) {
        if( it->second.m_elementType == type && it->second.m_elementCount == arity )
            return &it->second;
    }
    return NULL;
}

void magma_singleton::get_predefined_particle_channels(
    std::vector<std::pair<frantic::tstring, magma_data_type>>& outChannels ) const {
    outChannels.push_back( std::make_pair( frantic::tstring( _T("Position") ), s_dataTypes[_T("Vec3")] ) );
    outChannels.push_back( std::make_pair( frantic::tstring( _T("Velocity") ), s_dataTypes[_T("Vec3")] ) );
    outChannels.push_back( std::make_pair( frantic::tstring( _T("Normal") ), s_dataTypes[_T("Vec3")] ) );
    outChannels.push_back( std::make_pair( frantic::tstring( _T("Density") ), s_dataTypes[_T("Float")] ) );
    outChannels.push_back( std::make_pair( frantic::tstring( _T("Orientation") ), s_dataTypes[_T("Quat")] ) );
    outChannels.push_back( std::make_pair( frantic::tstring( _T("Color") ), s_dataTypes[_T("Vec3")] ) );
    outChannels.push_back( std::make_pair( frantic::tstring( _T("TextureCoord") ), s_dataTypes[_T("Vec3")] ) );
    outChannels.push_back( std::make_pair( frantic::tstring( _T("ID") ), s_dataTypes[_T("Int")] ) );
}

void magma_singleton::get_predefined_vertex_channels(
    std::vector<std::pair<frantic::tstring, magma_data_type>>& outChannels ) const {
    outChannels.push_back( std::make_pair( frantic::tstring( _T("Position") ), s_dataTypes[_T("Vec3")] ) );
}

void magma_singleton::get_predefined_face_channels(
    std::vector<std::pair<frantic::tstring, magma_data_type>>& outChannels ) const {}

void magma_singleton::get_predefined_face_vertex_channels(
    std::vector<std::pair<frantic::tstring, magma_data_type>>& outChannels ) const {}

} // namespace magma
} // namespace frantic
