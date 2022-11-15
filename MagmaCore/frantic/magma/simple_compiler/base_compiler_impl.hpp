// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_movable.hpp>
#include <frantic/magma/nodes/magma_input_geometry_interface.hpp>
#include <frantic/magma/nodes/magma_input_objects_interface.hpp>
#include <frantic/magma/nodes/magma_input_particles_interface.hpp>
#include <frantic/magma/simple_compiler/base_compiler.hpp>

#include <frantic/graphics/quat4f.hpp>
#include <frantic/graphics/vector3f.hpp>

#include <boost/function_types/parameter_types.hpp>
#include <boost/function_types/result_type.hpp>
#include <boost/integer.hpp>
#include <boost/move/move.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/end.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/size.hpp>

#include <memory>

namespace frantic {
namespace magma {
namespace simple_compiler {

/**
 * Traits classes for converting back and forth between magma_data_type and C++ types.
 */
template <class T>
struct traits;

/**
 * @overload For float type.
 */
template <>
struct traits<float> {
    inline static bool is_compatible( const frantic::magma::magma_data_type& type ) {
        return type.m_elementType == frantic::channels::data_type_float32 && type.m_elementCount == 1;
    }

    inline static frantic::magma::magma_data_type get_type() {
        return *frantic::magma::magma_singleton::get_named_data_type( _T("Float") );
    }

    static frantic::channels::channel_map s_map;

    inline static const frantic::channels::channel_map& get_static_map() {
        if( !s_map.channel_definition_complete() ) {
            s_map.define_channel<float>( _T("Value") );
            s_map.end_channel_definition();
        }
        return s_map;
    }
};

/**
 * @overload For int type.
 */
template <>
struct traits<int> {
    inline static bool is_compatible( const frantic::magma::magma_data_type& type ) {
        return type.m_elementType == frantic::channels::data_type_int32 && type.m_elementCount == 1;
    }

    inline static frantic::magma::magma_data_type get_type() {
        return *frantic::magma::magma_singleton::get_named_data_type( _T("Int") );
    }

    static frantic::channels::channel_map s_map;

    inline static const frantic::channels::channel_map& get_static_map() {
        if( !s_map.channel_definition_complete() ) {
            s_map.define_channel<int>( _T("Value") );
            s_map.end_channel_definition();
        }
        return s_map;
    }
};

/**
 * @overload For bool type.
 */
template <>
struct traits<bool> {
    typedef boost::int_t<sizeof( bool ) * CHAR_BIT>::fast bool_int_type;

    inline static bool is_compatible( const frantic::magma::magma_data_type& type ) {
        return type.m_elementType == frantic::channels::channel_data_type_traits<bool_int_type>::data_type() &&
               type.m_elementCount == 1;
    }

    inline static frantic::magma::magma_data_type get_type() {
        return *frantic::magma::magma_singleton::get_named_data_type( _T("Bool") );
    }

    static frantic::channels::channel_map s_map;

    inline static const frantic::channels::channel_map& get_static_map() {
        if( !s_map.channel_definition_complete() ) {
            s_map.define_channel<bool_int_type>( _T("Value") );
            s_map.end_channel_definition();
        }
        return s_map;
    }
};

/**
 * @overload For vec3 type.
 */
template <>
struct traits<vec3> {
    inline static bool is_compatible( const frantic::magma::magma_data_type& type ) {
        return type.m_elementType == frantic::channels::data_type_float32 && type.m_elementCount == 3;
    }

    inline static frantic::magma::magma_data_type get_type() {
        return *frantic::magma::magma_singleton::get_named_data_type( _T("Vec3") );
    }

    static frantic::channels::channel_map s_map;

    inline static const frantic::channels::channel_map& get_static_map() {
        if( !s_map.channel_definition_complete() ) {
            s_map.define_channel<vec3>( _T("Value") );
            s_map.end_channel_definition();
        }
        return s_map;
    }
};

/**
 * @overload For quat type.
 */
template <>
struct traits<quat> {
    inline static bool is_compatible( const frantic::magma::magma_data_type& type ) {
        return type.m_elementType == frantic::channels::data_type_float32 && type.m_elementCount == 4;
    }

    inline static frantic::magma::magma_data_type get_type() {
        return *frantic::magma::magma_singleton::get_named_data_type( _T("Quat") );
    }

    static frantic::channels::channel_map s_map;

    inline static const frantic::channels::channel_map& get_static_map() {
        if( !s_map.channel_definition_complete() ) {
            s_map.define_channel<quat>( _T("Value") );
            s_map.end_channel_definition();
        }
        return s_map;
    }
};

// This namespace stores types needed to implement base_compiler, but that shouldn't be used outside this
// implementation. (ie. Don't touch!)
namespace detail {
/**
 * Template class for invoking the correct operator() member function of a functor, based on the function signature
 * provided.
 * @tparam The signature of the operator() member function to invoke.
 */
template <class FnSig>
struct function_invoker;

/**
 * @overload To handle R(P1) type functions.
 */
template <class R, class P1>
struct function_invoker<R( P1 )> {
    template <class Functor>
    inline static void apply( char* stack, const Functor& fn, std::ptrdiff_t output, const std::ptrdiff_t inputs[] ) {
        *reinterpret_cast<R*>( stack + output ) = fn( *reinterpret_cast<P1*>( stack + inputs[0] ) );
    }
};

/**
 * @overload To handle R(P1,P2) type functions.
 */
template <class R, class P1, class P2>
struct function_invoker<R( P1, P2 )> {
    template <class Functor>
    inline static void apply( char* stack, const Functor& fn, std::ptrdiff_t output, const std::ptrdiff_t inputs[] ) {
        *reinterpret_cast<R*>( stack + output ) =
            fn( *reinterpret_cast<P1*>( stack + inputs[0] ), *reinterpret_cast<P2*>( stack + inputs[1] ) );
    }
};

/**
 * @overload To handle R(P1,P2,P3) type functions.
 */
template <class R, class P1, class P2, class P3>
struct function_invoker<R( P1, P2, P3 )> {
    template <class Functor>
    inline static void apply( char* stack, const Functor& fn, std::ptrdiff_t output, const std::ptrdiff_t inputs[] ) {
        *reinterpret_cast<R*>( stack + output ) =
            fn( *reinterpret_cast<P1*>( stack + inputs[0] ), *reinterpret_cast<P2*>( stack + inputs[1] ),
                *reinterpret_cast<P3*>( stack + inputs[2] ) );
    }
};

/**
 * @overload To handle R(P1,P2,P3,P4) type functions.
 */
template <class R, class P1, class P2, class P3, class P4>
struct function_invoker<R( P1, P2, P3, P4 )> {
    template <class Functor>
    inline static void apply( char* stack, const Functor& fn, std::ptrdiff_t output, const std::ptrdiff_t inputs[] ) {
        *reinterpret_cast<R*>( stack + output ) =
            fn( *reinterpret_cast<P1*>( stack + inputs[0] ), *reinterpret_cast<P2*>( stack + inputs[1] ),
                *reinterpret_cast<P3*>( stack + inputs[2] ), *reinterpret_cast<P4*>( stack + inputs[3] ) );
    }
};

/**
 * @overload To handle void(void*,P1) type functions.
 */
template <class P1>
struct function_invoker<void( void*, P1 )> {
    template <class Functor>
    inline static void apply( char* stack, const Functor& fn, std::ptrdiff_t output, const std::ptrdiff_t inputs[] ) {
        fn( stack + output, *reinterpret_cast<P1*>( stack + inputs[0] ) );
    }
};

/**
 * @overload To handle void(void*,P1,P2) type functions.
 */
template <class P1, class P2>
struct function_invoker<void( void*, P1, P2 )> {
    template <class Functor>
    inline static void apply( char* stack, const Functor& fn, std::ptrdiff_t output, const std::ptrdiff_t inputs[] ) {
        fn( stack + output, *reinterpret_cast<P1*>( stack + inputs[0] ), *reinterpret_cast<P2*>( stack + inputs[1] ) );
    }
};

/**
 * @overload To handle void(void*,P1,P2,P3) type functions.
 */
template <class P1, class P2, class P3>
struct function_invoker<void( void*, P1, P2, P3 )> {
    template <class Functor>
    inline static void apply( char* stack, const Functor& fn, std::ptrdiff_t output, const std::ptrdiff_t inputs[] ) {
        fn( stack + output, *reinterpret_cast<P1*>( stack + inputs[0] ), *reinterpret_cast<P2*>( stack + inputs[1] ),
            *reinterpret_cast<P3*>( stack + inputs[2] ) );
    }
};

template <class Functor, class FnSig>
class expression_impl : public base_compiler::expression {
    typedef typename boost::function_types::result_type<FnSig>::type return_type;
    typedef typename boost::function_types::parameter_types<FnSig>::type param_types;
    typedef function_invoker<FnSig> invoker;

    enum { NUM_INPUTS = boost::mpl::size<param_types>::value };

    std::ptrdiff_t m_inputRelPtrs[NUM_INPUTS];
    std::ptrdiff_t m_outputRelPtr;

    Functor m_fn;

    // The DUMMY template parameter is to avoid complete template
    // specialization, which is not permitted here in GCC.
    template <class T, class DUMMY = void>
    struct get_output_map_impl {
        inline static const frantic::channels::channel_map& apply( const Functor& /*fn*/ ) {
            return traits<T>::get_static_map();
        }
    };

    template <class DUMMY>
    struct get_output_map_impl<void, DUMMY> {
        inline static const frantic::channels::channel_map& apply( const Functor& fn ) { return fn.get_output_map(); }
    };

    // This is non-virtual on purpose!
    static void internal_apply( const base_compiler::expression* _this, base_compiler::state& data ) {
        invoker::apply( (char*)data.get_output_pointer(
                            0 ), // HACK Should be passing state directly and using its get/set member functions.
                        static_cast<const expression_impl*>( _this )->m_fn,
                        static_cast<const expression_impl*>( _this )->m_outputRelPtr,
                        static_cast<const expression_impl*>( _this )->m_inputRelPtrs );
    }

  public:
    expression_impl( const Functor& fn )
        : m_fn( fn ) {}

    expression_impl( const movable<Functor>& fn )
        : m_fn( fn ) {}

    // expression_impl( BOOST_RV_REF(Functor) fn ) : m_fn( fn )
    //{}

    virtual ~expression_impl() {}

    virtual void set_input( std::size_t inputIndex, std::ptrdiff_t relPtr ) { m_inputRelPtrs[inputIndex] = relPtr; }
    virtual void set_output( std::ptrdiff_t relPtr ) { m_outputRelPtr = relPtr; }

    virtual const frantic::channels::channel_map& get_output_map() const {
        // For regular function signatures, we use a static channel_map instance cached in the traits<T> structure.
        // For void(void*,...) style functions (which indicate multiple and/or complex returned types) we will ask
        //  the implementation functor itself to provide a channel_map.
        return get_output_map_impl<return_type>::apply( m_fn );
    }

    virtual void apply( base_compiler::state& data ) const {
        invoker::apply( (char*)data.get_output_pointer( 0 ) /*HACK for now*/, m_fn, m_outputRelPtr, m_inputRelPtrs );
    }

    // We can avoid virtual function calls by extracting a direct function ptr and storing that alongside the ptr to the
    // subexpression. This might be quicker since it eliminates an indirection, but it also might not matter much
    // relative to the actual overhead of calling the function. Gotta test, test, test
    //
    // Another way this might get used is if we switch to a LLVM compiled approach. Since some nodes will still be
    // implemented in MSVC++ we need to use C ptrs in order to cross between LLVM & MSVC since their C++ ABI will not
    // match (ie. Cannot directly call virtual member functions from LLVM that were compiled in MSVC++).
    virtual runtime_ptr get_runtime_ptr() const { return &internal_apply; }
};

template <class T>
class constant_expression : public base_compiler::expression {
    std::ptrdiff_t m_outPtr;
    T m_value;

    static void internal_apply( const expression* _this, base_compiler::state& data ) {
        data.set_temporary( static_cast<const constant_expression*>( _this )->m_outPtr,
                            static_cast<const constant_expression*>( _this )->m_value );
    }

  public:
    constant_expression( typename boost::call_traits<T>::param_type val )
        : m_value( val ) {}

    virtual ~constant_expression() {}

    virtual void set_input( std::size_t /*inputIndex*/, std::ptrdiff_t /*relPtr*/ ) { THROW_MAGMA_INTERNAL_ERROR(); }

    virtual void set_output( std::ptrdiff_t relPtr ) { m_outPtr = relPtr; }

    virtual const frantic::channels::channel_map& get_output_map() const { return traits<T>::get_static_map(); }

    virtual void apply( base_compiler::state& data ) const { data.set_temporary( m_outPtr, m_value ); }

    virtual runtime_ptr get_runtime_ptr() const { return &internal_apply; }
};

/**
 * We need to determine a modified function signature when working with expressions of the form void(void*,...) since
 * the void* is not a real input, but is instead an output parameter. That needs to be stripped out.
 *
 * The baseline case is no modification to the signature.
 * @tparam The function signature of an expression.
 */
template <class FnSig>
struct modified_signature {
    typedef FnSig type;
};

// Handles stripping out the void* output parameter for signatures of this type
template <class P1>
struct modified_signature<void( void*, P1 )> {
    typedef void( type )( P1 );
};

// Handles stripping out the void* output parameter for signatures of this type
template <class P1, class P2>
struct modified_signature<void( void*, P1, P2 )> {
    typedef void( type )( P1, P2 );
};

// Handles stripping out the void* output parameter for signatures of this type
template <class P1, class P2, class P3>
struct modified_signature<void( void*, P1, P2, P3 )> {
    typedef void( type )( P1, P2, P3 );
};

/**
 * We support tagging a class as movable so it isn't copied on assignment. We need to get at the underlying type though
 * so this template metafunction strips off the movable tag. The baseline version does nothing.
 */
template <class T>
struct remove_movable {
    typedef T type;
};

// Stips the movable wrapper type of the specified type.
template <class T>
struct remove_movable<movable<T>> {
    typedef T type;
};

/**
 * Checks that the runtime 'inputs' sequence corresponds to the types stored in the metasequence defined by
 * 'ParamIterator' and 'ParamIteratorEnd'. Returns true if the inputs match the sequence.
 * @tparam ParamIterator An iterator over an MPL type sequence, pointing to the "current" type we are iterating over.
 * @tparam ParamIteratorEnd An iterator over an MPL type sequence, pointing to the end of the metasequence we are
 * iterating over.
 * @param inputs A pointer to the "current" element in the list of inputs we are matching against.
 */
template <class ParamIterator, class ParamIteratorEnd>
struct check_params {
    inline static bool apply( const base_compiler::temporary_meta* inputs ) {
        typedef typename boost::mpl::deref<ParamIterator>::type CurParamType;
        if( !traits<CurParamType>::is_compatible( inputs->first ) )
            return false;
        return check_params<typename boost::mpl::next<ParamIterator>::type, ParamIteratorEnd>::apply( inputs + 1 );
    }
};

/**
 * Recursive termination case when the end iterator is reached.
 */
template <class ParamIteratorEnd>
struct check_params<ParamIteratorEnd, ParamIteratorEnd> {
    inline static bool apply( const base_compiler::temporary_meta* ) { return true; }
};

/**
 * Given a sequence of function signatures (denoted by iterators SigIterator and SigIteratorEnd), finds one that matches
 * the types of the 'inputs' sequence and allocates a subclass of expression that delegates to the provided functor's
 * operator() function that matches the selected signature.
 */
template <class Functor, class SigIterator, class SigIteratorEnd>
struct select_binding {
    inline static std::unique_ptr<base_compiler::expression>
    apply( const Functor& fn, const base_compiler::temporary_meta inputs[], std::size_t numInputs ) {
        typedef typename boost::mpl::deref<SigIterator>::type RealFnSig; // Get the signature from the iterator
        typedef typename modified_signature<RealFnSig>::type
            CurFnSig; // Modify the signature to strip out the void* parameter for the void(void*,...) style signatures.
        typedef typename boost::function_types::parameter_types<CurFnSig>::type
            CurParamTypes; // Convert the signature to a boost::mpl::vector<...>

        typedef typename boost::mpl::begin<CurParamTypes>::type
            CurParamsIt; // Get iterator to first element in parameter list
        typedef typename boost::mpl::end<CurParamTypes>::type CurParamsItEnd; // Get iterator to end of parameter list

        std::size_t ParamCount = boost::mpl::size<CurParamTypes>::value; // Extract the number of parameters so we can
                                                                         // make sure 'inputs' matches.

        if( numInputs == ParamCount && check_params<CurParamsIt, CurParamsItEnd>::apply( inputs ) ) {
            typedef typename remove_movable<Functor>::type RealFunctor; // Strip off a 'movable' wrapper type if
                                                                        // present.

            std::unique_ptr<base_compiler::expression> result( new expression_impl<RealFunctor, RealFnSig>( fn ) );

            for( std::size_t i = 0; i < numInputs; ++i )
                result->set_input( i, inputs[i].second );

            return result;
        } else {
            // The inputs didn't match the current signature so advance the signature iterator and check the next one.
            typedef typename boost::mpl::next<SigIterator>::type NextFnSig;
            return select_binding<Functor, NextFnSig, SigIteratorEnd>::apply( fn, inputs, numInputs );
        }
    }
};

/**
 * Recursive termination case when the end iterator is reached.
 */
template <class Functor, class FnMetaIteratorEnd>
struct select_binding<Functor, FnMetaIteratorEnd, FnMetaIteratorEnd> {
    inline static std::unique_ptr<base_compiler::expression>
    apply( const Functor&, const base_compiler::temporary_meta[], std::size_t ) {
        return std::unique_ptr<base_compiler::expression>();
    }
};

struct type_collector {
    std::vector<magma_data_type>* m_pCollection;

    type_collector( std::vector<magma_data_type>& binding )
        : m_pCollection( &binding ) {}

    template <typename T>
    void operator()( const T& ) const {
        m_pCollection->push_back( traits<T>::get_type() );
    }
};

template <class SigIterator, class SigIteratorEnd>
struct collect_bindings {
    static void apply( std::vector<std::vector<magma_data_type>>::iterator outBindingsIt ) {
        typedef typename boost::mpl::deref<SigIterator>::type RealFnSig; // Get the signature from the iterator
        typedef typename modified_signature<RealFnSig>::type
            CurFnSig; // Modify the signature to strip out the void* parameter for the void(void*,...) style signatures.
        typedef typename boost::function_types::parameter_types<CurFnSig>::type
            CurParamTypes; // Convert the signature to a boost::mpl::vector<...>

        boost::mpl::for_each<CurParamTypes>( type_collector( *outBindingsIt ) );

        typedef typename boost::mpl::next<SigIterator>::type NextFnSig;

        collect_bindings<NextFnSig, SigIteratorEnd>::apply( ++outBindingsIt );
    }
};

template <class FnMetaIteratorEnd>
struct collect_bindings<FnMetaIteratorEnd, FnMetaIteratorEnd> {
    static void apply( std::vector<std::vector<magma_data_type>>::iterator& ) {}
};
} // namespace detail

template <class ExpressionMetaData>
void base_compiler::compile_impl( expression_id exprID, const typename ExpressionMetaData::type& fn,
                                  const temporary_meta inputs[], std::size_t numInputs,
                                  std::size_t expectedNumOutputs ) {
    typedef typename boost::mpl::begin<typename ExpressionMetaData::bindings>::type BindingsIt;
    typedef typename boost::mpl::end<typename ExpressionMetaData::bindings>::type BindingsItEnd;

    // Create an instance of a subexpression subclass that binds to the input types. Assigns the inputs and returns the
    // new instance.
    std::unique_ptr<expression> result =
        detail::select_binding<typename ExpressionMetaData::type, BindingsIt, BindingsItEnd>::apply( fn, inputs,
                                                                                                     numInputs );
    if( !result.get() ) {
        std::vector<magma_data_type> foundInputs;
        std::vector<std::vector<magma_data_type>> expectedInputs;

        foundInputs.reserve( numInputs );
        expectedInputs.resize( boost::mpl::size<typename ExpressionMetaData::bindings>::value );

        for( std::size_t i = 0; i < numInputs; ++i ) {
            foundInputs.push_back( inputs[i].first );
            if( foundInputs.back().m_typeName == NULL ) {
                const magma_data_type* pBetterType = magma_singleton::get_matching_data_type(
                    foundInputs.back().m_elementType, foundInputs.back().m_elementCount );
                if( pBetterType != NULL )
                    foundInputs.back() = *pBetterType;
            }
        }

        detail::collect_bindings<BindingsIt, BindingsItEnd>::apply( expectedInputs.begin() );

        BOOST_THROW_EXCEPTION( magma_exception() << magma_exception::node_id( exprID )
                                                 << magma_exception::error_name( _T("Invalid input combination") )
                                                 << magma_exception::found_inputs( boost::move( foundInputs ) )
                                                 << magma_exception::expected_inputs( boost::move( expectedInputs ) ) );
    }

    const frantic::channels::channel_map& resultsMap = result->get_output_map();

    if( resultsMap.channel_count() != expectedNumOutputs )
        THROW_MAGMA_INTERNAL_ERROR( exprID, expectedNumOutputs, result->get_output_map().channel_count() );

    result->set_output( this->allocate_temporaries( exprID, resultsMap ) );

    this->register_expression( std::move( result ) );
}

template <class ExpressionMetaData>
void base_compiler::compile_impl( const frantic::magma::magma_node_base& node,
                                  const typename ExpressionMetaData::type& fn ) {
    if( node.get_num_inputs() != ExpressionMetaData::ARITY )
        THROW_MAGMA_INTERNAL_ERROR( node.get_id(), node.get_num_inputs(), (int)ExpressionMetaData::ARITY );

    temporary_meta inputs[ExpressionMetaData::ARITY];
    for( std::size_t i = 0; i < ExpressionMetaData::ARITY; ++i )
        inputs[i] = *this->get_input_value( node, (int)i, false );

    return this->compile_impl<ExpressionMetaData>( node.get_id(), fn, inputs, ExpressionMetaData::ARITY,
                                                   node.get_num_outputs() );
}

namespace detail {
template <class T>
struct type_to_name {};

template <>
struct type_to_name<nodes::magma_input_geometry_interface> {
    static const frantic::tchar* get_name() { return _T("Geometry"); }
};

template <>
struct type_to_name<nodes::magma_input_particles_interface> {
    static const frantic::tchar* get_name() { return _T("Particles"); }
};

template <>
struct type_to_name<nodes::magma_input_objects_interface> {
    static const frantic::tchar* get_name() { return _T("Objects"); }
};
}; // namespace detail

template <class Interface>
Interface* base_compiler::get_interface( expression_id exprID, int index, bool allowNull ) {
    if( exprID == magma_interface::INVALID_ID ) {
        if( !allowNull )
            throw magma_exception() << magma_exception::error_name( _T("Unconnected input socket") );
        return NULL;
    }

    std::pair<magma_interface::magma_id, int> input( exprID, index );

    // Interface* result = NULL;
    // do{
    const std::pair<magma_data_type, std::ptrdiff_t>& value = this->get_value( input.first, input.second );
    if( value.first.m_elementType !=
        frantic::channels::data_type_invalid ) // HACK This should be replaced later with a generalized type that
                                               // encapsulates the actual ptr value.
        throw magma_exception() << magma_exception::connected_id( input.first )
                                << magma_exception::connected_output_index( input.second )
                                << magma_exception::found_type( value.first )
                                << magma_exception::error_name( frantic::tstring() + _T("Expected a ") +
                                                                detail::type_to_name<Interface>::get_name() +
                                                                _T(" node") );

    // HACK: This is temporary, but is the general idea of what I want.
    if( value.first.m_typeName != frantic::tstring( detail::type_to_name<Interface>::get_name() ) )
        throw magma_exception() << magma_exception::connected_id( input.first )
                                << magma_exception::connected_output_index( input.second )
                                << magma_exception::found_type( value.first )
                                << magma_exception::error_name( frantic::tstring() + _T("Expected a ") +
                                                                detail::type_to_name<Interface>::get_name() +
                                                                _T(" node") );

    return reinterpret_cast<Interface*>( value.second );

    //	frantic::magma::magma_node_base* node = m_magma->get_node( input.first );
    //	if( !node )
    //		THROW_MAGMA_INTERNAL_ERROR();

    //	result = dynamic_cast<Interface*>( node );
    //	if( !result ){
    //		std::pair<magma_interface::magma_id, int> prev = input;

    //		if( frantic::magma::nodes::magma_elbow_node* elbow = dynamic_cast<frantic::magma::nodes::magma_elbow_node*>(
    //node ) ) 			input = elbow->get_input( input.second ); 		else if( frantic::magma::nodes::magma_blop_node* blop =
    //dynamic_cast<frantic::magma::nodes::magma_blop_node*>( node ) ) 			input.first = blop->get__internal_output_id();
    //		else if( frantic::magma::nodes::magma_blop_input_node* blopSocket =
    //dynamic_cast<frantic::magma::nodes::magma_blop_input_node*>( node ) ) 			input = blopSocket->get_output( input.second
    //); 		else if( frantic::magma::nodes::magma_loop_inputs_node* loopInputs =
    //dynamic_cast<frantic::magma::nodes::magma_loop_inputs_node*>( node ) ) 			input =
    //loopInputs->get_output_socket_passthrough( input.second ); 		else 			throw magma_exception() <<
    //magma_exception::error_name( _T("Incorrect input type") );

    //		//Make sure the next item is not disconnected. This should have been caught by visit() unless we have a
    //bug. 		if( input.first == magma_interface::INVALID_ID ) 			throw magma_exception() << magma_exception::node_id(
    //prev.first ) << magma_exception::input_index( prev.second ) << magma_exception::error_name( _T("Unconnected input
    //socket") );
    //	}
    //}while( !result );

    // return result;
}

template <class Interface>
Interface* base_compiler::get_input_interface( frantic::magma::magma_node_base& node, int inputIndex,
                                               bool allowUnconnected ) {
    Interface* result = NULL;
    std::pair<expression_id, int> input = node.get_input( inputIndex );

    if( input.first == magma_interface::INVALID_ID ) {
        if( !allowUnconnected )
            throw magma_exception() << magma_exception::node_id( node.get_id() )
                                    << magma_exception::input_index( inputIndex )
                                    << magma_exception::error_name( _T("Unconnected input socket") );
    } else {
        try {
            result = this->get_interface<Interface>( input.first, input.second );
        } catch( magma_exception& e ) {
            if( e.get_node_id() == magma_interface::INVALID_ID )
                e << magma_exception::node_id( node.get_id() ) << magma_exception::input_index( inputIndex );
            throw;
        }
    }

    return result;
}

} // namespace simple_compiler
} // namespace magma
} // namespace frantic
