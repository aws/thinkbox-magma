// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "frantic/magma/maya/maya_magma_common.hpp"
#include "frantic/magma/maya/maya_magma_serializable.hpp"

#include <memory>

namespace frantic {
namespace magma {
namespace maya {
namespace holder {

////////////////////////////////////////////////////////////////////////////////
/// maya_magma_holder
////////////////////////////////////////////////////////////////////////////////
class maya_magma_holder {
  private:
    boost::shared_ptr<magma_interface> m_magma;

  public:
    maya_magma_holder( std::unique_ptr<frantic::magma::magma_interface> magmaInterface );

    virtual ~maya_magma_holder();

#pragma region Operation on Magma
    virtual boost::shared_ptr<magma_interface> get_magma_interface();

    virtual void clear();

    // operations
    // virtual Interval get_validity( TimeValue t ) const;
    // virtual bool is_read_only() const;
    // virtual bool is_loading() const;
    // virtual void set_is_loading( bool loading = true );
    // virtual void reset();

    ////////////////////////////////////////
    // operation on node
    ////////////////////////////////////////

    virtual magma_id create_node( const frantic::tstring& typeName );

    virtual bool delete_node( magma_id id );

    // these methods I do not think we will ever use it; we all modifiication will be done in desc first,
    // and then we convert magma::desc to magma::holder
    //
    // virtual void delete_node_recursive( magma_id id );
    // virtual bool replace_node( magma_id idDest, magma_id idSrc );

    virtual frantic::tstring get_node_type( magma_id id ) const;

    virtual frantic::tstring get_type_description( const frantic::tstring& typeName ) const;

    virtual frantic::tstring get_type_category( const frantic::tstring& typeName ) const;

    // virtual bool is_node_container( magma_id id ) const;
    // virtual bool is_node_creatable( magma_id id ) const;

    ////////////////////////////////////////
    // property setting
    ////////////////////////////////////////

    virtual int get_node_num_properties( magma_id id ) const;

    virtual frantic::tstring get_node_property_type( magma_id id, const frantic::tstring& propName ) const;

    virtual frantic::tstring get_node_property_name( magma_id id, index_type index ) const;

    virtual bool set_node_property( magma_id id, const frantic::tstring& propName, property_variant_t val );

    // XXX support this ?
    std::vector<frantic::tstring> get_node_property_names( magma_id id ) const;

    std::vector<frantic::tstring> get_node_enum_values( magma_id id, const frantic::tstring& propName ) const;

    virtual bool get_node_property_readonly( magma_id id, const frantic::tstring& propName ) const;

    // we do not support this
    // virtual property_variant_t get_node_property( magma_id id, const frantic::tstring& propName ) const;
    // virtual bool add_node_property( magma_id id, const MCHAR* propName );

    ////////////////////////////////////////
    // input/output sockets
    ////////////////////////////////////////

    virtual int get_num_node_inputs( magma_id id ) const;

    virtual void set_num_node_inputs( magma_id id, int numInputs );

    virtual int get_num_node_outputs( magma_id id ) const;

    virtual input_socket_variant_t get_node_input_default_value( magma_id id, index_type socketIndex ) const;

    virtual bool set_node_input_default_value( magma_id id, index_type socketIndex, input_socket_variant_t val );

    virtual frantic::tstring get_node_input_description( magma_id id, index_type socketIndex ) const;

    virtual frantic::tstring get_node_output_description( magma_id id, index_type socketIndex ) const;

    ////////////////////////////////////////
    // input/output sockets connection
    ////////////////////////////////////////

    virtual bool set_node_input( magma_id id, index_type index, magma_id connectedID, index_type connectedIndex );

    // virtual Value* get_node_input( magma_id id, index_type index ) const;

    // virtual bool set_node_input_default_value( magma_id id, index_type socketIndex, const FPValue& val );
    // virtual bool set_num_node_inputs( magma_id id, int numInputs );
    // virtual bool set_num_node_outputs( magma_id id, int numOutputs );

    ////////////////////////////////////////
    // for getting all nodes in current magma flow
    ////////////////////////////////////////

    virtual std::vector<magma_id> get_nodes() const;

    // container
    // virtual magma_id get_container_source() const;
    // virtual magma_id get_container_sink() const;

    // BLOP
    virtual bool push_editable_BLOP( magma_id id );
    virtual magma_id pop_editable_BLOP();

    magma_id get_BLOP_source_id( magma_id id ) const;
    magma_id get_BLOP_sink_id( magma_id id ) const;

    // virtual int num_editing_BLOPs() const;
    // virtual magma_id_tab get_BLOP_stack() const;
    // virtual bool explode_blop( magma_id id );
    // virtual magma_id create_blop( const magma_id_tab& nodes );

    // miscellaneous
    // virtual Tab<const MCHAR*> get_type_names() const;
#pragma endregion
};

} // namespace holder
} // namespace maya
} // namespace magma
} // namespace frantic
