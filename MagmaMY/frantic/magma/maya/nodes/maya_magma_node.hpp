// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

// Contains code common to magma nodes for Maya

#include <frantic/magma/nodes/magma_simple_operator.hpp>

namespace frantic {
namespace magma {
namespace maya {
namespace nodes {

typedef frantic::magma::nodes::magma_simple_operator<0, magma::magma_node_base> magma_maya_simple_operator;

}
} // namespace maya
} // namespace magma
} // namespace frantic
