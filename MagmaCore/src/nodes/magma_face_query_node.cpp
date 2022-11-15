// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"
#include <frantic/magma/nodes/magma_face_query_node.hpp>
#include <frantic/magma/nodes/magma_node_impl.hpp>

namespace frantic {
namespace magma {
namespace nodes {

MAGMA_DEFINE_TYPE( "FaceQuery", "Object", magma_face_query_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_EXPOSE_PROPERTY( exposePosition, bool )
MAGMA_EXPOSE_PROPERTY( channels, std::vector<frantic::tstring> )
MAGMA_INPUT( "Geometry", boost::blank() )
MAGMA_INPUT( "ObjIndex", 0 )
MAGMA_INPUT( "FaceIndex", 0 )
MAGMA_INPUT( "BaryCoords", frantic::graphics::vector3f( 1.f / 3.f, 1.f / 3.f, 1.f / 3.f ) )
MAGMA_DESCRIPTION(
    "Uses barycentric coords to interpolate values from the vertices of a specified triangle in a mesh." )
MAGMA_DEFINE_TYPE_END

magma_face_query_node::magma_face_query_node() { this->set_exposePosition( true ); }

int magma_face_query_node::get_num_outputs() const {
    int result = (int)m_channels.size();

    if( get_exposePosition() )
        ++result;

    return result;
}

void magma_face_query_node::get_output_description( int index, frantic::tstring& outDescription ) const {
    if( get_exposePosition() ) {
        if( index == 0 ) {
            outDescription = _T("Position");
            return;
        }
        --index;
    }

    if( index >= 0 && index < (int)m_channels.size() )
        outDescription = m_channels[index];
}

MAGMA_DEFINE_TYPE( "VertexQuery", "Object", magma_vertex_query_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_EXPOSE_PROPERTY( exposePosition, bool )
MAGMA_EXPOSE_PROPERTY( channels, std::vector<frantic::tstring> )
MAGMA_INPUT( "Geometry", boost::blank() )
MAGMA_INPUT( "ObjIndex", 0 )
MAGMA_INPUT( "VertIndex", 0 )
MAGMA_DESCRIPTION( "Extracts values from a vertex in a mesh." )
MAGMA_DEFINE_TYPE_END

magma_vertex_query_node::magma_vertex_query_node() { this->set_exposePosition( true ); }

int magma_vertex_query_node::get_num_outputs() const {
    int result = (int)m_channels.size();

    if( get_exposePosition() )
        ++result;

    return result;
}

void magma_vertex_query_node::get_output_description( int index, frantic::tstring& outDescription ) const {
    if( get_exposePosition() ) {
        if( index == 0 ) {
            outDescription = _T("Position");
            return;
        }
        --index;
    }

    if( index >= 0 && index < (int)m_channels.size() )
        outDescription = m_channels[index];
}

MAGMA_DEFINE_TYPE( "ElementQuery", "Object", magma_element_query_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_INPUT( "Geometry", boost::blank() )
MAGMA_INPUT( "ObjIndex", 0 )
MAGMA_INPUT( "ElementIndex", 0 )
MAGMA_OUTPUT_NAMES( "SurfaceArea", "Volume", "Centroid" )
MAGMA_DESCRIPTION( "Queries properties of an element, which is a connected region of polygons." )
MAGMA_DEFINE_TYPE_END

int magma_element_query_node::get_num_outputs() const { return 3; }

MAGMA_DEFINE_TYPE( "MeshQuery", "Object", magma_mesh_query_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_INPUT( "Geometry", boost::blank() )
MAGMA_INPUT( "ObjIndex", 0 )
MAGMA_OUTPUT_NAMES( "FaceCount", "VertexCount", "ElementCount" )
MAGMA_DESCRIPTION( "Queries properties of an entire mesh, as opposed to individual pieces of a mesh." )
MAGMA_DEFINE_TYPE_END

int magma_mesh_query_node::get_num_outputs() const { return 3; }

} // namespace nodes
} // namespace magma
} // namespace frantic
