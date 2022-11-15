// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"
#include <frantic/magma/nodes/magma_blop_node.hpp>
#include <frantic/magma/nodes/magma_blop_socket_node.hpp>
#include <frantic/magma/nodes/magma_node_impl.hpp>

namespace frantic {
namespace magma {
namespace nodes {

MAGMA_DEFINE_TYPE( "BLOP", "BLOP", magma_blop_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_DESCRIPTION( "BLack box OPerator" )
MAGMA_TYPE_ATTR( container, true )
MAGMA_READONLY_PROPERTY( uiType, frantic::tstring )
MAGMA_HIDDEN_READONLY_PROPERTY( _internal_input_id, magma_interface::magma_id )
MAGMA_HIDDEN_READONLY_PROPERTY( _internal_output_id, magma_interface::magma_id )
MAGMA_DEFINE_TYPE_END

magma_blop_node::magma_blop_node() {
    m_internalInput = NULL;
    m_internalOutput = NULL;
}

magma_blop_node::~magma_blop_node() {}

void magma_blop_node::init( magma_blop_input_node* inputNode, magma_blop_output_node* outputNode ) {
    m_internalInput = inputNode;
    m_internalOutput = outputNode;

    if( m_internalInput )
        m_internalInput->set_parent( *this );
    if( m_internalOutput )
        m_internalOutput->set_parent( *this );
}

void magma_blop_node::do_clone( magma_node_base& _dest, clone_callback& cb ) const {
    magma_blop_node& dest = static_cast<magma_blop_node&>( _dest );

    dest.init( m_internalInput ? static_cast<magma_blop_input_node*>( m_internalInput->clone( cb ) ) : NULL,
               m_internalOutput ? static_cast<magma_blop_output_node*>( m_internalOutput->clone( cb ) ) : NULL );

    magma_node_base::do_clone( _dest, cb );

    for( std::vector<magma_node_base*>::const_iterator it = m_nodes.begin(), itEnd = m_nodes.end(); it != itEnd; ++it )
        dest.m_nodes.push_back( ( *it )->clone( cb ) );
}

const magma_interface::magma_id magma_blop_node::get__internal_input_id() const {
    return m_internalInput ? m_internalInput->get_id() : magma_interface::INVALID_ID;
}

const magma_interface::magma_id magma_blop_node::get__internal_output_id() const {
    return m_internalOutput ? m_internalOutput->get_id() : magma_interface::INVALID_ID;
}

int magma_blop_node::get_num_inputs() const { return (int)m_inputs.size(); }

void magma_blop_node::set_num_inputs( int numInputs ) {
    m_inputs.resize( std::max<std::size_t>( 0, static_cast<std::size_t>( numInputs ) ),
                     std::make_pair( magma_interface::INVALID_ID, 0 ) );
}

std::pair<magma_interface::magma_id, int> magma_blop_node::get_input( int index ) const { return m_inputs[index]; }

void magma_blop_node::set_input( int index, magma_interface::magma_id id, int socketIndex ) {
    m_inputs[index] = std::make_pair( id, socketIndex );
}

int magma_blop_node::get_num_outputs() const { return m_internalOutput ? m_internalOutput->get_num_inputs() : 0; }

void magma_blop_node::set_num_outputs( int numOutputs ) {
    if( m_internalOutput )
        m_internalOutput->set_num_inputs( numOutputs );
}

int magma_blop_node::get_num_contained_nodes() const {
    return 2 + (int)m_nodes.size(); // Include the source & sink
}

magma_node_base* magma_blop_node::get_contained_node( int index ) const {
    if( index == 0 )
        return m_internalInput;
    else if( index == 1 )
        return m_internalOutput;
    else
        return m_nodes[index - 2];
}

magma_node_base* magma_blop_node::get_contained_source() const { return m_internalInput; }

magma_node_base* magma_blop_node::get_contained_sink() const { return m_internalOutput; }

void magma_blop_node::append_contained_node( magma_node_base* newContainedNode ) {
    m_nodes.push_back( newContainedNode );
}

bool magma_blop_node::remove_contained_node( magma_interface::magma_id nodeID ) {
    bool result = false;

    // Do a breadth first search. See if the node is contained directly in this node.
    for( std::vector<magma_node_base*>::iterator it = m_nodes.begin(), itEnd = m_nodes.end(); it != itEnd && !result;
         ++it ) {
        if( *it != NULL && ( *it )->get_id() == nodeID ) {
            m_nodes.erase( it );
            result = true;
        }
    }

    // Now search any containers at this level too.
    for( std::vector<magma_node_base*>::const_iterator it = m_nodes.begin(), itEnd = m_nodes.end();
         it != itEnd && !result; ++it ) {
        if( *it != NULL )
            result = ( *it )->remove_contained_node( nodeID );
    }

    return result;
}

int magma_blop_node::get_num_nodes() const { return (int)m_nodes.size(); }

magma_node_base* magma_blop_node::get_node( int index ) { return m_nodes[index]; }

const magma_node_base* magma_blop_node::get_node( int index ) const { return m_nodes[index]; }

void magma_blop_node::append_node( magma_node_base* node ) { m_nodes.push_back( node ); }

bool magma_blop_node::delete_node( magma_interface::magma_id id, bool recursive ) {
    if( m_internalInput && m_internalInput->get_id() == id ) {
        m_internalInput = NULL;
        return true;
    }

    if( m_internalOutput && m_internalOutput->get_id() == id ) {
        m_internalOutput = NULL;
        return true;
    }

    // It wasn't an input so check the rest of the nodes.
    for( std::vector<magma_node_base*>::iterator it = m_nodes.begin(), itEnd = m_nodes.end(); it != itEnd; ++it ) {
        if( ( *it )->get_id() == id ) {
            std::swap( *it, m_nodes.back() );
            m_nodes.pop_back();
            return true;
        } else if( recursive ) {
            if( nodes::magma_blop_node* blop = dynamic_cast<nodes::magma_blop_node*>( *it ) ) {
                if( blop->delete_node( id ) )
                    return true;
            }
        }
    }

    return false;
}

void magma_blop_node::clear() {
    m_nodes.clear();
    this->set_num_inputs( 0 );
    this->set_num_outputs( 0 );
}

void magma_blop_node::set_output( int outputIndex, magma_interface::magma_id idNode, int socketIndex ) {
    if( m_internalOutput )
        m_internalOutput->set_input( outputIndex, idNode, socketIndex );
}

std::pair<magma_interface::magma_id, int> magma_blop_node::get_output( int outputIndex ) const {
    if( m_internalOutput )
        return m_internalOutput->get_input( outputIndex );
    return std::make_pair( magma_interface::INVALID_ID, 0 );
}

} // namespace nodes
} // namespace magma
} // namespace frantic
