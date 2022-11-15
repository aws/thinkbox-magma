// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"
#include <frantic/magma/magma_data_type.hpp>
#include <frantic/magma/magma_singleton.hpp>

#include <frantic/channels/channel_map.hpp>

#include <boost/throw_exception.hpp>

#include <stdexcept>

namespace frantic {
namespace magma {

const magma_data_type magma_data_type::s_invalidType;

namespace {
template <class T>
struct get_type_helper {
    inline static const magma_data_type& apply() { return magma_data_type::s_invalidType; }
};
template <>
struct get_type_helper<float> {
    inline static const magma_data_type& apply() { return *magma_singleton::get_named_data_type( _T("Float") ); }
};
template <>
struct get_type_helper<int> {
    inline static const magma_data_type& apply() { return *magma_singleton::get_named_data_type( _T("Int") ); }
};
template <>
struct get_type_helper<bool> {
    inline static const magma_data_type& apply() { return *magma_singleton::get_named_data_type( _T("Bool") ); }
};
template <>
struct get_type_helper<frantic::graphics::vector3f> {
    inline static const magma_data_type& apply() { return *magma_singleton::get_named_data_type( _T("Vec3") ); }
};
template <>
struct get_type_helper<frantic::graphics::quat4f> {
    inline static const magma_data_type& apply() { return *magma_singleton::get_named_data_type( _T("Quat") ); }
};
} // namespace

class get_type_visitor : public boost::static_visitor<const magma_data_type&> {
  public:
    template <class T>
    const magma_data_type& operator()( const T& ) const {
        return get_type_helper<typename boost::remove_reference<typename boost::remove_const<T>::type>::type>::apply();
    }
};

const magma_data_type& get_variant_type( const variant_t& val ) {
    return boost::apply_visitor( get_type_visitor(), val );
}

variant_t make_variant( const magma_data_type& type, const void* data ) {
    // TODO: Use Boost.MPL to loop over the types in the variant automatically.

    if( type == *magma_singleton::get_named_data_type( _T("Float") ) )
        return variant_t( *reinterpret_cast<const float*>( data ) );
    if( type == *magma_singleton::get_named_data_type( _T("Int") ) )
        return variant_t( *reinterpret_cast<const int*>( data ) );
    if( type == *magma_singleton::get_named_data_type( _T("Bool") ) )
        return variant_t( *reinterpret_cast<const bool*>( data ) );
    if( type == *magma_singleton::get_named_data_type( _T("Vec3") ) )
        return variant_t( *reinterpret_cast<const vec3*>( data ) );
    if( type == *magma_singleton::get_named_data_type( _T("Quat") ) )
        return variant_t( *reinterpret_cast<const quat*>( data ) );
    return variant_t();
}

class set_channel_visitor : public boost::static_visitor<void> {
    magma_data_type m_expectedType;
    void* m_dest;

  public:
    set_channel_visitor( void* dest, const magma_data_type& expectedType )
        : m_dest( dest )
        , m_expectedType( expectedType ) {}

    template <class T>
    void operator()( const T& val ) const {
        typedef typename boost::remove_reference<typename boost::remove_const<T>::type>::type value_type;

        if( m_expectedType != get_type_helper<value_type>::apply() )
            BOOST_THROW_EXCEPTION( std::runtime_error( "set_channel_visitor() - Type mismatch" ) );

        *static_cast<T*>( m_dest ) = val;
    }
};

void unpack_variant( void* dest, const magma_data_type& type, const variant_t& src ) {
    set_channel_visitor visitor( dest, type );

    boost::apply_visitor( visitor, src );
}

} // namespace magma
} // namespace frantic
