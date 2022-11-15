// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/functors/particles.hpp>
#include <frantic/magma/magma_exception.hpp>
#include <frantic/magma/nodes/magma_input_particles_interface.hpp>
#include <frantic/magma/nodes/magma_node_property.hpp>
#include <frantic/magma/nodes/magma_simple_operator.hpp>

#include <frantic/channels/channel_map.hpp>

#include <boost/mpl/vector.hpp>

#include <vector>

namespace frantic {
namespace magma {
namespace nodes {

class magma_particle_query_node : public magma_simple_operator<2> {
    MAGMA_PROPERTY( channels, std::vector<frantic::tstring> )

  public:
    class functor;

    struct meta {
        enum { ARITY = 1 };
        typedef frantic::magma::functors::particle_query type;
        typedef boost::mpl::vector<void( void*, int )> bindings;
    };

  public:
    static void create_type_definition( magma_node_type& outType );

    virtual void compile( magma_compiler_interface& compile );

    magma_particle_query_node();

    virtual int get_num_outputs() const;

    virtual void get_output_description( int index, frantic::tstring& outDescription ) const;
};

class particle_functor_base {
    struct channel_data {
        frantic::channels::channel_general_accessor inAccessor, outAccessor;
        frantic::channels::channel_type_convertor_function_t convertFn;
    };

    std::vector<channel_data> m_channels;

  protected:
    magma_input_particles_interface::const_particle_kdtree_ptr m_kdtree;
    magma_input_particles_interface::const_particle_array_ptr m_particles;

    frantic::channels::channel_map m_outMap;

  public:
    void set_particles( magma_input_particles_interface& prtInterface ) {
        m_particles = prtInterface.get_particles();
        m_kdtree = prtInterface.get_particle_kdtree();
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
};

class magma_particle_query_node::functor : private particle_functor_base {
  public:
    typedef void* return_type;
    typedef boost::mpl::vector<int> param_types;

    inline void calculate_result_layout( frantic::channels::channel_map& map ) const { map = m_outMap; }

  public:
    functor() { m_outMap.end_channel_definition(); }

    void set_particles( magma_input_particles_interface& prtInterface ) {
        particle_functor_base::set_particles( prtInterface );
    }

    void add_channel( const frantic::tstring& channelName ) { particle_functor_base::add_channel( channelName ); }

    inline void operator()( void* out, int particleIndex ) const throw() {
        particle_functor_base::apply( out, static_cast<std::size_t>( particleIndex ) );
    }
};

} // namespace nodes
} // namespace magma
} // namespace frantic
