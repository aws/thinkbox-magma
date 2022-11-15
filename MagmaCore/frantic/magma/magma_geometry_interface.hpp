// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <boost/intrusive_ptr.hpp>
#include <frantic/geometry/mesh_interface.hpp>
#include <frantic/graphics/boundbox3f.hpp>

namespace frantic {
namespace magma {

/**
 * This class provides an abstract interface for querying geometry in magma. It uses frantic::geometry::mesh_interface
 * to abstract the actual source mesh type away. This class provides further information on top of the mesh_interface
 * object it contains.
 */
class magma_geometry_interface {
  public:
    typedef boost::intrusive_ptr<magma_geometry_interface> ptr_type;
    typedef frantic::geometry::mesh_interface mesh_type;
    typedef boost::shared_ptr<mesh_type> mesh_ptr_type;

  private:
    mesh_ptr_type m_mesh;

    // We are using boost::intrusive to manage reference counting for this object.
    int m_refCount;
    friend void intrusive_ptr_add_ref( magma_geometry_interface* p );
    friend void intrusive_ptr_release( magma_geometry_interface* p );

  protected:
    magma_geometry_interface( mesh_ptr_type mesh )
        : m_mesh( mesh )
        , m_refCount( 0 ) {}

  public:
    virtual ~magma_geometry_interface() {}

    /**
     * This factory function will make an instance of a suitable subclass of magma_geometry_interface and return a ptr
     * to it.
     */
    static ptr_type create_instance( mesh_ptr_type mesh );
    static ptr_type create_instance( mesh_ptr_type mesh, const frantic::graphics::transform4f& objToWorldTM );

    /**
     * @return The contained mesh object.
     */
    inline const mesh_type& get_mesh() const { return *m_mesh; }
    inline mesh_type& get_mesh() { return *m_mesh; }

    /**
     * We can determine if this object is unique (ie. not shared) by inspecting the reference count
     */
    inline bool is_unique() const { return m_refCount == 1; }

    /**
     * This struct holds results from a query operation. (ie. intersect_ray(), or find_nearest_point())
     */
    struct query_result {
        frantic::graphics::vector3f pos;
        frantic::graphics::vector3f baryCoord; // TODO: How to handle non-trimeshes?
        frantic::graphics::vector3f normal;
        double distance;
        int objIndex;
        int faceIndex;

        /*union{
                float baryCoord[3];
                float baryGeneral;
        };
        bool triangleResult;*/

        query_result()
            : distance( FLT_MAX )
            , objIndex( 0 )
            , faceIndex( -1 ) {}
    };

    /**
     * The mesh is stored in a space that may be unique to it. We provide the transformation matrix that would move
     * geometric data into a consistent shared space for all objects.
     * @return The transformation that would move geometric data into world space.
     */
    virtual const frantic::graphics::transform4f& get_toworld_transform() const = 0;
    virtual void set_toworld_transform( const frantic::graphics::transform4f& ) = 0;

    virtual bool allocate_kdtree( bool forceRebuild = false ) = 0;
    virtual bool intersect_ray( const frantic::graphics::vector3f& pos, const frantic::graphics::vector3f& dir,
                                double maxDistance, query_result& outResult, bool ignoreBackfaces = false ) const = 0;
    virtual bool find_nearest_point( const frantic::graphics::vector3f& pos, double maxDistance,
                                     query_result& outResult, bool ignoreBackfaces = false ) const = 0;
    virtual bool in_volume( const frantic::graphics::vector3f& pos ) const = 0;
};

inline void intrusive_ptr_add_ref( magma_geometry_interface* p ) { ++p->m_refCount; }
inline void intrusive_ptr_release( magma_geometry_interface* p ) {
    if( --p->m_refCount == 0 )
        delete p;
}

typedef magma_geometry_interface::ptr_type magma_geometry_ptr;

} // namespace magma
} // namespace frantic
