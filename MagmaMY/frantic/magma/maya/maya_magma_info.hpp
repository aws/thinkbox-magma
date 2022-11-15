// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <typeinfo>
#include <vector>

#include <frantic/magma/magma_data_type.hpp>
#include <frantic/magma/magma_interface.hpp>

#include "frantic/convert/tstring.hpp"
#include "frantic/magma/maya/maya_magma_datatypes.hpp"

///
/// no one should directly to create any instance of these classes;
/// USE the factory class in maya_magma_factory.hpp to create them
///
namespace frantic {
namespace magma {
namespace maya {
namespace info {

////////////////////////////////////////////////////////////////////////////////
/// maya_magma_info_interface
////////////////////////////////////////////////////////////////////////////////
struct maya_magma_info_interface {
    virtual ~maya_magma_info_interface() {}

    virtual frantic::tstring to_tstring() const = 0;
};

////////////////////////////////////////////////////////////////////////////////
/// maya_magma_node_property_info
////////////////////////////////////////////////////////////////////////////////
struct maya_magma_node_property_info : public maya_magma_info_interface {
    int m_index;             // index of property num in a node (get created when iterate though a node's properties)
    frantic::tstring m_name; // property name (get from maya_magma_holder)
    frantic::tstring m_type; // property data type (get from maya_magma_holder)
    bool m_isReadOnly;       // is read only (get from maya_magma_holder)
    std::vector<frantic::tstring>
        m_acceptedValues; // sometimes property only accepted predefined string values; this stored all the predefine
                          // string values if need (get from maya_magma_holder)
  public:
    maya_magma_node_property_info()
        : m_index( -1 )
        , m_name( _T( "" ) )
        , m_type( _T( "" ) )
        , m_isReadOnly( false )
        , m_acceptedValues() {}

    ~maya_magma_node_property_info() {}

    frantic::tstring to_tstring() const;
};

////////////////////////////////////////////////////////////////////////////////
/// maya_magma_node_input_socket_info
////////////////////////////////////////////////////////////////////////////////
struct maya_magma_node_input_socket_info : public maya_magma_info_interface {
    int m_index;                                    // the index of input socket
    frantic::tstring m_description;                 // description of the input socket
    maya_magma_input_socket_data_type_t m_dataType; // supporting data type of the input socket
    holder::input_socket_variant_t m_data;          // this is the current data (used for displaying purpose)
  public:
    maya_magma_node_input_socket_info() {}

    ~maya_magma_node_input_socket_info() {}

    frantic::tstring to_tstring() const;
};

////////////////////////////////////////////////////////////////////////////////
/// maya_magma_node_output_socket_info
////////////////////////////////////////////////////////////////////////////////
struct maya_magma_node_output_socket_info : public maya_magma_info_interface {
    int m_index;
    frantic::tstring m_description;

  public:
    maya_magma_node_output_socket_info() {}

    ~maya_magma_node_output_socket_info() {}

    frantic::tstring to_tstring() const;
};

////////////////////////////////////////////////////////////////////////////////
/// maya_magma_node_info
////////////////////////////////////////////////////////////////////////////////
struct maya_magma_node_info : public maya_magma_info_interface {
    frantic::tstring m_nodeType;
    frantic::tstring m_typeDescription;
    frantic::tstring m_nodeCategory;
    std::vector<maya_magma_node_property_info> m_propertyInfos;
    std::vector<maya_magma_node_input_socket_info> m_inputSocketInfos;
    std::vector<maya_magma_node_output_socket_info> m_outputSocketInfos;

  public:
    maya_magma_node_info() {}

    ~maya_magma_node_info() {}

    void push_property_info( const maya_magma_node_property_info& info ) { m_propertyInfos.push_back( info ); }

    void push_input_socket_info( const maya_magma_node_input_socket_info& info ) {
        m_inputSocketInfos.push_back( info );
    }

    void push_output_socket_info( const maya_magma_node_output_socket_info& info ) {
        m_outputSocketInfos.push_back( info );
    }

    frantic::tstring properties_to_tstring( int index = -1 ) const;

    frantic::tstring input_socket_to_tstring( int index = -1 ) const;

    frantic::tstring output_socket_to_tstring( int index = -1 ) const;

    void add_properties_to_list( MStringArray& arr, int index = -1 ) const;

    void add_input_socket_to_list( MStringArray& arr, int index = -1 ) const;

    void add_output_socket_to_list( MStringArray& arr, int index = -1 ) const;

    frantic::tstring to_tstring() const;

    void add_to_list( MStringArray& arr ) const;
};

} // namespace info
} // namespace maya
} // namespace magma
} // namespace frantic
