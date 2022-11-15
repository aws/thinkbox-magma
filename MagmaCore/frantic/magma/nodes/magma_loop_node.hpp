// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_node_base.hpp>
#include <frantic/magma/nodes/magma_blop_node.hpp>
#include <frantic/magma/nodes/magma_blop_socket_node.hpp>
#include <frantic/magma/nodes/magma_node_property.hpp>

namespace frantic {
namespace magma {
namespace nodes {

class magma_container_base : public magma_node_base {
  public:
    class internal_node : public magma_node_base {
      public:
        virtual void set_parent( magma_container_base& owner ) = 0;
    };

  public:
    // Custom interface for magma_container_base
    virtual void init( magma_node_base* source, magma_node_base* sink );

    // From magma_node_base
    virtual int get_num_contained_nodes() const;

    virtual magma_node_base* get_contained_node( int index ) const;

    virtual magma_node_base* get_contained_source() const;

    virtual magma_node_base* get_contained_sink() const;

    virtual void append_contained_node( magma_node_base* newNode );

    virtual bool remove_contained_node( magma_interface::magma_id nodeID );

  protected:
    magma_container_base();

    virtual ~magma_container_base();

    // From magma_node_base
    virtual void do_clone( magma_node_base& dest, clone_callback& cb ) const;

  private:
    std::vector<magma_node_base*> m_nodes;
    internal_node *m_sourceNode, *m_sinkNode;
};

class magma_loop_node : public magma_container_base {
    std::vector<std::pair<magma_interface::magma_id, int>> m_inputs, m_outputInputs;
    std::vector<variant_t> m_inputDefaults, m_outputInputDefaults; // Parallel arrays :(
    std::vector<int> m_outputMask;

  protected:
    virtual void do_clone( magma_node_base& dest, clone_callback& cb ) const;
    virtual void do_update_output_count();

  public:
    MAGMA_REQUIRED_METHODS( magma_loop_node );
    MAGMA_PROPERTY( maxIterations, int );
    // MAGMA_PROPERTY( outputMask, std::vector<int> );

    const std::vector<int>& get_outputMask() const;
    void set_outputMask( const std::vector<int>& mask );

    const int get_numInputs() const { return static_cast<int>( m_inputs.size() ); }
    const int get_numOutputs() const { return static_cast<int>( m_outputInputs.size() ); }

    void set_numInputs( const int val );
    void set_numOutputs( const int val );

    static const frantic::tstring& get_uiType() {
        static frantic::tstring typeString( _T("Loop") );
        return typeString;
    }
    static const int get_numControlInputs() { return 0; }
    static const std::vector<frantic::tstring>& get_loopChannels() {
        static std::vector<frantic::tstring> emptyChannels;
        return emptyChannels;
    }

    magma_loop_node();

    virtual void init( magma_node_base* source, magma_node_base* sink );

    virtual int get_num_inputs() const;
    virtual std::pair<magma_interface::magma_id, int> get_input( int i ) const;
    virtual const variant_t& get_input_default_value( int i ) const;
    virtual void get_input_description( int i, frantic::tstring& outDescription );
    virtual void set_num_inputs( int numInputs );
    virtual void set_input( int i, magma_interface::magma_id id, int outputIndex );
    virtual void set_input_default_value( int i, const variant_t& value );

    virtual int get_num_outputs() const;
    virtual void set_num_outputs( int numOutputs );
    virtual void get_output_description( int i, frantic::tstring& outDescription ) const;
};

class magma_loop_inputs_node : public magma_container_base::internal_node {
  public:
    MAGMA_REQUIRED_METHODS( magma_loop_inputs_node );

    magma_loop_inputs_node();

    virtual void set_parent( magma_container_base& parent );

    virtual int get_num_inputs() const;

    virtual std::pair<magma_interface::magma_id, int> get_input( int index ) const;

    virtual void set_num_inputs( int numInputs );

    virtual void set_input( int index, magma_interface::magma_id id, int outputIndex );

    virtual int get_num_outputs() const;

    virtual void get_output_description( int i, frantic::tstring& outDescription ) const;

    // This is a hack to allow the compiler to figure out how this socket corresponds to the connected node at the
    // parent's level. It should only be used in our current hacky implementation for getting non-value inputs through
    // container levels.
    std::pair<magma_interface::magma_id, int> get_output_socket_passthrough( int i ) const;

  protected:
    magma_container_base& get_parent() const;

    magma_loop_inputs_node( std::size_t numSkippedInputs, std::size_t numInternalInputs = 0 );

  private:
    magma_container_base* m_parentNode;

    std::size_t m_numSkippedInputs;
    std::size_t m_numInternalInputs;
};

inline magma_container_base& magma_loop_inputs_node::get_parent() const { return *m_parentNode; }

class magma_loop_outputs_node : public magma_container_base::internal_node {
  public:
    MAGMA_REQUIRED_METHODS( magma_loop_outputs_node );

    magma_loop_outputs_node();

    // Sets the number of inputs on this node. Newly created sockets will connect to [defaultID, startingIndex + i]
    // where 'i' is the new socket's index.
    void update_connection_count( std::size_t numConnections, magma_interface::magma_id defaultID,
                                  int startingIndex = 0 );

    virtual void set_parent( magma_container_base& parent );

    virtual int get_num_inputs() const;

    virtual void get_input_description( int i, frantic::tstring& outDescription );

    virtual std::pair<magma_interface::magma_id, int> get_input( int index ) const;

    virtual const variant_t& get_input_default_value( int i ) const;

    virtual void set_num_inputs( int numInputs );

    virtual void set_input( int index, magma_interface::magma_id id, int outputIndex );

    virtual void set_input_default_value( int i, const variant_t& value );

    virtual int get_num_outputs() const;

  protected:
    magma_container_base& get_parent() const;

    magma_loop_outputs_node( std::size_t numExtraInputs );

    virtual void do_clone( magma_node_base& dest, clone_callback& cb ) const;

  private:
    magma_container_base* m_parentNode;

    std::size_t m_numExtraInputs;

    std::vector<std::pair<magma_interface::magma_id, int>> m_inputs;

    std::vector<variant_t> m_inputDefaults; // Parallel array to m_inputs.
};

inline magma_container_base& magma_loop_outputs_node::get_parent() const { return *m_parentNode; }

class magma_loop_channel_node : public nodes::magma_input_node {
    MAGMA_PROPERTY( channelName, frantic::tstring );
    MAGMA_PROPERTY( channelType, magma_data_type );

  public:
    MAGMA_REQUIRED_METHODS( magma_loop_channel_node );

    magma_loop_channel_node();

    virtual void get_output_description( int i, frantic::tstring& outDescription ) const;
};

} // namespace nodes
} // namespace magma
} // namespace frantic
