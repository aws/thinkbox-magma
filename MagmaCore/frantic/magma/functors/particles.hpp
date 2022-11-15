// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_singleton.hpp>
#include <frantic/magma/nodes/magma_input_particles_interface.hpp>

//#include <tbb/scalable_allocator.h>

namespace frantic {
namespace magma {
namespace functors {

class nearest_particle {
    frantic::magma::nodes::magma_input_particles_interface::const_particle_kdtree_ptr m_kdtree;
    frantic::channels::channel_map m_outMap; // Could be static

    struct results_holder {
        frantic::graphics::vector3f pos;
        bool valid;
        int prtIndex;
    };

  public:
    nearest_particle() {
        m_outMap.define_channel( _T("Position"), 3, frantic::channels::data_type_float32,
                                 offsetof( results_holder, pos ) );
        m_outMap.define_channel( _T("IsValid"), 1,
                                 frantic::magma::magma_singleton::get_named_data_type( _T("Bool") )->m_elementType,
                                 offsetof( results_holder, valid ) );
        m_outMap.define_channel( _T("ParticleIndex"), 1, frantic::channels::data_type_int32,
                                 offsetof( results_holder, prtIndex ) );
        m_outMap.end_channel_definition( 4u, true, false );
    }

    void set_particles( frantic::magma::nodes::magma_input_particles_interface& prtInterface ) {
        m_kdtree = prtInterface.get_particle_kdtree();
    }

    const frantic::channels::channel_map& get_output_map() const { return m_outMap; }

    void operator()( void* _out, const frantic::graphics::vector3f& pos ) const throw() {
        results_holder& out = *static_cast<results_holder*>( _out );

        frantic::magma::nodes::detail::particle_standin standin;

        try {
            if( m_kdtree->size() > 0 ) {
                standin = m_kdtree->locate_closest_particle( pos );

                out.pos = standin.pos;
                out.valid = true;
                out.prtIndex = static_cast<int>( standin.index );
            } else {
                memset( &out, 0, sizeof( results_holder ) );
            }
        } catch( ... ) { // Swallow all exceptions. We can't easily print anything due to threading constraints, and we
                         // don't have an elaborate error reporting structure.
            memset( &out, 0, sizeof( results_holder ) );
        }
    }
};

class nth_nearest_particle {
    frantic::magma::nodes::magma_input_particles_interface::const_particle_kdtree_ptr m_kdtree;
    frantic::channels::channel_map m_outMap; // Could be static
    int m_whichNearest;

    struct results_holder {
        frantic::graphics::vector3f pos;
        bool valid;
        int prtIndex;
    };

  public:
    nth_nearest_particle( int whichNearest )
        : m_whichNearest( whichNearest ) {
        m_outMap.define_channel( _T("Position"), 3, frantic::channels::data_type_float32,
                                 offsetof( results_holder, pos ) );
        m_outMap.define_channel( _T("IsValid"), 1,
                                 frantic::magma::magma_singleton::get_named_data_type( _T("Bool") )->m_elementType,
                                 offsetof( results_holder, valid ) );
        m_outMap.define_channel( _T("ParticleIndex"), 1, frantic::channels::data_type_int32,
                                 offsetof( results_holder, prtIndex ) );
        m_outMap.end_channel_definition( 4u, true, false );
    }

    void set_particles( frantic::magma::nodes::magma_input_particles_interface& prtInterface ) {
        m_kdtree = prtInterface.get_particle_kdtree();
    }

    const frantic::channels::channel_map& get_output_map() const { return m_outMap; }

    inline void operator()( void* _out, const frantic::graphics::vector3f& pos ) const throw() {
        results_holder& out = *static_cast<results_holder*>( _out );

        try {
            if( m_kdtree->size() > static_cast<std::size_t>( m_whichNearest ) ) {
                std::vector<std::pair<float, frantic::magma::nodes::detail::particle_standin>>
                    results; // TODO Could preallocate or cache this. Maybe use alloca instead of a vector...

                m_kdtree->locate_particles_count( pos, m_whichNearest, results );

                out.pos = results.back().second.pos;
                out.valid = true;
                out.prtIndex = static_cast<int>( results.back().second.index );
            } else {
                memset( &out, 0, sizeof( results_holder ) );
            }
        } catch( ... ) { // Swallow all exceptions. We can't easily print anything due to threading constraints, and we
                         // don't have an elaborate error reporting structure.
            memset( &out, 0, sizeof( results_holder ) );
        }
    }
};

namespace detail {
class particle_functor_base {
    struct channel_data {
        frantic::channels::channel_general_accessor inAccessor, outAccessor;
        frantic::channels::channel_type_convertor_function_t convertFn;
    };

    std::vector<channel_data> m_channels;

  protected:
    frantic::magma::nodes::magma_input_particles_interface::const_particle_kdtree_ptr m_kdtree;
    frantic::magma::nodes::magma_input_particles_interface::const_particle_array_ptr m_particles;

    frantic::channels::channel_map m_outMap;

  protected:
    inline void apply( void* out, std::size_t particleIndex ) const {
        if( particleIndex < m_particles->size() ) {
            for( std::vector<channel_data>::const_iterator chIt = m_channels.begin(), chItEnd = m_channels.end();
                 chIt != chItEnd; ++chIt ) {
                chIt->convertFn( chIt->outAccessor.get_channel_data_pointer( (char*)out ),
                                 chIt->inAccessor.get_channel_data_pointer( m_particles->at( particleIndex ) ),
                                 chIt->inAccessor.arity() );
            }
        } else {
            memset( out, 0, m_outMap.structure_size() );
        }
    }

    inline void apply( void* out,
                       const std::vector<std::pair<float, nodes::detail::particle_standin>>& nearestParticles ) const {
        // Allocate a temporary buffer to accumulate the weighted sums of the nearest particles before conversion to the
        // output type.
        char* tempBuffer = (char*)alloca( m_particles->get_channel_map().structure_size() );

        memset( tempBuffer, 0, m_particles->get_channel_map().structure_size() );

        for( std::vector<channel_data>::const_iterator chIt = m_channels.begin(), chItEnd = m_channels.end();
             chIt != chItEnd; ++chIt ) {
            for( std::vector<std::pair<float, nodes::detail::particle_standin>>::const_iterator
                     it = nearestParticles.begin(),
                     itEnd = nearestParticles.end();
                 it != itEnd; ++it ) {
                float weight = it->first; // Note: This is not normalized with the weight sum ... should it be?
                const char* src = chIt->inAccessor.get_channel_data_pointer(
                    m_particles->at( static_cast<std::size_t>( it->second.index ) ) );
                char* dest = chIt->inAccessor.get_channel_data_pointer( tempBuffer );

                chIt->inAccessor.weighted_increment( weight, src, dest );
            }

            chIt->convertFn( chIt->outAccessor.get_channel_data_pointer( (char*)out ),
                             chIt->inAccessor.get_channel_data_pointer( tempBuffer ), chIt->inAccessor.arity() );
        }
    }

  public:
    const frantic::channels::channel_map& get_output_map() const { return m_outMap; }

    void set_particles( frantic::magma::nodes::magma_input_particles_interface& prtInterface ) {
        m_particles = prtInterface.get_particles();
        m_kdtree = prtInterface.get_particle_kdtree();
    }

    void set_particles( frantic::magma::nodes::magma_input_particles_interface::const_particle_array_ptr particles,
                        frantic::magma::nodes::magma_input_particles_interface::const_particle_kdtree_ptr kdtree ) {
        m_particles = particles;
        m_kdtree = kdtree;
    }

    void add_channel( const frantic::tstring& channelName ) {
        if( !m_particles->get_channel_map().has_channel( channelName ) )
            throw magma_exception() << magma_exception::property_name( _T("channels") )
                                    << magma_exception::error_name(
                                           _T("Channel \"") + channelName +
                                           _T("\" is not available in this InputParticles object") );

        channel_data cd;
        cd.inAccessor = m_particles->get_channel_map().get_general_accessor( channelName );

        if( !m_outMap.has_channel( channelName ) ) {
            if( frantic::channels::is_channel_data_type_float( cd.inAccessor.data_type() ) )
                m_outMap.append_channel( channelName, cd.inAccessor.arity(), frantic::channels::data_type_float32 );
            else if( frantic::channels::is_channel_data_type_int( cd.inAccessor.data_type() ) )
                m_outMap.append_channel( channelName, cd.inAccessor.arity(), frantic::channels::data_type_int32 );
            else
                throw magma_exception() << magma_exception::property_name( _T("channels") )
                                        << magma_exception::error_name(
                                               _T("Channel \"") + channelName +
                                               _T("\" of InputParticles object is not a supported type") );
        }

        cd.outAccessor = m_outMap.get_general_accessor( channelName );
        cd.convertFn = frantic::channels::get_channel_type_convertor_function(
            cd.inAccessor.data_type(), cd.outAccessor.data_type(), channelName );

        m_channels.push_back( cd );
    }
};
} // namespace detail

class particle_sum_count : public detail::particle_functor_base {
    frantic::channels::channel_accessor<float> m_maxDistanceAcc, m_weightSumAcc;

  public:
    particle_sum_count() {
        m_outMap.define_channel<float>( _T("MaxDistance") );
        m_outMap.define_channel<float>( _T("TotalWeight") );
        m_outMap.end_channel_definition( 4u, true );

        m_maxDistanceAcc = m_outMap.get_accessor<float>( _T("MaxDistance") );
        m_weightSumAcc = m_outMap.get_accessor<float>( _T("TotalWeight") );
    }

    inline void operator()( void* out, const frantic::graphics::vector3f& pos, int numNeighbors,
                            float falloffPower ) const throw() {
        try {
            if( numNeighbors <= 0 ) {
                memset( out, 0, m_outMap.structure_size() );
            } else {
                // TODO: It would be better to use a tbb::scalable_allocator (or just a C-style array since we have an
                // upper limit ...)
                // std::vector< std::pair<float,nodes::detail::particle_standin>, tbb::scalable_allocator<
                // std::pair<float,nodes::detail::particle_standin> > > nearestParticles;
                std::vector<std::pair<float, nodes::detail::particle_standin>> nearestParticles;

                nearestParticles.reserve( numNeighbors );

                m_kdtree->locate_particles_count( pos, numNeighbors, nearestParticles );

                if( !nearestParticles.empty() ) {
                    m_maxDistanceAcc.get( (char*)out ) = std::sqrt( nearestParticles.back().first );

                    float weightSum = 0.f;

                    // For each particle, assign it a weight
                    for( std::vector<std::pair<float, nodes::detail::particle_standin>>::iterator
                             it = nearestParticles.begin(),
                             itEnd = nearestParticles.end();
                         it != itEnd; ++it ) {
                        float weight = std::pow( 1.f + std::sqrt( it->first ),
                                                 -falloffPower ); // We have the squared distance, and want the inverse
                                                                  // distance to a specific power.

                        it->first = weight;

                        weightSum += weight;
                    }

                    m_weightSumAcc.get( (char*)out ) = weightSum;

                    particle_functor_base::apply( out, nearestParticles );
                } else {
                    memset( out, 0, m_outMap.structure_size() );
                }
            }
        } catch( ... ) {
            memset( out, 0, m_outMap.structure_size() );
        }
    }
};

class particle_sum_radius : public detail::particle_functor_base {
    frantic::channels::channel_accessor<int> m_numParticlesAcc;
    frantic::channels::channel_accessor<float> m_weightSumAcc;

  public:
    particle_sum_radius() {
        m_outMap.define_channel<int>( _T("NumParticles") );
        m_outMap.define_channel<float>( _T("TotalWeight") );
        m_outMap.end_channel_definition( 4u, true );

        m_numParticlesAcc = m_outMap.get_accessor<int>( _T("NumParticles") );
        m_weightSumAcc = m_outMap.get_accessor<float>( _T("TotalWeight") );
    }

    inline void operator()( void* out, const frantic::graphics::vector3f& pos, float radius, float falloffPower ) const
        throw() {
        try {
            if( radius <= 0 ) {
                m_numParticlesAcc.get( static_cast<char*>( out ) ) = 0;
                m_weightSumAcc.get( static_cast<char*>( out ) ) = 0;
                return;
            }

            std::vector<std::pair<float, nodes::detail::particle_standin>> nearestParticles;

            float r2 = radius * radius;

            m_kdtree->locate_particles_range( pos, r2, nearestParticles );

            if( !nearestParticles.empty() ) {
                m_numParticlesAcc.get( (char*)out ) = static_cast<int>( nearestParticles.size() );

                float weightSum = 0.f;

                // For each particle, assign it a weight
                for( std::vector<std::pair<float, nodes::detail::particle_standin>>::iterator
                         it = nearestParticles.begin(),
                         itEnd = nearestParticles.end();
                     it != itEnd; ++it ) {
                    float weight =
                        ( it->first <= r2 ) ? powf( 1.f - std::sqrt( it->first ) / radius, falloffPower ) : 0.f;

                    it->first = weight;

                    weightSum += weight;
                }

                m_weightSumAcc.get( (char*)out ) = weightSum;

                particle_functor_base::apply( out, nearestParticles );
            } else {
                memset( out, 0, m_outMap.structure_size() );
            }
        } catch( ... ) {
            memset( out, 0, m_outMap.structure_size() );
        }
    }
};

class particle_query : public detail::particle_functor_base {
  public:
    particle_query() { m_outMap.end_channel_definition(); }

    // This hides the base class version. We want to avoid creating a kdtree in this case.
    void set_particles( frantic::magma::nodes::magma_input_particles_interface& prtInterface ) {
        detail::particle_functor_base::set_particles(
            prtInterface.get_particles(),
            frantic::magma::nodes::magma_input_particles_interface::const_particle_kdtree_ptr() );
    }

    void set_particles( frantic::magma::nodes::magma_input_particles_interface::const_particle_array_ptr particles ) {
        detail::particle_functor_base::set_particles(
            particles, frantic::magma::nodes::magma_input_particles_interface::const_particle_kdtree_ptr() );
    }

    inline void operator()( void* out, int particleIndex ) const throw() {
        particle_functor_base::apply( out, static_cast<std::size_t>( particleIndex ) );
    }
};

} // namespace functors
} // namespace magma
} // namespace frantic
