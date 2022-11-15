// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_node_base.hpp>
#include <frantic/magma/nodes/magma_blop_node.hpp>
#include <frantic/magma/nodes/magma_node_property.hpp>
#include <frantic/magma/nodes/magma_simple_operator.hpp>

#include <vector>

namespace frantic {
namespace magma {
namespace nodes {

// This node is a placeholder that allows nodes contained within a magma_blop_node to connect (indirectly) to the inputs
// of the container node.
class magma_blop_input_node : public magma_input_node {
    magma_blop_node* m_parent;

  protected:
    const magma_blop_node* get_parent() const { return m_parent; }

  public:
    MAGMA_REQUIRED_METHODS( magma_blop_input_node );

    magma_blop_input_node();

    void set_parent( magma_blop_node& parent ) { m_parent = &parent; }

    virtual int get_num_outputs() const { return m_parent->get_num_inputs(); }

    // This is not a magma_node_base member
    virtual std::pair<magma_interface::magma_id, int> get_output( int index ) const {
        return m_parent->get_input( index );
    }
};

// This node is a placeholder that simplifies the design of container nodes like magma_blop_node. This exists as a
// concrete node so that the outputs of the blop can be accessed indirectly via the inputs of this node.
class magma_blop_output_node : public magma_node_base {
    std::vector<std::pair<magma_interface::magma_id, int>> m_connections;
    magma_blop_node* m_parent;

  protected:
    const magma_blop_node* get_parent() const { return m_parent; }

  public:
    MAGMA_REQUIRED_METHODS( magma_blop_output_node );

    magma_blop_output_node();

    void set_parent( magma_blop_node& parent ) { m_parent = &parent; }

    virtual int get_num_inputs() const { return (int)m_connections.size(); }
    virtual void set_num_inputs( int numInputs ) {
        m_connections.resize( std::max( /*1*/ 0, numInputs ), std::make_pair( magma_interface::INVALID_ID, 0 ) );
    }
    virtual std::pair<magma_interface::magma_id, int> get_input( int index ) const { return m_connections[index]; }
    virtual void set_input( int index, magma_interface::magma_id id, int outputIndex ) {
        m_connections[index].first = id;
        m_connections[index].second = outputIndex;
    }

    virtual int get_num_outputs() const { return 0; }
};

} // namespace nodes
} // namespace magma
} // namespace frantic
