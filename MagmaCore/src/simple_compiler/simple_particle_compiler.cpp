// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/simple_compiler/base_compiler_impl.hpp>
#include <frantic/magma/simple_compiler/simple_particle_compiler.hpp>

#include <memory>
#include <utility>

namespace frantic {
namespace magma {
namespace simple_compiler {

void simple_particle_compiler::reset( const frantic::channels::channel_map& pcm,
                                      const frantic::channels::channel_map& nativePcm ) {
    this->reset_expression();

    m_channelMap = pcm;
    m_nativeMap = nativePcm;

    this->build();
}

void null_delete( void* ) {}

void simple_particle_compiler::reset( magma_interface& mi, const frantic::channels::channel_map& pcm,
                                      const frantic::channels::channel_map& nativePcm,
                                      boost::shared_ptr<context_base> contextData ) {
    this->reset_expression();

    this->set_abstract_syntax_tree( boost::shared_ptr<magma_interface>( &mi, &null_delete ) );
    this->set_compilation_context( contextData );

    m_channelMap = pcm;
    m_nativeMap = nativePcm;

    this->build();
}

namespace {
class particle_state : public base_compiler::state {
    char* m_particle;
    std::size_t m_particleIndex;

  public:
    particle_state( char* particle, std::size_t index )
        : m_particle( particle )
        , m_particleIndex( index ) {}

    inline char* get_particle_value( const frantic::channels::channel_general_accessor& acc ) {
        return acc.get_channel_data_pointer( m_particle );
    }

    inline std::size_t get_particle_index() const { return m_particleIndex; }
};
} // namespace

void simple_particle_compiler::eval( char* particle, std::size_t index ) const {
    particle_state theState( particle, index );

    this->do_eval( theState );
}

void simple_particle_compiler::eval_debug( char* particle, std::size_t index, debug_data& outValues ) const {
    particle_state theState( particle, index );

    this->do_eval_debug( theState, outValues );
}

namespace {
template <class DestT, class SrcT>
class input_expression : public base_compiler::expression {
    frantic::channels::channel_general_accessor m_in;
    frantic::channels::channel_map m_outMap;
    std::ptrdiff_t m_out;

    static void interal_apply( const base_compiler::expression* _this, base_compiler::state& data ) {
        SrcT* in = reinterpret_cast<SrcT*>( static_cast<particle_state&>( data ).get_particle_value(
            static_cast<const input_expression*>( _this )->m_in ) );

        std::ptrdiff_t out = static_cast<const input_expression*>( _this )->m_out;
        for( std::size_t i = 0, iEnd = static_cast<const input_expression*>( _this )->m_in.arity(); i < iEnd; ++i )
            data.set_temporary( out + i * sizeof( DestT ), static_cast<DestT>( in[i] ) );
    }

  public:
    input_expression( const frantic::channels::channel_general_accessor& inAcc )
        : m_in( inAcc ) {
        m_outMap.define_channel( _T("Value"), inAcc.arity(),
                                 frantic::channels::channel_data_type_traits<DestT>::data_type() );
        m_outMap.end_channel_definition();
    }

    virtual void set_input( std::size_t /*inputIndex*/, std::ptrdiff_t /*relPtr*/ ) {}
    virtual void set_output( std::ptrdiff_t relPtr ) { m_out = relPtr; }
    virtual const frantic::channels::channel_map& get_output_map() const { return m_outMap; }

    virtual void apply( base_compiler::state& data ) const {
        SrcT* in = reinterpret_cast<SrcT*>( static_cast<particle_state&>( data ).get_particle_value( m_in ) );

        for( std::size_t i = 0; i < m_in.arity(); ++i )
            data.set_temporary( m_out + i * sizeof( DestT ), static_cast<DestT>( in[i] ) );
    }

    virtual runtime_ptr get_runtime_ptr() const { return &interal_apply; }
};

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

frantic::channels::channel_map g_emptyMap;

template <class DestT, class SrcT>
class output_expression : public base_compiler::expression {
    std::ptrdiff_t m_in;
    frantic::channels::channel_general_accessor m_out;

    static void interal_apply( const base_compiler::expression* _this, base_compiler::state& data ) {
        DestT* out = reinterpret_cast<DestT*>( static_cast<particle_state&>( data ).get_particle_value(
            static_cast<const output_expression*>( _this )->m_out ) );

        std::ptrdiff_t in = static_cast<const output_expression*>( _this )->m_in;
        for( std::size_t i = 0, iEnd = static_cast<const output_expression*>( _this )->m_out.arity(); i < iEnd; ++i )
            out[i] = convert_impl<DestT, SrcT>::apply( data.get_temporary<SrcT>( in + i * sizeof( SrcT ) ) );
    }

  public:
    output_expression( const frantic::channels::channel_general_accessor& outAcc )
        : m_out( outAcc ) {
        if( !g_emptyMap.channel_definition_complete() )
            g_emptyMap.end_channel_definition();
    }

    virtual void set_input( std::size_t inputIndex, std::ptrdiff_t relPtr ) {
        if( inputIndex == 0 )
            m_in = relPtr;
    }
    virtual void set_output( std::ptrdiff_t /*relPtr*/ ) {}
    virtual const frantic::channels::channel_map& get_output_map() const { return g_emptyMap; }

    virtual void apply( base_compiler::state& data ) const {
        DestT* out = reinterpret_cast<DestT*>( static_cast<particle_state&>( data ).get_particle_value( m_out ) );

        for( std::size_t i = 0; i < m_out.arity(); ++i )
            out[i] = convert_impl<DestT, SrcT>::apply( data.get_temporary<SrcT>( m_in + i * sizeof( SrcT ) ) );
    }

    virtual runtime_ptr get_runtime_ptr() const { return &interal_apply; }
};
} // namespace

void simple_particle_compiler::compile_input_channel( expression_id exprID, const frantic::tstring& channelName,
                                                      const magma_data_type* expectedType ) {
    if( channelName == _T("Index") )
        return this->compile_index_channel( exprID, expectedType );

    if( !get_channel_map().has_channel( channelName ) ) {
        if( !get_native_channel_map().has_channel( channelName ) )
            throw magma_exception() << magma_exception::node_id( exprID )
                                    << magma_exception::property_name( _T("channelName") )
                                    << magma_exception::error_name( _T("Channel \"") + channelName +
                                                                    _T("\" Not Available in this PRT Object") );

        const frantic::channels::channel& nativeCh = get_native_channel_map()[channelName];
        get_channel_map().append_channel( nativeCh.name(), nativeCh.arity(), nativeCh.data_type() );
    }

    frantic::channels::channel_general_accessor acc = get_channel_map().get_general_accessor( channelName );

    std::unique_ptr<expression> result;
    switch( acc.data_type() ) {
    case frantic::channels::data_type_float16:
        result.reset( new input_expression<float, half>( acc ) );
        break;
    case frantic::channels::data_type_float32:
        result.reset( new input_expression<float, float>( acc ) );
        break;
    case frantic::channels::data_type_float64:
        result.reset( new input_expression<float, double>( acc ) );
        break;
    case frantic::channels::data_type_int8:
        result.reset( new input_expression<int, char>( acc ) );
        break;
    case frantic::channels::data_type_int16:
        result.reset( new input_expression<int, short>( acc ) );
        break;
    case frantic::channels::data_type_int32:
        result.reset( new input_expression<int, int>( acc ) );
        break;
    case frantic::channels::data_type_int64:
        result.reset( new input_expression<int, boost::int64_t>( acc ) );
        break;
    case frantic::channels::data_type_uint8:
        result.reset( new input_expression<int, unsigned char>( acc ) );
        break;
    case frantic::channels::data_type_uint16:
        result.reset( new input_expression<int, unsigned short>( acc ) );
        break;
    case frantic::channels::data_type_uint32:
        result.reset( new input_expression<int, unsigned int>( acc ) );
        break;
    case frantic::channels::data_type_uint64:
        result.reset( new input_expression<int, boost::uint64_t>( acc ) );
        break;
    default:
        THROW_MAGMA_INTERNAL_ERROR( exprID, (int)acc.data_type() );
    }

    magma_data_type dt;
    dt.m_elementCount = result->get_output_map()[0].arity();
    dt.m_elementType = result->get_output_map()[0].data_type();

    // We can optionally enforce a specific expected type from the particles.
    if( expectedType != NULL && *expectedType != dt )
        throw magma_exception() << magma_exception::node_id( exprID )
                                << magma_exception::property_name( _T("channelType") )
                                << magma_exception::found_type( dt ) << magma_exception::expected_type( *expectedType )
                                << magma_exception::error_name( _T("Channel \"") + channelName +
                                                                _T("\" is not the expected type") );

    result->set_output( this->allocate_temporary( exprID, dt ).second );

    this->register_expression( std::move( result ) );
}

std::pair<simple_particle_compiler::expression_id, int>
simple_particle_compiler::compile_default_input_channel( const frantic::tstring& channelName,
                                                         const magma_data_type* expectedType ) {
    std::pair<expression_id, int> result = this->get_arbitrary_expression_id();

    this->compile_input_channel( result.first, channelName, expectedType );

    return result;
}

void simple_particle_compiler::compile_output( expression_id exprID, const std::pair<expression_id, int>& inputValue,
                                               const frantic::tstring& channelName,
                                               const magma_data_type& expectedType ) {
    // Add to the native map if this channel is new to this particle stream
    if( !get_native_channel_map().has_channel( channelName ) ) {
        if( expectedType.m_elementType == frantic::channels::data_type_invalid )
            throw magma_exception() << magma_exception::node_id( exprID )
                                    << magma_exception::property_name( _T("channelType") )
                                    << magma_exception::error_name( _T("Channel \"") + channelName +
                                                                    _T("\" does Not Have a Type Specified") );
        get_native_channel_map().append_channel( channelName, expectedType.m_elementCount, expectedType.m_elementType );
    }

    // If the channel is not currently in the 'active' channel map, we need to add it so that we have somewhere to write
    // to. In theory we should probably skip evaluating this part of the expression tree though ...
    if( !get_channel_map().has_channel( channelName ) ) {
        const frantic::channels::channel& nativeCh = get_native_channel_map()[channelName];

        get_channel_map().append_channel( nativeCh.name(), nativeCh.arity(), nativeCh.data_type() );
    }

    temporary_meta inputs[] = { this->get_value( inputValue.first, inputValue.second ) };

    frantic::channels::channel_general_accessor acc = get_channel_map().get_general_accessor( channelName );

    magma_data_type dt;
    dt.m_elementType = acc.data_type();
    dt.m_elementCount = acc.arity();

    if( inputs[0].first.m_elementCount != acc.arity() )
        throw magma_exception() << magma_exception::node_id( exprID ) << magma_exception::found_type( inputs[0].first )
                                << magma_exception::expected_type( dt )
                                << magma_exception::error_name( _T("Cannot write to channel \"") + channelName +
                                                                _T("\" due to mismatched arity") );

    std::unique_ptr<expression> result;
    if( frantic::channels::is_channel_data_type_float( inputs[0].first.m_elementType ) ) {
        switch( acc.data_type() ) {
        case frantic::channels::data_type_float16:
            result.reset( new output_expression<half, float>( acc ) );
            break;
        case frantic::channels::data_type_float32:
            result.reset( new output_expression<float, float>( acc ) );
            break;
        case frantic::channels::data_type_float64:
            result.reset( new output_expression<double, float>( acc ) );
            break;
        case frantic::channels::data_type_int8:
        case frantic::channels::data_type_int16:
        case frantic::channels::data_type_int32:
        case frantic::channels::data_type_int64:
        case frantic::channels::data_type_uint8:
        case frantic::channels::data_type_uint16:
        case frantic::channels::data_type_uint32:
        case frantic::channels::data_type_uint64:
            throw magma_exception() << magma_exception::node_id( exprID )
                                    << magma_exception::found_type( inputs[0].first )
                                    << magma_exception::expected_type( dt )
                                    << magma_exception::error_name( _T("Cannot write to channel \"") + channelName +
                                                                    _T("\" due to incompatible types") );
        default:
            THROW_MAGMA_INTERNAL_ERROR( exprID, (int)acc.data_type() );
        }
    } else if( inputs[0].first == traits<bool>::get_type() ) {
        switch( acc.data_type() ) {
        case frantic::channels::data_type_float16:
            result.reset( new output_expression<half, bool>( acc ) );
            break;
        case frantic::channels::data_type_float32:
            result.reset( new output_expression<float, bool>( acc ) );
            break;
        case frantic::channels::data_type_float64:
            result.reset( new output_expression<double, bool>( acc ) );
            break;
        case frantic::channels::data_type_int8:
            result.reset( new output_expression<char, bool>( acc ) );
            break;
        case frantic::channels::data_type_int16:
            result.reset( new output_expression<short, bool>( acc ) );
            break;
        case frantic::channels::data_type_int32:
            result.reset( new output_expression<int, bool>( acc ) );
            break;
        case frantic::channels::data_type_int64:
            result.reset( new output_expression<boost::int64_t, bool>( acc ) );
            break;
        case frantic::channels::data_type_uint8:
            result.reset( new output_expression<unsigned char, bool>( acc ) );
            break;
        case frantic::channels::data_type_uint16:
            result.reset( new output_expression<unsigned short, bool>( acc ) );
            break;
        case frantic::channels::data_type_uint32:
            result.reset( new output_expression<unsigned int, bool>( acc ) );
            break;
        case frantic::channels::data_type_uint64:
            result.reset( new output_expression<boost::uint64_t, bool>( acc ) );
            break;
        default:
            THROW_MAGMA_INTERNAL_ERROR( exprID, (int)acc.data_type() );
        }
    } else if( inputs[0].first == traits<int>::get_type() ) {
        switch( acc.data_type() ) {
        case frantic::channels::data_type_float16:
        case frantic::channels::data_type_float32:
        case frantic::channels::data_type_float64:
            throw magma_exception() << magma_exception::node_id( exprID )
                                    << magma_exception::found_type( inputs[0].first )
                                    << magma_exception::expected_type( dt )
                                    << magma_exception::error_name( _T("Cannot write to channel \"") + channelName +
                                                                    _T("\" due to incompatible types") );
        case frantic::channels::data_type_int8:
            result.reset( new output_expression<char, int>( acc ) );
            break;
        case frantic::channels::data_type_int16:
            result.reset( new output_expression<short, int>( acc ) );
            break;
        case frantic::channels::data_type_int32:
            result.reset( new output_expression<int, int>( acc ) );
            break;
        case frantic::channels::data_type_int64:
            result.reset( new output_expression<boost::int64_t, int>( acc ) );
            break;
        case frantic::channels::data_type_uint8:
            result.reset( new output_expression<unsigned char, int>( acc ) );
            break;
        case frantic::channels::data_type_uint16:
            result.reset( new output_expression<unsigned short, int>( acc ) );
            break;
        case frantic::channels::data_type_uint32:
            result.reset( new output_expression<unsigned int, int>( acc ) );
            break;
        case frantic::channels::data_type_uint64:
            result.reset( new output_expression<boost::uint64_t, int>( acc ) );
            break;
        default:
            THROW_MAGMA_INTERNAL_ERROR( exprID, (int)acc.data_type() );
        }
    } else
        THROW_MAGMA_INTERNAL_ERROR( exprID, (int)inputs[0].first.m_elementType );

    result->set_input( 0, inputs[0].second );

    this->register_expression( std::move( result ) );
}

void simple_particle_compiler::compile( nodes::magma_input_channel_node* node ) {
    magma_data_type expectedType = node->get_channelType();

    this->compile_input_channel(
        node->get_id(), node->get_channelName(),
        ( expectedType.m_typeName != NULL || expectedType.m_elementType != frantic::channels::data_type_invalid )
            ? &expectedType
            : NULL );
}

void simple_particle_compiler::compile( nodes::magma_output_node* node ) {
    this->compile_output( node->get_id(), this->get_node_input( *node, 0 ), node->get_channelName(),
                          node->get_channelType() );
}

namespace {
class index_expression : public base_compiler::expression {
    std::ptrdiff_t m_out;

    static void internal_apply( const base_compiler::expression* _this, base_compiler::state& data ) {
        data.set_temporary( static_cast<const index_expression*>( _this )->m_out,
                            static_cast<int>( static_cast<particle_state&>( data ).get_particle_index() ) );
    }

  public:
    virtual void set_input( std::size_t /*inputIndex*/, std::ptrdiff_t /*relPtr*/ ) {}
    virtual void set_output( std::ptrdiff_t relPtr ) { m_out = relPtr; }
    virtual const frantic::channels::channel_map& get_output_map() const { return traits<int>::get_static_map(); }

    virtual void apply( base_compiler::state& data ) const {
        data.set_temporary( m_out, static_cast<int>( static_cast<particle_state&>( data ).get_particle_index() ) );
    }

    virtual runtime_ptr get_runtime_ptr() const { return &internal_apply; }
};
} // namespace

void simple_particle_compiler::compile_index_channel( expression_id exprID, const magma_data_type* expectedType ) {
    magma_data_type type = *magma_singleton::get_named_data_type( _T("Int") );

    // We can optionally enforce a specific expected type from the particles.
    if( expectedType != NULL && *expectedType != type )
        throw magma_exception() << magma_exception::node_id( exprID )
                                << magma_exception::property_name( _T("channelType") )
                                << magma_exception::found_type( type )
                                << magma_exception::expected_type( *expectedType )
                                << magma_exception::error_name( _T("Channel \"Index\" Is Not The Expected Type") );

    std::unique_ptr<expression> result( new index_expression );

    magma_data_type dt;
    dt.m_elementCount = result->get_output_map()[0].arity();
    dt.m_elementType = result->get_output_map()[0].data_type();

    result->set_output( this->allocate_temporary( exprID, dt ).second );

    this->register_expression( std::move( result ) );
}

} // namespace simple_compiler
} // namespace magma
} // namespace frantic
