// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"
#include <frantic/magma/nodes/magma_node_impl.hpp>
#include <frantic/magma/nodes/magma_object_query_node.hpp>

namespace frantic {
namespace magma {
namespace nodes {

MAGMA_DEFINE_TYPE( "PropertyQuery", "Object", magma_object_query_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_EXPOSE_PROPERTY( properties, std::vector<frantic::tstring> )
MAGMA_INPUT( "Objects", boost::blank() )
MAGMA_INPUT( "ObjIndex", 0 )
MAGMA_DESCRIPTION( "Extracts a collection of named properties from an object." )
MAGMA_DEFINE_TYPE_END

magma_object_query_node::magma_object_query_node() { m_properties.push_back( _T("Pos") ); }

int magma_object_query_node::get_num_outputs() const { return (int)m_properties.size(); }

void magma_object_query_node::get_output_description( int index, frantic::tstring& outDescription ) const {
    if( index >= 0 && index < static_cast<int>( m_properties.size() ) )
        outDescription = m_properties[index];
}

} // namespace nodes
} // namespace magma
} // namespace frantic
