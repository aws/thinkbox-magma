// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_node_base.hpp>
#include <frantic/magma/nodes/magma_node_property.hpp>

#include <vector>

namespace frantic {
namespace magma {
namespace nodes {

class magma_blop_input_node;
class magma_blop_output_node;

class magma_blop_node : public magma_node_base {
    std::vector<std::pair<magma_interface::magma_id, int>> m_inputs;
    std::vector<magma_node_base*> m_nodes;

    magma_blop_input_node* m_internalInput;
    magma_blop_output_node* m_internalOutput;

  protected:
    virtual void do_clone( magma_node_base& dest, clone_callback& cb ) const;

    // I'm not happy about exposing these, but I'm doing it for magma_loop_node so I can move on.
    // TODO make this better!
    magma_blop_input_node* get_internal_input() { return m_internalInput; }
    magma_blop_output_node* get_internal_output() { return m_internalOutput; }

  public:
    MAGMA_REQUIRED_METHODS( magma_blop_node );

    static const frantic::tstring& get_uiType() {
        static frantic::tstring typeString( _T("BLOP") );
        return typeString;
    }
    static const int get_numControlInputs() { return 0; }

    // Read-only properties that expose the id's of special nodes
    const magma_interface::magma_id get__internal_input_id() const;
    const magma_interface::magma_id get__internal_output_id() const;

    magma_blop_node();

    virtual ~magma_blop_node();

    // From magma_node_base

    virtual int get_num_inputs() const;

    virtual void set_num_inputs( int numInputs );

    virtual std::pair<magma_interface::magma_id, int> get_input( int index ) const;

    virtual void set_input( int index, magma_interface::magma_id id, int socketIndex );

    virtual int get_num_outputs() const;

    virtual void set_num_outputs( int numOutputs );

    virtual int get_num_contained_nodes() const;

    virtual magma_node_base* get_contained_node( int index ) const;

    virtual magma_node_base* get_contained_source() const;

    virtual magma_node_base* get_contained_sink() const;

    virtual void append_contained_node( magma_node_base* newNode );

    virtual bool remove_contained_node( magma_interface::magma_id nodeID );

    // Unique interface of magma_blop_node
    void init( magma_blop_input_node* inputNode, magma_blop_output_node* outputNode );

    void set_output( int outputIndex, magma_interface::magma_id idNode, int socketIndex );

    std::pair<magma_interface::magma_id, int> get_output( int outputIndex ) const;

    int get_num_nodes() const;

    magma_node_base* get_node( int index );

    const magma_node_base* get_node( int index ) const;

    void append_node( magma_node_base* node );

    /**
     * Deletes the node with the given id if it is contained in this BLOP.
     * @param id The id of the node to delete
     * @parma recursive If true, any contained BLOP nodes will be searched as well. If false only immediate contained
     * nodes will be searched.
     * @return True if the node was contained in this BLOP and was deleted. False otherwise.
     */
    bool delete_node( magma_interface::magma_id id, bool recursive = true );

    /**
     * Removes all contained nodes and sockets.
     */
    void clear();
};

} // namespace nodes
} // namespace magma
} // namespace frantic
