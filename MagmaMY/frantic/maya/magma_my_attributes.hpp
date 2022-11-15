// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <maya/MDataHandle.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnStringArrayData.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <frantic/maya/animation/animation_data.hpp>
#include <frantic/maya/convert.hpp>

namespace frantic {
namespace maya {
namespace attributes {

inline void delete_maya_attribute( const MString& mayaAttributeName, MFnDependencyNode& depNode ) {
    MStatus status;
    MObject mayaAttribute( depNode.attribute( mayaAttributeName.asChar(), &status ) );

    if( status != MS::kSuccess ) {
        throw std::runtime_error( "delete_maya_attribute could not find " + std::string( mayaAttributeName.asChar() ) +
                                  " attribute." );
    }

    status = depNode.removeAttribute( mayaAttribute );
    if( status != MS::kSuccess ) {
        throw std::runtime_error( "delete_maya_attribute couuld not delete " +
                                  std::string( mayaAttributeName.asChar() ) + " attribute." );
    }
}

template <typename T>
T get_value_from_maya_magma_attr( const frantic::tstring& mayaAttrName, const MFnDependencyNode& depNode ) {
    MStatus status;
    MPlug mayaAttrPlug = depNode.findPlug( frantic::strings::to_string( mayaAttrName ).c_str(), &status );
    if( status != MS::kSuccess ) {
        throw std::runtime_error( "get_value_from_maya_magma_attr could not get the plug (" +
                                  frantic::strings::to_string( mayaAttrName ) + ") from the depNode" );
    }

    T outValue;
    status = mayaAttrPlug.getValue( outValue );
    if( status != MS::kSuccess ) {
        throw std::runtime_error( "get_value_from_maya_magma_attr could not get the value for the plug (" +
                                  frantic::strings::to_string( mayaAttrName ) + ") " );
    }
    return outValue;
}

template <>
std::vector<frantic::tstring>
get_value_from_maya_magma_attr<std::vector<frantic::tstring>>( const frantic::tstring& mayaAttrName,
                                                               const MFnDependencyNode& depNode ) {
    // I can't return an MStringArray object directly.  Returning it will empty out its contents
    MStatus status;

    MPlug mayaAttrPlug = depNode.findPlug( frantic::strings::to_string( mayaAttrName ).c_str(), &status );
    if( status != MS::kSuccess ) {
        throw std::runtime_error( "get_value_from_maya_magma_attr could not get the plug (" +
                                  frantic::strings::to_string( mayaAttrName ) + ") from the depNode" );
    }

    MObject arraydataobject;
    status = mayaAttrPlug.getValue( arraydataobject );
    if( status != MS::kSuccess ) {
        throw std::runtime_error( "get_value_from_maya_magma_attr could not get the value for the plug (" +
                                  frantic::strings::to_string( mayaAttrName ) + ") " );
    }

    MFnStringArrayData arraydata( arraydataobject, &status );
    if( status != MS::kSuccess ) {
        throw std::runtime_error(
            "get_value_from_maya_magma_attr could not convert the value to a string array data (" +
            frantic::strings::to_string( mayaAttrName ) + ") " );
    }

    MStringArray marray = arraydata.array( &status );
    if( status != MS::kSuccess ) {
        throw std::runtime_error( "get_value_from_maya_magma_attr could not read the string array value (" +
                                  frantic::strings::to_string( mayaAttrName ) + ") " );
    }

    return from_maya_t( marray );
}

template <>
frantic::graphics::vector3f
get_value_from_maya_magma_attr<frantic::graphics::vector3f>( const frantic::tstring& mayaAttrName,
                                                             const MFnDependencyNode& depNode ) {
    frantic::tstring mayaAttrName0 = mayaAttrName + _T( "0" );
    frantic::tstring mayaAttrName1 = mayaAttrName + _T( "1" );
    frantic::tstring mayaAttrName2 = mayaAttrName + _T( "2" );

    float x = get_value_from_maya_magma_attr<float>( mayaAttrName0, depNode );
    float y = get_value_from_maya_magma_attr<float>( mayaAttrName1, depNode );
    float z = get_value_from_maya_magma_attr<float>( mayaAttrName2, depNode );

    return frantic::graphics::vector3f( x, y, z );
}

template <typename T>
void set_value_from_maya_magma_attr( const frantic::tstring& mayaAttrName, const MFnDependencyNode& depNode, T value ) {
    MStatus status;
    MPlug mayaAttrPlug = depNode.findPlug( frantic::strings::to_string( mayaAttrName ).c_str(), &status );
    if( status != MS::kSuccess ) {
        throw std::runtime_error( "set_value_from_maya_magma_attr could not get the plug (" +
                                  frantic::strings::to_string( mayaAttrName ) + ") from the depNode" );
    }

    status = mayaAttrPlug.setValue( value );
    if( status != MS::kSuccess ) {
        throw std::runtime_error( "set_value_from_maya_magma_attr could not set the value for the plug (" +
                                  frantic::strings::to_string( mayaAttrName ) + ") " );
    }
}

template <>
void set_value_from_maya_magma_attr<std::vector<frantic::tstring>>( const frantic::tstring& mayaAttrName,
                                                                    const MFnDependencyNode& depNode,
                                                                    std::vector<frantic::tstring> value ) {
    MStatus status;

    MPlug mayaAttrPlug = depNode.findPlug( frantic::strings::to_string( mayaAttrName ).c_str(), &status );
    if( status != MS::kSuccess ) {
        throw std::runtime_error( "set_value_from_maya_magma_attr could not get the plug (" +
                                  frantic::strings::to_string( mayaAttrName ) + ") from the depNode" );
    }

    MStringArray marray = to_maya_t( value );
    MDataHandle arrayHandle;
    status = mayaAttrPlug.getValue( arrayHandle );
    if( status != MS::kSuccess ) {
        throw std::runtime_error( "set_value_from_maya_magma_attr could not get the value for the plug (" +
                                  frantic::strings::to_string( mayaAttrName ) + ") " );
    }

    MObject arraydataobject = arrayHandle.data();
    MFnStringArrayData arraydata( arraydataobject, &status );
    if( status != MS::kSuccess ) {
        throw std::runtime_error( "set_value_from_maya_magma_attr could not read the value for the plug (" +
                                  frantic::strings::to_string( mayaAttrName ) + ") " );
    }

    status = arraydata.set( marray );
    if( status != MS::kSuccess ) {
        throw std::runtime_error( "set_value_from_maya_magma_attr could not assign the value for the plug (" +
                                  frantic::strings::to_string( mayaAttrName ) + ") " );
    }

    status = mayaAttrPlug.setValue( arrayHandle );
    if( status != MS::kSuccess ) {
        throw std::runtime_error( "set_value_from_maya_magma_attr could not set the value for the plug (" +
                                  frantic::strings::to_string( mayaAttrName ) + ") " );
    }
}

template <>
void set_value_from_maya_magma_attr<frantic::graphics::vector3f>( const frantic::tstring& mayaAttrName,
                                                                  const MFnDependencyNode& depNode,
                                                                  frantic::graphics::vector3f value ) {
    frantic::tstring mayaAttrName0 = mayaAttrName + _T( "0" );
    frantic::tstring mayaAttrName1 = mayaAttrName + _T( "1" );
    frantic::tstring mayaAttrName2 = mayaAttrName + _T( "2" );

    set_value_from_maya_magma_attr<float>( mayaAttrName0, depNode, value.x );
    set_value_from_maya_magma_attr<float>( mayaAttrName1, depNode, value.y );
    set_value_from_maya_magma_attr<float>( mayaAttrName2, depNode, value.z );
}

void set_animation_from_maya_attr( const frantic::tstring& mayaAttrName, const MFnDependencyNode& depNode,
                                   const frantic::maya::animation::animation_data& value ) {

    MStatus status;
    MPlug mayaAttrPlug = depNode.findPlug( frantic::strings::to_string( mayaAttrName ).c_str(), &status );
    if( status != MS::kSuccess ) {
        throw std::runtime_error( "set_animation_from_maya_attr could not get the plug (" +
                                  frantic::strings::to_string( mayaAttrName ) + ") from the depNode" );
    }

    bool ok = value.applyToCurve( mayaAttrPlug );
    if( !ok ) {
        throw std::runtime_error( "set_animation_from_maya_attr error setting animation for (" +
                                  frantic::strings::to_string( mayaAttrName ) + ")" );
    }
}

frantic::maya::animation::animation_data get_animation_from_maya_attr( const frantic::tstring& mayaAttrName,
                                                                       const MFnDependencyNode& depNode ) {

    MStatus status;
    MPlug mayaAttrPlug = depNode.findPlug( frantic::strings::to_string( mayaAttrName ).c_str(), &status );
    if( status != MS::kSuccess ) {
        throw std::runtime_error( "get_animation_from_maya_attr could not get the plug (" +
                                  frantic::strings::to_string( mayaAttrName ) + ") from the depNode" );
    }

    frantic::maya::animation::animation_data value;
    bool ok = value.loadFromCurve( mayaAttrPlug );
    if( !ok ) {
        bool hasAnim = frantic::maya::animation::animation_data::has_animation( mayaAttrPlug );
        if( hasAnim ) {
            throw std::runtime_error( "get_animation_from_maya_attr error getting animation for (" +
                                      frantic::strings::to_string( mayaAttrName ) + ")" );
        }
    }

    return value;
}

void set_animation_from_maya_attr_vec3( const frantic::tstring& mayaAttrName, const MFnDependencyNode& depNode,
                                        const frantic::maya::animation::animation_data_vector3& value ) {
    frantic::tstring mayaAttrName0 = mayaAttrName + _T( "0" );
    frantic::tstring mayaAttrName1 = mayaAttrName + _T( "1" );
    frantic::tstring mayaAttrName2 = mayaAttrName + _T( "2" );

    set_animation_from_maya_attr( mayaAttrName0, depNode, value.x() );
    set_animation_from_maya_attr( mayaAttrName1, depNode, value.y() );
    set_animation_from_maya_attr( mayaAttrName2, depNode, value.z() );
}

frantic::maya::animation::animation_data_vector3
get_animation_from_maya_attr_vec3( const frantic::tstring& mayaAttrName, const MFnDependencyNode& depNode ) {
    frantic::tstring mayaAttrName0 = mayaAttrName + _T( "0" );
    frantic::tstring mayaAttrName1 = mayaAttrName + _T( "1" );
    frantic::tstring mayaAttrName2 = mayaAttrName + _T( "2" );

    frantic::maya::animation::animation_data xVal = get_animation_from_maya_attr( mayaAttrName0, depNode );
    frantic::maya::animation::animation_data yVal = get_animation_from_maya_attr( mayaAttrName1, depNode );
    frantic::maya::animation::animation_data zVal = get_animation_from_maya_attr( mayaAttrName2, depNode );
    frantic::maya::animation::animation_data_vector3 value( xVal, yVal, zVal );

    return value;
}

} // namespace attributes
} // namespace maya
} // namespace frantic
