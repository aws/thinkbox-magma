// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/max3d/MagmaShadeContext.hpp>
#include <frantic/magma/max3d/MagmaTexmapExpression.hpp>

#include <frantic/magma/simple_compiler/base_compiler_impl.hpp> //Only for traits classes ... maybe those should not be impl details!

#include <frantic/max3d/convert.hpp>
#include <frantic/max3d/node_transform.hpp>
#include <frantic/max3d/shaders/map_query.hpp>

#pragma warning( push, 3 )
#include <imtl.h>
#pragma warning( pop )

namespace frantic {
namespace magma {
namespace max3d {

using frantic::magma::simple_compiler::base_compiler;

template <class Key, class Value>
struct key_less {
  public:
    bool operator()( const std::pair<Key, Value>& lhs, const Key& rhs ) const { return lhs.first < rhs; }

    bool operator()( const Key& lhs, const std::pair<Key, Value>& rhs ) const { return lhs < rhs.first; }
};

MagmaTexmapExpression::result_type::enum_t
MagmaTexmapExpression::result_type::from_string( const frantic::tstring& val ) {
    if( val == _T("Color") )
        return color;
    else if( val == _T("Mono") )
        return mono;
    else if( val == _T("Perturb") || val == _T("NormalPerturb") )
        return perturb;
    else
        return error;
}

class MagmaUVWShadeContext : public MagmaShadeContext {
    const std::vector<std::pair<int, std::ptrdiff_t>>* m_channelPtrs;
    const base_compiler::state* m_currentState;

#pragma warning( push )
#pragma warning( disable : 4822 )
    MagmaUVWShadeContext& operator=( const MagmaUVWShadeContext& );
#pragma warning( pop )

  public:
    void set_uvw_data( const std::vector<std::pair<int, std::ptrdiff_t>>& channelPtrs,
                       const base_compiler::state& curState ) {
        m_channelPtrs = &channelPtrs;
        m_currentState = &curState;
    }

    virtual Point3 UVW( int channel ) {
        std::vector<std::pair<int, std::ptrdiff_t>>::const_iterator it =
            std::lower_bound( m_channelPtrs->begin(), m_channelPtrs->end(), channel, key_less<int, std::ptrdiff_t>() );
        if( it != m_channelPtrs->end() && it->first == channel )
            return frantic::max3d::to_max_t( m_currentState->get_temporary<frantic::graphics::vector3f>( it->second ) );
        return Point3( 0, 0, 0 );
    }
};

MagmaTexmapExpression::MagmaTexmapExpression( Texmap* theMap, RenderGlobalContext* globContext,
                                              RenderInstance* rendInst, INode* node, bool inWorldSpace,
                                              result_type::enum_t resultType ) {
    // TODO: We should substitute a default context of some sort.
    if( !globContext )
        throw magma_exception() << magma_exception::error_name( _T("Cannot use TexmapEval in this context") );

    TimeValue t = globContext->time;

    m_texmap = theMap;
    m_globContext = globContext;
    m_rendInst = rendInst;
    m_node = node;
    m_resultType = resultType;

    m_toCamera = globContext->worldToCam;
    m_fromCamera = globContext->camToWorld;

    if( !inWorldSpace && node ) {
        Interval valid = FOREVER;
        Matrix3 nodeTM = frantic::max3d::get_node_transform( node, t, valid );

        m_toCamera = nodeTM * m_toCamera;
        m_fromCamera = m_fromCamera * Inverse( nodeTM );
    }

    // We currently don't bother complaining about missing UVWs. Maybe we can think on this?
    BitArray reqUVWs;
    // bool ignoreReqUVWs = false;

    frantic::max3d::shaders::update_map_for_shading( m_texmap, t );
    frantic::max3d::shaders::collect_map_requirements( m_texmap, reqUVWs );

    // These are always present
    m_inputBindings.push_back( std::make_pair( 1, 0 ) );
    m_inputBindings.push_back( std::make_pair( 2, 1 ) );
}

MagmaTexmapExpression::~MagmaTexmapExpression() {}

void MagmaTexmapExpression::bind_input_to_channel( std::size_t input, int mapChannel ) {
    std::vector<std::pair<std::size_t, int>>::iterator it =
        std::lower_bound( m_inputBindings.begin(), m_inputBindings.end(), input, key_less<std::size_t, int>() );
    if( it == m_inputBindings.end() || it->first != input )
        m_inputBindings.insert( it, std::make_pair( input, mapChannel ) );
    else
        it->second = mapChannel;
}

inline void insert_map_ptr( std::vector<std::pair<int, std::ptrdiff_t>>& mapPtrs, int channel, std::ptrdiff_t relPtr ) {
    std::vector<std::pair<int, std::ptrdiff_t>>::iterator itPtr =
        std::lower_bound( mapPtrs.begin(), mapPtrs.end(), channel, key_less<int, std::ptrdiff_t>() );
    if( itPtr == mapPtrs.end() || itPtr->first != channel )
        mapPtrs.insert( itPtr, std::make_pair( channel, relPtr ) );
    else
        itPtr->second = relPtr;
}

inline std::ptrdiff_t get_map_ptr( const std::vector<std::pair<int, std::ptrdiff_t>>& mapPtrs, int channel ) {
    std::vector<std::pair<int, std::ptrdiff_t>>::const_iterator itPtr =
        std::lower_bound( mapPtrs.begin(), mapPtrs.end(), channel, key_less<int, std::ptrdiff_t>() );
    if( itPtr != mapPtrs.end() && itPtr->first == channel )
        return itPtr->second;
    return -1;
}

void MagmaTexmapExpression::set_input( std::size_t inputIndex, std::ptrdiff_t relPtr ) {
    if( inputIndex < 6 ) {
        switch( inputIndex ) {
        case 0:
            m_inputs[0] = relPtr;
            break;
        case 1:
            m_inputs[1] = relPtr;
            insert_map_ptr( m_uvwInfo, 0, relPtr );
            break;
        case 2:
            m_inputs[2] = relPtr;
            insert_map_ptr( m_uvwInfo, 1, relPtr );
            break;
        case 3:
            m_inputs[3] = relPtr;
            break;
        case 4:
            m_inputs[4] = relPtr;
            break;
        case 5:
            m_inputs[5] = relPtr;
            break;
        }
    } else {
        // Figure out which map channel this input socket corresponds to, then store the pointer in m_uvwInfo.
        std::vector<std::pair<std::size_t, int>>::const_iterator it = std::lower_bound(
            m_inputBindings.begin(), m_inputBindings.end(), inputIndex, key_less<std::size_t, int>() );
        if( it != m_inputBindings.end() && it->first == inputIndex )
            insert_map_ptr( m_uvwInfo, it->second, relPtr );
    }
};

void MagmaTexmapExpression::set_output( std::ptrdiff_t relPtr ) { m_outPtr = relPtr; }

const frantic::channels::channel_map& MagmaTexmapExpression::get_output_map() const {
    switch( m_resultType ) {
    case result_type::color:
        return frantic::magma::simple_compiler::traits<frantic::graphics::vector3f>::get_static_map();
    case result_type::mono:
        return frantic::magma::simple_compiler::traits<float>::get_static_map();
    case result_type::perturb:
        return frantic::magma::simple_compiler::traits<frantic::graphics::vector3f>::get_static_map();
    default:
        __assume( 0 ); // MS specific optimization that says there is no need to check for other cases.
    }
}

// Like VectorTransform( Point3, Matrix3 ) expect it transposes the matrix first.
inline Point3 VectorTransposeTransform( const Point3& p, const Matrix3& m ) {
    return Point3( DotProd( p, m[0] ), DotProd( p, m[1] ), DotProd( p, m[2] ) );
}

void MagmaTexmapExpression::apply( base_compiler::state& data ) const {
    MagmaUVWShadeContext sc;

    sc.set_render_global_context( m_globContext );
    sc.set_render_instance( m_rendInst, m_node );
    sc.set_uvw_data( m_uvwInfo, data );

    sc.m_pos = frantic::max3d::to_max_t( data.get_temporary<frantic::graphics::vector3f>( m_inputs[0] ) ) * m_toCamera;
    sc.m_view = sc.m_origView =
        ( m_globContext->projType == PROJ_PERSPECTIVE ) ? Normalize( sc.m_pos ) : Point3( 0.f, 0.f, -1.f );
    sc.m_normal = sc.m_origNormal = Normalize( VectorTransposeTransform(
        frantic::max3d::to_max_t( data.get_temporary<frantic::graphics::vector3f>( m_inputs[0] ) ), m_fromCamera ) );
    sc.mtlNum = data.get_temporary<int>( m_inputs[4] );
    sc.m_ior = data.get_temporary<float>( m_inputs[5] );

    switch( m_resultType ) {
    case result_type::color: {
        AColor result = m_texmap->EvalColor( sc );
        data.set_temporary<frantic::graphics::vector3f>( m_outPtr,
                                                         frantic::graphics::vector3f( result.r, result.g, result.b ) );
    } break;
    case result_type::mono: {
        float result = m_texmap->EvalMono( sc );
        data.set_temporary<float>( m_outPtr, result );
    } break;
    case result_type::perturb: {
        Point3 result = m_texmap->EvalNormalPerturb( sc );
        data.set_temporary<frantic::graphics::vector3f>( m_outPtr,
                                                         frantic::graphics::vector3f( result.x, result.y, result.z ) );
    } break;
    default:
        __assume( 0 ); // MS specific optimization that says there is no need to check for other cases.
    }
}

void MagmaTexmapExpression::internal_apply( const base_compiler::expression* _this, base_compiler::state& data ) {
    static_cast<const MagmaTexmapExpression*>( _this )->MagmaTexmapExpression::apply(
        data ); // Directly refer to the function via the class to get static binding (and hopefully inlining).
}

MagmaTexmapExpression::runtime_ptr MagmaTexmapExpression::get_runtime_ptr() const { return &internal_apply; }

} // namespace max3d
} // namespace magma
} // namespace frantic
