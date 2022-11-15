// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <boost/blank.hpp>
#include <frantic/strings/tstring.hpp>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnStringArrayData.h>

#include <frantic/graphics/quat4f.hpp>
#include <frantic/graphics/vector3f.hpp>

#include "frantic/convert/tstring.hpp"
#include "frantic/maya/magma_my_attributes.hpp"

#include "frantic/magma/maya/maya_magma_attr_manager.hpp"
#include "frantic/magma/maya/maya_magma_description.hpp"
#include "frantic/magma/maya/maya_magma_exception.hpp"
#include "frantic/magma/maya/maya_magma_info_factory.hpp"

#include "frantic/maya/convert.hpp"

namespace frantic {
namespace magma {
namespace maya {
namespace attr {

namespace detail {

/// TODO refactor this file
/// TODO add error checking

/// - please ensure the uniqueness of the name of a maya attribute
/// - the name of maya attribute can not start with a numeric character otherwise maya could not recognize when reload a
/// saved scene
/// - magmaid_nodeType_{property,inputSocket}index_{(propertyName, if it's a property), (dataType, if it's a
/// inputSocket)}
frantic::tstring get_maya_general_attr_name( const frantic::tstring& nodeType, desc::desc_id descID, bool isProperty,
                                             int index, const frantic::tstring& suffixName ) {
    frantic::tstring outAttrName;
    outAttrName += nodeType + _T( "_" ) + frantic::convert::to_tstring( descID );
    outAttrName += ( isProperty ? _T( "_Property" ) : _T( "_InputSocket" ) ) + frantic::convert::to_tstring( index );
    outAttrName += _T( "_" ) + suffixName;
    return outAttrName;
}

frantic::tstring find_tstring_if_vector_match( const std::vector<frantic::tstring>& inputSocketAttrNames,
                                               const frantic::tstring& key ) {
    frantic::tstring outTString;
    std::vector<frantic::tstring>::const_iterator cit;
    for( cit = inputSocketAttrNames.begin(); cit != inputSocketAttrNames.end(); cit++ ) {
#ifdef FRANTIC_USE_WCHAR
        if( cit->find( key ) != frantic::tstring::npos ) {
#else
        if( cit->find( key ) != std::string::npos ) {
#endif
            outTString = *cit;
            break;
        }
    }
    return outTString;
}

bool is_tstring_contain( const frantic::tstring& mayaAttrName, const frantic::tstring& targetString ) {
    bool outResult = false;

#ifdef FRANTIC_USE_WCHAR
    if( mayaAttrName.find( targetString ) != frantic::tstring::npos ) {
#else
    if( mayaAttrName.find( targetString ) != std::string::npos ) {
#endif
        outResult = true;
    }
    return outResult;
}

/// because maya internally treat enum attributes as a int attribute so that if we create three enum attributes of a
/// children of a componund attribute maya can not display them properly; UI will display them as a vector3f maya
/// attribute. Therefore, we simply do not create a compound attribute.
void create_property_maya_attr( const info::maya_magma_node_info& nodeInfo, desc::maya_magma_desc_ptr desc,
                                desc::desc_id descID, MFnDependencyNode& depNode ) {
    MStatus status;
    MFnEnumAttribute fnEnumAttribute;
    MFnTypedAttribute fnTypedAttribute;
    MFnNumericAttribute fnNumAttribute;
    MFnStringData fnStringData;
    MFnStringArrayData fnStringListData;

    frantic::tstring mayaAttrName;

    const frantic::tstring& nodeType = nodeInfo.m_nodeType;
    const std::vector<info::maya_magma_node_property_info>& nodeProperties = nodeInfo.m_propertyInfos;

    std::vector<info::maya_magma_node_property_info>::const_iterator citProperty;
    for( citProperty = nodeProperties.begin(); citProperty != nodeProperties.end(); citProperty++ ) {
        // all the property data types are from maya_magma_holder::get_node_property_type
        int index = citProperty->m_index;
        MObject mayaAttr;

        if( citProperty->m_type == _T("Bool") ) {
            mayaAttrName =
                get_maya_general_attr_name( nodeType, descID, true, index, citProperty->m_name + _T("_Bool") );
            mayaAttr =
                fnNumAttribute.create( frantic::strings::to_string( mayaAttrName ).c_str(),
                                       frantic::strings::to_string( mayaAttrName ).c_str(), MFnNumericData::kBoolean );
            fnNumAttribute.setHidden( true );

            depNode.addAttribute( mayaAttr );
            desc->set_node_property_maya_attr_name( descID, citProperty->m_name, mayaAttrName );

            // for the moment, this just a convenient
            if( citProperty->m_name == _T("enabled") ) {
                // set a default value if property is "enabled"
                MPlug plug = depNode.findPlug( frantic::strings::to_string( mayaAttrName ).c_str(), &status );
                plug.setValue( true );
            } else if( citProperty->m_name == _T("exposePosition") ) {
                // set a default value if property is "exposePosition" and make it read only
                MPlug plug = depNode.findPlug( frantic::strings::to_string( mayaAttrName ).c_str(), &status );
                plug.setValue( true );
            }

        } else if( citProperty->m_type == _T("Int") ) {
            mayaAttrName =
                get_maya_general_attr_name( nodeType, descID, true, index, citProperty->m_name + _T("_Int") );
            mayaAttr =
                fnNumAttribute.create( frantic::strings::to_string( mayaAttrName ).c_str(),
                                       frantic::strings::to_string( mayaAttrName ).c_str(), MFnNumericData::kInt );
            fnNumAttribute.setHidden( true );

            depNode.addAttribute( mayaAttr );
            desc->set_node_property_maya_attr_name( descID, citProperty->m_name, mayaAttrName );

            // for the moment, this just a convenient
            if( nodeInfo.m_nodeType == _T("InputValue") && citProperty->m_name == _T("enumValue") ) {
                // set a default value if property is "enumValue"
                MPlug plug = depNode.findPlug( frantic::strings::to_string( mayaAttrName ).c_str(), &status );
                plug.setValue( 1 );
            } else if( nodeInfo.m_nodeType == _T("Mux") && citProperty->m_name == _T("NumInputs") ) {
                // Mux requires at least 3 input sockets
                MPlug plug = depNode.findPlug( frantic::strings::to_string( mayaAttrName ).c_str(), &status );
                plug.setValue( 3 );
                fnNumAttribute.setMin( 3 );
                // Let's not make it animatable.  It doesn't make sense.
                fnNumAttribute.setKeyable( false );
                fnNumAttribute.setConnectable( false );
            }

        } else if( citProperty->m_type == _T("Float") ) {
            mayaAttrName =
                get_maya_general_attr_name( nodeType, descID, true, index, citProperty->m_name + _T("_Float") );
            mayaAttr =
                fnNumAttribute.create( frantic::strings::to_string( mayaAttrName ).c_str(),
                                       frantic::strings::to_string( mayaAttrName ).c_str(), MFnNumericData::kFloat );
            fnNumAttribute.setHidden( true );

            depNode.addAttribute( mayaAttr );
            desc->set_node_property_maya_attr_name( descID, citProperty->m_name, mayaAttrName );

        } else if( citProperty->m_type == _T("String") ) {
            mayaAttrName =
                get_maya_general_attr_name( nodeType, descID, true, index, citProperty->m_name + _T("_String") );

            MObject defaultString = fnStringData.create( "" );
            mayaAttr = fnTypedAttribute.create( frantic::strings::to_string( mayaAttrName ).c_str(),
                                                frantic::strings::to_string( mayaAttrName ).c_str(), MFnData::kString,
                                                defaultString );
            fnTypedAttribute.setHidden( true );

            depNode.addAttribute( mayaAttr );
            desc->set_node_property_maya_attr_name( descID, citProperty->m_name, mayaAttrName );

            // for the moment, this just a convenient
            if( citProperty->m_name == _T("channelName") ) {
                // set a default value if property is "channelName"
                MPlug plug = depNode.findPlug( frantic::strings::to_string( mayaAttrName ).c_str(), &status );
                plug.setValue( "Color" );
            } else if( citProperty->m_name == _T("inputType") ) {
                // set a default value if property is "inputType"
                MPlug plug = depNode.findPlug( frantic::strings::to_string( mayaAttrName ).c_str(), &status );
                plug.setValue( "Point" );
            }

        } else if( citProperty->m_type == _T("Vector3f") ) {
            mayaAttrName =
                get_maya_general_attr_name( nodeType, descID, true, index, citProperty->m_name + _T("_Vector3f") );

            mayaAttr =
                fnNumAttribute.create( frantic::strings::to_string( mayaAttrName ).c_str(),
                                       frantic::strings::to_string( mayaAttrName ).c_str(), MFnNumericData::k3Float );
            fnNumAttribute.setHidden( true );

            depNode.addAttribute( mayaAttr );
            desc->set_node_property_maya_attr_name( descID, citProperty->m_name, mayaAttrName );

        } else if( citProperty->m_type == _T("MagmaDataType") ) {
            // create a maya string attribute for 'MagmaDataType';
            // I disable the ability for user selecting arity and dataType for a channel;
            // this might need to be changed in the future; but at the moment, I do not think it is a good idea to
            // complicate this too much
            mayaAttrName =
                get_maya_general_attr_name( nodeType, descID, true, index, citProperty->m_name + _T("_MagmaDataType") );

            MObject defaultString = fnStringData.create( "" );
            mayaAttr = fnTypedAttribute.create( frantic::strings::to_string( mayaAttrName ).c_str(),
                                                frantic::strings::to_string( mayaAttrName ).c_str(), MFnData::kString,
                                                defaultString );
            fnTypedAttribute.setHidden( true );

            depNode.addAttribute( mayaAttr );
            desc->set_node_property_maya_attr_name( descID, citProperty->m_name, mayaAttrName );

            // set a default value
            // since if an maya attribute's value is equal to its default, Maya does not store the
            // value during saving which might cause program to crashes when loading a scene because "MagmaDataType"
            // can't be empty
            MPlug plug = depNode.findPlug( frantic::strings::to_string( mayaAttrName ).c_str(), &status );
            plug.setValue( "Vec3" );

        } else if( citProperty->m_type == _T("StringList") ) {
            mayaAttrName =
                get_maya_general_attr_name( nodeType, descID, true, index, citProperty->m_name + _T("_StringList") );

            MObject defaultStringList = fnStringListData.create();
            mayaAttr = fnTypedAttribute.create( frantic::strings::to_string( mayaAttrName ).c_str(),
                                                frantic::strings::to_string( mayaAttrName ).c_str(),
                                                MFnData::kStringArray, defaultStringList );
            fnTypedAttribute.setHidden( true );

            depNode.addAttribute( mayaAttr );
            desc->set_node_property_maya_attr_name( descID, citProperty->m_name, mayaAttrName );

        } else if( citProperty->m_type == _T("IntList") ) {
            mayaAttrName =
                get_maya_general_attr_name( nodeType, descID, true, index, citProperty->m_name + _T("_IntList") );

            mayaAttr =
                fnTypedAttribute.create( frantic::strings::to_string( mayaAttrName ).c_str(),
                                         frantic::strings::to_string( mayaAttrName ).c_str(), MFnData::kIntArray );
            fnNumAttribute.setHidden( true );

            depNode.addAttribute( mayaAttr );
            desc->set_node_property_maya_attr_name( descID, citProperty->m_name, mayaAttrName );
        }
    }
}

void create_input_socket_maya_attr( const info::maya_magma_node_info& nodeInfo, desc::maya_magma_desc_ptr desc,
                                    desc::desc_id descID, MFnDependencyNode& depNode ) {
    MStatus status;
    frantic::tstring mayaAttrName;
    MFnNumericAttribute fnNumAttribute;
    MFnEnumAttribute fnEnumAttribute;

    const frantic::tstring& nodeType = nodeInfo.m_nodeType;
    const std::vector<info::maya_magma_node_input_socket_info>& inputSocketInfos = nodeInfo.m_inputSocketInfos;
    std::vector<info::maya_magma_node_input_socket_info>::const_iterator citInputSocket;
    for( citInputSocket = inputSocketInfos.begin(); citInputSocket != inputSocketInfos.end(); citInputSocket++ ) {
        int index = citInputSocket->m_index;

        maya_magma_input_socket_data_type_t inputSocketDataType = citInputSocket->m_dataType;
        if( inputSocketDataType != MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_NONE ) {
            // only create a maya attribute for input sockets when its data type is valid
            bool isSupportBool = maya::is_input_socket_data_type_support_bool( inputSocketDataType );
            bool isSupportFloat = maya::is_input_socket_data_type_support_float( inputSocketDataType );
            bool isSupportInt = maya::is_input_socket_data_type_support_int( inputSocketDataType );
            bool isSupportVec3 = maya::is_input_socket_data_type_support_vec3( inputSocketDataType );
            bool isSupportQuat = maya::is_input_socket_data_type_support_quat( inputSocketDataType );

            short defaultValue;
            // Set defaults.  Float has highest priority
            if( isSupportFloat ) {
                defaultValue = MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_FLOAT;
            } else if( isSupportVec3 ) {
                defaultValue = MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_VEC3;
            } else if( isSupportInt ) {
                defaultValue = MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_INT;
            } else if( isSupportBool ) {
                defaultValue = MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_BOOL;
            } else if( isSupportQuat ) {
                defaultValue = MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_QUAT;
            } else {
                defaultValue = MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_NONE;
            }

            // create enum attribute
            MObject inputSocketDataTypeAttr;
            frantic::tstring mayaEnumAttrName =
                get_maya_general_attr_name( nodeType, descID, false, index, _T( "enum" ) );
            inputSocketDataTypeAttr =
                fnEnumAttribute.create( frantic::strings::to_string( mayaEnumAttrName ).c_str(),
                                        frantic::strings::to_string( mayaEnumAttrName ).c_str(), defaultValue );
            fnEnumAttribute.setHidden( true );
            fnEnumAttribute.addField( _T( "None" ), MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_NONE );

            depNode.addAttribute( inputSocketDataTypeAttr );
            desc->set_node_input_socket_maya_enum_attr_name( descID, index, mayaEnumAttrName );

            if( isSupportBool ) {
                MObject inputSocketBoolAttr;

                fnEnumAttribute.addField( _T( "Bool" ), MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_BOOL );
                mayaAttrName = get_maya_general_attr_name( nodeType, descID, false, index, _T( "bool" ) );
                inputSocketBoolAttr = fnNumAttribute.create( frantic::strings::to_string( mayaAttrName ).c_str(),
                                                             frantic::strings::to_string( mayaAttrName ).c_str(),
                                                             MFnNumericData::kBoolean );
                fnNumAttribute.setHidden( true );

                depNode.addAttribute( inputSocketBoolAttr );
                desc->push_node_input_socket_maya_attr_name( descID, index, mayaAttrName );
            }

            if( isSupportFloat ) {
                MObject inputSocketFloatAttr;

                fnEnumAttribute.addField( "Float", MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_FLOAT );
                mayaAttrName = get_maya_general_attr_name( nodeType, descID, false, index, _T( "float" ) );
                inputSocketFloatAttr = fnNumAttribute.create( frantic::strings::to_string( mayaAttrName ).c_str(),
                                                              frantic::strings::to_string( mayaAttrName ).c_str(),
                                                              MFnNumericData::kFloat );
                fnNumAttribute.setHidden( true );

                depNode.addAttribute( inputSocketFloatAttr );
                desc->push_node_input_socket_maya_attr_name( descID, index, mayaAttrName );
            }

            if( isSupportInt ) {
                MObject inputSocketIntAttr;

                fnEnumAttribute.addField( "Integral", MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_INT );
                mayaAttrName = get_maya_general_attr_name( nodeType, descID, false, index, _T( "int" ) );
                inputSocketIntAttr =
                    fnNumAttribute.create( frantic::strings::to_string( mayaAttrName ).c_str(),
                                           frantic::strings::to_string( mayaAttrName ).c_str(), MFnNumericData::kInt );
                fnNumAttribute.setHidden( true );

                depNode.addAttribute( inputSocketIntAttr );
                desc->push_node_input_socket_maya_attr_name( descID, index, mayaAttrName );
            }

            if( isSupportVec3 ) {
                MObject inputSocketVec3Attr;

                fnEnumAttribute.addField( "Vec3", MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_VEC3 );
                mayaAttrName = get_maya_general_attr_name( nodeType, descID, false, index, _T( "vector3f" ) );
                inputSocketVec3Attr = fnNumAttribute.create( frantic::strings::to_string( mayaAttrName ).c_str(),
                                                             frantic::strings::to_string( mayaAttrName ).c_str(),
                                                             MFnNumericData::k3Float );
                fnNumAttribute.setHidden( true );

                depNode.addAttribute( inputSocketVec3Attr );
                desc->push_node_input_socket_maya_attr_name( descID, index, mayaAttrName );
            }

            if( isSupportQuat ) {
                // create a maya quaternion attribute is tricky. because maya does not native support for this.
                // what I did is create a float maya attribute along with a vector3f maya attribute
                //
                // XXX, the suffix name hacking is important to distinguish which parts belong to which fields in
                // quaternion the float maya attribute has a suffix "quat_real" (the real part of quaternion) the
                // vector3f maya attribute has a suffix "quat_imaginary" (the imaginary part of quaternion)
                //
                // these two suffix strings will be used at get_input_socket_value_from_maya_attrs to get values from
                // maya attributes if you need to change these two strings, makes sure you change the corresponding
                // strings in get_input_socket_value_from_maya_attrs as well
                MObject inputSocketQuatAttrRealPart;
                MObject inputSocketQuatAttrImaginaryPart;

                fnEnumAttribute.addField( "Quat", MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_QUAT );
                mayaAttrName = get_maya_general_attr_name( nodeType, descID, false, index, _T( "quat_real" ) );
                inputSocketQuatAttrRealPart = fnNumAttribute.create(
                    frantic::strings::to_string( mayaAttrName ).c_str(),
                    frantic::strings::to_string( mayaAttrName ).c_str(), MFnNumericData::kFloat );
                fnNumAttribute.setHidden( true );
                depNode.addAttribute( inputSocketQuatAttrRealPart );
                desc->push_node_input_socket_maya_attr_name( descID, index, mayaAttrName );

                mayaAttrName = get_maya_general_attr_name( nodeType, descID, false, index, _T( "quat_imaginary" ) );
                inputSocketQuatAttrImaginaryPart = fnNumAttribute.create(
                    frantic::strings::to_string( mayaAttrName ).c_str(),
                    frantic::strings::to_string( mayaAttrName ).c_str(), MFnNumericData::k3Float );
                fnNumAttribute.setHidden( true );
                depNode.addAttribute( inputSocketQuatAttrImaginaryPart );
                desc->push_node_input_socket_maya_attr_name( descID, index, mayaAttrName );
            }
        }
    }
}

holder::input_socket_variant_t
get_input_socket_value_from_maya_attrs( const maya_attribute_input_socket_data_type userSelectedDataType,
                                        const std::vector<frantic::tstring>& inputSocketAttrNames,
                                        const MFnDependencyNode& depNode ) {
    using frantic::maya::attributes::get_value_from_maya_magma_attr;
    holder::input_socket_variant_t outValue;
    frantic::tstring mayaAttrName;
    switch( userSelectedDataType ) {
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_NONE:
        break;
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_BOOL: {
        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "bool" ) );
        outValue = get_value_from_maya_magma_attr<bool>( mayaAttrName, depNode );
        break;
    }
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_FLOAT: {
        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "float" ) );
        outValue = get_value_from_maya_magma_attr<float>( mayaAttrName, depNode );
        break;
    }
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_INT: {
        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "int" ) );
        outValue = get_value_from_maya_magma_attr<int>( mayaAttrName, depNode );
        break;
    }
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_VEC3: {
        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "vector3f" ) );
        outValue = get_value_from_maya_magma_attr<frantic::graphics::vector3f>( mayaAttrName, depNode );
        break;
    }
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_QUAT: {
        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "quat_real" ) );
        float realPart = get_value_from_maya_magma_attr<float>( mayaAttrName, depNode );

        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "quat_imaginary" ) );
        frantic::graphics::vector3f imaginaryPart =
            get_value_from_maya_magma_attr<frantic::graphics::vector3f>( mayaAttrName, depNode );

        outValue = frantic::graphics::quat4f( realPart, imaginaryPart );
        break;
    }
    default: {
        assert( false );
        throw maya_magma_exception( "detail::get_input_socket_value_from_maya_attrs an unknown input socket datatype" );
        break;
    }
    }
    return outValue;
}

maya_attribute_input_socket_data_type get_input_socket_value_enum_from_maya_attrs( const frantic::tstring& enumAttrName,
                                                                                   const MFnDependencyNode& depNode ) {
    using frantic::maya::attributes::get_value_from_maya_magma_attr;
    if( enumAttrName == desc::kInvalidEnumAttrName )
        return static_cast<maya_attribute_input_socket_data_type>( -1 );

    MStatus status;
    MPlug enumAttrPlug = depNode.findPlug( frantic::strings::to_string( enumAttrName ).c_str(), &status );

    ////////////// 5/26/15 - PropertyQuery Pre-2.4 Legacy Load Hack ////////////////////
    // This can be removed when it is no longer necessary to support maya files Pre-2.4

    // We need to actually add the attributes to the maya object
    // The names of the attributes should have already been added to the attribute map
    // The other part of this hack can be found in maya_magma_serializer.cpp

    if( status != MStatus::kSuccess ) {
        if( enumAttrName.find( _T( "PropertyQuery_" ) ) != frantic::tstring::npos ) {

            // Extract the node index and use that to build the attribute name to add.
            // This way, if this is a legitimate error, we'll add an attribute that won't do anything
            // And the error will still occur.
            std::size_t secondUnderscoreIndex = enumAttrName.find( _T( "_" ), 14 );
            frantic::tstring indexString = enumAttrName.substr( 14, secondUnderscoreIndex - 14 );

            frantic::tstring theName = frantic::maya::from_maya_t( depNode.name() );

            MString unusedResult;

            MGlobal::executeCommand(
                frantic::maya::to_maya_t(
                    _T( "addAttr -ci true -h true -sn \"PropertyQuery_" ) + indexString +
                    _T( "_InputSocket1_enum\" -ln \"PropertyQuery_" ) + indexString +
                    _T( "_InputSocket1_enum\" -dv 3 -min 0 -max 3 -en \"None:Integral=3\" -at \"enum\" \"" ) + theName +
                    _T( "\";" ) ),
                unusedResult );
            MGlobal::executeCommand(
                frantic::maya::to_maya_t( _T( "addAttr -ci true -h true -sn \"PropertyQuery_" ) + indexString +
                                          _T( "_InputSocket1_int\" -ln \"PropertyQuery_" ) + indexString +
                                          _T( "_InputSocket1_int\" -at \"long\" \"" ) + theName + _T( "\";" ) ),
                unusedResult );

            enumAttrPlug = depNode.findPlug( frantic::strings::to_string( enumAttrName ).c_str(), &status );
        } else if( enumAttrName.find( _T( "Noise_" ) ) != frantic::tstring::npos ) {
            const bool notVecNoise = enumAttrName.find( _T( "VecNoise_" ) ) == frantic::tstring::npos;
            const std::size_t firstUnderScoreIndex = notVecNoise ? 6 : 9;
            const frantic::tstring nodeTypeName = notVecNoise ? _T( "Noise_" ) : _T( "VecNoise_" );
            const std::size_t secondUnderscoreIndex = enumAttrName.find( _T( "_" ), firstUnderScoreIndex );
            const frantic::tstring indexString =
                enumAttrName.substr( firstUnderScoreIndex, secondUnderscoreIndex - firstUnderScoreIndex );
            const frantic::tstring theName = frantic::maya::from_maya_t( depNode.name() );
            MString unusedResult;

            MGlobal::executeCommand(
                frantic::maya::to_maya_t(
                    _T( "addAttr -ci true -h true -sn \"" + nodeTypeName ) + indexString +
                    _T( "_InputSocket0_enum\" -ln \"" + nodeTypeName ) + indexString +
                    _T( "_InputSocket0_enum\" -dv 2 -min 2 -max 4 -en \"Float=2:Vec3=4\" -at \"enum\" \"" ) + theName +
                    _T( "\";" ) ),
                unusedResult );
            MGlobal::executeCommand(
                frantic::maya::to_maya_t( _T( "addAttr -ci true -h true -sn \"" + nodeTypeName ) + indexString +
                                          _T( "_InputSocket0_vector3f0\" -ln \"" + nodeTypeName ) + indexString +
                                          _T( "_InputSocket0_vector3f0\" -at \"float\" \"" ) + theName + _T( "\";" ) ),
                unusedResult );
            MGlobal::executeCommand(
                frantic::maya::to_maya_t( _T( "addAttr -ci true -h true -sn \"" + nodeTypeName ) + indexString +
                                          _T( "_InputSocket0_vector3f1\" -ln \"" + nodeTypeName ) + indexString +
                                          _T( "_InputSocket0_vector3f1\" -at \"float\" \"" ) + theName + _T( "\";" ) ),
                unusedResult );
            MGlobal::executeCommand(
                frantic::maya::to_maya_t( _T( "addAttr -ci true -h true -sn \"" + nodeTypeName ) + indexString +
                                          _T( "_InputSocket0_vector3f2\" -ln \"" + nodeTypeName ) + indexString +
                                          _T( "_InputSocket0_vector3f2\" -at \"float\" \"" ) + theName + _T( "\";" ) ),
                unusedResult );
            MGlobal::executeCommand(
                frantic::maya::to_maya_t( _T( "addAttr -ci true -h true -sn \"" + nodeTypeName ) + indexString +
                                          _T( "_InputSocket0_float\" -ln \"" + nodeTypeName ) + indexString +
                                          _T( "_InputSocket0_float\" -at \"float\" \"" ) + theName + _T( "\";" ) ),
                unusedResult );
            MGlobal::executeCommand(
                frantic::maya::to_maya_t(
                    _T( "addAttr -ci true -h true -sn \"" + nodeTypeName ) + indexString +
                    _T( "_InputSocket1_enum\" -ln \"" + nodeTypeName ) + indexString +
                    _T( "_InputSocket1_enum\" -dv 2 -min 2 -max 4 -en \"Float=2:Vec3=4\" -at \"enum\" \"" ) + theName +
                    _T( "\";" ) ),
                unusedResult );
            MGlobal::executeCommand(
                frantic::maya::to_maya_t( _T( "addAttr -ci true -h true -sn \"" + nodeTypeName ) + indexString +
                                          _T( "_InputSocket1_float\" -ln \"" + nodeTypeName ) + indexString +
                                          _T( "_InputSocket1_float\" -at \"float\" \"" ) + theName + _T( "\";" ) ),
                unusedResult );

            enumAttrPlug = depNode.findPlug( frantic::strings::to_string( enumAttrName ).c_str(), &status );
        } else if( enumAttrName.find( _T( "NearestPoint_" ) ) != frantic::tstring::npos ) {
            const std::size_t secondUnderscoreIndex = enumAttrName.find( _T( "_" ), 13 );
            const frantic::tstring indexString = enumAttrName.substr( 13, secondUnderscoreIndex - 13 );
            const frantic::tstring theName = frantic::maya::from_maya_t( depNode.name() );
            MString unusedResult;
            MGlobal::executeCommand(
                frantic::maya::to_maya_t(
                    _T( "addAttr -ci true -h true -sn \"NearestPoint_" ) + indexString +
                    _T( "_InputSocket1_enum\" -ln \"NearestPoint_" ) + indexString +
                    _T( "_InputSocket1_enum\" -dv 4 -min 4 -max 4 -en \"Vec3=4\" -at \"enum\" \"" ) + theName +
                    _T( "\";" ) ),
                unusedResult );
            MGlobal::executeCommand(
                frantic::maya::to_maya_t( _T( "addAttr -ci true -h true -sn \"NearestPoint_" ) + indexString +
                                          _T( "_InputSocket1_vector3f0\" -ln \"NearestPoint_" ) + indexString +
                                          _T( "_InputSocket1_vector3f0\" -at \"float\" \"" ) + theName + _T( "\";" ) ),
                unusedResult );
            MGlobal::executeCommand(
                frantic::maya::to_maya_t( _T( "addAttr -ci true -h true -sn \"NearestPoint_" ) + indexString +
                                          _T( "_InputSocket1_vector3f1\" -ln \"NearestPoint_" ) + indexString +
                                          _T( "_InputSocket1_vector3f1\" -at \"float\" \"" ) + theName + _T( "\";" ) ),
                unusedResult );
            MGlobal::executeCommand(
                frantic::maya::to_maya_t( _T( "addAttr -ci true -h true -sn \"NearestPoint_" ) + indexString +
                                          _T( "_InputSocket1_vector3f2\" -ln \"NearestPoint_" ) + indexString +
                                          _T( "_InputSocket1_vector3f2\" -at \"float\" \"" ) + theName + _T( "\";" ) ),
                unusedResult );
            MGlobal::executeCommand(
                frantic::maya::to_maya_t(
                    _T( "addAttr -ci true -h true -sn \"NearestPoint_" ) + indexString +
                    _T( "_InputSocket2_enum\" -ln \"NearestPoint_" ) + indexString +
                    _T( "_InputSocket2_enum\" -dv 1 -min 1 -max 1 -en \"Bool=1\" -at \"enum\" \"" ) + theName +
                    _T( "\";" ) ),
                unusedResult );
            MGlobal::executeCommand(
                frantic::maya::to_maya_t( _T( "addAttr -ci true -h true -sn \"NearestPoint_" ) + indexString +
                                          _T( "_InputSocket2_bool\" -ln \"NearestPoint_" ) + indexString +
                                          _T( "_InputSocket2_bool\" -at \"bool\" \"" ) + theName + _T( "\";" ) ),
                unusedResult );

            enumAttrPlug = depNode.findPlug( frantic::strings::to_string( enumAttrName ).c_str(), &status );
        } else if( enumAttrName.find( _T( "IntersectRay_" ) ) != frantic::tstring::npos ) {
            const std::size_t secondUnderscoreIndex = enumAttrName.find( _T( "_" ), 13 );
            const frantic::tstring indexString = enumAttrName.substr( 13, secondUnderscoreIndex - 13 );
            const frantic::tstring theName = frantic::maya::from_maya_t( depNode.name() );
            MString unusedResult;
            MGlobal::executeCommand(
                frantic::maya::to_maya_t(
                    _T( "addAttr -ci true -h true -sn \"IntersectRay_" ) + indexString +
                    _T( "_InputSocket1_enum\" -ln \"IntersectRay_" ) + indexString +
                    _T( "_InputSocket1_enum\" -dv 4 -min 4 -max 4 -en \"Vec3=4\" -at \"enum\" \"" ) + theName +
                    _T( "\";" ) ),
                unusedResult );
            MGlobal::executeCommand(
                frantic::maya::to_maya_t( _T( "addAttr -ci true -h true -sn \"IntersectRay_" ) + indexString +
                                          _T( "_InputSocket1_vector3f0\" -ln \"IntersectRay_" ) + indexString +
                                          _T( "_InputSocket1_vector3f0\" -at \"float\" \"" ) + theName + _T( "\";" ) ),
                unusedResult );
            MGlobal::executeCommand(
                frantic::maya::to_maya_t( _T( "addAttr -ci true -h true -sn \"IntersectRay_" ) + indexString +
                                          _T( "_InputSocket1_vector3f1\" -ln \"IntersectRay_" ) + indexString +
                                          _T( "_InputSocket1_vector3f1\" -at \"float\" \"" ) + theName + _T( "\";" ) ),
                unusedResult );
            MGlobal::executeCommand(
                frantic::maya::to_maya_t( _T( "addAttr -ci true -h true -sn \"IntersectRay_" ) + indexString +
                                          _T( "_InputSocket1_vector3f2\" -ln \"IntersectRay_" ) + indexString +
                                          _T( "_InputSocket1_vector3f2\" -at \"float\" \"" ) + theName + _T( "\";" ) ),
                unusedResult );
            MGlobal::executeCommand(
                frantic::maya::to_maya_t(
                    _T( "addAttr -ci true -h true -sn \"IntersectRay_" ) + indexString +
                    _T( "_InputSocket2_enum\" -ln \"IntersectRay_" ) + indexString +
                    _T( "_InputSocket2_enum\" -dv 4 -min 4 -max 4 -en \"Vec3=4\" -at \"enum\" \"" ) + theName +
                    _T( "\";" ) ),
                unusedResult );
            MGlobal::executeCommand(
                frantic::maya::to_maya_t( _T( "addAttr -ci true -h true -sn \"IntersectRay_" ) + indexString +
                                          _T( "_InputSocket2_vector3f0\" -ln \"IntersectRay_" ) + indexString +
                                          _T( "_InputSocket2_vector3f0\" -at \"float\" \"" ) + theName + _T( "\";" ) ),
                unusedResult );
            MGlobal::executeCommand(
                frantic::maya::to_maya_t( _T( "addAttr -ci true -h true -sn \"IntersectRay_" ) + indexString +
                                          _T( "_InputSocket2_vector3f1\" -ln \"IntersectRay_" ) + indexString +
                                          _T( "_InputSocket2_vector3f1\" -at \"float\" \"" ) + theName + _T( "\";" ) ),
                unusedResult );
            MGlobal::executeCommand(
                frantic::maya::to_maya_t( _T( "addAttr -ci true -h true -sn \"IntersectRay_" ) + indexString +
                                          _T( "_InputSocket2_vector3f2\" -ln \"IntersectRay_" ) + indexString +
                                          _T( "_InputSocket2_vector3f2\" -at \"float\" \"" ) + theName + _T( "\";" ) ),
                unusedResult );
            MGlobal::executeCommand(
                frantic::maya::to_maya_t(
                    _T( "addAttr -ci true -h true -sn \"IntersectRay_" ) + indexString +
                    _T( "_InputSocket3_enum\" -ln \"IntersectRay_" ) + indexString +
                    _T( "_InputSocket3_enum\" -dv 1 -min 1 -max 1 -en \"Bool=1\" -at \"enum\" \"" ) + theName +
                    _T( "\";" ) ),
                unusedResult );
            MGlobal::executeCommand(
                frantic::maya::to_maya_t( _T( "addAttr -ci true -h true -sn \"IntersectRay_" ) + indexString +
                                          _T( "_InputSocket3_bool\" -ln \"IntersectRay_" ) + indexString +
                                          _T( "_InputSocket3_bool\" -at \"bool\" \"" ) + theName + _T( "\";" ) ),
                unusedResult );

            enumAttrPlug = depNode.findPlug( frantic::strings::to_string( enumAttrName ).c_str(), &status );
        }
    }

    ////////////// End Hack ////////////////////////////////////////////////////////////

    throw_maya_magma_exception_if_not_success(
        status, _T( "detail::get_input_socket_value_enum_from_maya_attrs could not find mplug (" ) + enumAttrName +
                    _T( ") from depNode" ) );

    int userSelectedDataType;
    status = enumAttrPlug.getValue( userSelectedDataType );
    throw_maya_magma_exception_if_not_success(
        status, _T( "detail::get_input_socket_value_enum_from_maya_attrs could not get maya enum value " ) );

    return static_cast<maya_attribute_input_socket_data_type>( userSelectedDataType );
}

holder::input_socket_variant_t
get_input_socket_value_from_maya_attrs( const frantic::tstring& enumAttrName,
                                        const std::vector<frantic::tstring>& inputSocketAttrNames,
                                        const MFnDependencyNode& depNode ) {
    using frantic::maya::attributes::get_value_from_maya_magma_attr;
    if( enumAttrName == desc::kInvalidEnumAttrName )
        return boost::blank();

    maya_attribute_input_socket_data_type userSelectedDataType =
        get_input_socket_value_enum_from_maya_attrs( enumAttrName, depNode );
    return get_input_socket_value_from_maya_attrs( userSelectedDataType, inputSocketAttrNames, depNode );
}

void set_input_socket_value_from_maya_attrs( const maya_attribute_input_socket_data_type userSelectedDataType,
                                             const std::vector<frantic::tstring>& inputSocketAttrNames,
                                             const MFnDependencyNode& depNode, holder::input_socket_variant_t& value ) {
    using frantic::maya::attributes::set_value_from_maya_magma_attr;
    frantic::tstring mayaAttrName;
    switch( userSelectedDataType ) {
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_NONE:
        break;
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_BOOL: {
        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "bool" ) );
        set_value_from_maya_magma_attr( mayaAttrName, depNode, boost::get<bool>( value ) );
        break;
    }
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_FLOAT: {
        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "float" ) );
        set_value_from_maya_magma_attr( mayaAttrName, depNode, boost::get<float>( value ) );
        break;
    }
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_INT: {
        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "int" ) );
        set_value_from_maya_magma_attr( mayaAttrName, depNode, boost::get<int>( value ) );
        break;
    }
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_VEC3: {
        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "vector3f" ) );
        set_value_from_maya_magma_attr( mayaAttrName, depNode, boost::get<frantic::graphics::vector3f>( value ) );
        break;
    }
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_QUAT: {
        frantic::graphics::quat4f quat = boost::get<frantic::graphics::quat4f>( value );

        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "quat_real" ) );
        set_value_from_maya_magma_attr( mayaAttrName, depNode, quat.real_part() );

        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "quat_imaginary" ) );
        set_value_from_maya_magma_attr( mayaAttrName, depNode, quat.vector_part() );
        break;
    }
    default: {
        assert( false );
        throw maya_magma_exception( "detail::set_input_socket_value_from_maya_attrs an unknown input socket datatype" );
        break;
    }
    }
}

void set_input_socket_value_enum_from_maya_attrs( const frantic::tstring& enumAttrName,
                                                  const MFnDependencyNode& depNode,
                                                  const maya_attribute_input_socket_data_type value ) {
    MStatus status;
    MPlug enumAttrPlug = depNode.findPlug( frantic::strings::to_string( enumAttrName ).c_str(), &status );
    throw_maya_magma_exception_if_not_success(
        status, _T( "detail::set_input_socket_value_enum_from_maya_attrs could not find mplug (" ) + enumAttrName +
                    _T( ") from depNode" ) );

    status = enumAttrPlug.setValue( static_cast<int>( value ) );
    throw_maya_magma_exception_if_not_success(
        status, _T( "detail::set_input_socket_value_enum_from_maya_attrs could not get maya enum value " ) );
}

holder::property_variant_t get_property_value_from_maya_attrs( const frantic::tstring& mayaAttrName,
                                                               const MFnDependencyNode& depNode ) {
    using frantic::maya::attributes::get_value_from_maya_magma_attr;

    MStatus status;
    holder::property_variant_t outValue;

    if( is_tstring_contain( mayaAttrName, _T( "_Bool" ) ) ) {
        outValue = get_value_from_maya_magma_attr<bool>( mayaAttrName, depNode );
    } else if( is_tstring_contain( mayaAttrName, _T( "_IntList" ) ) ) {
        outValue = std::vector<int>(); // TODO:
    } else if( is_tstring_contain( mayaAttrName, _T( "_Int" ) ) ) {
        outValue = get_value_from_maya_magma_attr<int>( mayaAttrName, depNode );
    } else if( is_tstring_contain( mayaAttrName, _T( "_Float" ) ) ) {
        outValue = get_value_from_maya_magma_attr<float>( mayaAttrName, depNode );
    } else if( is_tstring_contain( mayaAttrName, _T( "_StringList" ) ) ) {
        outValue = get_value_from_maya_magma_attr<std::vector<frantic::tstring>>( mayaAttrName, depNode );
    } else if( is_tstring_contain( mayaAttrName, _T( "_String" ) ) ) {
        MString stringValue = get_value_from_maya_magma_attr<MString>( mayaAttrName, depNode );
        outValue = frantic::maya::from_maya_t( stringValue );
    } else if( is_tstring_contain( mayaAttrName, _T( "_Vector3f" ) ) ) {
        outValue = get_value_from_maya_magma_attr<frantic::graphics::vector3f>( mayaAttrName, depNode );
    } else if( is_tstring_contain( mayaAttrName, _T( "_MagmaDataType" ) ) ) {
        MString stringValue = get_value_from_maya_magma_attr<MString>( mayaAttrName, depNode );
        outValue = frantic::maya::from_maya_t( stringValue );
    } else {
        assert( false );
        throw maya_magma_exception( _T( "detail::get_property_value_from_maya_attrs an unknown maya attribute (" ) +
                                    mayaAttrName + _T( ")" ) );
    }
    return outValue;
}

void set_property_value_from_maya_attrs( const frantic::tstring& mayaAttrName, const MFnDependencyNode& depNode,
                                         holder::property_variant_t& value ) {
    using frantic::maya::attributes::set_value_from_maya_magma_attr;

    MStatus status;
    holder::property_variant_t outValue;

    if( is_tstring_contain( mayaAttrName, _T( "_Bool" ) ) ) {
        set_value_from_maya_magma_attr<bool>( mayaAttrName, depNode, boost::get<bool>( value ) );
    } else if( is_tstring_contain( mayaAttrName, _T( "_Int" ) ) ) {
        set_value_from_maya_magma_attr( mayaAttrName, depNode, boost::get<int>( value ) );
    } else if( is_tstring_contain( mayaAttrName, _T( "_Float" ) ) ) {
        set_value_from_maya_magma_attr( mayaAttrName, depNode, boost::get<float>( value ) );
    } else if( is_tstring_contain( mayaAttrName, _T( "_StringList" ) ) ) {
        set_value_from_maya_magma_attr( mayaAttrName, depNode, boost::get<std::vector<frantic::tstring>>( value ) );
    } else if( is_tstring_contain( mayaAttrName, _T( "_String" ) ) ) {
        MString stringValue = frantic::maya::to_maya_t( boost::get<frantic::tstring>( value ) );
        set_value_from_maya_magma_attr( mayaAttrName, depNode, stringValue );
    } else if( is_tstring_contain( mayaAttrName, _T( "_Vector3f" ) ) ) {
        set_value_from_maya_magma_attr( mayaAttrName, depNode, boost::get<frantic::graphics::vector3f>( value ) );
    } else if( is_tstring_contain( mayaAttrName, _T( "_MagmaDataType" ) ) ) {
        MString stringValue = frantic::maya::to_maya_t( boost::get<frantic::tstring>( value ) );
        set_value_from_maya_magma_attr( mayaAttrName, depNode, stringValue );
    } else {
        assert( false );
        throw maya_magma_exception( _T( "detail::set_property_value_from_maya_attrs an unknown maya attribute (" ) +
                                    mayaAttrName + _T( ")" ) );
    }
}

void delete_property_maya_attr( desc::maya_magma_desc_ptr desc, desc::desc_id descID, MFnDependencyNode& depNode ) {
    std::vector<frantic::tstring> propertyMayaAttrNames = desc->get_desc_node_property_maya_attr_names( descID );
    std::vector<frantic::tstring>::const_iterator cit;
    for( cit = propertyMayaAttrNames.begin(); cit != propertyMayaAttrNames.end(); cit++ ) {
        frantic::maya::attributes::delete_maya_attribute( cit->c_str(), depNode );
    }
}

void delete_input_sockets_maya_attr( desc::maya_magma_desc_ptr desc, desc::desc_id descID,
                                     MFnDependencyNode& depNode ) {
    std::vector<frantic::tstring> inputSocketMayaAttrNames = desc->get_desc_node_input_socket_maya_attr_names( descID );
    std::vector<frantic::tstring>::const_iterator cit;
    for( cit = inputSocketMayaAttrNames.begin(); cit != inputSocketMayaAttrNames.end(); cit++ ) {
        frantic::maya::attributes::delete_maya_attribute( cit->c_str(), depNode );
    }
}

void set_input_socket_animation_from_maya_attrs( const maya_attribute_input_socket_data_type userSelectedDataType,
                                                 const std::vector<frantic::tstring>& inputSocketAttrNames,
                                                 const MFnDependencyNode& depNode,
                                                 holder::input_socket_animation_variant_t& value ) {
    using frantic::maya::attributes::set_animation_from_maya_attr;
    using frantic::maya::attributes::set_animation_from_maya_attr_vec3;

    frantic::tstring mayaAttrName;
    switch( userSelectedDataType ) {
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_NONE:
        break;
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_BOOL: {
        // No animation support, do nothing
        break;
    }
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_FLOAT: {
        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "float" ) );
        set_animation_from_maya_attr( mayaAttrName, depNode,
                                      boost::get<frantic::maya::animation::animation_data>( value ) );
        break;
    }
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_INT: {
        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "int" ) );
        set_animation_from_maya_attr( mayaAttrName, depNode,
                                      boost::get<frantic::maya::animation::animation_data>( value ) );
        break;
    }
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_VEC3: {
        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "vector3f" ) );
        set_animation_from_maya_attr_vec3( mayaAttrName, depNode,
                                           boost::get<frantic::maya::animation::animation_data_vector3>( value ) );
        break;
    }
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_QUAT: {
        frantic::maya::animation::animation_data_vector4 val =
            boost::get<frantic::maya::animation::animation_data_vector4>( value );

        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "quat_real" ) );
        set_animation_from_maya_attr( mayaAttrName, depNode, val.getReal() );

        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "quat_imaginary" ) );
        set_animation_from_maya_attr_vec3( mayaAttrName, depNode, val.getImaginary() );

        break;
    }
    default: {
        assert( false );
        throw maya_magma_exception(
            "detail::set_input_socket_animation_from_maya_attrs an unknown input socket datatype" );
        break;
    }
    }
}

holder::input_socket_animation_variant_t
get_input_socket_animation_from_maya_attrs( const maya_attribute_input_socket_data_type userSelectedDataType,
                                            const std::vector<frantic::tstring>& inputSocketAttrNames,
                                            const MFnDependencyNode& depNode ) {
    using frantic::maya::attributes::get_animation_from_maya_attr;
    using frantic::maya::attributes::get_animation_from_maya_attr_vec3;

    holder::input_socket_animation_variant_t outValue;
    frantic::tstring mayaAttrName;
    switch( userSelectedDataType ) {
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_NONE:
        break;
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_BOOL: {
        // No animation support, do nothing
        break;
    }
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_FLOAT: {
        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "float" ) );
        outValue = get_animation_from_maya_attr( mayaAttrName, depNode );
        break;
    }
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_INT: {
        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "int" ) );
        outValue = get_animation_from_maya_attr( mayaAttrName, depNode );
        break;
    }
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_VEC3: {
        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "vector3f" ) );
        outValue = get_animation_from_maya_attr_vec3( mayaAttrName, depNode );
        break;
    }
    case MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_QUAT: {
        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "quat_real" ) );
        frantic::maya::animation::animation_data realPart;
        realPart = get_animation_from_maya_attr( mayaAttrName, depNode );

        mayaAttrName = find_tstring_if_vector_match( inputSocketAttrNames, _T( "quat_imaginary" ) );
        frantic::maya::animation::animation_data_vector3 imagPart;
        imagPart = get_animation_from_maya_attr_vec3( mayaAttrName, depNode );

        outValue = frantic::maya::animation::animation_data_vector4( realPart, imagPart );
        break;
    }
    default: {
        assert( false );
        throw maya_magma_exception(
            "detail::get_input_socket_animation_from_maya_attrs an unknown input socket datatype" );
        break;
    }
    }
    return outValue;
}

void set_property_animation_from_maya_attrs( const frantic::tstring& mayaAttrName, const MFnDependencyNode& depNode,
                                             holder::property_animation_variant_t& value ) {
    using frantic::maya::attributes::set_animation_from_maya_attr;
    using frantic::maya::attributes::set_animation_from_maya_attr_vec3;

    MStatus status;
    holder::property_animation_variant_t outValue;

    if( is_tstring_contain( mayaAttrName, _T( "_Bool" ) ) ) {
        // No animation support, do nothing
    } else if( is_tstring_contain( mayaAttrName, _T( "_Int" ) ) ) {
        set_animation_from_maya_attr( mayaAttrName, depNode,
                                      boost::get<frantic::maya::animation::animation_data>( value ) );
    } else if( is_tstring_contain( mayaAttrName, _T( "_Float" ) ) ) {
        set_animation_from_maya_attr( mayaAttrName, depNode,
                                      boost::get<frantic::maya::animation::animation_data>( value ) );
    } else if( is_tstring_contain( mayaAttrName, _T( "_StringList" ) ) ) {
        // No animation support, do nothing
    } else if( is_tstring_contain( mayaAttrName, _T( "_String" ) ) ) {
        // No animation support, do nothing
    } else if( is_tstring_contain( mayaAttrName, _T( "_Vector3f" ) ) ) {
        set_animation_from_maya_attr_vec3( mayaAttrName, depNode,
                                           boost::get<frantic::maya::animation::animation_data_vector3>( value ) );
    } else if( is_tstring_contain( mayaAttrName, _T( "_MagmaDataType" ) ) ) {
        // No animation support, do nothing
    } else {
        assert( false );
        throw maya_magma_exception( _T( "detail::set_property_animation_from_maya_attrs an unknown maya attribute (" ) +
                                    mayaAttrName + _T( ")" ) );
    }
}

holder::property_animation_variant_t get_property_animation_from_maya_attrs( const frantic::tstring& mayaAttrName,
                                                                             const MFnDependencyNode& depNode ) {
    using frantic::maya::attributes::get_animation_from_maya_attr;
    using frantic::maya::attributes::get_animation_from_maya_attr_vec3;

    MStatus status;
    holder::property_animation_variant_t outValue;

    if( is_tstring_contain( mayaAttrName, _T( "_Bool" ) ) ) {
        // No animation support, do nothing
    } else if( is_tstring_contain( mayaAttrName, _T( "_Int" ) ) ) {
        outValue = get_animation_from_maya_attr( mayaAttrName, depNode );
    } else if( is_tstring_contain( mayaAttrName, _T( "_Float" ) ) ) {
        outValue = get_animation_from_maya_attr( mayaAttrName, depNode );
    } else if( is_tstring_contain( mayaAttrName, _T( "_StringList" ) ) ) {
        // No animation support, do nothing
    } else if( is_tstring_contain( mayaAttrName, _T( "_String" ) ) ) {
        // No animation support, do nothing
    } else if( is_tstring_contain( mayaAttrName, _T( "_Vector3f" ) ) ) {
        outValue = get_animation_from_maya_attr_vec3( mayaAttrName, depNode );
    } else if( is_tstring_contain( mayaAttrName, _T( "_MagmaDataType" ) ) ) {
        // No animation support, do nothing
    } else {
        assert( false );
        throw maya_magma_exception( _T( "detail::get_property_animation_from_maya_attrs an unknown maya attribute (" ) +
                                    mayaAttrName + _T( ")" ) );
    }
    return outValue;
}
} // namespace detail

void maya_magma_attr_manager::create_maya_attr( const info::maya_magma_node_info& nodeInfo,
                                                desc::maya_magma_desc_ptr desc, desc::desc_id descID,
                                                MFnDependencyNode& depNode ) {
    // create maya attribute based on type of node property
    detail::create_property_maya_attr( nodeInfo, desc, descID, depNode );

    // create maya attribute based on type of node input socket
    detail::create_input_socket_maya_attr( nodeInfo, desc, descID, depNode );
}

void maya_magma_attr_manager::delete_maya_attr( desc::maya_magma_desc_ptr desc, desc::desc_id descID,
                                                MFnDependencyNode& depNode ) {
    // delete maya attributes associated with desc_node properties
    detail::delete_property_maya_attr( desc, descID, depNode );

    // delete maya attributes associated with desc_node input sockets
    detail::delete_input_sockets_maya_attr( desc, descID, depNode );
}

holder::input_socket_variant_t maya_magma_attr_manager::get_input_socket_value_from_maya_attrs(
    const frantic::tstring& enumAttrName, const std::vector<frantic::tstring>& inputSocketAttrNames,
    const MFnDependencyNode& depNode ) {
    return detail::get_input_socket_value_from_maya_attrs( enumAttrName, inputSocketAttrNames, depNode );
}

holder::input_socket_variant_t maya_magma_attr_manager::get_input_socket_value_from_maya_attrs(
    const maya_attribute_input_socket_data_type index, const std::vector<frantic::tstring>& inputSocketAttrNames,
    const MFnDependencyNode& depNode ) {
    return detail::get_input_socket_value_from_maya_attrs( index, inputSocketAttrNames, depNode );
}

maya_attribute_input_socket_data_type
maya_magma_attr_manager::get_input_socket_value_enum_from_maya_attrs( const frantic::tstring& enumAttrName,
                                                                      const MFnDependencyNode& depNode ) {
    return detail::get_input_socket_value_enum_from_maya_attrs( enumAttrName, depNode );
}

void maya_magma_attr_manager::set_input_socket_value_from_maya_attrs(
    const maya_attribute_input_socket_data_type index, const std::vector<frantic::tstring>& inputSocketAttrNames,
    const MFnDependencyNode& depNode, holder::input_socket_variant_t& value ) {
    detail::set_input_socket_value_from_maya_attrs( index, inputSocketAttrNames, depNode, value );
}

void maya_magma_attr_manager::set_input_socket_value_enum_from_maya_attrs(
    const frantic::tstring& enumAttrName, const MFnDependencyNode& depNode,
    const maya_attribute_input_socket_data_type value ) {
    detail::set_input_socket_value_enum_from_maya_attrs( enumAttrName, depNode, value );
}

holder::property_variant_t
maya_magma_attr_manager::get_property_value_from_maya_attrs( const frantic::tstring& mayaAttrName,
                                                             const MFnDependencyNode& depNode ) {
    return detail::get_property_value_from_maya_attrs( mayaAttrName, depNode );
}

void maya_magma_attr_manager::set_property_value_from_maya_attrs( const frantic::tstring& mayaAttrName,
                                                                  const MFnDependencyNode& depNode,
                                                                  holder::property_variant_t& value ) {
    detail::set_property_value_from_maya_attrs( mayaAttrName, depNode, value );
}

void maya_magma_attr_manager::set_input_socket_animation_from_maya_attrs(
    const maya_attribute_input_socket_data_type index, const std::vector<frantic::tstring>& inputSocketAttrNames,
    const MFnDependencyNode& depNode, holder::input_socket_animation_variant_t& value ) {
    detail::set_input_socket_animation_from_maya_attrs( index, inputSocketAttrNames, depNode, value );
}

holder::input_socket_animation_variant_t maya_magma_attr_manager::get_input_socket_animation_from_maya_attrs(
    const maya_attribute_input_socket_data_type index, const std::vector<frantic::tstring>& inputSocketAttrNames,
    const MFnDependencyNode& depNode ) {
    return detail::get_input_socket_animation_from_maya_attrs( index, inputSocketAttrNames, depNode );
}

void maya_magma_attr_manager::set_property_animation_from_maya_attrs( const frantic::tstring& mayaAttrName,
                                                                      const MFnDependencyNode& depNode,
                                                                      holder::property_animation_variant_t& value ) {
    detail::set_property_animation_from_maya_attrs( mayaAttrName, depNode, value );
}

holder::property_animation_variant_t
maya_magma_attr_manager::get_property_animation_from_maya_attrs( const frantic::tstring& mayaAttrName,
                                                                 const MFnDependencyNode& depNode ) {
    return detail::get_property_animation_from_maya_attrs( mayaAttrName, depNode );
}

void maya_magma_attr_manager::update_to_latest_version( desc::maya_magma_desc_ptr desc, MFnDependencyNode& depNode ) {
    std::vector<desc::desc_id> geometryNodes;
    frantic::tstring inputGeometry( _T( "InputGeometry" ) );

    const std::map<desc::desc_id, desc::maya_magma_desc_node>& nodes = desc->get_nodes();
    for( std::map<desc::desc_id, desc::maya_magma_desc_node>::const_iterator iter = nodes.begin(); iter != nodes.end();
         ++iter ) {

        if( iter->second.get_node_type() == inputGeometry ) {
            geometryNodes.push_back( iter->first );
        }
    }

    for( std::vector<desc::desc_id>::const_iterator iter = geometryNodes.begin(); iter != geometryNodes.end();
         ++iter ) {

        desc::desc_id id = *iter;

        // string geometryName -> string[] geometryNames
        frantic::tstring geometryName( _T( "geometryName" ) );
        if( desc->has_node_property_name( id, geometryName ) ) {
            frantic::tstring oldAttrName = desc->get_maya_attr_name_from_node_property( id, geometryName );

            // Get the currently stored value:
            holder::property_variant_t initialValue = get_property_value_from_maya_attrs( oldAttrName, depNode );

            // retrieve magma_node info
            info::maya_magma_node_info nodeInfo =
                factory::maya_magma_node_info_factory::create_node_infos( inputGeometry );
            frantic::tstring geometryNames( _T( "geometryNames" ) );

            // Create the new attribute
            bool added = false;
            frantic::tstring newAttrName;
            const std::vector<info::maya_magma_node_property_info>& nodeProperties = nodeInfo.m_propertyInfos;
            std::vector<info::maya_magma_node_property_info>::const_iterator citProperty;
            for( citProperty = nodeProperties.begin(); citProperty != nodeProperties.end(); citProperty++ ) {
                int index = citProperty->m_index;

                if( citProperty->m_name == geometryNames ) {
                    newAttrName = detail::get_maya_general_attr_name( inputGeometry, id, true, index,
                                                                      geometryNames + _T("_StringList") );

                    // Update the attribute with the old value
                    MFnStringArrayData fnStringListData;
                    MFnTypedAttribute fnTypedAttribute;
                    MStringArray initialValueArray;
                    initialValueArray.append(
                        frantic::maya::to_maya_t( boost::get<frantic::tstring>( initialValue ) ) );
                    MObject singletonList = fnStringListData.create( initialValueArray );
                    MObject mayaAttr = fnTypedAttribute.create( frantic::strings::to_string( newAttrName ).c_str(),
                                                                frantic::strings::to_string( newAttrName ).c_str(),
                                                                MFnData::kStringArray, singletonList );
                    fnTypedAttribute.setHidden( true );

                    depNode.addAttribute( mayaAttr );
                    desc->set_node_property_maya_attr_name( id, geometryNames, newAttrName );

                    added = true;
                    break;
                }
            }
            if( !added ) {
                // This should never occur
                throw maya_magma_exception(
                    _T( "maya_magma_attr_manager::update_to_latest_version unable to update attribute (" ) +
                    oldAttrName + _T( ")" ) );
            }

            //// Update the attribute with the old value
            // std::vector<frantic::tstring> newValue;
            // newValue.push_back( boost::get<frantic::tstring>(initialValue) );
            // holder::property_variant_t newValueVariant = newValue;
            // set_property_value_from_maya_attrs(newAttrName, depNode, newValueVariant);

            // Delete the old attribute and mapping
            frantic::maya::attributes::delete_maya_attribute( frantic::strings::to_string( oldAttrName ).c_str(),
                                                              depNode );
            desc->remove_node_property( id, geometryName );
        }
    }
}

} // namespace attr
} // namespace maya
} // namespace magma
} // namespace frantic
