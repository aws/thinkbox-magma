// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "frantic/magma/nodes/magma_standard_operators.hpp"

#include "frantic/magma/maya/maya_magma_singleton.hpp"
#include "frantic/magma/maya/nodes/magma_input_value_node.hpp"
#include "frantic/magma/maya/nodes/maya_magma_input_geometry_node.hpp"
#include "frantic/magma/maya/nodes/maya_magma_input_object_node.hpp"
#include "frantic/magma/maya/nodes/maya_magma_input_particles_node.hpp"
#include "frantic/magma/maya/nodes/maya_magma_transform_nodes.hpp"

namespace frantic {
namespace magma {
namespace maya {
namespace modifier {

maya_magma_singleton::maya_magma_singleton()
    : frantic::magma::magma_singleton( true ) {
    FF_LOG( debug ) << "maya_magma_singleton: defining nodes type" << std::endl;
    define_node_type<maya::nodes::magma_input_value_node>();
    define_node_type<maya::nodes::maya_magma_input_geometry_node>();
    define_node_type<maya::nodes::maya_magma_input_particles_node>();
    define_node_type<maya::nodes::maya_magma_input_object_node>();
    define_node_type<maya::nodes::maya_magma_from_space_node>();
    define_node_type<maya::nodes::maya_magma_to_space_node>();
    define_node_type<frantic::magma::nodes::magma_to_camera_node>();
    define_node_type<frantic::magma::nodes::magma_from_camera_node>();
}

maya_magma_singleton& maya_magma_singleton::get_instance() {
    static maya_magma_singleton theMayaMagmaSingleton;
    return theMayaMagmaSingleton;
}

} // namespace modifier
} // namespace maya
} // namespace magma
} // namespace frantic
