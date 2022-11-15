// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/simple_compiler/base_compiler_impl.hpp> //Only needed for traits!
#include <frantic/magma/simple_compiler/simple_mesh_compiler.hpp>

#pragma warning( push, 3 )
#pragma warning( disable : 4100 )
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#pragma warning( pop )

#include <memory>
#include <utility>

// Include last so assert() doesn't accidentally get redefined
#include <frantic/misc/hybrid_assert.hpp>

namespace frantic {
namespace magma {
namespace simple_compiler {

class buffered_mesh_channel : public frantic::geometry::mesh_channel {
    const frantic::geometry::mesh_channel* m_bufferedChannel;

    boost::scoped_array<char> m_buffer;
    std::size_t m_elementSize;

  public:
    buffered_mesh_channel( const frantic::geometry::mesh_channel* bufferedChannel )
        : mesh_channel( bufferedChannel->get_name(), bufferedChannel->get_channel_type(),
                        bufferedChannel->get_data_type(), bufferedChannel->get_data_arity(),
                        bufferedChannel->get_num_elements(), bufferedChannel->get_num_faces(),
                        !bufferedChannel->is_writeable() ) {
        this->set_transform_type( bufferedChannel->get_transform_type() );

        m_bufferedChannel = bufferedChannel;
        m_elementSize = bufferedChannel->get_element_size();
        m_buffer.reset( new char[m_elementSize * bufferedChannel->get_num_elements()] );
    }

    void commit() {
        const char* src = m_buffer.get();
        for( std::size_t i = 0, iEnd = m_bufferedChannel->get_num_elements(); i < iEnd; ++i, src += m_elementSize )
            m_bufferedChannel->set_value( i, src );
    }

    virtual void get_value( std::size_t index, void* outValue ) const {
        return m_bufferedChannel->get_value( index, outValue );
    }

    virtual void set_value( std::size_t index, const void* value ) const {
        assert( index < m_bufferedChannel->get_num_elements() );
        memcpy( m_buffer.get() + index * m_elementSize, value, m_elementSize );
    }

    virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t fvertIndex ) const {
        return m_bufferedChannel->get_fv_index( faceIndex, fvertIndex );
    }

    virtual std::size_t get_num_face_verts( std::size_t faceIndex ) const {
        return m_bufferedChannel->get_num_face_verts( faceIndex );
    }
};

void simple_mesh_compiler::eval() {
    switch( m_iterationPattern ) {
    case VERTEX:
        if( m_geometryIsDirty ) {
            m_currentGeometryHolder.m_geom->allocate_kdtree( true );
            m_geometryIsDirty = false;
        }

        if( this->is_threading() ) {
            tbb::parallel_for( tbb::blocked_range<std::size_t>( 0, m_meshInterface->get_num_verts(), 200 ),
                               boost::bind( &simple_mesh_compiler::eval_vertices, this, _1 ), tbb::auto_partitioner() );
        } else {
            this->eval_vertices( tbb::blocked_range<std::size_t>( 0, m_meshInterface->get_num_verts() ) );
        }

        if( m_currentGeometryHolder.m_geom && !m_currentGeometryHolder.m_geom->is_unique() ) {
            std::map<frantic::tstring, buffered_mesh_channel*>::const_iterator it =
                m_inputChannels.find( _T("Position") );
            if( it != m_inputChannels.end() && it->second != NULL )
                m_geometryIsDirty = true;
        }

        break;
    case FACE:
        if( this->is_threading() ) {
            tbb::parallel_for( tbb::blocked_range<std::size_t>( 0, m_meshInterface->get_num_faces(), 200 ),
                               boost::bind( &simple_mesh_compiler::eval_faces, this, _1 ), tbb::auto_partitioner() );
        } else {
            this->eval_faces( tbb::blocked_range<std::size_t>( 0, m_meshInterface->get_num_faces() ) );
        }
        break;
    case VERTEX_CUSTOM_FACES:
        if( this->is_threading() ) {
            tbb::parallel_for( tbb::blocked_range<std::size_t>( 0, m_meshInterface->get_num_faces(), 200 ),
                               boost::bind( &simple_mesh_compiler::eval_face_vertices, this, _1 ),
                               tbb::auto_partitioner() );
        } else {
            this->eval_face_vertices( tbb::blocked_range<std::size_t>( 0, m_meshInterface->get_num_faces() ) );
        }
        break;
    }

    // Commit any mesh values that were buffered (due to reading & writing the value in the expression).
    for( std::map<frantic::tstring, buffered_mesh_channel*>::iterator it = m_inputChannels.begin(),
                                                                      itEnd = m_inputChannels.end();
         it != itEnd; ++it ) {
        if( it->second != NULL )
            it->second->commit();
    }
}

simple_mesh_compiler::simple_mesh_compiler()
    : m_isThreaded( true )
    , m_iterationPattern( VERTEX )
    , m_geometryIsDirty( false ) {}

simple_mesh_compiler::~simple_mesh_compiler() {
    for( std::map<frantic::tstring, buffered_mesh_channel*>::iterator it = m_inputChannels.begin(),
                                                                      itEnd = m_inputChannels.end();
         it != itEnd; ++it )
        delete it->second;
}

namespace {
struct mesh_state : public base_compiler::state {
    boost::shared_ptr<frantic::geometry::mesh_interface> meshInterface;
    simple_mesh_compiler::MeshIterationPattern iterationPattern;
    std::size_t vertIndex, faceIndex, faceVertIndex;
};
} // namespace

void simple_mesh_compiler::eval_vertices( const tbb::blocked_range<std::size_t>& range ) const {
    mesh_state meshData;
    meshData.meshInterface = m_meshInterface;
    meshData.iterationPattern = VERTEX;
    meshData.faceIndex = 0;
    meshData.faceVertIndex = 0;
    meshData.vertIndex = range.begin();

    for( ; meshData.vertIndex < range.end(); ++meshData.vertIndex )
        this->do_eval( meshData );
}

void simple_mesh_compiler::eval_faces( const tbb::blocked_range<std::size_t>& range ) const {
    mesh_state meshData;
    meshData.meshInterface = m_meshInterface;
    meshData.iterationPattern = FACE;
    meshData.faceIndex = range.begin();
    meshData.faceVertIndex = 0;
    meshData.vertIndex = 0;

    for( ; meshData.faceIndex < range.end(); ++meshData.faceIndex )
        this->do_eval( meshData );
}

void simple_mesh_compiler::eval_face_vertices( const tbb::blocked_range<std::size_t>& range ) const {
    mesh_state meshData;
    meshData.meshInterface = m_meshInterface;
    meshData.iterationPattern = VERTEX_CUSTOM_FACES;
    meshData.faceIndex = range.begin();
    meshData.faceVertIndex = 0;
    meshData.vertIndex = 0;

    for( meshData.faceIndex = range.begin(); meshData.faceIndex < range.end(); ++meshData.faceIndex ) {
        std::size_t numVertsForFace = m_meshInterface->get_num_face_verts( (int)meshData.faceIndex );

        for( meshData.faceVertIndex = 0; meshData.faceVertIndex < numVertsForFace; ++meshData.faceVertIndex ) {
            meshData.vertIndex = m_meshInterface->get_face_vert_index( meshData.faceIndex, meshData.faceVertIndex );

            this->do_eval( meshData );
        }
    }
}

void simple_mesh_compiler::eval_debug( std::vector<debug_data>& outValues, std::size_t maxIterations ) {
    mesh_state meshData;
    meshData.meshInterface = m_meshInterface;
    meshData.iterationPattern = m_iterationPattern;
    meshData.faceIndex = 0;
    meshData.faceVertIndex = 0;
    meshData.vertIndex = 0;

    std::size_t iterTotal;

    switch( m_iterationPattern ) {
    case VERTEX:
        outValues.clear();
        outValues.resize( std::min( m_meshInterface->get_num_verts(), maxIterations ) );

        for( std::size_t iEnd = outValues.size(); meshData.vertIndex < iEnd; ++meshData.vertIndex )
            this->do_eval_debug( meshData, outValues[meshData.vertIndex] );

        break;
    case FACE:
        outValues.clear();
        outValues.resize( std::min( m_meshInterface->get_num_faces(), maxIterations ) );

        for( std::size_t iEnd = outValues.size(); meshData.faceIndex < iEnd; ++meshData.faceIndex )
            this->do_eval_debug( meshData, outValues[meshData.faceIndex] );

        break;
    case VERTEX_CUSTOM_FACES:
        iterTotal = 0;
        for( std::size_t i = 0, iEnd = m_meshInterface->get_num_faces(); i < iEnd; ++i )
            iterTotal += m_meshInterface->get_num_face_verts( (int)i );

        outValues.clear();
        outValues.resize( std::min( iterTotal, maxIterations ) );

        for( std::size_t iEnd = m_meshInterface->get_num_faces(), iter = 0;
             meshData.faceIndex < iEnd && iter < maxIterations; ++meshData.faceIndex ) {

            meshData.faceVertIndex = 0;

            for( std::size_t jEnd = m_meshInterface->get_num_face_verts( (int)meshData.faceIndex );
                 meshData.faceVertIndex < jEnd && iter < maxIterations; ++meshData.faceVertIndex, ++iter ) {
                meshData.vertIndex = m_meshInterface->get_face_vert_index( meshData.faceIndex, meshData.faceVertIndex );

                this->do_eval_debug( meshData, outValues[iter] );
            }
        }
        break;
    }
}

void simple_mesh_compiler::build() {
    reset_expression();

    m_geometryIsDirty = false;

    boost::shared_ptr<frantic::magma::magma_interface> magma = this->get_abstract_syntax_tree_ptr();

    if( !this->get_mesh_interface_ptr() || !this->get_context_ptr() || !magma )
        THROW_MAGMA_INTERNAL_ERROR();

    for( int i = 0, iEnd = magma->get_num_outputs( magma_interface::TOPMOST_LEVEL ); i < iEnd; ++i ) {
        std::pair<frantic::magma::magma_interface::magma_id, int> output =
            magma->get_output( magma_interface::TOPMOST_LEVEL, i );
        if( output.first == magma_interface::INVALID_ID )
            continue;

        magma_node_base* outNode = magma->get_node( output.first );
        if( !outNode )
            THROW_MAGMA_INTERNAL_ERROR();
        if( !outNode->get_enabled() )
            continue;

        // We want to evaluate each output node's input graph before evaluating the inputs so we can be sure we don't
        // read a modified value.
        std::pair<frantic::magma::magma_interface::magma_id, int> input = this->get_node_input( *outNode, 0 );
        if( input.first == magma_interface::INVALID_ID )
            continue;
    }

    for( int i = 0, iEnd = magma->get_num_outputs( magma_interface::TOPMOST_LEVEL ); i < iEnd; ++i ) {
        std::pair<frantic::magma::magma_interface::magma_id, int> output =
            magma->get_output( magma_interface::TOPMOST_LEVEL, i );
        if( output.first == magma_interface::INVALID_ID )
            continue;

        magma_node_base* outNode = magma->get_node( output.first );
        if( !outNode )
            THROW_MAGMA_INTERNAL_ERROR();
        if( !outNode->get_enabled() )
            continue;

        this->visit( *outNode );
    }
}

void simple_mesh_compiler::compile( nodes::magma_input_channel_node* node ) {
    magma_data_type expectedType = node->get_channelType();

    this->compile_input_channel(
        node->get_id(), node->get_channelName(),
        ( expectedType.m_typeName != NULL || expectedType.m_elementType != frantic::channels::data_type_invalid )
            ? &expectedType
            : NULL );
}

void simple_mesh_compiler::compile( nodes::magma_loop_channel_node* node ) {
    m_inputChannels.insert( std::make_pair( node->get_channelName(), static_cast<buffered_mesh_channel*>( NULL ) ) );

    base_compiler::compile( node );
}

void simple_mesh_compiler::compile( nodes::magma_output_node* node ) {
    this->compile_output( node->get_id(), this->get_node_input( *node, 0 ), node->get_channelName(),
                          node->get_channelType() );
}

void simple_mesh_compiler::compile( nodes::magma_vertex_query_node* node ) {
    using frantic::magma::nodes::magma_input_geometry_interface;

    magma_input_geometry_interface* geom = this->get_input_interface<magma_input_geometry_interface>( *node, 0 );

    // We need to check if we are processing the "current" mesh in this query node, so that we can buffer that channel.
    if( this->get_iteration_pattern() == VERTEX && geom->size() == 1 &&
        &geom->get_geometry( 0 )->get_mesh() == &this->get_mesh_interface() ) {
        for( std::vector<frantic::tstring>::const_iterator it = node->get_channels().begin(),
                                                           itEnd = node->get_channels().end();
             it != itEnd; ++it )
            m_inputChannels.insert( std::make_pair( *it, static_cast<buffered_mesh_channel*>( NULL ) ) );
    }

    base_compiler::compile( node );
}

void simple_mesh_compiler::compile( nodes::magma_face_query_node* node ) {
    using frantic::magma::nodes::magma_input_geometry_interface;

    magma_input_geometry_interface* geom = this->get_input_interface<magma_input_geometry_interface>( *node, 0 );

    // We need to check if we are processing the "current" mesh in this query node, so that we can buffer that channel.
    if( this->get_iteration_pattern() == FACE && geom->size() == 1 &&
        &geom->get_geometry( 0 )->get_mesh() == &this->get_mesh_interface() ) {
        for( std::vector<frantic::tstring>::const_iterator it = node->get_channels().begin(),
                                                           itEnd = node->get_channels().end();
             it != itEnd; ++it ) {
            if( m_inputChannels.find( *it ) == m_inputChannels.end() )
                m_inputChannels.insert( std::make_pair( *it, static_cast<buffered_mesh_channel*>( NULL ) ) );
        }
    }

    base_compiler::compile( node );
}

void simple_mesh_compiler::compile( nodes::magma_intersection_node* node ) {
    using frantic::magma::nodes::magma_input_geometry_interface;

    magma_input_geometry_interface* geom = this->get_input_interface<magma_input_geometry_interface>( *node, 0 );

    // We need to check if we are processing the "current" mesh in this query node, so that we can buffer the Position
    // channel (ie. vertex locations).
    if( this->get_iteration_pattern() == FACE && geom->size() == 1 &&
        &geom->get_geometry( 0 )->get_mesh() == &this->get_mesh_interface() )
        m_inputChannels.insert(
            std::make_pair( frantic::tstring( _T("Position") ), static_cast<buffered_mesh_channel*>( NULL ) ) );

    base_compiler::compile( node );
}

void simple_mesh_compiler::compile( nodes::magma_nearest_point_node* node ) {
    using frantic::magma::nodes::magma_input_geometry_interface;

    magma_input_geometry_interface* geom = this->get_input_interface<magma_input_geometry_interface>( *node, 0 );

    // We need to check if we are processing the "current" mesh in this query node, so that we can buffer the Position
    // channel (ie. vertex locations).
    if( this->get_iteration_pattern() == FACE && geom->size() == 1 &&
        &geom->get_geometry( 0 )->get_mesh() == &this->get_mesh_interface() )
        m_inputChannels.insert(
            std::make_pair( frantic::tstring( _T("Position") ), static_cast<buffered_mesh_channel*>( NULL ) ) );

    base_compiler::compile( node );
}

namespace {
struct vertex_indexer {
    inline static std::size_t get_index( const frantic::geometry::mesh_channel& /*accessor*/, mesh_state& data ) {
        return data.vertIndex;
    }
};

struct face_indexer {
    inline static std::size_t get_index( const frantic::geometry::mesh_channel& /*accessor*/, mesh_state& data ) {
        return data.faceIndex;
    }
};

struct face_vertex_indexer {
    inline static std::size_t get_index( const frantic::geometry::mesh_channel& accessor, mesh_state& data ) {
        return accessor.get_fv_index( data.faceIndex, data.faceVertIndex );
    }
};

struct element_indexer {
    inline static std::size_t get_index( const frantic::geometry::mesh_channel& accessor, mesh_state& data ) {
        return accessor.get_fv_index( data.faceIndex, 0 );
    }
};

// TODO We could specialize the template for when no type conversion is required.
template <class DestT, class SrcT, class IndexerType>
class input_expression : public base_compiler::expression {
    const frantic::geometry::mesh_channel* m_accessor;
    std::ptrdiff_t m_out;

    frantic::channels::channel_map m_outMap;

    static void interal_apply( const base_compiler::expression* _this, base_compiler::state& data );

  public:
    input_expression( const frantic::geometry::mesh_channel& accessor )
        : m_accessor( &accessor ) {
        m_outMap.define_channel( accessor.get_name(), accessor.get_data_arity(),
                                 frantic::channels::channel_data_type_traits<DestT>::data_type() );
        m_outMap.end_channel_definition();
    }

    virtual void set_input( std::size_t /*inputIndex*/, std::ptrdiff_t /*relPtr*/ ) {}
    virtual void set_output( std::ptrdiff_t relPtr ) { m_out = relPtr; }
    virtual const frantic::channels::channel_map& get_output_map() const { return m_outMap; }

    virtual void apply( base_compiler::state& data ) const {
        SrcT* in = (SrcT*)alloca( sizeof( SrcT ) * m_accessor->get_data_arity() );

        m_accessor->get_value( IndexerType::get_index( *m_accessor, static_cast<mesh_state&>( data ) ), in );

        for( std::size_t i = 0; i < m_accessor->get_data_arity(); ++i )
            data.set_temporary( m_out + i * sizeof( DestT ), static_cast<DestT>( in[i] ) );
    }

    virtual runtime_ptr get_runtime_ptr() const { return &interal_apply; }
};

template <class DestT, class SrcT, class IndexerType>
void input_expression<DestT, SrcT, IndexerType>::interal_apply( const base_compiler::expression* _this,
                                                                base_compiler::state& data ) {
    static_cast<const input_expression*>( _this )->input_expression::apply( data );
}

template <class IndexerType>
std::unique_ptr<base_compiler::expression> create_input_expression( base_compiler::expression_id exprID,
                                                                    const frantic::geometry::mesh_channel& channel ) {
    std::unique_ptr<base_compiler::expression> result;

    switch( channel.get_data_type() ) {
    case frantic::channels::data_type_float16:
        result.reset( new input_expression<float, half, IndexerType>( channel ) );
        break;
    case frantic::channels::data_type_float32:
        result.reset( new input_expression<float, float, IndexerType>( channel ) );
        break;
    case frantic::channels::data_type_float64:
        result.reset( new input_expression<float, double, IndexerType>( channel ) );
        break;
    case frantic::channels::data_type_int8:
        result.reset( new input_expression<int, char, IndexerType>( channel ) );
        break;
    case frantic::channels::data_type_int16:
        result.reset( new input_expression<int, short, IndexerType>( channel ) );
        break;
    case frantic::channels::data_type_int32:
        result.reset( new input_expression<int, int, IndexerType>( channel ) );
        break;
    case frantic::channels::data_type_int64:
        result.reset( new input_expression<int, boost::int64_t, IndexerType>( channel ) );
        break;
    case frantic::channels::data_type_uint8:
        result.reset( new input_expression<int, unsigned char, IndexerType>( channel ) );
        break;
    case frantic::channels::data_type_uint16:
        result.reset( new input_expression<int, unsigned short, IndexerType>( channel ) );
        break;
    case frantic::channels::data_type_uint32:
        result.reset( new input_expression<int, unsigned int, IndexerType>( channel ) );
        break;
    case frantic::channels::data_type_uint64:
        result.reset( new input_expression<int, boost::uint64_t, IndexerType>( channel ) );
        break;
    default:
        THROW_MAGMA_INTERNAL_ERROR( exprID, (int)channel.get_data_type() );
    }

    return result;
}

template <class ImplType>
class input_index_expression : public base_compiler::expression {
  protected:
    std::ptrdiff_t m_out;

    static void interal_apply( const base_compiler::expression* _this, base_compiler::state& data );

  public:
    virtual void set_input( std::size_t /*inputIndex*/, std::ptrdiff_t /*relPtr*/ ) {}
    virtual void set_output( std::ptrdiff_t relPtr ) { m_out = relPtr; }
    virtual const frantic::channels::channel_map& get_output_map() const { return traits<int>::get_static_map(); }
    virtual void apply( base_compiler::state& data ) const = 0;
    virtual runtime_ptr get_runtime_ptr() const { return &interal_apply; }
};

template <class ImplType>
void input_index_expression<ImplType>::interal_apply( const base_compiler::expression* _this,
                                                      base_compiler::state& data ) {
    static_cast<const ImplType*>( _this )->ImplType::apply( data );
}

class input_vertex_index_expression : public input_index_expression<input_vertex_index_expression> {
  public:
    virtual void apply( base_compiler::state& data ) const {
        data.set_temporary( m_out, static_cast<mesh_state&>( data ).vertIndex );
    }
};

class input_face_index_expression : public input_index_expression<input_face_index_expression> {
  public:
    virtual void apply( base_compiler::state& data ) const {
        data.set_temporary( m_out, static_cast<mesh_state&>( data ).faceIndex );
    }
};

class input_face_vertex_index_expression : public input_index_expression<input_face_vertex_index_expression> {
  public:
    virtual void apply( base_compiler::state& data ) const {
        data.set_temporary( m_out, static_cast<mesh_state&>( data ).faceVertIndex );
    }
};

class input_face_vertex_overall_index_expression
    : public input_index_expression<input_face_vertex_overall_index_expression> {
  public:
    virtual void apply( base_compiler::state& data ) const {
        data.set_temporary( m_out, static_cast<mesh_state&>( data ).faceIndex * 3 +
                                       static_cast<mesh_state&>( data ).faceVertIndex );
    } // HACK assumes triangle mesh!
};

frantic::channels::channel_map g_emptyMap;

template <class DestT, class SrcT>
struct convert_impl {
    inline static DestT apply( const SrcT& val ) { return static_cast<DestT>( val ); }
};

template <class DestT>
struct convert_impl<DestT, bool> {
    inline static DestT apply( bool val ) { return static_cast<DestT>( val ? 1 : 0 ); }
};

template <>
struct convert_impl<half, bool> {
    inline static half apply( bool val ) { return val ? 1.f : 0.f; }
};

template <class DestT, class SrcT, class IndexerType>
class output_expression : public base_compiler::expression {
    const frantic::geometry::mesh_channel* m_accessor;
    std::ptrdiff_t m_inPtr;

    static void interal_apply( const base_compiler::expression* _this, base_compiler::state& data );

  public:
    output_expression( const frantic::geometry::mesh_channel& accessor )
        : m_accessor( &accessor ) {}

    virtual void set_input( std::size_t inputIndex, std::ptrdiff_t relPtr ) {
        if( inputIndex == 0 )
            m_inPtr = relPtr;
    }
    virtual void set_output( std::ptrdiff_t /*relPtr*/ ) {}

    virtual const frantic::channels::channel_map& get_output_map() const {
        if( !g_emptyMap.channel_definition_complete() )
            g_emptyMap.end_channel_definition();
        return g_emptyMap;
    }

    virtual void apply( base_compiler::state& data ) const {
        DestT* temp = (DestT*)alloca( sizeof( DestT ) * m_accessor->get_data_arity() );

        for( std::size_t i = 0; i < m_accessor->get_data_arity(); ++i )
            temp[i] = convert_impl<DestT, SrcT>::apply( data.get_temporary<SrcT>( m_inPtr + i * sizeof( SrcT ) ) );

        mesh_state& meshData = static_cast<mesh_state&>( data );
        std::size_t index = IndexerType::get_index( *m_accessor, meshData );

        index = IndexerType::get_index( *m_accessor, meshData );

        m_accessor->set_value( index, temp );
    }

    virtual runtime_ptr get_runtime_ptr() const { return &interal_apply; }
};

template <class DestT, class SrcT, class IndexerType>
void output_expression<DestT, SrcT, IndexerType>::interal_apply( const base_compiler::expression* _this,
                                                                 base_compiler::state& data ) {
    static_cast<const output_expression*>( _this )->output_expression::apply( data );
}

template <class IndexerType>
std::unique_ptr<base_compiler::expression> create_output_expression( base_compiler::expression_id exprID,
                                                                     const frantic::geometry::mesh_channel& channel,
                                                                     const magma_data_type& inType ) {
    std::unique_ptr<base_compiler::expression> result;

    magma_data_type dt;
    dt.m_elementCount = channel.get_data_arity();
    dt.m_elementType = channel.get_data_type();

    if( inType.m_elementCount != dt.m_elementCount )
        throw magma_exception() << magma_exception::node_id( exprID ) << magma_exception::found_type( inType )
                                << magma_exception::expected_type( dt )
                                << magma_exception::error_name( _T("Cannot write to channel \"") + channel.get_name() +
                                                                _T("\" due to mismatched arity") );

    // TODO Depends on magma internal type representations
    if( inType.m_elementType == frantic::channels::data_type_float32 ) {
        switch( channel.get_data_type() ) {
        case frantic::channels::data_type_float16:
            result.reset( new output_expression<half, float, IndexerType>( channel ) );
            break;
        case frantic::channels::data_type_float32:
            result.reset( new output_expression<float, float, IndexerType>( channel ) );
            break;
        case frantic::channels::data_type_float64:
            result.reset( new output_expression<double, float, IndexerType>( channel ) );
            break;
        case frantic::channels::data_type_int8:
        case frantic::channels::data_type_int16:
        case frantic::channels::data_type_int32:
        case frantic::channels::data_type_int64:
        case frantic::channels::data_type_uint8:
        case frantic::channels::data_type_uint16:
        case frantic::channels::data_type_uint32:
        case frantic::channels::data_type_uint64:
            throw magma_exception() << magma_exception::node_id( exprID ) << magma_exception::found_type( inType )
                                    << magma_exception::expected_type( dt )
                                    << magma_exception::error_name( _T("Cannot write to channel \"") +
                                                                    channel.get_name() +
                                                                    _T("\" due to incompatible types") );
        default:
            THROW_MAGMA_INTERNAL_ERROR( exprID, (int)channel.get_data_type() );
        }
    } else if( inType == traits<bool>::get_type() ) {
        switch( channel.get_data_type() ) {
        case frantic::channels::data_type_float16:
            result.reset( new output_expression<half, bool, IndexerType>( channel ) );
            break;
        case frantic::channels::data_type_float32:
            result.reset( new output_expression<float, bool, IndexerType>( channel ) );
            break;
        case frantic::channels::data_type_float64:
            result.reset( new output_expression<double, bool, IndexerType>( channel ) );
            break;
        case frantic::channels::data_type_int8:
            result.reset( new output_expression<char, bool, IndexerType>( channel ) );
            break;
        case frantic::channels::data_type_int16:
            result.reset( new output_expression<short, bool, IndexerType>( channel ) );
            break;
        case frantic::channels::data_type_int32:
            result.reset( new output_expression<int, bool, IndexerType>( channel ) );
            break;
        case frantic::channels::data_type_int64:
            result.reset( new output_expression<boost::int64_t, bool, IndexerType>( channel ) );
            break;
        case frantic::channels::data_type_uint8:
            result.reset( new output_expression<unsigned char, bool, IndexerType>( channel ) );
            break;
        case frantic::channels::data_type_uint16:
            result.reset( new output_expression<unsigned short, bool, IndexerType>( channel ) );
            break;
        case frantic::channels::data_type_uint32:
            result.reset( new output_expression<unsigned int, bool, IndexerType>( channel ) );
            break;
        case frantic::channels::data_type_uint64:
            result.reset( new output_expression<boost::uint64_t, bool, IndexerType>( channel ) );
            break;
        default:
            THROW_MAGMA_INTERNAL_ERROR( exprID, (int)channel.get_data_type() );
        }
    } else if( inType.m_elementType == frantic::channels::data_type_int32 ) {
        switch( channel.get_data_type() ) {
        case frantic::channels::data_type_float16:
        case frantic::channels::data_type_float32:
        case frantic::channels::data_type_float64:
            throw magma_exception() << magma_exception::node_id( exprID ) << magma_exception::found_type( inType )
                                    << magma_exception::expected_type( dt )
                                    << magma_exception::error_name( _T("Cannot write to channel \"") +
                                                                    channel.get_name() +
                                                                    _T("\" due to incompatible types") );
        case frantic::channels::data_type_int8:
            result.reset( new output_expression<char, int, IndexerType>( channel ) );
            break;
        case frantic::channels::data_type_int16:
            result.reset( new output_expression<short, int, IndexerType>( channel ) );
            break;
        case frantic::channels::data_type_int32:
            result.reset( new output_expression<int, int, IndexerType>( channel ) );
            break;
        case frantic::channels::data_type_int64:
            result.reset( new output_expression<boost::int64_t, int, IndexerType>( channel ) );
            break;
        case frantic::channels::data_type_uint8:
            result.reset( new output_expression<unsigned char, int, IndexerType>( channel ) );
            break;
        case frantic::channels::data_type_uint16:
            result.reset( new output_expression<unsigned short, int, IndexerType>( channel ) );
            break;
        case frantic::channels::data_type_uint32:
            result.reset( new output_expression<unsigned int, int, IndexerType>( channel ) );
            break;
        case frantic::channels::data_type_uint64:
            result.reset( new output_expression<boost::uint64_t, int, IndexerType>( channel ) );
            break;
        default:
            THROW_MAGMA_INTERNAL_ERROR( exprID, (int)channel.get_data_type() );
        }
    } else {
        THROW_MAGMA_INTERNAL_ERROR( exprID, (int)inType.m_elementType );
    }

    return result;
}
} // namespace

using frantic::geometry::mesh_channel;

void simple_mesh_compiler::compile_input_channel( expression_id exprID, const frantic::tstring& channelName,
                                                  const magma_data_type* expectedType ) {
    std::unique_ptr<expression> result;

    // Handle the various index channels manually since they aren't actually part of the mesh.
    if( channelName == _T("VertexIndex") || ( get_iteration_pattern() == VERTEX && channelName == _T("Index") ) ) {
        result.reset( new input_vertex_index_expression );
    } else if( channelName == _T("FaceIndex") || ( get_iteration_pattern() == FACE && channelName == _T("Index") ) ) {
        result.reset( new input_face_index_expression );
    } else if( channelName == _T("FaceCornerIndex") && get_iteration_pattern() == VERTEX_CUSTOM_FACES ) {
        result.reset( new input_face_vertex_index_expression );
    } else if( channelName == _T("Index") && get_iteration_pattern() == VERTEX_CUSTOM_FACES ) {
        result.reset( new input_face_vertex_overall_index_expression );
    } else {
        const mesh_channel* channel = NULL;

        if( this->get_iteration_pattern() == VERTEX ) {
            if( !m_meshInterface->request_channel( channelName, true, false, false ) )
                throw magma_exception() << magma_exception::node_id( exprID )
                                        << magma_exception::property_name( _T("channelName") )
                                        << magma_exception::error_name(
                                               _T("Vertex Channel \"") + channelName +
                                               _T("\" not available in this Geometry Object") );

            channel = m_meshInterface->get_vertex_channels().get_channel( channelName );

            if( channel->get_channel_type() != mesh_channel::vertex )
                throw magma_exception() << magma_exception::node_id( exprID )
                                        << magma_exception::property_name( _T("channelName") )
                                        << magma_exception::error_name(
                                               _T("Vertex Channel \"") + channelName +
                                               _T("\" not available when iterating over vertices") );
        } else if( this->get_iteration_pattern() == FACE ) {
            if( !m_meshInterface->request_channel( channelName, false, false, false ) )
                throw magma_exception() << magma_exception::node_id( exprID )
                                        << magma_exception::property_name( _T("channelName") )
                                        << magma_exception::error_name(
                                               _T("Face Channel \"") + channelName +
                                               _T("\" not available in this Geometry Object") );

            channel = m_meshInterface->get_face_channels().get_channel( channelName );
        } else if( this->get_iteration_pattern() == VERTEX_CUSTOM_FACES ) {
            if( m_meshInterface->request_channel( channelName, true, false, false ) )
                channel = m_meshInterface->get_vertex_channels().get_channel( channelName );
            else if( m_meshInterface->request_channel( channelName, false, false, false ) )
                channel = m_meshInterface->get_face_channels().get_channel( channelName );
            else
                throw magma_exception() << magma_exception::node_id( exprID )
                                        << magma_exception::property_name( _T("channelName") )
                                        << magma_exception::error_name(
                                               _T("Channel \"") + channelName +
                                               _T("\" not available in this Geometry Object") );
        } else {
            THROW_MAGMA_INTERNAL_ERROR();
        }

        // Record this channel as being used.
        m_inputChannels.insert( std::make_pair( channelName, static_cast<buffered_mesh_channel*>( NULL ) ) );

        magma_data_type type;
        type.m_elementType = channel->get_data_type();
        type.m_elementCount = channel->get_data_arity();

        if( expectedType != NULL && *expectedType != type )
            throw magma_exception() << magma_exception::node_id( exprID )
                                    << magma_exception::property_name( _T("channelType") )
                                    << magma_exception::found_type( type )
                                    << magma_exception::expected_type( *expectedType )
                                    << magma_exception::error_name( _T("Channel \"") + channelName +
                                                                    _T("\" is not The Expected Type") );

        switch( this->get_iteration_pattern() ) {
        case VERTEX:
            result = create_input_expression<vertex_indexer>( exprID, *channel );
            break;
        case FACE:
            if( channel->get_channel_type() == mesh_channel::element )
                result = create_input_expression<element_indexer>( exprID, *channel );
            else
                result = create_input_expression<face_indexer>( exprID, *channel );
            break;
        case VERTEX_CUSTOM_FACES:
            result = create_input_expression<face_vertex_indexer>( exprID, *channel );
            break;
        }
    }

    magma_data_type dt;
    dt.m_elementCount = result->get_output_map()[0].arity();
    dt.m_elementType = result->get_output_map()[0].data_type();

    result->set_output( this->allocate_temporary( exprID, dt ).second );

    this->register_expression( std::move( result ) );
}

void simple_mesh_compiler::compile_output( expression_id exprID, const std::pair<expression_id, int>& inputValue,
                                           const frantic::tstring& channelName, const magma_data_type& expectedType ) {
    std::unique_ptr<base_compiler::expression> result;

    temporary_meta inputs[] = { this->get_value( inputValue.first, inputValue.second ) };

    if( m_iterationPattern == VERTEX ) {
        if( !m_meshInterface->request_channel( channelName, true, true, false ) )
            throw magma_exception() << magma_exception::node_id( exprID )
                                    << magma_exception::property_name( _T("channelName") )
                                    << magma_exception::error_name( _T("Vertex Channel \"") + channelName +
                                                                    _T("\" not available in this Geometry Object") );

        const mesh_channel* channel = m_meshInterface->get_vertex_channels().get_channel( channelName );

        if( channel->get_channel_type() != mesh_channel::vertex )
            throw magma_exception() << magma_exception::node_id( exprID )
                                    << magma_exception::property_name( _T("channelName") )
                                    << magma_exception::error_name(
                                           _T("Vertex Channel \"") + channelName +
                                           _T("\" not available for output when iterating over vertices") );

        std::map<frantic::tstring, buffered_mesh_channel*>::iterator it = m_inputChannels.find( channelName );
        if( it != m_inputChannels.end() ) {
            if( it->second != NULL )
                THROW_MAGMA_INTERNAL_ERROR( exprID, frantic::strings::to_string( channelName ) );

            // Redirect the access through a new buffered channel. Also record it so we can later commit & delete it.
            channel = it->second = new buffered_mesh_channel( channel );
        }

        result = create_output_expression<vertex_indexer>( exprID, *channel, inputs[0].first );
    } else if( m_iterationPattern == FACE ) {
        if( !m_meshInterface->request_channel( channelName, false, true, false ) )
            throw magma_exception() << magma_exception::node_id( exprID )
                                    << magma_exception::property_name( _T("channelName") )
                                    << magma_exception::error_name( _T("Face Channel \"") + channelName +
                                                                    _T("\" not available in this Geometry Object") );

        const mesh_channel* channel = m_meshInterface->get_face_channels().get_channel( channelName );

        if( channel->get_channel_type() != mesh_channel::face )
            throw magma_exception() << magma_exception::node_id( exprID )
                                    << magma_exception::property_name( _T("channelName") )
                                    << magma_exception::error_name(
                                           _T("Face Channel \"") + channelName +
                                           _T("\" not available for output when iterating over faces") );

        std::map<frantic::tstring, buffered_mesh_channel*>::iterator it = m_inputChannels.find( channelName );
        if( it != m_inputChannels.end() ) {
            if( it->second != NULL )
                THROW_MAGMA_INTERNAL_ERROR( exprID, frantic::strings::to_string( channelName ) );

            // Redirect the access through a new buffered channel. Also record it so we can later commit & delete it.
            channel = it->second = new buffered_mesh_channel( channel );
        }

        result = create_output_expression<face_indexer>( exprID, *channel, inputs[0].first );
    } else if( m_iterationPattern == VERTEX_CUSTOM_FACES ) {
        if( !m_meshInterface->request_channel( channelName, true, true, false ) )
            throw magma_exception() << magma_exception::node_id( exprID )
                                    << magma_exception::property_name( _T("channelName") )
                                    << magma_exception::error_name( _T("Vertex Channel \"") + channelName +
                                                                    _T("\" not available in this Geometry Object") );

        const mesh_channel* channel = m_meshInterface->get_vertex_channels().get_channel( channelName );

        if( channel->get_channel_type() != mesh_channel::face_vertex )
            throw magma_exception() << magma_exception::node_id( exprID )
                                    << magma_exception::property_name( _T("channelName") )
                                    << magma_exception::error_name(
                                           _T("Vertex Channel \"") + channelName +
                                           _T("\" not available for output when iterating over face-vertices") );

        std::map<frantic::tstring, buffered_mesh_channel*>::iterator it = m_inputChannels.find( channelName );
        if( it != m_inputChannels.end() ) {
            if( it->second != NULL )
                THROW_MAGMA_INTERNAL_ERROR( exprID, frantic::strings::to_string( channelName ) );

            // Redirect the access through a new buffered channel. Also record it so we can later commit & delete it.
            channel = it->second = new buffered_mesh_channel( channel );
        }

        result = create_output_expression<face_vertex_indexer>( exprID, *channel, inputs[0].first );
    } else {
        THROW_MAGMA_INTERNAL_ERROR();
    }

    // TODO We should compare expectedType with the actual type of the channel ... right?

    result->set_input( 0, inputs[0].second );

    this->register_expression( std::move( result ) );
}

void simple_mesh_compiler::compile_current_mesh( expression_id exprID ) {
    if( !m_currentGeometryHolder.m_geom )
        m_currentGeometryHolder.m_geom = magma_geometry_interface::create_instance(
            this->get_mesh_interface_ptr(), this->get_context_data().get_world_transform() );

    this->register_interface( exprID, &m_currentGeometryHolder );
}

} // namespace simple_compiler
} // namespace magma
} // namespace frantic
