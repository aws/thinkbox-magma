// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <boost/make_shared.hpp>
#include <map>
#include <string>
#include <vector>

#include "frantic/convert/tstring.hpp"

#include "frantic/magma/maya/maya_magma_common.hpp"
#include "frantic/magma/maya/maya_magma_datatypes.hpp"
#include "frantic/magma/maya/maya_magma_exception.hpp"
#include "frantic/magma/maya/maya_magma_serializable.hpp"
#include "frantic/magma/maya/maya_magma_serializer.hpp"

///
/// maya_magma_desc_node & maya_magma_desc_connection do not aware of each other, all the informations are maintained by
/// maya_magma_desc class; no complicate validations will be performed inside maya_magma_desc_node since it will be
/// checked when we pass desc data to real magma flow. (therefore, it does not make sense to perform these checks again)
/// NOTICE: do not try to expose any interface because you should be able to do anything you want via the apis I
/// provided so far
///
namespace frantic {
namespace magma {
namespace maya {
namespace desc {

class maya_magma_desc_input_socket : public maya_magma_serializable {
  private:
    desc_id m_descNodeID;

    desc_index_type m_descSocketIndex;

    // the reason I am having a maya enum attributes is that sometimes input sockets can accept more than
    // one data types. we have to create maya attributes for all the data types that input socket supports;
    // in addition to that, we also create a maya enum attribute for user to select which data type the
    // input socket will be using for currently magma flow
    frantic::tstring m_enumMayaAttrName;

    std::vector<frantic::tstring> m_inputSocketMayaAttrNames;

  public:
    explicit maya_magma_desc_input_socket( desc_id descID = kInvalidDescID,
                                           desc_index_type index = kInvalidDescSocketIndex,
                                           const frantic::tstring& mayaEnumAttrName = kInvalidEnumAttrName )
        : m_descNodeID( descID )
        , m_descSocketIndex( index )
        , m_enumMayaAttrName( mayaEnumAttrName )
        , m_inputSocketMayaAttrNames() {}

    ~maya_magma_desc_input_socket() {}

    ////////////////////////////////////////
    // getter & setter
    ////////////////////////////////////////
    void set_desc_node_id( desc_id descID ) { m_descNodeID = descID; }

    void set_desc_socket_index( desc_index_type index ) { m_descSocketIndex = index; }

    desc_id get_desc_node_id() const { return m_descNodeID; }

    desc_index_type get_desc_socket_index() const { return m_descSocketIndex; }

    frantic::tstring get_maya_enum_attr_name() const { return m_enumMayaAttrName; }

    void set_maya_enum_attr_name( const frantic::tstring& mayaAttrName ) { m_enumMayaAttrName = mayaAttrName; }

    void push_maya_attr_name( const frantic::tstring& mayaAttrName ) {
        m_inputSocketMayaAttrNames.push_back( mayaAttrName );
    }

    std::vector<frantic::tstring> get_input_socket_maya_attr_names() const { return m_inputSocketMayaAttrNames; }

    ////////////////////////////////////////
    // serializing & deserializing
    ////////////////////////////////////////

    void accept_serializer( maya_magma_serializer& serializer ) const;

    void accept_deserializer( maya_magma_deserializer& deserializer );

    std::string debug() const;
};

////////////////////////////////////////////////////////////////////////////////
/// maya_magma_desc_node
////////////////////////////////////////////////////////////////////////////////
class maya_magma_desc_node : public maya_magma_serializable {
  private:
    desc_id m_descNodeID;

    frantic::tstring m_nodeType;

    // key is the name of property, the value is the corresponding maya attribute name
    std::map<frantic::tstring, frantic::tstring> m_properties;

    // key is the index of a input socket, the value is the maya_magma_desc_input_socket (contained necessary
    // information for that input socket)
    std::map<desc_index_type, maya_magma_desc_input_socket> m_inputSockets;

    // node position in window
    int m_xPosition;
    int m_yPosition;

    // Name / Description
    frantic::tstring m_name;

    // parent node (to support BLOP), -1 if at the "top"
    int m_parentNode;

  public:
    explicit maya_magma_desc_node( desc_id descNodeID = kInvalidDescID,
                                   const frantic::tstring& nodeType = kInvalidNodeType )
        : m_descNodeID( descNodeID )
        , m_nodeType( nodeType )
        , m_properties()
        , m_inputSockets()
        , m_name( nodeType )
        , m_xPosition( 0 )
        , m_yPosition( 0 )
        , m_parentNode( -1 ) {}

    ~maya_magma_desc_node() {}

    // use this with a great caution, TODO try to get rid of this
    const std::map<desc_index_type, maya_magma_desc_input_socket>& get_node_input_socket_maya_attr() const {
        return m_inputSockets;
    }

    ////////////////////////////////////////
    // setter & getter
    ////////////////////////////////////////
    void set_desc_node_id( desc_id descID ) { m_descNodeID = descID; }

    desc_id get_desc_node_id() const { return m_descNodeID; }

    void set_node_type( const frantic::tstring& typeName ) { m_nodeType = typeName; }

    frantic::tstring get_node_type() const { return m_nodeType; }

    void set_node_name( const frantic::tstring& name ) { m_name = name; }

    frantic::tstring get_node_name() const { return m_name; }

    int get_x() const { return m_xPosition; }

    int get_y() const { return m_yPosition; }

    void set_position( int x, int y ) {
        m_xPosition = x;
        m_yPosition = y;
    }

    int get_parent() const { return m_parentNode; }

    void set_parent( int id ) { m_parentNode = id; }

    ////////////////////////////////////////
    // property
    ////////////////////////////////////////

    void set_node_property_maya_attr_name( const frantic::tstring& propertyName,
                                           const frantic::tstring& mayaAttrName ) {
        m_properties[propertyName] = mayaAttrName;
    }

    frantic::tstring get_node_property_maya_attr_name( const frantic::tstring& propertyName ) const;

    bool has_node_property_name( const frantic::tstring& propertyName ) const;

    void remove_node_property( const frantic::tstring& propertyName );

    // return the all property names of a maya_magma_desc_node
    std::vector<frantic::tstring> get_node_property_names() const;

    ////////////////////////////////////////
    // input socket
    // I do not allow users passing maya_magma_desc_input_socket to maya_magma_desc_node because I would like to
    // give maya_magma_desc_node a fully control over maya_magma_desc_input_socket.
    ////////////////////////////////////////

    void set_node_input_socket_maya_enum_attr_name( desc_index_type socketIndex, const frantic::tstring& mayaAttrName );

    void push_node_input_socket_maya_attr_name( desc_index_type socketIndex, const frantic::tstring& mayaAttrName );

    std::vector<desc_index_type> get_node_socket_indexes() const;

    frantic::tstring get_node_input_socket_maya_enum_attr_name( desc_index_type socketIndex ) const;

    std::vector<frantic::tstring> get_node_input_socket_maya_attr_names( desc_index_type socketIndex ) const;

    ////////////////////////////////////////
    // serializer & deserializer
    ////////////////////////////////////////

    void accept_serializer( maya_magma_serializer& serializer ) const;

    void accept_deserializer( maya_magma_deserializer& deserializer );

    std::string debug() const;
};

////////////////////////////////////////////////////////////////////////////////
/// maya_magma_desc_connection
////////////////////////////////////////////////////////////////////////////////
class maya_magma_desc_connection : public maya_magma_serializable {
  private:
    ///   src: srcSocketID--------->dstSocketID dst
    desc_id m_srcDescNodeID;

    desc_id m_dstDescNodeID;

    // this id will be m_src's output socket index
    desc_index_type m_srcSocketIndex;

    // this id will be m_dst's input socket index
    desc_index_type m_dstSocketIndex;

  public:
    explicit maya_magma_desc_connection( desc_id src = kInvalidDescID,
                                         desc_index_type srcSocket = kInvalidDescSocketIndex,
                                         desc_id dst = kInvalidDescID,
                                         desc_index_type dstSocket = kInvalidDescSocketIndex )
        : m_srcDescNodeID( src )
        , m_dstDescNodeID( dst )
        , m_srcSocketIndex( srcSocket )
        , m_dstSocketIndex( dstSocket ) {}

    ~maya_magma_desc_connection() {}

    bool operator==( const maya_magma_desc_connection& rhs ) const {
        bool outResult = false;
        if( this->m_srcDescNodeID == rhs.m_srcDescNodeID && this->m_srcSocketIndex == rhs.m_srcSocketIndex &&
            this->m_dstDescNodeID == rhs.m_dstDescNodeID && this->m_dstSocketIndex == rhs.m_dstSocketIndex )
            outResult = true;
        return outResult;
    }

    bool operator!=( const maya_magma_desc_connection& rhs ) const { return !( operator==( rhs ) ); }

    desc_id get_src_desc_node_id() const { return m_srcDescNodeID; }

    desc_id get_dst_desc_node_id() const { return m_dstDescNodeID; }

    desc_index_type get_src_socket_index() const { return m_srcSocketIndex; }

    desc_index_type get_dst_socket_index() const { return m_dstSocketIndex; }

    void set_src_desc_node_id( desc_id src ) { m_srcDescNodeID = src; }

    void set_dst_desc_node_id( desc_id dst ) { m_dstDescNodeID = dst; }

    void set_src_socket_index( desc_index_type srcSocketIndex ) { m_srcSocketIndex = srcSocketIndex; }

    void set_dst_socket_index( desc_index_type dstSocketIndex ) { m_dstSocketIndex = dstSocketIndex; }

    ////////////////////////////////////////
    // serializer & deserializer
    ////////////////////////////////////////

    void accept_serializer( maya_magma_serializer& serializer ) const;

    void accept_deserializer( maya_magma_deserializer& deserializer );

    std::string debug() const;
};

////////////////////////////////////////////////////////////////////////////////
/// maya_magma_desc
////////////////////////////////////////////////////////////////////////////////
class maya_magma_desc : public maya_magma_serializable {
  private:
    std::map<desc_id, maya_magma_desc_node> m_nodes;

    std::vector<maya_magma_desc_connection> m_connections;

    maya_magma_serializer_t m_serializer_type;

  public:
    maya_magma_desc()
        : m_nodes()
        , m_connections()
        , m_serializer_type( MAYA_MAGMA_SERIALIZER_STANDARD ) {}

    ~maya_magma_desc() {}

    // use this with a great caution, TODO try to get rid of this
    const std::map<desc_id, maya_magma_desc_node>& get_nodes() const { return m_nodes; }

    // use this with a great caution, TODO try to get rid of this
    const std::vector<maya_magma_desc_connection>& get_connections() const { return m_connections; }

    ////////////////////////////////////////
    /// setter & getter
    ////////////////////////////////////////
    maya_magma_serializer_t get_serializer_type() const { return m_serializer_type; }

    void set_serializer_type( maya_magma_serializer_t type ) { m_serializer_type = type; }

    std::vector<frantic::tstring> get_desc_node_property_maya_attr_names( desc_id id ) const;

    std::vector<frantic::tstring> get_desc_node_property_names( desc_id id ) const;

    /// the enum attribute will also get returned
    std::vector<frantic::tstring> get_desc_node_input_socket_maya_attr_names( desc_id id ) const;

    std::vector<frantic::tstring> get_desc_node_input_socket_maya_attr_names( desc_id id, desc_index_type index ) const;

    std::vector<frantic::tstring> get_desc_node_input_socket_names( desc_id id, desc_index_type socketIndex ) const;

    frantic::tstring get_desc_node_input_socket_maya_enum_attr_name( desc_id id, desc_index_type socketIndex ) const;

    virtual std::vector<maya_magma_desc_connection> get_node_connections( desc_id id, bool getInputs = true,
                                                                          bool getOutputs = false ) const;

    ////////////////////////////////////////
    // node operation
    ////////////////////////////////////////

    virtual desc_id create_node( const frantic::tstring& typeName, int xPos = 0, int yPos = 0 );
    virtual desc_id set_node( const maya_magma_desc_node& copy );

    // delete its desc_node & all the connections associated with that desc_node
    virtual void delete_node( desc_id id );

    virtual bool contains_node( desc_id id ) const;

    ////////////////////////////////////////
    // node operation
    ////////////////////////////////////////

    virtual void set_node_position( desc_id id, int x, int y );

    virtual void get_node_position( desc_id id, int& outX, int& outY ) const;

    virtual frantic::tstring get_node_type( desc_id id ) const;

    virtual frantic::tstring get_node_name( desc_id id ) const;

    virtual void set_node_name( desc_id id, const frantic::tstring& name );

    virtual int get_node_parent( desc_id id ) const;

    virtual void set_node_parent( desc_id id, desc_id parent );

    ////////////////////////////////////////
    // node operation
    ////////////////////////////////////////

    // TODO do we validate desc_id exist in m_nodes ?  check to see whether desc_id exist in current node ?
    virtual void create_connection( desc_id srcID, desc_index_type srcSocketIndex, desc_id dstID,
                                    desc_index_type dstSocketIndex );

    // TODO delete connection where duplication connection exist in m_connections;
    virtual void delete_connection( desc_id srcID, desc_index_type srcSocketIndex, desc_id dstID,
                                    desc_index_type dstSocketIndex );

    ////////////////////////////////////////
    // property operation
    ////////////////////////////////////////

    virtual void set_node_property_maya_attr_name( desc_id id, const frantic::tstring& propertyName,
                                                   const frantic::tstring& mayaAttrName );

    virtual frantic::tstring get_maya_attr_name_from_node_property( desc_id id,
                                                                    const frantic::tstring& propertyName ) const;

    virtual void remove_node_property( desc_id id, const frantic::tstring& propertyName );

    virtual bool has_node_property_name( desc_id id, const frantic::tstring& propertyName ) const;

    ////////////////////////////////////////
    // input value
    ////////////////////////////////////////

    virtual void set_node_input_socket_maya_enum_attr_name( desc_id id, desc_index_type socketIndex,
                                                            const frantic::tstring& mayaAttrName );

    // virtual frantic::tstring get_maya_attr_name_from_desc_node_input_socket( desc_id id, desc_index_type socketIndex,
    // const frantic::tstring& propertyName ) const;

    virtual void push_node_input_socket_maya_attr_name( desc_id id, desc_index_type socketIndex,
                                                        const frantic::tstring& mayaAttrName );

    ////////////////////////////////////////
    // Maya serializable interface (maya_magma_desc_mpxdata will called these methods)
    ////////////////////////////////////////

    virtual void to_stream( std::ostream& out, bool isAscii ) const;

    virtual void from_stream( std::istream& in, std::size_t length );

    ////////////////////////////////////////
    // implement serializable interface
    ////////////////////////////////////////

    virtual void accept_serializer( maya_magma_serializer& serializer ) const;

    virtual void accept_deserializer( maya_magma_deserializer& deserializer );

    ////////////////////////////////////////
    /// for debug
    ////////////////////////////////////////

    std::string debug() const;

  protected:
    desc_id get_next_available_desc_id() const;

    maya_magma_desc_node* find_desc_id( desc_id id );

    const maya_magma_desc_node* find_desc_id( desc_id id ) const;
};

} // namespace desc
} // namespace maya
} // namespace magma
} // namespace frantic
