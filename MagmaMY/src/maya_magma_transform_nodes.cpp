// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "frantic/magma/maya/nodes/maya_magma_transform_nodes.hpp"

#include <frantic/magma/magma_compiler_interface.hpp>
#include <frantic/magma/nodes/magma_input_objects_interface.hpp>

#include <frantic/magma/nodes/magma_node_impl.hpp>

namespace frantic {
namespace magma {
namespace maya {
namespace nodes {

MAGMA_DEFINE_TYPE( "FromSpace", "Transform", maya_magma_from_space_node )
MAGMA_ENUM_PROPERTY( inputType, "Point", "Vector", "Normal" )
MAGMA_INPUT( "Vector", frantic::graphics::vector3f( 0, 0, 0 ) )
MAGMA_INPUT( "Objects", boost::blank() )
MAGMA_INPUT( "ObjIndex", -1 )
MAGMA_OUTPUT_NAMES( "Wold Space" )
MAGMA_DESCRIPTION( "Transforms the input to worldspace from the specified node's space." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "ToSpace", "Transform", maya_magma_to_space_node )
MAGMA_ENUM_PROPERTY( inputType, "Point", "Vector", "Normal" )
MAGMA_INPUT( "Vector (WS)", frantic::graphics::vector3f( 0, 0, 0 ) )
MAGMA_INPUT( "Objects", boost::blank() )
MAGMA_INPUT( "ObjIndex", -1 )
MAGMA_OUTPUT_NAMES( "Object Space" )
MAGMA_DESCRIPTION( "Transforms the input to the specified object's space from worldspace." )
MAGMA_DEFINE_TYPE_END

maya_magma_transform_node_base::~maya_magma_transform_node_base() {}

void maya_magma_transform_node_base::compile_as_extension_type( frantic::magma::magma_compiler_interface& compiler ) {
    using namespace frantic::magma;

    std::vector<frantic::graphics::transform4f> tm;

    std::pair<magma_interface::magma_id, int> input = this->get_input( 1 );
    if( !( input.first == magma_interface::INVALID_ID ) ) {
        frantic::magma::nodes::magma_input_objects_interface* inputObjs =
            compiler.get_objects_interface( input.first, input.second );
        if( !inputObjs ) {
            throw magma_exception() << magma_exception::node_id( get_id() )
                                    << magma_exception::property_name( _T("node") )
                                    << magma_exception::error_name(
                                           _T("Transform operator's node was undefined or deleted") );
        }

        tm.resize( inputObjs->size() );

        for( std::size_t i = 0; i < inputObjs->size(); ++i ) {
            if( !inputObjs->get_property<frantic::graphics::transform4f>( i, _T("worldMatrix"), tm[i] ) )
                throw magma_exception() << magma_exception::node_id( get_id() )
                                        << magma_exception::property_name( _T("inputType") )
                                        << magma_exception::error_name(
                                               _T("Invalid value: The input object does not have a valid transform.") );
        }
    } else {

        throw magma_exception() << magma_exception::node_id( get_id() ) << magma_exception::property_name( _T("node") )
                                << magma_exception::error_name(
                                       _T("Transform operator's node was undefined or deleted") );
    }

    if( m_applyInverse )
        for( std::size_t i = 0; i < tm.size(); ++i )
            tm[i] = tm[i].to_inverse();

    std::pair<frantic::magma::magma_interface::magma_id, int> vectorInput = compiler.get_node_input( *this, 0 );
    std::pair<frantic::magma::magma_interface::magma_id, int> indexInput = compiler.get_node_input( *this, 2 );

    if( get_inputType() == _T("Point") ) {
        compiler.compile_transforms( get_id(), tm, frantic::magma::magma_compiler_interface::transform_point,
                                     vectorInput, indexInput );
    } else if( get_inputType() == _T("Vector") ) {
        compiler.compile_transforms( get_id(), tm, frantic::magma::magma_compiler_interface::transform_direction,
                                     vectorInput, indexInput );
    } else if( get_inputType() == _T("Normal") ) {
        compiler.compile_transforms( get_id(), tm, frantic::magma::magma_compiler_interface::transform_normal,
                                     vectorInput, indexInput );
    } else {
        throw magma_exception() << magma_exception::node_id( get_id() )
                                << magma_exception::property_name( _T("inputType") )
                                << magma_exception::error_name( _T("Invalid value: \"") + get_inputType() + _T("\"") );
    }
}

maya_magma_from_space_node::maya_magma_from_space_node() {
    m_applyInverse = false;
    set_inputType( _T( "Point" ) );
    set_num_inputs( 3 );
}

maya_magma_to_space_node::maya_magma_to_space_node() {
    m_applyInverse = true;
    set_inputType( _T( "Point" ) );
    set_num_inputs( 3 );
}

} // namespace nodes
} // namespace maya
} // namespace magma
} // namespace frantic