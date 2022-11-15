// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_compiler_interface.hpp>
#include <frantic/magma/magma_exception.hpp>
#include <frantic/magma/magma_node_type.hpp>

#pragma warning( push )
#pragma warning( disable : 4512 )
#include <boost/assign/list_inserter.hpp> // for 'push_back()'
#pragma warning( pop )

#include <memory>
#include <utility>

//#define MAGMA_DEFINE_TYPE( name, category, type ) \
//namespace{ \
//	magma_node_base* create_##type(){ return new type; } \
//} \
//void type::compile( magma_compiler_interface& compiler ){ \
//	compiler.compile( this ); \
//} \
//void type::create_type_definition( magma_node_type& outType ){ \
//	outType.init_type( name, category, &create_##type ); \
//	typedef type this_type;
#define MAGMA_DEFINE_TYPE( name, category, type )                                                                      \
    namespace {                                                                                                        \
    frantic::magma::magma_node_base* create_##type() { return new type; }                                              \
    }                                                                                                                  \
    void type::compile( frantic::magma::magma_compiler_interface& compiler ) { compiler.compile( this ); }             \
    void type::create_type_definition( frantic::magma::magma_node_type& _outType ) {                                   \
        frantic::magma::magma_node_type_impl& outType =                                                                \
            dynamic_cast<frantic::magma::magma_node_type_impl&>( _outType );                                           \
        outType.set_name( _T( name ) );                                                                                \
        outType.set_category( _T( category ) );                                                                        \
        outType.set_factory( &create_##type );                                                                         \
        {                                                                                                              \
            std::unique_ptr<frantic::magma::property_meta<bool, magma_node_base>> propMeta(                            \
                new frantic::magma::property_meta<bool, magma_node_base> );                                            \
            propMeta->m_name = _T("enabled");                                                                          \
            propMeta->m_getter = &magma_node_base::get_enabled;                                                        \
            propMeta->m_setter = &magma_node_base::set_enabled;                                                        \
            outType.add_property( std::unique_ptr<frantic::magma::property_meta_interface>( propMeta.release() ),      \
                                  true );                                                                              \
        }                                                                                                              \
        typedef type this_type;

//#define MAGMA_MAKE_HELPERS( name, type ) \
//	struct name##_helper{ \
//		static void get( magma_node_base* node, void* outVal ){ \
//			*static_cast< type* >(outVal) = reinterpret_cast<this_type*>(node)->get_##name(); \
//		} \
//		static void set( magma_node_base* node, const void* inVal ){ \
//			static_cast<this_type*>(node)->set_##name( *reinterpret_cast< type const* >(inVal) ); \
//		} \
//	};

//#define MAGMA_EXPOSE_PROPERTY_EX( name, type, isVisible ) \
//	MAGMA_MAKE_HELPERS( name, type ) \
//	outType.add_property< type >( #name, &name##_helper::get , &name##_helper::set, (isVisible) );
//
//#define MAGMA_EXPOSE_PROPERTY( name, type ) MAGMA_EXPOSE_PROPERTY_EX( name, type, true )
//#define MAGMA_HIDDEN_PROPERTY( name, type ) MAGMA_EXPOSE_PROPERTY_EX( name, type, false )
//
// #define MAGMA_READONLY_PROPERTY( name, type ) \
//struct name##_helper{ \
//	static void get( magma_node_base* node, void* outVal ){ \
//		*reinterpret_cast< type* >(outVal) = static_cast<this_type*>(node)->get_##name(); \
//	} \
//}; \
//outType.add_property< type >( "" #name, &name##_helper::get );

#define MAGMA_EXPOSE_PROPERTY( name, type )                                                                            \
    {                                                                                                                  \
        frantic::magma::detail::register_property_impl<type, this_type>::apply(                                        \
            outType, _T( #name ), &this_type::get_##name, &this_type::set_##name, true );                              \
    }

/*std::unique_ptr< frantic::magma::property_meta<type,this_type> > propMeta( new
   frantic::magma::property_meta<type,this_type> ); \
        propMeta->m_name = _T( #name ); \
        propMeta->m_getter = &this_type::get_##name; \
        propMeta->m_setter = &this_type::set_##name; \
        outType.add_property( std::unique_ptr<frantic::magma::property_meta_interface>( propMeta.release() ), true ); */

#define MAGMA_EXPOSE_ARRAY_PROPERTY( name, type )                                                                      \
    {                                                                                                                  \
        std::unique_ptr<frantic::magma::property_array_meta<type, this_type>> propMeta(                                \
            new frantic::magma::property_array_meta<type, this_type> );                                                \
        propMeta->m_name = _T( #name );                                                                                \
        propMeta->m_getter = &this_type::get_##name;                                                                   \
        propMeta->m_setter = &this_type::set_##name;                                                                   \
        outType.add_property( std::unique_ptr<frantic::magma::property_meta_interface>( propMeta.release() ), true );  \
    }

#define MAGMA_READONLY_PROPERTY( name, type )                                                                          \
    {                                                                                                                  \
        frantic::magma::detail::register_property_impl<type, this_type>::apply( outType, _T( #name ),                  \
                                                                                &this_type::get_##name, NULL, true );  \
    }
/*std::unique_ptr< frantic::magma::property_meta<type,this_type> > propMeta( new
frantic::magma::property_meta<type,this_type> ); \
propMeta->m_name = _T( #name ); \
propMeta->m_getter = &this_type::get_##name; \
propMeta->m_setter = NULL; \
outType.add_property( std::unique_ptr<frantic::magma::property_meta_interface>( propMeta.release() ), true ); \
}*/

#define MAGMA_READONLY_ARRAY_PROPERTY( name, type )                                                                    \
    {                                                                                                                  \
        std::unique_ptr<frantic::magma::property_array_meta<type, this_type>> propMeta(                                \
            new frantic::magma::property_array_meta<type, this_type> );                                                \
        propMeta->m_name = _T( #name );                                                                                \
        propMeta->m_getter = &this_type::get_##name;                                                                   \
        propMeta->m_setter = NULL;                                                                                     \
        outType.add_property( std::unique_ptr<frantic::magma::property_meta_interface>( propMeta.release() ), true );  \
    }

#define MAGMA_HIDDEN_PROPERTY( name, type )                                                                            \
    {                                                                                                                  \
        std::unique_ptr<frantic::magma::property_meta<type, this_type>> propMeta(                                      \
            new frantic::magma::property_meta<type, this_type> );                                                      \
        propMeta->m_name = _T( #name );                                                                                \
        propMeta->m_getter = &this_type::get_##name;                                                                   \
        propMeta->m_setter = &this_type::set_##name;                                                                   \
        outType.add_property( std::unique_ptr<frantic::magma::property_meta_interface>( propMeta.release() ), false ); \
    }

#define MAGMA_HIDDEN_READONLY_PROPERTY( name, type )                                                                   \
    {                                                                                                                  \
        std::unique_ptr<frantic::magma::property_meta<type, this_type>> propMeta(                                      \
            new frantic::magma::property_meta<type, this_type> );                                                      \
        propMeta->m_name = _T( #name );                                                                                \
        propMeta->m_getter = &this_type::get_##name;                                                                   \
        propMeta->m_setter = NULL;                                                                                     \
        outType.add_property( std::unique_ptr<frantic::magma::property_meta_interface>( propMeta.release() ), false ); \
    }

//#define MAGMA_ENUM_PROPERTY( propName, ... ) \
//{ \
//	std::unique_ptr< frantic::magma::property_enum_meta<this_type> > propMeta( new frantic::magma::property_enum_meta<this_type> ); \
//	propMeta->m_name = #propName; \
//	propMeta->m_getter = &this_type::get_##propName; \
//	propMeta->m_setter = &this_type::set_##propName; \
//	boost::assign::push_back(propMeta->m_values) = __VA_ARGS__; \
//	outType.add_property( std::unique_ptr<frantic::magma::property_meta_interface>( propMeta.release() ), true ); \
//}

template <class FnDelegate>
struct string_converter {
    FnDelegate m_fnDelegate;

  public:
    explicit string_converter( const FnDelegate& fnDelegate )
        : m_fnDelegate( fnDelegate ) {}

    void operator()( const char* str ) {
        *m_fnDelegate = frantic::strings::to_tstring( str );
        ++m_fnDelegate;
    }
};

template <class FnDelegate>
inline string_converter<FnDelegate> make_string_converter( const FnDelegate& fnDelegate ) {
    return string_converter<FnDelegate>( fnDelegate );
}

#define MAGMA_ENUM_PROPERTY( propName, ... )                                                                           \
    {                                                                                                                  \
        std::unique_ptr<frantic::magma::property_enum_meta<this_type>> propMeta(                                       \
            new frantic::magma::property_enum_meta<this_type> );                                                       \
        propMeta->m_name = _T( #propName );                                                                            \
        propMeta->m_getter = &this_type::get_##propName;                                                               \
        propMeta->m_setter = &this_type::set_##propName;                                                               \
        boost::assign::make_list_inserter( make_string_converter( std::back_inserter( propMeta->m_values ) ) ) =       \
            __VA_ARGS__;                                                                                               \
        outType.add_property( std::unique_ptr<frantic::magma::property_meta_interface>( propMeta.release() ), true );  \
    }

// This uses a non-standard C++ feature (Variadic Macros) but it is part of the C99 standard and VS 2005+ and GCC3.0+
// support it in C++ so I think its reasonable to use.
//#define MAGMA_ENUM_PROPERTY( propName, ... ) \
//	struct propName##_helper{ \
//		static void get( magma_node_base* node, void* outVal ){ \
//			*reinterpret_cast< std::string* >(outVal) = reinterpret_cast<this_type*>(node)->get_##propName();
//\
//		} \
//		static void set( magma_node_base* node, const void* inVal ){ \
//			const magma_node_type& type = node->get_type(); \
//			const std::vector<std::string>& options = type.get_property_info( type.get_property_index( #propName
//) ).m_enumerationValues; \
//			if( std::find( options.begin(), options.end(), *reinterpret_cast< const std::string* >(inVal) ) ==
//options.end() ) \
//				throw magma_exception() << magma_exception::node_id( node->get_id() ) <<
//magma_exception::property_name( #propName ) << magma_exception::error_name( "Invalid Enumeration" ); \
//			reinterpret_cast<this_type*>(node)->set_##propName( *reinterpret_cast< const std::string* >(inVal)
//); \
//		} \
//	}; \
//	{ \
//		std::vector<std::string> vals; \
//		boost::assign::push_back(vals) = __VA_ARGS__; \
//		outType.add_enumeration( #propName, vals, &propName##_helper::get , &propName##_helper::set ); \
//	};

//#define MAGMA_INPUT_NAMES( ... ) \
//	static const char* inputNames[] = { __VA_ARGS__ }; outType.set_input_names( sizeof(inputNames) / sizeof(char*), inputNames );
//
//#define MAGMA_INPUT_DEFAULTS( ... ) \
//	static const variant_t inputDefaults[] = { __VA_ARGS__ }; outType.set_input_defaults( sizeof(inputDefaults) /
//sizeof(variant_t), inputDefaults );
//
//#define MAGMA_OUTPUT_NAMES( ... ) \
//	static const char* outputNames[] = { __VA_ARGS__ }; outType.set_output_names( sizeof(outputNames) /
//sizeof(char*), outputNames );

#define MAGMA_INPUT( name, defaultValue )                                                                              \
    outType.add_input( std::unique_ptr<frantic::magma::input_meta_interface>(                                          \
        new frantic::magma::input_meta( _T( name ), frantic::magma::variant_t( defaultValue ) ) ) );

#define MAGMA_INPUT_NO_DEFAULT( name )                                                                                 \
    outType.add_input(                                                                                                 \
        std::unique_ptr<frantic::magma::input_meta_interface>( new frantic::magma::input_meta( _T( name ) ) ) );

#define MAGMA_INPUT_BEGIN( name )                                                                                      \
    {                                                                                                                  \
        std::unique_ptr<frantic::magma::input_meta> inputMeta( new frantic::magma::input_meta( _T( name ) ) );

#define MAGMA_INPUT_ATTR( attrName, ... ) inputMeta->set_##attrName( __VA_ARGS__ );

#define MAGMA_INPUT_END                                                                                                \
    outType.add_input( (std::unique_ptr<frantic::magma::input_meta_interface>)inputMeta );                             \
    }

#define MAGMA_INPUT_NAMES( ... )                                                                                       \
    static const char* inputNames[] = { __VA_ARGS__ };                                                                 \
    for( int i = 0, iEnd = sizeof( inputNames ) / sizeof( char* ); i < iEnd; ++i )                                     \
        outType.add_input( std::unique_ptr<frantic::magma::input_meta_interface>(                                      \
            new frantic::magma::input_meta( frantic::strings::to_tstring( inputNames[i] ) ) ) );

#define MAGMA_OUTPUT_NAMES( ... )                                                                                      \
    static const char* outputNames[] = { __VA_ARGS__ };                                                                \
    for( int i = 0, iEnd = sizeof( outputNames ) / sizeof( char* ); i < iEnd; ++i )                                    \
        outType.add_output( std::unique_ptr<frantic::magma::output_meta_interface>(                                    \
            new frantic::magma::output_meta( frantic::strings::to_tstring( outputNames[i] ) ) ) );

#define MAGMA_DESCRIPTION( desc ) outType.set_description( _T( desc ) );

#define MAGMA_TYPE_ATTR( name, ... ) outType.set_##name( __VA_ARGS__ );

#define MAGMA_DEFINE_TYPE_END                                                                                          \
    ;                                                                                                                  \
    }

namespace frantic {
namespace magma {

class property_meta_base : public property_meta_interface {
  public:
    bool m_isVisible;
    frantic::tstring m_name;

    virtual bool is_visible() const { return m_isVisible; }
    virtual bool is_readonly() const { return false; }
    virtual const frantic::tstring& get_name() const { return m_name; }
};

template <class T>
class property_meta_partial : public property_meta_base {
  public:
    virtual const std::type_info& get_type() const { return typeid( T ); }

    virtual void copy_value( magma_node_base* dest, const magma_node_base* src ) const {
        T tempVal;
        this->get_value( src, &tempVal );
        this->set_value( dest, &tempVal );
    }
};

namespace detail {

// Used to select the expected method of passing parameters and return types from get/set property.
template <class T>
struct property_traits {
    typedef typename boost::call_traits<T>::param_type return_type;
    typedef typename boost::call_traits<T>::param_type param_type;
};

} // namespace detail

template <class T, class Owner>
class property_meta : public property_meta_partial<T> {
  public:
    typename frantic::magma::detail::property_traits<T>::return_type ( Owner::*m_getter )() const;
    void ( Owner::*m_setter )( typename frantic::magma::detail::property_traits<T>::param_type );

  public:
    virtual bool is_readonly() const { return ( m_setter == NULL ); }

    virtual void get_value( const magma_node_base* node, void* dest ) const {
        *reinterpret_cast<T*>( dest ) = ( static_cast<const Owner*>( node )->*m_getter )();
    }

    virtual void set_value( magma_node_base* node, const void* src ) const {
        if( m_setter != NULL )
            ( static_cast<Owner*>( node )->*m_setter )( *reinterpret_cast<const T*>( src ) );
    }
};

template <class T>
class property_static_meta : public property_meta_partial<T> {
  public:
    typename frantic::magma::detail::property_traits<T>::return_type ( *m_getter )();
    void ( *m_setter )( typename frantic::magma::detail::property_traits<T>::param_type );

  public:
    virtual bool is_readonly() const { return ( m_setter == NULL ); }

    virtual void get_value( const magma_node_base* /*node*/, void* dest ) const {
        *reinterpret_cast<T*>( dest ) = m_getter();
    }

    virtual void set_value( magma_node_base* node, const void* src ) const {
        if( m_setter != NULL )
            m_setter( *reinterpret_cast<const T*>( src ) );
    }
};

template <class T, class Owner>
class property_array_meta : public property_meta_base {
  public:
    void ( Owner::*m_getter )( std::vector<T>& ) const;
    void ( Owner::*m_setter )( const std::vector<T>& );

  public:
    virtual bool is_readonly() const { return ( m_setter == NULL ); }

    virtual void get_value( const magma_node_base* node, void* dest ) const {
        ( static_cast<const Owner*>( node )->*m_getter )( *reinterpret_cast<std::vector<T>*>( dest ) );
    }

    virtual void set_value( magma_node_base* node, const void* src ) const {
        if( m_setter != NULL )
            ( static_cast<Owner*>( node )->*m_setter )( *reinterpret_cast<const std::vector<T>*>( src ) );
    }

    virtual const std::type_info& get_type() const { return typeid( std::vector<T> ); }

    virtual void copy_value( magma_node_base* dest, const magma_node_base* src ) const {
        std::vector<T> tempVal;
        this->get_value( src, &tempVal );
        this->set_value( dest, &tempVal );
    }
};

template <class Owner>
class property_enum_meta : public property_meta<frantic::tstring, Owner> {
  public:
    std::vector<frantic::tstring> m_values;

  public:
    virtual void set_value( magma_node_base* node, const void* src ) const {
        const frantic::tstring& value = *reinterpret_cast<const frantic::tstring*>( src );

        if( std::find( m_values.begin(), m_values.end(), value ) == m_values.end() )
            throw magma_exception() << magma_exception::node_id( node->get_id() )
                                    << magma_exception::property_name( this->get_name() )
                                    << magma_exception::error_name( _T("Invalid Enumeration Value \"") + value +
                                                                    _T("\"") );

        ( static_cast<Owner*>( node )->*( this->m_setter ) )( value );
    }

    virtual void get_enumeration_values( std::vector<frantic::tstring>& outValues ) const { outValues = m_values; }
};

namespace detail {
template <class T, class Owner>
struct register_property_impl {
    inline static void apply( frantic::magma::magma_node_type_impl& outType, const frantic::tchar propName[],
                              typename detail::property_traits<T>::return_type ( Owner::*getter )() const,
                              void ( Owner::*setter )( typename detail::property_traits<T>::param_type ),
                              bool isVisible ) {
        std::unique_ptr<frantic::magma::property_meta<T, Owner>> propMeta(
            new frantic::magma::property_meta<T, Owner> );
        propMeta->m_name = propName;
        propMeta->m_getter = getter;
        propMeta->m_setter = setter;
        outType.add_property(
            static_cast<std::unique_ptr<frantic::magma::property_meta_interface>>( std::move( propMeta ) ), isVisible );
    }

    inline static void apply( frantic::magma::magma_node_type_impl& outType, const frantic::tchar propName[],
                              typename detail::property_traits<T>::return_type ( *getter )(),
                              void ( *setter )( typename detail::property_traits<T>::param_type ), bool isVisible ) {
        std::unique_ptr<frantic::magma::property_static_meta<T>> propMeta(
            new frantic::magma::property_static_meta<T> );
        propMeta->m_name = propName;
        propMeta->m_getter = getter;
        propMeta->m_setter = setter;
        outType.add_property(
            static_cast<std::unique_ptr<frantic::magma::property_meta_interface>>( std::move( propMeta ) ), isVisible );
    }
};

// TODO Handle more signatures to allow different types of property access.
} // namespace detail

class input_meta : public input_meta_interface {
    frantic::tstring m_name;
    variant_t m_defaultValue;
    bool m_visitable;

  public:
    input_meta( const frantic::tstring& name )
        : m_name( name )
        , m_visitable( true ) {}

    input_meta( const frantic::tstring& name, const variant_t& defaultValue )
        : m_name( name )
        , m_defaultValue( defaultValue )
        , m_visitable( true ) {}

    virtual const frantic::tstring& get_name() const { return m_name; }

    virtual const variant_t& get_default_value() const { return m_defaultValue; }

    virtual bool get_visitable() const { return m_visitable; }

    inline void set_default( const variant_t& value ) { m_defaultValue = value; }

    inline void set_visitable( bool visitable ) { m_visitable = visitable; }
};

class output_meta : public output_meta_interface {
    frantic::tstring m_name;

  public:
    output_meta( const frantic::tstring& name )
        : m_name( name ) {}

    virtual const frantic::tstring& get_name() const { return m_name; }
};

} // namespace magma
} // namespace frantic
