// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/max3d/MagmaMaxContext.hpp>

#include <frantic/magma/max3d/nodes/magma_input_value_node.hpp>
#include <frantic/magma/nodes/magma_node_impl.hpp>
#include <frantic/max3d/particles/IMaxKrakatoaPRTObject.hpp>

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

MSTR magma_input_value_node::max_impl::s_ClassName( _T("MagmaInputValueNode") );
// Class_ID magma_input_value_node::max_impl::s_ClassID( 0x4b2d56d2, 0x4bf864b7 ); //This needs to be defined in the
// DLL, not in this project because it must be different for Krakatoa, Genome, etc.

void magma_input_value_node::max_impl::DefineParameters( ParamBlockDesc2& paramDesc ) {
    paramDesc.AddParam( kController, _T("controller"), TYPE_REFTARG, 0, 0, p_end );
}

RefResult magma_input_value_node::max_impl::NotifyRefChanged( const Interval& /*changeInt*/, RefTargetHandle hTarget,
                                                              PartID& /*partID*/, RefMessage message,
                                                              BOOL /*propagate*/ ) {
    if( hTarget == m_pblock ) {
        if( message == REFMSG_CHANGE )
            return REF_SUCCEED;
    }
    return REF_DONTCARE;
}

int magma_input_value_node::max_impl::NumSubs() { return 2; }

Animatable* magma_input_value_node::max_impl::SubAnim( int i ) {
    // return (i == 0) ? (ReferenceTarget*)m_pblock : (i == 1) ? (ReferenceTarget*)get_controller() : NULL;
    return ( i == 0 )               ? (ReferenceTarget*)m_pblock
           : ( i == 1 && m_pblock ) ? m_pblock->GetReferenceTarget( kController )
                                    : NULL;
}

#if MAX_VERSION_MAJOR < 24
TSTR magma_input_value_node::max_impl::SubAnimName( int i ) {
#else
TSTR magma_input_value_node::max_impl::SubAnimName( int i, bool localized ) {
#endif
    if( i == 0 && m_pblock )
        return m_pblock->GetLocalName();
    else if( i == 1 )
        return _T("Value");
    return _T("");
}

BOOL magma_input_value_node::max_impl::AssignController( Animatable* control, int subAnim ) {
    if( m_pblock && subAnim == 1 ) {
        // set_controller( static_cast<Control*>(control) );
        m_pblock->SetValue( kController, 0, static_cast<Control*>( control ) );

        NotifyDependents( FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED );

        return TRUE;
    }
    return FALSE;
}

Interval magma_input_value_node::max_impl::get_validity( TimeValue t ) const {
    Interval iv = MagmaMaxNodeExtension<max_impl>::get_validity( t );

    float f;
    Point3 p;
    Quat q;
    Matrix3 m;
    ScaleValue s;

    ReferenceTarget* controller = m_pblock->GetReferenceTarget( kController );
    if( controller ) {
        switch( controller->SuperClassID() ) {
        case CTRL_FLOAT_CLASS_ID:
            static_cast<Control*>( controller )->GetValue( t, &f, iv );
            break;
        case CTRL_POINT3_CLASS_ID:
            static_cast<Control*>( controller )->GetValue( t, &p, iv );
            break;
        case CTRL_MATRIX3_CLASS_ID:
            static_cast<Control*>( controller )->GetValue( t, &m, iv );
            break;
        case CTRL_POSITION_CLASS_ID:
            static_cast<Control*>( controller )->GetValue( t, &p, iv );
            break;
        case CTRL_ROTATION_CLASS_ID:
            static_cast<Control*>( controller )->GetValue( t, &q, iv );
            break;
        case CTRL_SCALE_CLASS_ID:
            static_cast<Control*>( controller )->GetValue( t, &s, iv );
            break;
        default:
            break;
        }
    }

    return iv;
}

void magma_input_value_node::compile_as_extension_type( frantic::magma::magma_compiler_interface& compiler ) {
    Control* c = get_controller();
    if( !c )
        throw magma_exception() << magma_exception::node_id( get_id() )
                                << magma_exception::property_name( _T("controller") )
                                << magma_exception::error_name( _T("InputValue controller was undefined or deleted") );

    TimeValue t = TIME_NegInfinity;
    compiler.get_context_data().get_property( _T("Time"), t );

    frantic::magma::variant_t val;

    if( c->SuperClassID() == CTRL_FLOAT_CLASS_ID ) {
        float tempVal;

        Interval garbage;
        c->GetValue( t, &tempVal, garbage );

        if( !get_forceInteger() ) {
            val = tempVal;
        } else {
            val = (int)std::floor( tempVal + 0.5f );
        }
    } else if( c->SuperClassID() == CTRL_POINT3_CLASS_ID || c->SuperClassID() == CTRL_POSITION_CLASS_ID ) {
        Point3 p;
        Interval garbage;
        c->GetValue( t, &p, garbage );

        val = frantic::max3d::from_max_t( p );
    } else if( c->SuperClassID() == CTRL_ROTATION_CLASS_ID ) {
        Quat q;
        Interval garbage;
        c->GetValue( t, &q, garbage );

        val = frantic::max3d::from_max_t( q );
    } else
        THROW_MAGMA_INTERNAL_ERROR();

    compiler.compile_constant( this->get_id(), val );
}
} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic

using frantic::magma::max3d::MagmaMaxNodeExtension;

template <>
MagmaMaxNodeExtension<frantic::magma::nodes::max3d::magma_input_value_node::max_impl>::MyClassDesc
    MagmaMaxNodeExtension<frantic::magma::nodes::max3d::magma_input_value_node::max_impl>::s_classDesc;

ClassDesc2* GetMagmaInputValueNodeDesc() {
    return &frantic::magma::nodes::max3d::magma_input_value_node::max_impl::s_classDesc;
}
