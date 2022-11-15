// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "frantic/magma/maya/nodes/maya_magma_node.hpp"

#include <frantic/magma/magma_node_base.hpp>
#include <frantic/magma/nodes/magma_input_objects_interface.hpp>
#include <frantic/magma/nodes/magma_node_property.hpp>

#include <maya/MFnDependencyNode.h>

namespace frantic {
namespace magma {
namespace maya {
namespace nodes {

class maya_magma_input_object_node : public magma_maya_simple_operator,
                                     public frantic::magma::nodes::magma_input_objects_interface {

    MAGMA_REQUIRED_METHODS( magma_input_particles_node );
    MAGMA_PROPERTY( objectName, frantic::tstring );

  private:
    MObject m_sourceNode;

    void get_transform( std::size_t index, frantic::graphics::transform4f& outTransform, bool& foundOutTransform );

  protected:
    virtual bool get_property_internal( std::size_t /*index*/, const frantic::tstring& propName,
                                        const std::type_info& typeInfo, void* outValue );

  public:
    virtual std::size_t size() const { return 1; };

    virtual void get_property( std::size_t /*index*/, const frantic::tstring& propName, variant_t& outValue );

    virtual int get_num_outputs() const { return 1; }

    virtual void compile_as_extension_type( frantic::magma::magma_compiler_interface& compiler );
};

} // namespace nodes
} // namespace maya
} // namespace magma
} // namespace frantic
