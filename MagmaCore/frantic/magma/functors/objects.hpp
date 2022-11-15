// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_singleton.hpp>
#include <frantic/magma/nodes/magma_input_objects_interface.hpp>

namespace frantic {
namespace magma {
namespace functors {

namespace {

// append a channel with any of the frantic::magma::variant_t types to a channel map ( except boost::blank ), throws a
// magma_exception if it's none of the accepted types.
void define_property( frantic::channels::channel_map& map, const frantic::tstring& propertyName,
                      const std::type_info& propertyType ) {
    if( propertyType == typeid( int ) ) {
        map.define_channel( propertyName, 1, frantic::channels::data_type_int32 );
    } else if( propertyType == typeid( float ) ) {
        map.define_channel( propertyName, 1, frantic::channels::data_type_float32 );
    } else if( propertyType == typeid( bool ) ) {
        map.define_channel( propertyName, 1, frantic::channels::data_type_uint8 );
    } else if( propertyType == typeid( vec3 ) ) {
        map.define_channel( propertyName, 3, frantic::channels::data_type_float32 );
    } else if( propertyType == typeid( quat ) ) {
        map.define_channel( propertyName, 4, frantic::channels::data_type_float32 );
    } else {
        throw magma_exception() << magma_exception::property_name( _T("properties") )
                                << magma_exception::error_name( _T("Could not define \"") + propertyName +
                                                                _T("\" property does not have a valid type") );
    }
}

// memcpy frantic::magma::variant_t types ( except boost::blank which throws a magma_exception )
void copy_variant( char* out, frantic::magma::variant_t val ) {
    if( val.type() == typeid( boost::blank ) )
        throw magma_exception() << magma_exception::property_name( _T("properties") )
                                << magma_exception::error_name( _T("compy_variant: failed to copy invalid type") );

    if( val.type() == typeid( int ) ) {
        memcpy( out, &( boost::get<int>( val ) ), sizeof( int ) );
    } else if( val.type() == typeid( float ) ) {
        memcpy( out, &( boost::get<float>( val ) ), sizeof( float ) );
    } else if( val.type() == typeid( bool ) ) {
        memcpy( out, &( boost::get<bool>( val ) ), sizeof( bool ) );
    } else if( val.type() == typeid( vec3 ) ) {
        memcpy( out, &( boost::get<vec3>( val ) ), sizeof( vec3 ) );
    } else if( val.type() == typeid( quat ) ) {
        memcpy( out, &( boost::get<quat>( val ) ), sizeof( quat ) );
    }
}
} // namespace

class object_query {
  protected:
    frantic::magma::nodes::magma_input_objects_interface* m_objInterface;
    const std::vector<frantic::tstring>& m_properties;

    std::vector<std::vector<char>> m_cachedOutput;

    frantic::channels::channel_map m_outMap;

    void define_map_and_cache_properties() {
        std::vector<frantic::magma::variant_t> vals( m_properties.size() );
        for( std::size_t i = 0; i < m_properties.size(); ++i ) {
            m_objInterface->get_property( 0, m_properties[i], vals[i] );
            if( vals[i].type() == typeid( boost::blank ) )
                throw magma_exception() << magma_exception::property_name( _T("properties") )
                                        << magma_exception::error_name(
                                               _T("The InputObjects don't all have a property named: \"") +
                                               m_properties[i] + _T("\"") );

            frantic::tstring propName = m_properties[i];
            std::replace( propName.begin(), propName.end(), '.', '_' );

            define_property( m_outMap, propName, vals[i].type() );
        }
        m_outMap.end_channel_definition( 4, true );

        m_cachedOutput =
            std::vector<std::vector<char>>( m_objInterface->size(), std::vector<char>( m_outMap.structure_size() ) );

        frantic::magma::variant_t val;
        frantic::channels::channel_general_accessor propertyAccessor;
        for( std::size_t i = 0; i < m_properties.size(); ++i ) {
            frantic::tstring propName = m_properties[i];
            std::replace( propName.begin(), propName.end(), '.', '_' );
            propertyAccessor = m_outMap.get_general_accessor( propName );
            for( std::size_t k = 0; k < m_objInterface->size(); ++k ) {
                m_objInterface->get_property( k, m_properties[i], val );

                char* outVal = propertyAccessor.get_channel_data_pointer( &m_cachedOutput[k][0] );

                if( val.type() == typeid( boost::blank ) )
                    throw magma_exception()
                        << magma_exception::property_name( _T("properties") )
                        << magma_exception::error_name( _T("The InputObjects don't all have a property named: \"") +
                                                        m_properties[i] + _T("\"") );

                // make sure the objects don't have different types for the same property
                if( val.type() != vals[i].type() )
                    throw magma_exception()
                        << magma_exception::property_name( _T("properties") )
                        << magma_exception::error_name(
                               _T("The InputObjects don't all have the same type for the property: \"") +
                               m_properties[i] + _T("\"") );

                copy_variant( outVal, val );
            }
        }
    }

    object_query& operator=( const object_query& ); // not implemented

  public:
    object_query( frantic::magma::nodes::magma_input_objects_interface* objInterface,
                  const std::vector<frantic::tstring>& properties )
        : m_objInterface( objInterface )
        , m_properties( properties ) {
        define_map_and_cache_properties();
    }

    const frantic::channels::channel_map& get_output_map() const { return m_outMap; }

    inline void operator()( void* out, int _objIndex = 0 ) const throw() {
        std::size_t objIndex = static_cast<std::size_t>( _objIndex );

        if( objIndex < m_objInterface->size() ) {
            memcpy( out, &m_cachedOutput[objIndex][0], m_outMap.structure_size() );
        } else {
            memset( out, 0, m_outMap.structure_size() );
        }
    }
};
} // namespace functors
} // namespace magma
} // namespace frantic
