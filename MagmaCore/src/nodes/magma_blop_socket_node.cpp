// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"
#include <frantic/magma/nodes/magma_blop_socket_node.hpp>
#include <frantic/magma/nodes/magma_node_impl.hpp>

namespace frantic {
namespace magma {
namespace nodes {

MAGMA_DEFINE_TYPE( "BLOPSocket", "System", magma_blop_input_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_TYPE_ATTR( public, false );
MAGMA_DEFINE_TYPE_END

magma_blop_input_node::magma_blop_input_node() {}

MAGMA_DEFINE_TYPE( "BLOPOutput", "System", magma_blop_output_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_TYPE_ATTR( public, false );
MAGMA_DEFINE_TYPE_END

magma_blop_output_node::magma_blop_output_node() { this->set_num_inputs( 1 ); }

} // namespace nodes
} // namespace magma
} // namespace frantic
