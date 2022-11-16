// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/max3d/nodes/magma_curve_op_node.hpp>
#include <frantic/magma/max3d/nodes/magma_geometry_input_node.hpp>
#include <frantic/magma/max3d/nodes/magma_input_object_node.hpp>
#include <frantic/magma/max3d/nodes/magma_input_particles_node.hpp>
#include <frantic/magma/max3d/nodes/magma_input_value_node.hpp>
#include <frantic/magma/max3d/nodes/magma_script_op_node.hpp>
#include <frantic/magma/max3d/nodes/magma_texmap_op_node.hpp>
#include <frantic/magma/max3d/nodes/magma_transform_node.hpp>

#include <frantic/magma/magma_singleton.hpp>
#include <frantic/magma/nodes/magma_node_impl.hpp>

#include <frantic/magma/max3d/magma_max_node_impl.hpp>

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

MAGMA_DEFINE_MAX_TYPE( "Curve", "Function", magma_curve_op_node )
MAGMA_READONLY_PROPERTY( curve, FPInterface* )
MAGMA_INPUT_NAMES( "Scalar" )
MAGMA_DESCRIPTION( "Uses a curve to transform the input value into another value." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_MAX_TYPE( "InputGeometry", "Input", magma_input_geometry_node )
MAGMA_EXPOSE_ARRAY_PROPERTY( nodes, INode* )
MAGMA_OUTPUT_NAMES( "Geometry", "Object Count" )
MAGMA_DESCRIPTION( "Exposes the geometry of a node for other operators." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_MAX_TYPE( "InputObject", "Input", magma_input_object_node )
MAGMA_EXPOSE_PROPERTY( object, INode* )
MAGMA_DESCRIPTION( "Exposes the properties of an object for other operators." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_MAX_TYPE( "InputParticles", "Input", magma_input_particles_node )
MAGMA_EXPOSE_PROPERTY( node, INode* )
MAGMA_OUTPUT_NAMES( "Particles", "Count" )
MAGMA_DESCRIPTION( "Exposes the particles of a Krakatoa PRT object for use with other operators." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_MAX_TYPE( "InputValue", "Input", magma_input_value_node )
MAGMA_EXPOSE_PROPERTY( forceInteger, bool )
MAGMA_EXPOSE_PROPERTY( controller, Control* )
MAGMA_DESCRIPTION( "An animatable value of a specified type." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "InputScript", "Input", magma_script_op_node )
MAGMA_EXPOSE_PROPERTY( script, M_STD_STRING )
// MAGMA_EXPOSE_PROPERTY( outputType, magma_data_type )
MAGMA_DESCRIPTION( "Exposes a constant value by evaluating a snippet of MAXScript. Supports "
                   "Integer,Float,Bool,Point3,Color,Quat results." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_MAX_TYPE( "InputTexmap", "Input", magma_texmap_op_node )
MAGMA_ENUM_PROPERTY( resultType, "Color", "Mono", "Perturb" )
MAGMA_EXPOSE_PROPERTY( texmap, Texmap* )
MAGMA_EXPOSE_PROPERTY( channels, std::vector<M_STD_STRING> )
MAGMA_DESCRIPTION( "Evaluates a texture map." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_MAX_TYPE( "FromSpace", "Transform", magma_from_space_node )
MAGMA_ENUM_PROPERTY( inputType, "Point", "Vector", "Normal" )
MAGMA_EXPOSE_PROPERTY( node, INode* )
MAGMA_INPUT_NAMES( "Vector" )
MAGMA_DESCRIPTION( "Transforms the input to worldspace from the specified node's space." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_MAX_TYPE( "ToSpace", "Transform", magma_to_space_node )
MAGMA_ENUM_PROPERTY( inputType, "Point", "Vector", "Normal" )
MAGMA_EXPOSE_PROPERTY( node, INode* )
MAGMA_INPUT_NAMES( "Vector (WS)" )
MAGMA_DESCRIPTION( "Transforms the input to the specified node's space from worldspace." )
MAGMA_DEFINE_TYPE_END

void define_max_nodes( frantic::magma::magma_singleton& ms ) {
    ms.define_node_type<magma_input_value_node>();
    ms.define_node_type<magma_from_space_node>();
    ms.define_node_type<magma_to_space_node>();
    ms.define_node_type<magma_input_geometry_node>();
    ms.define_node_type<magma_texmap_op_node>();
    ms.define_node_type<magma_input_particles_node>();
    ms.define_node_type<magma_input_object_node>();
    ms.define_node_type<magma_script_op_node>();
    ms.define_node_type<magma_curve_op_node>();
}

} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic
