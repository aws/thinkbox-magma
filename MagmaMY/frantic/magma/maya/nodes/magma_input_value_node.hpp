// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <boost/blank.hpp>

#include "frantic/magma/maya/nodes/maya_magma_node.hpp"
#include <frantic/magma/magma_data_type.hpp>
#include <frantic/magma/nodes/magma_node_property.hpp>

namespace frantic {
namespace magma {
namespace maya {
namespace nodes {

class magma_input_value_node : public magma_maya_simple_operator {
  public:
    MAGMA_REQUIRED_METHODS( magma_input_value_node )
    // I try MAGMA_ENUM_PROPERTY; however it causes compile-time errors;
    // MAGMA_ENUM_PROPERTY( enumValue, "Float", "Int", "Vector3f" )
    // so that I just stick with int in term of enumValue. 1 means float, 2 means int, 3 means vector3f
    MAGMA_PROPERTY( enumValue, int )
    MAGMA_PROPERTY( fValue, float )
    MAGMA_PROPERTY( iValue, int )
    MAGMA_PROPERTY( vec3Value, frantic::graphics::vector3f )

    virtual void compile_as_extension_type( frantic::magma::magma_compiler_interface& compiler );

    magma_input_value_node() {}
};

} // namespace nodes
} // namespace maya
} // namespace magma
} // namespace frantic
