// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_data_type.hpp>
#include <frantic/magma/magma_exception.hpp>
#include <frantic/magma/magma_node_type.hpp>

#include <boost/smart_ptr.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace frantic {
namespace magma {

class magma_singleton;

namespace nodes {
void define_standard_operators( magma_singleton& singleton );
}

class magma_singleton {
    std::map<frantic::tstring, std::size_t> m_nodeTypeMap;
    std::vector<boost::shared_ptr<magma_node_type>> m_nodeTypes;

    friend void frantic::magma::nodes::define_standard_operators( magma_singleton& self );

  private:
    static std::map<frantic::tstring, magma_data_type> s_dataTypes;

    inline static void define_data_type( const frantic::tstring& name, frantic::channels::data_type_t repr,
                                         std::size_t arity ) {
        std::map<frantic::tstring, magma_data_type>::const_iterator it = s_dataTypes.find( name );
        if( it != s_dataTypes.end() ) {
            // Allow the same exact type to be defined multiple times ...
            if( it->second.m_elementCount != arity || it->second.m_elementType != repr )
                THROW_MAGMA_INTERNAL_ERROR();
            return;
        }

        magma_data_type& type = s_dataTypes[name];
        type.m_elementType = repr;
        type.m_elementCount = arity;
        type.m_typeName = s_dataTypes.find( name )->first.c_str();
    }

    template <class T>
    inline static void define_data_type( const frantic::tstring& name ) {
        define_data_type( name, frantic::channels::channel_data_type_traits<T>::data_type(),
                          frantic::channels::channel_data_type_traits<T>::arity() );
    }

  protected:
    inline void define_node_type( boost::shared_ptr<magma_node_type> type ) {
        if( m_nodeTypeMap.find( type->get_name() ) != m_nodeTypeMap.end() )
            THROW_MAGMA_INTERNAL_ERROR();
        m_nodeTypeMap[type->get_name()] = m_nodeTypes.size();
        m_nodeTypes.push_back( type );
    }

    magma_singleton( bool defineStandardTypes = true );

    virtual ~magma_singleton() {}

    // Override this if you have a different magma_interface implementation.
    virtual std::unique_ptr<magma_interface> create_magma_instance_impl();

  public:
    template <class T>
    inline void define_node_type() {
        boost::shared_ptr<magma_node_type_impl> type( new magma_node_type_impl( *this ) );
        T::create_type_definition( *type );

        define_node_type( type );
    }

  public:
    // In order to embed Magma in your application, you need to override this object and provide
    // a singleton instance. Custom types can be defined in the overriden constructor.
    //
    // static magma_singleton& get_instance();

    std::unique_ptr<magma_interface> create_magma_instance();

    std::size_t get_num_node_types() const;

    const frantic::tstring& get_node_type_name( std::size_t i ) const;

    const magma_node_type* get_named_node_type( const frantic::tstring& name ) const;

    /**
     * This static function provides named access to the underlying data types passed between Magma nodes
     * at runtime. It is static so that it can be accessed globally.
     */
    static const magma_data_type* get_named_data_type( const frantic::tstring& name );

    /**
     * Gets the type which best matches the "legacy" named channel data types.
     */
    static const magma_data_type* get_matching_data_type( frantic::channels::data_type_t type, std::size_t arity );

    virtual void
    get_predefined_particle_channels( std::vector<std::pair<frantic::tstring, magma_data_type>>& outChannels ) const;

    virtual void
    get_predefined_vertex_channels( std::vector<std::pair<frantic::tstring, magma_data_type>>& outChannels ) const;

    virtual void
    get_predefined_face_channels( std::vector<std::pair<frantic::tstring, magma_data_type>>& outChannels ) const;

    virtual void
    get_predefined_face_vertex_channels( std::vector<std::pair<frantic::tstring, magma_data_type>>& outChannels ) const;
};

} // namespace magma
} // namespace frantic
