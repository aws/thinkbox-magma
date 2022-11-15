// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"
#include <frantic/magma/nodes/magma_input_channel_node.hpp>
#include <frantic/magma/nodes/magma_node_impl.hpp>

namespace frantic {
namespace magma {
namespace nodes {

MAGMA_DEFINE_TYPE( "InputChannel", "Input", magma_input_channel_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_EXPOSE_PROPERTY( channelName, frantic::tstring );
MAGMA_EXPOSE_PROPERTY( channelType, magma_data_type );
MAGMA_DESCRIPTION( "Reads a value from the specified channel for the current iteration." )
MAGMA_DEFINE_TYPE_END

} // namespace nodes
} // namespace magma
} // namespace frantic
