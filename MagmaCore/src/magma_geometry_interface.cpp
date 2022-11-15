// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/geometry/generic_mesh_kdtree.hpp>
#include <frantic/geometry/mesh_interface_utils.hpp>
#include <frantic/geometry/polygon_utils.hpp>
#include <frantic/geometry/triangle_utils.hpp>
#include <frantic/graphics/transform4f.hpp>
#include <frantic/magma/magma_geometry_interface.hpp>

#include <frantic/graphics/plane3f.hpp>

using namespace frantic::graphics;

namespace frantic {
namespace magma {

class mesh_interface_kdtree_traits {
  public:
    typedef magma_geometry_interface::mesh_type mesh_type;
    typedef const magma_geometry_interface::mesh_type* mesh_type_const_ptr;
    typedef magma_geometry_interface::query_result raytrace_result;
    typedef magma_geometry_interface::query_result nearest_point_result;

    inline static unsigned get_count( const mesh_type& inst ) { return (unsigned)inst.get_num_faces(); }
    inline static frantic::graphics::boundbox3f get_bounds( const mesh_type& inst );
    inline static frantic::graphics::boundbox3f get_clipped_bounds( const mesh_type& inst, unsigned index,
                                                                    const frantic::graphics::boundbox3f& bounds );
    inline static bool intersect_ray( const mesh_type& inst, unsigned index, const frantic::graphics::ray3f& ray,
                                      double tMin, double tMax, raytrace_result& outIntersection,
                                      bool ignoreBackfaces = false );
    inline static bool find_nearest_point( const mesh_type& inst, unsigned index,
                                           const frantic::graphics::vector3f& point, double maxDistance,
                                           nearest_point_result& outNearestPoint, bool ignoreBackfaces = false );
};

frantic::graphics::boundbox3f mesh_interface_kdtree_traits::get_bounds( const mesh_type& inst ) {
    frantic::graphics::boundbox3f result;
    frantic::graphics::vector3f v;

    for( std::size_t i = 0, iEnd = inst.get_num_verts(); i < iEnd; ++i ) {
        inst.get_vert( i, reinterpret_cast<float( & )[3]>( v.x ) );
        result += v;
    }

    return result;
}

frantic::graphics::boundbox3f
mesh_interface_kdtree_traits::get_clipped_bounds( const mesh_type& inst, unsigned index,
                                                  const frantic::graphics::boundbox3f& bounds ) {
    // HACK: We are assuming triangles!!!!!!
    frantic::graphics::vector3f tri[3];
    frantic::graphics::boundbox3f clippedBounds;

    inst.get_face_verts( index, reinterpret_cast<float( * )[3]>( tri ) );

    bounds.intersect_with_triangle( tri[0], tri[1], tri[2], clippedBounds );

    return clippedBounds;
}

bool mesh_interface_kdtree_traits::intersect_ray( const mesh_type& inst, unsigned index,
                                                  const frantic::graphics::ray3f& ray, double tMin, double tMax,
                                                  raytrace_result& outIntersection, bool ignoreBackfaces ) {
    typedef frantic::graphics::vector3f vector_type;

    vector_type tri[3];

    inst.get_face_verts( index, reinterpret_cast<float( * )[3]>( tri ) );

    /*plane3f plane = plane3f::from_triangle( tri[0], tri[1], tri[2] );
    double distance = plane.get_distance_to_intersection( ray );
    vector3f pt = ray.at( distance );
    vector3f barycentricCoord = compute_barycentric_coordinates( pt, tri[0], tri[1], tri[2] );*/

    vector_type p1p0 = tri[1] - tri[0];
    vector_type p2p0 = tri[2] - tri[0];
    vector_type pAp0 = ray.origin() - tri[0];
    vector_type d = ray.direction();

    vector_type dXp2p0 = vector_type::cross( d, p2p0 );
    vector_type pAp0Xp1p0 = vector_type::cross( pAp0, p1p0 );

    // Used to use floats but the precision caused problems.
    double V = vector_type::dot_double( dXp2p0, p1p0 );
    double Va = vector_type::dot_double( pAp0Xp1p0, p2p0 );

    double t = Va / V;

    // We need to be careful of the openness of the interval here. I am using closed on both to catch the kdtree
    // splitting algorithm that puts axis aligned triangles arbitrarily on one of the sides when it lies directly on the
    // split plane.
    if( t <= tMax && t >= tMin ) {
        double V1 = vector_type::dot_double( dXp2p0, pAp0 );
        double V2 = vector_type::dot_double( pAp0Xp1p0, d );

        float b = static_cast<float>( V1 / V );
        float c = static_cast<float>( V2 / V );
        float a = 1.f - b - c;

        if( a >= 0 && b >= 0.f && c >= 0.f ) {
            if( ignoreBackfaces ) {
                const std::size_t numFaceVerts = inst.get_num_face_verts( (int)index );

                typedef float vertex_t[3];
                boost::scoped_array<vertex_t> faceVerts( new vertex_t[numFaceVerts] );

                inst.get_face_verts( (int)index, faceVerts.get() );

                typedef frantic::geometry::face_vertex_iterator vertex_it;

                frantic::graphics::vector3f normal = frantic::geometry::get_polygon_normal3(
                    vertex_it( &inst, (int)index, vertex_it::begin ), vertex_it( &inst, (int)index, vertex_it::end ) );
                const bool notBackface = frantic::graphics::vector3f::dot( ray.direction(), normal ) <= 0;
                if( notBackface ) {
                    outIntersection.distance = t;
                    outIntersection.faceIndex = (int)index;
                    outIntersection.baryCoord.x = a;
                    outIntersection.baryCoord.y = b;
                    outIntersection.baryCoord.z = c;

                    outIntersection.pos = a * tri[0] + b * tri[1] + c * tri[2];
                    outIntersection.normal = vector_type::cross( p1p0, p2p0 );
                }
                return notBackface;
            } else {
                outIntersection.distance = t;
                outIntersection.faceIndex = (int)index;
                outIntersection.baryCoord.x = a;
                outIntersection.baryCoord.y = b;
                outIntersection.baryCoord.z = c;

                outIntersection.pos = a * tri[0] + b * tri[1] + c * tri[2];
                outIntersection.normal = vector_type::cross( p1p0, p2p0 );
                return true;
            }
        }
    }

    return false;
}

// TODO: This is horribly inefficient!
bool mesh_interface_kdtree_traits::find_nearest_point( const mesh_type& inst, unsigned index,
                                                       const frantic::graphics::vector3f& point, double maxDistance,
                                                       nearest_point_result& outNearestPoint, bool ignoreBackfaces ) {
    typedef frantic::graphics::vector3f vector_type;

    vector_type tri[3];

    inst.get_face_verts( index, reinterpret_cast<float( * )[3]>( tri ) );

    // frantic::geometry::nearest_point_search_result npsr;

    // frantic::geometry::nearest_point_on_triangle( point, tri[0], tri[1], tri[2], npsr );

    plane3f plane = plane3f::from_triangle( tri[0], tri[1], tri[2] );

    // Project the point onto the triangle's plane
    float distance = plane.get_signed_distance_to_plane( point );
    if( fabsf( distance ) < maxDistance ) {

        // Check that the intersection is inside the bounding box (we're intersecting the ray with the intersection of
        // the triangle and the bounding box)
        vector3f projected = point - distance * plane.normal();

        // Now get the barycentric coordinates of this projection
        vector3f barycentricCoord = compute_barycentric_coordinates( projected, tri[0], tri[1], tri[2] );
        if( barycentricCoord.is_inf() )
            return false;

        if( barycentricCoord.x >= 0 && barycentricCoord.y >= 0 && barycentricCoord.z >= 0 ) {
            distance = std::abs( distance ); // Projected and barycentricCoord are both accurate
        } else {
            frantic::geometry::detail::nearest_point_on_triangle( projected, barycentricCoord, tri[0], tri[1], tri[2] );
            distance = vector3f::distance( point, projected );
        }

        if( distance < maxDistance ) {
            if( ignoreBackfaces ) {
                const std::size_t numFaceVerts = inst.get_num_face_verts( (int)index );

                typedef float vertex_t[3];
                boost::scoped_array<vertex_t> faceVerts( new vertex_t[numFaceVerts] );

                inst.get_face_verts( (int)index, faceVerts.get() );

                typedef frantic::geometry::face_vertex_iterator vertex_it;

                frantic::graphics::vector3f normal = frantic::geometry::get_polygon_normal3(
                    vertex_it( &inst, (int)index, vertex_it::begin ), vertex_it( &inst, (int)index, vertex_it::end ) );
                const frantic::graphics::vector3f direction = projected - point;
                const bool notBackface = frantic::graphics::vector3f::dot( direction, normal ) <= 0;
                if( notBackface ) {
                    outNearestPoint.distance = distance;
                    outNearestPoint.pos = projected;
                    outNearestPoint.normal = plane.normal();
                    outNearestPoint.faceIndex = (int)index;
                    outNearestPoint.baryCoord = barycentricCoord;
                }
                return notBackface;
            } else {
                outNearestPoint.distance = distance;
                outNearestPoint.pos = projected;
                outNearestPoint.normal = plane.normal();
                outNearestPoint.faceIndex = (int)index;
                outNearestPoint.baryCoord = barycentricCoord;
                return true;
            }
        }
    }

    return false;
}

class magma_geometry_impl : public magma_geometry_interface {
    typedef frantic::geometry::generic_mesh_kdtree<mesh_interface_kdtree_traits> kdtree_type;

    kdtree_type m_kdtree;
    frantic::graphics::transform4f m_toWorld, m_fromWorld;

  public:
    magma_geometry_impl( mesh_ptr_type mesh, const frantic::graphics::transform4f& objToWorldTM )
        : magma_geometry_interface( mesh )
        , m_toWorld( objToWorldTM )
        , m_fromWorld( objToWorldTM.to_inverse() ) {
        // m_kdtree.set_mesh( mesh );
    }

    virtual const frantic::graphics::transform4f& get_toworld_transform() const { return m_toWorld; }

    virtual void set_toworld_transform( const frantic::graphics::transform4f& tm ) {
        m_toWorld = tm;
        m_fromWorld = tm.to_inverse();
    }

    virtual ~magma_geometry_impl() {}

    virtual bool allocate_kdtree( bool forceRebuild ) {
        // Have we initialized the kdtree already?
        if( &m_kdtree.get_mesh() == &this->get_mesh() && !forceRebuild )
            return false;

        m_kdtree.set_mesh( &this->get_mesh() );
        m_kdtree.finalize();
        return true;
    }

    virtual bool intersect_ray( const frantic::graphics::vector3f& pos, const frantic::graphics::vector3f& dir,
                                double maxDistance, query_result& outResult, bool ignoreBackfaces = false ) const {
        frantic::graphics::ray3f r( m_fromWorld * pos, m_fromWorld.transform_no_translation( dir ) );

        bool result = m_kdtree.intersect_ray( r, 0.0, maxDistance, outResult, ignoreBackfaces );
        if( result ) {
            // frantic::graphics::vector3f faceVerts[3];

            // get_mesh().get_face_verts( outResult.faceIndex, reinterpret_cast<float(&)[3][3]>(faceVerts) );

            // outResult.pos = m_toWorld * ( outResult.baryCoord.x * faceVerts[0] + outResult.baryCoord.y * faceVerts[1]
            // + outResult.baryCoord.z * faceVerts[2] ); outResult.normal = frantic::graphics::vector3f::normalize(
            //	m_fromWorld.transpose_transform_no_translation(
            //		frantic::graphics::vector3f::cross( faceVerts[2] - faceVerts[1], faceVerts[0] - faceVerts[1] ) //Order
            //taken from plane3f::from_triangle ) );

            outResult.pos = m_toWorld * outResult.pos;
            outResult.normal = frantic::graphics::vector3f::normalize(
                m_fromWorld.transpose_transform_no_translation( outResult.normal ) );
        }

        return result;
    }

    virtual bool find_nearest_point( const frantic::graphics::vector3f& pos, double maxDistance,
                                     query_result& outResult, bool ignoreBackfaces = false ) const {
        bool result = m_kdtree.find_nearest_point( m_fromWorld * pos, maxDistance, outResult, ignoreBackfaces );
        if( result ) {
            // frantic::graphics::vector3f faceVerts[3];
            //
            // get_mesh().get_face_verts( outResult.faceIndex, reinterpret_cast<float(&)[3][3]>(faceVerts) );

            // outResult.pos = m_toWorld * ( outResult.baryCoord.x * faceVerts[0] + outResult.baryCoord.y * faceVerts[1]
            // + outResult.baryCoord.z * faceVerts[2] ); outResult.normal = frantic::graphics::vector3f::normalize(
            //	m_fromWorld.transpose_transform_no_translation(
            //		frantic::graphics::vector3f::cross( faceVerts[2] - faceVerts[1], faceVerts[0] - faceVerts[1] ) //Order
            //taken from plane3f::from_triangle ) );

            outResult.pos = m_toWorld * outResult.pos;
            outResult.normal = frantic::graphics::vector3f::normalize(
                m_fromWorld.transpose_transform_no_translation( outResult.normal ) );
        }

        return result;
    }

    virtual bool in_volume( const frantic::graphics::vector3f& pos ) const {
        frantic::graphics::vector3f objPos = m_fromWorld * pos;

        query_result qr;

        if( !m_kdtree.find_nearest_point( objPos, FLT_MAX, qr ) )
            return false;

        bool result = frantic::graphics::vector3f::dot( ( qr.pos - objPos ), qr.normal ) >= 0;

        if( qr.baryCoord.x <= 0.001f || qr.baryCoord.y <= 0.001f || qr.baryCoord.z <= 0.001f ) {
            float tri[3][3];

            static const int LIMIT = 2; // Number of times it will try a new face. Increase for accuracy.

            int counter = 0;

            do {
                this->get_mesh().get_face_verts( qr.faceIndex, tri );

                frantic::graphics::vector3f dir( ( tri[0][0] + tri[1][0] + tri[2][0] ) / 3.f - objPos.x,
                                                 ( tri[0][1] + tri[1][1] + tri[2][1] ) / 3.f - objPos.y,
                                                 ( tri[0][2] + tri[1][2] + tri[2][2] ) / 3.f - objPos.z );

                qr.distance = 1.01; // Give it a little extra in order to counteract rounding error.

                // We are shooting at a face-center. If we missed something is seriously messed up...
                bool mustBeTrue =
                    m_kdtree.intersect_ray( frantic::graphics::ray3f( objPos, dir ), 0.0, qr.distance, qr );
                // if( !mustBeTrue ){
                //	mustBeTrue = m_kdtree.intersect_ray( frantic::graphics::ray3f( objPos, dir ), 0.0, qr.distance,
                //qr );
                // }

                assert( mustBeTrue );

            } while( ++counter < LIMIT &&
                     ( qr.baryCoord.x <= 0.001f || qr.baryCoord.y <= 0.001f || qr.baryCoord.z <= 0.001f ) );

            result = frantic::graphics::vector3f::dot( ( qr.pos - objPos ), qr.normal ) >= 0;
        }

        return result;
    }
};

magma_geometry_ptr magma_geometry_interface::create_instance( mesh_ptr_type mesh ) {
    return magma_geometry_ptr( new magma_geometry_impl( mesh, frantic::graphics::transform4f() ) );
}

magma_geometry_ptr magma_geometry_interface::create_instance( mesh_ptr_type mesh,
                                                              const frantic::graphics::transform4f& objToWorldTM ) {
    return magma_geometry_ptr( new magma_geometry_impl( mesh, objToWorldTM ) );
}

} // namespace magma
} // namespace frantic
