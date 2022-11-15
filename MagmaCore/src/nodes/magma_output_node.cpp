// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"
#include <frantic/magma/nodes/magma_node_impl.hpp>
#include <frantic/magma/nodes/magma_output_node.hpp>

namespace frantic {
namespace magma {
namespace nodes {

MAGMA_DEFINE_TYPE( "Output", "System", magma_output_node )
MAGMA_EXPOSE_PROPERTY( channelName, frantic::tstring );
MAGMA_EXPOSE_PROPERTY( channelType, magma_data_type );
MAGMA_INPUT( "Value", boost::blank() );
MAGMA_DESCRIPTION( "Writes a value into the specified channel for the current iteration." );
MAGMA_DEFINE_TYPE_END

magma_output_node::magma_output_node() { m_channelName = _T("Pick a channel"); }

} // namespace nodes
} // namespace magma
} // namespace frantic
