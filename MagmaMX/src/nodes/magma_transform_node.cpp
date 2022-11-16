// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/max3d/MagmaMaxContext.hpp>

#include <frantic/magma/magma_exception.hpp>
#include <frantic/magma/max3d/nodes/magma_transform_node.hpp>
#include <frantic/magma/nodes/magma_input_objects_interface.hpp>
#include <frantic/magma/nodes/magma_node_impl.hpp>

#include <frantic/graphics/camera.hpp>
#include <frantic/graphics/transform4f.hpp>

#include <frantic/max3d/convert.hpp>
#include <frantic/max3d/node_transform.hpp>
#include <frantic/max3d/particles/IMaxKrakatoaPRTObject.hpp>

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

void magma_transform_node_base::DefineParameters( ParamBlockDesc2& paramDesc ) {
    paramDesc.AddParam( kNode, _T("node"), TYPE_INODE, 0, 0, p_end );
}

magma_transform_node_base::~magma_transform_node_base() {}

magma_from_space_node::magma_from_space_node() {
    m_applyInverse = false;
    set_inputType( _T("Point") );
}

magma_to_space_node::magma_to_space_node() {
    m_applyInverse = true;
    set_inputType( _T("Point") );
}

MSTR magma_from_space_node::max_impl::s_ClassName( _T("MagmaFromSpaceNode") );
// Class_ID magma_from_space_node::max_impl::s_ClassID( 0x5c35334b, 0x6f0d5c67 ); //This needs to be defined in the DLL,
// not in this project because it must be different for Krakatoa, Genome, etc.

MSTR magma_to_space_node::max_impl::s_ClassName( _T("MagmaToSpaceNode") );
// Class_ID magma_to_space_node::max_impl::s_ClassID( 0x3a53048f, 0x26046e57 ); //This needs to be defined in the DLL,
// not in this project because it must be different for Krakatoa, Genome, etc.

Interval magma_from_space_node::max_impl::get_validity( TimeValue t ) const {
    Interval result = MagmaMaxNodeExtension<max_impl>::get_validity( t );

    if( INode* node = m_pblock->GetINode( kNode ) )
        node->GetNodeTM( t, &result );

    return result;
}

Interval magma_to_space_node::max_impl::get_validity( TimeValue t ) const {
    Interval result = MagmaMaxNodeExtension<max_impl>::get_validity( t );

    if( INode* node = m_pblock->GetINode( kNode ) )
        node->GetNodeTM( t, &result );

    return result;
}

void magma_transform_node_base::compile_as_extension_type( frantic::magma::magma_compiler_interface& compiler ) {
    using namespace frantic::magma;

    /*if( !inode ){
            throw magma_exception()
                    << magma_exception::node_id( get_id() )
                    << magma_exception::property_name( "node" )
                    << magma_exception::error_name( "Transform operator's node was undefined or deleted" );
    }*/

    TimeValue t = TIME_NegInfinity;
    compiler.get_context_data().get_property( _T("Time"), t );

    Interval valid = FOREVER;
    std::vector<frantic::graphics::transform4f> tm; // = frantic::max3d::from_max_t( inode->GetNodeTM( t ) );

    std::pair<magma_interface::magma_id, int> input = this->get_input( 1 );
    if( !( input.first == magma_interface::INVALID_ID ) ) {
        // frantic::magma::nodes::magma_input_objects_interface* inputObj =
        // sc->evaluate_input_for_interface<frantic::magma::nodes::magma_input_objects_interface>( this, 1 );
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
            if( !inputObjs->get_property( i, _T("Transform"), tm[i] ) )
                THROW_MAGMA_INTERNAL_ERROR();
        }
    } else {
        INode* inode = get_node();

        if( inode ) {
            tm.push_back( frantic::max3d::from_max_t( frantic::max3d::get_node_transform( inode, t, valid ) ) );
        } else {
            throw magma_exception() << magma_exception::node_id( get_id() )
                                    << magma_exception::property_name( _T("node") )
                                    << magma_exception::error_name(
                                           _T("Transform operator's node was undefined or deleted") );
        }
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

void magma_from_space_node::compile_as_extension_type( frantic::magma::magma_compiler_interface& compiler ) {
    magma_transform_node_base::compile_as_extension_type( compiler );
}

void magma_to_space_node::compile_as_extension_type( frantic::magma::magma_compiler_interface& compiler ) {
    magma_transform_node_base::compile_as_extension_type( compiler );
}

} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic

using frantic::magma::max3d::MagmaMaxNodeExtension;

template <>
MagmaMaxNodeExtension<frantic::magma::nodes::max3d::magma_from_space_node::max_impl>::MyClassDesc
    MagmaMaxNodeExtension<frantic::magma::nodes::max3d::magma_from_space_node::max_impl>::s_classDesc;

template <>
MagmaMaxNodeExtension<frantic::magma::nodes::max3d::magma_to_space_node::max_impl>::MyClassDesc
    MagmaMaxNodeExtension<frantic::magma::nodes::max3d::magma_to_space_node::max_impl>::s_classDesc;

ClassDesc2* GetMagmaFromSpaceNodeDesc() {
    return &frantic::magma::nodes::max3d::magma_from_space_node::max_impl::s_classDesc;
}

ClassDesc2* GetMagmaToSpaceNodeDesc() {
    return &frantic::magma::nodes::max3d::magma_to_space_node::max_impl::s_classDesc;
}
