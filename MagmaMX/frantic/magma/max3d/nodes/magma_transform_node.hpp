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

class magma_transform_node_base : public magma_simple_operator<3, magma_max_node_base> {
  public:
    enum { kNode = 1 };

    static void DefineParameters( ParamBlockDesc2& paramDesc );

  protected:
    bool m_applyInverse;

  public:
    MAGMA_PROPERTY( inputType, frantic::tstring );
    MAGMA_MAX_PROPERTY( node, INode, kNode );

    virtual ~magma_transform_node_base();

    virtual void compile_as_extension_type( frantic::magma::magma_compiler_interface& compiler );
};

class magma_from_space_node : public magma_transform_node_base {
  public:
    class max_impl : public MagmaMaxNodeExtension<max_impl> {
      public:
        static MSTR s_ClassName;
        static Class_ID s_ClassID;

        virtual Interval get_validity( TimeValue t ) const;

        inline static void DefineParameters( ParamBlockDesc2& paramDesc ) {
            magma_transform_node_base::DefineParameters( paramDesc );
        }

        virtual RefResult NotifyRefChanged( const Interval& /*changeInt*/, RefTargetHandle /*hTarget*/,
                                            PartID& /*partID*/, RefMessage /*message*/, BOOL /*propagate*/ ) {
            return REF_SUCCEED;
        }
    };

  public:
    MAGMA_MAX_REQUIRED_METHODS( magma_from_space_node );

    magma_from_space_node();
};

class magma_to_space_node : public magma_transform_node_base {
  public:
    class max_impl : public MagmaMaxNodeExtension<max_impl> {
      public:
        static MSTR s_ClassName;
        static Class_ID s_ClassID;

        virtual Interval get_validity( TimeValue t ) const;

        inline static void DefineParameters( ParamBlockDesc2& paramDesc ) {
            magma_transform_node_base::DefineParameters( paramDesc );
        }

        virtual RefResult NotifyRefChanged( const Interval& /*changeInt*/, RefTargetHandle /*hTarget*/,
                                            PartID& /*partID*/, RefMessage /*message*/, BOOL /*propagate*/ ) {
            return REF_SUCCEED;
        }
    };

  public:
    MAGMA_MAX_REQUIRED_METHODS( magma_to_space_node );

    magma_to_space_node();
};

} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic
