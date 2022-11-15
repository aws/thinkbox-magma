// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "frantic/magma/maya/nodes/maya_magma_node.hpp"

#include <frantic/magma/magma_node_base.hpp>
#include <frantic/magma/nodes/magma_input_particles_interface.hpp>
#include <frantic/magma/nodes/magma_node_property.hpp>

namespace frantic {
namespace magma {
namespace maya {
namespace nodes {

class maya_magma_input_particles_node : public magma_maya_simple_operator,
                                        public frantic::magma::nodes::magma_input_particles_interface {

    MAGMA_REQUIRED_METHODS( magma_input_particles_node );
    MAGMA_PROPERTY( particleName, frantic::tstring );

  private:
    particle_array_ptr m_cachedParticles;
    particle_kdtree_ptr m_cachedKDTree;

  public:
    virtual int get_num_outputs() const { return 2; }
    virtual const_particle_array_ptr get_particles() const;
    virtual const_particle_kdtree_ptr get_particle_kdtree();

    virtual void compile_as_extension_type( frantic::magma::magma_compiler_interface& compiler );
};

} // namespace nodes
} // namespace maya
} // namespace magma
} // namespace frantic
