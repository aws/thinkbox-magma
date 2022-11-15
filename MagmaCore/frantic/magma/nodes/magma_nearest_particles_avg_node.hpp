// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/functors/particles.hpp>
#include <frantic/magma/magma_singleton.hpp>
#include <frantic/magma/nodes/magma_input_particles_interface.hpp>
#include <frantic/magma/nodes/magma_node_property.hpp>
#include <frantic/magma/nodes/magma_particle_query_node.hpp>
#include <frantic/magma/nodes/magma_simple_operator.hpp>

#include <frantic/channels/channel_map.hpp>

#include <boost/mpl/vector.hpp>

#include <vector>

namespace frantic {
namespace magma {
namespace nodes {

class magma_nearest_particle_node : public magma_simple_operator<2> {
    MAGMA_PROPERTY( whichNearest, int );

  public:
    struct meta {
        enum { ARITY = 1 };
        typedef frantic::magma::functors::nearest_particle type;
        typedef boost::mpl::vector<void( void*, vec3 )> bindings;
    };

    struct nth_meta {
        enum { ARITY = 1 };
        typedef frantic::magma::functors::nth_nearest_particle type;
        typedef boost::mpl::vector<void( void*, vec3 )> bindings;
    };

    MAGMA_REQUIRED_METHODS( magma_nearest_particle_node );

    magma_nearest_particle_node()
        : m_whichNearest( 1 ) {}

    virtual int get_num_outputs() const { return 3; }
};

class magma_nearest_particles_avg_node : public magma_simple_operator<4> {
    MAGMA_PROPERTY( channels, std::vector<frantic::tstring> )

  public:
    inline const int get_numNeighbors() const { return boost::get<int>( this->get_input_default_value( 2 ) ); }
    inline void set_numNeighbors( int val ) { this->set_input_default_value( 2, val ); }

    inline const float get_falloffPower() const { return boost::get<float>( this->get_input_default_value( 3 ) ); }
    inline void set_falloffPower( float val ) { this->set_input_default_value( 3, val ); }

  public:
    struct meta {
        enum { ARITY = 3 };
        typedef frantic::magma::functors::particle_sum_count type;
        typedef boost::mpl::vector<void( void*, vec3, int, float )> bindings;
    };

  public:
    static void create_type_definition( magma_node_type& outType );

    virtual void compile( magma_compiler_interface& compile );

    magma_nearest_particles_avg_node();

    virtual int get_num_outputs() const;

    virtual void get_output_description( int index, frantic::tstring& outDescription ) const;
};

class magma_particle_kernel_node : public magma_simple_operator<4> {
    MAGMA_PROPERTY( channels, std::vector<frantic::tstring> )
  public:
    inline const float get_radius() const { return boost::get<float>( this->get_input_default_value( 2 ) ); }
    inline void set_radius( float val ) { this->set_input_default_value( 2, val ); }

    inline const float get_falloffPower() const { return boost::get<float>( this->get_input_default_value( 3 ) ); }
    inline void set_falloffPower( float val ) { this->set_input_default_value( 3, val ); }

  public:
    struct meta {
        enum { ARITY = 3 };
        typedef frantic::magma::functors::particle_sum_radius type;
        typedef boost::mpl::vector<void( void*, vec3, float, float )> bindings;
    };

  public:
    static void create_type_definition( magma_node_type& outType );

    virtual void compile( magma_compiler_interface& compile );

    magma_particle_kernel_node();

    virtual int get_num_outputs() const;

    virtual void get_output_description( int index, frantic::tstring& outDescription ) const;
};

} // namespace nodes
} // namespace magma
} // namespace frantic
