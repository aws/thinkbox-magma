// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "frantic/magma/maya/maya_magma_datatypes.hpp"
#include "frantic/magma/maya/maya_magma_serializable.hpp"

namespace frantic {
namespace magma {
namespace maya {

////////////////////////////////////////////////////////////////////////////////
/// maya_magma_standard_serializer
////////////////////////////////////////////////////////////////////////////////
class maya_magma_standard_serializer : public maya_magma_serializer {
  public:
    maya_magma_standard_serializer( std::ostream& out, bool isAscii )
        : maya_magma_serializer( out, isAscii ) {}

    // {m_descNodeID, inputSocketIndex, enumAttrName, inputSocketAttrName0, inputSocketAttrName1, inputSocketAttrName2,
    // ...}
    void visit( const desc::maya_magma_desc_input_socket* e );

    // <m_descNodeID, m_nodeType>
    // (m_descNodeID, m_propertyAttrs[key], m_propertyAttrs[value])
    void visit( const desc::maya_magma_desc_node* e );

    // [m_srcDescNodeId, m_srcSocketIndex, m_dstDescNodeID, m_dstSocketIndex]
    void visit( const desc::maya_magma_desc_connection* e );

    void visit( const desc::maya_magma_desc* e );
};

////////////////////////////////////////////////////////////////////////////////
/// maya_magma_standard_deserializer
////////////////////////////////////////////////////////////////////////////////
class maya_magma_standard_deserializer : public maya_magma_deserializer {
  public:
    maya_magma_standard_deserializer( std::istream& in, std::size_t length )
        : maya_magma_deserializer( in, length ) {}

    // {m_descNodeID, inputSocketIndex, enumAttrName, inputSocketAttrName0, inputSocketAttrName1, inputSocketAttrName2,
    // ...}
    void visit( desc::maya_magma_desc_input_socket* e );

    void visit( desc::maya_magma_desc_node* e );

    void visit( desc::maya_magma_desc_connection* e );

    void visit( desc::maya_magma_desc* e );
};

} // namespace maya
} // namespace magma
} // namespace frantic
