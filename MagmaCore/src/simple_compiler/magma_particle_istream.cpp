// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/simple_compiler/magma_particle_istream.hpp>
#include <frantic/magma/simple_compiler/simple_particle_compiler.hpp>

#include <frantic/channels/channel_map_adaptor.hpp>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

#include <memory>

namespace frantic {
namespace magma {
namespace simple_compiler {

class simple_compiler_particle_istream : public frantic::particles::streams::delegated_particle_istream {
    simple_particle_compiler m_compiler;
    boost::shared_ptr<magma_interface> m_magma;

    frantic::channels::channel_map m_map;
    frantic::channels::channel_map_adaptor m_adaptor;

    boost::scoped_array<char> m_parallelBuffer;
    std::size_t m_parallelBufferSize;

    bool m_hasParticlesCached;
    std::size_t m_currentCachedParticle;

    exception_callback m_errorCallback;

  public:
    simple_compiler_particle_istream( particle_istream_ptr delegatePin, boost::shared_ptr<magma_interface> magma,
                                      boost::shared_ptr<magma_compiler_interface::context_base> contextData )
        : delegated_particle_istream( delegatePin )
        , m_magma( magma )
        , m_parallelBufferSize( 0 ) {
        m_map = m_delegate->get_channel_map();
        m_compiler.set_abstract_syntax_tree( magma );
        m_compiler.set_compilation_context( contextData );
        m_compiler.reset( m_delegate->get_channel_map(), m_delegate->get_native_channel_map() );
        // m_compiler.reset( *m_magma, m_delegate->get_channel_map(), m_delegate->get_native_channel_map(), contextData
        // );
        m_delegate->set_channel_map( m_compiler.get_channel_map() );

        m_adaptor.set( m_map, m_compiler.get_channel_map() );

        m_hasParticlesCached = false;
        m_currentCachedParticle = std::numeric_limits<std::size_t>::max();
    }

    inline void set_error_callback( const exception_callback& ec ) { m_errorCallback = ec; }

    virtual void set_channel_map( const frantic::channels::channel_map& map ) {
        // Only update anything if our channel map is actually changing.
        if( map != m_map ) {
            m_map = map;
            try {
                m_compiler.reset( map, m_delegate->get_native_channel_map() );
                // m_compiler.reset( *m_magma, map, m_delegate->get_native_channel_map(),
                // m_compiler.get_context_data_ptr() );
            } catch( const magma_exception& e ) {
                if( m_errorCallback )
                    m_errorCallback( e );
                throw;
            }
            m_delegate->set_channel_map( m_compiler.get_channel_map() );

            m_adaptor.set( m_map, m_compiler.get_channel_map() );
        }
    }

    virtual const frantic::channels::channel_map& get_channel_map() const { return m_map; }

    virtual const frantic::channels::channel_map& get_native_channel_map() const {
        return m_compiler.get_native_channel_map();
    }

    virtual std::size_t particle_size() const { return m_map.structure_size(); }

    void set_default_particle( char* newDefault ) {
        const frantic::channels::channel_map& myMap = get_channel_map();
        const frantic::channels::channel_map& delegateMap = m_delegate->get_channel_map();

        frantic::channels::channel_map_adaptor tempAdaptor( delegateMap, myMap );

        if( tempAdaptor.is_identity() ) {
            m_delegate->set_default_particle( newDefault );
        } else {
            char* tempDefault = (char*)alloca( delegateMap.structure_size() );

            delegateMap.construct_structure( tempDefault );

            tempAdaptor.copy_structure( tempDefault, newDefault );

            m_delegate->set_default_particle( tempDefault );
        }
    }

    virtual bool get_particle( char* particle );
    virtual bool get_particles( char* particles, std::size_t& inoutCount );
};

bool simple_compiler_particle_istream::get_particle( char* particle ) {
    boost::int64_t index = m_delegate->particle_index();

    /*if( index < 0 ){
            m_hasParticlesCached = m_compiler.get_input_particles().get() != NULL;
            m_currentCachedParticle = 0;

            if( m_hasParticlesCached ){
                    frantic::particles::particle_array& particles = *m_compiler.get_input_particles();
                    frantic::particles::particle_kdtree< frantic::magma::nodes::detail::particle_standin >& kdtree =
    *m_compiler.get_input_particle_kdtree();

                    particles.reset( m_compiler.get_channel_map() );

                    char* buffer = (char*)alloca( particles.get_channel_map().structure_size() );

                    while( m_delegate->get_particle( buffer ) )
                            particles.push_back( buffer );

                    frantic::channels::channel_accessor<frantic::graphics::vector3f> posAccessor =
    particles.get_channel_map().get_accessor<frantic::graphics::vector3f>( "Position" );

                    kdtree.clear();

                    boost::int64_t i = 0;
                    for( frantic::particles::particle_array::const_iterator it = particles.begin(), itEnd =
    particles.end(); it != itEnd; ++it, ++i ) kdtree.add_particle( frantic::magma::nodes::detail::particle_standin(
    posAccessor.get( *it ), i ) );

                    kdtree.balance_kdtree();
            }
    }

    if( m_hasParticlesCached ){
            frantic::particles::particle_array& particles = *m_compiler.get_input_particles();

            if( m_currentCachedParticle >= particles.size() ){
                    particles.clear();
                    return false;
            }

            char* buffer = m_adaptor.is_identity() ? particle : (char*)alloca(
    particles.get_channel_map().structure_size() );

            memcpy( buffer, particles.at( m_currentCachedParticle++ ), particles.get_channel_map().structure_size() );

            m_compiler.eval( buffer, static_cast<std::size_t>( index ) );

            if( buffer != particle )
                    m_adaptor.copy_structure( particle, buffer );

            return true;
    }else{*/
    char* buffer = m_adaptor.is_identity() ? particle : (char*)alloca( m_compiler.get_channel_map().structure_size() );

    if( !m_delegate->get_particle( buffer ) )
        return false;

    m_compiler.eval( buffer, static_cast<std::size_t>( index + 1 ) );

    if( buffer != particle )
        m_adaptor.copy_structure( particle, buffer );

    return true;
    //}
}

struct parallel_particle_op_adaptor {
    std::size_t indexBase;
    char *src, *dest;

    const frantic::channels::channel_map_adaptor& adaptor;
    const frantic::magma::simple_compiler::simple_particle_compiler& compiler;

    parallel_particle_op_adaptor( const frantic::magma::simple_compiler::simple_particle_compiler& _compiler,
                                  std::size_t _indexBase, char* _src, char* _dest,
                                  const frantic::channels::channel_map_adaptor& _adaptor )
        : adaptor( _adaptor )
        , compiler( _compiler )
        , indexBase( _indexBase )
        , src( _src )
        , dest( _dest ) {}

    void operator()( const tbb::blocked_range<std::size_t>& range ) const {
        char* src = this->src + range.begin() * adaptor.source_size();
        char* dest = this->dest + range.begin() * adaptor.dest_size();
        for( std::size_t i = range.begin(); i < range.end();
             ++i, src += adaptor.source_size(), dest += adaptor.dest_size() ) {
            compiler.eval( src, indexBase + i );
            adaptor.copy_structure( dest, src );
        }
    }
};

struct parallel_particle_op {
    std::size_t indexBase;
    char* particles;

    const frantic::magma::simple_compiler::simple_particle_compiler& compiler;

    parallel_particle_op( const frantic::magma::simple_compiler::simple_particle_compiler& _compiler,
                          std::size_t _indexBase, char* _particles )
        : compiler( _compiler )
        , indexBase( _indexBase )
        , particles( _particles ) {}

    void operator()( const tbb::blocked_range<std::size_t>& range ) const {
        char* particles = this->particles + range.begin() * compiler.get_channel_map().structure_size();
        for( std::size_t i = range.begin(); i < range.end();
             ++i, particles += compiler.get_channel_map().structure_size() )
            compiler.eval( particles, indexBase + i );
    }
};

bool simple_compiler_particle_istream::get_particles( char* particles, std::size_t& inoutCount ) {
    /*for( std::size_t i = 0; i < inoutCount; ++i, particles += m_adaptor.dest_size() ){
            if( !this->get_particle( particles ) ){
                    inoutCount = i;
                    return false;
            }
    }
    return true;*/
    bool result;
    std::size_t indexBase = static_cast<std::size_t>( m_delegate->particle_index() + 1 );

    if( !m_adaptor.is_identity() ) {
        if( inoutCount > m_parallelBufferSize ) {
            m_parallelBufferSize = std::max<std::size_t>( 4096u, std::max( 2 * m_parallelBufferSize, inoutCount ) );
            m_parallelBuffer.reset( new char[m_compiler.get_channel_map().structure_size() * inoutCount] );
        }

        char* buffer = m_parallelBuffer.get();

        result = m_delegate->get_particles( buffer, inoutCount );

        // TODO: replace this loop with a parallel_for
        /*for( std::size_t i = 0; i < inoutCount; ++i, buffer += m_adaptor.source_size(), particles +=
        m_adaptor.dest_size() ){ m_compiler.eval( buffer, indexBase + i ); m_adaptor.copy_structure( particles, buffer
        );
        }*/
        tbb::parallel_for( tbb::blocked_range<std::size_t>( 0, inoutCount ),
                           parallel_particle_op_adaptor( m_compiler, indexBase, buffer, particles, m_adaptor ) );
    } else {
        result = m_delegate->get_particles( particles, inoutCount );

        // TODO: replace this loop with a parallel_for
        // for( std::size_t i = 0; i < inoutCount; ++i, particles += m_map.structure_size() )
        //	m_compiler.eval( particles, indexBase + i );
        tbb::parallel_for( tbb::blocked_range<std::size_t>( 0, inoutCount ),
                           parallel_particle_op( m_compiler, indexBase, particles ) );
    }

    return result;
}

particle_istream_ptr
apply_simple_compiler_expression( particle_istream_ptr pin, boost::shared_ptr<magma_interface> magma,
                                  boost::shared_ptr<magma_compiler_interface::context_base> contextData,
                                  const exception_callback& errorCallback ) {
    int numOutputs = magma->get_num_outputs( magma_interface::TOPMOST_LEVEL );
    if( numOutputs > 0 ) {
        bool doesSomething = false;

        for( int i = 0; !doesSomething && i < numOutputs; ++i ) {
            if( magma->get_output( magma_interface::TOPMOST_LEVEL, i ).first != magma_interface::INVALID_ID )
                doesSomething = true;
        }

        if( doesSomething ) {
            std::unique_ptr<simple_compiler_particle_istream> magmaStream(
                new simple_compiler_particle_istream( pin, magma, contextData ) );

            magmaStream->set_error_callback( errorCallback );

            pin.reset( magmaStream.release() );
        }
    }

    return pin;
}

} // namespace simple_compiler
} // namespace magma
} // namespace frantic
