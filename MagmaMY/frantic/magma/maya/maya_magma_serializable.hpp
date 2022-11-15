// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "frantic/magma/maya/maya_magma_datatypes.hpp"

///
/// Visitor Pattern
/// discuss the format for serializing  (XML or JSON ? )
///
namespace frantic {
namespace magma {
namespace maya {

////////////////////////////////////////////////////////////////////////////////
/// interface: maya_magma_serializer (visitor1)
////////////////////////////////////////////////////////////////////////////////
class maya_magma_serializer {
  protected:
    std::ostream& m_out;
    bool m_isAscii;

  public:
    maya_magma_serializer( std::ostream& out, bool isAscii )
        : m_out( out )
        , m_isAscii( isAscii ) {}

    virtual ~maya_magma_serializer() {}

    virtual void visit( const desc::maya_magma_desc_input_socket* e ) = 0;

    virtual void visit( const desc::maya_magma_desc_node* e ) = 0;

    virtual void visit( const desc::maya_magma_desc_connection* e ) = 0;

    virtual void visit( const desc::maya_magma_desc* e ) = 0;
};

////////////////////////////////////////////////////////////////////////////////
/// interface: deserializer (visitor2)
////////////////////////////////////////////////////////////////////////////////
class maya_magma_deserializer {
  protected:
    std::istream& m_in;
    std::size_t m_length;

  public:
    maya_magma_deserializer( std::istream& in, std::size_t length )
        : m_in( in )
        , m_length( length ) {}

    virtual ~maya_magma_deserializer() {}

    virtual void visit( desc::maya_magma_desc_input_socket* e ) = 0;

    virtual void visit( desc::maya_magma_desc_node* e ) = 0;

    virtual void visit( desc::maya_magma_desc_connection* e ) = 0;

    virtual void visit( desc::maya_magma_desc* e ) = 0;
};

////////////////////////////////////////////////////////////////////////////////
/// interface: maya_magma_serializable (a interface to be visited)
////////////////////////////////////////////////////////////////////////////////
class maya_magma_serializable {
  public:
    virtual void accept_serializer( maya_magma_serializer& serializer ) const = 0;

    virtual void accept_deserializer( maya_magma_deserializer& deserializer ) = 0;
};

} // namespace maya
} // namespace magma
} // namespace frantic
