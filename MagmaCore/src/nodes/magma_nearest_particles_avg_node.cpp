// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"
#include <frantic/magma/nodes/magma_nearest_particles_avg_node.hpp>
#include <frantic/magma/nodes/magma_node_impl.hpp>

namespace frantic {
namespace magma {
namespace nodes {

MAGMA_DEFINE_TYPE( "ParticleSumCount", "Object", magma_nearest_particles_avg_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_EXPOSE_PROPERTY( channels, std::vector<frantic::tstring> )
MAGMA_HIDDEN_PROPERTY( numNeighbors, int )
MAGMA_HIDDEN_PROPERTY( falloffPower, float )
MAGMA_INPUT( "Particles", boost::blank() )
MAGMA_INPUT( "Lookup Point (WS)", frantic::graphics::vector3f( 0.f ) )
MAGMA_INPUT( "Neighbors", 4 )
MAGMA_INPUT( "Falloff", 0.f )
MAGMA_DESCRIPTION(
    "Sums channel values from the N-nearest particles to the lookup point. Optionally applies a distance based "
    "weighting during the summation. Weight = 1.0 / ( (1.0 + distance ) ^ falloffPower )" )
MAGMA_DEFINE_TYPE_END

magma_nearest_particles_avg_node::magma_nearest_particles_avg_node() {
    // m_channels.push_back( "Position" );
    set_numNeighbors( 2 );
    set_falloffPower( 0.f );
}

#define NUM_BUILTIN_OUTPUTS 2

int magma_nearest_particles_avg_node::get_num_outputs() const { return (int)m_channels.size() + NUM_BUILTIN_OUTPUTS; }

void magma_nearest_particles_avg_node::get_output_description( int index, frantic::tstring& outDescription ) const {
    if( index < 0 || index > ( NUM_BUILTIN_OUTPUTS + (int)m_channels.size() ) )
        return;

    if( index >= NUM_BUILTIN_OUTPUTS )
        outDescription = m_channels[index - NUM_BUILTIN_OUTPUTS];
    else {
        switch( index ) {
        case 0:
            outDescription = _T("MaxDistance");
            break;
        case 1:
            outDescription = _T("TotalWeight");
            break;
        }
    }
}

MAGMA_DEFINE_TYPE( "ParticleSumRadius", "Object", magma_particle_kernel_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_EXPOSE_PROPERTY( channels, std::vector<frantic::tstring> )
MAGMA_HIDDEN_PROPERTY( radius, float )       // Made hidden to allow saved scenes to have the property present.
MAGMA_HIDDEN_PROPERTY( falloffPower, float ) // Made hidden to allow saved scenes to have the property present.
MAGMA_INPUT( "Particles", boost::blank() )
MAGMA_INPUT( "Lookup Point (WS)", frantic::graphics::vector3f( 0.f ) )
MAGMA_INPUT( "Radius", 1.f )
MAGMA_INPUT( "Falloff", 1.f )
MAGMA_DESCRIPTION( "Sums channel values from particles within a spherical area. Optionally applies a distance based "
                   "weighting during the summation. Weight = 1.0 - ( distance / radius ) ^ falloffPower" )
MAGMA_DEFINE_TYPE_END

magma_particle_kernel_node::magma_particle_kernel_node() {}

#define NUM_KERNEL_BUILTIN_OUTPUTS 2

int magma_particle_kernel_node::get_num_outputs() const { return (int)m_channels.size() + NUM_KERNEL_BUILTIN_OUTPUTS; }

void magma_particle_kernel_node::get_output_description( int index, frantic::tstring& outDescription ) const {
    if( index < 0 || index > ( NUM_KERNEL_BUILTIN_OUTPUTS + (int)m_channels.size() ) )
        return;

    if( index >= NUM_KERNEL_BUILTIN_OUTPUTS )
        outDescription = m_channels[index - NUM_KERNEL_BUILTIN_OUTPUTS];
    else {
        switch( index ) {
        case 0:
            outDescription = _T("NumParticles");
            break;
        case 1:
            outDescription = _T("TotalWeight");
            break;
        }
    }
}

} // namespace nodes
} // namespace magma
} // namespace frantic
  /*
  #include <frantic/magma/nodes/magma_input_particles_interface.hpp>
  
  namespace frantic{ namespace magma{ namespace nodes{
  
  MAGMA_DEFINE_TYPE( "InputNeighbors", "Object", magma_neighbor_particles_node )
          MAGMA_OUTPUT_NAMES( "Particles" )
          MAGMA_DESCRIPTION( "Access to the neighbor particles of \"this\" particle." )
  MAGMA_DEFINE_TYPE_END
  
  }}}
  */