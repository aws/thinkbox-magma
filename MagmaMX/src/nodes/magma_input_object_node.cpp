// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/max3d/MagmaMaxContext.hpp>

#include <frantic/magma/magma_compiler_interface.hpp>
#include <frantic/magma/max3d/nodes/magma_input_object_node.hpp>
#include <frantic/magma/nodes/magma_node_impl.hpp>
#include <frantic/max3d/convert.hpp>
#include <frantic/max3d/node_transform.hpp>
#include <frantic/max3d/particles/IMaxKrakatoaPRTObject.hpp>

using namespace frantic::max3d::particles;

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

MSTR magma_input_object_node::max_impl::s_ClassName( _T("MagmaInputObjectNode") );
// Class_ID magma_input_object_node::max_impl::s_ClassID(0x5b72a14, 0x2f64316b); //This needs to be defined in the DLL,
// not in this project because it must be different for Krakatoa, Genome, etc.

void magma_input_object_node::max_impl::DefineParameters( ParamBlockDesc2& paramDesc ) {
    paramDesc.AddParam( kObject, _T("object"), TYPE_INODE, 0, 0, p_end );
}

void magma_input_object_node::max_impl::reset_validity() { m_cachedValidity = FOREVER; }

void magma_input_object_node::max_impl::update_validity( Interval iv ) { m_cachedValidity &= iv; }

Interval magma_input_object_node::max_impl::get_validity( TimeValue /*t*/ ) const { return m_cachedValidity; }

RefResult magma_input_object_node::max_impl::NotifyRefChanged( const Interval& /*changeInt*/, RefTargetHandle hTarget,
                                                               PartID& /*partID*/, RefMessage message,
                                                               BOOL /*propagate*/ ) {
    if( hTarget == m_pblock ) {
        if( message == REFMSG_CHANGE )
            return REF_SUCCEED;
    }
    return REF_DONTCARE;
}

void magma_input_object_node::compile_as_extension_type( frantic::magma::magma_compiler_interface& compiler ) {
    ReferenceTarget* obj = get_object();
    if( !obj )
        throw magma_exception() << magma_exception::node_id( get_id() ) << magma_exception::property_name( _T("node") )
                                << magma_exception::error_name( _T("Object was undefined or deleted") );

    m_cachedTime = TIME_NegInfinity;

    static_cast<max_impl*>( this->get_max_object() )->reset_validity();

    compiler.get_context_data().get_property( _T("Time"), m_cachedTime );
    compiler.register_interface( get_id(), static_cast<frantic::magma::nodes::magma_input_objects_interface*>( this ) );
}

namespace {
class get_property_visitor : public boost::static_visitor<bool>, boost::noncopyable {
    void* m_outValue;
    const std::type_info& m_outType;

  public:
    get_property_visitor( const std::type_info& outType, void* outValue )
        : m_outType( outType )
        , m_outValue( outValue ) {}

    bool operator()( boost::blank ) { return false; }

    template <class T>
    bool operator()( T val ) {
        if( m_outType != typeid( T ) )
            return false;

        *reinterpret_cast<T*>( m_outValue ) = val;

        return true;
    }
};
} // namespace

bool magma_input_object_node::get_property_internal( std::size_t /*index*/, const frantic::tstring& propName,
                                                     const std::type_info& typeInfo, void* outValue ) {
    std::size_t index = 0;
    INode* node = get_object();
    if( !node )
        return false;

    Interval iv = FOREVER;

    if( propName == _T("Transform") && typeid( frantic::graphics::transform4f ) == typeInfo ) {
        *reinterpret_cast<frantic::graphics::transform4f*>( outValue ) =
            frantic::max3d::from_max_t( frantic::max3d::get_node_transform( node, m_cachedTime, iv ) );

        static_cast<max_impl*>( this->get_max_object() )->update_validity( iv );

        return true;
    }

    // Fall back on the variant based approach if we don't have a specific one in mind.

    variant_t result;
    this->get_property( index, propName, result );

    get_property_visitor visitor( typeInfo, outValue );

    return boost::apply_visitor( visitor, result );
}

void magma_input_object_node::get_property( std::size_t /*index*/, const frantic::tstring& propName,
                                            variant_t& outValue ) {
    ReferenceTarget* rtarg = get_object();
    if( !rtarg )
        return;

    Value* result = frantic::max3d::mxs::expression( _T("try(theObj.") + propName + _T(")catch(undefined)") )
                        .bind( _T("theObj"), rtarg )
                        .at_time( m_cachedTime )
                        .evaluate<Value*>();
    if( !result || result == &undefined )
        return;

    // We currently have no idea what the validity of the property we just grabbed is, so we have to assume its instant.
    static_cast<max_impl*>( this->get_max_object() )->update_validity( Interval( m_cachedTime, m_cachedTime ) );

    if( is_int( result ) ) {
        outValue = result->to_int();
    } else if( is_number( result ) ) {
        outValue = result->to_float();
    } else if( is_bool( result ) ) {
        outValue = result->to_bool();
    } else if( is_point3( result ) ) {
        outValue = frantic::max3d::from_max_t( result->to_point3() );
    } else if( is_color( result ) ) {
        outValue = frantic::max3d::from_max_t( (Point3)result->to_acolor() );
    } else if( is_quat( result ) ) {
        outValue = frantic::max3d::from_max_t( result->to_quat() );
    }
}

} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic

using frantic::magma::max3d::MagmaMaxNodeExtension;

template <>
MagmaMaxNodeExtension<frantic::magma::nodes::max3d::magma_input_object_node::max_impl>::MyClassDesc
    MagmaMaxNodeExtension<frantic::magma::nodes::max3d::magma_input_object_node::max_impl>::s_classDesc;

ClassDesc2* GetMagmaInputObjectNodeDesc() {
    return &frantic::magma::nodes::max3d::magma_input_object_node::max_impl::s_classDesc;
}
