// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_data_type.hpp>
#include <frantic/magma/magma_interface.hpp>

namespace frantic {
namespace magma {

class magma_node_type;
class magma_node_type_impl;
class magma_compiler_interface;

class clone_callback;

// This macro declares the methods required in each concrete subclass of magma_node_base. Typically the definition is
// provided via. macro MAGMA_DEFINE_NODE in magma_node_impl.hpp create_type_definition() is used to populate a
// magma_node_type instance with metadata about this class. compile() is used to implment a visitor pattern. The node
// should call magma_compiler_interface::compile() providing itself as a pointer to its actual type.
#define MAGMA_REQUIRED_METHODS( className )                                                                            \
  public:                                                                                                              \
    static void create_type_definition( frantic::magma::magma_node_type& outType );                                    \
    virtual void compile( frantic::magma::magma_compiler_interface& compiler );

class magma_node_base {
  protected:
    magma_interface::magma_id m_id;
    const magma_node_type* m_type;
    bool m_enabled;

    friend class magma_node_type_impl;

    /**
     * A subclass overrides this function (BUT ALSO CALL THE PARENT VERSION!) to copy the data specific to the sublcass
     * during a clone operation.
     * @param dest This is the node that has been cloned, but needs its subclass specific data set from 'this'.
     * @param cb This callback is used to record any nodes that are contained in 'this' that are cloned during this
     * call. Just pass it to magma_node_base::clone() when cloning contained/owned nodes.
     */
    virtual void do_clone( magma_node_base& dest, clone_callback& cb ) const;

  public:
    magma_node_base()
        : m_id( magma_interface::INVALID_ID )
        , m_type( NULL )
        , m_enabled( true ) {}

    virtual ~magma_node_base() {}

    magma_interface::magma_id get_id() const { return m_id; }

    /**
     * Sets the node's id. This is dangerous to use, so I hope you know what you are doing. It's virtual to allow
     * base classes to capture changing IDs.
     * @param idNew The new ID to use.
     */
    virtual void set_id( magma_interface::magma_id idNew ) { m_id = idNew; }

    /**
     * @return A reference to the type descriptor object associated with this type of node.
     */
    const magma_node_type& get_type() const { return *m_type; }

    /**
     * @return True if this node is enabled, or false if its first input should be passed through since this node is
     * disabled.
     */
    const bool get_enabled() const { return m_enabled; }

    /**
     * @param enabled Whether the node should be enabled or disabled.
     */
    virtual void set_enabled( bool enabled = true ) { m_enabled = enabled; }

    /**
     * Returns a deep copy of the node
     */
    // DEPRECATED
    // virtual magma_node_base* clone() const;

    /**
     * Deep copies the node and all contained nodes, registering them (including the clone of 'this') with the supplied
     * callback object
     * @note The default implementation will copy all properties, inputs, and outputs. Anything specific to the subclass
     * must be handled by overriding do_clone(). This is non-virtual on purpose, since m_type can create a new instance
     * of the same type. The virtual function do_clone() should be overriden to handle copying subclass data.
     * @param cb A callback object that records each newly created node.
     * @return a pointer to the newly created clone of 'this'.
     */
    magma_node_base* clone( clone_callback& cb ) const;

    /**
     * @return The number of input sockets for this node.
     */
    virtual int get_num_inputs() const = 0;

    /**
     * @param numInputs The number of inputs this node will now have.
     */
    virtual void set_num_inputs( int numInputs ) = 0;

    /**
     * @param i Which input socket to query.
     * @return The node and associated output socket that is connected to this node's i'th input socket.
     */
    virtual std::pair<magma_interface::magma_id, int> get_input( int i ) const = 0;

    /**
     * @param i Which input socket to modify
     * @param id Which node to connect to this input socket.
     * @param outputIndex Which output socket in the specified node to connect to this input socket.
     */
    virtual void set_input( int i, magma_interface::magma_id id, int outputIndex = 0 ) = 0;

    /**
     * If a node's input is not connected to anything, it may have a default value to use instead.
     * @param i The index of the input to get it's default value for.
     */
    virtual const variant_t& get_input_default_value( int /*i*/ ) const {
        static variant_t defaultValue;
        return defaultValue;
    }

    /**
     * If the input specified supports a default value, this returns the expected type.
     * @param i The index of the input to get it's default value for.
     * @return The typeinfo for the type accepted in the variant_t supplied to set_input_default_value().
     */
    virtual const std::type_info& get_input_default_value_type( int /*i*/ ) const { return typeid( variant_t ); }

    /**
     * If a node's input is not connected to anything, it may have a default value to use instead. This assigns that
     * value.
     * @param i The index of the input to set it's default value for.
     * @param value The new default value.
     */
    virtual void set_input_default_value( int /*i*/, const variant_t& /*value*/ ) {
        // throw std::logic_error(""); //Removed this throw and made it just do nothing instead.
    }

    /**
     * Retrieves a description of the expected input to the i'th socket.
     * @param i Index of the socket to describe
     * @param[out] outDescription String to assign the description to.
     */
    virtual void get_input_description( int i, frantic::tstring& outDescription );

    /**
     * When traversing the node graph, some inputs should not be visited in the standard order. Override this function
     * to indicate that.
     * @param i Index of the socket to query.
     */
    virtual bool get_input_visitable( int i ) const;

    /**
     * @return The number of output sockets on this node.
     */
    virtual int get_num_outputs() const { return 1; }

    /**
     * @param numOutputs The number of output sockets this node will now have.
     */
    virtual void set_num_outputs( int /*numOutputs*/ ) {}

    /**
     * Retrieves a description of the output value created by the i'th output socket.
     * @param i Index of the socket to describe
     * @param[out] outDescription String to assign the description to.
     */
    virtual void get_output_description( int i, frantic::tstring& outDescription ) const;

    /**
     * If this node has an embedded node topology contained within it, this returns the count.
     * @return The number of nodes in the contained topology. Returns -1 if there is no contained topology.
     */
    virtual int get_num_contained_nodes() const { return -1; }

    /**
     * Returns the N'th contained node.
     * @param index Which node to retrieve from the contained node topology.
     * @return The ID of the N'th contained node.
     */
    virtual magma_node_base* get_contained_node( int index ) const {
        (void)index;
        return NULL;
    }

    /**
     * The contained node topology may expose a built-in source node. This retrieve's its ID.
     * @note This could be replaced by using a special index with get_contained_node()...
     * @return The ID of the built-in source node in the contained node topology.
     */
    virtual magma_node_base* get_contained_source() const { return NULL; }

    /**
     * The contained node topology is required to expose only a single "sink" (ie. terminating) node. This retrieves its
     * ID.
     * @note This could be replaced by using a special index with get_contained_node()...
     * @return The ID of the single sink node in the contained node topology.
     */
    virtual magma_node_base* get_contained_sink() const { return NULL; }

    /**
     * Appends a new node to the contained topology
     * @param newContainedNode The node to append
     */
    virtual void append_contained_node( magma_node_base* newContainedNode ) {
        (void)newContainedNode; // To prevent the unused parameter warning
    }

    /**
     * Removes the specified node from the contained topology.
     * @param nodeID The ID of the node to remove.
     * @return True if the node was removed. False if the node was not in the contained topology.
     */
    virtual bool remove_contained_node( magma_interface::magma_id nodeID ) {
        (void)nodeID;
        return false;
    }

    /**
     * To be implemented by the macro BEGIN_EXPOSE_TYPE in the most derived class.
     */
    virtual void compile( magma_compiler_interface& compiler ) = 0;

    /**
     * Magma nodes that are considered the "standard set" will be directly handled by the compiler via a compile(
     * specific_type* ) call. Those that are not handled by the compiler are considered "extended types" and need to
     * determine how to compile themselves for each supported compiler. This function is how that is done. The default
     * implementation is meant for nodes in the standard set so extended nodes must override it.
     *
     * Likely implementors will want to dynamic cast the compiler to a type known to them, and then use its specific
     * exposure to compile the node. If the compiler is not a known type, then this node is not supported and an
     * appropriate error should be thrown. Calling magma_node_base::compile_as_extension_type() is a reasonable way to
     * make sure the correct error is thrown.
     *
     * @param compiler The compiler currently compiling this node.
     */
    virtual void compile_as_extension_type( magma_compiler_interface& compiler );
};

/**
 * A callback class used for recording cloned nodes during a recursive clone of the node heirarchy
 */
class clone_callback {
  public:
    virtual ~clone_callback() {}

    /**
     * When cloning a heirarchy, some nodes contain other nodes so this callback is used to record the new nodes that
     * are created in addition to the ones that were directly asked to clone themselves.
     * @param newNode A reference to the newly created clone node.
     */
    virtual void register_clone( magma_node_base& newNode ) = 0;
};

} // namespace magma
} // namespace frantic
