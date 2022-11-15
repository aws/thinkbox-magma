// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_geometry_interface.hpp>
#include <frantic/magma/nodes/magma_simple_operator.hpp>

namespace frantic {
namespace geometry {
class mesh_interface;
}
} // namespace frantic

namespace frantic {
namespace magma {
namespace nodes {

class magma_input_geometry_interface {
  public:
    virtual ~magma_input_geometry_interface() {}

    /**
     * @return The number of objects held.
     */
    virtual std::size_t size() const = 0;

    /**
     * @param index The index of the object to retrieve
     * @return A pointer to a magma_geometry object.
     */
    virtual magma_geometry_ptr get_geometry( std::size_t index ) const = 0;

    /**
     * Helper method to fill a container with all the mesh objects contained.
     * Ex.
     *  std::vector<magma_geometry_ptr> meshes;
     *  magma_input_geometry_interface* geomInterface;
     *
     *  geomInterface->get_all_geometry( std::back_inserter( meshes ) );
     * @param it An iterator meeting the requirements of an STL output iterator.
     */
    template <class OutputIterator>
    inline void get_all_geometry( OutputIterator it ) const {
        for( std::size_t i = 0, iEnd = this->size(); i < iEnd; ++i, ++it )
            *it = this->get_geometry( i );
    }
};

} // namespace nodes
} // namespace magma
} // namespace frantic
