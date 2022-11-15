// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/magma_exception.hpp>
#include <frantic/magma/magma_interface.hpp>
#include <frantic/magma/magma_singleton.hpp>
#include <frantic/magma/nodes/magma_blop_node.hpp>
#include <frantic/magma/nodes/magma_loop_node.hpp>
#include <frantic/magma/nodes/magma_output_node.hpp>

#include <boost/scoped_ptr.hpp>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace frantic {
namespace magma {

class magma_root_node : public magma_node_base {
    std::vector<magma_node_base*> m_nodes;

  protected:
    virtual void do_clone( magma_node_base& _dest, clone_callback& cb ) const {
        magma_root_node& dest = static_cast<magma_root_node&>( _dest );

        // Do not do the base class clone here, since we don't want to notify the callback about the root node.
        // magma_node_base::do_clone( _dest, cb );

        for( std::vector<magma_node_base*>::const_iterator it = m_nodes.begin(), itEnd = m_nodes.end(); it != itEnd;
             ++it )
            dest.m_nodes.push_back( ( *it )->clone( cb ) );
    }

  public:
    magma_root_node() {}

    virtual ~magma_root_node() {}

    // From magma_node_base

    virtual int get_num_inputs() const { return 0; }

    virtual void set_num_inputs( int /*numInputs*/ ) {}

    virtual std::pair<magma_interface::magma_id, int> get_input( int /*index*/ ) const {
        return std::make_pair<int, int>( magma_interface::INVALID_ID, 0 );
    }

    virtual void set_input( int index, magma_interface::magma_id /*id*/, int /*socketIndex*/ ) {}

    virtual int get_num_outputs() const { return 0; }

    virtual void set_num_outputs( int /*numOutputs*/ ) {}

    virtual int get_num_contained_nodes() const { return static_cast<int>( m_nodes.size() ); }

    virtual magma_node_base* get_contained_node( int index ) const { return m_nodes[index]; }

    virtual void append_contained_node( magma_node_base* newNode ) { m_nodes.push_back( newNode ); }

    virtual bool remove_contained_node( magma_interface::magma_id nodeID ) {
        bool result = false;

        // Do a breadth first search. See if the node is contained directly in this node.
        for( std::vector<magma_node_base*>::iterator it = m_nodes.begin(), itEnd = m_nodes.end(); it != itEnd; ++it ) {
            if( *it != NULL && ( *it )->get_id() == nodeID ) {
                m_nodes.erase( it );
                result = true;
                break; // Gotta break out of the loop since the iterator range is now invalid.
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

    virtual void compile( magma_compiler_interface& /*compiler*/ ) {
        throw std::logic_error( "Cannot compile root node" );
    }

    void clear() { m_nodes.clear(); }
};

class magma_impl : public magma_interface {
    // std::vector< nodes::magma_blop_node* > m_blopStack;
    std::vector<magma_node_base*> m_containerStack;
    // nodes::magma_blop_node m_rootBlop;
    magma_root_node m_rootNode;
    std::vector<magma_id> m_toplevelOutputs;

    typedef std::map<magma_id, magma_node_base*> id_map_type;
    id_map_type m_idMap;

    magma_id m_nextID;

    magma_singleton* m_singleton;

    boost::uint64_t
        m_instanceID; // ID of this magma_impl instance. Defaults to std::numeric_limits<boost::uint64_t>::max();

    inline static bool is_valid_id( magma_id id ) {
        return id >= 0; // && id < m_nextID;
    }

    inline const magma_node_base& get_node_by_id( magma_id id, bool allowTopLevel = false ) const {
        if( !is_valid_id( id ) ) {
            if( id == CURRENT_EDITING && ( allowTopLevel || m_containerStack.size() > 1 ) )
                return *m_containerStack.back();
            else if( allowTopLevel && id == TOPMOST_LEVEL )
                return m_rootNode;
            throw magma_exception() << magma_exception::node_id( id )
                                    << magma_exception::error_name( _T("Not A Valid ID") );
        }
        id_map_type::const_iterator it = m_idMap.find( id );
        if( it == m_idMap.end() )
            throw magma_exception() << magma_exception::node_id( id )
                                    << magma_exception::error_name( _T("Reference To Deleted Node") );
        return *it->second;
    }

    inline magma_node_base& get_node_by_id( magma_id id, bool allowTopLevel = false ) {
        // Cast away the const because I know its ok and I want to avoid code duplication.
        return const_cast<magma_node_base&>(
            static_cast<const magma_impl*>( this )->get_node_by_id( id, allowTopLevel ) );
    }

    bool check_for_loop( magma_node_base* node, magma_id checkForID ) {
        if( node->get_id() == checkForID )
            return false;

        for( int i = 0, iEnd = node->get_num_inputs(); i < iEnd; ++i ) {
            if( magma_node_base* nextNode = get_node( node->get_input( i ).first ) ) {
                if( !check_for_loop( nextNode, checkForID ) )
                    return false;
            }
        }

        return true;
    }

    magma_id get_next_id() {
        // magma_id resultID = m_nextID;
        magma_id resultID =
            m_idMap.empty()
                ? 0
                : std::max( 0, m_idMap.rbegin()->first + 1 ); // Always pick an id slightly larger than the largest.
                                                              // This will wrap around when we've made too many nodes.

        // Search for an unused ID. m_nextID may refer to a used ID if someone has been requesting specific indices for
        // previously created nodes.
        for( id_map_type::iterator it = m_idMap.find( resultID ), itEnd = m_idMap.end(); it != itEnd;
             it = m_idMap.find( resultID ) ) {
            if( !is_valid_id( ++resultID ) )
                break;
        }

        // If we couldn't find any valid ID, so let's see if there are any unused ids between 0 and m_nextID
        if( !is_valid_id( resultID ) ) {
            for( resultID = 0; resultID < m_nextID && m_idMap.find( resultID ) == m_idMap.end(); ++resultID )
                ;

            if( resultID == m_nextID )
                throw std::runtime_error( "get_next_id() Could not find a valid id to use" );
        }

        // Record this id so we can start searching from here for the next time.
        m_nextID = resultID;

        return resultID;
    }

    void delete_output_node( nodes::magma_output_node* output ) {
        std::vector<magma_id>::iterator it =
            std::find( m_toplevelOutputs.begin(), m_toplevelOutputs.end(), output->get_id() );
        if( it != m_toplevelOutputs.end() )
            m_toplevelOutputs.erase( it );
    }

  private:
    virtual void set_singleton( magma_singleton& ms ) { m_singleton = &ms; }

  protected:
    virtual bool get_property( magma_id id, const frantic::tstring& name, const std::type_info& type, void* outVal ) {
        const magma_node_base& theNode = get_node_by_id( id );

        const property_meta_interface* propMeta = theNode.get_type().get_property_by_name( name );
        if( !propMeta )
            return false;

        if( propMeta->get_type() != type )
            throw magma_exception() << magma_exception::node_id( id ) << magma_exception::property_name( name )
                                    << magma_exception::error_name(
                                           frantic::tstring() + _T("Cannot convert from: \"") +
                                           frantic::strings::to_tstring( propMeta->get_type().name() ) +
                                           _T("\" to \"") + frantic::strings::to_tstring( type.name() ) + _T("\"") );

        propMeta->get_value( &theNode, outVal );

        return true;
    }

    virtual bool set_property( magma_id id, const frantic::tstring& name, const std::type_info& type,
                               const void* val ) {
        magma_node_base& theNode = get_node_by_id( id );

        const property_meta_interface* propMeta = theNode.get_type().get_property_by_name( name );
        if( !propMeta )
            return false;

        if( propMeta->is_readonly() )
            throw magma_exception() << magma_exception::node_id( id ) << magma_exception::property_name( name )
                                    << magma_exception::error_name( _T("Cannot Set Read-Only Property") );

        if( propMeta->get_type() != type )
            throw magma_exception() << magma_exception::node_id( id ) << magma_exception::property_name( name )
                                    << magma_exception::error_name(
                                           frantic::tstring() + _T("Cannot convert from: \"") +
                                           frantic::strings::to_tstring( type.name() ) + _T("\" to \"") +
                                           frantic::strings::to_tstring( propMeta->get_type().name() ) + _T("\"") );

        propMeta->set_value( &theNode, val );

        return true;
    }

  public:
    magma_impl()
        : m_nextID( 0 )
        , m_instanceID( std::numeric_limits<boost::uint64_t>::max() ) {
        m_containerStack.push_back( &m_rootNode );
    }

    virtual ~magma_impl() {
        for( id_map_type::iterator it = m_idMap.begin(), itEnd = m_idMap.end(); it != itEnd; ++it )
            delete it->second;
    }

    virtual magma_node_base* get_node( magma_id id ) {
        if( !is_valid_id( id ) )
            return NULL;
        id_map_type::const_iterator it = m_idMap.find( id );
        if( it == m_idMap.end() )
            return NULL;
        return it->second;
    }

    virtual magma_singleton* get_singleton() { return m_singleton; }

    virtual void set_expression_id( boost::uint64_t id ) { m_instanceID = id; }

    virtual boost::uint64_t get_expression_id() const { return m_instanceID; }

    virtual void clear() {
        m_containerStack.clear();
        m_containerStack.push_back( &m_rootNode );

        for( id_map_type::iterator it = m_idMap.begin(), itEnd = m_idMap.end(); it != itEnd; ++it )
            delete it->second;

        m_idMap.clear();
        m_rootNode.clear();
        m_toplevelOutputs.clear();

        m_nextID = 0;
    }

    virtual std::unique_ptr<magma_interface> clone() {
        class clone_impl : public clone_callback {
            magma_impl& m_newMagma;

#pragma warning( push )
#pragma warning( disable : 4822 )
            clone_impl& operator=( const clone_impl& rhs );
#pragma warning( pop )

          public:
            clone_impl( magma_impl& clonedMagma )
                : m_newMagma( clonedMagma ) {}

            virtual void register_clone( magma_node_base& clonedNode ) {
                m_newMagma.m_idMap.insert( std::make_pair( clonedNode.get_id(), &clonedNode ) );
            }
        };

        std::unique_ptr<magma_impl> result( new magma_impl );
        result->m_singleton = m_singleton;
        result->m_toplevelOutputs.assign( m_toplevelOutputs.begin(), m_toplevelOutputs.end() );
        result->m_nextID = m_nextID;
        result->m_instanceID = m_instanceID;

        // We are purposefully not copying m_blopStack. Should we?

        clone_impl cb( *result );

        for( int i = 0, iEnd = m_rootNode.get_num_contained_nodes(); i < iEnd; ++i )
            result->m_rootNode.append_contained_node( m_rootNode.get_contained_node( i )->clone( cb ) );

        return std::unique_ptr<magma_interface>( std::move( result ) );
    }

    virtual magma_id create_node( const frantic::tstring& type, magma_id requestedID = INVALID_ID ) {
        magma_id resultID = requestedID;

        // If we have requested an ID, but it is taken we pick the next available ID.
        if( !is_valid_id( resultID ) || m_idMap.find( resultID ) != m_idMap.end() )
            resultID = get_next_id();

        if( const magma_node_type* nodeType = m_singleton->get_named_node_type( type ) ) {
            if( !nodeType->is_public() &&
                nodeType->get_name() != _T("Missing") ) // Hack to allow MagmaHoldeIO to create Missing nodes. We should
                                                        // move that logic in here.
                return INVALID_ID;

            std::unique_ptr<magma_node_base> newNode( nodeType->create_instance( resultID ) );

            if( nodes::magma_output_node* outputNode = dynamic_cast<nodes::magma_output_node*>( newNode.get() ) ) {
                // Output nodes get special handling, and shouldn't be created inside of blops.
                if( m_containerStack.size() > 1 )
                    throw std::runtime_error( "Cannot create an Output node in this context" );

                m_toplevelOutputs.push_back( outputNode->get_id() );
            }

            m_idMap[newNode->get_id()] = newNode.get(); // Have to undo this if a blop throws an exception. Do it before
                                                        // processing contained nodes to claim the ID though.

            try {
                // BLOP nodes have a hidden internal node to manage input connections, and we need to assign it an id.
                // TODO: Make the socket a contained object that doesn't need to be allocated. Just grab it and set its
                // ID.
                if( nodes::magma_loop_node* loopNode = dynamic_cast<nodes::magma_loop_node*>( newNode.get() ) ) {
                    const magma_node_type* inType = m_singleton->get_named_node_type( type + _T("__Input") );
                    const magma_node_type* outType = m_singleton->get_named_node_type( type + _T("__Output") );
                    if( !inType || !outType )
                        THROW_MAGMA_INTERNAL_ERROR();

                    std::unique_ptr<nodes::magma_loop_inputs_node> inNode(
                        static_cast<nodes::magma_loop_inputs_node*>( inType->create_instance( get_next_id() ) ) );
                    m_idMap[inNode->get_id()] =
                        inNode.get(); // Don't use release() here since argument evaluation order is unefined (ie.
                                      // get_id() might get called after relase!)

                    std::unique_ptr<nodes::magma_loop_outputs_node> outNode(
                        static_cast<nodes::magma_loop_outputs_node*>( outType->create_instance( get_next_id() ) ) );
                    m_idMap[outNode->get_id()] =
                        outNode.get(); // Don't use release() here since argument evaluation order is unefined (ie.
                                       // get_id() might get called after relase!)

                    loopNode->init( inNode.release(), outNode.release() );
                } else if( nodes::magma_blop_node* blopNode = dynamic_cast<nodes::magma_blop_node*>( newNode.get() ) ) {
                    const magma_node_type* inType = m_singleton->get_named_node_type( _T("BLOPSocket") );
                    const magma_node_type* outType = m_singleton->get_named_node_type( _T("BLOPOutput") );
                    if( !inType || !outType )
                        THROW_MAGMA_INTERNAL_ERROR();

                    std::unique_ptr<nodes::magma_blop_input_node> inNode(
                        static_cast<nodes::magma_blop_input_node*>( inType->create_instance( get_next_id() ) ) );
                    m_idMap[inNode->get_id()] =
                        inNode.get(); // Don't use release() here since argument evaluation order is unefined (ie.
                                      // get_id() might get called after relase!)

                    std::unique_ptr<nodes::magma_blop_output_node> outNode(
                        static_cast<nodes::magma_blop_output_node*>( outType->create_instance( get_next_id() ) ) );
                    m_idMap[outNode->get_id()] =
                        outNode.get(); // Don't use release() here since argument evaluation order is unefined (ie.
                                       // get_id() might get called after relase!)

                    blopNode->init( inNode.release(), outNode.release() );
                }
            } catch( ... ) {
                m_idMap.erase( newNode->get_id() ); // Remove newNode, since it is about to be deleted by this
                                                    // exception.
                throw;
            }

            m_containerStack.back()->append_contained_node( newNode.get() );

            newNode.release();
        } else {
            resultID = INVALID_ID;
        }

        return resultID;
    };

    void recursive_delete( magma_node_base* node ) {
        if( !node )
            return;

        for( int i = 0, iEnd = node->get_num_contained_nodes(); i < iEnd; ++i ) {
            magma_node_base* containedNode = node->get_contained_node( i );
            if( containedNode != NULL )
                recursive_delete( containedNode );
        }

        m_idMap.erase( node->get_id() );

        delete node;
    }

    virtual void replace_node( magma_id idDest, magma_id idSrc ) {
        if( idSrc == INVALID_ID ) {
            std::unique_ptr<magma_node_base> theNodePtr( get_node( idDest ) );
            if( theNodePtr.get() != NULL ) {
                m_rootNode.remove_contained_node( idDest ); // This traverses the heirarchy removing the target node
                                                            // from the first discovered container.

                if( nodes::magma_output_node* output = dynamic_cast<nodes::magma_output_node*>( theNodePtr.get() ) )
                    delete_output_node( output );

                recursive_delete( theNodePtr.release() );
            }
        } else if( idSrc != idDest ) {
            magma_node_base* srcNode = get_node( idSrc );

            if( srcNode ) {
                std::unique_ptr<magma_node_base> theNodePtr( get_node( idDest ) );
                if( theNodePtr.get() != NULL ) {
                    m_rootNode.remove_contained_node( idDest ); // This traverses the heirarchy removing the target node
                                                                // from the first discovered container.

                    if( nodes::magma_output_node* output =
                            dynamic_cast<nodes::magma_output_node*>( theNodePtr.get() ) ) {
                        if( dynamic_cast<nodes::magma_output_node*>( srcNode ) == NULL )
                            delete_output_node( output );
                    }

                    recursive_delete( theNodePtr.release() );
                }

                srcNode->set_id( idDest );

                m_idMap[idDest] = srcNode;
                m_idMap.erase( idSrc );
            }
        }
    }

    virtual int get_num_nodes( magma_id id ) {
        magma_node_base& theNode = get_node_by_id( id, true );

        return std::max( 0, theNode.get_num_contained_nodes() );
    }

    virtual magma_id get_id( magma_id id, int index ) {
        magma_node_base& theNode = get_node_by_id( id, true );

        if( index >= 0 && index < theNode.get_num_contained_nodes() ) {
            if( magma_node_base* containedNode = theNode.get_contained_node( index ) )
                return containedNode->get_id();
        } else if( index == -1 ) {
            if( magma_node_base* sourceNode = theNode.get_contained_source() )
                return sourceNode->get_id();
        } else if( index == -2 ) {
            if( magma_node_base* sinkNode = theNode.get_contained_sink() )
                return sinkNode->get_id();
        }
        return INVALID_ID;
    }

    virtual const frantic::tstring& get_type( magma_id id ) { return get_node_by_id( id ).get_type().get_name(); }

    virtual bool is_container_node( magma_id id ) { return get_node_by_id( id ).get_type().is_container(); }

    virtual void get_description( magma_id id, frantic::tstring& outDescription ) {
        outDescription = get_node_by_id( id ).get_type().get_description();
    }

    virtual std::pair<magma_id, int> get_input( magma_id id, int index ) {
        std::pair<magma_id, int> result( INVALID_ID, 0 );

        if( id == CURRENT_EDITING ) { // This is pretty sketchy. Basically it changes the meaning of get_input().
            if( m_containerStack.size() > 1 &&
                index >= 0 /*&& index < m_blopStack.back()->get_num_inputs()*/ ) { // Removed the upper bound check for
                                                                                   // saving/loading purposes. Doesn't
                                                                                   // really cause any problems.
                magma_node_base* containedSource = m_containerStack.back()->get_contained_source();
                if( containedSource != NULL )
                    result = std::make_pair( containedSource->get_id(), index );
                // result = std::make_pair(m_blopStack.back()->get__internal_input_id(), index);
            }
        } else {
            magma_node_base& theNode = get_node_by_id( id );
            if( index >= 0 && index < theNode.get_num_inputs() )
                result = theNode.get_input( index );
        }

        // I was returning invalid_id when it was a deleted node, but it became very difficult to debug when the input
        // was aliased like that. if( is_valid_id(result.first) && m_idMap.find(result.first) == m_idMap.end() ) 	result
        //= std::make_pair<int,int>(INVALID_ID,0);

        return result;
    }

    virtual void set_input( magma_id id, int socketIndex, magma_id outputNodeId, int outputSocketIndex ) {
        magma_node_base& theNode = get_node_by_id( id );
        if( socketIndex >= 0 && socketIndex < theNode.get_num_inputs() &&
            ( is_valid_id( outputNodeId ) || outputNodeId == INVALID_ID ) && outputSocketIndex >= 0 ) {
            // If we are connecting to a valid node, make sure we aren't making a loop.
            if( magma_node_base* node = get_node( outputNodeId ) ) {
                if( !check_for_loop( node, id ) )
                    throw magma_exception()
                        << magma_exception::node_id( id ) << magma_exception::input_index( socketIndex )
                        << magma_exception::connected_id( outputNodeId )
                        << magma_exception::connected_output_index( outputSocketIndex )
                        << magma_exception::error_name( _T("Invalid Connection, Graph Cycle Detected") );
            }

            theNode.set_input( socketIndex, outputNodeId, outputSocketIndex );
        }
    }

    virtual int get_num_inputs( magma_id id ) { return get_node_by_id( id ).get_num_inputs(); }

    virtual void set_num_inputs( magma_id id, int numInputs ) {
        if( numInputs < 0 )
            numInputs = 0;

        get_node_by_id( id ).set_num_inputs( numInputs );
    }

    virtual void set_input_default_value( magma_id id, int socketIndex, const variant_t& value ) {
        magma_node_base& theNode = get_node_by_id( id );
        if( socketIndex >= 0 && socketIndex < theNode.get_num_inputs() )
            theNode.set_input_default_value( socketIndex, value );
    }

    virtual const variant_t& get_input_default_value( magma_id id, int socketIndex ) {
        magma_node_base& theNode = get_node_by_id( id );
        if( socketIndex >= 0 && socketIndex < theNode.get_num_inputs() )
            return theNode.get_input_default_value( socketIndex );

        static variant_t defaultValue;
        return defaultValue;
    }

    virtual void get_input_description( magma_id id, int i, frantic::tstring& outDescription ) {
        magma_node_base& theNode = get_node_by_id( id );

        if( i < 0 || i >= theNode.get_num_inputs() )
            return;

        theNode.get_input_description( i, outDescription );
    }

    virtual int get_num_properties( magma_id id ) { return get_node_by_id( id ).get_type().get_num_properties(); }

#pragma warning( push )
#pragma warning( disable : 4995 )
    virtual void get_property_info( magma_id id, int index, frantic::tstring& outName,
                                    const std::type_info*& outType ) {
        magma_node_base& theNode = get_node_by_id( id );

        if( index >= 0 && index < theNode.get_type().get_num_properties() ) {
            // const magma_node_type::prop_info& type = theNode.get_type().get_property_info( index );

            // outName = type.m_name;
            // outType = type.m_typeInfo;
            const property_meta_interface& propMeta = theNode.get_type().get_property( index );

            outName = propMeta.get_name();
            outType = &propMeta.get_type();
        } else {
            outType = &typeid( void );
        }
    }
#pragma warning( pop )

    virtual const frantic::tstring& get_property_name( magma_id id, int index ) {
        magma_node_base& theNode = get_node_by_id( id );

        if( index >= 0 && index < theNode.get_type().get_num_properties() )
            return theNode.get_type().get_property( index ).get_name();

        // throw std::out_of_range();
        return *(frantic::tstring*)NULL;
    }

    virtual const std::type_info& get_property_type( magma_id id, const frantic::tstring& name ) {
        magma_node_base& theNode = get_node_by_id( id );

        const property_meta_interface* propMeta = theNode.get_type().get_property_by_name( name );

        if( !propMeta )
            return typeid( void );

        return propMeta->get_type();

        /*int propIndex = theNode.get_type().get_property_index( name );
        if( propIndex < 0 )
                return typeid(void);

        return *theNode.get_type().get_property_info( propIndex ).m_typeInfo;*/
    }

    virtual bool get_property_writable( magma_id id, const frantic::tstring& name ) const {
        const magma_node_base& theNode = get_node_by_id( id );

        const property_meta_interface* propMeta = theNode.get_type().get_property_by_name( name );

        if( !propMeta )
            return false;

        return !propMeta->is_readonly();
    }

    virtual bool get_property_accepted_values( magma_id id, const frantic::tstring& name,
                                               std::vector<frantic::tstring>& outValues ) {
        magma_node_base& theNode = get_node_by_id( id );

        const property_meta_interface* propMeta = theNode.get_type().get_property_by_name( name );

        if( !propMeta )
            return false;

        propMeta->get_enumeration_values( outValues );

        return true;
    }

    virtual int get_num_outputs( magma_id id ) {
        magma_node_base& theNode = get_node_by_id( id, true );
        if( &theNode == &m_rootNode )
            return (int)m_toplevelOutputs.size();

        return theNode.get_num_outputs();
    }

    virtual void set_num_outputs( magma_id id, int numOutputs ) {
        if( numOutputs < 0 )
            numOutputs = 0;

        get_node_by_id( id, true ).set_num_outputs( numOutputs );
    }

    virtual void get_output_description( magma_id id, int i, frantic::tstring& outDescription ) {
        magma_node_base& theNode = get_node_by_id( id );

        if( i < 0 || i >= theNode.get_num_outputs() )
            return;

        theNode.get_output_description( i, outDescription );
    }

    virtual std::pair<magma_id, int> get_output( magma_id id, int index ) {
        // I'm thinking this is a bit sketchy. I should investigate why this can happen (is it the legacy way of using
        // CURRENT_BLOP to set outputs?). NOTE: Its also used in create/explode_blop

        magma_node_base& theNode = get_node_by_id( id, true );
        if( &theNode == &m_rootNode ) {
            return ( index >= 0 && index < (int)m_toplevelOutputs.size() )
                       ? std::make_pair( m_toplevelOutputs[index], 0 )
                       : std::make_pair<int, int>( INVALID_ID, 0 );
        } else {
            magma_node_base* sinkNode = theNode.get_contained_sink();
            if( sinkNode != NULL ) {
                if( index < 0 )
                    return std::make_pair( sinkNode->get_id(), 0 );
                if( sinkNode != NULL && index >= 0 && index < sinkNode->get_num_inputs() )
                    return sinkNode->get_input( index );
            }
        }

        return std::make_pair<int, int>( INVALID_ID, 0 );
    }

    // DEPRECATED
    virtual void set_output( magma_id idDest, int index, magma_id idSrc, int srcSocketIndex ) {}

    virtual void pushBLOP( magma_id id ) {
        magma_node_base& theNode = get_node_by_id( id );

        if( !theNode.get_type().is_container() )
            throw magma_exception() << magma_exception::node_id( id )
                                    << magma_exception::error_name( _T("Not a container node") );

        m_containerStack.push_back( &theNode );
    }

    virtual magma_id popBLOP() {
        magma_id result = INVALID_ID;
        if( m_containerStack.size() > 1u ) {
            result = m_containerStack.back()->get_id();
            m_containerStack.pop_back();
        }

        return result;
    }

    virtual int getBLOPStackDepth() { return static_cast<int>( m_containerStack.size() - 1 ); }

    virtual magma_id getBLOPStackAtDepth( int d ) {
        int depth = static_cast<int>( m_containerStack.size() - 1 );
        if( d < 0 || d >= depth )
            return INVALID_ID;
        return m_containerStack[depth - d]->get_id();
    }

    /**
     * Returns a pointer to the container for the specified node, as well as the index of the node within that
     * container.
     */
    std::pair<magma_node_base*, int> find_containing_node_recursive( magma_node_base* currentContainer, magma_id id ) {
        std::pair<magma_node_base*, int> result( (nodes::magma_blop_node*)NULL, -1 );

        for( int i = 0, iEnd = currentContainer->get_num_contained_nodes(); i < iEnd; ++i ) {
            if( magma_node_base* nextNode = currentContainer->get_contained_node( i ) ) {
                if( nextNode->get_id() == id ) {
                    result.first = currentContainer;
                    result.second = i;

                    break;
                } else if( nextNode->get_num_contained_nodes() > 0 ) {
                    result = find_containing_node_recursive( nextNode, id );
                    if( result.first != NULL )
                        break;
                }
            }
        }

        return result;
    }

    virtual void explode_blop( magma_id id ) {
        if( id == CURRENT_EDITING && m_containerStack.size() > 1 )
            id = m_containerStack.back()->get_id();

        if( !is_valid_id( id ) )
            return;

        // Find the BLOP in question ...
        magma_node_base* container;
        int index;

        boost::tie( container, index ) = find_containing_node_recursive( &m_rootNode, id );

        if( !container || index < 0 )
            return;

        if( nodes::magma_blop_node* blop =
                dynamic_cast<nodes::magma_blop_node*>( container->get_contained_node( index ) ) ) {
            // Fix all nodes in the container pointed at the blop
            for( int i = 0, iEnd = container->get_num_contained_nodes(); i < iEnd; ++i ) {
                magma_node_base* node = container->get_contained_node( i );
                if( !node )
                    continue;

                for( int j = 0, jEnd = node->get_num_inputs(); j < jEnd; ++j ) {
                    std::pair<magma_id, int> conn = node->get_input( j );
                    if( conn.first == id ) {
                        conn = ( conn.second >= 0 && conn.second < blop->get_num_outputs() )
                                   ? blop->get_output( conn.second )
                                   : std::make_pair<int, int>( magma_interface::INVALID_ID, 0 );

                        // A BLOP output can be connected to a BLOP input directly, so I need to handle that.
                        if( conn.first == blop->get__internal_input_id() )
                            conn = ( conn.second >= 0 && conn.second < blop->get_num_inputs() )
                                       ? blop->get_input( conn.second )
                                       : std::make_pair<int, int>( magma_interface::INVALID_ID, 0 );

                        node->set_input( j, conn.first, conn.second );
                    }
                }
            }

            // For each node in the blop, fix its inputs if necessary and move them into the parent container
            for( int i = 0, iEnd = blop->get_num_nodes(); i < iEnd; ++i ) {
                magma_node_base* node = blop->get_node( i );
                for( int j = 0, jEnd = node->get_num_inputs(); j < jEnd; ++j ) {
                    std::pair<magma_id, int> conn = node->get_input( j );
                    if( conn.first == blop->get__internal_input_id() ) {
                        conn = ( conn.second >= 0 && conn.second < blop->get_num_inputs() )
                                   ? blop->get_input( conn.second )
                                   : std::make_pair<int, int>( magma_interface::INVALID_ID, 0 );
                        node->set_input( j, conn.first, conn.second );
                    }
                }

                container->append_contained_node( node );
            }

            // Don't use the normal delete because the contained nodes are actually in two locations now!
            // delete_node( id );

            container->remove_contained_node( id );

            m_idMap.erase( id );

            delete blop;
        }
    }

    virtual magma_id create_blop( const std::set<magma_id>& nodeSet ) {
        // Validate the nodeSet entries to prevent invalid choices. You can't move Output nodes, or a container's
        // source/sink nodes into another container.
        // TODO We can also validate that all the selected nodes are actually in the (current?) parent container.
        for( std::set<magma_id>::const_iterator it = nodeSet.begin(), itEnd = nodeSet.end(); it != itEnd; ++it ) {
            magma_node_base* theNode = get_node( *it );
            if( !theNode || theNode == m_containerStack.back()->get_contained_source() ||
                theNode == m_containerStack.back()->get_contained_sink() ||
                theNode->get_type().get_name() == _T("Output") )
                return INVALID_ID; // throw std::invalid_argument( "Invalid node for create_blop" );
        }

        magma_id id = create_node( _T("BLOP") );

        typedef std::pair<magma_id, int> socket_id;
        typedef std::pair<socket_id, socket_id> connection_record;

        // Record any changed inputs in this vector in case we need to roll them back
        std::vector<connection_record> undoElements;

        try {
            nodes::magma_blop_node& blop = dynamic_cast<nodes::magma_blop_node&>( get_node_by_id( id ) );

            for( int i = 0, iEnd = m_containerStack.back()->get_num_contained_nodes(); i < iEnd; ++i ) {
                magma_node_base* node = m_containerStack.back()->get_contained_node( i );
                if( nodeSet.find( node->get_id() ) != nodeSet.end() ) {
                    // This node needs to move inside the BLOP. And if it connects to a non-internal node, a new input
                    // socket needs to be created.

                    for( int j = 0, jEnd = node->get_num_inputs(); j < jEnd; ++j ) {
                        std::pair<magma_id, int> conn = node->get_input( j );
                        if( nodeSet.find( conn.first ) == nodeSet.end() && is_valid_node( conn.first ) ) {
                            undoElements.push_back( connection_record( socket_id( node->get_id(), j ), conn ) );

                            int result = -1;
                            for( int k = 0, kEnd = blop.get_num_inputs(); k < kEnd && result < 0; ++k ) {
                                if( conn == blop.get_input( k ) )
                                    result = k;
                            }

                            if( result < 0 ) {
                                result = blop.get_num_inputs();
                                blop.set_num_inputs( result + 1 );
                                blop.set_input( result, conn.first, conn.second );
                            }

                            node->set_input( j, blop.get__internal_input_id(), result );
                        }
                    }

                    blop.append_node( node );
                } else {
                    // This node needs to update its inputs if they have moved inside the BLOP.

                    for( int j = 0, jEnd = node->get_num_inputs(); j < jEnd; ++j ) {
                        std::pair<magma_id, int> conn = node->get_input( j );
                        if( nodeSet.find( conn.first ) != nodeSet.end() ) {
                            undoElements.push_back( connection_record( socket_id( node->get_id(), j ), conn ) );

                            int result = -1;
                            for( int k = 0, kEnd = blop.get_num_outputs(); k < kEnd && result < 0; ++k ) {
                                // We are forced to have at least one output, so we need to handle sockets initialized
                                // to INVALID_ID
                                if( blop.get_output( k ).first == INVALID_ID ) {
                                    blop.set_output( k, conn.first, conn.second );
                                    result = k;
                                } else if( conn == blop.get_output( k ) )
                                    result = k;
                            }

                            if( result < 0 ) {
                                result = blop.get_num_outputs();
                                blop.set_num_outputs( result + 1 );
                                blop.set_output( result, conn.first, conn.second );
                            }

                            node->set_input( j, id, result );
                        }
                    }
                }
            } // for( int i = 0, iEnd = m_blopStack.back()->get_num_nodes(); i < iEnd; ++i )

            // Do a sanity check to make sure there are no connections from the input socket back to the node itself.
            for( int i = 0, iEnd = blop.get_num_inputs(); i < iEnd; ++i ) {
                if( magma_node_base* node = get_node( blop.get_input( i ).first ) ) {
                    if( !check_for_loop( node, blop.get_id() ) )
                        throw magma_exception()
                            << magma_exception::node_id( node->get_id() )
                            << magma_exception::error_name( _T("Unable to create BLOP due to connection loop") );
                }
            }

            // Remove all the nodes that were added to the blop from the parent container. Do this last so we don't have
            // to deal with rolling back.
            for( std::set<magma_id>::const_iterator it = nodeSet.begin(), itEnd = nodeSet.end(); it != itEnd; ++it )
                m_containerStack.back()->remove_contained_node( *it );

            return id;
        } catch( const std::exception& ) {
            // Roll back all connection changes. We have likely detected a loop, so we can't use
            // magma_interface::set_input, which will enter an infinite recursion.
            for( std::vector<connection_record>::reverse_iterator it = undoElements.rbegin(),
                                                                  itEnd = undoElements.rend();
                 it != itEnd; ++it )
                get_node_by_id( it->first.first ).set_input( it->first.second, it->second.first, it->second.second );

            // Clear the blop first, so that we don't delete the nodes that were moved into it.
            if( nodes::magma_blop_node* blop = dynamic_cast<nodes::magma_blop_node*>( get_node( id ) ) )
                blop->clear();

            // Delete the BLOP we created.
            delete_node( id );

            // Propogate the exception
            throw;
        }
    }
};

std::unique_ptr<magma_interface> create_magma_instance() { return std::unique_ptr<magma_interface>( new magma_impl ); }

} // namespace magma
} // namespace frantic
