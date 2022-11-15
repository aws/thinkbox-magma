// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#pragma once

#include <frantic/magma/nodes/magma_input_geometry_interface.hpp>
#include <frantic/magma/nodes/magma_node_property.hpp>
#include <frantic/magma/nodes/magma_simple_operator.hpp>

#include <boost/mpl/vector.hpp>

#include <vector>

namespace frantic {
namespace magma {
namespace functors {
class in_volume;
}
} // namespace magma
} // namespace frantic

namespace frantic {
namespace magma {
namespace nodes {

class magma_in_volume_node : public magma_simple_operator<2> {
  public:
    struct meta {
        enum { ARITY = 1 };
        typedef frantic::magma::functors::in_volume type;
        typedef boost::mpl::vector<bool( vec3 )> bindings;
    };

    MAGMA_REQUIRED_METHODS( magma_in_volume_node );

    class functor {
        std::vector<magma_geometry_ptr> m_meshes;

      public:
        typedef bool return_type;
        typedef boost::mpl::vector<frantic::graphics::vector3f> param_types;

      public:
        void set_geometry( const magma_input_geometry_interface& geomInterface ) {
            geomInterface.get_all_geometry( std::back_inserter( m_meshes ) );

            for( std::vector<magma_geometry_ptr>::iterator it = m_meshes.begin(), itEnd = m_meshes.end(); it != itEnd;
                 ++it )
                ( *it )->allocate_kdtree();
        }

        bool operator()( const frantic::graphics::vector3f& pos ) const throw() {
            bool result = false;

            try {
                for( std::vector<magma_geometry_ptr>::const_iterator it = m_meshes.begin(), itEnd = m_meshes.end();
                     !result && it != itEnd; ++it ) {
                    if( ( *it )->in_volume( pos ) )
                        result = true;
                }
            } catch( ... ) {
                result = false;
            }

            return result;
        }

        void operator()( bool& out, const frantic::graphics::vector3f& pos ) const throw() { out = operator()( pos ); }
    };
};

} // namespace nodes
} // namespace magma
} // namespace frantic
