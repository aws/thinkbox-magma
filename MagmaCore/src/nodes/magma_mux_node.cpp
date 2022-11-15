// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"
#include <frantic/magma/nodes/magma_mux_node.hpp>
#include <frantic/magma/nodes/magma_node_impl.hpp>

namespace frantic {
namespace magma {
namespace nodes {

MAGMA_DEFINE_TYPE( "Mux", "Logic", magma_mux_node )
MAGMA_DESCRIPTION( "Selects an input based on the 'Selector' input index." )
MAGMA_DEFINE_TYPE_END

magma_mux_node::magma_mux_node() { set_num_inputs( 3 ); }

} // namespace nodes
} // namespace magma
} // namespace frantic
