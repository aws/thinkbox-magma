// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_any.hpp>
#include <frantic/magma/magma_data_type.hpp>

#include <boost/call_traits.hpp>

#include <memory>
#include <set>
#include <string>

namespace frantic {
namespace magma {

class magma_node_base;
class magma_singleton;

class magma_interface {
  public:
    typedef int magma_id;
    enum magma_id_enums {
        INVALID_ID = -1,      // ID to use for invalid nodes.
        CURRENT_EDITING = -2, // ID to use when retrieving the compound node currently being edited.
        TOPMOST_LEVEL = -3 // ID to use when specifying the topmost level of nodes (ie. the default level) regardless of
                           // the current editing stack.
    };

  private:
    friend class magma_singleton;

    virtual void set_singleton( magma_singleton& ms ) = 0;

  public:
    virtual ~magma_interface() {}

    /**
     * Sets an ID associated with the magma_interface instance, used to identify the origin of magma_exceptions.
     */
    virtual void set_expression_id( boost::uint64_t id ) = 0;

    /**
     * Retrieves the ID associated with this magma_interface instance via set_expression_id().
     */
    virtual boost::uint64_t get_expression_id() const = 0;

    /**
     * Removes all nodes and resets back to the intial state.
     */
    virtual void clear() = 0;

    /**
     * Clones the entire set of nodes, such that there is no shared state between *this and the object returned but the
     * are functionally equivalent.
     */
    virtual std::unique_ptr<magma_interface> clone() = 0;

    /**
     * Creates a new node of the given type and adds it to the current compound node being edited (or top level if no
     * blops are active).
     * @param type The typename of the node to create.
     * @param requestedID The id requested for this node. It may not be possible for the request to be granted.
     * @return The unique ID assigned to the node.
     */
    virtual magma_id create_node( const frantic::tstring& type, magma_id requestedID = INVALID_ID ) = 0;

    /**
     * Replaces the specified destinatio with the source node, keeping the destination id.
     * @param idDest The id of node to replace. This is the id that will remain.
     * @param idSrc The id of the node to re-label as idDest. If INVALID_ID, then the original idDest node is simply
     * deleted instead of replaced.
     */
    virtual void replace_node( magma_id idDest, magma_id idSrc ) = 0;

    /**
     * Deletes the node with the given id.
     * @param id The node to delete
     */
    inline void delete_node( magma_id id ) { replace_node( id, INVALID_ID ); }

    /**
     * Returns true if a valid node with the specified id exists
     */
    inline bool is_valid_node( magma_id id ) { return ( NULL != get_node( id ) ); }

    /**
     * @return The number of nodes contained in the specified compound node.
     */
    virtual int get_num_nodes( magma_id id ) = 0;

    /**
     * @return The id of the indexed node in the specified compound node.
     */
    virtual magma_id get_id( magma_id id, int index ) = 0;

    /**
     * @return the typename of the node with the given id.
     */
    virtual const frantic::tstring& get_type( magma_id id ) = 0;

    /**
     * @return True iff the specified node can/does contain other nodes within it.
     */
    virtual bool is_container_node( magma_id id ) = 0;

    /**
     * Gets a text description for the function of the specified node.
     * @param id The id of the node to query
     * @param[out] outDescription The node's description is stored here.
     */
    // virtual void get_description( magma_id id, frantic::tstring& outDescription ) = 0;

    /**
     * Gets the number of properties associated with the node with the given id.
     * @param id The id of the node to query
     * @return The number of properties on the given node.
     */
    virtual int get_num_properties( magma_id id ) = 0;

    /**
     * Returns the name and type of the indexed property of the specified node.
     * @param id The id of the node to query
     * @param index The index of the property within the node to query
     * @param[out] outName Will be assigned the name of the property
     * @parma[out] outType Will be assigned the type of the property
     */
    virtual void get_property_info( magma_id id, int index, frantic::tstring& outName,
                                    const std::type_info*& outType ) = 0;

#pragma deprecated( get_property_info )

    /**
     * If the specified property is a string property that only accepts specific values, this will fill a vector
     * with the different values that are accepted.
     * @param id Which node to query
     * @param name The name of the property to query
     * @param[out] outValues The various accepted values will be stored here.
     * @return Will return true iff this property does has specific values it accepts.
     */
    virtual bool get_property_accepted_values( magma_id id, const frantic::tstring& name,
                                               std::vector<frantic::tstring>& outValues ) = 0;

    /**
     * Returns the name of the I'th property defined by the node.
     * @param id The id of the node to query
     * @param index The index of the property.
     * @return The name of the indexed property.
     */
    virtual const frantic::tstring& get_property_name( magma_id id, int index ) = 0;

    /**
     * Returns the type of the named parameter.
     * @param id The id of the node to query
     * @param name The name of the property.
     * @return The type of the named property. If a property with that name is not found return typeid(void).
     */
    virtual const std::type_info& get_property_type( magma_id id, const frantic::tstring& name ) = 0;

    /**
     * Returns true if the property is able to be set. Otherwise the property is read-only and calls to set_property()
     * will fail.
     * @param id The id of the node to query
     * @param name The name of the property.
     * @return True if the property is able to be set.
     */
    virtual bool get_property_writable( magma_id id, const frantic::tstring& name ) const = 0;

    /**
     * Allows typed access to properties.
     * @tparam T The type of the property to access.
     * @param id The id of the node to access.
     * @param name The name of the property to access.
     * @param[out] outVal The property's value will be assigned to this reference using operator=().
     */
    template <class T>
    inline bool get_property( magma_id id, const frantic::tstring& name, T& outVal ) {
        return get_property( id, name, typeid( T ), &outVal );
    }

    /**
     * Allows typed access to properties.
     * @tparam T The type of the property to access.
     * @param id The id of the node to access.
     * @param name The name of the property to access.
     * @param[in] val This value will be the new value assigned to the node's property.
     */
    template <class T>
    inline bool set_property( magma_id id, const frantic::tstring& name,
                              typename boost::call_traits<T>::param_type val ) {
        return set_property( id, name, typeid( T ), &val );
    }

    /**
     * Allows generic access to properties.
     * @param id The id of the node to access.
     * @param name The name of the property to access.
     * @param[in] val This value will be the new value assigned to the node's property. It is an opaque type that
     *                can store *any* value. (Similar to boost::any)
     */
    inline bool set_property( magma_id id, const frantic::tstring& name, const magma_any& val ) {
        return set_property( id, name, val.get_type(), val.get_ptr() );
    }

    /**
     * Returns the id of the indexed input to the specified node.
     * @param id The node to get the input from. Use CURRENT_EDITING to get the id of the currently editing compound
     * node's input socket placeholder nodes.
     * @param index Which input to get.
     * @return The id and output index of the node connected to the specified input slot of the node.
     */
    virtual std::pair<magma_id, int> get_input( magma_id id, int index ) = 0;

    /**
     * Connects the indexed input slot of the node to the node specified.
     * @param id The node whose connections will be modified.
     * @param socketIndex The index of the input socket to modify.
     * @param outputNodeId The target to connect to the indexed input socketIndex.
     * @param outSocketIndex Which output socket of outputNodeId to connect to the specified input socket.
     */
    virtual void set_input( magma_id id, int socketIndex, magma_id outputNodeId, int outputSocketIndex = 0 ) = 0;

    /**
     * Gets the number of input slots associated with the node. This can change on a per-node basis for some types of
     * nodes (ie. BLOPs can have an arbitrary number).
     * @param id The node to get information on.
     * @return The number of input slots associated with the node.
     */
    virtual int get_num_inputs( magma_id id ) = 0;

    /**
     * For nodes that support it, assigns the number of input slots the node should use.
     * @param id The node to modify
     * @param numInputs The new number of inputs the node should have.
     */
    virtual void set_num_inputs( magma_id id, int numInputs ) = 0;

    /**
     * Set the default value for a node to use if a input is not connected to a given socket.
     * @param id The id of the node to modify
     * @param socketIndex The index of the input socket to provide a default value for.
     * @param value The value to use as a default.
     */
    virtual void set_input_default_value( magma_id id, int socketIndex, const variant_t& value ) = 0;

    /**
     * Gets the default value for a specific input socket of a node.
     * @param id The id of the node to retrieve the default from
     * @param socketIndex The index of the input socket to retrieve its default value from.
     */
    virtual const variant_t& get_input_default_value( magma_id id, int socketIndex ) = 0;

    /**
     * Gets a string description for the specified input for display to the user.
     * @param id The id of the node to retrieve the description from
     * @param socketIndex The index of the input socket to retrieve its description from
     * @param[out] outDescription The resulting description is stored here.
     */
    virtual void get_input_description( magma_id id, int i, frantic::tstring& outDescription ) = 0;

    /**
     * Returns the number of outputs for the specified node. Use CURRENT_BLOP to get the active BLOP's number of
     * outputs. If no BLOPs are active, it returns the equivalent of specifying TOPMOST_LEVEL.
     * @note This will currently always be 1.
     * @return The number of outputs in the specified node.
     */
    virtual int get_num_outputs( magma_id id ) = 0;

    /**
     * Sets the number of outputs for specified node. Use CURRENT_BLOP to set the active BLOP's number of outputs. If no
     * BLOPs are active it does nothing.
     * @note Currently does nothing.
     */
    virtual void set_num_outputs( magma_id id, int numOutputs ) = 0;

    /**
     * Gets a string description for the specified output for display to the user.
     * @param id The id of the node to retrieve the description from
     * @param socketIndex The index of the output socket to retrieve its description from
     * @param[out] outDescription The resulting description is stored here.
     */
    virtual void get_output_description( magma_id id, int i, frantic::tstring& outDescription ) = 0;

    /**
     * Returns the node in the specified output slot of the selected BLOP, or the topmost node level if TOPMOST_LEVEL or
     * CURRENT_BLOP and no active blops.
     * @note Currently only a single output is supported.
     * @param index Which output to retrieve
     * @return The id of the output node.
     */
    virtual std::pair<magma_id, int> get_output( magma_id id, int index ) = 0;

    /**
     * Sets the indexed output of the specified node. Use CURRENT_BLOP to affect the currently editing BLOP or
     * TOPMOST_LEVEL to affect the topmost level.
     * @param index Which output to connect
     * @param id The node to connect to the output slot.
     */
    virtual void set_output( magma_id idDest, int index, magma_id idSrc, int srcSocketIndex ) = 0;

    /**
     * Selects a new blop to be affected by proceding calls. Will be removed by a matching call to popBLOP().
     * @param id The id of blop to push.
     */
    virtual void pushBLOP( magma_id id ) = 0;

    /**
     * Removes the last blop pushed via pushBLOP().
     * @return The id of the item removed.
     */
    virtual magma_id popBLOP() = 0;

    /**
     * @return The number of blops currently pushed via pushBLOP().
     */
    virtual int getBLOPStackDepth() = 0;

    /**
     * @return The id of the blop at depth 'depth'. 0 Is the currently editing blop.
     */
    virtual magma_id getBLOPStackAtDepth( int depth ) = 0;

    /**
     * Takes an existing BLOP and replaces it with all the nodes that were previously contained within it.
     * @param id The id of the BLOP to explode
     */
    virtual void explode_blop( magma_id id ) = 0;

    /**
     * Creates a new BLOP from a collection of nodes, automatically fixing replacing connection to the contained nodes
     * with output and inputs to the BLOP
     * @param fromNodes The collection of nodes that will be contained in the new BLOP
     * @return The ID of the new BLOP node.
     */
    virtual magma_id create_blop( const std::set<magma_id>& fromNodes ) = 0;

  protected:
    /**
     * Provides type-safe but generic retrieval of the node's properties.
     * @param id The node to access.
     * @param name The name of the parameter to access.
     * @param inType The type_info of actual type of the data pointed at by inoutVal.
     * @param inoutVal A pointer to a variable to set via operator=() with the property's stored value.
     */
    virtual bool get_property( magma_id id, const frantic::tstring& name, const std::type_info& inType,
                               void* inoutVal ) = 0;

    /**
     * Provides type-safe but generic setting of the node's properties.
     * @param id The node to access.
     * @param name The name of the parameter to access.
     * @param inType The type_info of actual type of the data pointed at by inVal.
     * @param inoutVal A pointer to the new value to use for the property. Assigned via operator=().
     */
    virtual bool set_property( magma_id id, const frantic::tstring& name, const std::type_info& inType,
                               const void* inVal ) = 0;

  public:
    // For internal use only. Do not touch.
    virtual magma_node_base* get_node( magma_id id ) = 0;

    virtual magma_singleton* get_singleton() = 0;
};

} // namespace magma
} // namespace frantic
