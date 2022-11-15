// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "frantic/magma/maya/nodes/maya_magma_node.hpp"
#include <frantic/magma/magma_node_base.hpp>
#include <frantic/magma/nodes/magma_input_geometry_interface.hpp>
#include <frantic/magma/nodes/magma_input_objects_interface.hpp>
#include <frantic/magma/nodes/magma_node_property.hpp>

#include <frantic/geometry/dcel_mesh_interface.hpp>

#include <memory>
#include <stdexcept>

namespace frantic {
namespace magma {
namespace maya {
namespace nodes {

class maya_magma_input_geometry_node : public magma_maya_simple_operator,
                                       public frantic::magma::nodes::magma_input_geometry_interface,
                                       public frantic::magma::nodes::magma_input_objects_interface {

    MAGMA_REQUIRED_METHODS( maya_magma_input_geometry_node );
    MAGMA_PROPERTY( geometryNames, std::vector<frantic::tstring> );

  private:
    std::vector<magma_geometry_ptr> m_meshData;
    std::vector<MObject> m_objects;

    void get_transform( std::size_t index, frantic::graphics::transform4f& outTransform, bool& foundOutTransform );

  public:
    virtual int get_num_outputs() const { return 3; }

    std::size_t size() const;
    magma_geometry_ptr get_geometry( std::size_t index ) const;
    virtual void compile_as_extension_type( frantic::magma::magma_compiler_interface& compiler );

    // From magma_input_objects_interface

  protected:
    virtual bool get_property_internal( std::size_t index, const frantic::tstring& propName,
                                        const std::type_info& typeInfo, void* outValue );

  public:
    // Already implemented as part of other interface. The result should be the same for both...hopefully...
    // virtual std::size_t size() const;

    virtual void get_property( std::size_t index, const frantic::tstring& propName, variant_t& outValue );
};

} // namespace nodes
} // namespace maya
} // namespace magma
} // namespace frantic

// TODO: move this to another hpp file?
#include <frantic/geometry/mesh_interface.hpp>
#include <maya/MFloatArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFnMesh.h>
#include <maya/MMatrix.h>
#include <maya/MPointArray.h>

namespace frantic {
namespace magma {
namespace maya {
namespace nodes {

#pragma region maya_magma_mesh
class maya_magma_mesh : public frantic::geometry::mesh_interface {

  private:
    /**
     * Sets up the available default channels for use in magma
     */
    void initialize_default_channels();

    /**
     * Attempts to add auxiliary channels for use in magma
     * @param channelName channel to add
     * @param vertexChannel If true, we are expecting a per-vertex channel. If false we are expecting a per-face
     * channel.
     * @return true if added successfully
     */
    bool populate_channel( const frantic::tstring& channelName, bool vertexChannel );

  public:
    maya_magma_mesh( MFnMesh& mesh );

    /**
     * @return True if the mesh_interface is bound to a legitimate mesh object underneath.
     */
    bool is_valid() const;

    /**
     * When a channel is not accessible via get_vertex_channels().get_channel() or get_face_channels().get_channel()
     * this method can be used to have the object try and populate that channel if it is possible.
     * @param channelName The name of the channel we want to populate
     * @param vertexChannel If true, we are expecting a per-vertex channel. If false we are expecting a per-face
     * channel.
     * @param forOutput If true, we are expecting to write to this channel. If false we will only read from it. Some
     * channels cannot be written to.
     * @param throwOnError If true and the channel is unknown (or readonly and 'forOutput' was true) then an exception
     * is throw. Otherwise it returns false.
     * @return True if the channel requested was able to populated, false otherwise.
     */
    bool request_channel( const frantic::tstring& channelName, bool vertexChannel, bool forOutput,
                          bool throwOnError = true );

    /**
     * @return The number of vertices in the mesh.
     */
    std::size_t get_num_verts() const;

    /**
     * Gets a single vertex
     * @param index Which vertex index
     * @param outValues Result is stored here BTW, that is the syntax for a reference to a 3 float array.
     */
    void get_vert( std::size_t index, float ( &outValues )[3] ) const;

    /**
     * @return The number of vertices in the mesh.
     */
    std::size_t get_num_faces() const;

    /**
     * @return The number of vertices associated with the specified face. ie. 3 for triangles, 4 for quads, etc.
     */
    std::size_t get_num_face_verts( std::size_t faceIndex ) const;

    /**
     * @param faceIndex Which face
     * @param fvertIndex Which vertex in the face. In [ 0, get_num_face_verts(faceIndex) )
     * @return The vertex index that is at the specified slot in the face.
     */
    std::size_t get_face_vert_index( std::size_t faceIndex, std::size_t fvertIndex ) const;

    /**
     * Gets the face vertex id.  This is used for getting certain properties in Maya that are NOT defined in a per-point
     * fashion
     * @param faceIndex Which face
     * @param fvertIndex Which vertex in the face. In [ 0, get_num_face_verts(faceIndex) )
     * @return The vertex ID that is at the specified slot in the face.
     */
    std::size_t get_face_vert_id( std::size_t faceIndex, std::size_t fvertIndex ) const;

    /**
     * Gets the maya polygon and vertex index from the face vertex id
     * @param id Which fv ID
     * @param faceIndex Which Maya polygon stored here
     * @param fvertIndex Which Maya per polygon vertex stored here
     * @return True if successfully retrieved
     */
    bool get_maya_face_vert_from_id( std::size_t id, std::size_t& outFaceIndex, std::size_t& outFVertIndex ) const;

    /**
     * Gets the maya polyon and vertex index from the face vertex id
     * @param id Which fv ID
     * @return the Maya vertex ID
     */
    std::size_t get_maya_vertex_from_id( std::size_t id ) const;

    /**
     * Gets the maya polygon ID from our face ID
     * @param id Which face faceIndex
     * @return the Maya Polygon ID
     */
    std::size_t get_maya_polygon_from_face_id( std::size_t faceIndex ) const;

    /**
     * Gets the vertex indices for all verts in the face.
     * @param faceIndex Which face
     * @param outValues Vertex indices for this face are stored here. Must be at least std::size_t[
     * get_num_face_verts(faceIndex) ].
     */
    void get_face_vert_indices( std::size_t faceIndex, std::size_t outValues[] ) const;

    /**
     * Gets the vertex positions for all verts in the face.
     * @param faceIndex Which face
     * @param outValues Result is stored here. This array must be at least float[ get_num_face_verts(faceIndex) ][3].
     */
    void get_face_verts( std::size_t faceIndex, float outValues[][3] ) const;

    /**
     * A mesh "element" is a connected region of polygons that may or may not be closed. If there are multiple elements
     * then that indicates multiple unconnected regions of connected polygons.
     * @note We don't provide an obvious iteration method for elements because a mesh is stored as a collection of faces
     * which is a lower-level abstraction than elements. The simplest way to work with elements is to query the
     * FaceElement channel which gives an element index for each face.
     * @return The number of disjoint collections of connected plygons in the mesh.
     */
    std::size_t get_num_elements() const;

    /**
     *  A mesh "element" is a connected region of polygons that may or may not be closed. This method queries the
     * element index that a face is a part of.
     * @note In order to use this channel, you must first call request_channel( "FaceElementIndex", false, false, false
     * )
     * @param faceIndex The face to query
     * @return The index of the element this face is a part of.
     */
    std::size_t get_face_element_index( std::size_t faceIndex ) const;

    /**
     * @return the Maya MFnMesh object used to create this geometry
     */
    MObject get_maya_mesh_object() const;

    virtual void init_adjacency();
    virtual bool has_adjacency() const;
    virtual bool init_vertex_iterator( frantic::geometry::vertex_iterator& vIt, std::size_t vertexIndex ) const;
    virtual bool advance_vertex_iterator( frantic::geometry::vertex_iterator& vIt ) const;
    virtual std::size_t get_edge_endpoint( frantic::geometry::vertex_iterator& vIt ) const;
    virtual std::size_t get_edge_left_face( frantic::geometry::vertex_iterator& vIt ) const;
    virtual std::size_t get_edge_right_face( frantic::geometry::vertex_iterator& vIt ) const;
    virtual bool is_edge_visible( frantic::geometry::vertex_iterator& vIt ) const;
    virtual bool is_edge_boundary( frantic::geometry::vertex_iterator& vIt ) const;
    virtual void init_face_iterator( frantic::geometry::face_iterator& fIt, std::size_t faceIndex ) const;
    virtual bool advance_face_iterator( frantic::geometry::face_iterator& fIt ) const;
    virtual std::size_t get_face_neighbor( frantic::geometry::face_iterator& fIt ) const;
    virtual std::size_t get_face_next_vertex( frantic::geometry::face_iterator& fIt ) const;
    virtual std::size_t get_face_prev_vertex( frantic::geometry::face_iterator& fIt ) const;

  private:
    MObject m_mesh_object;

    bool m_EnableCache;
    MPointArray m_points; // Cached vertex-points for faster look up

    // Extra MetaData
    struct polygon_map_data {
        std::size_t polygonID;  // Polygon ID
        std::size_t triangleID; // Triangle ID
        int triangleVertex[3];  // Vertex to get position
        int polygonVertex[3];   // Vertex to get per polygon ID
        int ourVertexID[3];     // Our Vertex ID
    };

    std::vector<polygon_map_data> m_FaceIDToPolygonData;  // Maps (FaceID) to (PolygonID, TriangleID, Maya VertexID,
                                                          // PerPolygonVertexID, Our VertexID)
    std::vector<std::pair<int, int>> m_IDToPolygonVertex; // Maps (Our VertexID) to (PolygonID,PerPolygonVertexID)
    std::vector<int> m_IDToVertex;                        // Maps (Our VertexID) to (Maya VertexID)

    std::unique_ptr<frantic::geometry::dcel_mesh_interface> m_adjacencyDelegate;
};
#pragma endregion

#pragma region Geometry Property Accessors
#pragma region Vertex Property Accessors
class maya_magma_mesh_vertex_accessor : public frantic::geometry::mesh_channel {
  protected:
    const maya_magma_mesh* m_mesh;

  public:
    virtual std::size_t get_num_face_verts( std::size_t /*faceIndex*/ ) const { return 3; }
    virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t fvertIndex ) const {
        return m_mesh->get_face_vert_id( faceIndex, fvertIndex );
    }
    virtual void set_value( std::size_t index, const void* value ) const {}; // Read Only
    virtual void get_value( std::size_t index, void* outValue ) const = 0;

    maya_magma_mesh_vertex_accessor( const frantic::tstring& name, frantic::channels::data_type_t type,
                                     std::size_t arity,
                                     frantic::geometry::mesh_channel::transform_type::enum_t transformType,
                                     const maya_magma_mesh* mesh )
        : frantic::geometry::mesh_channel( name, mesh_channel::vertex, type, arity, mesh->get_num_verts(),
                                           mesh->get_num_faces(), true )
        , m_mesh( mesh ) {
        frantic::geometry::mesh_channel::set_transform_type( transformType );
    }
};

class maya_magma_mesh_vertex_position_accessor : public maya_magma_mesh_vertex_accessor {
  public:
    maya_magma_mesh_vertex_position_accessor( const maya_magma_mesh* mesh )
        : maya_magma_mesh_vertex_accessor( _T("Position"), frantic::channels::data_type_float32, 3,
                                           transform_type::point, mesh ) {}

    virtual void get_value( std::size_t index, void* outValue ) const;
};

class maya_magma_mesh_vertex_normal_accessor : public maya_magma_mesh_vertex_accessor {
  public:
    maya_magma_mesh_vertex_normal_accessor( bool isSmooth, const maya_magma_mesh* mesh )
        : maya_magma_mesh_vertex_accessor( _T("Normal"), frantic::channels::data_type_float32, 3,
                                           transform_type::normal, mesh ) {}

    virtual void get_value( std::size_t index, void* outValue ) const;
};

class maya_magma_mesh_vertex_color_accessor : public maya_magma_mesh_vertex_accessor {
  private:
    const frantic::tstring m_set_name;

  public:
    maya_magma_mesh_vertex_color_accessor( const frantic::tstring& colorSetName, const maya_magma_mesh* mesh )
        : maya_magma_mesh_vertex_accessor( colorSetName, frantic::channels::data_type_float32, 3,
                                           transform_type::normal, mesh )
        , m_set_name( colorSetName ) {}

    maya_magma_mesh_vertex_color_accessor( const maya_magma_mesh* mesh )
        : maya_magma_mesh_vertex_accessor( _T("Color"), frantic::channels::data_type_float32, 3, transform_type::normal,
                                           mesh )
        , m_set_name() {}

    virtual void get_value( std::size_t index, void* outValue ) const;
};

class maya_magma_mesh_vertex_texture_accessor : public maya_magma_mesh_vertex_accessor {
  private:
    const frantic::tstring m_set_name;

  public:
    maya_magma_mesh_vertex_texture_accessor( const frantic::tstring& textureSetName, const maya_magma_mesh* mesh )
        : maya_magma_mesh_vertex_accessor( textureSetName, frantic::channels::data_type_float32, 3,
                                           transform_type::normal, mesh )
        , m_set_name( textureSetName ) {}

    maya_magma_mesh_vertex_texture_accessor( const maya_magma_mesh* mesh )
        : maya_magma_mesh_vertex_accessor( _T("TextureCoord"), frantic::channels::data_type_float32, 3,
                                           transform_type::normal, mesh )
        , m_set_name() {}

    virtual void get_value( std::size_t index, void* outValue ) const;
};
#pragma endregion

#pragma region Face Property Accessors
class maya_magma_mesh_face_accessor : public frantic::geometry::mesh_channel {
  protected:
    const maya_magma_mesh* m_mesh;

  public:
    virtual std::size_t get_num_face_verts( std::size_t /*faceIndex*/ ) const { return 1; }
    virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t fvertIndex ) const {
        return m_mesh->get_face_vert_id( faceIndex, fvertIndex );
    }
    virtual void set_value( std::size_t index, const void* value ) const {}; // Read Only
    virtual void get_value( std::size_t index, void* outValue ) const = 0;

    maya_magma_mesh_face_accessor( const frantic::tstring& name, frantic::channels::data_type_t type, std::size_t arity,
                                   frantic::geometry::mesh_channel::transform_type::enum_t transformType,
                                   const maya_magma_mesh* mesh )
        : frantic::geometry::mesh_channel( name, mesh_channel::face, type, arity, mesh->get_num_verts(),
                                           mesh->get_num_faces(), true )
        , m_mesh( mesh ) {
        frantic::geometry::mesh_channel::set_transform_type( transformType );
    }
};

class maya_magma_mesh_face_normal_accessor : public maya_magma_mesh_face_accessor {
  public:
    maya_magma_mesh_face_normal_accessor( const maya_magma_mesh* mesh )
        : maya_magma_mesh_face_accessor( _T("FaceNormal"), frantic::channels::data_type_float32, 3,
                                         transform_type::normal, mesh ) {}

    virtual void get_value( std::size_t index, void* outValue ) const;
};

#pragma endregion
#pragma endregion
} // namespace nodes
} // namespace maya
} // namespace magma
} // namespace frantic
