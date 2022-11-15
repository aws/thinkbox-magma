// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#pragma once

#include <frantic/magma/magma_singleton.hpp>
#include <frantic/magma/nodes/magma_input_geometry_interface.hpp>
#include <frantic/magma/nodes/magma_node_property.hpp>
#include <frantic/magma/nodes/magma_simple_operator.hpp>

#include <frantic/channels/channel_map.hpp>

#include <boost/mpl/vector.hpp>

#include <vector>

namespace frantic {
namespace magma {
namespace functors {
class nearest_point;
}
} // namespace magma
} // namespace frantic

namespace frantic {
namespace magma {
namespace nodes {

class magma_nearest_point_node : public magma_simple_operator<3> {
  public:
    struct meta {
        enum { ARITY = 2 };
        typedef frantic::magma::functors::nearest_point type;
        typedef boost::mpl::vector<void( void*, vec3, int ), void( void*, vec3, float ), void( void*, vec3, bool )>
            bindings;
    };

    MAGMA_REQUIRED_METHODS( magma_nearest_point_node );

    virtual int get_num_outputs() const { return 7; }

    // TODO Deprecated
    class functor {
        std::vector<magma_geometry_ptr> m_meshes;

        struct results_holder {
            frantic::graphics::vector3f pos;
            bool valid;
            int objIndex;
            int faceIndex;
            float distance;
            frantic::graphics::vector3f faceNormal;
            frantic::graphics::vector3f baryCoords;
        };

      public:
        typedef void* return_type;
        typedef boost::mpl::vector<frantic::graphics::vector3f> param_types;

        inline void calculate_result_layout( frantic::channels::channel_map& map ) const {
            map.reset();
            map.define_channel( _T("Position"), 3, frantic::channels::data_type_float32,
                                offsetof( results_holder, pos ) );
            map.define_channel( _T("IsValid"), 1, magma_singleton::get_named_data_type( _T("Bool") )->m_elementType,
                                offsetof( results_holder, valid ) );
            map.define_channel( _T("ObjIndex"), 1, frantic::channels::data_type_int32,
                                offsetof( results_holder, objIndex ) );
            map.define_channel( _T("FaceIndex"), 1, frantic::channels::data_type_int32,
                                offsetof( results_holder, faceIndex ) );
            map.define_channel( _T("Distance"), 1, frantic::channels::data_type_float32,
                                offsetof( results_holder, distance ) );
            map.define_channel( _T("FaceNormal"), 3, frantic::channels::data_type_float32,
                                offsetof( results_holder, faceNormal ) );
            map.define_channel( _T("BaryCoords"), 3, frantic::channels::data_type_float32,
                                offsetof( results_holder, baryCoords ) );
            map.end_channel_definition( 4u, true, false );
        }

      public:
        void set_geometry( const magma_input_geometry_interface& geomInterface ) {
            geomInterface.get_all_geometry( std::back_inserter( m_meshes ) );

            for( std::vector<magma_geometry_ptr>::iterator it = m_meshes.begin(), itEnd = m_meshes.end(); it != itEnd;
                 ++it )
                ( *it )->allocate_kdtree();
        }

        bool operator()( const frantic::graphics::vector3f& pos,
                         magma_geometry_interface::query_result& outResult ) const throw() {
            bool result = false;

            // TODO: We could use the bounding boxes of the individual primitves to accelerate the determination of
            // closest primitives.
            try {
                int curPrim = 0;
                for( std::vector<magma_geometry_ptr>::const_iterator it = m_meshes.begin(), itEnd = m_meshes.end();
                     it != itEnd; ++it, ++curPrim ) {
                    bool isValid = ( *it )->find_nearest_point( pos, outResult.distance, outResult );
                    if( isValid ) {
                        result = true;
                        outResult.objIndex = curPrim;
                    }
                }
            } catch( ... ) {
                result = false;
            }

            return result;
        }

        inline void operator()( void* _out, const frantic::graphics::vector3f& pos ) const throw() {
            results_holder& out = *static_cast<results_holder*>( _out );

            frantic::magma::magma_geometry_interface::query_result qr;

            qr.distance = std::numeric_limits<float>::max();

            if( operator()( pos, qr ) ) {
                out.pos = qr.pos;
                out.valid = true;
                out.objIndex = qr.objIndex;
                out.faceIndex = qr.faceIndex;
                out.distance = frantic::graphics::vector3f::dot( ( qr.pos - pos ), qr.normal ) <= 0
                                   ? (float)qr.distance
                                   : -(float)qr.distance;
                out.faceNormal = qr.normal;
                out.baryCoords = qr.baryCoord;
            } else {
                memset( &out, 0, sizeof( results_holder ) );
            }
        }
    };
};

} // namespace nodes
} // namespace magma
} // namespace frantic
