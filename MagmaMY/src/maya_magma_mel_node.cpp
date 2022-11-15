// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "stdafx.h"

#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnNumericAttribute.h>

#include "frantic/convert/tstring.hpp"
#include "frantic/maya/parser_args.hpp"
#include "frantic/maya/selection.hpp"
#include <frantic/logging/logging_level.hpp>
#include <frantic/maya/convert.hpp>

#include "frantic/magma/maya/maya_magma_holder.hpp"
#include "frantic/magma/maya/maya_magma_mel_node.hpp"

#include "frantic/magma/maya/maya_magma_attr_manager.hpp"
#include "frantic/magma/maya/maya_magma_datatypes.hpp"
#include "frantic/magma/maya/maya_magma_info_factory.hpp"
#include "frantic/magma/maya/maya_magma_singleton.hpp"

#include "frantic/magma/maya/maya_magma_gui.hpp"
#include "frantic/magma/maya/maya_magma_mel_window.hpp"

namespace frantic {
namespace magma {
namespace maya {
namespace mel {

const MString maya_magma_mel_create_node::commandName = "create_magma_node";

void* maya_magma_mel_create_node::creator() { return new maya_magma_mel_create_node; }

MStatus maya_magma_mel_create_node::parseArgs( const MArgList& args,
                                               frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                                               MFnDependencyNode& depNode ) {
    MStatus outStatus( MS::kFailure );
    MString nodeType;

    if( MS::kSuccess == frantic::maya::parser_args::get_arg( args, "-t", "-type", nodeType ) ) {

        int parent;
        bool hasParent = frantic::maya::parser_args::get_int_arg( args, "-p", "-parent", parent );

        // retrieve magma_node info from
        info::maya_magma_node_info magmaNodeInfo = factory::maya_magma_node_info_factory::create_node_infos(
            frantic::strings::to_tstring( nodeType.asChar() ) );

        // create a maya_magma_desc_node
#if defined( FRANTIC_USE_WCHAR )
        desc::desc_id descID = desc->create_node( nodeType.asWChar() );
#else
        desc::desc_id descID = desc->create_node( nodeType.asChar() );
#endif
        if( hasParent ) {
            desc->set_node_parent( descID, parent );
        }

        // process maya_magma_node_info
        attr::maya_magma_attr_manager::create_maya_attr( magmaNodeInfo, desc, descID, depNode );

        // Add internal nodes/default connections
        if( magmaNodeInfo.m_nodeType == _T("BLOP") || magmaNodeInfo.m_nodeType == _T("Loop") ) {
            frantic::tstring inputNodeName, outputNodeName;
            if( magmaNodeInfo.m_nodeType == _T("BLOP") ) {
                inputNodeName = _T( "BLOPSocket" );
                outputNodeName = _T( "BLOPOutput" );
            } else if( magmaNodeInfo.m_nodeType == _T("Loop") ) {
                inputNodeName = _T( "Loop__Input" );
                outputNodeName = _T( "Loop__Output" );
            }

            desc::desc_id inputNodeID = desc->create_node( inputNodeName );
            desc->set_node_parent( inputNodeID, descID );
            desc::desc_id outputNodeIF = desc->create_node( outputNodeName );
            desc->set_node_parent( outputNodeIF, descID );

            if( magmaNodeInfo.m_nodeType == _T("Loop") ) {
                desc->create_connection( inputNodeID, 1, outputNodeIF, 1 );
            }
        }

        FF_LOG( debug ) << _T( "create_magma_node nodeType:" ) << frantic::strings::to_tstring( nodeType.asChar() )
                        << _T( " descID:" ) << descID << std::endl;
        setResult( descID );
        outStatus = MS::kSuccess;

        int xpos, ypos;
        if( MS::kSuccess != frantic::maya::parser_args::get_arg( args, "-x", "-xposition", xpos ) ) {
            xpos = 0;
        }
        if( MS::kSuccess != frantic::maya::parser_args::get_arg( args, "-y", "-yposition", ypos ) ) {
            ypos = 0;
        }
        desc->set_node_position( descID, xpos, ypos );

        maya_MagmaFLUX* magma = maya_magma_mel_window::findMagmaFLUXWidgetByNode( depNode );
        if( magma != NULL ) {
            magma->addNode( descID, desc, depNode );
        }
    }
    return outStatus;
}

////////////////////////////////////////////////////////////////////////////////

const MString maya_magma_mel_delete_node::commandName = "delete_magma_node";

namespace {

void deleteBLOPNodeHelper( std::vector<desc::desc_id>& toDelete, frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                           MFnDependencyNode& depNode ) {

    // Go through and get all the sub nodes
    const std::map<desc::desc_id, desc::maya_magma_desc_node>& nodesMap = desc->get_nodes();
    for( std::size_t i = 0; i < toDelete.size(); i++ ) {
        desc::desc_id currentParent = toDelete[i];

        for( std::map<desc::desc_id, desc::maya_magma_desc_node>::const_iterator iter = nodesMap.begin();
             iter != nodesMap.end(); ++iter ) {
            if( desc->get_node_parent( iter->first ) == currentParent )
                toDelete.push_back( iter->first );
        }
    }

    // Delete them all
    for( std::vector<desc::desc_id>::const_iterator iter = toDelete.begin(); iter != toDelete.end(); ++iter ) {
        desc::desc_id descID = *iter;

        // delete corresponding maya attributes of that desc node
        attr::maya_magma_attr_manager::delete_maya_attr( desc, descID, depNode );

        // delete that desc node
        desc->delete_node( descID );
    }
}
} // namespace

void* maya_magma_mel_delete_node::creator() { return new maya_magma_mel_delete_node; }

MStatus maya_magma_mel_delete_node::parseArgs( const MArgList& args,
                                               frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                                               MFnDependencyNode& depNode ) {
    MStatus outStatus( MS::kFailure );
    int descID;
    if( MS::kSuccess == frantic::maya::parser_args::get_arg( args, "-i", "-id", descID ) ) {

        if( desc->get_node_parent( descID ) >= 0 ) {
            // Delete BLOP node, go ahead and delete all the sub nodes
            std::vector<desc::desc_id> toDelete;
            toDelete.push_back( descID );
            deleteBLOPNodeHelper( toDelete, desc, depNode );

        } else {
            // delete corresponding maya attributes of that desc node
            attr::maya_magma_attr_manager::delete_maya_attr( desc, descID, depNode );

            // delete that desc node
            desc->delete_node( descID );
        }

        FF_LOG( debug ) << "delete_magma_node: descID:" << descID << std::endl;
        setResult( "Success" );
        outStatus = MS::kSuccess;

        maya_MagmaFLUX* magma = maya_magma_mel_window::findMagmaFLUXWidgetByNode( depNode );
        if( magma != NULL ) {
            magma->removeNode( descID );
        }
    }
    return outStatus;
}

////////////////////////////////////////////////////////////////////////////////

const MString maya_magma_mel_retrieve_node_instance_info::commandName = "retrieve_magma_node_instance_info";

void* maya_magma_mel_retrieve_node_instance_info::creator() { return new maya_magma_mel_retrieve_node_instance_info; }

MStatus maya_magma_mel_retrieve_node_instance_info::parseArgs( const MArgList& args,
                                                               frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                                                               MFnDependencyNode& depNode ) {
    FF_LOG( debug ) << "maya_magma_mel_retrieve_node_instance_info:" << std::endl;
    MStatus outStatus( MS::kFailure );
    int descID;
    if( MS::kSuccess == frantic::maya::parser_args::get_arg( args, "-i", "-id", descID ) ) {

        const std::map<desc::desc_id, desc::maya_magma_desc_node>& nodes = desc->get_nodes();
        std::map<desc::desc_id, desc::maya_magma_desc_node>::const_iterator iter = nodes.find( descID );
        if( iter == nodes.end() ) {
            throw maya_magma_exception( "maya_magma_mel_retrieve_node_instance_info: no node of given ID" );
        }

        int options = 0;
        int colorIndex = -1;
        bool isGetType = frantic::maya::parser_args::get_bool_arg( args, "-t", "-type" );
        if( isGetType )
            options++;
        bool isGetX = frantic::maya::parser_args::get_bool_arg( args, "-x", "-xposition" );
        if( isGetX )
            options++;
        bool isGetY = frantic::maya::parser_args::get_bool_arg( args, "-y", "-yposition" );
        if( isGetY )
            options++;
        bool isGetXY = frantic::maya::parser_args::get_bool_arg( args, "-xy", "-xyposition" );
        if( isGetXY )
            options++;
        bool isGetName = frantic::maya::parser_args::get_bool_arg( args, "-n", "-name" );
        if( isGetName )
            options++;
        bool isGetNumInputSockets = frantic::maya::parser_args::get_bool_arg( args, "-is", "-inputSocket" );
        if( isGetNumInputSockets )
            options++;
        bool isGetNumOutputSockets = frantic::maya::parser_args::get_bool_arg( args, "-os", "-outputSockets" );
        if( isGetNumOutputSockets )
            options++;

        if( options > 1 ) {
            throw maya_magma_exception( "maya_magma_mel_retrieve_node_instance_info: multiple options given" );
        }

        if( isGetType ) {
            setResult( frantic::strings::to_string( iter->second.get_node_type() ).c_str() );
            outStatus = MS::kSuccess;
        } else if( isGetX ) {
            int xpos, ypos;
            desc->get_node_position( descID, xpos, ypos );
            setResult( xpos );
            outStatus = MS::kSuccess;
        } else if( isGetY ) {
            int xpos, ypos;
            desc->get_node_position( descID, xpos, ypos );
            setResult( ypos );
            outStatus = MS::kSuccess;
        } else if( isGetXY ) {
            MIntArray result( 2 );
            int xpos, ypos;
            desc->get_node_position( descID, xpos, ypos );
            result[0] = xpos;
            result[1] = ypos;
            setResult( result );
            outStatus = MS::kSuccess;
        } else if( isGetName ) {
            frantic::tstring name = desc->get_node_name( descID );
            setResult( frantic::maya::to_maya_t( name ) );
            outStatus = MS::kSuccess;
        } else if( isGetNumInputSockets ) {
            MStringArray result;
            frantic::tstring type = iter->second.get_node_type();
            if( type == _T("Mux") ) {
                frantic::tstring attribute =
                    desc->get_maya_attr_name_from_node_property( iter->first, _T("NumInputs") );
                holder::property_variant_t attributeValue =
                    attr::maya_magma_attr_manager::get_property_value_from_maya_attrs( attribute, depNode );
                int value = boost::get<int>( attributeValue );

                for( int i = 0; i < value - 1; i++ ) {
                    std::stringstream str;
                    str << "Input " << i;

                    frantic::magma::maya::info::maya_magma_node_input_socket_info socket;
                    socket.m_dataType = MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_NONE;
                    socket.m_index = i;
                    socket.m_description = frantic::strings::to_tstring( str.str() );
                    result.append( frantic::strings::to_string( socket.to_tstring() ).c_str() );
                }
                {
                    frantic::magma::maya::info::maya_magma_node_input_socket_info socket;
                    socket.m_index = value - 1;
                    socket.m_description = _T("Selector");
                    result.append( frantic::strings::to_string( socket.to_tstring() ).c_str() );
                }

            } else {
                info::maya_magma_node_info magmaNodeInfo =
                    factory::maya_magma_node_info_factory::create_node_infos( type );
                magmaNodeInfo.add_input_socket_to_list( result );
            }
            setResult( result );
            outStatus = MS::kSuccess;
        } else if( isGetNumOutputSockets ) {
            MStringArray result;
            frantic::tstring type = iter->second.get_node_type();

            info::maya_magma_node_info magmaNodeInfo = factory::maya_magma_node_info_factory::create_node_infos( type );
            // In the description for ParticleQuery it has "position" socket out by default, but this is meant to be
            // accounted for in one of the attributes
            if( type != _T("ParticleQuery") && type != _T("PropertyQuery") ) {
                magmaNodeInfo.add_output_socket_to_list( result );
            }

            frantic::tstring extraSockets;
            if( iter->second.has_node_property_name( _T( "channels" ) ) ) {
                extraSockets = _T( "channels" );
            } else if( iter->second.has_node_property_name( _T( "properties" ) ) ) {
                extraSockets = _T( "properties" );
            }

            if( extraSockets.length() > 0 ) {
                frantic::tstring attribute = desc->get_maya_attr_name_from_node_property( iter->first, extraSockets );
                holder::property_variant_t attributeValue =
                    attr::maya_magma_attr_manager::get_property_value_from_maya_attrs( attribute, depNode );
                std::vector<frantic::tstring> value = boost::get<std::vector<frantic::tstring>>( attributeValue );

                int nextIndex = result.length();
                for( std::vector<frantic::tstring>::const_iterator extras = value.begin(); extras != value.end();
                     ++extras ) {
                    frantic::magma::maya::info::maya_magma_node_output_socket_info socket;
                    socket.m_index = nextIndex;
                    socket.m_description = ( *extras );
                    result.append( frantic::strings::to_string( socket.to_tstring() ).c_str() );
                    nextIndex++;
                }
            }

            setResult( result );
            outStatus = MS::kSuccess;
        }
    }
    return outStatus;
}

////////////////////////////////////////////////////////////////////////////////

const MString maya_magma_mel_set_node_instance_info::commandName = "set_magma_node_instance_info";

void* maya_magma_mel_set_node_instance_info::creator() { return new maya_magma_mel_set_node_instance_info; }

MStatus maya_magma_mel_set_node_instance_info::parseArgs( const MArgList& args,
                                                          frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                                                          MFnDependencyNode& depNode ) {
    FF_LOG( debug ) << "maya_magma_mel_set_node_instance_info:" << std::endl;
    MStatus outStatus( MS::kFailure );
    int descID;
    if( MS::kSuccess == frantic::maya::parser_args::get_arg( args, "-i", "-id", descID ) ) {

        const std::map<desc::desc_id, desc::maya_magma_desc_node>& nodes = desc->get_nodes();
        std::map<desc::desc_id, desc::maya_magma_desc_node>::const_iterator iter = nodes.find( descID );
        if( iter == nodes.end() ) {
            throw maya_magma_exception( "maya_magma_mel_set_node_instance_info: no node of given ID" );
        }

        bool isSetX = false;
        bool isSetY = false;
        bool isSetName = false;
        bool isSetR = false;
        bool isSetG = false;
        bool isSetB = false;
        bool isSetA = false;
        int xpos, ypos;
        MString name;
        int r, g, b, a;

        if( MS::kSuccess == frantic::maya::parser_args::get_arg( args, "-x", "-xposition", xpos ) ) {
            isSetX = true;
        }
        if( MS::kSuccess == frantic::maya::parser_args::get_arg( args, "-y", "-yposition", ypos ) ) {
            isSetY = true;
        }
        if( MS::kSuccess == frantic::maya::parser_args::get_arg( args, "-n", "-name", name ) ) {
            isSetName = true;
        }
        if( MS::kSuccess == frantic::maya::parser_args::get_arg( args, "-r", "-red", r ) ) {
            isSetR = true;
        }
        if( MS::kSuccess == frantic::maya::parser_args::get_arg( args, "-g", "-green", g ) ) {
            isSetG = true;
        }
        if( MS::kSuccess == frantic::maya::parser_args::get_arg( args, "-b", "-blue", b ) ) {
            isSetB = true;
        }
        if( MS::kSuccess == frantic::maya::parser_args::get_arg( args, "-a", "-alpha", a ) ) {
            isSetA = true;
        }

        maya_MagmaFLUX* magma = maya_magma_mel_window::findMagmaFLUXWidgetByNode( depNode );
        if( isSetX || isSetY ) {
            int currX, currY;
            desc->get_node_position( descID, currX, currY );
            if( isSetX )
                currX = xpos;
            if( isSetY )
                currY = ypos;
            desc->set_node_position( descID, currX, currY );

            if( magma != NULL ) {
                magma->setNodePosition( descID, currX, currY );
            }
        }
        if( isSetName ) {
            frantic::tstring newName = frantic::maya::from_maya_t( name );
            desc->set_node_name( descID, newName );
            if( magma != NULL ) {
                magma->setNodeName( descID, frantic::strings::to_string( newName ).c_str() );
            }
        }
        if( isSetR || isSetG || isSetB || isSetA ) {
            if( magma != NULL ) {
                unsigned char rgba[4];

                int which = 0;

                bool issocket = false;
                int socket = 0;
                bool isOutput = false;

                if( frantic::maya::parser_args::get_arg( args, "-si", "-socketindex", socket ) &&
                    frantic::maya::parser_args::get_bool_arg( args, "-st", "-sockettype", isOutput ) )
                    issocket = true;

                if( frantic::maya::parser_args::get_bool_arg( args, "-h", "-header" ) )
                    which = 1;
                else if( frantic::maya::parser_args::get_bool_arg( args, "-o", "-outline" ) )
                    which = 2;
                else if( frantic::maya::parser_args::get_bool_arg( args, "-t", "-title" ) )
                    which = 3;
                else
                    which = 0;

                bool ok = false;
                switch( which ) {
                case 0: // Node color / Socket color
                    if( !issocket )
                        ok = magma->getNodeColor( descID, rgba );
                    else
                        ok = magma->getNodeSocketColor( descID, socket, isOutput, rgba );
                    break;
                case 1: // Node title header
                    if( !issocket )
                        ok = magma->getNodeTitleColor( descID, rgba );
                    else
                        ok = magma->getNodeSocketTextColor( descID, socket, isOutput, rgba );
                    break;
                case 2: // Node outline / Socket outline
                    if( !issocket )
                        ok = magma->getNodeOutlineColor( descID, rgba );
                    else
                        ok = magma->getNodeSocketOutlineColor( descID, socket, isOutput, rgba );
                    break;
                case 3: // Node title text / Socket text
                    if( !issocket )
                        ok = magma->getNodeTitleTextColor( descID, rgba );
                    else
                        ok = magma->getNodeSocketTextColor( descID, socket, isOutput, rgba );
                    break;
                }

                if( ok ) {
                    if( isSetR )
                        rgba[0] = ( r < 0 ? 0 : ( r > 255 ? 255 : (unsigned char)r ) );
                    if( isSetG )
                        rgba[1] = ( g < 0 ? 0 : ( g > 255 ? 255 : (unsigned char)g ) );
                    if( isSetB )
                        rgba[2] = ( b < 0 ? 0 : ( b > 255 ? 255 : (unsigned char)b ) );
                    if( isSetA )
                        rgba[3] = ( a < 0 ? 0 : ( a > 255 ? 255 : (unsigned char)a ) );
                } else {
                    which = -1;
                }

                switch( which ) {
                case 0: // Node color / Socket color
                    if( !issocket )
                        magma->setNodeColor( descID, rgba );
                    else
                        magma->setNodeSocketColor( descID, socket, isOutput, rgba );
                    break;
                case 1: // Node title header
                    if( !issocket )
                        magma->setNodeTitleColor( descID, rgba );
                    else
                        magma->setNodeSocketTextColor( descID, socket, isOutput, rgba );
                    break;
                case 2: // Node outline / Socket outline
                    if( !issocket )
                        magma->setNodeOutlineColor( descID, rgba );
                    else
                        magma->setNodeSocketOutlineColor( descID, socket, isOutput, rgba );
                    break;
                case 3: // Node title text / Socket text
                    if( !issocket )
                        magma->setNodeTitleTextColor( descID, rgba );
                    else
                        magma->setNodeSocketTextColor( descID, socket, isOutput, rgba );
                    break;
                }
            }
        }

        outStatus = MS::kSuccess;
    }
    return outStatus;
}

////////////////////////////////////////////////////////////////////////////////

const MString maya_magma_mel_retrieve_node_ids::commandName = "retrieve_magma_node_ids";

void* maya_magma_mel_retrieve_node_ids::creator() { return new maya_magma_mel_retrieve_node_ids; }

MStatus maya_magma_mel_retrieve_node_ids::parseArgs( const MArgList& args,
                                                     frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                                                     MFnDependencyNode& depNode ) {
    FF_LOG( debug ) << "maya_magma_mel_retrieve_node_ids:" << std::endl;

    const std::map<desc::desc_id, desc::maya_magma_desc_node>& nodes = desc->get_nodes();

    MIntArray results;
    for( std::map<desc::desc_id, desc::maya_magma_desc_node>::const_iterator iter = nodes.begin(); iter != nodes.end();
         ++iter ) {
        results.append( iter->first );
    }
    setResult( results );

    MStatus outStatus( MS::kSuccess );
    return outStatus;
}

////////////////////////////////////////////////////////////////////////////////

const MString maya_magma_mel_retrieve_node_info::commandName = "retrieve_magma_node_info";

void* maya_magma_mel_retrieve_node_info::creator() { return new maya_magma_mel_retrieve_node_info; }

MStatus maya_magma_mel_retrieve_node_info::doIt( const MArgList& args ) {
    FF_LOG( debug ) << "maya_magma_mel_retrieve_node_info:" << std::endl;
    MStatus outStatus( MS::kFailure );

    try {
        // -t nodeType
        // -a (on-print all infos; off
        // -p property (on, off)
        // -is inputsocket  (on, off)
        // -os outputSocket (on, off)
        // -i  index (either the index in inputSocket, or the index in outputSocket, or the index in property
        // for example
        // retrieve_magma_node_info -t Add -is on -i 0
        MString nodeType;
        if( MS::kSuccess == frantic::maya::parser_args::get_arg( args, "-t", "-type", nodeType ) ) {
            bool isReturnAll = frantic::maya::parser_args::get_bool_arg( args, "-a", "-all" );
            bool isReturnProperty = frantic::maya::parser_args::get_bool_arg( args, "-p", "-property" );
            bool isReturnInputSocket = frantic::maya::parser_args::get_bool_arg( args, "-is", "-inputSocket" );
            bool isReturnOutputSocket = frantic::maya::parser_args::get_bool_arg( args, "-os", "-outputSocket" );

            int index;
            bool isIndexSpecified = frantic::maya::parser_args::get_int_arg( args, "-i", "-index", index );

            int operation =
                int( isReturnProperty ) + int( isReturnInputSocket ) + int( isReturnOutputSocket ) + int( isReturnAll );
            if( operation > 1 ) {
                throw maya_magma_exception(
                    "maya_magma_mel_retrieve_node_info: more than one operation are specified at the same time" );
            } else if( operation == 0 ) {
                throw maya_magma_exception( "maya_magma_mel_retrieve_node_info: no operation has been specified" );
            } else if( operation == 1 ) {
                info::maya_magma_node_info magmaNodeInfo = factory::maya_magma_node_info_factory::create_node_infos(
                    frantic::strings::to_tstring( nodeType.asChar() ) );

                MStringArray result;
                if( isReturnAll ) {
                    magmaNodeInfo.add_to_list( result );
                } else if( isReturnProperty && isIndexSpecified ) {
                    magmaNodeInfo.add_properties_to_list( result, index );
                } else if( isReturnProperty && !isIndexSpecified ) {
                    magmaNodeInfo.add_properties_to_list( result );
                } else if( isReturnInputSocket && isIndexSpecified ) {
                    magmaNodeInfo.add_input_socket_to_list( result, index );
                } else if( isReturnInputSocket && !isIndexSpecified ) {
                    magmaNodeInfo.add_input_socket_to_list( result );
                } else if( isReturnOutputSocket && isIndexSpecified ) {
                    magmaNodeInfo.add_output_socket_to_list( result, index );
                } else if( isReturnOutputSocket && !isIndexSpecified ) {
                    magmaNodeInfo.add_output_socket_to_list( result );
                }

                setResult( result );
                outStatus = MS::kSuccess;
            }
        } else
            throw maya_magma_exception(
                "maya_magma_mel_retrieve_node_info: no node type information has been specified" );
    } catch( std::exception& e ) {
        FF_LOG( error ) << e.what() << std::endl;
        MGlobal::displayError( e.what() );
    } catch( ... ) {
        FF_LOG( stats ) << "YOU should never get here" << std::endl;
        assert( false );
    }
    return outStatus;
}

} // namespace mel
} // namespace maya
} // namespace magma
} // namespace frantic
