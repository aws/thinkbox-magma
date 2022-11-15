// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/nodes/magma_loop_node.hpp>
#include <frantic/magma/nodes/magma_node_impl.hpp>

namespace frantic {
namespace magma {
namespace nodes {

magma_container_base::magma_container_base()
    : m_sourceNode( NULL )
    , m_sinkNode( NULL ) {}

magma_container_base::~magma_container_base() {}

void magma_container_base::do_clone( magma_node_base& _dest, clone_callback& cb ) const {
    magma_container_base& dest = static_cast<magma_container_base&>( _dest );

    dest.m_sourceNode = static_cast<internal_node*>( m_sourceNode ? m_sourceNode->clone( cb ) : NULL );
    dest.m_sourceNode->set_parent( dest );

    dest.m_sinkNode = static_cast<internal_node*>( m_sinkNode ? m_sinkNode->clone( cb ) : NULL );
    dest.m_sinkNode->set_parent( dest );

    // Calling init was doing some default initialization that should be avoided during cloning.
    /*dest.init(
            m_sourceNode ? m_sourceNode->clone( cb ) : NULL,
            m_sinkNode ? m_sinkNode->clone( cb ) : NULL
    );*/

    magma_node_base::do_clone( _dest, cb );

    for( std::vector<magma_node_base*>::const_iterator it = m_nodes.begin(), itEnd = m_nodes.end(); it != itEnd; ++it )
        dest.m_nodes.push_back( ( *it )->clone( cb ) );
}

void magma_container_base::init( magma_node_base* source, magma_node_base* sink ) {
    m_sourceNode = dynamic_cast<internal_node*>( source );
    m_sinkNode = dynamic_cast<internal_node*>( sink );

    if( m_sourceNode )
        m_sourceNode->set_parent( *this );
    if( m_sinkNode )
        m_sinkNode->set_parent( *this );
}

int magma_container_base::get_num_contained_nodes() const { return 2 + static_cast<int>( m_nodes.size() ); }

magma_node_base* magma_container_base::get_contained_node( int index ) const {
    if( index == 0 )
        return m_sourceNode;
    else if( index == 1 )
        return m_sinkNode;
    else
        return m_nodes[index - 2];
}

magma_node_base* magma_container_base::get_contained_source() const { return m_sourceNode; }

magma_node_base* magma_container_base::get_contained_sink() const { return m_sinkNode; }

void magma_container_base::append_contained_node( magma_node_base* newNode ) { m_nodes.push_back( newNode ); }

bool magma_container_base::remove_contained_node( magma_interface::magma_id nodeID ) {
    bool result = false;

    // Do a breadth first search. See if the node is contained directly in this node.
    for( std::vector<magma_node_base*>::iterator it = m_nodes.begin(), itEnd = m_nodes.end(); it != itEnd && !result;
         /*++it*/ ) {
        if( *it != NULL && ( *it )->get_id() == nodeID ) {
            it = m_nodes.erase( it );
            result = true;
        } else {
            ++it;
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

MAGMA_DEFINE_TYPE( "Loop", "Loop", magma_loop_node )
MAGMA_TYPE_ATTR( disableable,
                 false ) // TODO: I think this might make sense to disable. Just pass all the inputs through directly.
MAGMA_DESCRIPTION( "Runs an operation in a loop until a condition becomes false" )
MAGMA_TYPE_ATTR( container, true )
MAGMA_EXPOSE_PROPERTY( maxIterations, int )
MAGMA_EXPOSE_PROPERTY( outputMask, std::vector<int> )
MAGMA_EXPOSE_PROPERTY( numInputs, int )
MAGMA_EXPOSE_PROPERTY( numOutputs, int )
MAGMA_READONLY_PROPERTY( uiType, frantic::tstring )
MAGMA_READONLY_PROPERTY( numControlInputs, int )
MAGMA_READONLY_PROPERTY( loopChannels, std::vector<frantic::tstring> )
// MAGMA_HIDDEN_READONLY_PROPERTY( _internal_input_id, magma_interface::magma_id )
// MAGMA_HIDDEN_READONLY_PROPERTY( _internal_output_id, magma_interface::magma_id )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Loop__Input", "System", magma_loop_inputs_node )
MAGMA_TYPE_ATTR( public, false );
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Loop__Output", "System", magma_loop_outputs_node )
MAGMA_TYPE_ATTR( public, false );
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "LoopChannel", "Input", magma_loop_channel_node )
MAGMA_DESCRIPTION( "Accesses an input channel for the current iteration of a loop" )
MAGMA_EXPOSE_PROPERTY( channelName, frantic::tstring )
MAGMA_DEFINE_TYPE_END

magma_loop_node::magma_loop_node()
    : m_maxIterations( 1000 ) {}

void magma_loop_node::do_clone( magma_node_base& dest, clone_callback& cb ) const {
    // magma_blop_node::do_clone( dest, cb );
    magma_container_base::do_clone( dest, cb );
}

void magma_loop_node::do_update_output_count() {
    // Make sure its not null. This can happen during do_clone().
    // if( magma_blop_output_node* outputNode = this->get_internal_output() )
    //	static_cast<magma_loop_outputs_node*>(outputNode)->update_connection_count( m_outputMask.size() );

    if( magma_node_base* sinkNode = this->get_contained_sink() )
        // NOTE: This needs to be modified to account for any extra internal sockets provided by the implementation
        // node.
        static_cast<magma_loop_outputs_node*>( sinkNode )
            ->update_connection_count( m_outputInputs.size(), this->get_contained_source()->get_id(),
                                       1 + static_cast<int>( m_inputs.size() ) );
}

void magma_loop_node::set_numInputs( int val ) {
    std::size_t oldSize = m_inputs.size();

    m_inputs.resize( static_cast<std::size_t>( std::max( 0, val ) ), std::make_pair( magma_interface::INVALID_ID, 0 ) );
    m_inputDefaults.resize( m_inputs.size() );

    m_outputMask.clear();

    for( std::size_t i = 0, iEnd = m_outputInputs.size(); i < iEnd; ++i )
        m_outputMask.push_back( static_cast<int>( i + m_inputs.size() ) );

    int nSkipSockets = 1; // Hardcoded for now. This is the number of sockets on the source that aren't affected by
                          // numInputs & numOutputs.

    // Since out 'input' sockets are always before our 'outputInput' sockets, changing the count here will affect the
    // socket indices of anything connected to an 'outputInput' socket. So we loop through the container fixing up any
    // problems.
    //
    // TODO We don't fix connections for anything else. Why start here? Or perhaps we can make a more general event for
    // when output indices change or disappear.
    if( m_inputs.size() < oldSize ) {
        // We removed sockets in the middle, so we need to re-index anything that was pointing at the outputInput
        // sockets. We also disconnect anything that was connected to a removed socket. TODO Should we be doing that?

        magma_interface::magma_id sourceID = this->get_contained_source()->get_id();

        for( int i = 0, iEnd = this->get_num_contained_nodes(); i < iEnd; ++i ) {
            magma_node_base* node = this->get_contained_node( i );
            if( !node )
                continue;

            for( int j = 0, jEnd = node->get_num_inputs(); j < jEnd; ++j ) {
                std::pair<magma_interface::magma_id, int> input = node->get_input( j );
                if( input.first == sourceID ) { // We ignore the first input as the iteration index socket.
                    if( input.second - nSkipSockets >=
                        static_cast<int>(
                            oldSize ) ) // We need to re-index these connections since the output index changed
                        node->set_input( j, sourceID, input.second - static_cast<int>( oldSize - m_inputs.size() ) );
                    else if( input.second - nSkipSockets >=
                             static_cast<int>( m_inputs.size() ) ) // We need to disconnect these because the socket the
                                                                   // connected to is gone.
                        node->set_input( j, magma_interface::INVALID_ID, 0 );
                }
            }
        }
    } else if( m_inputs.size() > oldSize &&
               m_outputInputs.size() > 0 ) { // Added 'm_outputInputs.size() > 0' to prevent fixing when loading
        // We added sockets in the middle, so we need to re-index anything that was pointing at the outputInput sockets

        magma_interface::magma_id sourceID = this->get_contained_source()->get_id();

        for( int i = 0, iEnd = this->get_num_contained_nodes(); i < iEnd; ++i ) {
            magma_node_base* node = this->get_contained_node( i );
            if( !node )
                continue;

            for( int j = 0, jEnd = node->get_num_inputs(); j < jEnd; ++j ) {
                std::pair<magma_interface::magma_id, int> input = node->get_input( j );
                if( input.first == sourceID &&
                    input.second - nSkipSockets >=
                        static_cast<int>(
                            oldSize ) ) // We need to re-index these connections since the output index changed
                    node->set_input( j, sourceID, input.second + static_cast<int>( m_inputs.size() - oldSize ) );
            }
        }
    }
}

void magma_loop_node::set_numOutputs( int val ) {
    std::size_t oldSize = m_outputInputs.size();

    m_outputInputs.resize( static_cast<std::size_t>( std::max( 0, val ) ),
                           std::make_pair( magma_interface::INVALID_ID, 0 ) );
    m_outputInputDefaults.resize( m_outputInputs.size() );

    m_outputMask.clear();

    for( std::size_t i = 0, iEnd = m_outputInputs.size(); i < iEnd; ++i )
        m_outputMask.push_back( static_cast<int>( i + m_inputs.size() ) );

    this->do_update_output_count();

    if( oldSize > m_outputInputs.size() ) {
        // We are removing some sockets, so go through the node list and disconnect any nodes that were connected to a
        // socket we are deleting.

        magma_interface::magma_id sourceID = this->get_contained_source()->get_id();

        int nSkipSockets = 1; // Hardcoded for now. This is the number of sockets on the source that aren't affected by
                              // numInputs & numOutputs.
        int maxNewIndex = nSkipSockets + static_cast<int>( m_outputInputs.size() + m_inputs.size() );

        for( int i = 0, iEnd = this->get_num_contained_nodes(); i < iEnd; ++i ) {
            magma_node_base* node = this->get_contained_node( i );
            if( !node )
                continue;

            for( int j = 0, jEnd = node->get_num_inputs(); j < jEnd; ++j ) {
                std::pair<magma_interface::magma_id, int> input = node->get_input( j );
                if( input.first == sourceID &&
                    input.second >=
                        maxNewIndex ) // We need to re-index these connections since the output index changed
                    node->set_input( j, magma_interface::INVALID_ID, 0 );
            }
        }
    }
}

const std::vector<int>& magma_loop_node::get_outputMask() const { return m_outputMask; }

void magma_loop_node::set_outputMask( const std::vector<int>& mask ) {
    // m_outputMask.clear();
    //
    // std::vector<int> newMask;

    //// Add each int to the list, stripping out duplicates.
    //// TODO Should this be ordered?
    // for( std::vector<int>::const_iterator it = mask.begin(), itEnd = mask.end(); it != itEnd; ++it ){
    //	if( *it >= 0 && std::find( m_outputMask.begin(), m_outputMask.end(), *it ) == m_outputMask.end() )
    //		newMask.push_back( *it );
    // }

    // m_outputMask.swap( newMask );

    // this->do_update_output_count();
}

void magma_loop_node::init( magma_node_base* source, magma_node_base* sink ) {
    magma_container_base::init( source, sink );

    this->set_numOutputs( 1 );
}

int magma_loop_node::get_num_inputs() const { return static_cast<int>( m_inputs.size() + m_outputInputs.size() ); }

void magma_loop_node::set_num_inputs( int /*numInputs*/ ) {}

std::pair<magma_interface::magma_id, int> magma_loop_node::get_input( int _i ) const {
    std::size_t i = static_cast<std::size_t>( _i );

    if( i < m_inputs.size() )
        return m_inputs[i];
    return m_outputInputs[i - m_inputs.size()];
}

const variant_t& magma_loop_node::get_input_default_value( int _i ) const {
    std::size_t i = static_cast<std::size_t>( _i );

    if( i < m_inputDefaults.size() )
        return m_inputDefaults[i];
    return m_outputInputDefaults[i - m_inputDefaults.size()];
}

void magma_loop_node::set_input_default_value( int _i, const variant_t& value ) {
    std::size_t i = static_cast<std::size_t>( _i );

    if( i < m_inputDefaults.size() )
        m_inputDefaults[i] = value;
    else
        m_outputInputDefaults[i - m_inputDefaults.size()] = value;
}

void magma_loop_node::set_input( int _i, magma_interface::magma_id id, int outputIndex ) {
    std::size_t i = static_cast<std::size_t>( _i );

    if( i < m_inputs.size() )
        m_inputs[i] = std::make_pair( id, outputIndex );
    else
        m_outputInputs[i - m_inputs.size()] = std::make_pair( id, outputIndex );
}

void magma_loop_node::get_input_description( int i, frantic::tstring& outDescription ) {
    if( i < static_cast<int>( m_inputs.size() ) )
        outDescription = _T("Accumulator ") + boost::lexical_cast<frantic::tstring>( i + 1 );
    else
        outDescription =
            _T("Accumulator ") + boost::lexical_cast<frantic::tstring>( i - static_cast<int>( m_inputs.size() ) + 1 );
}

int magma_loop_node::get_num_outputs() const { return static_cast<int>( m_outputInputs.size() ); }

void magma_loop_node::set_num_outputs( int /*numOutputs*/ ) {}

void magma_loop_node::get_output_description( int i, frantic::tstring& outDescription ) const {
    const_cast<magma_loop_node*>( this )->get_input_description(
        i + static_cast<int>( m_inputs.size() ), outDescription ); // Const cast because of sloppy design :(
}

magma_loop_inputs_node::magma_loop_inputs_node()
    : m_parentNode( NULL )
    , m_numSkippedInputs( 0 )
    , m_numInternalInputs( 1 ) {}

magma_loop_inputs_node::magma_loop_inputs_node( std::size_t numSkippedInputs, std::size_t numInternalInputs )
    : m_parentNode( NULL )
    , m_numSkippedInputs( numSkippedInputs )
    , m_numInternalInputs( numInternalInputs + 1 ) // Add one for iteration index
{}

void magma_loop_inputs_node::set_parent( magma_container_base& parent ) { m_parentNode = &parent; }

int magma_loop_inputs_node::get_num_inputs() const { return 0; }

std::pair<magma_interface::magma_id, int> magma_loop_inputs_node::get_input( int index ) const {
    return std::make_pair( magma_interface::INVALID_ID, 0 );
}

void magma_loop_inputs_node::set_num_inputs( int /*numInputs*/ ) {}

void magma_loop_inputs_node::set_input( int /*index*/, magma_interface::magma_id /*id*/, int /*outputIndex*/ ) {}

int magma_loop_inputs_node::get_num_outputs() const {
    return m_parentNode ? m_parentNode->get_num_inputs() + static_cast<int>( m_numInternalInputs ) -
                              static_cast<int>( m_numSkippedInputs )
                        : 1; // Add one for the iteration index, then skip some inputs from the parent.
}

void magma_loop_inputs_node::get_output_description( int i, frantic::tstring& outDescription ) const {
    if( i == 0 ) {
        outDescription = _T("Iteration Index");
    } else if( i < m_numInternalInputs ) {
        outDescription = _T("");
    } else {
        get_parent().get_input_description( i - static_cast<int>( m_numInternalInputs ) +
                                                static_cast<int>( m_numSkippedInputs ),
                                            outDescription ); // Const cast to work around poor const-ness.
    }
}

std::pair<magma_interface::magma_id, int> magma_loop_inputs_node::get_output_socket_passthrough( int i ) const {
    if( i >= m_numInternalInputs ) // Skip IterationIndex
        return get_parent().get_input( i - static_cast<int>( m_numInternalInputs ) +
                                       static_cast<int>( m_numSkippedInputs ) );
    return std::make_pair( magma_interface::INVALID_ID, 0 );
}

magma_loop_outputs_node::magma_loop_outputs_node()
    : m_parentNode( NULL )
    , m_numExtraInputs( 1 ) {
    m_inputs.resize( m_numExtraInputs, std::make_pair( magma_interface::INVALID_ID, 0 ) );
    m_inputDefaults.push_back( false );
    m_inputDefaults.resize( m_inputs.size() );
}

magma_loop_outputs_node::magma_loop_outputs_node( std::size_t numExtraInputs )
    : m_parentNode( NULL )
    , m_numExtraInputs( numExtraInputs ) {
    m_inputs.resize( m_numExtraInputs, std::make_pair( magma_interface::INVALID_ID, 0 ) );
}

void magma_loop_outputs_node::do_clone( magma_node_base& dest, clone_callback& cb ) const {
    // We need to set the expected number of inputs here directly, since set_num_inputs() is disabled for this node.
    static_cast<magma_loop_outputs_node&>( dest ).m_inputs.assign( m_inputs.size(),
                                                                   std::make_pair( magma_interface::INVALID_ID, 0 ) );

    internal_node::do_clone( dest, cb );
}

void magma_loop_outputs_node::update_connection_count( std::size_t numConnections, magma_interface::magma_id defaultID,
                                                       int startingIndex ) {
    std::size_t oldSize = m_inputs.size();

    m_inputs.resize( numConnections + m_numExtraInputs,
                     std::make_pair( magma_interface::INVALID_ID, 0 ) ); // Add the 'm_numExtraInputs'.
    m_inputDefaults.resize( m_inputs.size() );

    for( std::size_t i = oldSize, iEnd = m_inputs.size(); i < iEnd; ++i ) {
        m_inputs[i].first = defaultID;
        m_inputs[i].second = startingIndex + static_cast<int>( i - m_numExtraInputs );
    }
}

void magma_loop_outputs_node::set_parent( magma_container_base& parent ) { m_parentNode = &parent; }

int magma_loop_outputs_node::get_num_inputs() const { return static_cast<int>( m_inputs.size() ); }

void magma_loop_outputs_node::get_input_description( int i, frantic::tstring& outDescription ) {
    if( i < static_cast<int>( m_numExtraInputs ) )
        outDescription = _T("Condition"); // TODO Replace this with some sort of virtual function call.
    else if( m_parentNode ) {
        m_parentNode->get_output_description( i - static_cast<int>( m_numExtraInputs ), outDescription );
    } else {
        internal_node::get_input_description( i, outDescription );
    }
}

void magma_loop_outputs_node::set_num_inputs( int /*numInputs*/ ) {} // Cannot directly set the number of inputs.

std::pair<magma_interface::magma_id, int> magma_loop_outputs_node::get_input( int index ) const {
    return m_inputs[index];
}

const variant_t& magma_loop_outputs_node::get_input_default_value( int i ) const { return m_inputDefaults[i]; }

void magma_loop_outputs_node::set_input( int index, magma_interface::magma_id id, int outputIndex ) {
    m_inputs[index] = std::make_pair( id, outputIndex );
}

void magma_loop_outputs_node::set_input_default_value( int i, const variant_t& value ) { m_inputDefaults[i] = value; }

int magma_loop_outputs_node::get_num_outputs() const { return 0; }

magma_loop_channel_node::magma_loop_channel_node()
    : m_channelName( _T("<undefined>") )
    , m_channelType() {}

void magma_loop_channel_node::get_output_description( int i, frantic::tstring& outDescription ) const {
    if( i == 0 )
        outDescription = get_channelName();
}

} // namespace nodes
} // namespace magma
} // namespace frantic
