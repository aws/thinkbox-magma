// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/nodes/magma_node_property.hpp>
#include <frantic/magma/nodes/magma_simple_operator.hpp>

#include <frantic/magma/max3d/MagmaMaxNodeExtension.hpp>

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

template <>
inline Control* magma_max_node_base::get_max_property<Control>( ParamID id, int tabIndex ) const {
    return static_cast<Control*>( get_max_property<ReferenceTarget>( id, tabIndex ) );
}

class magma_input_value_node : public magma_max_input_node {
  public:
    enum { kController = 1 };

    class max_impl : public MagmaMaxNodeExtension<max_impl> {
      public:
        static MSTR s_ClassName;
        static Class_ID s_ClassID;

        static void DefineParameters( ParamBlockDesc2& paramDesc );

        virtual RefResult NotifyRefChanged( const Interval& changeInt, RefTargetHandle hTarget, PartID& partID,
                                            RefMessage message, BOOL propagate );

        virtual int NumSubs();
        virtual Animatable* SubAnim( int i );

#if MAX_VERSION_MAJOR >= 24
        virtual TSTR SubAnimName( int i, bool localized );
#else
        virtual TSTR SubAnimName( int i );
#endif

        virtual BOOL AssignController( Animatable* control, int subAnim );

        // From IMagmaNode
        virtual Interval get_validity( TimeValue t ) const;
    };

  public:
    MAGMA_MAX_REQUIRED_METHODS( magma_input_value_node );
    MAGMA_PROPERTY( forceInteger, bool );
    MAGMA_MAX_PROPERTY( controller, Control, kController );

    magma_input_value_node() { m_forceInteger = false; }
};

} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic
