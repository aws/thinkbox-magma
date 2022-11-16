// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_node_base.hpp>
#include <frantic/magma/nodes/magma_node_property.hpp>

#include <frantic/magma/max3d/MagmaMaxNodeExtension.hpp>

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

class magma_script_op_node : public magma_max_input_node {
  public:
    class max_impl : public MagmaMaxNodeExtension<max_impl> {
      public:
        static MSTR s_ClassName;
        static Class_ID s_ClassID;

        static void DefineParameters( ParamBlockDesc2& paramDesc );

        virtual RefResult NotifyRefChanged( const Interval& changeInt, RefTargetHandle hTarget, PartID& partID,
                                            RefMessage message, BOOL propagate );

        // From IMagmaNode
        virtual Interval get_validity( TimeValue t ) const;
    };

  public:
    MAGMA_MAX_REQUIRED_METHODS( magma_script_op_node );
    MAGMA_PROPERTY( script, M_STD_STRING );
    MAGMA_PROPERTY( outputType, magma_data_type );

    magma_script_op_node() { m_script = _T("[0,0,0]"); }
};

} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic
