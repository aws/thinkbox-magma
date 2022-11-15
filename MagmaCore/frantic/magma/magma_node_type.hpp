// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_data_type.hpp>
#include <frantic/magma/magma_node_base.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace frantic {
namespace magma {

class magma_node_base;
class magma_singleton;

class property_meta_interface {
  public:
    virtual ~property_meta_interface() {}

    virtual bool is_visible() const = 0;
    virtual bool is_readonly() const = 0;
    virtual const std::type_info& get_type() const = 0;
    virtual const frantic::tstring& get_name() const = 0;
    virtual void get_value( const magma_node_base* node, void* dest ) const = 0;
    virtual void set_value( magma_node_base* node, const void* src ) const = 0;
    virtual void copy_value( magma_node_base* dest, const magma_node_base* src ) const = 0;

    virtual void get_enumeration_values( std::vector<frantic::tstring>& /*outValues*/ ) const {}
};

class input_meta_interface {
  public:
    virtual ~input_meta_interface() {}

    virtual const frantic::tstring& get_name() const = 0;
    virtual const variant_t& get_default_value() const = 0;
    virtual bool get_visitable() const = 0;
};

class output_meta_interface {
  public:
    virtual const frantic::tstring& get_name() const = 0;
};

class magma_node_type {
  public:
    virtual ~magma_node_type() {}

    virtual magma_singleton&
    get_singleton() const = 0; // alternatively we could have nodes track the magma_interface they are contained within
                               // and that would make this unneccessary.

    virtual magma_node_base* create_instance( magma_interface::magma_id id ) const = 0;

    // True if this should be visible to the user, false if this node is internally constructed and used.
    virtual bool is_public() const = 0;
    virtual bool is_container() const = 0;

    virtual const frantic::tstring& get_name() const = 0;
    virtual const frantic::tstring& get_category() const = 0;
    virtual const frantic::tstring& get_description() const = 0;

    virtual int get_num_properties() const = 0;
    virtual const property_meta_interface& get_property( int i ) const = 0;
    virtual const property_meta_interface* get_property_by_name( const frantic::tstring& propName ) const = 0;

    virtual int get_num_inputs() const = 0;
    virtual const input_meta_interface& get_input( int i ) const = 0;

    virtual int get_num_outputs() const = 0;
    virtual const output_meta_interface& get_output( int i ) const = 0;
};

// TODO: Hide this implementation somewhere.
class magma_node_type_impl : public magma_node_type {
    magma_node_base* ( *m_creatorFn )();

    magma_singleton* m_singleton;

    bool m_isPublic, m_isContainer;

    frantic::tstring m_name;
    frantic::tstring m_category;
    frantic::tstring m_description;

    std::map<frantic::tstring, std::size_t> m_propNameToIndices;
    std::size_t m_numHiddenProps;

    std::vector<property_meta_interface*> m_props;
    std::vector<input_meta_interface*> m_inputs;
    std::vector<output_meta_interface*> m_outputs;

  public:
    magma_node_type_impl( magma_singleton& singleton );
    virtual ~magma_node_type_impl();

    void set_name( const frantic::tstring& name ) { m_name = name; }
    void set_category( const frantic::tstring& category ) { m_category = category; }
    void set_description( const frantic::tstring& description ) { m_description = description; }
    void set_factory( magma_node_base* ( *factory )() ) { m_creatorFn = factory; }
    void set_public( bool isPublic ) { m_isPublic = isPublic; }
    void set_container( bool isContainer ) { m_isContainer = isContainer; }
    void set_disableable( bool isDisableable );

    void add_property( std::unique_ptr<property_meta_interface> propMeta, bool isVisible = true );
    void add_input( std::unique_ptr<input_meta_interface> inputMeta ) { m_inputs.push_back( inputMeta.release() ); }
    void add_output( std::unique_ptr<output_meta_interface> outputMeta ) {
        m_outputs.push_back( outputMeta.release() );
    }

    virtual magma_node_base* create_instance( magma_interface::magma_id id ) const;

    virtual magma_singleton& get_singleton() const { return *m_singleton; }

    virtual bool is_public() const { return m_isPublic; }
    virtual bool is_container() const { return m_isContainer; }

    virtual const frantic::tstring& get_name() const { return m_name; }
    virtual const frantic::tstring& get_category() const { return m_category; }
    virtual const frantic::tstring& get_description() const { return m_description; }

    virtual int get_num_properties() const { return static_cast<int>( m_props.size() - m_numHiddenProps ); }
    virtual const property_meta_interface& get_property( int i ) const { return *m_props[i]; }
    virtual const property_meta_interface* get_property_by_name( const frantic::tstring& propName ) const;

    virtual int get_num_inputs() const { return static_cast<int>( m_inputs.size() ); }
    virtual const input_meta_interface& get_input( int i ) const { return *m_inputs[i]; }

    virtual int get_num_outputs() const { return static_cast<int>( m_outputs.size() ); }
    virtual const output_meta_interface& get_output( int i ) const { return *m_outputs[i]; }
};

} // namespace magma
} // namespace frantic
