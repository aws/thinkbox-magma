// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "stdafx.h"

#include <frantic/sstream/tsstream.hpp>

#include "frantic/magma/maya/maya_magma_description.hpp"
#include "frantic/magma/maya/maya_magma_mel_loader.hpp"

#include "frantic/magma/maya/maya_magma_attr_manager.hpp"
#include "frantic/magma/maya/maya_magma_datatypes.hpp"
#include "frantic/magma/maya/maya_magma_info_factory.hpp"
#include "frantic/maya/parser_args.hpp"

#include "frantic/magma/maya/maya_magma_gui.hpp"
#include "frantic/magma/maya/maya_magma_mel_window.hpp"

#include <frantic/maya/convert.hpp>

#include <maya/MDataHandle.h>
#include <maya/MFnStringArrayData.h>

#include <map>
#include <set>
#include <sstream>

namespace frantic {
namespace magma {
namespace maya {
namespace mel {

namespace detail {
// Helper method to serialize animation data
frantic::tstring serializeAnimationData( const frantic::maya::animation::animation_data& data ) {
    if( data.isEmpty() )
        return _T( "" );

    frantic::tsstream::tostringstream stream;
    stream << data.m_animCurveType << "," << data.m_preInfinityType << "," << data.m_postInfinityType << ","
           << ( data.m_weighted ? 1 : 0 ) << "#"; // separator

    bool first = true;
    for( std::vector<frantic::maya::animation::animation_data::animation_keyframe_data>::const_iterator iter =
             data.m_keyframes.begin();
         iter != data.m_keyframes.end(); ++iter ) {
        stream << ( first ? "" : ":" ) // separator
               << iter->m_time.unit() << "," << iter->m_time.value() << "," << iter->m_value << ","
               << iter->m_inTangent.m_tangentType << "," << iter->m_inTangent.m_angle.unit() << ","
               << iter->m_inTangent.m_angle.value() << "," << iter->m_inTangent.m_weight << ","
               << iter->m_outTangent.m_tangentType << "," << iter->m_outTangent.m_angle.unit() << ","
               << iter->m_outTangent.m_angle.value() << "," << iter->m_outTangent.m_weight;
        first = false;
    }

    return stream.str();
}

// Helper method to serialize vector3 animation data
frantic::tstring serializeAnimationData( const frantic::maya::animation::animation_data_vector3& data ) {
    frantic::tsstream::tostringstream stream;
    stream << serializeAnimationData( data.x() ) << ";" << serializeAnimationData( data.y() ) << ";"
           << serializeAnimationData( data.z() );
    return stream.str();
}

// Helper method to serialize quaternion animation data
frantic::tstring serializeAnimationData( const frantic::maya::animation::animation_data_vector4& data ) {
    frantic::tsstream::tostringstream stream;
    stream << serializeAnimationData( data.w() ) << ";" << serializeAnimationData( data.x() ) << ";"
           << serializeAnimationData( data.y() ) << ";" << serializeAnimationData( data.z() );
    return stream.str();
}

// Helper method to deserialize animation data
frantic::maya::animation::animation_data deserializeAnimationData( const MString& data ) {
    frantic::maya::animation::animation_data result;
    if( data.length() <= 0 )
        return result;

    MStringArray mainData;
    data.split( '#', mainData );

    MStringArray globalData;
    mainData[0].split( ',', globalData );
    result.m_animCurveType = static_cast<MFnAnimCurve::AnimCurveType>( globalData[0].asInt() );
    result.m_preInfinityType = static_cast<MFnAnimCurve::InfinityType>( globalData[1].asInt() );
    result.m_postInfinityType = static_cast<MFnAnimCurve::InfinityType>( globalData[2].asInt() );
    result.m_weighted = static_cast<bool>( globalData[3].asInt() );

    MStringArray keyFrames;
    mainData[1].split( ':', keyFrames );
    for( unsigned int i = 0; i < keyFrames.length(); i++ ) {
        frantic::maya::animation::animation_data::animation_keyframe_data current;

        MStringArray keyFrameData;
        keyFrames[i].split( ',', keyFrameData );

        current.m_time = MTime( keyFrameData[1].asDouble(), static_cast<MTime::Unit>( keyFrameData[0].asInt() ) );
        current.m_value = keyFrameData[2].asDouble();

        current.m_inTangent.m_tangentType = static_cast<MFnAnimCurve::TangentType>( keyFrameData[3].asInt() );
        current.m_inTangent.m_angle =
            MAngle( keyFrameData[5].asDouble(), static_cast<MAngle::Unit>( keyFrameData[4].asInt() ) );
        current.m_inTangent.m_weight = keyFrameData[6].asDouble();

        current.m_outTangent.m_tangentType = static_cast<MFnAnimCurve::TangentType>( keyFrameData[7].asInt() );
        current.m_outTangent.m_angle =
            MAngle( keyFrameData[9].asDouble(), static_cast<MAngle::Unit>( keyFrameData[8].asInt() ) );
        current.m_outTangent.m_weight = keyFrameData[10].asDouble();

        result.m_keyframes.push_back( current );
    }

    return result;
}

// Helper method to deserialize vector3 animation data
frantic::maya::animation::animation_data_vector3 deserializeAnimationDataVec3( const MString& data ) {
    MStringArray parts;
    data.split( ';', parts );

    frantic::maya::animation::animation_data x( deserializeAnimationData( parts[0] ) );
    frantic::maya::animation::animation_data y( deserializeAnimationData( parts[1] ) );
    frantic::maya::animation::animation_data z( deserializeAnimationData( parts[2] ) );
    frantic::maya::animation::animation_data_vector3 result( x, y, z );
    return result;
}

// Helper method to deserialize quaternion animation data
frantic::maya::animation::animation_data_vector4 deserializeAnimationDataVec4( const MString& data ) {
    MStringArray parts;
    data.split( ';', parts );

    frantic::maya::animation::animation_data w( deserializeAnimationData( parts[0] ) );
    frantic::maya::animation::animation_data x( deserializeAnimationData( parts[1] ) );
    frantic::maya::animation::animation_data y( deserializeAnimationData( parts[2] ) );
    frantic::maya::animation::animation_data z( deserializeAnimationData( parts[3] ) );
    frantic::maya::animation::animation_data_vector4 result( w, x, y, z );
    return result;
}
} // namespace detail

const MString maya_magma_mel_load_setup::commandName = "get_magma_description";

void* maya_magma_mel_load_setup::creator() { return new maya_magma_mel_load_setup; }

MStatus maya_magma_mel_load_setup::parseArgs( const MArgList& args,
                                              frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                                              MFnDependencyNode& depNode ) {
    MStatus outStatus( MS::kFailure );

    int options = 0;
    bool isGetNodes = frantic::maya::parser_args::get_bool_arg( args, "-n", "-nodes" );
    if( isGetNodes )
        options++;
    bool isGetEdges = frantic::maya::parser_args::get_bool_arg( args, "-e", "-edges" );
    if( isGetEdges )
        options++;
    bool isGetValues = frantic::maya::parser_args::get_bool_arg( args, "-v", "-values" );
    if( isGetValues )
        options++;
    bool isGetInputSocketValues = frantic::maya::parser_args::get_bool_arg( args, "-iv", "-inputsocketvalues" );
    if( isGetInputSocketValues )
        options++;
    bool isGetValuesAnimated = frantic::maya::parser_args::get_bool_arg( args, "-va", "-valuesanimated" );
    if( isGetValuesAnimated )
        options++;
    bool isGetInputSocketValuesAnimated =
        frantic::maya::parser_args::get_bool_arg( args, "-iva", "-inputsocketvaluesanimated" );
    if( isGetInputSocketValuesAnimated )
        options++;

    if( options > 1 ) {
        throw maya_magma_exception( "maya_magma_mel_load_setup::parseArgs multiple options given" );
    } else if( options <= 0 ) {
        throw maya_magma_exception( "maya_magma_mel_load_setup::parseArgs no option specified" );
    }

    bool isSaveSelectedOnly = frantic::maya::parser_args::get_bool_arg( args, "-s", "-selection" );

    std::set<desc::desc_id> isSelectedMap;
    if( isSaveSelectedOnly || isGetNodes ) {
        maya_MagmaFLUX* magma = maya_magma_mel_window::findMagmaFLUXWidgetByNode( depNode );
        if( magma != NULL ) {
            std::vector<int> ids = magma->getSelectedNodes();
            for( std::vector<int>::const_iterator iter = ids.begin(); iter != ids.end(); ++iter ) {
                isSelectedMap.insert( *iter );
            }
        } else if( isSaveSelectedOnly ) {
            MGlobal::displayWarning( "No window found for \"" + depNode.name() + "\".  Unable to get selected nodes." );
            return outStatus;
        }
    }

    if( isGetNodes ) {
        MStringArray result;
        const std::map<desc::desc_id, desc::maya_magma_desc_node>& nodes = desc->get_nodes();
        for( std::map<desc::desc_id, desc::maya_magma_desc_node>::const_iterator iter = nodes.begin();
             iter != nodes.end(); ++iter ) {
            desc::desc_id descID = iter->first;

            if( isSaveSelectedOnly && isSelectedMap.count( descID ) <= 0 )
                continue;

            frantic::tstring type = iter->second.get_node_type();
            frantic::tstring name = iter->second.get_node_name();

            int x = iter->second.get_x();
            int y = iter->second.get_y();

            {
                // Node ID
                std::stringstream s1;
                s1 << descID;
                result.append( s1.str().c_str() );
            }

            {
                // Node Type
                result.append( type.c_str() );
            }

            {
                // Node x position
                std::stringstream s1;
                s1 << x;
                result.append( s1.str().c_str() );
            }

            {
                // Node y position
                std::stringstream s1;
                s1 << y;
                result.append( s1.str().c_str() );
            }

            {
                // Node selected
                size_t selected = isSelectedMap.count( descID );
                std::stringstream s1;
                s1 << selected;
                result.append( s1.str().c_str() );
            }

            {
                // Node Name/Description
                result.append( name.c_str() );
            }
        }
        setResult( result );

    } else if( isGetEdges ) {
        MIntArray result;
        std::vector<desc::maya_magma_desc_connection> edges = desc->get_connections();
        for( std::vector<desc::maya_magma_desc_connection>::const_iterator iter = edges.begin(); iter != edges.end();
             ++iter ) {
            desc::desc_id src = iter->get_src_desc_node_id();
            desc::desc_id dst = iter->get_dst_desc_node_id();

            if( isSaveSelectedOnly && ( isSelectedMap.count( src ) <= 0 || isSelectedMap.count( dst ) <= 0 ) )
                continue;

            desc::desc_index_type srcIndex = iter->get_src_socket_index();
            desc::desc_index_type dstIndex = iter->get_dst_socket_index();

            result.append( src );
            result.append( srcIndex );
            result.append( dst );
            result.append( dstIndex );
        }
        setResult( result );

    } else if( isGetValues ) {
        MStringArray result;
        const std::map<desc::desc_id, desc::maya_magma_desc_node>& nodes = desc->get_nodes();
        for( std::map<desc::desc_id, desc::maya_magma_desc_node>::const_iterator nodeIter = nodes.begin();
             nodeIter != nodes.end(); ++nodeIter ) {

            if( isSaveSelectedOnly && isSelectedMap.count( nodeIter->first ) <= 0 )
                continue;

            std::vector<frantic::tstring> attributeNames = desc->get_desc_node_property_names( nodeIter->first );

            for( std::vector<frantic::tstring>::const_iterator iter = attributeNames.begin();
                 iter != attributeNames.end(); ++iter ) {

                frantic::tstring attribute = desc->get_maya_attr_name_from_node_property( nodeIter->first, *iter );
                holder::property_variant_t attributeValue =
                    attr::maya_magma_attr_manager::get_property_value_from_maya_attrs( attribute, depNode );

                {
                    // Node ID
                    std::stringstream s1;
                    s1 << nodeIter->first;
                    result.append( s1.str().c_str() );
                }

                {
                    // Property name
                    result.append( iter->c_str() );
                }

                {
                    // Property value
                    std::stringstream valueAsString;
                    if( attributeValue.type() == typeid( bool ) ) {
                        bool value = boost::get<bool>( attributeValue );
                        valueAsString << value;

                    } else if( attributeValue.type() == typeid( int ) ) {
                        int value = boost::get<int>( attributeValue );
                        valueAsString << value;

                    } else if( attributeValue.type() == typeid( float ) ) {
                        float value = boost::get<float>( attributeValue );
                        valueAsString << value;

                    } else if( attributeValue.type() == typeid( frantic::tstring ) ) {
                        frantic::tstring value = boost::get<frantic::tstring>( attributeValue );
                        valueAsString << frantic::strings::to_string( value );

                    } else if( attributeValue.type() == typeid( frantic::graphics::vector3f ) ) {
                        frantic::graphics::vector3f value = boost::get<frantic::graphics::vector3f>( attributeValue );
                        valueAsString << value.x << "," << value.y << "," << value.z;

                    } else if( attributeValue.type() == typeid( frantic::graphics::quat4f ) ) {
                        frantic::graphics::quat4f value = boost::get<frantic::graphics::quat4f>( attributeValue );
                        valueAsString << value.w << "," << value.x << "," << value.y << "," << value.z;

                    } else if( attributeValue.type() == typeid( std::vector<frantic::tstring> ) ) {
                        std::vector<frantic::tstring> value =
                            boost::get<std::vector<frantic::tstring>>( attributeValue );
                        bool first = true;
                        for( std::vector<frantic::tstring>::const_iterator iter = value.begin(); iter != value.end();
                             ++iter ) {
                            if( first )
                                first = false;
                            else
                                valueAsString << ",";
                            valueAsString << frantic::strings::to_string( *iter );
                        }

                    } else /*if ( value.type() == typeid(boost::blank) )*/ {
                        valueAsString << "";
                    }
                    result.append( valueAsString.str().c_str() );
                }
            }
        }

        setResult( result );

    } else if( isGetValuesAnimated ) {
        MStringArray result;
        const std::map<desc::desc_id, desc::maya_magma_desc_node>& nodes = desc->get_nodes();
        for( std::map<desc::desc_id, desc::maya_magma_desc_node>::const_iterator nodeIter = nodes.begin();
             nodeIter != nodes.end(); ++nodeIter ) {

            if( isSaveSelectedOnly && isSelectedMap.count( nodeIter->first ) <= 0 )
                continue;

            std::vector<frantic::tstring> attributeNames = desc->get_desc_node_property_names( nodeIter->first );

            for( std::vector<frantic::tstring>::const_iterator iter = attributeNames.begin();
                 iter != attributeNames.end(); ++iter ) {

                frantic::tstring attribute = desc->get_maya_attr_name_from_node_property( nodeIter->first, *iter );
                holder::property_animation_variant_t attributeAnim =
                    attr::maya_magma_attr_manager::get_property_animation_from_maya_attrs( attribute, depNode );

                // Make sure this property has an animation value
                if( attributeAnim.type() == typeid( boost::blank ) ) {
                    continue;
                }

                // Check Property value
                frantic::tstring valueAsString;
                if( attributeAnim.type() == typeid( frantic::maya::animation::animation_data ) ) {
                    frantic::maya::animation::animation_data value =
                        boost::get<frantic::maya::animation::animation_data>( attributeAnim );

                    // Don't bother saving if there's nothing
                    if( value.isEmpty() )
                        continue;

                    valueAsString = detail::serializeAnimationData( value );

                } else if( attributeAnim.type() == typeid( frantic::maya::animation::animation_data_vector3 ) ) {
                    frantic::maya::animation::animation_data_vector3 value =
                        boost::get<frantic::maya::animation::animation_data_vector3>( attributeAnim );

                    // Don't bother saving if there's nothing
                    if( value.isEmpty() )
                        continue;

                    valueAsString = detail::serializeAnimationData( value );

                } else if( attributeAnim.type() == typeid( frantic::maya::animation::animation_data_vector4 ) ) {
                    frantic::maya::animation::animation_data_vector4 value =
                        boost::get<frantic::maya::animation::animation_data_vector4>( attributeAnim );

                    // Don't bother saving if there's nothing
                    if( value.isEmpty() )
                        continue;

                    valueAsString = detail::serializeAnimationData( value );

                } else /*if ( value.type() == typeid(boost::blank) )*/ {
                    continue;
                }

                {
                    // Node ID
                    std::stringstream s1;
                    s1 << nodeIter->first;
                    result.append( s1.str().c_str() );
                }

                {
                    // Property name
                    result.append( iter->c_str() );
                }

                {
                    // Property value
                    result.append( valueAsString.c_str() );
                }
            }
        }

        setResult( result );

    } else if( isGetInputSocketValues ) {
        MStringArray result;

        // For each node
        std::map<frantic::tstring, info::maya_magma_node_info> typesToInfo;
        const std::map<desc::desc_id, desc::maya_magma_desc_node>& nodes = desc->get_nodes();
        for( std::map<desc::desc_id, desc::maya_magma_desc_node>::const_iterator nodeIter = nodes.begin();
             nodeIter != nodes.end(); ++nodeIter ) {

            if( isSaveSelectedOnly && isSelectedMap.count( nodeIter->first ) <= 0 )
                continue;

            frantic::tstring type = nodeIter->second.get_node_type();
            if( typesToInfo.count( type ) <= 0 ) {
                typesToInfo[type] = factory::maya_magma_node_info_factory::create_node_infos( type );
            }

            // For each input socket
            const std::vector<info::maya_magma_node_input_socket_info>& inputSocketInfos =
                typesToInfo[type].m_inputSocketInfos;
            std::vector<info::maya_magma_node_input_socket_info>::const_iterator citInputSocket;
            for( citInputSocket = inputSocketInfos.begin(); citInputSocket != inputSocketInfos.end();
                 citInputSocket++ ) {
                int index = citInputSocket->m_index;

                maya_magma_input_socket_data_type_t inputSocketDataType = citInputSocket->m_dataType;
                if( inputSocketDataType != MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_NONE ) {
                    std::vector<frantic::tstring> mayaAttrNames =
                        nodeIter->second.get_node_input_socket_maya_attr_names( index );

                    std::stringstream idstr;
                    idstr << nodeIter->first;
                    std::string idValue = idstr.str();
                    std::stringstream indstr;
                    indstr << index;
                    std::string indexValue = indstr.str();

                    bool isSupportBool = maya::is_input_socket_data_type_support_bool( inputSocketDataType );
                    bool isSupportFloat = maya::is_input_socket_data_type_support_float( inputSocketDataType );
                    bool isSupportInt = maya::is_input_socket_data_type_support_int( inputSocketDataType );
                    bool isSupportVec3 = maya::is_input_socket_data_type_support_vec3( inputSocketDataType );
                    bool isSupportQuat = maya::is_input_socket_data_type_support_quat( inputSocketDataType );

                    if( true ) {
                        // Enum Value
                        result.append( idValue.c_str() );    // index
                        result.append( indexValue.c_str() ); // socket
                        result.append( "enum" );             // type
                        {
                            // Value
                            frantic::tstring enumAttrName =
                                nodeIter->second.get_node_input_socket_maya_enum_attr_name( index );
                            int userSelectedDataType =
                                attr::maya_magma_attr_manager::get_input_socket_value_enum_from_maya_attrs(
                                    enumAttrName, depNode );
                            std::stringstream s1;
                            s1 << userSelectedDataType;
                            result.append( s1.str().c_str() );
                        }
                    }

                    if( isSupportBool ) {
                        // Bool
                        result.append( idValue.c_str() );    // index
                        result.append( indexValue.c_str() ); // socket
                        result.append( "bool" );             // type
                        {
                            // Value
                            holder::input_socket_variant_t value =
                                attr::maya_magma_attr_manager::get_input_socket_value_from_maya_attrs(
                                    MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_BOOL, mayaAttrNames, depNode );
                            std::stringstream s1;
                            s1 << boost::get<bool>( value );
                            result.append( s1.str().c_str() );
                        }
                    }
                    if( isSupportFloat ) {
                        // Float
                        result.append( idValue.c_str() );    // index
                        result.append( indexValue.c_str() ); // socket
                        result.append( "float" );            // type
                        {
                            // Value
                            holder::input_socket_variant_t value =
                                attr::maya_magma_attr_manager::get_input_socket_value_from_maya_attrs(
                                    MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_FLOAT, mayaAttrNames, depNode );
                            std::stringstream s1;
                            s1 << boost::get<float>( value );
                            result.append( s1.str().c_str() );
                        }
                    }
                    if( isSupportInt ) {
                        // Int
                        result.append( idValue.c_str() );    // index
                        result.append( indexValue.c_str() ); // socket
                        result.append( "int" );              // type
                        {
                            // Value
                            holder::input_socket_variant_t value =
                                attr::maya_magma_attr_manager::get_input_socket_value_from_maya_attrs(
                                    MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_INT, mayaAttrNames, depNode );
                            std::stringstream s1;
                            s1 << boost::get<int>( value );
                            result.append( s1.str().c_str() );
                        }
                    }
                    if( isSupportVec3 ) {
                        // Vector3
                        result.append( idValue.c_str() );    // index
                        result.append( indexValue.c_str() ); // socket
                        result.append( "vec3" );             // type
                        {
                            // Value
                            holder::input_socket_variant_t value =
                                attr::maya_magma_attr_manager::get_input_socket_value_from_maya_attrs(
                                    MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_VEC3, mayaAttrNames, depNode );
                            frantic::graphics::vector3f val = boost::get<frantic::graphics::vector3f>( value );
                            std::stringstream s1;
                            s1 << val.x << "," << val.y << "," << val.z;
                            result.append( s1.str().c_str() );
                        }
                    }
                    if( isSupportQuat ) {
                        // Quaternion
                        result.append( idValue.c_str() );    // index
                        result.append( indexValue.c_str() ); // socket
                        result.append( "quat" );             // type
                        {
                            // Value
                            holder::input_socket_variant_t value =
                                attr::maya_magma_attr_manager::get_input_socket_value_from_maya_attrs(
                                    MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_QUAT, mayaAttrNames, depNode );
                            frantic::graphics::quat4f val = boost::get<frantic::graphics::quat4f>( value );
                            std::stringstream s1;
                            s1 << val.w << "," << val.x << "," << val.y << "," << val.z;
                            result.append( s1.str().c_str() );
                        }
                    }
                }
            }
        }

        setResult( result );
    } else if( isGetInputSocketValuesAnimated ) {
        MStringArray result;

        // For each node
        std::map<frantic::tstring, info::maya_magma_node_info> typesToInfo;
        const std::map<desc::desc_id, desc::maya_magma_desc_node>& nodes = desc->get_nodes();
        for( std::map<desc::desc_id, desc::maya_magma_desc_node>::const_iterator nodeIter = nodes.begin();
             nodeIter != nodes.end(); ++nodeIter ) {

            if( isSaveSelectedOnly && isSelectedMap.count( nodeIter->first ) <= 0 )
                continue;

            frantic::tstring type = nodeIter->second.get_node_type();
            if( typesToInfo.count( type ) <= 0 ) {
                typesToInfo[type] = factory::maya_magma_node_info_factory::create_node_infos( type );
            }

            // For each input socket
            const std::vector<info::maya_magma_node_input_socket_info>& inputSocketInfos =
                typesToInfo[type].m_inputSocketInfos;
            std::vector<info::maya_magma_node_input_socket_info>::const_iterator citInputSocket;
            for( citInputSocket = inputSocketInfos.begin(); citInputSocket != inputSocketInfos.end();
                 citInputSocket++ ) {
                int index = citInputSocket->m_index;

                maya_magma_input_socket_data_type_t inputSocketDataType = citInputSocket->m_dataType;
                if( inputSocketDataType != MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_NONE ) {
                    std::vector<frantic::tstring> mayaAttrNames =
                        nodeIter->second.get_node_input_socket_maya_attr_names( index );

                    std::stringstream idstr;
                    idstr << nodeIter->first;
                    std::string idValue = idstr.str();
                    std::stringstream indstr;
                    indstr << index;
                    std::string indexValue = indstr.str();

                    bool isSupportFloat = maya::is_input_socket_data_type_support_float( inputSocketDataType );
                    bool isSupportInt = maya::is_input_socket_data_type_support_int( inputSocketDataType );
                    bool isSupportVec3 = maya::is_input_socket_data_type_support_vec3( inputSocketDataType );
                    bool isSupportQuat = maya::is_input_socket_data_type_support_quat( inputSocketDataType );

                    if( isSupportFloat ) {
                        // Float
                        holder::input_socket_animation_variant_t value =
                            attr::maya_magma_attr_manager::get_input_socket_animation_from_maya_attrs(
                                MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_FLOAT, mayaAttrNames, depNode );
                        frantic::maya::animation::animation_data animData =
                            boost::get<frantic::maya::animation::animation_data>( value );

                        // Don't bother saving if there's nothing
                        if( !animData.isEmpty() ) {
                            frantic::tstring s1 = detail::serializeAnimationData( animData );
                            result.append( idValue.c_str() );    // index
                            result.append( indexValue.c_str() ); // socket
                            result.append( "float" );            // type
                            result.append( s1.c_str() );         // value
                        }
                    }
                    if( isSupportInt ) {
                        // Int
                        holder::input_socket_animation_variant_t value =
                            attr::maya_magma_attr_manager::get_input_socket_animation_from_maya_attrs(
                                MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_INT, mayaAttrNames, depNode );
                        frantic::maya::animation::animation_data animData =
                            boost::get<frantic::maya::animation::animation_data>( value );

                        // Don't bother saving if there's nothing
                        if( !animData.isEmpty() ) {
                            frantic::tstring s1 = detail::serializeAnimationData( animData );
                            result.append( s1.c_str() );
                            result.append( idValue.c_str() );    // index
                            result.append( indexValue.c_str() ); // socket
                            result.append( "int" );              // type
                            result.append( s1.c_str() );         // value
                        }
                    }
                    if( isSupportVec3 ) {
                        // Vector3
                        holder::input_socket_animation_variant_t value =
                            attr::maya_magma_attr_manager::get_input_socket_animation_from_maya_attrs(
                                MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_VEC3, mayaAttrNames, depNode );
                        frantic::maya::animation::animation_data_vector3 animData =
                            boost::get<frantic::maya::animation::animation_data_vector3>( value );

                        // Don't bother saving if there's nothing
                        if( !animData.isEmpty() ) {
                            frantic::tstring s1 = detail::serializeAnimationData( animData );
                            result.append( idValue.c_str() );    // index
                            result.append( indexValue.c_str() ); // socket
                            result.append( "vec3" );             // type
                            result.append( s1.c_str() );         // value
                        }
                    }
                    if( isSupportQuat ) {
                        // Quaternion
                        holder::input_socket_animation_variant_t value =
                            attr::maya_magma_attr_manager::get_input_socket_animation_from_maya_attrs(
                                MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_QUAT, mayaAttrNames, depNode );
                        frantic::maya::animation::animation_data_vector4 animData =
                            boost::get<frantic::maya::animation::animation_data_vector4>( value );

                        // Don't bother saving if there's nothing
                        if( !animData.isEmpty() ) {
                            frantic::tstring s1 = detail::serializeAnimationData( animData );
                            result.append( idValue.c_str() );    // index
                            result.append( indexValue.c_str() ); // socket
                            result.append( "quat" );             // type
                            result.append( s1.c_str() );         // value
                        }
                    }
                }
            }
        }

        setResult( result );
    }

    outStatus = MS::kSuccess;
    return outStatus;
}

////////////////////////////////////////////////////////////////////////////////

const MString maya_magma_mel_apply_setup::commandName = "apply_magma_description";

void* maya_magma_mel_apply_setup::creator() { return new maya_magma_mel_apply_setup; }

namespace detail {
// helper struct for parseArgs function
struct nodeInfo {
    frantic::tstring type;
    int x;
    int y;
    bool selected;
    frantic::tstring name;
};
} // namespace detail

MStatus maya_magma_mel_apply_setup::parseArgs( const MArgList& args,
                                               frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                                               MFnDependencyNode& depNode ) {
    MStatus outStatus( MS::kFailure );

    bool clear = frantic::maya::parser_args::get_bool_arg( args, "-rn", "-replacenodes" );

    MStringArray nodes;
    bool hasNodes = frantic::maya::parser_args::get_string_array_arg( args, "-n", "-nodes", nodes );

    MIntArray edges;
    bool hasEdges = frantic::maya::parser_args::get_int_array_arg( args, "-e", "-edges", edges );

    MStringArray values;
    bool hasValues = frantic::maya::parser_args::get_string_array_arg( args, "-v", "-values", values );

    MStringArray inputSockets;
    bool hasInputSockets =
        frantic::maya::parser_args::get_string_array_arg( args, "-iv", "-inputsocketvalues", inputSockets );

    MStringArray valuesAnimated;
    bool hasValuesAnimated =
        frantic::maya::parser_args::get_string_array_arg( args, "-va", "-valuesanimated", valuesAnimated );

    MStringArray inputSocketsAnimated;
    bool hasInputSocketsAnimated = frantic::maya::parser_args::get_string_array_arg(
        args, "-iva", "-inputsocketvaluesanimated", inputSocketsAnimated );

    int xOffset;
    bool hasXOffset = frantic::maya::parser_args::get_int_arg( args, "-x", "-xoffset", xOffset );

    int yOffset;
    bool hasYOffset = frantic::maya::parser_args::get_int_arg( args, "-y", "-yoffset", yOffset );

    bool overwrite;
    if( !hasNodes ) {
        // throw maya_magma_exception("maya_magma_mel_apply_setup::parseArgs no nodes given");
        overwrite = true;
    } else {
        overwrite = false;
    }

    bool selectNodes;
    selectNodes = hasNodes;

    // Lets parse some of the arguments first so we don't end up in an inconsistent state when an error occurs
    std::map<frantic::tstring, info::maya_magma_node_info> typesToInfo;
    std::map<desc::desc_id, detail::nodeInfo> nodeIDToData;
    if( hasNodes ) {
        for( unsigned int i = 0; i < nodes.length(); i += 6 ) {
            detail::nodeInfo data;

            desc::desc_id id = nodes[i + 0].asInt();

            data.type = frantic::maya::from_maya_t( nodes[i + 1] );

            data.x = nodes[i + 2].asInt();
            data.y = nodes[i + 3].asInt();

            data.selected =
                ( nodes[i + 4].isInt() ) ? ( nodes[i + 4].asInt() != 0 ) : ( nodes[i + 4] == MString( "true" ) );

            data.name = frantic::maya::from_maya_t( nodes[i + 5] );

            if( hasXOffset )
                data.x += xOffset;
            if( hasYOffset )
                data.y += yOffset;

            if( typesToInfo.count( data.type ) <= 0 ) {
                typesToInfo[data.type] =
                    frantic::magma::maya::factory::maya_magma_node_info_factory::create_node_infos( data.type );
            }

            nodeIDToData[id] = data;
        }
    }

    maya_MagmaFLUX* magma = maya_magma_mel_window::findMagmaFLUXWidgetByNode( depNode );

    if( clear ) {
        // Clear the current graph
        const std::map<desc::desc_id, desc::maya_magma_desc_node>& nodes = desc->get_nodes();
        std::vector<desc::desc_id> todelete;
        for( std::map<desc::desc_id, desc::maya_magma_desc_node>::const_iterator iter = nodes.begin();
             iter != nodes.end(); ++iter ) {
            todelete.push_back( iter->first );
        }

        for( std::vector<desc::desc_id>::const_iterator iter = todelete.begin(); iter != todelete.end(); ++iter ) {
            desc::desc_id descID = *iter;

            // delete corresponding maya attributes of that desc node
            attr::maya_magma_attr_manager::delete_maya_attr( desc, descID, depNode );

            // delete that desc node
            desc->delete_node( descID );

            if( magma != NULL ) {
                magma->removeNode( descID );
            }
        }
    }

    std::map<desc::desc_id, desc::desc_id> remap;

    if( hasNodes ) {
        // Add the nodes
        for( std::map<desc::desc_id, detail::nodeInfo>::iterator iter = nodeIDToData.begin();
             iter != nodeIDToData.end(); ++iter ) {

            // retrieve magma_node info from
            info::maya_magma_node_info magmaNodeInfo = typesToInfo[iter->second.type];

            // create a maya_magma_desc_node
            desc::desc_id descID = desc->create_node( iter->second.type );

            // process maya_magma_node_info
            attr::maya_magma_attr_manager::create_maya_attr( magmaNodeInfo, desc, descID, depNode );

            // Position
            desc->set_node_position( descID, iter->second.x, iter->second.y );

            // Name
            desc->set_node_name( descID, iter->second.name );

            // Update the index in this PRT Object
            remap[iter->first] = descID;

            if( magma != NULL ) {
                magma->addNode( descID, desc, depNode );
            }
        }
    }

    // Set the properties
    if( hasValues ) {
        for( unsigned int i = 0; i < values.length(); i += 3 ) {
            desc::desc_id id = values[i + 0].asInt();
            frantic::tstring name = frantic::maya::from_maya_t( values[i + 1] );
            MString val = values[i + 2];

            desc::desc_id remapId;
            if( overwrite ) {
                remapId = id;
            } else {
                if( remap.count( id ) <= 0 )
                    continue;
                remapId = remap[id];
            }

            // Update the property
            frantic::tstring attribute = desc->get_maya_attr_name_from_node_property( remapId, name );
            holder::property_variant_t attributeValue =
                attr::maya_magma_attr_manager::get_property_value_from_maya_attrs( attribute, depNode );

            // Use the return value to determine what type it should be and assign the new value appropriately
            try {
                if( attributeValue.type() == typeid( bool ) ) {
                    bool value;
                    if( val.isInt() )
                        value = ( val.asInt() != 0 );
                    else
                        value = ( val == MString( "true" ) );
                    attributeValue = value;

                } else if( attributeValue.type() == typeid( int ) ) {
                    int value = val.asInt();
                    attributeValue = value;

                } else if( attributeValue.type() == typeid( float ) ) {
                    float value = val.asFloat();
                    attributeValue = value;

                } else if( attributeValue.type() == typeid( frantic::tstring ) ) {
                    frantic::tstring value = frantic::maya::from_maya_t( val );
                    attributeValue = value;

                } else if( attributeValue.type() == typeid( frantic::graphics::vector3f ) ) {
                    MStringArray valArray;
                    val.split( ',', valArray );
                    frantic::graphics::vector3f value( valArray[0].asFloat(), valArray[1].asFloat(),
                                                       valArray[2].asFloat() );
                    attributeValue = value;

                } else if( attributeValue.type() == typeid( frantic::graphics::quat4f ) ) {
                    MStringArray valArray;
                    val.split( ',', valArray );
                    frantic::graphics::quat4f value( valArray[0].asFloat(), valArray[1].asFloat(),
                                                     valArray[2].asFloat(), valArray[3].asFloat() );
                    attributeValue = value;

                } else if( attributeValue.type() == typeid( std::vector<frantic::tstring> ) ) {
                    MStringArray valArray;
                    val.split( ',', valArray );
                    std::vector<frantic::tstring> value = frantic::maya::from_maya_t( valArray );
                    attributeValue = value;

                } else /*if ( value.type() == typeid(boost::blank) )*/ {
                    continue;
                }
            } catch( std::exception& e ) {
                FF_LOG( debug ) << "maya_magma_mel_apply_setup: set properties: error: " << e.what() << std::endl;
                continue;
            }

            attr::maya_magma_attr_manager::set_property_value_from_maya_attrs( attribute, depNode, attributeValue );
        }
    }

    // Set the properties for animation
    if( hasValuesAnimated ) {
        for( unsigned int i = 0; i < valuesAnimated.length(); i += 3 ) {
            desc::desc_id id = valuesAnimated[i + 0].asInt();
            frantic::tstring name = frantic::maya::from_maya_t( valuesAnimated[i + 1] );
            MString val = valuesAnimated[i + 2];

            desc::desc_id remapId;
            if( overwrite ) {
                remapId = id;
            } else {
                if( remap.count( id ) <= 0 )
                    continue;
                remapId = remap[id];
            }

            // Update the property
            frantic::tstring attribute = desc->get_maya_attr_name_from_node_property( remapId, name );
            holder::property_variant_t attributeValue =
                attr::maya_magma_attr_manager::get_property_value_from_maya_attrs( attribute, depNode );

            // Use the return value to determine what type it should be and assign the new value appropriately
            holder::property_animation_variant_t animationValue;
            try {
                if( attributeValue.type() == typeid( bool ) ) {
                    // Does not support animation, ignore it
                    continue;

                } else if( attributeValue.type() == typeid( int ) ) {
                    animationValue = detail::deserializeAnimationData( val );

                } else if( attributeValue.type() == typeid( float ) ) {
                    animationValue = detail::deserializeAnimationData( val );

                } else if( attributeValue.type() == typeid( frantic::tstring ) ) {
                    // Does not support animation, ignore it
                    continue;

                } else if( attributeValue.type() == typeid( frantic::graphics::vector3f ) ) {
                    animationValue = detail::deserializeAnimationDataVec3( val );

                } else if( attributeValue.type() == typeid( frantic::graphics::quat4f ) ) {
                    animationValue = detail::deserializeAnimationDataVec4( val );

                } else if( attributeValue.type() == typeid( std::vector<frantic::tstring> ) ) {
                    // Does not support animation, ignore it
                    continue;

                } else /*if ( value.type() == typeid(boost::blank) )*/ {
                    continue;
                }
            } catch( std::exception& e ) {
                FF_LOG( debug ) << "maya_magma_mel_apply_setup: set properties: error: " << e.what() << std::endl;
                continue;
            }

            attr::maya_magma_attr_manager::set_property_animation_from_maya_attrs( attribute, depNode, animationValue );
        }
    }

    // Set the input socket properties
    if( hasInputSockets ) {
        for( unsigned int i = 0; i < inputSockets.length(); i += 4 ) {
            desc::desc_id nodeID = inputSockets[i + 0].asInt();
            desc::desc_index_type socketID = inputSockets[i + 1].asInt();
            MString type = inputSockets[i + 2];
            MString mvalue = inputSockets[i + 3];

            desc::desc_id remapID;
            if( overwrite ) {
                remapID = nodeID;
            } else {
                if( remap.count( nodeID ) <= 0 )
                    continue;
                remapID = remap[nodeID];
            }

            if( type == MString( "enum" ) ) {
                frantic::tstring enumAttrName =
                    desc->get_desc_node_input_socket_maya_enum_attr_name( remapID, socketID );
                maya_attribute_input_socket_data_type value =
                    static_cast<maya_attribute_input_socket_data_type>( mvalue.asInt() );
                attr::maya_magma_attr_manager::set_input_socket_value_enum_from_maya_attrs( enumAttrName, depNode,
                                                                                            value );

            } else {

                maya_attribute_input_socket_data_type index;
                holder::input_socket_variant_t attributeValue;

                std::vector<frantic::tstring> mayaAttrNames = desc->get_desc_node_input_socket_maya_attr_names(
                    remapID, socketID ); // TODO: move this to a map for efficiency

                if( type == MString( "bool" ) ) {
                    index = MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_BOOL;
                    bool value;
                    if( mvalue.isInt() )
                        value = ( mvalue.asInt() != 0 );
                    else
                        value = ( mvalue == MString( "true" ) );
                    attributeValue = value;

                } else if( type == MString( "float" ) ) {
                    index = MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_FLOAT;
                    float value = mvalue.asFloat();
                    attributeValue = value;

                } else if( type == MString( "int" ) ) {
                    index = MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_INT;
                    int value = mvalue.asInt();
                    attributeValue = value;

                } else if( type == MString( "vec3" ) ) {
                    index = MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_VEC3;
                    MStringArray valArray;
                    mvalue.split( ',', valArray );
                    frantic::graphics::vector3f value( valArray[0].asFloat(), valArray[1].asFloat(),
                                                       valArray[2].asFloat() );
                    attributeValue = value;

                } else if( type == MString( "quat" ) ) {
                    index = MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_QUAT;
                    MStringArray valArray;
                    mvalue.split( ',', valArray );
                    frantic::graphics::quat4f value( valArray[0].asFloat(), valArray[1].asFloat(),
                                                     valArray[2].asFloat(), valArray[3].asFloat() );
                    attributeValue = value;

                } else {
                    continue;
                }

                attr::maya_magma_attr_manager::set_input_socket_value_from_maya_attrs( index, mayaAttrNames, depNode,
                                                                                       attributeValue );
            }
        }
    }

    // Set the input socket properties
    if( hasInputSocketsAnimated ) {
        for( unsigned int i = 0; i < inputSocketsAnimated.length(); i += 4 ) {
            desc::desc_id nodeID = inputSocketsAnimated[i + 0].asInt();
            desc::desc_index_type socketID = inputSocketsAnimated[i + 1].asInt();
            MString type = inputSocketsAnimated[i + 2];
            MString mvalue = inputSocketsAnimated[i + 3];

            desc::desc_id remapID;
            if( overwrite ) {
                remapID = nodeID;
            } else {
                if( remap.count( nodeID ) <= 0 )
                    continue;
                remapID = remap[nodeID];
            }

            maya_attribute_input_socket_data_type index;
            holder::input_socket_animation_variant_t animationValue;

            std::vector<frantic::tstring> mayaAttrNames = desc->get_desc_node_input_socket_maya_attr_names(
                remapID, socketID ); // TODO: move this to a map for efficiency

            if( type == MString( "float" ) ) {
                index = MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_FLOAT;
                animationValue = detail::deserializeAnimationData( mvalue );

            } else if( type == MString( "int" ) ) {
                index = MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_INT;
                animationValue = detail::deserializeAnimationData( mvalue );

            } else if( type == MString( "vec3" ) ) {
                index = MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_VEC3;
                animationValue = detail::deserializeAnimationDataVec3( mvalue );

            } else if( type == MString( "quat" ) ) {
                index = MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_QUAT;
                animationValue = detail::deserializeAnimationDataVec4( mvalue );

            } else {
                continue;
            }

            attr::maya_magma_attr_manager::set_input_socket_animation_from_maya_attrs( index, mayaAttrNames, depNode,
                                                                                       animationValue );
        }
    }

    // Set the connections (done last since properties could affect available sockets)
    if( hasEdges ) {
        for( unsigned int i = 0; i < edges.length(); i += 4 ) {
            desc::desc_id srcID = edges[i + 0];
            desc::desc_index_type srcSock = edges[i + 1];
            desc::desc_id dstID = edges[i + 2];
            desc::desc_index_type dstSock = edges[i + 3];

            desc::desc_id remapSrc;
            desc::desc_id remapDst;
            if( overwrite ) {
                remapSrc = srcID;
                remapDst = dstID;
            } else {
                if( remap.count( srcID ) <= 0 || remap.count( dstID ) <= 0 )
                    continue;
                remapSrc = remap[srcID];
                remapDst = remap[dstID];
            }

            desc->create_connection( remapSrc, srcSock, remapDst, dstSock );

            if( magma != NULL ) {
                magma->addConnection( remapSrc, srcSock, remapDst, dstSock );
            }
        }
    }

    // Reselect nodes
    if( hasNodes ) {
        if( selectNodes ) {
            if( magma != NULL ) {
                std::vector<int> nodes;
                for( std::map<desc::desc_id, detail::nodeInfo>::iterator iter = nodeIDToData.begin();
                     iter != nodeIDToData.end(); ++iter ) {
                    if( iter->second.selected ) {
                        desc::desc_id id = iter->first;
                        desc::desc_id remapID;
                        if( overwrite ) {
                            remapID = id;
                        } else {
                            remapID = remap[id];
                        }

                        nodes.push_back( remapID );
                    }
                }

                std::vector<maya_MagmaFLUX::connection_data> edges;
                magma->setSelection( nodes, edges );
            }
        }
    }

    outStatus = MS::kSuccess;
    return outStatus;
}

} // namespace mel
} // namespace maya
} // namespace magma
} // namespace frantic
