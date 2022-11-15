// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"
#include <frantic/magma/nodes/magma_input_particles_interface.hpp>
#include <frantic/magma/nodes/magma_mux_node.hpp>
#include <frantic/magma/nodes/magma_node_impl.hpp>
#include <frantic/magma/nodes/magma_standard_operators.hpp>

#include <frantic/magma/magma_singleton.hpp>

#ifndef TRUE
#define TRUE 1
#endif

namespace frantic {
namespace magma {
namespace nodes {

MAGMA_DEFINE_TYPE( "Elbow", "System", magma_elbow_node )
MAGMA_INPUT_NAMES( "" )
MAGMA_OUTPUT_NAMES( "" )
MAGMA_DESCRIPTION( "A permanent pass-through node for routing wires along custom paths." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Missing", "System", magma_missing_node )
MAGMA_TYPE_ATTR( public, false )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_DESCRIPTION( "Whatever type this node was supposed to be, we totally lost it." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Negate", "Arithmetic", magma_negate_node )
MAGMA_INPUT_NAMES( "Value" )
MAGMA_DESCRIPTION( "Flips the sign of the input." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Abs", "Arithmetic", magma_abs_node )
MAGMA_INPUT_NAMES( "Value" )
MAGMA_DESCRIPTION( "Returns the absolute value of the input." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "ComponentSum", "Vector", magma_component_sum_node )
MAGMA_INPUT_NAMES( "Vector" )
MAGMA_DESCRIPTION( "Returns the sum of the individual components of a vector. Ex. ComponentSum( [1,2,3] ) = 6" )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Magnitude", "Vector", magma_magnitude_node )
MAGMA_INPUT_NAMES( "Vector" )
MAGMA_DESCRIPTION( "Returns the magnitude of the given vector." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Normalize", "Vector", magma_normalize_node )
MAGMA_INPUT_NAMES( "Vector" )
MAGMA_DESCRIPTION( "Normalizes a vector by dividing by its magnitude so that the result's magnitude is 1." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Floor", "Arithmetic", magma_floor_node )
MAGMA_INPUT_NAMES( "Value" )
MAGMA_DESCRIPTION( "Returns the closest whole number that is less than or equal to the input." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Ceil", "Arithmetic", magma_ceil_node )
MAGMA_INPUT_NAMES( "Value" )
MAGMA_DESCRIPTION( "Returns the closest whole number that is greater than or equal to the input." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Sqrt", "Arithmetic", magma_sqrt_node )
MAGMA_INPUT_NAMES( "Value" )
MAGMA_DESCRIPTION( "Returns the positive square root of the input." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Log", "Arithmetic", magma_log_node )
MAGMA_INPUT_NAMES( "Base", "Value" )
MAGMA_DESCRIPTION( "Returns the natural logarithm of the input. Given x, returns y such that x = e ^ y." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Cos", "Trigonometry", magma_cos_node )
MAGMA_INPUT_NAMES( "Radians" )
MAGMA_DESCRIPTION( "Returns the cosine function for the given angle (in radians)." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "ACos", "Trigonometry", magma_acos_node )
MAGMA_INPUT_NAMES( "Value" )
MAGMA_DESCRIPTION( "Returns the inverse of the cosine function, returning an angle (in radians). Input must be in "
                   "[-1,1], result is in [0,pi]." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Sin", "Trigonometry", magma_sin_node )
MAGMA_INPUT_NAMES( "Radians" )
MAGMA_DESCRIPTION( "Returns the sine function for the given angle (in radians)." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "ASin", "Trigonometry", magma_asin_node )
MAGMA_INPUT_NAMES( "Value" )
MAGMA_DESCRIPTION( "Returns the inverse of the sine function, returning an angle (in radians). Input must be in "
                   "[-1,1], result is in [-pi/2,pi/2]." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Tan", "Trigonometry", magma_tan_node )
MAGMA_INPUT_NAMES( "Radians" )
MAGMA_DESCRIPTION( "Returns the tangent function ( sin(x)/cos(x) ) for the given angle (in radians)." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "ATan", "Trigonometry", magma_atan_node )
MAGMA_INPUT_NAMES( "Value" )
MAGMA_DESCRIPTION(
    "Returns the inverse of the tangent function, returning an angle (in radians). Result is in [-pi/2,pi/2]." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "LogicalNot", "Logic", magma_logical_not_node )
MAGMA_INPUT_NAMES( "Boolean" )
MAGMA_DESCRIPTION( "Returns the opposite boolean value." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "ToFloat", "Convert", magma_to_float_node )
MAGMA_INPUT_NAMES( "Scalar" )
MAGMA_DESCRIPTION( "Converts the input scalar (ie. single value) to a floating point number." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "ToInt", "Convert", magma_to_int_node )
MAGMA_INPUT_NAMES( "Scalar" )
MAGMA_DESCRIPTION( "Converts the input scalar (ie. single value) to an integer, truncating any decimal portion." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "ToWorld", "Transform", magma_to_world_node )
MAGMA_ENUM_PROPERTY( inputType, "Point", "Vector", "Normal" )
MAGMA_INPUT_NAMES( "Vector (OS)" )
MAGMA_OUTPUT_NAMES( "World Space" )
MAGMA_DESCRIPTION( "Transforms the input to worldspace from the initial particle space." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "FromWorld", "Transform", magma_from_world_node )
MAGMA_ENUM_PROPERTY( inputType, "Point", "Vector", "Normal" )
MAGMA_INPUT_NAMES( "Vector (WS)" )
MAGMA_OUTPUT_NAMES( "Object Space" )
MAGMA_DESCRIPTION( "Transforms the input from worldspace back to the initial particle space." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "ToCamera", "Transform", magma_to_camera_node )
MAGMA_ENUM_PROPERTY( inputType, "Point", "Vector", "Normal" )
MAGMA_INPUT_NAMES( "Vector (WS)" )
MAGMA_OUTPUT_NAMES( "Camera Space" )
MAGMA_DESCRIPTION( "Transforms the input from worldspace to camera space." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "FromCamera", "Transform", magma_from_camera_node )
MAGMA_ENUM_PROPERTY( inputType, "Point", "Vector", "Normal" )
MAGMA_INPUT_NAMES( "Vector (CS)" )
MAGMA_OUTPUT_NAMES( "World Space" )
MAGMA_DESCRIPTION( "Transforms the input from cameraspace back to worldspace." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Breakout", "Convert", magma_breakout_node )
MAGMA_INPUT_NAMES( "Vector" )
MAGMA_OUTPUT_NAMES( "X", "Y", "Z" )
MAGMA_DESCRIPTION( "Allows access to the individual components of a vector." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Noise", "Function", magma_noise_node )
MAGMA_EXPOSE_PROPERTY( numOctaves, int )
MAGMA_EXPOSE_PROPERTY( lacunarity, float )
MAGMA_EXPOSE_PROPERTY( normalize, bool )
MAGMA_INPUT_NAMES( "Value" )
MAGMA_INPUT( "Phase", 0.f )
MAGMA_DESCRIPTION( "Computes a procedural noise scalar value." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "VecNoise", "Function", magma_vecnoise_node )
MAGMA_EXPOSE_PROPERTY( numOctaves, int )
MAGMA_EXPOSE_PROPERTY( lacunarity, float )
MAGMA_EXPOSE_PROPERTY( normalize, bool )
MAGMA_INPUT_NAMES( "Vector" )
MAGMA_INPUT( "Phase", 0.f )
MAGMA_DESCRIPTION( "Computes a procedural noise vector value." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "UniformRandom", "Function", magma_random_node )
MAGMA_INPUT_NO_DEFAULT( "Seed" )
MAGMA_INPUT( "Min. Value", 0.f )
MAGMA_INPUT( "Max. Value", 1.f )
MAGMA_DESCRIPTION( "Computes a pseudorandom value using the uniform distribution [Min, Max)." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "VecUniformRandom", "Function", magma_vecrandom_node )
MAGMA_INPUT_NO_DEFAULT( "Seed" )
MAGMA_INPUT( "Min. Value", 0.f )
MAGMA_INPUT( "Max. Value", 1.f )
MAGMA_DESCRIPTION( "Computes a pseudorandom vector value [Min, Max)." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "ExponentialRandom", "Function", magma_exponential_random_node )
MAGMA_INPUT_NO_DEFAULT( "Seed" )
MAGMA_INPUT( "Lambda", 1.f )
MAGMA_DESCRIPTION( "Computes a pseudorandom value using an exponential distribution." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "WeibullRandom", "Function", magma_weibull_random_node )
MAGMA_INPUT_NO_DEFAULT( "Seed" )
MAGMA_INPUT( "Lambda", 1.f )
MAGMA_INPUT( "Kappa", 1.f )
MAGMA_DESCRIPTION( "Computes a pseudorandom value using a Weibull distribution." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "GaussianRandom", "Function", magma_gaussian_random_node )
MAGMA_INPUT_NO_DEFAULT( "Seed" )
MAGMA_INPUT( "Mean", 0.f )
MAGMA_INPUT( "Sigma", 1.f )
MAGMA_DESCRIPTION( "Computes a pseudorandom value using a Gaussian (normal) distribution." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "TriangleRandom", "Function", magma_triangle_random_node )
MAGMA_INPUT_NO_DEFAULT( "Seed" )
MAGMA_INPUT( "Min.", 0.f )
MAGMA_INPUT( "Probable", 0.5f )
MAGMA_INPUT( "Max.", 1.f )
MAGMA_DESCRIPTION( "Computes a pseudorandom value using a triangle distribution." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "UniformOnSphere", "Function", magma_uniform_on_sphere_random_node )
MAGMA_INPUT_NO_DEFAULT( "Seed" )
MAGMA_INPUT( "Dimensions", 3 )
MAGMA_DESCRIPTION( "Computes a pseudorandom vector on the surface of a unit sphere." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Add", "Arithmetic", magma_add_node )
// MAGMA_INPUT_NAMES( "Left Value", "Right Value" )
// MAGMA_INPUT_DEFAULTS( variant_t(), variant_t(0.f) );
MAGMA_INPUT( "Left Value", 0.f )
MAGMA_INPUT( "Right Value", 0.f )
MAGMA_DESCRIPTION( "Returns the sum of the two inputs." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Subtract", "Arithmetic", magma_sub_node )
// MAGMA_INPUT_NAMES( "Left Value", "Right Value" )
// MAGMA_INPUT_DEFAULTS( variant_t(), variant_t(0.f) );
MAGMA_INPUT( "Left Value", 1.f )
MAGMA_INPUT( "Right Value", 0.f )
MAGMA_DESCRIPTION( "Returns the difference between the two inputs." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Multiply", "Arithmetic", magma_mul_node )
// MAGMA_INPUT_NAMES( "Left Value", "Right Value" )
// MAGMA_INPUT_DEFAULTS( variant_t(), variant_t(1.f) );
MAGMA_INPUT( "Left Value", 1.f )
MAGMA_INPUT( "Right Value", 1.f )
MAGMA_DESCRIPTION( "Returns the product of the two inputs." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Divide", "Arithmetic", magma_div_node )
// MAGMA_INPUT_NAMES( "Dividend", "Divisor" )
// MAGMA_INPUT_DEFAULTS( variant_t(), variant_t(1.f) );
MAGMA_INPUT( "Dividend", 1.f )
MAGMA_INPUT( "Divisor", 1.f )
MAGMA_DESCRIPTION( "Returns the quotient of the two inputs." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Modulo", "Arithmetic", magma_mod_node )
MAGMA_INPUT( "Dividend", 0 )
MAGMA_INPUT( "Divisor", 1 )
MAGMA_DESCRIPTION( "Returns the remainder of the quotient of the two inputs." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Power", "Arithmetic", magma_pow_node )
// MAGMA_INPUT_NAMES( "Base", "Exponent" )
// MAGMA_INPUT_DEFAULTS( variant_t(), variant_t(1.f) );
MAGMA_INPUT( "Base", 1.f )
MAGMA_INPUT( "Exponent", 1.f )
MAGMA_DESCRIPTION( "Returns 'Base' multiplied by itself 'Exponent' times." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "ATan2", "Trigonometry", magma_atan2_node )
MAGMA_INPUT_NAMES( "Dividend", "Divisor" )
MAGMA_DESCRIPTION(
    "Returns the inverse of the tangent function of the quotient of the two inputs, returning an angle (in radians). "
    "Uses the sign of each input to choose the appropriate quadrant. Result is in [-pi,pi]" )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "VectorDot", "Vector", magma_dot_node )
MAGMA_INPUT_NAMES( "Left Vector", "Right Vector" )
MAGMA_DESCRIPTION( "Returns the inner product of the two input vectors." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "VectorCross", "Vector", magma_cross_node )
MAGMA_INPUT_NAMES( "Left Vector", "Right Vector" )
MAGMA_DESCRIPTION( "Returns the cross product of the two input vectors. Result will be orthogonal to both inputs." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Less", "Logic", magma_less_node )
MAGMA_EXPOSE_PROPERTY( useTolerance, bool )
MAGMA_EXPOSE_PROPERTY( toleranceExp, int )
MAGMA_INPUT( "Left Scalar", boost::blank() )
MAGMA_INPUT( "Right Scalar", 0.f )
MAGMA_DESCRIPTION( "Returns true if the first value is strictly less than the second. Otherwise false." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "LessOrEqual", "Logic", magma_lesseq_node )
MAGMA_EXPOSE_PROPERTY( useTolerance, bool )
MAGMA_EXPOSE_PROPERTY( toleranceExp, int )
MAGMA_INPUT( "Left Scalar", boost::blank() )
MAGMA_INPUT( "Right Scalar", 0.f )
MAGMA_DESCRIPTION( "Returns true if the first value is less than or equal to the second. Otherwise false." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Greater", "Logic", magma_greater_node )
MAGMA_EXPOSE_PROPERTY( useTolerance, bool )
MAGMA_EXPOSE_PROPERTY( toleranceExp, int )
MAGMA_INPUT( "Left Scalar", boost::blank() )
MAGMA_INPUT( "Right Scalar", 0.f )
MAGMA_DESCRIPTION( "Returns true if the first value is strictly greater than the second. Otherwise false." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "GreaterOrEqual", "Logic", magma_greatereq_node )
MAGMA_EXPOSE_PROPERTY( useTolerance, bool )
MAGMA_EXPOSE_PROPERTY( toleranceExp, int )
MAGMA_INPUT( "Left Scalar", boost::blank() )
MAGMA_INPUT( "Right Scalar", 0.f )
MAGMA_DESCRIPTION( "Returns true if the first value is greater than or equal to the second. Otherwise false." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Equal", "Logic", magma_equal_node )
MAGMA_EXPOSE_PROPERTY( useTolerance, bool )
MAGMA_EXPOSE_PROPERTY( toleranceExp, int )
MAGMA_INPUT( "Left Scalar", boost::blank() )
MAGMA_INPUT( "Right Scalar", 0.f )
MAGMA_DESCRIPTION( "Returns true if the first value is equal to the second. Otherwise false." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "NotEqual", "Logic", magma_notequal_node )
MAGMA_EXPOSE_PROPERTY( useTolerance, bool )
MAGMA_EXPOSE_PROPERTY( toleranceExp, int )
MAGMA_INPUT( "Left Scalar", boost::blank() )
MAGMA_INPUT( "Right Scalar", 0.f )
MAGMA_DESCRIPTION( "Returns true if the first value is not equal to the second. Otherwise false." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "LogicalAnd", "Logic", magma_logical_and_node )
MAGMA_INPUT_NAMES( "Boolean", "Boolean" )
MAGMA_DESCRIPTION( "Returns true if both inputs are true. Otherwise false." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "LogicalOr", "Logic", magma_logical_or_node )
MAGMA_INPUT_NAMES( "Boolean", "Boolean" )
MAGMA_DESCRIPTION( "Returns true if either input is true. Otherwise false." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "LogicalXor", "Logic", magma_logical_xor_node )
MAGMA_INPUT_NAMES( "Boolean", "Boolean" )
MAGMA_DESCRIPTION( "Returns true if only one of the inputs is true. Otherwise false." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "TransformByQuat", "Transform", magma_quat_mul_node )
MAGMA_INPUT( "Quat", frantic::graphics::quat4f() )
MAGMA_INPUT( "QuatOrVector", frantic::graphics::vector3f( 0 ) )
MAGMA_DESCRIPTION( "Transforms a quaternion or vector by another quaternion." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "NearestPoint", "Object", magma_nearest_point_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_INPUT_NAMES( "Geometry", "Lookup Point (WS)" )
MAGMA_INPUT( "Ignore Backfaces", 0 )
MAGMA_OUTPUT_NAMES( "Position", "IsValid", "ObjIndex", "FaceIndex", "Distance", "Normal", "Bary Coords" )
MAGMA_DESCRIPTION(
    "Finds the closest point on the geometry to the given lookup point. The lookup point is expected in worldspace." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "IntersectRay", "Object", magma_intersection_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_INPUT_NAMES( "Geometry", "Ray Origin (WS)", "Ray Direction (WS)" )
MAGMA_INPUT( "Ignore Backfaces", 0 )
MAGMA_OUTPUT_NAMES( "Position", "IsValid", "ObjIndex", "FaceIndex", "Distance", "Normal", "Bary Coords" )
MAGMA_DESCRIPTION(
    "Finds the closest intersection of the ray with given origin and direction on the geometry. The ray origin and ray "
    "direction is expected in worldspace. The output distance is scaled by the length of the ray direction. " )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "InVolume", "Object", magma_in_volume_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_INPUT_NAMES( "Geometry", "Lookup Point (WS)" )
MAGMA_OUTPUT_NAMES( "IsInVolume" )
MAGMA_DESCRIPTION( "Determines if a point is inside a volume. The lookup point is expected in worldspace." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "NearestParticle", "Object", magma_nearest_particle_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_EXPOSE_PROPERTY( whichNearest, int )
MAGMA_INPUT_NAMES( "Particles", "Lookup Point (WS)" )
MAGMA_OUTPUT_NAMES( "Position", "IsValid", "ParticleIndex" )
MAGMA_DESCRIPTION( "Finds the closest particle to the lookup point. The lookup point is expected in worldspace." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Blend", "Function", magma_blend_node )
MAGMA_INPUT( "Value At 0", boost::blank() )
MAGMA_INPUT( "Value At 1", boost::blank() )
MAGMA_INPUT( "Blend Amount", 0.f )
MAGMA_DESCRIPTION( "Does a linear combination of the first two inputs. 'Blend Amount' should be in [0,1]." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Clamp", "Function", magma_clamp_node )
MAGMA_INPUT( "Value", boost::blank() )
MAGMA_INPUT( "Min", 0.f )
MAGMA_INPUT( "Max", 1.f )
MAGMA_DESCRIPTION( "Forces the input to be in the range [Min, Max]." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "Switch", "Logic", magma_logicswitch_node )
MAGMA_INPUT( "If True", boost::blank() )
MAGMA_INPUT( "If False", boost::blank() )
MAGMA_INPUT( "Boolean", TRUE )
MAGMA_DESCRIPTION( "Picks an input based on a True/False value." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "ToVector", "Convert", magma_to_vector_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_INPUT( "X", 0.f )
MAGMA_INPUT( "Y", 0.f )
MAGMA_INPUT( "Z", 0.f )
MAGMA_DESCRIPTION( "Creates a vector from three float values." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "MatrixMulVec", "Vector", magma_matrix_mul_node )
MAGMA_INPUT( "Vec", frantic::graphics::vector3f( 0, 0, 0 ) )
MAGMA_INPUT( "Col1", frantic::graphics::vector3f( 1, 0, 0 ) )
MAGMA_INPUT( "Col2", frantic::graphics::vector3f( 0, 1, 0 ) );
MAGMA_INPUT( "Col3", frantic::graphics::vector3f( 0, 0, 1 ) );
MAGMA_DESCRIPTION( "Multiplies a 3x3 matrix created from the supplied column vectors, by the first vector." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "VectorsToQuat", "Convert", magma_vectors_to_quat_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_INPUT( "X", frantic::graphics::vector3f::from_xaxis() )
MAGMA_INPUT( "Y", frantic::graphics::vector3f::from_yaxis() )
MAGMA_INPUT( "Z", frantic::graphics::vector3f::from_zaxis() )
MAGMA_DESCRIPTION( "Creates a quaternion from an orthonormal axis system. (ie. A 3x3 rotation matrix)" )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "EulerAnglesToQuat", "Convert", magma_eulerangles_to_quat_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_INPUT( "XAngleRadians", 0.f )
MAGMA_INPUT( "YAngleRadians", 0.f )
MAGMA_INPUT( "ZAngleRadians", 0.f )
MAGMA_DESCRIPTION( "Creates a quaternion from an euler angles (in radians!) rotation. The rotation is done around the "
                   "X, then Y, then Z axes." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "AngleAxisToQuat", "Convert", magma_angleaxis_to_quat_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_INPUT( "AngleRadians", 0.f )
MAGMA_INPUT( "Axis", frantic::graphics::vector3f::from_zaxis() )
MAGMA_DESCRIPTION( "Creates a quaternion from an axis and an angle (in radians!) to rotate around it." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_TYPE( "QuatToVectors", "Convert", magma_quat_to_vectors_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_INPUT( "Quat", frantic::graphics::quat4f() )
MAGMA_OUTPUT_NAMES( "X", "Y", "Z" )
MAGMA_DESCRIPTION( "Extracts the 3 orthonormal axes of an orientation quaternion. (ie. A 3x3 rotation matrix)" )
MAGMA_DEFINE_TYPE_END

magma_to_world_node::magma_to_world_node() { m_inputType = _T("Point"); }

magma_from_world_node::magma_from_world_node() { m_inputType = _T("Point"); }

magma_to_camera_node::magma_to_camera_node() { m_inputType = _T("Point"); }

magma_from_camera_node::magma_from_camera_node() { m_inputType = _T("Point"); }

magma_noise_node::magma_noise_node() {
    m_numOctaves = 4;
    m_lacunarity = 0.5f;
    m_normalize = true;
}

magma_vecnoise_node::magma_vecnoise_node() {
    m_numOctaves = 4;
    m_lacunarity = 0.5f;
    m_normalize = true;
}

// magma_nearest_particle_node::magma_nearest_particle_node(){
//	m_whichNearest = 1;
// }

void define_standard_operators( magma_singleton& ms ) {
    ms.define_node_type<magma_elbow_node>();
    ms.define_node_type<magma_missing_node>();

    ms.define_node_type<magma_negate_node>();
    ms.define_node_type<magma_abs_node>();
    ms.define_node_type<magma_component_sum_node>();
    ms.define_node_type<magma_magnitude_node>();
    ms.define_node_type<magma_normalize_node>();
    ms.define_node_type<magma_floor_node>();
    ms.define_node_type<magma_ceil_node>();
    ms.define_node_type<magma_sqrt_node>();
    ms.define_node_type<magma_log_node>();
    ms.define_node_type<magma_cos_node>();
    ms.define_node_type<magma_acos_node>();
    ms.define_node_type<magma_sin_node>();
    ms.define_node_type<magma_asin_node>();
    ms.define_node_type<magma_tan_node>();
    ms.define_node_type<magma_atan_node>();
    ms.define_node_type<magma_logical_not_node>();
    ms.define_node_type<magma_to_float_node>();
    ms.define_node_type<magma_to_int_node>();
    ms.define_node_type<magma_breakout_node>();
    ms.define_node_type<magma_noise_node>();
    ms.define_node_type<magma_vecnoise_node>();
    ms.define_node_type<magma_random_node>();
    ms.define_node_type<magma_vecrandom_node>();
    ms.define_node_type<magma_exponential_random_node>();
    ms.define_node_type<magma_weibull_random_node>();
    ms.define_node_type<magma_gaussian_random_node>();
    ms.define_node_type<magma_triangle_random_node>();
    ms.define_node_type<magma_uniform_on_sphere_random_node>();
    ms.define_node_type<magma_to_world_node>();
    ms.define_node_type<magma_from_world_node>();
    ms.define_node_type<magma_quat_to_vectors_node>();

    ms.define_node_type<magma_add_node>();
    ms.define_node_type<magma_sub_node>();
    ms.define_node_type<magma_mul_node>();
    ms.define_node_type<magma_div_node>();
    ms.define_node_type<magma_mod_node>();
    ms.define_node_type<magma_pow_node>();
    ms.define_node_type<magma_atan2_node>();
    ms.define_node_type<magma_dot_node>();
    ms.define_node_type<magma_cross_node>();
    ms.define_node_type<magma_less_node>();
    ms.define_node_type<magma_lesseq_node>();
    ms.define_node_type<magma_greater_node>();
    ms.define_node_type<magma_greatereq_node>();
    ms.define_node_type<magma_equal_node>();
    ms.define_node_type<magma_notequal_node>();
    ms.define_node_type<magma_logical_and_node>();
    ms.define_node_type<magma_logical_or_node>();
    ms.define_node_type<magma_logical_xor_node>();
    ms.define_node_type<magma_nearest_point_node>();
    ms.define_node_type<magma_intersection_node>();
    ms.define_node_type<magma_in_volume_node>();
    ms.define_node_type<magma_nearest_particle_node>();
    ms.define_node_type<magma_nearest_particles_avg_node>();
    ms.define_node_type<magma_particle_kernel_node>();
    ms.define_node_type<magma_quat_mul_node>();

    ms.define_node_type<magma_blend_node>();
    ms.define_node_type<magma_clamp_node>();
    ms.define_node_type<magma_logicswitch_node>();
    ms.define_node_type<magma_to_vector_node>();
    ms.define_node_type<magma_face_query_node>();
    ms.define_node_type<magma_vertex_query_node>();
    ms.define_node_type<magma_element_query_node>();
    ms.define_node_type<magma_mesh_query_node>();
    ms.define_node_type<magma_particle_query_node>();
    ms.define_node_type<magma_object_query_node>();
    ms.define_node_type<magma_vectors_to_quat_node>();
    ms.define_node_type<magma_eulerangles_to_quat_node>();
    ms.define_node_type<magma_angleaxis_to_quat_node>();

    ms.define_node_type<magma_matrix_mul_node>();

    ms.define_node_type<magma_mux_node>();

    // ms.define_node_type<magma_neighbor_particles_node>();
}

} // namespace nodes
} // namespace magma
} // namespace frantic

namespace frantic {
namespace magma {
namespace functors {

frantic::channels::channel_map quat_to_vectors::s_staticMap;

}
} // namespace magma
} // namespace frantic
