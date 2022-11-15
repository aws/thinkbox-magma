// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"
#include <frantic/magma/nodes/magma_node_impl.hpp>
#include <frantic/magma/nodes/magma_particle_query_node.hpp>

namespace frantic {
namespace magma {
namespace nodes {

MAGMA_DEFINE_TYPE( "ParticleQuery", "Object", magma_particle_query_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_EXPOSE_PROPERTY( channels, std::vector<frantic::tstring> )
MAGMA_INPUT_NAMES( "Particles", "Index" )
MAGMA_DESCRIPTION( "Reads a collection of channels from the indexed particle." )
MAGMA_DEFINE_TYPE_END

magma_particle_query_node::magma_particle_query_node() { m_channels.push_back( _T("Position") ); }

int magma_particle_query_node::get_num_outputs() const { return (int)m_channels.size(); }

void magma_particle_query_node::get_output_description( int index, frantic::tstring& outDescription ) const {
    if( index >= 0 && index < static_cast<int>( m_channels.size() ) )
        outDescription = m_channels[index];
}

} // namespace nodes
} // namespace magma
} // namespace frantic
