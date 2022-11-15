// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "stdafx.h"

#include "frantic/magma/maya/maya_magma_gui.hpp"
#include "frantic/magma/maya/maya_magma_mel_window.hpp"

#include "NodeView/connection.h"
#include "NodeView/node.h"
#include "NodeView/socket.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

#include <maya/MDataHandle.h>
#include <maya/MFloatVector.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnStringArrayData.h>
#include <maya/MQtUtil.h>

#include <sstream>

#include "frantic/magma/maya/maya_magma_desc_mpxdata.hpp"
#include "frantic/maya/parser_args.hpp"
#include "frantic/maya/selection.hpp"
#include <frantic/logging/logging_level.hpp>
#include <frantic/maya/convert.hpp>
#include <frantic/maya/maya_util.hpp>

namespace frantic {
namespace magma {
namespace maya {
namespace mel {

const MString maya_magma_mel_window::commandName = "add_magma_flux_window";

std::map<frantic::tstring, maya_MagmaFLUX*> maya_magma_mel_window::m_WindowNameToMagmaFLUX;
std::map<frantic::tstring, maya_MagmaFLUX*> maya_magma_mel_window::m_NodeNameToMagmaFLUX;

void* maya_magma_mel_window::creator() { return new maya_magma_mel_window; }

#pragma region Window Finding and Window Update Events
maya_MagmaFLUX* maya_magma_mel_window::findMagmaFLUXWidgetByWindowName( const MString& windowName ) {
    frantic::tstring name = frantic::maya::from_maya_t( windowName );
    if( m_WindowNameToMagmaFLUX.count( name ) > 0 ) {
        return m_WindowNameToMagmaFLUX[name];
    }

    return NULL;
}

maya_MagmaFLUX* maya_magma_mel_window::findMagmaFLUXWidgetByNode( const MFnDependencyNode& node ) {
    MString name = frantic::maya::maya_util::get_node_full_name( node );
    return findMagmaFLUXWidgetByNodeName( name );
}

maya_MagmaFLUX* maya_magma_mel_window::findMagmaFLUXWidgetByNodeName( const MString& nodeName ) {
    frantic::tstring name = frantic::maya::from_maya_t( nodeName );
    return findMagmaFLUXWidgetByNodeName( name );
}

maya_MagmaFLUX* maya_magma_mel_window::findMagmaFLUXWidgetByNodeName( const frantic::tstring& nodeName ) {
    if( m_NodeNameToMagmaFLUX.count( nodeName ) > 0 ) {
        return m_NodeNameToMagmaFLUX[nodeName];
    }

    return NULL;
}

std::vector<maya_MagmaFLUX*> maya_magma_mel_window::findAllMagmaFLUXWidgets() {
    std::vector<maya_MagmaFLUX*> result;
    for( std::map<frantic::tstring, maya_MagmaFLUX*>::const_iterator iter = m_WindowNameToMagmaFLUX.begin();
         iter != m_WindowNameToMagmaFLUX.end(); ++iter ) {
        result.push_back( iter->second );
    }
    return result;
}

void maya_magma_mel_window::onAttributeChangedUpdateNodesInWindow( MPlug& plug ) {
    MObject prtNode = plug.node();
    MFnDependencyNode depNode( prtNode );

    maya_MagmaFLUX* magma = maya_magma_mel_window::findMagmaFLUXWidgetByNode( depNode );
    if( magma != NULL ) {
        magma->onAttributeChangedUpdateNodesInWindow( plug );
    }
}

void maya_magma_mel_window::addErrorNode( const MObject& prtNode, int id ) {
    MFnDependencyNode depNode( prtNode );
    maya_MagmaFLUX* magma = maya_magma_mel_window::findMagmaFLUXWidgetByNode( depNode );
    if( magma != NULL ) {
        magma->setNodeError( id );
    }
}

void maya_magma_mel_window::clearErrorNodes( const MObject& prtNode ) {
    MFnDependencyNode depNode( prtNode );
    maya_MagmaFLUX* magma = maya_magma_mel_window::findMagmaFLUXWidgetByNode( depNode );
    if( magma != NULL ) {
        magma->clearNodeErrors();
    }
}

void maya_magma_mel_window::removeMagmaFLUX( maya_MagmaFLUX* window ) {
    frantic::tstring winName = frantic::maya::from_maya_t( window->getWindowName() );
    m_WindowNameToMagmaFLUX.erase( winName );

    frantic::tstring nodeName = frantic::maya::from_maya_t( window->getMayaNodeName() );
    m_NodeNameToMagmaFLUX.erase( nodeName );
}

void maya_magma_mel_window::addMagmaFLUX( maya_MagmaFLUX* window ) {
    frantic::tstring winName = frantic::maya::from_maya_t( window->getWindowName() );
    m_WindowNameToMagmaFLUX[winName] = window;

    frantic::tstring nodeName = frantic::maya::from_maya_t( window->getMayaNodeName() );
    m_NodeNameToMagmaFLUX[nodeName] = window;
}
#pragma endregion

MStatus maya_magma_mel_window::parseArgs( const MArgList& args, frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                                          MFnDependencyNode& depNode ) {
    QWidget* windowWidget = NULL;

    MString windowName;
    MString layoutName;

    MStatus hasWindowName;
    MStatus hasLayoutName;
    hasWindowName = frantic::maya::parser_args::get_arg( args, "-wn", "-windowName", windowName );
    hasLayoutName = frantic::maya::parser_args::get_arg( args, "-ln", "-layoutName", layoutName );

    // MString nodeName = frantic::maya::selection::get_current_selected_object_names()[0];
    MString nodeName = frantic::maya::maya_util::get_node_full_name( depNode );

    if( findMagmaFLUXWidgetByWindowName( windowName ) != NULL ) {
        throw maya_magma_exception(
            "maya_magma_mel_window::parseArgs specified window already contains a magma flux gui" );
    } else if( findMagmaFLUXWidgetByNode( depNode ) != NULL ) {
        throw maya_magma_exception(
            "maya_magma_mel_window::parseArgs specified node already associated with a magma flux gui" );
    }

    if( hasWindowName == MS::kSuccess && hasLayoutName == MS::kSuccess ) {
        windowWidget = MQtUtil::findLayout( layoutName, MQtUtil::findWindow( windowName ) );
    } else if( hasWindowName == MS::kSuccess && hasLayoutName != MS::kSuccess ) {
        windowWidget = MQtUtil::findWindow( windowName );
        //} else if ( hasWindowName != MS::kSuccess && hasLayoutName == MS::kSuccess ) {
        //	windowWidget = MQtUtil::findLayout( windowName );
    } else {
        throw maya_magma_exception( "maya_magma_mel_window::parseArgs no window name was specified" );
    }

    MStatus outStatus( MS::kFailure );
    if( !windowWidget ) {
        FF_LOG( error ) << "-b layoutName = NULL: " << windowName.asChar() << " not found" << std::endl;
        return outStatus;
    }
    FF_LOG( debug ) << "Create window for node: " << nodeName.asChar() << std::endl;

    // Set up the GUI
    QLayout* layout = windowWidget->layout();
    if( layout == NULL ) {
        // TODO: throw an exception instead?
        layout = new QGridLayout;
        windowWidget->setLayout( layout );
    }

    maya_MagmaFLUX* magma = new maya_MagmaFLUX( windowName, nodeName, desc, depNode );
    layout->addWidget( magma );

    QSizePolicy sizePolicy;
    sizePolicy.setHorizontalPolicy( QSizePolicy::Expanding );
    sizePolicy.setVerticalPolicy( QSizePolicy::Expanding );
    magma->setSizePolicy( sizePolicy );

    addMagmaFLUX( magma );

    outStatus = MS::kSuccess;
    return outStatus;
}

////////////////////////////////////////////////////////////////////////////////

const MString maya_magma_mel_window_retrieve::commandName = "magma_flux_window_query";

void* maya_magma_mel_window_retrieve::creator() { return new maya_magma_mel_window_retrieve; }

MStatus maya_magma_mel_window_retrieve::parseArgs( const MArgList& args,
                                                   frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                                                   MFnDependencyNode& depNode ) {
    maya_MagmaFLUX* windowWidget;

    MString windowName;
    bool getNodes;
    bool getEdges;
    bool getLastXY;
    MStatus hasWindowName;

    int argCount = 0;
    hasWindowName = frantic::maya::parser_args::get_arg( args, "-i", "-id", windowName );
    getNodes = frantic::maya::parser_args::get_bool_arg( args, "-n", "-nodes" );
    getEdges = frantic::maya::parser_args::get_bool_arg( args, "-e", "-edges" );
    getLastXY = frantic::maya::parser_args::get_bool_arg( args, "-lxy", "-lastXY" );

    if( hasWindowName != MS::kSuccess ) {
        windowWidget = maya_magma_mel_window::findMagmaFLUXWidgetByNode( depNode );
    } else {
        windowWidget = maya_magma_mel_window::findMagmaFLUXWidgetByWindowName( windowName );
    }

    if( windowWidget == NULL ) {
        throw maya_magma_exception( "maya_magma_mel_window_retrieve::parseArgs window not found" );
    }

    if( getNodes )
        argCount++;
    if( getEdges )
        argCount++;
    if( getLastXY )
        argCount++;

    if( argCount > 1 ) {
        throw maya_magma_exception( "maya_magma_mel_window_retrieve::parseArgs more than one option specified, please "
                                    "specify just one option" );
    } else if( argCount <= 0 ) {
        throw maya_magma_exception( "maya_magma_mel_window_retrieve::parseArgs no option specified" );
    }

    if( getNodes ) {
        std::vector<int> nodes = windowWidget->getSelectedNodes();
        MIntArray result;
        for( std::vector<int>::const_iterator iter = nodes.begin(); iter != nodes.end(); ++iter ) {
            result.append( *iter );
        }
        setResult( result );

    } else if( getEdges ) {
        std::vector<maya_MagmaFLUX::connection_data> edges = windowWidget->getSelectedEdges();
        MStringArray result;
        for( std::vector<maya_MagmaFLUX::connection_data>::const_iterator iter = edges.begin(); iter != edges.end();
             ++iter ) {
            result.append( iter->toStdString().c_str() );
        }
        setResult( result );

    } else if( getLastXY ) {
        MIntArray result( 2 );
        int x, y;
        windowWidget->getLastNodePosition( x, y );
        result[0] = x;
        result[1] = y;
        setResult( result );
    }

    MStatus outStatus( MS::kSuccess );
    return outStatus;
}

////////////////////////////////////////////////////////////////////////////////

const MString maya_magma_mel_window_set::commandName = "magma_flux_window_set";

void* maya_magma_mel_window_set::creator() { return new maya_magma_mel_window_set; }

MStatus maya_magma_mel_window_set::parseArgs( const MArgList& args,
                                              frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                                              MFnDependencyNode& depNode ) {
    maya_MagmaFLUX* windowWidget;

    MString windowName;
    MIntArray nodeIDs;
    MStringArray edgeData;
    MStatus hasWindowName;

    hasWindowName = frantic::maya::parser_args::get_arg( args, "-i", "-id", windowName );

    if( hasWindowName != MS::kSuccess ) {
        windowWidget = maya_magma_mel_window::findMagmaFLUXWidgetByNode( depNode );
    } else {
        windowWidget = maya_magma_mel_window::findMagmaFLUXWidgetByWindowName( windowName );
    }

    if( windowWidget == NULL ) {
        throw maya_magma_exception( "magma_flux_window_set::parseArgs window not found" );
    }

    {
        // Node / Edges Selection
        bool selected = false;
        std::vector<int> nodes;
        std::vector<maya_MagmaFLUX::connection_data> edges;
        if( frantic::maya::parser_args::get_int_array_arg( args, "-n", "-nodes", nodeIDs ) ) {
            for( unsigned int i = 0; i < nodeIDs.length(); i++ ) {
                nodes.push_back( nodeIDs[i] );
            }
            selected = true;
        }
        if( frantic::maya::parser_args::get_string_array_arg( args, "-e", "-edges", edgeData ) ) {
            for( unsigned int i = 0; i < edgeData.length(); i++ ) {
                frantic::tstring data = frantic::maya::from_maya_t( edgeData[i] );
                edges.push_back( maya_MagmaFLUX::connection_data( data ) );
            }
            selected = true;
        }

        if( selected ) {
            windowWidget->setSelection( nodes, edges );
        }
    }

    {
        // Last Node position
        int lastX, lastY;
        bool hasLastX, hasLastY;
        hasLastX = frantic::maya::parser_args::get_int_arg( args, "-lx", "-lastX", lastX );
        hasLastY = frantic::maya::parser_args::get_int_arg( args, "-ly", "-lastY", lastY );
        if( hasLastX || hasLastY ) {
            int lx, ly;
            windowWidget->getLastNodePosition( lx, ly );
            if( hasLastX )
                lx = lastX;
            if( hasLastY )
                ly = lastY;
            windowWidget->setLastNodePosition( lx, ly );
        }
    }

    {
        // Window background color
        int r, g, b;
        bool hasR, hasG, hasB;
        hasR = frantic::maya::parser_args::get_int_arg( args, "-bgr", "-backgroundred", r );
        hasG = frantic::maya::parser_args::get_int_arg( args, "-bgg", "-backgroundgreen", g );
        hasB = frantic::maya::parser_args::get_int_arg( args, "-bgb", "-backgroundblue", b );

        if( hasR || hasG || hasB ) {
            QColor color = windowWidget->backgroundColor();
            if( hasR )
                color.setRed( r );
            if( hasG )
                color.setGreen( g );
            if( hasB )
                color.setBlue( b );
            windowWidget->setBackgroundColor( color );
        }
    }

    {
        // Grid color
        int r, g, b;
        bool hasR, hasG, hasB;
        hasR = frantic::maya::parser_args::get_int_arg( args, "-gr", "-gridred", r );
        hasG = frantic::maya::parser_args::get_int_arg( args, "-gg", "-gridgreen", g );
        hasB = frantic::maya::parser_args::get_int_arg( args, "-gb", "-gridblue", b );

        if( hasR || hasG || hasB ) {
            QColor color = windowWidget->gridColor();
            if( hasR )
                color.setRed( r );
            if( hasG )
                color.setGreen( g );
            if( hasB )
                color.setBlue( b );
            windowWidget->setGridColor( color );
        }
    }

    {
        // Grid size
        int size;
        bool hasarg = frantic::maya::parser_args::get_int_arg( args, "-gs", "-gridsize", size );
        if( hasarg ) {
            windowWidget->setGridSize( size );
        }
    }

    {
        // Grid on/off
        bool on;
        bool hasarg = frantic::maya::parser_args::get_bool_arg( args, "-g", "-grid", on );
        if( hasarg ) {
            windowWidget->setGridLines( on );
        }
    }

    {
        // Grid snap
        bool on;
        bool hasarg = frantic::maya::parser_args::get_bool_arg( args, "-s", "-snaptogrid", on );
        if( hasarg ) {
            windowWidget->setSnapToGrid( on );
        }
    }

    {
        // Pan and Zoom
        double cx, cy;
        bool hasCx = frantic::maya::parser_args::get_float_arg( args, "-cx", "-centerx", cx );
        bool hasCy = frantic::maya::parser_args::get_float_arg( args, "-cy", "-centery", cy );
        if( hasCx || hasCy ) {
            QPointF currentCenter =
                windowWidget->mapToScene( windowWidget->viewport()->rect() ).boundingRect().center();
            if( hasCx )
                currentCenter.setX( cx );
            if( hasCy )
                currentCenter.setY( cy );
            windowWidget->centerOn( currentCenter );
        }

        double zoom;
        bool hasZoom = frantic::maya::parser_args::get_float_arg( args, "-z", "-zoom", zoom );
        if( hasZoom ) {
            windowWidget->setZoom( zoom );
        }
    }

    {
        // Selection mode
        int mode;
        bool hasSelectionMode = frantic::maya::parser_args::get_int_arg( args, "-sm", "-selectmode", mode );
        if( hasSelectionMode ) {
            if( mode < 0 || mode >= 0x3 ) {
                // Invalid mode ignored
            } else {
                windowWidget->setRubberBandSelectionMode( static_cast<Qt::ItemSelectionMode>( mode ) );
            }
        }
    }

    MStatus outStatus( MS::kSuccess );
    return outStatus;
}

////////////////////////////////////////////////////////////////////////////////

const MString maya_magma_mel_window_menu::commandName = "magma_flux_window_menu";

void* maya_magma_mel_window_menu::creator() { return new maya_magma_mel_window_menu; }

MStatus maya_magma_mel_window_menu::parseArgs( const MArgList& args,
                                               frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                                               MFnDependencyNode& depNode ) {
    maya_MagmaFLUX* windowWidget;

    MString windowName;
    MStatus hasWindowName;
    hasWindowName = frantic::maya::parser_args::get_arg( args, "-i", "-id", windowName );

    if( hasWindowName != MS::kSuccess ) {
        windowWidget = maya_magma_mel_window::findMagmaFLUXWidgetByNode( depNode );
    } else {
        windowWidget = maya_magma_mel_window::findMagmaFLUXWidgetByWindowName( windowName );
    }

    if( windowWidget == NULL ) {
        throw maya_magma_exception( "magma_flux_window_menu::parseArgs window not found" );
    }

    bool clear;
    bool hasMenuPath;
    MStringArray menuPath;
    bool hasMenuCommand;
    MString menuCommand;
    bool hasShowCommand;
    MIntArray showCommandPosition;
    clear = frantic::maya::parser_args::get_bool_arg( args, "-c", "-clear" );

    hasMenuPath = frantic::maya::parser_args::get_string_array_arg( args, "-m", "-menu", menuPath );
    hasMenuCommand = frantic::maya::parser_args::get_arg( args, "-a", "-action", menuCommand );
    hasShowCommand = frantic::maya::parser_args::get_int_array_arg( args, "-s", "-show", showCommandPosition );

    if( hasMenuPath != hasMenuCommand ) {
        throw maya_magma_exception(
            "magma_flux_window_menu::parseArgs menu path and action command must both be specified" );
    }
    if( hasShowCommand && showCommandPosition.length() < 2 ) {
        throw maya_magma_exception( "magma_flux_window_menu::parseArgs show command argument must be of size 2" );
    }

    if( clear ) {
        windowWidget->clearRightClickMenu();
    }

    if( hasMenuPath && hasMenuCommand ) {
        windowWidget->addRightClickMenu( frantic::maya::from_maya_t( menuPath ),
                                         frantic::maya::from_maya_t( menuCommand ) );
    }

    if( hasShowCommand ) {
        MString result = windowWidget->showRightClickMenu( showCommandPosition[0], showCommandPosition[1] );
        setResult( result );
    }

    MStatus outStatus( MS::kSuccess );
    return outStatus;
}

} // namespace mel
} // namespace maya
} // namespace magma
} // namespace frantic
