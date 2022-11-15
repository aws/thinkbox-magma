// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/simple_compiler/base_compiler.hpp>
#include <frantic/magma/simple_compiler/base_compiler_impl.hpp>

#include <frantic/magma/nodes/magma_blop_node.hpp>

#include <frantic/magma/functors/geometry.hpp>
#include <frantic/magma/functors/particles.hpp>

#include <frantic/volumetrics/field_interface.hpp>

#include <boost/scope_exit.hpp>

#include <memory>
#include <utility>

namespace frantic {
namespace magma {
namespace simple_compiler {

namespace {
const int DEFAULT_VALUE_ID = -3; // This ID is used when allocating default values
const int TEMPORARY_NODE_ID =
    -13; // This ID is used temporarily while compiling some nodes, but should never appear in a finished expression.
} // namespace

frantic::channels::channel_map traits<float>::s_map;
frantic::channels::channel_map traits<int>::s_map;
frantic::channels::channel_map traits<bool>::s_map;
frantic::channels::channel_map traits<vec3>::s_map;
frantic::channels::channel_map traits<quat>::s_map;

base_compiler::base_compiler()
    : m_temporaryStorageSize( 0 )
    , m_currentLoop( NULL ) {}

base_compiler::~base_compiler() {
    for( expression_list::iterator it = m_currentExpression.begin(), itEnd = m_currentExpression.end(); it != itEnd;
         ++it )
        delete *it;
    m_currentExpression.clear();
}

void base_compiler::do_move( base_compiler& other ) {
    m_magma = other.m_magma;
    m_context = other.m_context;
    m_temporaryStorageSize = other.m_temporaryStorageSize;

    m_temporaryResults.swap( other.m_temporaryResults );
    m_currentExpression.swap( other.m_currentExpression );
}

boost::shared_ptr<base_compiler::context_base> base_compiler::get_context_ptr() const { return m_context; }

boost::shared_ptr<frantic::magma::magma_interface> base_compiler::get_abstract_syntax_tree_ptr() const {
    return m_magma;
}

void base_compiler::set_abstract_syntax_tree( boost::shared_ptr<frantic::magma::magma_interface> magma ) {
    m_magma = magma;
}

void base_compiler::set_compilation_context( boost::shared_ptr<context_base> context ) { m_context = context; }

bool base_compiler::empty() const { return m_currentExpression.empty(); }

void base_compiler::reset_expression() {
    for( expression_list::iterator it = m_currentExpression.begin(), itEnd = m_currentExpression.end(); it != itEnd;
         ++it )
        delete *it;

    m_temporaryStorageSize = 0;
    m_currentExpression.clear();
    m_temporaryResults.clear();
}

// Don't need to catch exceptions and assign the expression ID because build_subtree will do that.
void base_compiler::build() {
    reset_expression();

    if( !m_context || !m_magma )
        THROW_MAGMA_INTERNAL_ERROR();

    for( int i = 0, iEnd = m_magma->get_num_outputs( magma_interface::TOPMOST_LEVEL ); i < iEnd; ++i ) {
        std::pair<frantic::magma::magma_interface::magma_id, int> output =
            m_magma->get_output( magma_interface::TOPMOST_LEVEL, i );
        if( output.first == magma_interface::INVALID_ID )
            continue;

        magma_node_base* outNode = m_magma->get_node( output.first );
        if( !outNode )
            THROW_MAGMA_INTERNAL_ERROR();
        if( !outNode->get_enabled() || outNode->get_input( 0 ).first == magma_interface::INVALID_ID )
            continue;

        this->build_subtree( outNode->get_id() );
    }
}

void base_compiler::build_subtree( frantic::magma::magma_interface::magma_id nodeID ) {
    try {
        if( nodeID == magma_interface::INVALID_ID )
            THROW_MAGMA_INTERNAL_ERROR();

        if( !this->is_visited( nodeID ) ) {
            magma_node_base* node = m_magma->get_node( nodeID );
            if( !node )
                THROW_MAGMA_INTERNAL_ERROR();

            this->visit( *node );
        }
    } catch( magma_exception& e ) {
        e << magma_exception::source_expression_id( m_magma->get_expression_id() );
        throw;
    }
}

void base_compiler::visit( frantic::magma::magma_node_base& node ) {
    for( int i = 0, iEnd = node.get_num_inputs(); i < iEnd; ++i ) {
        std::pair<frantic::magma::magma_interface::magma_id, int> input = node.get_input( i );
        if( input.first != magma_interface::INVALID_ID && !this->is_visited( input.first ) &&
            ( node.get_input_visitable( i ) || ( i == 0 && !node.get_enabled() ) ) ) {
            magma_node_base* nextNode = m_magma->get_node( input.first );
            if( !nextNode )
                throw magma_exception() << magma_exception::node_id( node.get_id() )
                                        << magma_exception::input_index( i )
                                        << magma_exception::connected_id( input.first )
                                        << magma_exception::connected_output_index( input.second )
                                        << magma_exception::error_name(
                                               _T("The connected node doesn't actually exist!") );

            this->visit( *nextNode );
        }
    }

    if( node.get_enabled() ) {
        // We need to evaluate any contained nodes. TODO: Replace this by generalizing magma_node_base to handle
        // contained nodes.
        //  TODO This could likely be handled via. magma_blop_node::compile() couldn't it?
        if( nodes::magma_blop_node* blopNode = dynamic_cast<nodes::magma_blop_node*>( &node ) ) {
            if( typeid( *blopNode ) == typeid( nodes::magma_blop_node ) ) {
                // Only want to visit inside on a REAL blop node.
                // this->visit( *blopNode->get_internal_output() );
                magma_interface::magma_id internalOutputID = blopNode->get__internal_output_id();
                magma_node_base* nextNode = m_magma->get_node( internalOutputID );
                if( !nextNode )
                    THROW_MAGMA_INTERNAL_ERROR( blopNode->get_id() );

                this->visit( *nextNode );
            }
        }

        // All the inputs are available such that we can finalize the current node.
        try {
            node.compile( *this );
        } catch( frantic::magma::magma_exception& e ) {
            if( e.get_node_id() == frantic::magma::magma_interface::INVALID_ID )
                e << magma_exception::node_id( node.get_id() );
            throw;
        }

        // Make sure we produced the appropriate number of output temporaries
        expression_meta_map::iterator it = m_temporaryResults.find( node.get_id() );
        if( ( ( it == m_temporaryResults.end() ) ? 0u : it->second.get_num_values() ) !=
            static_cast<std::size_t>( node.get_num_outputs() ) )
            THROW_MAGMA_INTERNAL_ERROR();

    } else if( node.get_num_inputs() >= 1 && node.get_num_outputs() == 1 ) {
        const temporary_meta* val = this->get_input_value( node, 0 );
        if( !val )
            throw magma_exception() << magma_exception::node_id( node.get_id() )
                                    << magma_exception::error_name( _T("Cannot disable this node") );

        this->register_temporary( node.get_id(), val->first, val->second );
    } else {
        throw magma_exception() << magma_exception::node_id( node.get_id() )
                                << magma_exception::error_name( _T("Cannot disable this node") );
    }
}

bool base_compiler::is_visited( frantic::magma::magma_interface::magma_id nodeID ) {
    return m_temporaryResults.find( nodeID ) != m_temporaryResults.end();
}

const base_compiler::temporary_meta& base_compiler::get_value( expression_id exprID, int index ) {
    if( exprID == magma_interface::INVALID_ID )
        THROW_MAGMA_INTERNAL_ERROR();

    expression_meta_map::iterator it = m_temporaryResults.find( exprID );
    if( it == m_temporaryResults.end() )
        THROW_MAGMA_INTERNAL_ERROR( exprID, index );

    if( (std::size_t)index >= it->second.get_num_values() ) {
        throw magma_exception() << magma_exception::node_id( exprID ) << magma_exception::output_index( index )
                                << magma_exception::error_name( _T( "Socket index out of bounds." ) );
    }

    return it->second.get_temporary_value( index );
}

magma_data_type base_compiler::get_input_type( const frantic::magma::magma_node_base& node, int inputIndex,
                                               bool allowUnconnected ) {
    std::pair<frantic::magma::magma_interface::magma_id, int> input = node.get_input( inputIndex );

    if( input.first == magma_interface::INVALID_ID ) {
        const variant_t& defaultValue = node.get_input_default_value( inputIndex );
        if( defaultValue.type() == typeid( boost::blank ) ) {
            if( allowUnconnected )
                return magma_data_type();
            throw magma_exception() << magma_exception::node_id( node.get_id() )
                                    << magma_exception::input_index( inputIndex )
                                    << magma_exception::error_name( _T("Unconnected input socket") );
        }

        return get_variant_type( defaultValue );
    } else {
        return this->get_value( input.first, input.second ).first;
    }
}

const base_compiler::temporary_meta* base_compiler::get_input_value( const frantic::magma::magma_node_base& node,
                                                                     int inputIndex, bool allowUnconnected ) {
    std::pair<frantic::magma::magma_interface::magma_id, int> input = node.get_input( inputIndex );

    if( input.first == magma_interface::INVALID_ID ) {
        const variant_t& defaultValue = node.get_input_default_value( inputIndex );
        if( defaultValue.type() == typeid( boost::blank ) ) {
            if( allowUnconnected )
                return NULL;
            throw magma_exception() << magma_exception::node_id( node.get_id() )
                                    << magma_exception::input_index( inputIndex )
                                    << magma_exception::error_name( _T("Unconnected input socket") );
        }

        return &this->allocate_constant( DEFAULT_VALUE_ID, defaultValue );
    } else {
        return &this->get_value( input.first, input.second );
    }
}

std::pair<base_compiler::expression_id, int> base_compiler::get_arbitrary_expression_id() {
    return std::pair<expression_id, int>( DEFAULT_VALUE_ID,
                                          (int)m_temporaryResults[DEFAULT_VALUE_ID].get_num_values() );
}

const base_compiler::temporary_meta& base_compiler::allocate_temporary( expression_id exprID,
                                                                        const frantic::magma::magma_data_type& dt ) {
    std::ptrdiff_t result = m_temporaryStorageSize;
    std::size_t allocSize = frantic::channels::sizeof_channel_data_type( dt.m_elementType ) * dt.m_elementCount;

    // We want to always be aligned on 4 bytes, so enforce that here;
    if( ( allocSize % 4 ) != 0 )
        allocSize += 4 - ( allocSize % 4 );

    m_temporaryStorageSize += allocSize;
    m_temporaryResults[exprID].add_temporary_value( dt, result );

    return m_temporaryResults[exprID].get_temporary_value( m_temporaryResults[exprID].get_num_values() - 1 );
}

std::ptrdiff_t base_compiler::allocate_temporaries( expression_id exprID,
                                                    const frantic::channels::channel_map& outMap ) {
    if( !outMap.channel_definition_complete() )
        THROW_MAGMA_INTERNAL_ERROR( exprID );

    std::ptrdiff_t result = m_temporaryStorageSize;
    std::size_t allocSize = outMap.structure_size();

    // We want to always be aligned on 4 bytes, so enforce that here;
    if( ( allocSize % 4 ) != 0 )
        allocSize += 4 - ( allocSize % 4 );

    m_temporaryStorageSize += allocSize;

    for( std::size_t i = 0, iEnd = outMap.channel_count(); i < iEnd; ++i ) {
        const frantic::channels::channel& ch = outMap[i];

        magma_data_type dt;
        dt.m_elementCount = ch.arity();
        dt.m_elementType = ch.data_type();

        m_temporaryResults[exprID].add_temporary_value( dt, result + ch.offset() );
    }

    return result;
}

std::ptrdiff_t base_compiler::allocate_temporaries( expression_id exprID, const frantic::channels::channel_map& outMap,
                                                    const std::vector<std::size_t>& shuffleIndices ) {
    if( !outMap.channel_definition_complete() )
        THROW_MAGMA_INTERNAL_ERROR( exprID );

    std::ptrdiff_t result = m_temporaryStorageSize;
    std::size_t allocSize = outMap.structure_size();

    // We want to always be aligned on 4 bytes, so enforce that here;
    if( ( allocSize % 4 ) != 0 )
        allocSize += 4 - ( allocSize % 4 );

    m_temporaryStorageSize += allocSize;

    for( std::vector<std::size_t>::const_iterator it = shuffleIndices.begin(), itEnd = shuffleIndices.end();
         it != itEnd; ++it ) {
        const frantic::channels::channel& ch = outMap[*it];

        magma_data_type dt;
        dt.m_elementCount = ch.arity();
        dt.m_elementType = ch.data_type();

        m_temporaryResults[exprID].add_temporary_value( dt, result + ch.offset() );
    }

    return result;
}

namespace {
struct make_constant_expression : public boost::static_visitor<base_compiler::expression*> {
  public:
    template <class T>
    base_compiler::expression* operator()( const T& val ) const {
        return new detail::constant_expression<T>( val );
    }

    base_compiler::expression* operator()( boost::blank ) const { THROW_MAGMA_INTERNAL_ERROR(); }
};
} // namespace

const base_compiler::temporary_meta& base_compiler::allocate_constant( expression_id exprID,
                                                                       const frantic::magma::variant_t& val ) {
    std::unique_ptr<expression> result( boost::apply_visitor( make_constant_expression(), val ) );

    magma_data_type dt;
    dt.m_elementType = result->get_output_map()[0].data_type();
    dt.m_elementCount = result->get_output_map()[0].arity();

    const temporary_meta& meta = this->allocate_temporary( exprID, dt );

    result->set_output( meta.second );

    this->register_expression( std::move( result ) );

    return meta;
}

void base_compiler::register_temporary( expression_id exprID, const frantic::magma::magma_data_type& dt,
                                        std::ptrdiff_t relPtr ) {
    m_temporaryResults[exprID].add_temporary_value( dt, relPtr );
}

void base_compiler::register_expression( std::unique_ptr<expression> expr ) {
    m_currentExpression.push_back( expr.release() );
}

void base_compiler::compile_constant( magma_interface::magma_id id, const variant_t& value ) {
    this->allocate_constant( id, value );
}

std::pair<base_compiler::expression_id, int> base_compiler::compile_constant( const variant_t& value ) {
    std::pair<expression_id, int> result = this->get_arbitrary_expression_id();

    this->allocate_constant( DEFAULT_VALUE_ID, value );

    return result;
}

void base_compiler::register_interface( magma_interface::magma_id id, nodes::magma_input_geometry_interface* ifPtr ) {
    // Just mark it "empty" and let someone else deal with finding the ptr ... Not good, but I grafted this on at the
    // end and should look for a better way in the future.

    magma_data_type dt;
    dt.m_elementCount = 0;
    dt.m_elementType = frantic::channels::data_type_invalid;
    dt.m_typeName = detail::type_to_name<nodes::magma_input_geometry_interface>::get_name();

    this->register_temporary( id, dt, reinterpret_cast<std::ptrdiff_t>( ifPtr ) );
}

void base_compiler::register_interface( magma_interface::magma_id id, nodes::magma_input_particles_interface* ifPtr ) {
    // Just mark it "empty" and let someone else deal with finding the ptr ... Not good, but I grafted this on at the
    // end and should look for a better way in the future.

    magma_data_type dt;
    dt.m_elementCount = 0;
    dt.m_elementType = frantic::channels::data_type_invalid;
    dt.m_typeName = detail::type_to_name<nodes::magma_input_particles_interface>::get_name();

    this->register_temporary( id, dt, reinterpret_cast<std::ptrdiff_t>( ifPtr ) );
}

void base_compiler::register_interface( magma_interface::magma_id id, nodes::magma_input_objects_interface* ifPtr ) {
    // Just mark it "empty" and let someone else deal with finding the ptr ... Not good, but I grafted this on at the
    // end and should look for a better way in the future.

    magma_data_type dt;
    dt.m_elementCount = 0;
    dt.m_elementType = frantic::channels::data_type_invalid;
    dt.m_typeName = detail::type_to_name<nodes::magma_input_objects_interface>::get_name();

    this->register_temporary( id, dt, reinterpret_cast<std::ptrdiff_t>( ifPtr ) );
}

frantic::magma::nodes::magma_input_geometry_interface*
base_compiler::get_geometry_interface( frantic::magma::magma_interface::magma_id id, int outputIndex ) {
    return this->get_interface<frantic::magma::nodes::magma_input_geometry_interface>( id, outputIndex );
}

frantic::magma::nodes::magma_input_particles_interface*
base_compiler::get_particles_interface( frantic::magma::magma_interface::magma_id id, int outputIndex ) {
    return this->get_interface<frantic::magma::nodes::magma_input_particles_interface>( id, outputIndex );
}

frantic::magma::nodes::magma_input_objects_interface*
base_compiler::get_objects_interface( frantic::magma::magma_interface::magma_id id, int outputIndex ) {
    return this->get_interface<frantic::magma::nodes::magma_input_objects_interface>( id, outputIndex );
}

void base_compiler::compile_expression( std::unique_ptr<expression> expr, expression_id exprID,
                                        const std::vector<std::pair<expression_id, int>>& inputs,
                                        const std::vector<magma_data_type>& expectedTypes ) {
    if( inputs.size() != expectedTypes.size() )
        THROW_MAGMA_INTERNAL_ERROR( exprID );

    std::vector<magma_data_type>::const_iterator itTypes = expectedTypes.begin();

    for( std::vector<std::pair<expression_id, int>>::const_iterator it = inputs.begin(), itEnd = inputs.end();
         it != itEnd; ++it, ++itTypes ) {
        const temporary_meta& meta = this->get_value( it->first, it->second );
        if( meta.first != *itTypes )
            throw magma_exception() << magma_exception::node_id( exprID )
                                    << magma_exception::input_index( (int)( it - inputs.begin() ) )
                                    << magma_exception::connected_id( it->first )
                                    << magma_exception::connected_output_index( it->second )
                                    << magma_exception::found_type( meta.first )
                                    << magma_exception::expected_type( *itTypes )
                                    << magma_exception::error_name( _T("Input value is incompatible type") );

        expr->set_input( static_cast<std::size_t>( it - inputs.begin() ), meta.second );
    }

    expr->set_output( this->allocate_temporaries( exprID, expr->get_output_map() ) );

    this->register_expression( std::move( expr ) );
}

// TODO Maybe make this inline and put it in base_compiler_impl.hpp for performance reasons
void base_compiler::do_eval( state& theState ) const {
    theState.m_memory =
        (char*)alloca( this->m_temporaryStorageSize ); // TODO Switch to malloc if this fails due to large sizes ...

    for( expression_list::const_iterator it = this->m_currentExpression.begin(),
                                         itEnd = this->m_currentExpression.end();
         it != itEnd; ++it )
        ( *it )->apply( theState );
}

// TODO Maybe make this inline and put it in base_compiler_impl.hpp for performance reasons
void base_compiler::do_eval_debug( state& theState, debug_data& outDebugData ) const {
    theState.m_memory =
        (char*)alloca( this->m_temporaryStorageSize ); // TODO Switch to malloc if this fails due to large sizes ...

    for( expression_list::const_iterator it = this->m_currentExpression.begin(),
                                         itEnd = this->m_currentExpression.end();
         it != itEnd; ++it )
        ( *it )->apply( theState );

    for( expression_meta_map::const_iterator it = this->m_temporaryResults.begin(),
                                             itEnd = this->m_temporaryResults.end();
         it != itEnd; ++it ) {
        for( std::size_t i = 0, iEnd = it->second.get_num_values(); i < iEnd; ++i ) {
            const temporary_meta& meta = it->second.get_temporary_value( i );

            variant_t theVal;

            if( meta.first.m_elementType == frantic::channels::data_type_float32 ) {
                if( meta.first.m_elementCount == 1 )
                    theVal = theState.get_temporary<float>( meta.second );
                else if( meta.first.m_elementCount == 3 )
                    theVal = theState.get_temporary<vec3>( meta.second );
                else if( meta.first.m_elementCount == 4 )
                    theVal = theState.get_temporary<quat>( meta.second );
            } else if( meta.first == traits<int>::get_type() ) {
                theVal = theState.get_temporary<int>( meta.second );
            } else if( meta.first == traits<bool>::get_type() ) {
                theVal = theState.get_temporary<bool>( meta.second );
            }

            outDebugData.append_value( it->first, theVal );
        }
    }
}

struct transform_point_meta {
    enum { ARITY = 2 };
    typedef frantic::magma::functors::transform_point type;
    typedef boost::mpl::vector<vec3( vec3, int )> bindings;
};

struct transform_direction_meta {
    enum { ARITY = 2 };
    typedef frantic::magma::functors::transform_direction type;
    typedef boost::mpl::vector<vec3( vec3, int )> bindings;
};

struct transform_normal_meta {
    enum { ARITY = 2 };
    typedef frantic::magma::functors::transform_normal type;
    typedef boost::mpl::vector<vec3( vec3, int )> bindings;
};

void base_compiler::compile_transforms( magma_interface::magma_id id,
                                        const std::vector<frantic::graphics::transform4f>& tm, transform_type tmType,
                                        const std::pair<frantic::magma::magma_interface::magma_id, int>& vectorInput,
                                        const std::pair<frantic::magma::magma_interface::magma_id, int>& indexInput ) {
    temporary_meta inputs[] = { this->get_value( vectorInput.first, vectorInput.second ),
                                this->get_value( indexInput.first, indexInput.second ) };
    if( tmType == transform_point )
        this->compile_impl<transform_point_meta>( id, transform_point_meta::type( tm ), inputs, 2, 1 );
    else if( tmType == transform_direction )
        this->compile_impl<transform_direction_meta>( id, transform_direction_meta::type( tm ), inputs, 2, 1 );
    else if( tmType == transform_normal )
        this->compile_impl<transform_normal_meta>( id, transform_normal_meta::type( tm ), inputs, 2, 1 );
    else
        THROW_MAGMA_INTERNAL_ERROR( id, (int)tmType );
}

void base_compiler::compile( nodes::magma_negate_node* node ) {
    this->compile_impl<nodes::magma_negate_node::meta>( *node, nodes::magma_negate_node::meta::type() );
}

void base_compiler::compile( nodes::magma_abs_node* node ) {
    this->compile_impl<nodes::magma_abs_node::meta>( *node, nodes::magma_abs_node::meta::type() );
}

void base_compiler::compile( nodes::magma_floor_node* node ) {
    this->compile_impl<nodes::magma_floor_node::meta>( *node, nodes::magma_floor_node::meta::type() );
}

void base_compiler::compile( nodes::magma_ceil_node* node ) {
    this->compile_impl<nodes::magma_ceil_node::meta>( *node, nodes::magma_ceil_node::meta::type() );
}

void base_compiler::compile( nodes::magma_sqrt_node* node ) {
    this->compile_impl<nodes::magma_sqrt_node::meta>( *node, nodes::magma_sqrt_node::meta::type() );
}

void base_compiler::compile( nodes::magma_log_node* node ) {
    this->compile_impl<nodes::magma_log_node::meta>( *node, nodes::magma_log_node::meta::type() );
}

void base_compiler::compile( nodes::magma_add_node* node ) {
    this->compile_impl<nodes::magma_add_node::meta>( *node, nodes::magma_add_node::meta::type() );
}

void base_compiler::compile( nodes::magma_sub_node* node ) {
    this->compile_impl<nodes::magma_sub_node::meta>( *node, nodes::magma_sub_node::meta::type() );
}

void base_compiler::compile( nodes::magma_mul_node* node ) {
    this->compile_impl<nodes::magma_mul_node::meta>( *node, nodes::magma_mul_node::meta::type() );
}

void base_compiler::compile( nodes::magma_div_node* node ) {
    this->compile_impl<nodes::magma_div_node::meta>( *node, nodes::magma_div_node::meta::type() );
}

void base_compiler::compile( nodes::magma_mod_node* node ) {
    this->compile_impl<nodes::magma_mod_node::meta>( *node, nodes::magma_mod_node::meta::type() );
}

void base_compiler::compile( nodes::magma_pow_node* node ) {
    this->compile_impl<nodes::magma_pow_node::meta>( *node, nodes::magma_pow_node::meta::type() );
}

void base_compiler::compile( nodes::magma_component_sum_node* node ) {
    this->compile_impl<nodes::magma_component_sum_node::meta>( *node, nodes::magma_component_sum_node::meta::type() );
}

void base_compiler::compile( nodes::magma_normalize_node* node ) {
    this->compile_impl<nodes::magma_normalize_node::meta>( *node, nodes::magma_normalize_node::meta::type() );
}

void base_compiler::compile( nodes::magma_magnitude_node* node ) {
    this->compile_impl<nodes::magma_magnitude_node::meta>( *node, nodes::magma_magnitude_node::meta::type() );
}

void base_compiler::compile( nodes::magma_dot_node* node ) {
    this->compile_impl<nodes::magma_dot_node::meta>( *node, nodes::magma_dot_node::meta::type() );
}

void base_compiler::compile( nodes::magma_cross_node* node ) {
    this->compile_impl<nodes::magma_cross_node::meta>( *node, nodes::magma_cross_node::meta::type() );
}

void base_compiler::compile( nodes::magma_matrix_mul_node* node ) {
    this->compile_impl<nodes::magma_matrix_mul_node::meta>( *node, nodes::magma_matrix_mul_node::meta::type() );
}

void base_compiler::compile( nodes::magma_cos_node* node ) {
    this->compile_impl<nodes::magma_cos_node::meta>( *node, nodes::magma_cos_node::meta::type() );
}

void base_compiler::compile( nodes::magma_acos_node* node ) {
    this->compile_impl<nodes::magma_acos_node::meta>( *node, nodes::magma_acos_node::meta::type() );
}

void base_compiler::compile( nodes::magma_sin_node* node ) {
    this->compile_impl<nodes::magma_sin_node::meta>( *node, nodes::magma_sin_node::meta::type() );
}

void base_compiler::compile( nodes::magma_asin_node* node ) {
    this->compile_impl<nodes::magma_asin_node::meta>( *node, nodes::magma_asin_node::meta::type() );
}

void base_compiler::compile( nodes::magma_tan_node* node ) {
    this->compile_impl<nodes::magma_tan_node::meta>( *node, nodes::magma_tan_node::meta::type() );
}

void base_compiler::compile( nodes::magma_atan_node* node ) {
    this->compile_impl<nodes::magma_atan_node::meta>( *node, nodes::magma_atan_node::meta::type() );
}

void base_compiler::compile( nodes::magma_atan2_node* node ) {
    this->compile_impl<nodes::magma_atan2_node::meta>( *node, nodes::magma_atan2_node::meta::type() );
}

void base_compiler::compile( nodes::magma_logical_not_node* node ) {
    this->compile_impl<nodes::magma_logical_not_node::meta>( *node, nodes::magma_logical_not_node::meta::type() );
}

void base_compiler::compile( nodes::magma_less_node* node ) {
    bool useTol = node->get_useTolerance() &&
                  *magma_singleton::get_named_data_type( _T("Float") ) == this->get_input_type( *node, 0 ) &&
                  *magma_singleton::get_named_data_type( _T("Float") ) == this->get_input_type( *node, 1 );

    if( useTol ) {
        this->compile_impl<nodes::magma_less_node::meta_tol>(
            *node, nodes::magma_less_node::meta_tol::type( std::pow( 10.f, -node->get_toleranceExp() ) ) );
    } else {
        this->compile_impl<nodes::magma_less_node::meta>( *node, nodes::magma_less_node::meta::type() );
    }
}

void base_compiler::compile( nodes::magma_lesseq_node* node ) {
    bool useTol = node->get_useTolerance() &&
                  *magma_singleton::get_named_data_type( _T("Float") ) == this->get_input_type( *node, 0 ) &&
                  *magma_singleton::get_named_data_type( _T("Float") ) == this->get_input_type( *node, 1 );

    if( useTol ) {
        this->compile_impl<nodes::magma_lesseq_node::meta_tol>(
            *node, nodes::magma_lesseq_node::meta_tol::type( std::pow( 10.f, -node->get_toleranceExp() ) ) );
    } else {
        this->compile_impl<nodes::magma_lesseq_node::meta>( *node, nodes::magma_lesseq_node::meta::type() );
    }
}

void base_compiler::compile( nodes::magma_greater_node* node ) {
    bool useTol = node->get_useTolerance() &&
                  *magma_singleton::get_named_data_type( _T("Float") ) == this->get_input_type( *node, 0 ) &&
                  *magma_singleton::get_named_data_type( _T("Float") ) == this->get_input_type( *node, 1 );

    if( useTol ) {
        this->compile_impl<nodes::magma_greater_node::meta_tol>(
            *node, nodes::magma_greater_node::meta_tol::type( std::pow( 10.f, -node->get_toleranceExp() ) ) );
    } else {
        this->compile_impl<nodes::magma_greater_node::meta>( *node, nodes::magma_greater_node::meta::type() );
    }
}

void base_compiler::compile( nodes::magma_greatereq_node* node ) {
    bool useTol = node->get_useTolerance() &&
                  *magma_singleton::get_named_data_type( _T("Float") ) == this->get_input_type( *node, 0 ) &&
                  *magma_singleton::get_named_data_type( _T("Float") ) == this->get_input_type( *node, 1 );

    if( useTol ) {
        this->compile_impl<nodes::magma_greatereq_node::meta_tol>(
            *node, nodes::magma_greatereq_node::meta_tol::type( std::pow( 10.f, -node->get_toleranceExp() ) ) );
    } else {
        this->compile_impl<nodes::magma_greatereq_node::meta>( *node, nodes::magma_greatereq_node::meta::type() );
    }
}

void base_compiler::compile( nodes::magma_equal_node* node ) {
    bool useTol = node->get_useTolerance() &&
                  frantic::channels::is_channel_data_type_float( this->get_input_type( *node, 0 ).m_elementType ) &&
                  frantic::channels::is_channel_data_type_float( this->get_input_type( *node, 1 ).m_elementType );

    if( useTol ) {
        this->compile_impl<nodes::magma_equal_node::meta_tol>(
            *node, nodes::magma_equal_node::meta_tol::type( std::pow( 10.f, -node->get_toleranceExp() ) ) );
    } else {
        this->compile_impl<nodes::magma_equal_node::meta>( *node, nodes::magma_equal_node::meta::type() );
    }
}

void base_compiler::compile( nodes::magma_notequal_node* node ) {
    bool useTol = node->get_useTolerance() &&
                  frantic::channels::is_channel_data_type_float( this->get_input_type( *node, 0 ).m_elementType ) &&
                  frantic::channels::is_channel_data_type_float( this->get_input_type( *node, 1 ).m_elementType );

    if( useTol ) {
        this->compile_impl<nodes::magma_notequal_node::meta_tol>(
            *node, nodes::magma_notequal_node::meta_tol::type( std::pow( 10.f, -node->get_toleranceExp() ) ) );
    } else {
        this->compile_impl<nodes::magma_notequal_node::meta>( *node, nodes::magma_notequal_node::meta::type() );
    }
}

void base_compiler::compile( nodes::magma_logical_and_node* node ) {
    this->compile_impl<nodes::magma_logical_and_node::meta>( *node, nodes::magma_logical_and_node::meta::type() );
}

void base_compiler::compile( nodes::magma_logical_or_node* node ) {
    this->compile_impl<nodes::magma_logical_or_node::meta>( *node, nodes::magma_logical_or_node::meta::type() );
}

void base_compiler::compile( nodes::magma_logical_xor_node* node ) {
    this->compile_impl<nodes::magma_logical_xor_node::meta>( *node, nodes::magma_logical_xor_node::meta::type() );
}

void base_compiler::compile( nodes::magma_logicswitch_node* node ) {
    this->compile_impl<nodes::magma_logicswitch_node::meta>( *node, nodes::magma_logicswitch_node::meta::type() );
}

// void base_compiler::compile( nodes::magma_mux_node* node){
//
// }

void base_compiler::compile( nodes::magma_to_float_node* node ) {
    this->compile_impl<nodes::magma_to_float_node::meta>( *node, nodes::magma_to_float_node::meta::type() );
}

void base_compiler::compile( nodes::magma_to_int_node* node ) {
    this->compile_impl<nodes::magma_to_int_node::meta>( *node, nodes::magma_to_int_node::meta::type() );
}

void base_compiler::compile( nodes::magma_breakout_node* node ) {
    const temporary_meta* input = this->get_input_value( *node, 0 );

    if( input->first != traits<vec3>::get_type() )
        throw magma_exception() << magma_exception::node_id( node->get_id() ) << magma_exception::input_index( 0 )
                                << magma_exception::connected_id( node->get_input( 0 ).first )
                                << magma_exception::connected_output_index( node->get_input( 0 ).second )
                                << magma_exception::found_type( input->first )
                                << magma_exception::expected_type( traits<vec3>::get_type() )
                                << magma_exception::error_name( _T("Breakout requires a Vec3 input") );

    magma_data_type dt = traits<float>::get_type();

    std::size_t valSize = frantic::channels::sizeof_channel_data_type( dt.m_elementType );

    for( std::size_t i = 0; i < input->first.m_elementCount; ++i )
        this->register_temporary( node->get_id(), dt, input->second + static_cast<std::ptrdiff_t>( i * valSize ) );
}

void base_compiler::compile( nodes::magma_to_vector_node* node ) {
    this->compile_impl<nodes::magma_to_vector_node::meta>( *node, nodes::magma_to_vector_node::meta::type() );
}

void base_compiler::compile( nodes::magma_quat_to_vectors_node* node ) {
    this->compile_impl<nodes::magma_quat_to_vectors_node::meta>( *node,
                                                                 nodes::magma_quat_to_vectors_node::meta::type() );
}

void base_compiler::compile( nodes::magma_vectors_to_quat_node* node ) {
    this->compile_impl<nodes::magma_vectors_to_quat_node::meta>( *node,
                                                                 nodes::magma_vectors_to_quat_node::meta::type() );
}

void base_compiler::compile( nodes::magma_eulerangles_to_quat_node* node ) {
    this->compile_impl<nodes::magma_eulerangles_to_quat_node::meta>(
        *node, nodes::magma_eulerangles_to_quat_node::meta::type() );
}

void base_compiler::compile( nodes::magma_angleaxis_to_quat_node* node ) {
    this->compile_impl<nodes::magma_angleaxis_to_quat_node::meta>( *node,
                                                                   nodes::magma_angleaxis_to_quat_node::meta::type() );
}

void base_compiler::compile( nodes::magma_noise_node* node ) {
    this->compile_impl<nodes::magma_noise_node::meta>(
        *node, nodes::magma_noise_node::meta::type( 1.f, node->get_numOctaves(), node->get_lacunarity(),
                                                    node->get_normalize() ) );
}

void base_compiler::compile( nodes::magma_vecnoise_node* node ) {
    this->compile_impl<nodes::magma_vecnoise_node::meta>(
        *node, nodes::magma_vecnoise_node::meta::type( 1.f, node->get_numOctaves(), node->get_lacunarity(),
                                                       node->get_normalize() ) );
}

void base_compiler::compile( nodes::magma_random_node* node ) {
    this->compile_impl<nodes::magma_random_node::meta>( *node, nodes::magma_random_node::meta::type() );
}

void base_compiler::compile( nodes::magma_vecrandom_node* node ) {
    this->compile_impl<nodes::magma_vecrandom_node::meta>( *node, nodes::magma_vecrandom_node::meta::type() );
}

void base_compiler::compile( nodes::magma_exponential_random_node* node ) {
    this->compile_impl<nodes::magma_exponential_random_node::meta>(
        *node, nodes::magma_exponential_random_node::meta::type() );
}

void base_compiler::compile( nodes::magma_weibull_random_node* node ) {
    this->compile_impl<nodes::magma_weibull_random_node::meta>( *node, nodes::magma_weibull_random_node::meta::type() );
}

void base_compiler::compile( nodes::magma_gaussian_random_node* node ) {
    this->compile_impl<nodes::magma_gaussian_random_node::meta>( *node,
                                                                 nodes::magma_gaussian_random_node::meta::type() );
}

void base_compiler::compile( nodes::magma_triangle_random_node* node ) {
    this->compile_impl<nodes::magma_triangle_random_node::meta>( *node,
                                                                 nodes::magma_triangle_random_node::meta::type() );
}

void base_compiler::compile( nodes::magma_uniform_on_sphere_random_node* node ) {
    this->compile_impl<nodes::magma_uniform_on_sphere_random_node::meta>(
        *node, nodes::magma_uniform_on_sphere_random_node::meta::type() );
}

void base_compiler::compile( nodes::magma_blend_node* node ) {
    this->compile_impl<nodes::magma_blend_node::meta>( *node, nodes::magma_blend_node::meta::type() );
}

void base_compiler::compile( nodes::magma_clamp_node* node ) {
    this->compile_impl<nodes::magma_clamp_node::meta>( *node, nodes::magma_clamp_node::meta::type() );
}

void base_compiler::compile( nodes::magma_to_world_node* node ) {
    frantic::graphics::transform4f tm = this->get_context_data().get_world_transform();

    if( node->get_inputType() == _T("Point") )
        this->compile_impl<nodes::magma_to_world_node::point_meta>(
            *node, nodes::magma_to_world_node::point_meta::type( tm ) );
    else if( node->get_inputType() == _T("Vector") )
        this->compile_impl<nodes::magma_to_world_node::direction_meta>(
            *node, nodes::magma_to_world_node::direction_meta::type( tm ) );
    else if( node->get_inputType() == _T("Normal") )
        this->compile_impl<nodes::magma_to_world_node::normal_meta>(
            *node, nodes::magma_to_world_node::normal_meta::type( tm ) );
    else
        throw magma_exception() << magma_exception::property_name( _T("inputType") )
                                << magma_exception::error_name( _T("Invalid transform type: \"") +
                                                                node->get_inputType() + _T("\"") );
}

void base_compiler::compile( nodes::magma_from_world_node* node ) {
    frantic::graphics::transform4f tm = this->get_context_data().get_world_transform( true );

    if( node->get_inputType() == _T("Point") )
        this->compile_impl<nodes::magma_from_world_node::point_meta>(
            *node, nodes::magma_from_world_node::point_meta::type( tm ) );
    else if( node->get_inputType() == _T("Vector") )
        this->compile_impl<nodes::magma_from_world_node::direction_meta>(
            *node, nodes::magma_from_world_node::direction_meta::type( tm ) );
    else if( node->get_inputType() == _T("Normal") )
        this->compile_impl<nodes::magma_from_world_node::normal_meta>(
            *node, nodes::magma_from_world_node::normal_meta::type( tm ) );
    else
        throw magma_exception() << magma_exception::property_name( _T("inputType") )
                                << magma_exception::error_name( _T("Invalid transform type: \"") +
                                                                node->get_inputType() + _T("\"") );
}

void base_compiler::compile( nodes::magma_to_camera_node* node ) {
    frantic::graphics::transform4f tm = this->get_context_data().get_camera_transform();

    if( node->get_inputType() == _T("Point") )
        this->compile_impl<nodes::magma_to_camera_node::point_meta>(
            *node, nodes::magma_to_camera_node::point_meta::type( tm ) );
    else if( node->get_inputType() == _T("Vector") )
        this->compile_impl<nodes::magma_to_camera_node::direction_meta>(
            *node, nodes::magma_to_camera_node::direction_meta::type( tm ) );
    else if( node->get_inputType() == _T("Normal") )
        this->compile_impl<nodes::magma_to_camera_node::normal_meta>(
            *node, nodes::magma_to_camera_node::normal_meta::type( tm ) );
    else
        throw magma_exception() << magma_exception::property_name( _T("inputType") )
                                << magma_exception::error_name( _T("Invalid transform type: \"") +
                                                                node->get_inputType() + _T("\"") );
}

void base_compiler::compile( nodes::magma_from_camera_node* node ) {
    frantic::graphics::transform4f tm = this->get_context_data().get_camera_transform( true );

    if( node->get_inputType() == _T("Point") )
        this->compile_impl<nodes::magma_from_camera_node::point_meta>(
            *node, nodes::magma_from_camera_node::point_meta::type( tm ) );
    else if( node->get_inputType() == _T("Vector") )
        this->compile_impl<nodes::magma_from_camera_node::direction_meta>(
            *node, nodes::magma_from_camera_node::direction_meta::type( tm ) );
    else if( node->get_inputType() == _T("Normal") )
        this->compile_impl<nodes::magma_from_camera_node::normal_meta>(
            *node, nodes::magma_from_camera_node::normal_meta::type( tm ) );
    else
        throw magma_exception() << magma_exception::property_name( _T("inputType") )
                                << magma_exception::error_name( _T("Invalid transform type: \"") +
                                                                node->get_inputType() + _T("\"") );
}

void base_compiler::compile( nodes::magma_quat_mul_node* node ) {
    this->compile_impl<nodes::magma_quat_mul_node::meta>( *node, nodes::magma_quat_mul_node::meta::type() );
}

void base_compiler::compile( nodes::magma_nearest_point_node* node ) {
    using frantic::magma::nodes::magma_input_geometry_interface;

    magma_input_geometry_interface* geom = this->get_input_interface<magma_input_geometry_interface>( *node, 0 );

    temporary_meta inputs[] = { *this->get_input_value( *node, 1 ), *this->get_input_value( *node, 2, true ) };

    nodes::magma_nearest_point_node::meta::type fn;

    fn.set_geometry( *geom );

    this->compile_impl<nodes::magma_nearest_point_node::meta>( node->get_id(), fn, inputs, 2, 7 );
}

void base_compiler::compile( nodes::magma_intersection_node* node ) {
    using frantic::magma::nodes::magma_input_geometry_interface;

    magma_input_geometry_interface* geom = this->get_input_interface<magma_input_geometry_interface>( *node, 0 );

    temporary_meta inputs[] = { *this->get_input_value( *node, 1 ), *this->get_input_value( *node, 2 ),
                                *this->get_input_value( *node, 3, true ) };

    nodes::magma_intersection_node::meta::type fn;

    fn.set_geometry( *geom );

    this->compile_impl<nodes::magma_intersection_node::meta>( node->get_id(), fn, inputs, 3, 7 );
}

void base_compiler::compile( nodes::magma_in_volume_node* node ) {
    using frantic::magma::nodes::magma_input_geometry_interface;

    magma_input_geometry_interface* geom = this->get_input_interface<magma_input_geometry_interface>( *node, 0 );

    temporary_meta inputs[] = { *this->get_input_value( *node, 1 ) };

    nodes::magma_in_volume_node::meta::type fn;

    fn.set_geometry( *geom );

    this->compile_impl<nodes::magma_in_volume_node::meta>( node->get_id(), fn, inputs, 1, 1 );
}

void base_compiler::compile( nodes::magma_nearest_particle_node* node ) {
    using frantic::magma::nodes::magma_input_particles_interface;

    magma_input_particles_interface* parts = this->get_input_interface<magma_input_particles_interface>( *node, 0 );

    temporary_meta inputs[] = { *this->get_input_value( *node, 1 ) };

    if( node->get_whichNearest() <= 1 ) {
        nodes::magma_nearest_particle_node::meta::type fn;

        fn.set_particles( *parts );

        this->compile_impl<nodes::magma_nearest_particle_node::meta>( node->get_id(), fn, inputs, 1, 3 );
    } else {
        nodes::magma_nearest_particle_node::nth_meta::type fn( node->get_whichNearest() );

        fn.set_particles( *parts );

        this->compile_impl<nodes::magma_nearest_particle_node::nth_meta>( node->get_id(), fn, inputs, 1, 3 );
    }
}

void base_compiler::compile( nodes::magma_nearest_particles_avg_node* node ) {
    using frantic::magma::nodes::magma_input_particles_interface;

    magma_input_particles_interface* parts = this->get_input_interface<magma_input_particles_interface>( *node, 0 );

    temporary_meta inputs[] = { *this->get_input_value( *node, 1 ), *this->get_input_value( *node, 2 ),
                                *this->get_input_value( *node, 3 ) };

    // We could optimize the case when input[3] (ie. falloff power) is 0 which corresponds to a constant even weighting
    // for all particles.

    nodes::magma_nearest_particles_avg_node::meta::type fn;

    fn.set_particles( *parts );

    for( std::vector<frantic::tstring>::const_iterator it = node->get_channels().begin(),
                                                       itEnd = node->get_channels().end();
         it != itEnd; ++it )
        fn.add_channel( *it );

    this->compile_impl<nodes::magma_nearest_particles_avg_node::meta>( node->get_id(), fn, inputs, 3,
                                                                       2 + node->get_channels().size() );
}

void base_compiler::compile( nodes::magma_particle_kernel_node* node ) {
    using frantic::magma::nodes::magma_input_particles_interface;

    magma_input_particles_interface* parts = this->get_input_interface<magma_input_particles_interface>( *node, 0 );

    temporary_meta inputs[] = { *this->get_input_value( *node, 1 ), *this->get_input_value( *node, 2 ),
                                *this->get_input_value( *node, 3 ) };

    // We could optimize the case when input[3] (ie. falloff power) is 0 which corresponds to a constant even weighting
    // for all particles.

    nodes::magma_particle_kernel_node::meta::type fn;

    fn.set_particles( *parts );

    for( std::vector<frantic::tstring>::const_iterator it = node->get_channels().begin(),
                                                       itEnd = node->get_channels().end();
         it != itEnd; ++it )
        fn.add_channel( *it );

    this->compile_impl<nodes::magma_particle_kernel_node::meta>( node->get_id(), fn, inputs, 3,
                                                                 2 + node->get_channels().size() );
}

void base_compiler::compile( nodes::magma_face_query_node* node ) {
    using frantic::magma::nodes::magma_input_geometry_interface;

    magma_input_geometry_interface* geom = this->get_input_interface<magma_input_geometry_interface>( *node, 0 );

    temporary_meta inputs[] = { *this->get_input_value( *node, 1 ), *this->get_input_value( *node, 2 ),
                                *this->get_input_value( *node, 3 ) };

    nodes::magma_face_query_node::meta::type fn;

    try {
        fn.set_geometry( *geom );

        if( node->get_exposePosition() )
            fn.add_channel( _T("Position") );

        for( std::vector<frantic::tstring>::const_iterator it = node->get_channels().begin(),
                                                           itEnd = node->get_channels().end();
             it != itEnd; ++it )
            fn.add_channel( *it );
    } catch( frantic::magma::magma_exception& e ) {
        e << frantic::magma::magma_exception::node_id( node->get_id() );
        throw;
    }

    this->compile_impl<nodes::magma_face_query_node::meta>(
        node->get_id(), fn, inputs, 3, ( node->get_exposePosition() ? 1 : 0 ) + node->get_channels().size() );
}

void base_compiler::compile( nodes::magma_vertex_query_node* node ) {
    using frantic::magma::nodes::magma_input_geometry_interface;

    magma_input_geometry_interface* geom = this->get_input_interface<magma_input_geometry_interface>( *node, 0 );

    temporary_meta inputs[] = { *this->get_input_value( *node, 1 ), *this->get_input_value( *node, 2 ) };

    nodes::magma_vertex_query_node::meta::type fn;

    try {
        fn.set_geometry( *geom );

        if( node->get_exposePosition() )
            fn.add_channel( _T("Position") );

        for( std::vector<frantic::tstring>::const_iterator it = node->get_channels().begin(),
                                                           itEnd = node->get_channels().end();
             it != itEnd; ++it )
            fn.add_channel( *it );
    } catch( frantic::magma::magma_exception& e ) {
        e << frantic::magma::magma_exception::node_id( node->get_id() );
        throw;
    }

    this->compile_impl<nodes::magma_vertex_query_node::meta>(
        node->get_id(), fn, inputs, 2, ( node->get_exposePosition() ? 1 : 0 ) + node->get_channels().size() );
}

void base_compiler::compile( nodes::magma_element_query_node* node ) {
    using frantic::magma::nodes::magma_input_geometry_interface;

    magma_input_geometry_interface* geom = this->get_input_interface<magma_input_geometry_interface>( *node, 0 );

    temporary_meta inputs[] = { *this->get_input_value( *node, 1 ), *this->get_input_value( *node, 2 ) };

    nodes::magma_element_query_node::meta::type fn;

    try {
        fn.set_geometry( *geom );

        fn.add_channel( _T("FaceElementArea") );
        fn.add_channel( _T("FaceElementVolume") );
        fn.add_channel( _T("FaceElementCentroid") );
        // for( std::vector<frantic::tstring>::const_iterator it = node->get().begin(), itEnd =
        // node->get_channels().end(); it != itEnd; ++it ) 	fn.add_channel( *it );
    } catch( frantic::magma::magma_exception& e ) {
        e << frantic::magma::magma_exception::node_id( node->get_id() );
        throw;
    }

    this->compile_impl<nodes::magma_element_query_node::meta>( node->get_id(), fn, inputs, 2, 3 );
}

void base_compiler::compile( nodes::magma_mesh_query_node* node ) {
    using frantic::magma::nodes::magma_input_geometry_interface;

    magma_input_geometry_interface* geom = this->get_input_interface<magma_input_geometry_interface>( *node, 0 );

    temporary_meta inputs[] = { *this->get_input_value( *node, 1 ) };

    nodes::magma_mesh_query_node::meta::type fn;

    fn.set_geometry( *geom );

    this->compile_impl<nodes::magma_mesh_query_node::meta>( node->get_id(), fn, inputs, 1, 3 );
}

void base_compiler::compile( nodes::magma_particle_query_node* node ) {
    using frantic::magma::nodes::magma_input_particles_interface;

    magma_input_particles_interface* prts = this->get_input_interface<magma_input_particles_interface>( *node, 0 );

    temporary_meta inputs[] = { *this->get_input_value( *node, 1 ) };

    nodes::magma_particle_query_node::meta::type fn;

    fn.set_particles( *prts );

    for( std::vector<frantic::tstring>::const_iterator it = node->get_channels().begin(),
                                                       itEnd = node->get_channels().end();
         it != itEnd; ++it )
        fn.add_channel( *it );

    this->compile_impl<nodes::magma_particle_query_node::meta>( node->get_id(), fn, inputs, 1,
                                                                node->get_channels().size() );
}

void base_compiler::compile( nodes::magma_object_query_node* node ) {
    using frantic::magma::nodes::magma_input_objects_interface;

    magma_input_objects_interface* obj = this->get_input_interface<magma_input_objects_interface>( *node, 0 );

    temporary_meta inputs[] = { *this->get_input_value( *node, 1 ) };

    nodes::magma_object_query_node::meta::type fn( obj, node->get_properties() );

    this->compile_impl<nodes::magma_object_query_node::meta>( node->get_id(), fn, inputs, 1,
                                                              node->get_properties().size() );

    /*int testing = 1;
    int* objectIndex = &testing;//reinterpret_cast<int*>( inputTempMeta.second ); // TODO Get this to work with the
    given index.

    for( std::vector<frantic::tstring>::const_iterator it = node->get_properties().begin(), itEnd =
    node->get_properties().end(); it != itEnd; ++it ){ frantic::magma::variant_t val; val.type; obj->get_property( 0,
    *it, val ); if( val.type() == typeid( boost::blank ) ) throw magma_exception() << magma_exception::node_id(
    node->get_id() ) << magma_exception::connected_id( node->get_input(0).first ) << magma_exception::property_name(
    _T("properties") ) << magma_exception::error_name( _T("The InputObject does not have a property named: \"") + *it +
    _T("\"") );

            this->compile_constant( node->get_id(), val );
    }*/
}

void base_compiler::compile( nodes::magma_elbow_node* node ) {
    const temporary_meta& inputMeta = *this->get_input_value( *node, 0 );

    this->register_temporary( node->get_id(), inputMeta.first, inputMeta.second );
}

namespace {
// Adapts a field_interface to the subexpression interface.
class field_expression : public base_compiler::expression {
    std::ptrdiff_t m_inPosOffset, m_outputOffset;
    boost::shared_ptr<frantic::volumetrics::field_interface> m_adaptedField;

    static void internal_apply( const base_compiler::expression* _this, base_compiler::state& data );

  public:
    field_expression()
        : m_inPosOffset( -1 )
        , m_outputOffset( -1 ) {}

    virtual ~field_expression() {}

    void set_field( boost::shared_ptr<frantic::volumetrics::field_interface> field ) { m_adaptedField = field; }

    virtual void set_input( std::size_t inputIndex, std::ptrdiff_t relPtr ) {
        if( inputIndex == 0 )
            m_inPosOffset = relPtr;
    }

    virtual void set_output( std::ptrdiff_t relPtr ) { m_outputOffset = relPtr; }

    virtual const frantic::channels::channel_map& get_output_map() const { return m_adaptedField->get_channel_map(); }

    virtual void apply( base_compiler::state& data ) const {
        m_adaptedField->evaluate_field( data.get_output_pointer( m_outputOffset ),
                                        data.get_temporary<vec3>( m_inPosOffset ) );
    }
};

void field_expression::internal_apply( const base_compiler::expression* _this, base_compiler::state& data ) {
    return static_cast<const field_expression*>( _this )->field_expression::apply( data );
}
} // namespace

void base_compiler::compile_field( magma_interface::magma_id id,
                                   const boost::shared_ptr<frantic::volumetrics::field_interface>& field,
                                   const std::pair<magma_interface::magma_id, int>& posInput,
                                   const std::vector<frantic::tstring>& reorderedChannels ) {
    std::unique_ptr<field_expression> result( new field_expression );

    result->set_field( field );

    const std::pair<frantic::magma::magma_data_type, std::ptrdiff_t>& inputMeta =
        this->get_value( posInput.first, posInput.second );
    if( inputMeta.first != *frantic::magma::magma_singleton::get_named_data_type( _T("Vec3") ) )
        throw magma_exception() << magma_exception::node_id( id ) << magma_exception::input_index( 1 )
                                << magma_exception::connected_id( posInput.first )
                                << magma_exception::connected_output_index( posInput.second )
                                << magma_exception::expected_type(
                                       *frantic::magma::magma_singleton::get_named_data_type( _T("Vec3") ) )
                                << magma_exception::found_type( inputMeta.first )
                                << magma_exception::error_name( _T("Position input must be a vector") );

    result->set_input( 0, inputMeta.second );

    // Generate a permutation of the channel map's outputs so they are re-ordered. This works around a limitation of
    // channel_map where the channel order must be in increasing offset, while we want to expose some channels in a
    // different order than their memory layout dictates.
    if( !reorderedChannels.empty() ) {
        std::vector<std::size_t> shuffleIndices;

        shuffleIndices.reserve( reorderedChannels.size() );

        for( std::vector<frantic::tstring>::const_iterator it = reorderedChannels.begin(),
                                                           itEnd = reorderedChannels.end();
             it != itEnd; ++it ) {
            std::size_t i, iEnd;
            for( i = 0, iEnd = field->get_channel_map().channel_count();
                 i < iEnd && field->get_channel_map()[i].name() != *it; ++i )
                ; // Do nothing.

            if( i < iEnd ) {
                shuffleIndices.push_back(
                    i ); // We found the channel we want to go next, so add it to the shuffle vector.
            } else {
                throw magma_exception() << magma_exception::node_id( id ) << magma_exception::output_index( (int)i )
                                        << magma_exception::error_name( _T("Output channel \"") + *it +
                                                                        _T ("\" is not present") );
            }
        }

        result->set_output( this->allocate_temporaries( id, result->get_output_map(), shuffleIndices ) );
    } else {
        result->set_output( this->allocate_temporaries( id, result->get_output_map() ) );
    }

    this->register_expression( static_cast<std::unique_ptr<expression>>( std::move( result ) ) );
}

void base_compiler::compile( nodes::magma_blop_input_node* node ) {
    for( int i = 0, iEnd = node->get_num_outputs(); i < iEnd; ++i ) {
        std::pair<frantic::magma::magma_interface::magma_id, int> input = node->get_output( i );
        if( input.first == magma_interface::INVALID_ID )
            throw magma_exception() << magma_exception::node_id( node->get_id() ) << magma_exception::output_index( i )
                                    << magma_exception::error_name( _T("Blop Input was not connected") );

        if( !this->is_visited( input.first ) ) {
            // This happens when a node inside a blop is making a function call to outside the blop.
            // The *better* alternative is to have a recursive compilation evaluate the container node initially. This
            // might evaluate redundant data though ...
            //  HACK The visitation algorithm should solve this.

            magma_node_base* node = m_magma->get_node( input.first );
            if( !node )
                THROW_MAGMA_INTERNAL_ERROR( node->get_id(), i, input.first, input.second );

            this->visit( *node );
        }

        const temporary_meta& inMeta = this->get_value( input.first, input.second );

        this->register_temporary( node->get_id(), inMeta.first, inMeta.second );
    }
}

void base_compiler::compile( nodes::magma_blop_node* node ) {
    magma_node_base* implNode = m_magma->get_node( node->get__internal_output_id() );
    if( !implNode )
        THROW_MAGMA_INTERNAL_ERROR( node->get_id() );

    // Just copy the results metadata from the inputs to the internal output node, since we don't do any additional
    // processing.
    for( int i = 0, iEnd = implNode->get_num_inputs(); i < iEnd; ++i ) {
        const temporary_meta& inputMeta = *this->get_input_value( *implNode, i );

        this->register_temporary( node->get_id(), inputMeta.first, inputMeta.second );
    }
}

void base_compiler::compile( nodes::magma_blop_output_node* node ) {
    // Do nothing. This node only exists to simplify the heirarchy traversal algorithms.
}

namespace {
class loop_state : public base_compiler::state {
    int m_loopIndex;
    bool m_terminated;

  public:
    loop_state( base_compiler::state& parentState )
        : base_compiler::state( parentState ) {
        m_loopIndex = 0;
        m_terminated = false;
    }

    bool get_terminated() const { return m_terminated; }
    int get_current_index() const { return m_loopIndex; }

    void terminate() { m_terminated = true; }
    void advance() { ++m_loopIndex; }
};

class loop_expression : public base_compiler::expression {
    std::vector<expression*> m_condExpr, m_bodyExpr;
    int m_maxIterations;

    frantic::channels::channel_map m_outMap;
    std::vector<std::ptrdiff_t> m_inputPtrs;
    std::ptrdiff_t m_outPtr;

  public:
    loop_expression( const frantic::channels::channel_map& outMap, int maxIterations ) {
        m_outMap = outMap;
        m_maxIterations = maxIterations;

        m_inputPtrs.resize( outMap.channel_count(), -1 );
        m_outPtr = -1;
    }

    template <class ForwardIterator>
    void set_condition_expression( ForwardIterator begin, ForwardIterator end ) {
        m_condExpr.assign( begin, end );
    }

    template <class ForwardIterator>
    void set_body_expression( ForwardIterator begin, ForwardIterator end ) {
        m_bodyExpr.assign( begin, end );
    }

    virtual void set_input( std::size_t inputIndex, std::ptrdiff_t relPtr ) { m_inputPtrs[inputIndex] = relPtr; }
    virtual void set_output( std::ptrdiff_t relPtr ) { m_outPtr = relPtr; }
    virtual const frantic::channels::channel_map& get_output_map() const { return m_outMap; }

    virtual void apply( base_compiler::state& data ) const {
        // Copy the initial values to the output location.
        for( std::size_t i = 0, iEnd = m_outMap.channel_count(); i < iEnd; ++i ) {
            void* dest = m_outMap[i].get_channel_data_pointer( data.get_output_pointer( m_outPtr ) );
            void* src = data.get_output_pointer( m_inputPtrs[i] );
            memcpy( dest, src, m_outMap[i].primitive_size() );
        }

        loop_state ls( data );

    loopStart:
        for( std::vector<expression*>::const_iterator it = m_condExpr.begin(), itEnd = m_condExpr.end(); it != itEnd;
             ++it )
            ( *it )->apply( ls );

        if( !ls.get_terminated() ) {
            for( std::vector<expression*>::const_iterator it = m_bodyExpr.begin(), itEnd = m_bodyExpr.end();
                 it != itEnd; ++it )
                ( *it )->apply( ls );

            ls.advance();

            // Only proceed with the loop if we haven't hit the maximum iteration count. This is to prevent accidental
            // infinite loops.
            if( ls.get_current_index() < m_maxIterations )
                goto loopStart;
        }
    }

    virtual runtime_ptr get_runtime_ptr() const { return NULL; }
};

class loop_index_expression : public base_compiler::expression {
    std::ptrdiff_t m_outPtr;

  public:
    virtual void set_input( std::size_t /*inputIndex*/, std::ptrdiff_t /*relPtr*/ ) {}
    virtual void set_output( std::ptrdiff_t relPtr ) { m_outPtr = relPtr; }
    virtual const frantic::channels::channel_map& get_output_map() const { return traits<int>::get_static_map(); }

    virtual void apply( base_compiler::state& data ) const {
        data.set_temporary( m_outPtr, static_cast<loop_state&>( data ).get_current_index() );
    }

    virtual runtime_ptr get_runtime_ptr() const { return NULL; }
};

frantic::channels::channel_map g_emptyMap;

class loop_condition_expression : public base_compiler::expression {
    std::ptrdiff_t m_inPtr;

  public:
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
        if( !data.get_temporary<bool>( m_inPtr ) )
            static_cast<loop_state&>( data ).terminate();
    }

    virtual runtime_ptr get_runtime_ptr() const { return NULL; }
};

class loop_update_expression : public base_compiler::expression {
    frantic::channels::channel_map m_outMap;
    std::vector<std::ptrdiff_t> m_inputPtrs;
    std::ptrdiff_t m_outPtr;

  public:
    loop_update_expression( const frantic::channels::channel_map& outMap ) {
        m_outMap = outMap;
        m_inputPtrs.resize( outMap.channel_count(), -1 );
        m_outPtr = -1;
    }

    virtual void set_input( std::size_t inputIndex, std::ptrdiff_t relPtr ) { m_inputPtrs[inputIndex] = relPtr; }
    virtual void set_output( std::ptrdiff_t relPtr ) { m_outPtr = relPtr; }

    // This is a little weird, but this node doesn't allocate its own memory, it just updates memory owned by the parent
    // loop node.
    virtual const frantic::channels::channel_map& get_output_map() const {
        if( !g_emptyMap.channel_definition_complete() )
            g_emptyMap.end_channel_definition();
        return g_emptyMap;
    }

    virtual void apply( base_compiler::state& data ) const {
        for( std::size_t i = 0, iEnd = m_outMap.channel_count(); i < iEnd; ++i ) {
            void* dest = m_outMap[i].get_channel_data_pointer( data.get_output_pointer( m_outPtr ) );
            void* src = data.get_output_pointer( m_inputPtrs[i] );
            memcpy( dest, src, m_outMap[i].primitive_size() );
        }
    }

    virtual runtime_ptr get_runtime_ptr() const { return NULL; }
};
} // namespace

void base_compiler::compile( nodes::magma_loop_node* node ) {
    // Our expression must:
    //  1. Copy the incoming updateable values to the output location
    //  2. Evaluate the condition output of the inner expression, if false we are done
    //  3. If true, then evaluate the loop expression updating the input/output values
    //  4. goto 2

    magma_interface::magma_id loopInputID = node->get_contained_source()->get_id(); // node->get__internal_input_id();
    magma_interface::magma_id loopOutputID = node->get_contained_sink()->get_id();  // node->get__internal_output_id();

    magma_node_base* loopOutputNode = m_magma->get_node( loopOutputID );
    if( !loopOutputNode )
        THROW_MAGMA_INTERNAL_ERROR( node->get_id() );

    // We need to store a list of the inputs to the loop container node. Only some of them are actually of interest
    // as true inputs to the loop container (at runtime).
    std::vector<temporary_meta> originalInputs;
    for( int i = 0, iEnd = node->get_num_inputs(); i < iEnd; ++i )
        originalInputs.push_back( *this->get_input_value( *node, i ) );

    const std::vector<int>& outputMask = node->get_outputMask();

    // Calculate the output layout from the loop using the list of channel indices in 'outputMask'.
    frantic::channels::channel_map outputLayout;

    for( std::vector<int>::const_iterator it = outputMask.begin(), itEnd = outputMask.end(); it != itEnd; ++it ) {
        if( *it < 0 || *it >= node->get_num_inputs() )
            throw magma_exception() << magma_exception::node_id( node->get_id() )
                                    << magma_exception::property_name( _T("outputMask") )
                                    << magma_exception::error_name(
                                           _T("Invalid index \"") + boost::lexical_cast<frantic::tstring>( *it ) +
                                           _T("in outputMask[") +
                                           boost::lexical_cast<frantic::tstring>( it - outputMask.begin() ) + _T("]") );

        const temporary_meta& meta = originalInputs[*it];

        if( meta.first.m_elementType == frantic::channels::data_type_invalid )
            throw magma_exception()
                << magma_exception::node_id( node->get_id() ) << magma_exception::input_index( *it )
                << magma_exception::output_index( static_cast<int>( it - outputMask.begin() ) )
                << magma_exception::error_name(
                       _T("Non-value inputs (ex. InputGeometry) cannot be in the loop output list") );

        frantic::tstring outDescription;
        node->get_output_description( static_cast<int>( it - outputMask.begin() ), outDescription );

        // Replace any invalid characters with underscores
        for( frantic::tstring::iterator itStr = outDescription.begin(), itStrEnd = outDescription.end();
             itStr != itStrEnd; ++itStr ) {
            if( !std::isalnum( *itStr, std::locale::classic() ) )
                *itStr = _T( '_' );
        }

        outputLayout.define_channel( outDescription, originalInputs[*it].first.m_elementCount,
                                     originalInputs[*it].first.m_elementType );
    }

    outputLayout.end_channel_definition( 4u, true );

    // Allocate storage for the output parameters
    std::ptrdiff_t outPtr = this->allocate_temporaries( node->get_id(), outputLayout );

    // Allocate the storage & expression for exposing the loop iteration index
    std::ptrdiff_t iterPtr = this->allocate_temporary( loopInputID, traits<int>::get_type() ).second;

    // Expose the input values from the internal input socket. Input only params are forwarded from their source but
    // output parameters are redirected to the output location of the loop container node.
    int i = 0;
    for( std::vector<temporary_meta>::const_iterator it = originalInputs.begin(), itEnd = originalInputs.end();
         it != itEnd; ++it, ++i ) {
        std::vector<int>::const_iterator outMaskIt = std::find( outputMask.begin(), outputMask.end(), i );

        if( outMaskIt != outputMask.end() ) {
            const temporary_meta& meta =
                this->get_value( node->get_id(), static_cast<int>( outMaskIt - outputMask.begin() ) );

            this->register_temporary( loopInputID, meta.first, meta.second );
        } else {
            const temporary_meta& meta = *this->get_input_value( *node, i );

            this->register_temporary( loopInputID, meta.first, meta.second );
        }
    }

    // Figure out where the conditional part of the expression begins, so we can steal those and put them inside the
    // loop.
    std::size_t expressionStart = m_currentExpression.size();

    // Create the loop container's expression, and assign all the inputs that correspond to output parameters so
    // they can be initially copied into the output location.
    std::unique_ptr<loop_expression> loopExpr( new loop_expression( outputLayout, node->get_maxIterations() ) );

    i = 0;
    for( std::vector<int>::const_iterator it = outputMask.begin(), itEnd = outputMask.end(); it != itEnd; ++it, ++i )
        loopExpr->set_input( i, originalInputs[*it].second );
    loopExpr->set_output( outPtr );

    // Create the expression that exposes the iteration index. This isn't always used, but the current implementation of
    // is_visited() prevents it from actually getting visited if done the lazy way.
    std::unique_ptr<expression> indexExpr( new loop_index_expression );

    // We expect the parent magma_loop_node to have allocated the storage for this node already.
    indexExpr->set_output( this->get_value( loopInputID, 0 ).second );

    this->register_expression( std::move( indexExpr ) );

    // Build the subtree that goes into the condition output
    std::pair<magma_interface::magma_id, int> condInput = loopOutputNode->get_input( 0 );
    if( condInput.first != magma_interface::INVALID_ID )
        this->build_subtree( condInput.first );

    const temporary_meta& condMeta = *this->get_input_value( *loopOutputNode, 0 );
    if( condMeta.first != traits<bool>::get_type() )
        throw magma_exception() << magma_exception::node_id( loopOutputNode->get_id() )
                                << magma_exception::input_index( 0 ) << magma_exception::found_type( condMeta.first )
                                << magma_exception::expected_type( traits<bool>::get_type() )
                                << magma_exception::error_name( _T("Invalid type") );

    // Create the expression that evaluates the condition boolean and adjusts the loop state to communicate the possible
    // loop termination.
    std::unique_ptr<expression> condExpr( new loop_condition_expression );

    condExpr->set_input( 0, condMeta.second );

    this->register_expression( std::move( condExpr ) );

    // Steal the expression created and add it to the loop container's expression.
    loopExpr->set_condition_expression( m_currentExpression.begin() + expressionStart, m_currentExpression.end() );

    m_currentExpression.resize( expressionStart );

    // Build the rest of the expression connected to the internal output. This is the actual loop body.
    this->build_subtree( loopOutputID );

    std::unique_ptr<expression> loopUpdateExpr( new loop_update_expression( outputLayout ) );

    for( int i = 1, iEnd = loopOutputNode->get_num_inputs(); i < iEnd; ++i ) {
        const temporary_meta& meta = *this->get_input_value( *loopOutputNode, i );
        const temporary_meta& expectedMeta = this->get_value( node->get_id(), i - 1 );
        if( meta.first != expectedMeta.first )
            throw magma_exception() << magma_exception::node_id( loopOutputNode->get_id() )
                                    << magma_exception::input_index( i ) << magma_exception::found_type( meta.first )
                                    << magma_exception::expected_type( expectedMeta.first )
                                    << magma_exception::error_name( _T("Invalid type") );

        loopUpdateExpr->set_input( i - 1, meta.second );
    }
    loopUpdateExpr->set_output( outPtr );

    this->register_expression( std::move( loopUpdateExpr ) );

    // Steal the expression created and add it to the loop container's expression.
    loopExpr->set_body_expression( m_currentExpression.begin() + expressionStart, m_currentExpression.end() );

    m_currentExpression.resize( expressionStart );

    // Register the entire loop as a single expression. TODO: We need to figure out how to deal with getting
    // debugging data out of here
    this->register_expression( static_cast<std::unique_ptr<expression>>( std::move( loopExpr ) ) );
}

void base_compiler::compile( nodes::magma_loop_inputs_node* node ) {
    // DO NOTHING
}

void base_compiler::compile( nodes::magma_loop_outputs_node* node ) {
    // DO NOTHING
}

void base_compiler::compile( nodes::magma_loop_channel_node* node ) {
    if( !m_currentLoop )
        THROW_MAGMA_INTERNAL_ERROR( node->get_id() );

    // The loop channel node's implementation is defined by the loop that is currently being compiled.
    m_currentLoop->handle_loop_input( *this, node->get_id(), node->get_channelName() );
}

void base_compiler::compile_foreach(
    std::unique_ptr<foreach_expression> expr, expression_id exprID,
    const std::vector<std::pair<expression_id, int>>& loopControlInputs,
    const std::vector<magma_data_type>& loopControlExpectedTypes,
    magma_interface::magma_id loopBodyInputNode,  // TODO This is kind of weird design, since we are passing a nodeID
                                                  // here but avoiding referring to a node by not passing one ...
    magma_interface::magma_id loopBodyOutputNode, // TODO This is kind of weird design, since we are passing a nodeID
                                                  // here but avoiding referring to a node by not passing one ...
    const std::vector<std::pair<expression_id, int>>& loopBodyInputs, const std::vector<int>& loopBodyOutputMask ) {
    // Our expression must:
    //  1. Copy the incoming updateable values to the output location
    //  2. Evaluate the condition output of the inner expression, if false we are done
    //  3. If true, then evaluate the loop expression updating the input/output values
    //  4. goto 2

    frantic::magma::simple_compiler::base_compiler::foreach_expression* oldLoop = m_currentLoop;

    m_currentLoop = expr.get();

    BOOST_SCOPE_EXIT( ( &m_currentLoop )( oldLoop ) ) { m_currentLoop = oldLoop; }
    BOOST_SCOPE_EXIT_END;

    // Process the control inputs
    if( loopControlInputs.size() != loopControlExpectedTypes.size() )
        THROW_MAGMA_INTERNAL_ERROR( exprID );

    std::vector<magma_data_type>::const_iterator itTypes = loopControlExpectedTypes.begin();

    for( std::vector<std::pair<expression_id, int>>::const_iterator it = loopControlInputs.begin(),
                                                                    itEnd = loopControlInputs.end();
         it != itEnd; ++it, ++itTypes ) {
        const temporary_meta& meta = this->get_value( it->first, it->second );
        if( meta.first != *itTypes )
            throw magma_exception() << magma_exception::node_id( exprID )
                                    << magma_exception::input_index( (int)( it - loopControlInputs.begin() ) )
                                    << magma_exception::connected_id( it->first )
                                    << magma_exception::connected_output_index( it->second )
                                    << magma_exception::found_type( meta.first )
                                    << magma_exception::expected_type( *itTypes )
                                    << magma_exception::error_name( _T("Input value is incompatible type") );

        expr->set_input( static_cast<std::size_t>( it - loopControlInputs.begin() ), meta.second );
    }

    // Calculate the output layout from the loop using the list of channel indices in 'outputMask'.
    frantic::channels::channel_map outputLayout;

    for( std::vector<int>::const_iterator it = loopBodyOutputMask.begin(), itEnd = loopBodyOutputMask.end();
         it != itEnd; ++it ) {
        if( *it < 0 || *it >= (int)loopBodyInputs.size() )
            throw magma_exception() << magma_exception::node_id( exprID )
                                    << magma_exception::property_name( _T("outputMask") )
                                    << magma_exception::error_name(
                                           _T("Invalid index \"") + boost::lexical_cast<frantic::tstring>( *it ) +
                                           _T("in outputMask[") +
                                           boost::lexical_cast<frantic::tstring>( it - loopBodyOutputMask.begin() ) +
                                           _T("]") );

        const temporary_meta& meta = this->get_value( loopBodyInputs[*it].first, loopBodyInputs[*it].second );

        if( meta.first.m_elementType == frantic::channels::data_type_invalid )
            throw magma_exception()
                << magma_exception::node_id( exprID ) << magma_exception::input_index( *it )
                << magma_exception::output_index( static_cast<int>( it - loopBodyOutputMask.begin() ) )
                << magma_exception::error_name(
                       _T("Non-value inputs (ex. InputGeometry) cannot be in the loop output list") );

        frantic::tstring outDescription =
            _T("Input") + boost::lexical_cast<frantic::tstring>( it - loopBodyOutputMask.begin() );

        outputLayout.define_channel( outDescription, meta.first.m_elementCount, meta.first.m_elementType );
    }

    outputLayout.end_channel_definition( 4u, true );

    // Allocate storage for the output parameters
    std::ptrdiff_t outPtr = this->allocate_temporaries( exprID, outputLayout );

    // Allocate the storage & expression for exposing the loop iteration index
    std::ptrdiff_t iterPtr = this->allocate_temporary( loopBodyInputNode, traits<int>::get_type() ).second;

    expr->set_body_variables( outputLayout );
    {
        std::size_t bodyVarIndex = 0;
        for( std::vector<int>::const_iterator it = loopBodyOutputMask.begin(), itEnd = loopBodyOutputMask.end();
             it != itEnd; ++it, ++bodyVarIndex )
            expr->set_body_variable_init(
                bodyVarIndex, this->get_value( loopBodyInputs[*it].first, loopBodyInputs[*it].second ).second );

        // for( std::vector< std::pair<expression_id,int> >::const_iterator it = loopBodyInputs.begin(), itEnd =
        // loopBodyInputs.end(); it != itEnd; ++it, ++bodyVarIndex ) 	expr->set_body_variable_init( bodyVarIndex,
        //this->get_value( it->first, it->second ).second );
    }
    expr->set_output( outPtr );
    expr->set_iteration_output( iterPtr );
    expr->set_temporary_variables( this->allocate_temporaries(
        -77, expr->get_temporary_variables() ) ); // Arbitrarily chose -77. Should probably use a defined constant.

    magma_interface::magma_id loopInputID = loopBodyInputNode;

    // Expose the input values from the internal input socket. Input only params are forwarded from their source but
    // output parameters are redirected to the output location of the loop container node.
    for( int i = 0, iEnd = (int)loopBodyInputs.size(); i < iEnd; ++i ) {
        std::vector<int>::const_iterator outMaskIt =
            std::find( loopBodyOutputMask.begin(), loopBodyOutputMask.end(), i );

        if( outMaskIt != loopBodyOutputMask.end() ) {
            const temporary_meta& meta =
                this->get_value( exprID, static_cast<int>( outMaskIt - loopBodyOutputMask.begin() ) );

            this->register_temporary( loopInputID, meta.first, meta.second );
        } else {
            const temporary_meta& meta = this->get_value( loopBodyInputs[i].first, loopBodyInputs[i].second );

            this->register_temporary( loopInputID, meta.first, meta.second );
        }
    }

    // Figure out where the conditional part of the expression begins, so we can steal those and put them inside the
    // loop.
    std::size_t expressionStart = m_currentExpression.size();

    magma_node_base* loopOutputNode = m_magma->get_node( loopBodyOutputNode );
    if( !loopOutputNode )
        THROW_MAGMA_INTERNAL_ERROR( exprID );

    if( static_cast<std::size_t>( loopOutputNode->get_num_inputs() ) != outputLayout.channel_count() )
        THROW_MAGMA_INTERNAL_ERROR( exprID, loopOutputNode->get_num_inputs(), outputLayout.channel_count() );

    // Build the expression associated with the loop body.
    this->build_subtree( loopBodyOutputNode );

    for( std::size_t i = 0, iEnd = outputLayout.channel_count(); i < iEnd; ++i ) {
        const temporary_meta& meta = *this->get_input_value( *loopOutputNode, (int)i );
        const temporary_meta& expectedMeta = this->get_value( exprID, (int)i );
        if( meta.first != expectedMeta.first )
            throw magma_exception() << magma_exception::node_id( loopOutputNode->get_id() )
                                    << magma_exception::input_index( (int)i )
                                    << magma_exception::found_type( meta.first )
                                    << magma_exception::expected_type( expectedMeta.first )
                                    << magma_exception::error_name( _T("Invalid type") );

        expr->set_body_variable_update( i, meta.second );
    }

    // Steal the expression created and add it to the loop container's expression.
    expr->set_loop_body( m_currentExpression.begin() + expressionStart, m_currentExpression.end() );

    m_currentExpression.resize( expressionStart );

    // Register the entire loop as a single expression. TODO: We need to figure out how to deal with getting
    // debugging data out of here
    this->register_expression( static_cast<std::unique_ptr<expression>>( std::move( expr ) ) );
}

// Handle the generic case for non-standard nodes.
void base_compiler::compile( magma_node_base* node ) { node->compile_as_extension_type( *this ); }

namespace {
class mux_expression : public base_compiler::expression {
    std::vector<std::ptrdiff_t> m_inputRelPtrs;
    std::ptrdiff_t m_outputRelPtr;

    frantic::channels::channel_map m_outMap;

  public:
    mux_expression( std::size_t numInputs, const magma_data_type& resultType ) {
        m_inputRelPtrs.resize( numInputs, -1 );
        m_outputRelPtr = -1;

        m_outMap.define_channel( _T("Value"), resultType.m_elementCount, resultType.m_elementType );
        m_outMap.end_channel_definition();
    }

    virtual void set_input( std::size_t inputIndex, std::ptrdiff_t relPtr ) { m_inputRelPtrs[inputIndex] = relPtr; }
    virtual void set_output( std::ptrdiff_t relPtr ) { m_outputRelPtr = relPtr; }

    virtual const frantic::channels::channel_map& get_output_map() const { return m_outMap; }

    virtual void apply( base_compiler::state& data ) const {
        int select = data.get_temporary<int>( m_inputRelPtrs.back() );
        char* dest = (char*)data.get_output_pointer( m_outputRelPtr );
        char* src = (char*)data.get_output_pointer(
            m_inputRelPtrs[frantic::math::clamp( select, 0, (int)m_inputRelPtrs.size() - 2 )] );

        m_outMap.copy_structure( dest, src );
    }
};
} // namespace

void base_compiler::compile( nodes::magma_mux_node* node ) {
    int ni = node->get_num_inputs();

    boost::scoped_array<temporary_meta> inputs( new temporary_meta[ni] );

    for( int i = 0; i < ni; ++i ) {
        inputs[i] = *this->get_input_value( *node, i );

        if( i > 0 && i < ni - 1 && inputs[i].first != inputs[0].first )
            throw magma_exception() << magma_exception::node_id( node->get_id() )
                                    << magma_exception::connected_id( node->get_input( i ).first )
                                    << magma_exception::connected_output_index( node->get_input( i ).second )
                                    << magma_exception::found_type( inputs[i].first )
                                    << magma_exception::expected_type( inputs[0].first )
                                    << magma_exception::error_name( _T("Mux inputs must all have same type") );
    }

    if( !traits<int>::is_compatible( inputs[ni - 1].first ) )
        throw magma_exception() << magma_exception::node_id( node->get_id() )
                                << magma_exception::connected_id( node->get_input( ni - 1 ).first )
                                << magma_exception::connected_output_index( node->get_input( ni - 1 ).second )
                                << magma_exception::found_type( inputs[ni - 1].first )
                                << magma_exception::expected_type( traits<int>::get_type() )
                                << magma_exception::error_name( _T("Mux selector input must be Int") );

    std::unique_ptr<expression> result( new mux_expression( (std::size_t)ni, inputs[0].first ) );

    std::ptrdiff_t resultRelPtr = this->allocate_temporary( node->get_id(), inputs[1].first ).second;

    for( int i = 0; i < ni; ++i )
        result->set_input( (std::size_t)i, inputs[i].second );
    result->set_output( resultRelPtr );

    this->register_expression( std::move( result ) );
}

} // namespace simple_compiler
} // namespace magma
} // namespace frantic
