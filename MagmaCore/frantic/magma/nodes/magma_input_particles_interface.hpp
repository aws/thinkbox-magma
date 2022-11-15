// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/nodes/magma_simple_operator.hpp>
#include <frantic/particles/particle_array.hpp>
#include <frantic/particles/particle_kdtree.hpp>

namespace frantic {
namespace magma {
namespace nodes {

namespace detail {
struct particle_standin {
    frantic::graphics::vector3f pos;
    boost::int64_t index;

    particle_standin()
        : index( -1 ) {}

    particle_standin( const frantic::graphics::vector3f& pos, boost::int64_t index )
        : pos( pos )
        , index( index ) {}

    float operator[]( int i ) const { return pos[i]; }

    operator const frantic::graphics::vector3f&() const { return pos; }
};
} // namespace detail

class magma_input_particles_interface {
  public:
    typedef boost::shared_ptr<frantic::particles::particle_array> particle_array_ptr;
    typedef boost::shared_ptr<const frantic::particles::particle_array> const_particle_array_ptr;
    typedef boost::shared_ptr<frantic::particles::particle_kdtree<detail::particle_standin>> particle_kdtree_ptr;
    typedef boost::shared_ptr<const frantic::particles::particle_kdtree<detail::particle_standin>>
        const_particle_kdtree_ptr;

  public:
    virtual const_particle_array_ptr get_particles() const = 0;
    virtual const_particle_kdtree_ptr get_particle_kdtree() = 0; // Non const to allow for lazy evaluation.

    // virtual std::size_t size_t() const = 0;
    // virtual const_particle_array_ptr get_particles( std::size_t index ) const = 0;
    // virtual const_particle_kdtree_ptr get_particle_kdtree( std::size_t index ) = 0; //Non const to allow for lazy
    // evaluation.
};
/*
class magma_neighbor_particles_node : public magma_input_particles_interface{
        const_particle_array_ptr m_particles;
        const_particle_kdtree_ptr m_kdtree;

public:
        static void create_type_definition( magma_node_type& outType );

        virtual void compile( magma_compiler_interface& compile );

        virtual void compile_as_extension_type( magma_compiler_interface& compiler );

        virtual const_particle_array_ptr get_particles() const {
                return m_particles;
        }

        virtual const_particle_kdtree_ptr get_particle_kdtree() {
                return m_kdtree;
        }
};
*/
} // namespace nodes
} // namespace magma
} // namespace frantic
