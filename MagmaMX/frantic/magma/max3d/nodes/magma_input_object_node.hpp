// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/nodes/magma_input_objects_interface.hpp>
#include <frantic/magma/nodes/magma_node_property.hpp>

#include <frantic/magma/max3d/MagmaMaxNodeExtension.hpp>

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

class magma_input_object_node : public magma_max_input_node, public magma_input_objects_interface {
  public:
    enum {
        // kNode = 1 //TODO: Make this multiple nodes
        kObject = 1
    };

    class max_impl : public MagmaMaxNodeExtension<max_impl> {
        Interval m_cachedValidity;

      public:
        static MSTR s_ClassName;
        static Class_ID s_ClassID;

        static void DefineParameters( ParamBlockDesc2& paramDesc );

        virtual void reset_validity();

        virtual void update_validity( Interval iv );

        virtual Interval get_validity( TimeValue t ) const;

        virtual RefResult NotifyRefChanged( const Interval& changeInt, RefTargetHandle hTarget, PartID& partID,
                                            RefMessage message, BOOL propagate );
    };

  private:
    TimeValue m_cachedTime;

  protected:
    virtual bool get_property_internal( std::size_t /*index*/, const frantic::tstring& propName,
                                        const std::type_info& typeInfo, void* outValue );

  public:
    MAGMA_MAX_REQUIRED_METHODS( magma_input_object_node );
    MAGMA_MAX_PROPERTY( object, INode, kObject );

    virtual std::size_t size() const { return 1; };

    virtual void get_property( std::size_t /*index*/, const frantic::tstring& propName, variant_t& outValue );
};

} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic
