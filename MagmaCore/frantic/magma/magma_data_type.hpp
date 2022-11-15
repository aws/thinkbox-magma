// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#pragma warning( push )
#pragma warning( disable : 4345 4512 4100 )
#include <boost/variant.hpp>
#pragma warning( pop )

#include <frantic/channels/channel_map.hpp>
#include <frantic/channels/named_channel_data.hpp>
#include <frantic/graphics/quat4f.hpp>
#include <frantic/graphics/vector3f.hpp>
#include <frantic/strings/tstring.hpp>

namespace frantic {
namespace magma {

typedef frantic::graphics::vector3f vec3;
typedef frantic::graphics::quat4f quat;

typedef boost::variant<boost::blank, float, int, bool, vec3, quat> variant_t;

struct magma_data_type {
    frantic::channels::data_type_t m_elementType;
    std::size_t m_elementCount;
    const frantic::tchar* m_typeName;

    static const magma_data_type s_invalidType;

    magma_data_type()
        : m_typeName( NULL )
        , m_elementType( frantic::channels::data_type_invalid )
        , m_elementCount( 0 ) {}

    bool operator==( const magma_data_type& rhs ) const {
        return ( m_elementType == rhs.m_elementType && m_elementCount == rhs.m_elementCount );
    }

    bool operator!=( const magma_data_type& rhs ) const {
        return ( m_elementType != rhs.m_elementType || m_elementCount != rhs.m_elementCount );
    }

    inline frantic::tstring to_string() const {
        if( m_typeName )
            return m_typeName;
        if( m_elementType != frantic::channels::data_type_invalid )
            return frantic::channels::channel_data_type_str( m_elementCount, m_elementType );
        return frantic::tstring();
    }
};

inline std::ostream& operator<<( std::ostream& out, const frantic::magma::magma_data_type& v ) {
    out << frantic::strings::to_string( v.to_string() );
    return out;
}

inline std::wostream& operator<<( std::wostream& out, const frantic::magma::magma_data_type& v ) {
    out << frantic::strings::to_wstring( v.to_string() );
    return out;
}

const magma_data_type& get_variant_type( const variant_t& val );

variant_t make_variant( const magma_data_type& type, const void* data );

// Convenience function.
inline variant_t make_variant( frantic::channels::data_type_t type, std::size_t arity, const void* data ) {
    magma_data_type dt;
    dt.m_elementCount = arity;
    dt.m_elementType = type;
    return make_variant( dt, data );
}

void unpack_variant( void* dest, const magma_data_type& type, const variant_t& src );

} // namespace magma
} // namespace frantic
