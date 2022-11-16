// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_interface.hpp>

#if MAX_VERSION_MAJOR >= 14
#include <Animatable.h>
#endif

#undef base_type

// This Interface_ID can be used to get a pointer to a IMagmaHolder object. This is only really useful in C++, since we
// need each DLL to have a unique descriptor and therefore (I think) a unique Interface_ID. I expect implementers of
//this interface to make their own Interface_ID for use in their specific FPInterfaceDesc objects.
#define MagmaHolder_INTERFACE Interface_ID( 0x660b1727, 0x377b3eba )

// For legacy reasons some parameters are stored in the holder that shouldn't be. This refmsg is sent with a PartID that
// is the parameter ID that changed.
#define REFMSG_MAGMALEGACYCHANGE ( REFMSG_USER + 0x73ed21 )

namespace frantic {
namespace magma {
namespace max3d {

/**
 * This interface is provided for exposing the methods in magma_interface so it can be manipulated in MaxScript in a
 * consistent manner across the various Magma based products (Krakatoa, Genome, etc.)
 *
 * Magma is made up of a system of connected nodes. Each node has an ID, a type, a collection of named properties, a
 * number of inputs and a number of outputs. Nodes are primarily referred to by their ID. Each node maintains a list of
 * which other nodes its inputs are connected to (by storing the ID & output socket index). Some nodes allow default
 * constant values to be used if its input socket is left unconnected. Certain nodes (ie. BLOPS) contain other nodes.
 *
 * BLOP nodes contain a list of the nodes contained within it. 'Outside' the BLOP has a number of input and output
 * sockets like any other node. The differnce is that there is a whole network of nodes contained within the BLOP (ie.
 * the "inside") connecting the input and output sockets with any number of operations in between. The 3dsMax interface
 * to BLOPs involves maintain a stack of whih the top is the "current" BLOP. Most methods for iterating over nodes will
 * implicitly refer to the "current" BLOP's contained nodes. There is an imaginary BLOP with no ID that is considered
 * the top-level BLOP when the stack is empty.
 *
 * A BLOP maintains a set of implicit input and output nodes that are not real. There is an implcit node corresponding
 * to each input/output socket on the 'outside' of the BLOP. To get the ID to use when you want to connect a node to a
 * BLOP's imaginary input #1 you should use the special case of get_node_input( magma_interface::CURRENT_BLOP, 1 ). The
 * ID returned will  not be a node that is included when iterating over the BLOP's children. The ID of the BLOP's
 * implicit output nodes is never needed. The N'th output can be set by set_node_output( magma_interface::CURRENT_BLOP,
 * N, someID, someSocket ).
 *
 * Currently at any time you can access nodes by ID that aren't actually in the current BLOP but this may change at a
 * later date so don't rely on it. Currently the node IDs are unique at a global level, and not per-BLOP.
 */
class IMagmaHolder : public FPMixinInterface {
  public:
    typedef magma_interface::magma_id magma_id;
    typedef Tab<magma_id> magma_id_tab;
    typedef int index_type;

    virtual ~IMagmaHolder() {}

    // From FPMixinInterface
    virtual FPInterfaceDesc* GetDesc() { return GetDescByID( MagmaHolder_INTERFACE ); }

    virtual FPInterfaceDesc* GetDescByID( Interface_ID id ) = 0;

    /**
     * Get access to the underlying magma_interface object, so we can compile it for example. NOT SCRIPTER exposed
     * (obv).
     * @note This should be const in future versions.
     */
    virtual boost::shared_ptr<magma_interface> get_magma_interface() = 0;

    /**
     * Computes the time interval over which the magma holder is unchanged by animated inputs.
     * @param t The time at which to get the validity.
     * @return The validity interval of the modifier.
     */
    virtual Interval get_validity( TimeValue t ) const = 0;

    /**
     * Determines if the magma flow is read-only (ie. locked).
     * @return True if the flow cannot be changed, false otherwise.
     */
    virtual bool is_read_only() const = 0;

    /**
     * Resets the entire magma flow to the default, empty state.
     */
    virtual void reset() = 0;

    /**
     * Creates a new node with the given type and returns the ID of the new node.
     * @param typeName The name of the node type to create a new instance.
     *
     * @return magma_id The id of the new node. This will be < 0 if the node could not be created.
     */
    virtual magma_id create_node( const MCHAR* typeName ) = 0;

    /**
     * Deletes the node with the given ID (and all its contained nodes if a BLOP).
     * @param id The ID of the node to delete
     * @return False if the node could not be deleted.
     */
    virtual bool delete_node( magma_id id ) = 0;

    /**
     * Deletes the current node with idDest and changes the ID of the node with idSrc to be idDest, replacing that node.
     * All nodes that connected to the 'old' idDest will still be connected to idSrc. All nodes previously connected to
     * idSrc will point to an invalid ID (so make sure to updat those first!).
     * @param idDest The node to replace, it will be deleted if it currently exists. If this is
     * magma_interface::INVALID_ID then this is equivalent to delete_node( idSrc ).
     * @param idSrc The node whose ID we want to change in order to replace the other one. If this is
     * magma_interface::INVALID_ID then this is equivalent to delete_node( idDest ). Otherwise this must be a valid ID.
     * @return False if both idSrc and idDest were magma_interface::INVALID_ID, or ifSrc was not a valid node.
     */
    virtual bool replace_node( magma_id idDest, magma_id idSrc ) = 0;

    /**
     * Get's the name of the type of a node.
     * @param id The ID of the node to query.
     * @return The name of the type of the node with the given ID. This will be an empty string for invalid IDs.
     */
    virtual const MCHAR* get_node_type( magma_id id ) const = 0;

    /**
     * Determines if the given node contains other nodes
     * @param id The ID of the node to query.
     * @return True iff the node can contain other nodes (ie. it is eligible for PushEditableBLOP, which should really
     * be renamed).
     */
    virtual bool is_node_container( magma_id id ) const = 0;

    /**
     * Determines if the given node's type is directly creatable.
     * @param id The ID of the node to query.
     * @return True iff the node's type can be directly created. (Alternatively the node might only be creatable as a
     * side-effect of some other process. ex. Loop__Inputs node)
     */
    virtual bool is_node_creatable( magma_id id ) const = 0;

    /**
     * This struct adds a scope to the enumeration values for describing the categories of properties to return when
     * calling get_node_property_names.
     */
    struct property_type {
        enum enum_t { all, builtin, userdefined };
    };

    /**
     * @param id The ID of the node to query
     * @param builtInOnly
     * @return A collection of the names of all the node's properties.
     */
    virtual Tab<const MCHAR*> get_node_property_names( magma_id id,
                                                       TYPE_ENUM_TYPE propType = property_type::all ) const = 0;

    /**
     * Gets the type of the specified property.
     * @param id The ID of the node to query
     * @param name The name of the property to query
     * @return The type of the specified property. Will return an empty string if the node or property doesn't exist.
     */
    virtual MSTR get_node_property_type( magma_id id, const MCHAR* propName ) const = 0;

    /**
     * Determines if the specified property of a node is read-only.
     * @param id The ID of the node to query
     * @param name The name of the property to query
     * @return True if the property cannot be written to.
     */
    virtual bool get_node_property_readonly( magma_id id, const MCHAR* propName ) const = 0;

    /**
     * Adds a new property to the specified node.
     * @param id The ID of the node to modify.
     * @param propName The name of the property to add.
     * @return True if the property was added. False if it failed (possibly because the property already exists, or the
     * node was invalid).
     */
    virtual bool add_node_property( magma_id id, const MCHAR* propName ) = 0;

    /**
     * Gets the value of the specified property
     * @param id The ID of the node to query
     * @param propName The name of the property to query
     * @return The value of the property. Will contain (Value*)&undefined if the node or property doesn't exist.
     */
    virtual FPValue get_node_property( magma_id id, const MCHAR* propName ) const = 0;

    /**
     * Sets the value of the specified property.
     * @param id The ID of the node to modify
     * @param propName The name of the property to modify
     * @param val The new value to assign to the property
     * @return False if the property could not be set. This might be because the node or property don't exist, or the
     * value may not be the correct type or it may be an invalid value for this property.
     */
    virtual bool set_node_property( magma_id id, const MCHAR* propName, const FPValue& val ) = 0;

    /**
     * Returns an array of String objects that are the accepted values for this property.
     * @note This is only used by properties that report their type as "Enum".
     * @param id The ID of the node to query
     * @param propName The name of the property to query
     * @return An array of strings that are accepted values for the specified enumeration property. It will be an empty
     * array for invalid nodes/properties, or ones that are not enums.
     */
    virtual Tab<const MCHAR*> get_node_enum_values( magma_id id, const MCHAR* propName ) const = 0;

    /**
     * Gets the number of input sockets for the specified node.
     * @param id The ID of the node to query
     * @return The number of input sockets for the specified node.
     */
    virtual int get_num_node_inputs( magma_id id ) const = 0;

    /**
     * Sets the number of input sockets (if possible on the specified node).
     * @param id The ID of the node to modify
     * @param numInputs The number of input sockets the specified node should have.
     * @return False if the node could not have its socket count changed. It might not support it, or 'numInputs' might
     * be invalid.
     */
    virtual bool set_num_node_inputs( magma_id id, int numInputs ) = 0;

    /**
     * Gets the default value to be used if this socket is left unconnected. Will be undefined if this socket cannot be
     * unconnected.
     * @param id The ID of the node to query
     * @param socketIndex The socket index to get the default value of.
     * @return A default value for the socket. Will be undefined if an invalid node or socket is specified. Will also be
     * undefined if the queried socket does not support a default value.
     */
    virtual FPValue get_node_input_default_value( magma_id id, index_type socketIndex ) const = 0;

    /**
     * Sets the default value to be used if this socket us left unconnected.
     * @param id The ID of the node to modify
     * @param socketIndex The index of the socket on the node to modify.
     * @param val The new value to use as the socket's default.
     * @return False if the default value could not be set. This might be because the socket does not support default
     * values, or the supplied value is an unsupported type. It will also be false if an invalid ID or socket index is
     * specified.
     */
    virtual bool set_node_input_default_value( magma_id id, index_type socketIndex, const FPValue& val ) = 0;

    /**
     * Gets a description string for the input socket of a node.
     * @param id The ID of the node to query
     * @param socketIndex The index of the socket for the node to query.
     * @return A string describing what this socket input should be, and what the node does with it. Will be
     *         an empty string if the node or socket index is invalid.
     */
    virtual MSTR get_node_input_description( magma_id id, index_type socketIndex ) const = 0;

    /**
     * Gets the node connected to the specified node's input socket, as well as the output socket index.
     * @param id The ID of the node to query. Can specify magma_interface::CURRENT_BLOP to get at the current BLOP's
     * implicit input nodes.
     * @param index The index of the input socket to query
     * @return A pair (2-element array) containing the id of the connected node, and the index of the output socket. The
     * first element will be magma_interface::INVALID_ID if the socket is unconnected. If the socket is unconnected the
     * second element will be meaningless. Will return (Value*)&undefined if the queried node or socket do not exist.
     */
    virtual Value* get_node_input( magma_id id, index_type index ) const = 0;

    /**
     * Connects the specified node's input socket to one of the output connections of another node. Providing an
     * undefined id (-1) will set the input to unconnected.
     * @param id The ID of the node to modify
     * @param index The index of the input socket to modify
     * @param connectedID The ID of the node to connect to
     * @param connectedIndex The index of the output socket to connect to.
     * @return False if the connection failed to be made.
     */
    virtual bool set_node_input( magma_id id, index_type index, magma_id connectedID, index_type connectedIndex ) = 0;

    /**
     * Returns the number of output sockets that the specified node has
     * @param id The ID of the node to query
     * @return The number of output sockets the node has
     */
    virtual int get_num_node_outputs( magma_id id ) const = 0;

    /**
     * Sets the number of output sockets on a specified node. Most nodes do not support this.
     * @param id The ID of the node to modify
     * @param numOutputs The number of output sockets for the node to have.
     * @return True if the number of output sockets was set.
     */
    virtual bool set_num_node_outputs( magma_id id, int numOutputs ) = 0;

    /**
     * Retrieves a string description of the specified output socket of the node.
     * @param id The ID of the node to query
     * @param socketIndex The index of the output socket to query
     * @return A string description of the value produced at this output socket. Will be empty if the node or socket is
     * invalid.
     */
    virtual MSTR get_node_output_description( magma_id id, index_type socketIndex ) const = 0;

    /**
     * @return A collection of all the IDs of the nodes contained in the "current" BLOP.
     */
    virtual magma_id_tab get_nodes() const = 0;

    /**
     * @return The ID of the source node contained inside the "current" conatiner node.
     */
    virtual magma_id get_container_source() const = 0;

    /**
     * @return The ID of the sink node contained inside the "current" conatiner node.
     */
    virtual magma_id get_container_sink() const = 0;

    /**
     * Pushes the specified BLOP onto the stack and makes it the "current" BLOP.
     * @param id The ID of the BLOP to push.
     * @return False if the specified node did not exist, or was not a BLOP.
     */
    virtual bool push_editable_BLOP( magma_id id ) = 0;

    /**
     * Removes the topmost BLOP from the stack, making the next node down the "current" BLOP.
     * @return The ID of the BLOP that was popped. Will return magma_interface::INVALID_ID if there were no BLOPs to
     * pop.
     */
    virtual magma_id pop_editable_BLOP() = 0;

    /**
     * @return How many BLOPs are on the stack.
     */
    virtual int num_editing_BLOPs() const = 0;

    /**
     * @return The entire current BLOP stack
     */
    virtual magma_id_tab get_BLOP_stack() const = 0;

    /**
     * Replaces a specified BLOP with new nodes equivalent to the contents of the BLOP. The BLOP is then deleted.
     * @param id The ID of the BLOP to explode.
     * @return False if the node could not be exploded. Either the node didn't exist, or it wasn't a BLOP.
     */
    virtual bool explode_blop( magma_id id ) = 0;

    /**
     * Creates a BLOP for a collection of nodes, if possible.
     * @param nodes The collection of node IDs from which to make the BLOP.
     * @return The ID of the new BLOP. Will be magma_interface::INVALID_ID if a BLOP could not be created.
     */
    virtual magma_id create_blop( const magma_id_tab& nodes ) = 0;

    /**
     * @return A collection of the names of all the node types that can be created.
     */
    virtual Tab<const MCHAR*> get_type_names() const = 0;

    /**
     * @param The name of the type to query
     * @return The category that the node type belongs to
     */
    virtual MSTR get_type_category( const MCHAR* typeName ) const = 0;

    /**
     * @param The name of the type to query
     * @return A brief description of what nodes of the specified type do.
     */
    virtual MSTR get_type_description( const MCHAR* typeName ) const = 0;

    /**
     * Some operations behave differently when the holder is in "loading" mode.
     * @return True if the holder is loading.
     */
    virtual bool is_loading() const = 0;

    /**
     * Some operations behave differently when the holder is in "loading" mode.
     * @param loading The new loading state for the holder.
     */
    virtual void set_is_loading( bool loading = true ) = 0;

  public:
    enum {
        kFnReset,
        kFnCreateNode,
        kFnDeleteNode,
        kFnReplaceNode,
        kFnGetNodeType,
        kFnGetNodePropertyNames,
        kFnGetNodePropertyType,
        kFnAddNodeProperty,
        kFnGetNodeProperty,
        kFnSetNodeProperty,
        kFnGetNodeEnumValues,
        kFnGetNumNodeInputs,
        kFnSetNumNodeInputs,
        kFnGetNodeInputDefaultValue,
        kFnSetNodeInputDefaultValue,
        kFnGetNodeInputDescription,
        kFnGetNodeInput,
        kFnSetNodeInput,
        kFnGetNumNodeOutputs,
        kFnSetNumNodeOutputs,
        kFnGetNodeOutputDescription,
        kFnGetNodes,
        kFnPushEditableBLOP,
        kFnPopEditableBLOP,
        kFnNumEditingBLOPs,
        kFnGetBLOPStack,
        kFnExplodeBLOP,
        kFnCreateBLOP,
        kFnGetTypeNames,
        kFnGetTypeCategory,
        kFnGetTypeDescription,
        kFnGetReadOnly,
        kFnGetIsLoading,
        kFnSetIsLoading,
        kFnIsNodeContainer,         // Out of logical order due to adding it to later version
        kFnGetNodePropertyReadOnly, // Out of logical order due to adding it to later version
        kFnIsNodeCreatable,
        kFnGetContainerSource,
        kFnGetContainerSink
    };

    enum {
        kEnumPropertyType,
    };

#define TYPE_MAGMA_ID TYPE_INT
#define TYPE_MAGMA_ID_RSLT TYPE_INT_RSLT
#define TYPE_MAGMA_ID_FIELD TYPE_INT_FIELD
#define TYPE_MAGMA_ID_TAB TYPE_INT_TAB
#define TYPE_MAGMA_ID_TAB_BV TYPE_INT_TAB_BV
#define TYPE_MAGMA_ID_TAB_BV_RSLT TYPE_INT_TAB_BV_RSLT
#define TYPE_MAGMA_ID_TAB_BR TYPE_INT_TAB_BR
#define TYPE_MAGMA_ID_TAB_BR_FIELD TYPE_INT_TAB_BR_FIELD

#pragma warning( push )
#pragma warning( disable : 4238 4100 )
    BEGIN_FUNCTION_MAP
    VFN_0( kFnReset, reset )
    FN_1( kFnCreateNode, TYPE_MAGMA_ID, create_node, TYPE_STRING )
    FN_1( kFnDeleteNode, TYPE_bool, delete_node, TYPE_MAGMA_ID )
    FN_2( kFnReplaceNode, TYPE_bool, replace_node, TYPE_MAGMA_ID, TYPE_MAGMA_ID )
    FN_1( kFnGetNodeType, TYPE_STRING, get_node_type, TYPE_MAGMA_ID )
    FN_1( kFnIsNodeContainer, TYPE_bool, is_node_container, TYPE_MAGMA_ID )
    FN_1( kFnIsNodeCreatable, TYPE_bool, is_node_creatable, TYPE_MAGMA_ID )
    FN_2( kFnGetNodePropertyNames, TYPE_STRING_TAB_BV, get_node_property_names, TYPE_MAGMA_ID, TYPE_ENUM )
    FN_2( kFnGetNodePropertyType, TYPE_TSTR_BV, get_node_property_type, TYPE_MAGMA_ID, TYPE_STRING )
    FN_2( kFnGetNodePropertyReadOnly, TYPE_bool, get_node_property_readonly, TYPE_MAGMA_ID, TYPE_STRING )
    FN_2( kFnAddNodeProperty, TYPE_bool, add_node_property, TYPE_MAGMA_ID, TYPE_STRING )
    FN_2( kFnGetNodeProperty, TYPE_FPVALUE_BV, get_node_property, TYPE_MAGMA_ID, TYPE_STRING )
    FN_3( kFnSetNodeProperty, TYPE_bool, set_node_property, TYPE_MAGMA_ID, TYPE_STRING, TYPE_FPVALUE_BR )
    FN_2( kFnGetNodeEnumValues, TYPE_STRING_TAB_BV, get_node_enum_values, TYPE_MAGMA_ID, TYPE_STRING )
    FN_1( kFnGetNumNodeInputs, TYPE_INT, get_num_node_inputs, TYPE_MAGMA_ID )
    FN_2( kFnSetNumNodeInputs, TYPE_bool, set_num_node_inputs, TYPE_MAGMA_ID, TYPE_INT )
    FN_2( kFnGetNodeInputDefaultValue, TYPE_FPVALUE_BV, get_node_input_default_value, TYPE_MAGMA_ID, TYPE_INDEX )
    FN_3( kFnSetNodeInputDefaultValue, TYPE_bool, set_node_input_default_value, TYPE_MAGMA_ID, TYPE_INDEX,
          TYPE_FPVALUE_BR )
    FN_2( kFnGetNodeInputDescription, TYPE_TSTR_BV, get_node_input_description, TYPE_MAGMA_ID, TYPE_INDEX )
    FN_2( kFnGetNodeInput, TYPE_VALUE, get_node_input, TYPE_MAGMA_ID, TYPE_INDEX )
    FN_4( kFnSetNodeInput, TYPE_bool, set_node_input, TYPE_MAGMA_ID, TYPE_INDEX, TYPE_MAGMA_ID, TYPE_INDEX )
    FN_1( kFnGetNumNodeOutputs, TYPE_INT, get_num_node_outputs, TYPE_MAGMA_ID )
    FN_2( kFnSetNumNodeOutputs, TYPE_bool, set_num_node_outputs, TYPE_MAGMA_ID, TYPE_INT )
    FN_2( kFnGetNodeOutputDescription, TYPE_TSTR_BV, get_node_output_description, TYPE_MAGMA_ID, TYPE_INDEX )
    FN_0( kFnGetNodes, TYPE_MAGMA_ID_TAB_BV, get_nodes )
    FN_0( kFnGetContainerSource, TYPE_MAGMA_ID, get_container_source )
    FN_0( kFnGetContainerSink, TYPE_MAGMA_ID, get_container_sink )
    FN_1( kFnPushEditableBLOP, TYPE_bool, push_editable_BLOP, TYPE_MAGMA_ID )
    FN_0( kFnPopEditableBLOP, TYPE_MAGMA_ID, pop_editable_BLOP )
    FN_0( kFnNumEditingBLOPs, TYPE_INT, num_editing_BLOPs )
    FN_0( kFnGetBLOPStack, TYPE_MAGMA_ID_TAB_BV, get_BLOP_stack )
    FN_1( kFnExplodeBLOP, TYPE_bool, explode_blop, TYPE_MAGMA_ID )
    FN_1( kFnCreateBLOP, TYPE_MAGMA_ID, create_blop, TYPE_MAGMA_ID_TAB_BR )
    FN_0( kFnGetTypeNames, TYPE_STRING_TAB_BV, get_type_names )
    FN_1( kFnGetTypeCategory, TYPE_TSTR_BV, get_type_category, TYPE_STRING )
    FN_1( kFnGetTypeDescription, TYPE_TSTR_BV, get_type_description, TYPE_STRING )
    RO_PROP_FN( kFnGetReadOnly, is_read_only, TYPE_bool )
    PROP_FNS( kFnGetIsLoading, is_loading, kFnSetIsLoading, set_is_loading, TYPE_bool )
    END_FUNCTION_MAP
#pragma warning( pop )

  private:
    friend class MagmaHolderClassDesc;

    inline static void init_fpinterface_desc( FPInterfaceDesc& desc ) {
        desc.AppendEnum( kEnumPropertyType, 3, _T("all"), IMagmaHolder::property_type::all, _T("internal"),
                         IMagmaHolder::property_type::builtin, _T("custom"), IMagmaHolder::property_type::userdefined,
                         p_end );

        desc.AppendFunction( kFnReset, _T("Reset"), 0, TYPE_VOID, 0, 0, p_end );
        desc.AppendFunction( kFnCreateNode, _T("CreateNode"), 0, TYPE_MAGMA_ID, 0, 1, _T("TypeName"), 0, TYPE_STRING,
                             p_end );
        desc.AppendFunction( kFnDeleteNode, _T("DeleteNode"), 0, TYPE_bool, 0, 1, _T("NodeID"), 0, TYPE_MAGMA_ID,
                             p_end );
        desc.AppendFunction( kFnReplaceNode, _T("ReplaceNode"), 0, TYPE_bool, 0, 2, _T("DestID"), 0, TYPE_MAGMA_ID,
                             _T("SourceID"), 0, TYPE_MAGMA_ID, p_end );
        desc.AppendFunction( kFnGetNodeType, _T("GetNodeType"), 0, TYPE_STRING, 0, 1, _T("NodeID"), 0, TYPE_MAGMA_ID,
                             p_end );
        desc.AppendFunction( kFnIsNodeContainer, _T("IsNodeContainer"), 0, TYPE_bool, 0, 1, _T("NodeID"), 0,
                             TYPE_MAGMA_ID, p_end );
        desc.AppendFunction( kFnIsNodeCreatable, _T("IsNodeCreatable"), 0, TYPE_bool, 0, 1, _T("NodeID"), 0,
                             TYPE_MAGMA_ID, p_end );
        desc.AppendFunction( kFnGetNodePropertyNames, _T("GetNodePropertyNames"), 0, TYPE_STRING_TAB_BV, 0, 2,
                             _T("NodeID"), 0, TYPE_MAGMA_ID, _T("PropertyType"), 0, TYPE_ENUM, kEnumPropertyType,
                             p_end );
        desc.AppendFunction( kFnGetNodePropertyType, _T("GetNodePropertyType"), 0, TYPE_TSTR_BV, 0, 2, _T("NodeID"), 0,
                             TYPE_MAGMA_ID, _T("PropertyName"), 0, TYPE_STRING, p_end );
        desc.AppendFunction( kFnGetNodePropertyReadOnly, _T("GetNodePropertyReadOnly"), 0, TYPE_bool, 0, 2,
                             _T("NodeID"), 0, TYPE_MAGMA_ID, _T("PropertyName"), 0, TYPE_STRING, p_end );
        desc.AppendFunction( kFnAddNodeProperty, _T("AddNodeProperty"), 0, TYPE_bool, 0, 2, _T("NodeID"), 0,
                             TYPE_MAGMA_ID, _T("PropertyName"), 0, TYPE_STRING, p_end );
        desc.AppendFunction( kFnGetNodeProperty, _T("GetNodeProperty"), 0, TYPE_FPVALUE_BV, 0, 2, _T("NodeID"), 0,
                             TYPE_MAGMA_ID, _T("PropertyName"), 0, TYPE_STRING, p_end );
        desc.AppendFunction( kFnSetNodeProperty, _T("SetNodeProperty"), 0, TYPE_bool, 0, 3, _T("NodeID"), 0,
                             TYPE_MAGMA_ID, _T("PropertyName"), 0, TYPE_STRING, _T("Value"), 0, TYPE_FPVALUE, p_end );
        desc.AppendFunction( kFnGetNodeEnumValues, _T("GetNodeEnumValues"), 0, TYPE_STRING_TAB_BV, 0, 2, _T("NodeID"),
                             0, TYPE_MAGMA_ID, _T("PropertyName"), 0, TYPE_STRING, p_end );
        desc.AppendFunction( kFnGetNumNodeInputs, _T("GetNumNodeInputs"), 0, TYPE_INT, 0, 1, _T("NodeID"), 0,
                             TYPE_MAGMA_ID, p_end );
        desc.AppendFunction( kFnSetNumNodeInputs, _T("SetNumNodeInputs"), 0, TYPE_bool, 0, 2, _T("NodeID"), 0,
                             TYPE_MAGMA_ID, _T("NumInputs"), 0, TYPE_INT, p_end );
        desc.AppendFunction( kFnGetNodeInputDefaultValue, _T("GetNodeInputDefaultValue"), 0, TYPE_FPVALUE_BV, 0, 2,
                             _T("NodeID"), 0, TYPE_MAGMA_ID, _T("InputIndex"), 0, TYPE_INDEX, p_end );
        desc.AppendFunction( kFnSetNodeInputDefaultValue, _T("SetNodeInputDefaultValue"), 0, TYPE_bool, 0, 3,
                             _T("NodeID"), 0, TYPE_MAGMA_ID, _T("InputIndex"), 0, TYPE_INDEX, _T("Value"), 0,
                             TYPE_FPVALUE, p_end );
        desc.AppendFunction( kFnGetNodeInputDescription, _T("GetNodeInputDescription"), 0, TYPE_TSTR_BV, 0, 2,
                             _T("NodeID"), 0, TYPE_MAGMA_ID, _T("InputIndex"), 0, TYPE_INDEX, p_end );
        desc.AppendFunction( kFnGetNodeInput, _T("GetNodeInput"), 0, TYPE_VALUE, 0, 2, _T("NodeID"), 0, TYPE_MAGMA_ID,
                             _T("InputIndex"), 0, TYPE_INDEX, p_end );
        desc.AppendFunction( kFnSetNodeInput, _T("SetNodeInput"), 0, TYPE_bool, 0, 4, _T("NodeID"), 0, TYPE_MAGMA_ID,
                             _T("InputIndex"), 0, TYPE_INDEX, _T("ConnectedID"), 0, TYPE_MAGMA_ID,
                             _T("ConnectedSocket"), 0, TYPE_INDEX, p_end );
        desc.AppendFunction( kFnGetNumNodeOutputs, _T("GetNumNodeOutputs"), 0, TYPE_INT, 0, 1, _T("NodeID"), 0,
                             TYPE_MAGMA_ID, p_end );
        desc.AppendFunction( kFnSetNumNodeOutputs, _T("SetNumNodeOutputs"), 0, TYPE_bool, 0, 2, _T("NodeID"), 0,
                             TYPE_MAGMA_ID, _T("NumOutputs"), 0, TYPE_INT, p_end );
        desc.AppendFunction( kFnGetNodeOutputDescription, _T("GetNodeOutputDescription"), 0, TYPE_TSTR_BV, 0, 2,
                             _T("NodeID"), 0, TYPE_MAGMA_ID, _T("OutputIndex"), 0, TYPE_INDEX, p_end );
        desc.AppendFunction( kFnGetNodes, _T("GetNodes"), 0, TYPE_MAGMA_ID_TAB_BV, 0, 0, p_end );
        desc.AppendFunction( kFnGetContainerSource, _T("GetContainerSource"), 0, TYPE_MAGMA_ID, 0, 0, p_end );
        desc.AppendFunction( kFnGetContainerSink, _T("GetContainerSink"), 0, TYPE_MAGMA_ID, 0, 0, p_end );
        desc.AppendFunction( kFnPushEditableBLOP, _T("PushEditableBLOP"), 0, TYPE_bool, 0, 1, _T("NodeID"), 0,
                             TYPE_MAGMA_ID, p_end );
        desc.AppendFunction( kFnPopEditableBLOP, _T("PopEditableBLOP"), 0, TYPE_MAGMA_ID, 0, 0, p_end );
        desc.AppendFunction( kFnNumEditingBLOPs, _T("NumEditingBLOPs"), 0, TYPE_INT, 0, 0, p_end );
        desc.AppendFunction( kFnGetBLOPStack, _T("GetBLOPStack"), 0, TYPE_MAGMA_ID_TAB_BV, 0, 0, p_end );
        desc.AppendFunction( kFnExplodeBLOP, _T("ExplodeBLOP"), 0, TYPE_bool, 0, 1, _T("NodeID"), 0, TYPE_MAGMA_ID,
                             p_end );
        desc.AppendFunction( kFnCreateBLOP, _T("CreateBLOP"), 0, TYPE_MAGMA_ID, 0, 1, _T("NodeIDs"), 0,
                             TYPE_MAGMA_ID_TAB, p_end );
        desc.AppendFunction( kFnGetTypeNames, _T("GetTypeNames"), 0, TYPE_STRING_TAB_BV, 0, 0, p_end );
        desc.AppendFunction( kFnGetTypeCategory, _T("GetTypeCategory"), 0, TYPE_TSTR_BV, 0, 1, _T("TypeName"), 0,
                             TYPE_STRING, p_end );
        desc.AppendFunction( kFnGetTypeDescription, _T("GetTypeDescription"), 0, TYPE_TSTR_BV, 0, 1, _T("TypeName"), 0,
                             TYPE_STRING, p_end );

        desc.AppendProperty( kFnGetReadOnly, FP_NO_FUNCTION, _T("ReadOnly"), 0, TYPE_bool, p_end );
        desc.AppendProperty( kFnGetIsLoading, kFnSetIsLoading, _T("Loading"), 0, TYPE_bool, p_end );
    }
};

inline IMagmaHolder* GetMagmaHolderInterface( Animatable* anim ) {
    return anim ? (IMagmaHolder*)anim->GetInterface( MagmaHolder_INTERFACE ) : NULL;
}

} // namespace max3d
} // namespace magma
} // namespace frantic
