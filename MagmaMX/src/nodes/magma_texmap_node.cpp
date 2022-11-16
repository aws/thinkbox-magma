// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/max3d/MagmaShadeContext.hpp>
#include <frantic/magma/max3d/MagmaTexmapExpression.hpp>
#include <frantic/magma/max3d/nodes/magma_texmap_node.hpp>

// Have to include these first because MXS has annoying is_array macro
#include <frantic/max3d/convert.hpp>
#include <frantic/max3d/shaders/map_query.hpp>

#undef is_array

#include <frantic/magma/max3d/magma_max_node_impl.hpp>

#include <frantic/magma/simple_compiler/base_compiler.hpp>
#include <frantic/magma/simple_compiler/base_compiler_impl.hpp>

#include <memory>

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

MAGMA_DEFINE_MAX_TYPE( "TexmapEval", "Object", magma_texmap_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_EXPOSE_PROPERTY( texmap, Texmap* )
MAGMA_EXPOSE_ARRAY_PROPERTY( mapChannels, int )
MAGMA_ENUM_PROPERTY( resultType, "Color", "Mono", "NormalPerturb" )
MAGMA_INPUT( "Lookup Point (OS)", frantic::graphics::vector3f( 0 ) )
MAGMA_INPUT( "VertexColor", frantic::graphics::vector3f( 0 ) )
MAGMA_INPUT( "TextureCoord", frantic::graphics::vector3f( 0 ) )
MAGMA_INPUT( "Normal (OS)", frantic::graphics::vector3f( 0 ) )
MAGMA_INPUT( "MtlID", 0 )
MAGMA_INPUT( "IOR", 1.f )
MAGMA_OUTPUT_NAMES( "Result" )
MAGMA_DESCRIPTION( "Evaluates a 3D texmap at a point in space" )
MAGMA_DEFINE_TYPE_END

MSTR magma_texmap_node::max_impl::s_ClassName( _T("TexmapEval" ) );
// magma_texmap_node::max_impl::s_ClassID; Defined in host DLL

void magma_texmap_node::max_impl::DefineParameters( ParamBlockDesc2& paramDesc ) {
    paramDesc.AddParam( kTexmap, _T("texmap"), TYPE_TEXMAP, 0, 0, p_end );
}

Interval magma_texmap_node::max_impl::get_validity( TimeValue t ) const {
    Interval result = MagmaMaxNodeExtension<max_impl>::get_validity( t );

    if( Texmap* tx = m_pblock->GetTexmap( kTexmap ) )
        frantic::max3d::shaders::update_map_for_shading( tx, t, &result );

    return result;
}

RefResult magma_texmap_node::max_impl::NotifyRefChanged( const Interval& /*changeInt*/, RefTargetHandle hTarget,
                                                         PartID& /*partID*/, RefMessage message, BOOL /*propagate*/ ) {
    if( hTarget == m_pblock ) {
        if( message == REFMSG_CHANGE )
            return REF_SUCCEED;
    }
    return REF_DONTCARE;
}

} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic

using frantic::magma::max3d::MagmaMaxNodeExtension;

template <>
MagmaMaxNodeExtension<frantic::magma::nodes::max3d::magma_texmap_node::max_impl>::MyClassDesc
    MagmaMaxNodeExtension<frantic::magma::nodes::max3d::magma_texmap_node::max_impl>::s_classDesc;

ClassDesc2* GetTexmapNodeClassDesc() { return &frantic::magma::nodes::max3d::magma_texmap_node::max_impl::s_classDesc; }

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

using frantic::magma::max3d::MagmaTexmapExpression;

void magma_texmap_node::compile_as_extension_type( frantic::magma::magma_compiler_interface& compiler ) {
    using frantic::magma::simple_compiler::base_compiler;

    Texmap* texmap = get_texmap();
    if( !texmap )
        throw magma_exception() << magma_exception::node_id( get_id() )
                                << magma_exception::property_name( _T("texmap") )
                                << magma_exception::error_name( _T("The texmap was undefined or deleted") );

    MagmaTexmapExpression::result_type::enum_t resultType =
        MagmaTexmapExpression::result_type::from_string( get_resultType() );
    if( resultType == MagmaTexmapExpression::result_type::error )
        throw magma_exception() << magma_exception::node_id( get_id() )
                                << magma_exception::property_name( _T("resultType") )
                                << magma_exception::error_name( _T("The result type \"") + get_resultType() +
                                                                _T("\" was unknown") );

    // Work around for some const-ness issues.
    const RenderGlobalContext* cglobContext = NULL;
    compiler.get_context_data().get_property( _T("MaxRenderGlobalContext"), cglobContext );

    RenderGlobalContext* globContext = const_cast<RenderGlobalContext*>( cglobContext );

    // TODO: We should substitute a default context of some sort.
    if( !globContext )
        throw magma_exception() << magma_exception::error_name( _T("Cannot use TexmapEval in this context") );

    INode* curNode = NULL;
    compiler.get_context_data().get_property( _T("CurrentINode"), curNode );

    bool inWorldSpace = true;
    compiler.get_context_data().get_property( _T("InWorldSpace"), inWorldSpace );

    RenderInstance* rendInst = NULL;
    if( curNode != NULL ) {
        for( int i = 0, iEnd = globContext->NumRenderInstances(); i < iEnd && !rendInst; ++i ) {
            if( RenderInstance* curInst = globContext->GetRenderInstance( i ) ) {
                if( curInst->GetINode() == curNode )
                    rendInst = curInst;
            }
        }
    }

    if( base_compiler* sc = dynamic_cast<base_compiler*>( &compiler ) ) {
        std::unique_ptr<MagmaTexmapExpression> expr(
            new MagmaTexmapExpression( texmap, globContext, rendInst, curNode, inWorldSpace, resultType ) );

        std::vector<std::pair<magma_interface::magma_id, int>> inputs;
        std::vector<magma_data_type> inputTypes;

        inputs.push_back( sc->get_node_input( *this, 0 ) );
        inputs.push_back( sc->get_node_input( *this, 1 ) );
        inputs.push_back( sc->get_node_input( *this, 2 ) );
        inputs.push_back( sc->get_node_input( *this, 3 ) );
        inputs.push_back( sc->get_node_input( *this, 4 ) );
        inputs.push_back( sc->get_node_input( *this, 5 ) );

        inputTypes.push_back( simple_compiler::traits<vec3>::get_type() );  // Position
        inputTypes.push_back( simple_compiler::traits<vec3>::get_type() );  // Color
        inputTypes.push_back( simple_compiler::traits<vec3>::get_type() );  // TextureCoord
        inputTypes.push_back( simple_compiler::traits<vec3>::get_type() );  // Normal
        inputTypes.push_back( simple_compiler::traits<int>::get_type() );   // MtlID
        inputTypes.push_back( simple_compiler::traits<float>::get_type() ); // IOR

        for( int i = 6, iEnd = this->get_num_inputs(); i < iEnd; ++i ) {
            inputs.push_back( sc->get_node_input( *this, i ) );
            inputTypes.push_back( simple_compiler::traits<vec3>::get_type() );

            expr->bind_input_to_channel( i, this->m_mapChannelConnections[i - 6].channel );
        }
        std::unique_ptr<base_compiler::expression> baseExpression(
            static_cast<base_compiler::expression*>( expr.release() ) );
        sc->compile_expression( std::move( baseExpression ), this->get_id(), inputs, inputTypes );
    } else {
        magma_max_node_base::compile_as_extension_type( compiler );
    }
}

} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic
