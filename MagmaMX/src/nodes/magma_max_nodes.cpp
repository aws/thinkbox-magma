// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/max3d/nodes/magma_curve_op_node.hpp>
#include <frantic/magma/max3d/nodes/magma_geometry_input_node.hpp>
#include <frantic/magma/max3d/nodes/magma_input_object_node.hpp>
#include <frantic/magma/max3d/nodes/magma_input_particles_node.hpp>
#include <frantic/magma/max3d/nodes/magma_input_value_node.hpp>
#include <frantic/magma/max3d/nodes/magma_script_op_node.hpp>
#include <frantic/magma/max3d/nodes/magma_transform_node.hpp>

#include <frantic/magma/max3d/magma_max_node_impl.hpp>

#include <frantic/magma/magma_singleton.hpp>

using frantic::graphics::vector3f;

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

MAGMA_DEFINE_MAX_TYPE( "Curve", "Function", magma_curve_op_node )
MAGMA_READONLY_PROPERTY( curve, FPInterface* )
MAGMA_INPUT( "Scalar", 0.f )
MAGMA_DESCRIPTION( "Uses a curve to transform the input value into another value." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_MAX_TYPE( "InputGeometry", "Input", magma_input_geometry_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_EXPOSE_ARRAY_PROPERTY( nodes, INode* )
MAGMA_OUTPUT_NAMES( "Geometry", "Object Count", "Objects" )
MAGMA_DESCRIPTION( "Exposes the geometry of a node for other operators." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_MAX_TYPE( "InputObject", "Input", magma_input_object_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_EXPOSE_PROPERTY( object, INode* )
MAGMA_DESCRIPTION( "Exposes the properties of an object for other operators." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_MAX_TYPE( "InputParticles", "Input", magma_input_particles_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_EXPOSE_PROPERTY( node, INode* )
MAGMA_OUTPUT_NAMES( "Particles", "Count" )
MAGMA_DESCRIPTION( "Exposes the particles of a Krakatoa PRT object for use with other operators." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_MAX_TYPE( "InputValue", "Input", magma_input_value_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_EXPOSE_PROPERTY( forceInteger, bool )
MAGMA_EXPOSE_PROPERTY( controller, Control* )
MAGMA_DESCRIPTION( "An animatable value of a specified type." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_MAX_TYPE( "InputScript", "Input", magma_script_op_node )
MAGMA_TYPE_ATTR( disableable, false )
MAGMA_EXPOSE_PROPERTY( script, M_STD_STRING )
MAGMA_DESCRIPTION( "Exposes a constant value by evaluating a snippet of MAXScript. Supports "
                   "Integer,Float,Bool,Point3,Color,Quat results." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_MAX_TYPE( "FromSpace", "Transform", magma_from_space_node )
MAGMA_ENUM_PROPERTY( inputType, "Point", "Vector", "Normal" )
MAGMA_EXPOSE_PROPERTY( node, INode* )
MAGMA_INPUT( "Vector", frantic::graphics::vector3f( 0, 0, 0 ) )
MAGMA_INPUT( "Objects", boost::blank() )
MAGMA_INPUT( "ObjIndex", 0 )
MAGMA_DESCRIPTION( "Transforms the input to worldspace from the specified node's space." )
MAGMA_DEFINE_TYPE_END

MAGMA_DEFINE_MAX_TYPE( "ToSpace", "Transform", magma_to_space_node )
MAGMA_ENUM_PROPERTY( inputType, "Point", "Vector", "Normal" )
MAGMA_EXPOSE_PROPERTY( node, INode* )
MAGMA_INPUT( "Vector (WS)", frantic::graphics::vector3f( 0, 0, 0 ) )
MAGMA_INPUT( "Objects", boost::blank() )
MAGMA_INPUT( "ObjIndex", 0 )
MAGMA_DESCRIPTION( "Transforms the input to the specified object's space from worldspace." )
MAGMA_DEFINE_TYPE_END

void define_max_nodes( frantic::magma::magma_singleton& ms ) {
    ms.define_node_type<magma_input_value_node>();
    ms.define_node_type<magma_from_space_node>();
    ms.define_node_type<magma_to_space_node>();
    ms.define_node_type<magma_input_geometry_node>();
    ms.define_node_type<magma_input_particles_node>();
    ms.define_node_type<magma_input_object_node>();
    ms.define_node_type<magma_script_op_node>();
    ms.define_node_type<magma_curve_op_node>();
}

} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic

extern ClassDesc2* GetMagmaInputValueNodeDesc();
extern ClassDesc2* GetMagmaToSpaceNodeDesc();
extern ClassDesc2* GetMagmaFromSpaceNodeDesc();
extern ClassDesc2* GetMagmaInputGeometryNodeDesc();
extern ClassDesc2* GetMagmaInputParticlesNodeDesc();
extern ClassDesc2* GetMagmaInputObjectNodeDesc();
extern ClassDesc2* GetMagmaCurveNodeDesc();
extern ClassDesc2* GetMagmaScriptNodeDesc();

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

// inline void ProcessClassDesc( std::vector<ClassDesc*>& outClassDescs, SubClassList* classList, ClassDesc2* cd ){
//	//We look to see if this CD already exists, and if not we will expose this one. If it does exist, but this one
//	//has a newer version # in the ParamBlock2, then we will delete the other one and replace it with this one. No
//biggie. 	int index = classList->FindClass( GetMagmaInputValueNodeDesc()->ClassID() ); 	if( index < 0 )
//		outClassDescs.push_back( cd );
//	else{
//		ClassDesc* cdOther = (*classList)[index].CD();
//		if( cd->GetParamBlockDesc( 0 )->Version() > cdOther->GetParamBlockDesc( 0 )->Version() ){
//			MCHAR dllName[2][MAX_PATH+1];
//			dllName[0][MAX_PATH] = '\0';
//			dllName[1][MAX_PATH] = '\0';
//
//			GetModuleFileName( GetModuleHandle(NULL), dllName[0], MAX_PATH );
//			GetModuleFileName( GetCOREInterface()->GetDllDir()[(*classList)[index].DllNumber()].handle,
//dllName[1], MAX_PATH );
//
//			std::cerr << "Warning: Class \"" << cdOther->ClassName() << "\" from " << dllName[1] << " replaced
//by \"" << cd->ClassName() << "\" from " << dllName[1] << std::endl;
//
//			classList->DeleteClass( cdOther );
//			outClassDescs.push_back( cd );
//		}
//	}
// }

void GetClassDescs( std::vector<ClassDesc*>& outClassDescs ) {
    /*SubClassList* classList = GetCOREInterface()->GetDllDir().ClassDir().GetClassList( REF_TARGET_CLASS_ID );

    ProcessClassDesc( outClassDescs, classList, GetMagmaInputValueNodeDesc() );
    ProcessClassDesc( outClassDescs, classList, GetMagmaToSpaceNodeDesc() );
    ProcessClassDesc( outClassDescs, classList, GetMagmaFromSpaceNodeDesc() );
    ProcessClassDesc( outClassDescs, classList, GetMagmaInputParticlesNodeDesc() );
    ProcessClassDesc( outClassDescs, classList, GetMagmaInputGeometryNodeDesc() );
    ProcessClassDesc( outClassDescs, classList, GetMagmaInputObjectNodeDesc() );
    ProcessClassDesc( outClassDescs, classList, GetMagmaCurveNodeDesc() );*/

    /*if( classList->FindClass( GetMagmaInputValueNodeDesc()->ClassID() ) < 0 )
            outClassDescs.push_back( GetMagmaInputValueNodeDesc() );
    if( classList->FindClass( GetMagmaToSpaceNodeDesc()->ClassID() ) < 0 )
            outClassDescs.push_back( GetMagmaToSpaceNodeDesc() );
    if( classList->FindClass( GetMagmaFromSpaceNodeDesc()->ClassID() ) < 0 )
            outClassDescs.push_back( GetMagmaFromSpaceNodeDesc() );
    if( classList->FindClass( GetMagmaInputParticlesNodeDesc()->ClassID() ) < 0 )
            outClassDescs.push_back( GetMagmaInputParticlesNodeDesc() );
    if( classList->FindClass( GetMagmaInputGeometryNodeDesc()->ClassID() ) < 0 )
            outClassDescs.push_back( GetMagmaInputGeometryNodeDesc() );
    if( classList->FindClass( GetMagmaInputObjectNodeDesc()->ClassID() ) < 0 )
            outClassDescs.push_back( GetMagmaInputObjectNodeDesc() );
    if( classList->FindClass( GetMagmaCurveNodeDesc()->ClassID() ) < 0 )
            outClassDescs.push_back( GetMagmaCurveNodeDesc() );*/

    outClassDescs.push_back( GetMagmaInputValueNodeDesc() );
    outClassDescs.push_back( GetMagmaToSpaceNodeDesc() );
    outClassDescs.push_back( GetMagmaFromSpaceNodeDesc() );
    outClassDescs.push_back( GetMagmaInputParticlesNodeDesc() );
    outClassDescs.push_back( GetMagmaInputGeometryNodeDesc() );
    outClassDescs.push_back( GetMagmaInputObjectNodeDesc() );
    outClassDescs.push_back( GetMagmaCurveNodeDesc() );
    outClassDescs.push_back( GetMagmaScriptNodeDesc() );
}

} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic
