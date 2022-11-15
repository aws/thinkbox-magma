// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/nodes/magma_node_property.hpp>
#include <frantic/magma/nodes/magma_simple_operator.hpp>

namespace frantic {
namespace magma {
namespace maya {
namespace nodes {

class maya_magma_transform_node_base : public frantic::magma::nodes::magma_simple_operator<3> {
  protected:
    bool m_applyInverse;

  public:
    MAGMA_PROPERTY( inputType, frantic::tstring );

    virtual ~maya_magma_transform_node_base();

    virtual void compile_as_extension_type( frantic::magma::magma_compiler_interface& compiler );
};

class maya_magma_from_space_node : public maya_magma_transform_node_base {
    MAGMA_REQUIRED_METHODS( magma_from_space_node );

    maya_magma_from_space_node();
};

class maya_magma_to_space_node : public maya_magma_transform_node_base {
    MAGMA_REQUIRED_METHODS( magma_to_space_node );

    maya_magma_to_space_node();
};

} // namespace nodes
} // namespace maya
} // namespace magma
} // namespace frantic