// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "stdafx.h"

#include <boost/regex.hpp>

#include <frantic/sstream/tsstream.hpp>

#include "frantic/magma/maya/maya_magma_gui.hpp"

#include "frantic/magma/maya/maya_magma_attr_manager.hpp"
#include "frantic/magma/maya/maya_magma_info.hpp"
#include "frantic/magma/maya/maya_magma_info_factory.hpp"
#include "frantic/magma/maya/maya_magma_mel_window.hpp"
#include "frantic/maya/selection.hpp"

#include "NodeView/connection.h"
#include "NodeView/node.h"
#include "NodeView/socket.h"
#include <sstream>

#include <maya/MDataHandle.h>
#include <maya/MFloatVector.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnStringArrayData.h>
#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include "frantic/magma/maya/maya_magma_desc_mpxdata.hpp"
#include <frantic/maya/convert.hpp>

#include <QtGui>

#if MAYA_API_VERSION >= 201650
#include <QMenu>
#endif

namespace frantic {
namespace magma {
namespace maya {

static int m_parentNode = -1;

///////////////////////////////////////////////////////////////////////
#pragma region connection_data
maya_MagmaFLUX::connection_data::connection_data()
    : m_source( -1 )
    , m_source_index( -1 )
    , m_destination( -1 )
    , m_destination_index( -1 ) {}

maya_MagmaFLUX::connection_data::connection_data( int src, int srcSocket, int dst, int dstSocket )
    : m_source( src )
    , m_source_index( srcSocket )
    , m_destination( dst )
    , m_destination_index( dstSocket ) {}

maya_MagmaFLUX::connection_data::connection_data( frantic::tstring fromString ) {
    boost::basic_regex<frantic::tstring::value_type> propertyparser(
        _T( "\\s*(\\S+?)\\s*,\\s*(\\S+?)\\s*,\\s*(\\S+?)\\s*,\\s*(\\S+?)\\s*" ) );
    boost::match_results<frantic::tstring::const_iterator> what;
    if( boost::regex_search( fromString, what, propertyparser, boost::match_default ) ) {
        m_source = boost::lexical_cast<int>( frantic::tstring( what[1].first, what[1].second ) );
        m_source_index = boost::lexical_cast<int>( frantic::tstring( what[2].first, what[2].second ) );
        m_destination = boost::lexical_cast<int>( frantic::tstring( what[3].first, what[3].second ) );
        m_destination_index = boost::lexical_cast<int>( frantic::tstring( what[4].first, what[4].second ) );
    } else {
        m_source = -1;
        m_source_index = -1;
        m_destination = -1;
        m_destination_index = -1;
    }
}

bool maya_MagmaFLUX::connection_data::operator==( const connection_data& rhs ) const {
    return ( m_source == rhs.m_source && m_source_index == rhs.m_source_index && m_destination == rhs.m_destination &&
             m_destination_index == rhs.m_destination_index );
}

bool maya_MagmaFLUX::connection_data::operator!=( const connection_data& rhs ) const { return !( *this == rhs ); }

bool maya_MagmaFLUX::connection_data::operator>( const connection_data& rhs ) const {
    if( m_source > rhs.m_source )
        return true;
    if( m_source < rhs.m_source )
        return false;

    if( m_source_index > rhs.m_source_index )
        return true;
    if( m_source_index < rhs.m_source_index )
        return false;

    if( m_destination > rhs.m_destination )
        return true;
    if( m_destination < rhs.m_destination )
        return false;

    if( m_destination_index > rhs.m_destination_index )
        return true;
    if( m_destination_index < rhs.m_destination_index )
        return false;

    return false;
}

bool maya_MagmaFLUX::connection_data::operator<=( const connection_data& rhs ) const { return !( *this > rhs ); }

bool maya_MagmaFLUX::connection_data::operator<( const connection_data& rhs ) const {
    // if( m_source > rhs.m_source ) return false;
    // if( m_source < rhs.m_source ) return true;

    // if( m_source_index > rhs.m_source_index ) return false;
    // if( m_source_index < rhs.m_source_index ) return true;
    //
    // if( m_destination > rhs.m_destination ) return false;
    // if( m_destination < rhs.m_destination ) return true;

    // if( m_destination_index > rhs.m_destination_index ) return false;
    // if( m_destination_index < rhs.m_destination_index ) return true;

    return ( *this <= rhs ) && ( *this != rhs );
}

bool maya_MagmaFLUX::connection_data::operator>=( const connection_data& rhs ) const { return !( *this < rhs ); }

std::string maya_MagmaFLUX::connection_data::toStdString( char separator ) const {
    frantic::tsstream::tostringstream stream;
    stream << m_source << separator << m_source_index << separator << m_destination << separator << m_destination_index;
    return frantic::strings::to_string( stream.str() );
}
#pragma endregion
///////////////////////////////////////////////////////////////////////
#pragma region node_meta_data
maya_MagmaFLUX::node_meta_data::node_meta_data()
    : m_type()
    , m_base_output_socket_count( 0 )
    , m_base_input_socket_count( 0 ) {
    m_rgba[0] = 200;
    m_rgba[1] = 255;
    m_rgba[2] = 200;
    m_rgba[3] = 255;

    m_title_rgba[0] = 200;
    m_title_rgba[1] = 255;
    m_title_rgba[2] = 200;
    m_title_rgba[3] = 255;

    m_outline_rgba[0] = 0;
    m_outline_rgba[1] = 0;
    m_outline_rgba[2] = 0;
    m_outline_rgba[3] = 255;

    m_title_text_rgba[0] = 0;
    m_title_text_rgba[1] = 0;
    m_title_text_rgba[2] = 0;
    m_title_text_rgba[3] = 255;
}

maya_MagmaFLUX::node_meta_data::node_meta_data( Node* node, const frantic::tstring& type, int inputCount,
                                                int outputCount )
    : m_type( type )
    , m_base_output_socket_count( outputCount )
    , m_base_input_socket_count( inputCount ) {
    QColor baseColor = node->backgroundBrush().color();
    QColor titleColor = node->titleBarBrush().color();
    QColor outlineColor = node->outlinePen().color();
    QColor titleTextColor = node->titleTextBrush().color();

    setNodeColor( baseColor );
    setTitleColor( titleColor );
    setOutlineColor( outlineColor );
    setTitleTextColor( titleTextColor );
}

void maya_MagmaFLUX::node_meta_data::setNodeColor( const QColor& color ) {
    m_rgba[0] = color.red();
    m_rgba[1] = color.green();
    m_rgba[2] = color.blue();
    m_rgba[3] = color.alpha();
}

QColor maya_MagmaFLUX::node_meta_data::getNodeColor() const {
    return QColor( m_rgba[0], m_rgba[1], m_rgba[2], m_rgba[3] );
}

void maya_MagmaFLUX::node_meta_data::setTitleColor( const QColor& color ) {
    m_title_rgba[0] = color.red();
    m_title_rgba[1] = color.green();
    m_title_rgba[2] = color.blue();
    m_title_rgba[3] = color.alpha();
}

QColor maya_MagmaFLUX::node_meta_data::getTitleColor() const {
    return QColor( m_title_rgba[0], m_title_rgba[1], m_title_rgba[2], m_title_rgba[3] );
}

void maya_MagmaFLUX::node_meta_data::setOutlineColor( const QColor& color ) {
    m_outline_rgba[0] = color.red();
    m_outline_rgba[1] = color.green();
    m_outline_rgba[2] = color.blue();
    m_outline_rgba[3] = color.alpha();
}

QColor maya_MagmaFLUX::node_meta_data::getOutlineColor() const {
    return QColor( m_outline_rgba[0], m_outline_rgba[1], m_outline_rgba[2], m_outline_rgba[3] );
}

void maya_MagmaFLUX::node_meta_data::setTitleTextColor( const QColor& color ) {
    m_title_text_rgba[0] = color.red();
    m_title_text_rgba[1] = color.green();
    m_title_text_rgba[2] = color.blue();
    m_title_text_rgba[3] = color.alpha();
}

QColor maya_MagmaFLUX::node_meta_data::getTitleTextColor() const {
    return QColor( m_title_text_rgba[0], m_title_text_rgba[1], m_title_text_rgba[2], m_title_text_rgba[3] );
}
#pragma endregion
///////////////////////////////////////////////////////////////////////
#pragma region MagmaFLUXQAction
maya_MagmaFLUX::MagmaFLUXQAction::MagmaFLUXQAction( QString& text, QObject* parent,
                                                    const std::vector<frantic::tstring>& path,
                                                    const frantic::tstring& command )
    : QAction( text, parent )
    , m_path( path )
    , m_command( command ) {}

maya_MagmaFLUX::MagmaFLUXQAction::~MagmaFLUXQAction() {}

bool maya_MagmaFLUX::MagmaFLUXQAction::runCommand( const QPoint& position, const MString& nodeName,
                                                   MString& outResult ) {
    frantic::tsstream::tostringstream cmd;
    // function ( x, y, path, prtnode )
    cmd << m_command << " " << position.x() << " " << position.y();

    cmd << " "
        << "{";
    bool first = true;
    for( std::vector<frantic::tstring>::const_iterator iter = m_path.begin(); iter != m_path.end(); ++iter ) {
        cmd << ( first ? "" : "," ) << "\"" << *iter << "\"";
        first = false;
    }
    cmd << "}";

    cmd << " "
        << "\"" << frantic::strings::to_tstring( nodeName.asChar() ) << "\""
        << ";";

    MStatus stat;
    stat = MGlobal::executeCommand( cmd.str().c_str(), outResult );

    return ( stat == MS::kSuccess );
}

#pragma endregion
///////////////////////////////////////////////////////////////////////
#pragma region MenuLayout
maya_MagmaFLUX::MenuLayout::MenuLayout( const frantic::tstring& name, QWidget* parent )
    : m_name( name )
    , m_command( _T( "" ) )
    , m_submenu_items()
    , m_menuHasChanged( true )
    , m_qmenu( NULL )
    , m_parent( parent ) {}
maya_MagmaFLUX::MenuLayout::~MenuLayout() {}

QMenu* maya_MagmaFLUX::MenuLayout::getQMenu() {
    std::vector<frantic::tstring> path;
    return createMenuDetail( path );
}

QMenu* maya_MagmaFLUX::MenuLayout::createMenuDetail( std::vector<frantic::tstring>& path ) {
    if( m_menuHasChanged || m_qmenu == NULL ) {
        m_menuHasChanged = false;
        if( m_qmenu != NULL ) {
            delete m_qmenu;
            m_qmenu = NULL;
        }

        QString name( frantic::strings::to_string( m_name ).c_str() );
        m_qmenu = new QMenu( name, m_parent );
        for( std::vector<MenuLayout_ptr>::const_iterator iter = m_submenu_items.begin(); iter != m_submenu_items.end();
             ++iter ) {
            path.push_back( ( *iter )->m_name );

            if( ( *iter )->m_command.length() != 0 ) {
                QString cmdName( frantic::strings::to_string( ( *iter )->m_name ).c_str() );
                MagmaFLUXQAction* action = new MagmaFLUXQAction( cmdName, m_parent, path, ( *iter )->m_command );
                m_qmenu->addAction( action );
            }

            if( ( *iter )->m_submenu_items.size() != 0 ) {
                QMenu* subMenu = ( *iter )->createMenuDetail( path );
                m_qmenu->addMenu( subMenu );
            }

            path.pop_back();
        }
    }
    return m_qmenu;
}

bool maya_MagmaFLUX::MenuLayout::insert( const std::vector<frantic::tstring>& path, const frantic::tstring& command,
                                         int index ) {
    if( index < path.size() ) {
        MenuLayout_ptr menu = getSubmenu( path[index], true );

        if( index + 1 < path.size() ) {
            bool changed = menu->insert( path, command, index + 1 );
            if( changed )
                m_menuHasChanged = true;
            return changed;
        }

        menu->m_command = command;
        m_menuHasChanged = true;
        return true;
    }

    return false;
}

maya_MagmaFLUX::MenuLayout::MenuLayout_ptr maya_MagmaFLUX::MenuLayout::getSubmenu( const frantic::tstring& item,
                                                                                   bool autocreate ) {
    for( std::vector<MenuLayout_ptr>::const_iterator iter = m_submenu_items.begin(); iter != m_submenu_items.end();
         ++iter ) {
        if( ( *iter )->m_name == item )
            return ( *iter );
    }

    MenuLayout_ptr result;
    if( autocreate ) {
        result.reset( new MenuLayout( item ) );
        m_submenu_items.push_back( result );
        m_menuHasChanged = true;
    }
    return result;
}

void maya_MagmaFLUX::MenuLayout::clear() {
    for( std::vector<MenuLayout_ptr>::const_iterator iter = m_submenu_items.begin(); iter != m_submenu_items.end();
         ++iter ) {
        ( *iter )->clear();
    }
    m_submenu_items.clear();
    m_menuHasChanged = true;
}

#pragma endregion
///////////////////////////////////////////////////////////////////////
maya_MagmaFLUX::maya_MagmaFLUX( MString& windowName, MString& nodeName,
                                frantic::magma::maya::desc::maya_magma_desc_ptr desc, MFnDependencyNode& mayaNode,
                                QWidget* parent )
    : NodeView( parent )
    , m_EnableForwardMelCommand( false )
    , m_WindowName( windowName )
    , m_MayaNodeName( nodeName )
    , m_MayaObject( mayaNode.object() )
    , m_melCommandOnNodeSelectionChanged( _T( "MagmaFLUX_nodeSelectionChanged" ) )
    , m_melCommandOnConnectionSelectionChanged( _T( "MagmaFLUX_connectionSelectionChanged" ) )
    , m_melCommandOnConnectionCreated( _T( "MagmaFLUX_connectionCreated" ) )
    , m_melCommandOnConnectionDeleted( _T( "MagmaFLUX_connectionDeleted" ) )
    , m_melCommandOnKeyboardPressed( _T( "MagmaFLUX_keyPressed" ) )
    , m_melCommandOnConnectionToEmpty( _T( "MagmaFLUX_connectToEmpty" ) )
    , m_description( desc )
    , m_lastNodeX( 0 )
    , m_lastNodeY( 0 )
    , m_lastMousePressX( 0 )
    , m_lastMousePressY( 0 )
    , m_rightClickMenu( _T( "" ), this ) {
    QObject::connect( this, SIGNAL( connectionCreated( Socket* ) ), this, SLOT( onConnectionCreated( Socket* ) ) );
    QObject::connect( this, SIGNAL( connectToEmpty( Socket*, QPointF ) ), this,
                      SLOT( onConnectedToEmpty( Socket*, QPointF ) ) );
    QObject::connect( this, SIGNAL( nodeAdded( Node* ) ), this, SLOT( onNodeAdded( Node* ) ) );
    QObject::connect( this, SIGNAL( selectionChanged() ), this, SLOT( onSelectionChanged() ) );
    QObject::connect( this, SIGNAL( mousePress( QMouseEvent* ) ), this, SLOT( onMousePress( QMouseEvent* ) ) );
    QObject::connect( this, SIGNAL( mouseRelease( QMouseEvent* ) ), this, SLOT( onMouseRelease( QMouseEvent* ) ) );

    if( desc != NULL ) {
        // Set up the nodes and connections
        const std::map<frantic::magma::maya::desc::desc_id, frantic::magma::maya::desc::maya_magma_desc_node>& nodes =
            m_description->get_nodes();

        for( std::map<frantic::magma::maya::desc::desc_id,
                      frantic::magma::maya::desc::maya_magma_desc_node>::const_iterator iter = nodes.begin();
             iter != nodes.end(); ++iter ) {
            // Initialize node
            Node* magmaNode = addNode( iter->first, desc, mayaNode );
        }

        const std::vector<frantic::magma::maya::desc::maya_magma_desc_connection>& edges =
            m_description->get_connections();
        for( std::vector<frantic::magma::maya::desc::maya_magma_desc_connection>::const_iterator iter = edges.begin();
             iter != edges.end(); ++iter ) {
            addConnection( iter->get_src_desc_node_id(), iter->get_src_socket_index(), iter->get_dst_desc_node_id(),
                           iter->get_dst_socket_index() );
        }
    }

    setContextMenuPolicy( Qt::CustomContextMenu );
    QObject::connect( this, SIGNAL( customContextMenuRequested( const QPoint& ) ), this,
                      SLOT( onRightClickShowMenu( const QPoint& ) ) );

    m_EnableForwardMelCommand = true;
}

maya_MagmaFLUX::~maya_MagmaFLUX() { frantic::magma::maya::mel::maya_magma_mel_window::removeMagmaFLUX( this ); }

void maya_MagmaFLUX::onConnectionCreated( Socket* sock ) {
    foreach( Connection* conn, sock->connections() ) {
        Socket* srcSocket = conn->sourceSocket();
        Socket* dstSocket = conn->destinationSocket();
        if( srcSocket == NULL || dstSocket == NULL )
            continue;

        // Make sure the socket is in a node
        Node* checkSrc = dynamic_cast<Node*>( srcSocket->parentItem() );
        Node* checkDst = dynamic_cast<Node*>( dstSocket->parentItem() );
        if( checkSrc == NULL || checkDst == NULL )
            continue;

        // Make sure the node is already there
        // TODO: What happens if they aren't?  Is it possible?
        if( this->m_NodeToID.count( checkSrc ) <= 0 || m_NodeToID.count( checkDst ) <= 0 )
            continue;
        int srcID = m_NodeToID[checkSrc];
        int dstID = m_NodeToID[checkDst];

        // Make sure the appropriate socket is there
        int srcSocketIndex = 0;
        int dstSocketIndex = 0;
        for( ; srcSocketIndex < checkSrc->getOutputSocketCount(); srcSocketIndex++ ) {
            if( checkSrc->getOutputSocket( srcSocketIndex ) == srcSocket )
                break;
        }
        for( ; dstSocketIndex < checkDst->getInputSocketCount(); dstSocketIndex++ ) {
            if( checkDst->getInputSocket( dstSocketIndex ) == dstSocket )
                break;
        }
        if( srcSocketIndex >= checkSrc->getOutputSocketCount() || dstSocketIndex >= checkDst->getInputSocketCount() )
            continue;

        // Add the delete event handler
        QObject::connect( conn, SIGNAL( destroyed( QObject* ) ), this, SLOT( onConnectionToBeDestroyed( QObject* ) ) );
        conn->setCurvature( 0.5f );

        // Check if this edge has already been added
        connection_data data( srcID, srcSocketIndex, dstID, dstSocketIndex );
        if( m_ConnectionDataToConnection.count( data ) > 0 )
            continue;

        // Add the connection
        m_ConnectionDataToConnection[data] = conn; // Reserve a spot
        bool ok = true;
        try {
            this->m_description->create_connection( data.m_source, data.m_source_index, data.m_destination,
                                                    data.m_destination_index );
        } catch( maya_magma_exception& ) {
            ok = false;
        }

        if( ok ) {
            m_ConnectionToConnectionData[conn] = data;

            if( m_EnableForwardMelCommand ) {
                // Create connection event
                frantic::tsstream::tostringstream createCmd;
                createCmd << m_melCommandOnConnectionCreated << " "
                          << frantic::strings::to_tstring( m_MayaNodeName.asChar() ) << " "
                          << frantic::strings::to_tstring( data.toStdString( ' ' ) ) << ";";

                MString result;
                MStatus stat;
                stat = MGlobal::executeCommand( frantic::strings::to_string( createCmd.str() ).c_str(), result );
            }
        } else {
            // Something went wrong, unreserve the spot
            m_ConnectionDataToConnection.erase( data );
        }
        break; // Done
    }
}

void maya_MagmaFLUX::onNodeAdded( Node* node ) {
    QObject::connect( node, SIGNAL( destroyed( QObject* ) ), this, SLOT( onNodeToBeDestroyed( QObject* ) ) );
}

void maya_MagmaFLUX::onNodeToBeDestroyed( QObject* obj ) {
    // Note this is presumably not safe since by the time we get here, the subclass has been deallocated.
    // As long as you don't reference anything in obj, we should be safe...
    Node* node = reinterpret_cast<Node*>( obj );

    if( m_NodeToID.count( node ) > 0 ) {
        int id = m_NodeToID[node];
        m_NodeToID.erase( node );
        m_IDToNode.erase( id );

        deleteNodeInformation( id );

        if( m_description->contains_node( id ) ) {
            MFnDependencyNode depNode( this->m_MayaObject );
            attr::maya_magma_attr_manager::delete_maya_attr( this->m_description, id, depNode );
            this->m_description->delete_node( id );
        }

        // delete connections
        std::vector<Connection*> edgesToRemove;
        for( std::map<Connection*, connection_data>::const_iterator iter = m_ConnectionToConnectionData.begin();
             iter != m_ConnectionToConnectionData.end(); ++iter ) {
            if( iter->second.m_source == id || iter->second.m_destination == id )
                edgesToRemove.push_back( iter->first );
        }
        for( std::vector<Connection*>::const_iterator iter = edgesToRemove.begin(); iter != edgesToRemove.end();
             ++iter ) {
            delete *iter;
        }
    }
}

void maya_MagmaFLUX::onConnectionToBeDestroyed( QObject* obj ) {
    // Note this is presumably not safe since by the time we get here, the subclass has been deallocated.
    // As long as you don't reference anything in obj, we should be safe...
    Connection* conn = reinterpret_cast<Connection*>( obj );
    if( !conn )
        return;

    if( m_ConnectionToConnectionData.count( conn ) > 0 ) {
        connection_data data = m_ConnectionToConnectionData[conn];

        // Work around to dealing with double edges to the same node.  One of the connections will be deleted leaving
        // the other but it is shown as deleted in the magma description
        Node* srcNode = findNode( data.m_source );
        Node* dstNode = findNode( data.m_destination );

        m_ConnectionToConnectionData.erase( conn );

        if( srcNode != NULL && dstNode != NULL ) {
            if( data.m_source_index < srcNode->getOutputSocketCount() &&
                data.m_destination_index < dstNode->getInputSocketCount() ) {
                Socket* srcSock = srcNode->getOutputSocket( data.m_source_index );
                Socket* dstSock = dstNode->getInputSocket( data.m_destination_index );

                // Find the connection that was added
                foreach( Connection* currentConn, srcSock->connections() ) {
                    if( currentConn == conn )
                        continue; // Probably unnecessary but just in case

                    Socket* srcSocket = currentConn->sourceSocket();
                    Socket* dstSocket = currentConn->destinationSocket();

                    // Check if the sockets are the ones we're looking for
                    if( srcSock != srcSocket || dstSock != dstSocket )
                        continue;

                    // Replace the connection
                    m_ConnectionDataToConnection[data] = currentConn;
                    m_ConnectionToConnectionData[currentConn] = data;
                    return;
                }
            }
        }

        m_ConnectionDataToConnection.erase( data );

        this->m_description->delete_connection( data.m_source, data.m_source_index, data.m_destination,
                                                data.m_destination_index );

        if( m_EnableForwardMelCommand ) {
            // delete connection event
            frantic::tsstream::tostringstream createCmd;
            createCmd << m_melCommandOnConnectionDeleted << " "
                      << frantic::strings::to_tstring( m_MayaNodeName.asChar() ) << " "
                      << frantic::strings::to_tstring( data.toStdString( ' ' ) ) << ";";
            MString result;
            MStatus stat = MGlobal::executeCommand( frantic::strings::to_string( createCmd.str() ).c_str(), result );
        }
    }
}

void maya_MagmaFLUX::deleteConnection( Connection* conn ) {
    this->scene()->removeItem( conn );

    Socket* src = conn->sourceSocket();
    if( src != 0 )
        src->removeConnection( (QObject*)conn );

    Socket* dst = conn->destinationSocket();
    if( dst != 0 )
        dst->removeConnection( (QObject*)conn );

    delete conn;
}

void maya_MagmaFLUX::onSelectionChanged() {
    if( m_EnableForwardMelCommand ) {
        bool forwardNodeSelection = !this->m_melCommandOnNodeSelectionChanged.empty();
        bool forwardConnSelection = !this->m_melCommandOnConnectionSelectionChanged.empty();
        if( !forwardNodeSelection && !forwardConnSelection )
            return;

        frantic::tsstream::tostringstream cmdNode;
        if( forwardNodeSelection ) {
            cmdNode << m_melCommandOnNodeSelectionChanged << " "
                    << frantic::strings::to_tstring( m_MayaNodeName.asChar() ) << " "
                    << "{";
        }

        frantic::tsstream::tostringstream cmdConn;
        if( forwardConnSelection ) {
            cmdConn << m_melCommandOnConnectionSelectionChanged << " "
                    << frantic::strings::to_tstring( m_MayaNodeName.asChar() ) << " "
                    << "{";
        }

        bool firstNode = true;
        bool firstConn = true;
        foreach( QGraphicsItem* item, this->scene()->selectedItems() ) {
            Node* node = qgraphicsitem_cast<Node*>( item );
            if( node != NULL ) {
                if( m_NodeToID.count( node ) > 0 ) {
                    if( forwardNodeSelection ) {
                        cmdNode << ( firstNode ? "" : "," ) << m_NodeToID[node];
                        firstNode = false;
                    }

                    m_lastNodeX = static_cast<int>( node->x() );
                    m_lastNodeY = static_cast<int>( node->y() );
                }
            }

            if( forwardConnSelection ) {
                Connection* conn = qgraphicsitem_cast<Connection*>( item );
                if( conn != NULL ) {
                    if( m_ConnectionToConnectionData.count( conn ) > 0 ) {
                        cmdConn << ( firstConn ? "" : "," ) << "\""
                                << frantic::strings::to_tstring( m_ConnectionToConnectionData[conn].toStdString() )
                                << "\"";
                        firstConn = false;
                    }
                }
            }
        }

        MString result;
        MStatus stat;
        if( forwardConnSelection ) {
            cmdConn << "}"
                    << ";";
            stat = MGlobal::executeCommand( cmdConn.str().c_str(), result );
        }
        if( forwardNodeSelection ) {
            cmdNode << "}"
                    << ";";
            stat = MGlobal::executeCommand( cmdNode.str().c_str(), result );
        }
    }
}

void maya_MagmaFLUX::onMousePress( QMouseEvent* event ) {
    m_lastMousePressX = event->pos().x();
    m_lastMousePressY = event->pos().y();
}

void maya_MagmaFLUX::onMouseRelease( QMouseEvent* event ) {
    foreach( QGraphicsItem* item, this->scene()->selectedItems() ) {
        Node* node = qgraphicsitem_cast<Node*>( item );
        if( node != NULL ) {
            if( this->m_NodeToID.count( node ) > 0 ) {
                int xPosition = static_cast<int>( node->x() );
                int yPosition = static_cast<int>( node->y() );
                int id = m_NodeToID[node];

                m_description->set_node_position( id, xPosition, yPosition );

                m_lastNodeX = xPosition;
                m_lastNodeY = yPosition;
            }
        }
    }
}

Node* maya_MagmaFLUX::addNode( int id, const char* type, const char* name, int xPosition, int yPosition ) {
    if( m_IDToNode.count( id ) <= 0 ) {
        m_IDToNode[id] = NULL; // Reserve a spot

        frantic::magma::maya::info::maya_magma_node_info magmaNodeInfo;
        try {
            magmaNodeInfo = frantic::magma::maya::factory::maya_magma_node_info_factory::create_node_infos(
                frantic::strings::to_tstring( type ) );
        } catch( std::exception& e ) {
            FF_LOG( error ) << e.what() << std::endl;
            MGlobal::displayError( e.what() );
        }

        int numInputs = (int)magmaNodeInfo.m_inputSocketInfos.size();
        int numOutputs = (int)magmaNodeInfo.m_outputSocketInfos.size();

        Node* magmaNode = this->createNode( numInputs, numOutputs, type );
        magmaNode->beginModifyNode();
        magmaNode->setTitleText( name );

        // For ParticleQuery, the Position output socket is enabled in the channel by default yet it is not accounted
        // for in the channels property
        frantic::tstring stype = frantic::strings::to_tstring( type );
        if( stype == _T("ParticleQuery") || stype == _T("PropertyQuery") ) {
            numOutputs = 0;
        }

        // For VertexQuery, there's a checkbox "exposePosition" to enable and disable the Position output socket
        else if( stype == _T("VertexQuery") || stype == _T("FaceQuery") ) {
            numOutputs = 0;
        }

        // For Mux, inputs depend solely on the number of inputs property
        else if( stype == _T("Mux") ) {
            numInputs = 0;
        }

        // Set up input Sockets
        if( numInputs > 0 ) {
            for( std::vector<frantic::magma::maya::info::maya_magma_node_input_socket_info>::const_iterator iter =
                     magmaNodeInfo.m_inputSocketInfos.begin();
                 iter != magmaNodeInfo.m_inputSocketInfos.end(); ++iter ) {
                int index = iter->m_index;
                QString name( frantic::strings::to_string( iter->m_description ).c_str() );
                magmaNode->getInputSocket( index )->setSocketName( name );
            }
        }
        // Set up output Sockets
        if( numOutputs > 0 ) {
            for( std::vector<frantic::magma::maya::info::maya_magma_node_output_socket_info>::const_iterator iter =
                     magmaNodeInfo.m_outputSocketInfos.begin();
                 iter != magmaNodeInfo.m_outputSocketInfos.end(); ++iter ) {
                int index = iter->m_index;
                QString name( frantic::strings::to_string( iter->m_description ).c_str() );
                magmaNode->getOutputSocket( index )->setSocketName( name );
            }
        }

        magmaNode->setPos( static_cast<qreal>( xPosition ), static_cast<qreal>( yPosition ) );

        m_IDToNode[id] = magmaNode;
        m_NodeToID[magmaNode] = id;
        m_IDToMetaInfo[id] = node_meta_data( magmaNode, frantic::strings::to_tstring( type ), numInputs, numOutputs );

        // m_lastNodeX = xPosition;
        // m_lastNodeY = yPosition;
        magmaNode->endModifyNode();
        return magmaNode;
    }
    return NULL;
}

Node* maya_MagmaFLUX::addNode( int id, frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                               MFnDependencyNode& mayaNode ) {
    if( desc->get_node_parent( id ) != m_parentNode )
        return NULL;

    // Set up basic properties
    frantic::tstring type = desc->get_node_type( id );
    frantic::tstring name = desc->get_node_name( id );
    int x, y;
    desc->get_node_position( id, x, y );
    Node* node =
        addNode( id, frantic::strings::to_string( type ).c_str(), frantic::strings::to_string( name ).c_str(), x, y );
    if( node == NULL )
        return node;

    MStatus status;

    // Make additional changes to nodes based on its properties
    std::vector<frantic::tstring> attributes = desc->get_desc_node_property_maya_attr_names( id );
    for( std::vector<frantic::tstring>::const_iterator iter = attributes.begin(); iter != attributes.end(); ++iter ) {
        MPlug propertyPlug = mayaNode.findPlug( frantic::strings::to_string( *iter ).c_str(), &status );
        if( status == MStatus::kSuccess ) {
            onAttributeChangedUpdateNodesInWindow( propertyPlug );
        }
    }
    return node;
}

Node* maya_MagmaFLUX::findNode( int id ) const {
    std::map<int, Node*>::const_iterator iter = m_IDToNode.find( id );
    if( iter != m_IDToNode.end() )
        return iter->second;
    return NULL;
}

Connection* maya_MagmaFLUX::findConnection( int srcID, int srcSocketIndex, int dstID, int dstSocketIndex ) const {
    maya_MagmaFLUX::connection_data edge( srcID, srcSocketIndex, dstID, dstSocketIndex );
    std::map<connection_data, Connection*>::const_iterator iter = m_ConnectionDataToConnection.find( edge );
    if( iter != m_ConnectionDataToConnection.end() )
        return iter->second;
    return NULL;
}

bool maya_MagmaFLUX::removeNode( int id ) {
    Node* toRemove = findNode( id );
    if( toRemove == NULL )
        return false;

    this->deleteNode( toRemove );
    return true;
}

bool maya_MagmaFLUX::addConnection( int srcID, int srcSocketIndex, int dstID, int dstSocketIndex ) {
    maya_MagmaFLUX::connection_data edge( srcID, srcSocketIndex, dstID, dstSocketIndex );
    if( m_ConnectionDataToConnection.count( edge ) <= 0 ) {
        // Find the corresponding nodes in the gui
        Node* srcNode = findNode( srcID );
        Node* dstNode = findNode( dstID );
        if( srcNode == NULL || dstNode == NULL )
            return false;

        m_ConnectionDataToConnection[edge] = NULL; // Reserve a spot
        if( srcSocketIndex < srcNode->getOutputSocketCount() && dstSocketIndex < dstNode->getInputSocketCount() ) {
            Socket* srcSocket = srcNode->getOutputSocket( srcSocketIndex );
            Socket* dstSocket = dstNode->getInputSocket( dstSocketIndex );
            if( srcSocket != NULL && dstSocket != NULL ) {
                bool ok = srcSocket->createConnection( dstNode->getInputSocket( dstSocketIndex ) );
                if( ok ) {
                    // Find the connection that was added
                    foreach( Connection* conn, srcSocket->connections() ) {
                        Socket* srcSock = conn->sourceSocket();
                        Socket* dstSock = conn->destinationSocket();

                        // Check if the sockets are the ones we're looking for
                        if( srcSock != srcSocket || dstSock != dstSocket )
                            continue;

                        m_ConnectionDataToConnection[edge] = conn;
                        m_ConnectionToConnectionData[conn] = edge;

                        if( m_EnableForwardMelCommand ) {
                            // Create connection event
                            frantic::tsstream::tostringstream createCmd;
                            createCmd << m_melCommandOnConnectionCreated << " "
                                      << frantic::strings::to_tstring( m_MayaNodeName.asChar() ) << " "
                                      << frantic::strings::to_tstring( edge.toStdString( ' ' ) ) << ";";
                            MString result;
                            MStatus stat = MGlobal::executeCommand(
                                frantic::strings::to_string( createCmd.str() ).c_str(), result );
                        }
                        return true;
                    }
                }
            }
        }
        m_ConnectionDataToConnection.erase( edge ); // Something went wrong with adding, unreserve the spot
    }
    return false;
}

bool maya_MagmaFLUX::removeConnection( int srcID, int srcSocketIndex, int dstID, int dstSocketIndex ) {
    Connection* toRemove = findConnection( srcID, srcSocketIndex, dstID, dstSocketIndex );
    if( toRemove == NULL )
        return false;

    this->deleteConnection( toRemove );
    return true;
}

void maya_MagmaFLUX::setNodePosition( int id, int xPosition, int yPosition ) {
    Node* node = findNode( id );
    if( node == NULL )
        return;

    node->setPos( static_cast<qreal>( xPosition ), static_cast<qreal>( yPosition ) );
}

void maya_MagmaFLUX::setNodeColor( int id, const unsigned char rgba[4] ) {
    Node* node = findNode( id );
    if( node == NULL )
        return;

    m_IDToMetaInfo[id].m_rgba[0] = rgba[0];
    m_IDToMetaInfo[id].m_rgba[1] = rgba[1];
    m_IDToMetaInfo[id].m_rgba[2] = rgba[2];
    m_IDToMetaInfo[id].m_rgba[3] = rgba[3];
    updateNodeAppearance( id );
}

bool maya_MagmaFLUX::getNodeColor( int id, unsigned char rgba[4] ) const {
    Node* node = findNode( id );
    if( node == NULL )
        return false;

    std::map<int, node_meta_data>::const_iterator iter = m_IDToMetaInfo.find( id );
    rgba[0] = iter->second.m_rgba[0];
    rgba[1] = iter->second.m_rgba[1];
    rgba[2] = iter->second.m_rgba[2];
    rgba[3] = iter->second.m_rgba[3];
    return true;
}

void maya_MagmaFLUX::setNodeTitleColor( int id, const unsigned char rgba[4] ) {
    Node* node = findNode( id );
    if( node == NULL )
        return;

    m_IDToMetaInfo[id].m_title_rgba[0] = rgba[0];
    m_IDToMetaInfo[id].m_title_rgba[1] = rgba[1];
    m_IDToMetaInfo[id].m_title_rgba[2] = rgba[2];
    m_IDToMetaInfo[id].m_title_rgba[3] = rgba[3];
    updateNodeAppearance( id );
}

bool maya_MagmaFLUX::getNodeTitleColor( int id, unsigned char rgba[4] ) const {
    Node* node = findNode( id );
    if( node == NULL )
        return false;

    std::map<int, node_meta_data>::const_iterator iter = m_IDToMetaInfo.find( id );
    rgba[0] = iter->second.m_title_rgba[0];
    rgba[1] = iter->second.m_title_rgba[1];
    rgba[2] = iter->second.m_title_rgba[2];
    rgba[3] = iter->second.m_title_rgba[3];
    return true;
}

void maya_MagmaFLUX::setNodeOutlineColor( int id, const unsigned char rgba[4] ) {
    Node* node = findNode( id );
    if( node == NULL )
        return;

    m_IDToMetaInfo[id].m_outline_rgba[0] = rgba[0];
    m_IDToMetaInfo[id].m_outline_rgba[1] = rgba[1];
    m_IDToMetaInfo[id].m_outline_rgba[2] = rgba[2];
    m_IDToMetaInfo[id].m_outline_rgba[3] = rgba[3];
    updateNodeAppearance( id );
}

bool maya_MagmaFLUX::getNodeOutlineColor( int id, unsigned char rgba[4] ) const {
    Node* node = findNode( id );
    if( node == NULL )
        return false;

    std::map<int, node_meta_data>::const_iterator iter = m_IDToMetaInfo.find( id );
    rgba[0] = iter->second.m_outline_rgba[0];
    rgba[1] = iter->second.m_outline_rgba[1];
    rgba[2] = iter->second.m_outline_rgba[2];
    rgba[3] = iter->second.m_outline_rgba[3];
    return true;
}

void maya_MagmaFLUX::setNodeTitleTextColor( int id, const unsigned char rgba[4] ) {
    Node* node = findNode( id );
    if( node == NULL )
        return;

    m_IDToMetaInfo[id].m_title_text_rgba[0] = rgba[0];
    m_IDToMetaInfo[id].m_title_text_rgba[1] = rgba[1];
    m_IDToMetaInfo[id].m_title_text_rgba[2] = rgba[2];
    m_IDToMetaInfo[id].m_title_text_rgba[3] = rgba[3];
    updateNodeAppearance( id );
}

bool maya_MagmaFLUX::getNodeTitleTextColor( int id, unsigned char rgba[4] ) const {
    Node* node = findNode( id );
    if( node == NULL )
        return false;

    std::map<int, node_meta_data>::const_iterator iter = m_IDToMetaInfo.find( id );
    rgba[0] = iter->second.m_title_text_rgba[0];
    rgba[1] = iter->second.m_title_text_rgba[1];
    rgba[2] = iter->second.m_title_text_rgba[2];
    rgba[3] = iter->second.m_title_text_rgba[3];
    return true;
}

void maya_MagmaFLUX::setNodeSocketColor( int id, int socket, bool isOutput, const unsigned char rgba[4] ) {
    Node* node = findNode( id );
    if( node == NULL )
        return;

    Socket* sock;
    if( isOutput ) {
        if( socket >= node->getOutputSocketCount() )
            return;
        sock = node->getOutputSocket( socket );
    } else {
        if( socket >= node->getInputSocketCount() )
            return;
        sock = node->getInputSocket( socket );
    }

    node->beginModifyNode();
    QColor color( rgba[0], rgba[1], rgba[2], rgba[3] );
    sock->setFillColor( color );
    node->endModifyNode();
}

bool maya_MagmaFLUX::getNodeSocketColor( int id, int socket, bool isOutput, unsigned char rgba[4] ) const {
    Node* node = findNode( id );
    if( node == NULL )
        return false;

    Socket* sock;
    if( isOutput ) {
        if( socket >= node->getOutputSocketCount() )
            return false;
        sock = node->getOutputSocket( socket );
    } else {
        if( socket >= node->getInputSocketCount() )
            return false;
        sock = node->getInputSocket( socket );
    }

    QColor color = sock->fillColor();
    int r = color.red();
    int g = color.green();
    int b = color.blue();
    int a = color.alpha();
    rgba[0] = static_cast<unsigned char>( r );
    rgba[1] = static_cast<unsigned char>( g );
    rgba[2] = static_cast<unsigned char>( b );
    rgba[3] = static_cast<unsigned char>( a );

    return true;
}

void maya_MagmaFLUX::setNodeSocketTextColor( int id, int socket, bool isOutput, const unsigned char rgba[4] ) {
    Node* node = findNode( id );
    if( node == NULL )
        return;

    Socket* sock;
    if( isOutput ) {
        if( socket >= node->getOutputSocketCount() )
            return;
        sock = node->getOutputSocket( socket );
    } else {
        if( socket >= node->getInputSocketCount() )
            return;
        sock = node->getInputSocket( socket );
    }

    node->beginModifyNode();
    QColor color( rgba[0], rgba[1], rgba[2], rgba[3] );
    sock->setLabelColor( color );
    node->endModifyNode();
}

bool maya_MagmaFLUX::getNodeSocketTextColor( int id, int socket, bool isOutput, unsigned char rgba[4] ) const {
    Node* node = findNode( id );
    if( node == NULL )
        return false;

    Socket* sock;
    if( isOutput ) {
        if( socket >= node->getOutputSocketCount() )
            return false;
        sock = node->getOutputSocket( socket );
    } else {
        if( socket >= node->getInputSocketCount() )
            return false;
        sock = node->getInputSocket( socket );
    }

    QColor color = sock->labelColor();
    int r = color.red();
    int g = color.green();
    int b = color.blue();
    int a = color.alpha();
    rgba[0] = static_cast<unsigned char>( r );
    rgba[1] = static_cast<unsigned char>( g );
    rgba[2] = static_cast<unsigned char>( b );
    rgba[3] = static_cast<unsigned char>( a );

    return true;
}

void maya_MagmaFLUX::setNodeSocketOutlineColor( int id, int socket, bool isOutput, const unsigned char rgba[4] ) {
    Node* node = findNode( id );
    if( node == NULL )
        return;

    Socket* sock;
    if( isOutput ) {
        if( socket >= node->getOutputSocketCount() )
            return;
        sock = node->getOutputSocket( socket );
    } else {
        if( socket >= node->getInputSocketCount() )
            return;
        sock = node->getInputSocket( socket );
    }

    node->beginModifyNode();
    QColor color( rgba[0], rgba[1], rgba[2], rgba[3] );
    sock->setOutlineColor( color );
    node->endModifyNode();
}

bool maya_MagmaFLUX::getNodeSocketOutlineColor( int id, int socket, bool isOutput, unsigned char rgba[4] ) const {
    Node* node = findNode( id );
    if( node == NULL )
        return false;

    Socket* sock;
    if( isOutput ) {
        if( socket >= node->getOutputSocketCount() )
            return false;
        sock = node->getOutputSocket( socket );
    } else {
        if( socket >= node->getInputSocketCount() )
            return false;
        sock = node->getInputSocket( socket );
    }

    QColor color = sock->outlineColor();
    int r = color.red();
    int g = color.green();
    int b = color.blue();
    int a = color.alpha();
    rgba[0] = static_cast<unsigned char>( r );
    rgba[1] = static_cast<unsigned char>( g );
    rgba[2] = static_cast<unsigned char>( b );
    rgba[3] = static_cast<unsigned char>( a );

    return true;
}

void maya_MagmaFLUX::setNodeError( int id, bool isError ) {
    Node* node = findNode( id );
    if( node == NULL )
        return;

    if( isError ) {
        m_IDToIsError.insert( id );
    } else {
        m_IDToIsError.erase( id );
    }
    updateNodeAppearance( id );
}

void maya_MagmaFLUX::clearNodeErrors() {
    std::vector<int> ids;
    for( std::set<int>::const_iterator iter = m_IDToIsError.begin(); iter != m_IDToIsError.end(); ++iter ) {
        ids.push_back( *iter );
    }

    m_IDToIsError.clear();
    for( std::vector<int>::const_iterator iter = ids.begin(); iter != ids.end(); ++iter ) {
        updateNodeAppearance( *iter );
    }
}

void maya_MagmaFLUX::updateNodeAppearance( int id ) {
    // This method is used to modify a color from its based color due to other properties
    Node* node = findNode( id );
    if( node == NULL )
        return;

    node->beginModifyNode();

    QColor baseColor = m_IDToMetaInfo[id].getNodeColor();
    QColor titleColor = m_IDToMetaInfo[id].getTitleColor();
    QColor outlineColor = m_IDToMetaInfo[id].getOutlineColor();
    QColor titleTextColor = m_IDToMetaInfo[id].getTitleTextColor();

    if( m_IDToIsError.count( id ) > 0 ) {
        QPen outlinePen = node->outlinePen();
        outlinePen.setColor( Qt::red );
        node->setOutlinePen( outlinePen );

        QPen selectPen = node->selectedPen();
        QColor selectErrorColor( 255, 128, 128 );
        selectPen.setColor( selectErrorColor );
        node->setSelectedPen( selectPen );

    } else {
        QPen outlinePen = node->outlinePen();
        outlinePen.setColor( outlineColor );
        node->setOutlinePen( outlinePen );

        QPen selectPen = node->selectedPen();
        selectPen.setColor( QColor( 255, 255, 255 ) );
        node->setSelectedPen( selectPen );
    }

    QBrush baseBrush = node->backgroundBrush();
    QBrush titleBrush = node->titleBarBrush();
    QBrush titleTextBrush = node->titleTextBrush();
    baseBrush.setColor( baseColor );
    titleBrush.setColor( titleColor );
    titleTextBrush.setColor( titleTextColor );

    node->setBackgroundBrush( baseBrush );
    node->setSelectedBrush( baseBrush );
    node->setTitleBarBrush( titleBrush );
    node->setTitleBarSelectedBrush( titleBrush );
    node->setTitleTextBrush( titleTextBrush );

    node->endModifyNode();
}

void maya_MagmaFLUX::setConnectionColor( int srcID, int srcSocketIndex, int dstID, int dstSocketIndex,
                                         const unsigned char rgba[4] ) {
    Connection* conn = findConnection( srcID, srcSocketIndex, dstID, dstSocketIndex );
    if( conn == NULL )
        return;

    QColor color( rgba[0], rgba[1], rgba[2], rgba[3] );

    QBrush brush = conn->brush();
    brush.setColor( color );
    conn->setBrush( brush );

    QPen pen = conn->pen();
    pen.setColor( color );
    conn->setPen( pen );
}

bool maya_MagmaFLUX::getConnectionColor( int srcID, int srcSocketIndex, int dstID, int dstSocketIndex,
                                         unsigned char rgba[4] ) const {
    Connection* conn = findConnection( srcID, srcSocketIndex, dstID, dstSocketIndex );
    if( conn == NULL )
        return false;

    QColor color = conn->brush().color();
    int r = color.red();
    int g = color.green();
    int b = color.blue();
    int a = color.alpha();
    rgba[0] = static_cast<unsigned char>( r );
    rgba[1] = static_cast<unsigned char>( g );
    rgba[2] = static_cast<unsigned char>( b );
    rgba[3] = static_cast<unsigned char>( a );

    return true;
}

void maya_MagmaFLUX::setNodeExtraOutputSockets( int id, const std::vector<frantic::tstring>& socketNames,
                                                bool tryShift ) {
    Node* node = findNode( id );
    if( node == NULL )
        return;

    int index;
    int outputBaseCount = m_IDToMetaInfo[id].m_base_output_socket_count;

    // Cancel forwarding of mel script for now (something's causing it to crash)
    bool initialForwarding = m_EnableForwardMelCommand;
    m_EnableForwardMelCommand = false;

    int finalCount = outputBaseCount + (int)socketNames.size();
    int currentCount = node->getOutputSocketCount();

    // Shift connections (implementation is specific to nodes with "exposePosition" for now)
    std::vector<std::vector<connection_data>> lastConnections;
    int numSockets;
    if( tryShift ) {
        // Keep track of the last set of connections
        numSockets = std::min( finalCount, currentCount );
        for( int i = 0; i < currentCount; i++ ) {
            // Get all the connections there
            std::vector<Connection*> toDelete;
            foreach( Connection* conn, node->getOutputSocket( i )->connections() ) {
                toDelete.push_back( conn );
            }

            // Backup the needed connections and delete the rest
            if( i >= currentCount - numSockets )
                lastConnections.push_back( std::vector<connection_data>() );
            for( std::vector<Connection*>::const_iterator iter = toDelete.begin(); iter != toDelete.end(); ++iter ) {
                Connection* conn = *iter;
                if( i >= currentCount - numSockets )
                    lastConnections.back().push_back( m_ConnectionToConnectionData[conn] );
                deleteConnection( conn );
            }
        }
    }

    // Add additional sockets if needed
    while( node->getOutputSocketCount() < finalCount ) {
        node->addOutputSocket();
    }

    // Rename existing sockets
    index = outputBaseCount;
    for( std::vector<frantic::tstring>::const_iterator iter = socketNames.begin(); iter != socketNames.end(); ++iter ) {
        node->setOutputSocketName( index, frantic::strings::to_string( *iter ).c_str() );
        index++;
    }

    // Remove sockets if needed
    while( node->getOutputSocketCount() > finalCount ) {
        std::vector<Connection*> toDelete;
        foreach( Connection* conn, node->getOutputSocket( node->getOutputSocketCount() - 1 )->connections() ) {
            toDelete.push_back( conn );
        }

        for( std::vector<Connection*>::const_iterator iter = toDelete.begin(); iter != toDelete.end(); ++iter ) {
            Connection* conn = *iter;
            deleteConnection( conn );
        }
        node->deleteOutputSocket( node->getOutputSocketCount() - 1 );
    }

    // Restore connections
    if( tryShift ) {
        for( int i = numSockets - 1; i >= 0; i-- ) {
            int newSocketIndex = finalCount - numSockets + i;
            for( std::vector<connection_data>::const_iterator iter = lastConnections[i].begin();
                 iter != lastConnections[i].end(); ++iter ) {
                connection_data lastConnection = *iter;
                lastConnection.m_source_index = newSocketIndex;
                this->m_description->create_connection( lastConnection.m_source, lastConnection.m_source_index,
                                                        lastConnection.m_destination,
                                                        lastConnection.m_destination_index );
                this->addConnection( lastConnection.m_source, lastConnection.m_source_index,
                                     lastConnection.m_destination, lastConnection.m_destination_index );
            }
        }
    }

    // Restore forwarding
    m_EnableForwardMelCommand = initialForwarding;
}

void maya_MagmaFLUX::setNodeExtraInputSockets( int id, const std::vector<frantic::tstring>& socketNames,
                                               bool tryShift ) {
    Node* node = findNode( id );
    if( node == NULL )
        return;

    int index;
    int inputBaseCount = m_IDToMetaInfo[id].m_base_input_socket_count;

    // Cancel forwarding of mel script for now (something's causing it to crash)
    bool initialForwarding = m_EnableForwardMelCommand;
    m_EnableForwardMelCommand = false;

    // Shift connections (implementation is specific to Mux node right now)
    connection_data lastConnection( -1, -1, -1, -1 );
    if( tryShift ) {
        if( node->getInputSocketCount() > 0 ) {
            QList<Connection*> connections = node->getInputSocket( node->getInputSocketCount() - 1 )->connections();
            if( !connections.empty() ) {
                Connection* conn = connections.back();
                lastConnection =
                    m_ConnectionToConnectionData[conn]; // There should be only one connection per input socket
                deleteConnection( conn );
            }
        }
    }

    // Add additional sockets if needed
    bool socketsChanged = false;
    while( node->getInputSocketCount() < inputBaseCount + socketNames.size() ) {
        node->addInputSocket();
        socketsChanged = true;
    }

    // Rename existing sockets
    index = inputBaseCount;
    for( std::vector<frantic::tstring>::const_iterator iter = socketNames.begin(); iter != socketNames.end(); ++iter ) {
        node->setInputSocketName( index, frantic::strings::to_string( *iter ).c_str() );
        index++;
    }

    // Remove sockets if needed
    while( node->getInputSocketCount() > inputBaseCount + socketNames.size() ) {
        std::vector<Connection*> toDelete;
        foreach( Connection* conn, node->getInputSocket( node->getInputSocketCount() - 1 )->connections() ) {
            toDelete.push_back( conn );
        }

        for( std::vector<Connection*>::const_iterator iter = toDelete.begin(); iter != toDelete.end(); ++iter ) {
            Connection* conn = *iter;
            deleteConnection( conn );
        }
        node->deleteInputSocket( node->getInputSocketCount() - 1 );
        socketsChanged = true;
    }

    if( tryShift && socketsChanged ) {
        // Delete any connection that was there previously since it's meant for the selector
        if( node->getInputSocketCount() > 0 ) {
            std::vector<Connection*> toDelete;
            foreach( Connection* conn, node->getInputSocket( node->getInputSocketCount() - 1 )->connections() ) {
                toDelete.push_back( conn );
            }

            for( std::vector<Connection*>::const_iterator iter = toDelete.begin(); iter != toDelete.end(); ++iter ) {
                Connection* conn = *iter;
                deleteConnection( conn );
            }
        }
    }

    if( tryShift && socketsChanged ) {
        // Reconnect mux selector to the end
        if( lastConnection.m_source >= 0 && lastConnection.m_source_index >= 0 ) {
            lastConnection.m_destination_index = node->getInputSocketCount() - 1;
            this->m_description->create_connection( lastConnection.m_source, lastConnection.m_source_index,
                                                    lastConnection.m_destination, lastConnection.m_destination_index );
            this->addConnection( lastConnection.m_source, lastConnection.m_source_index, lastConnection.m_destination,
                                 lastConnection.m_destination_index );
        }
    }

    // Restore forwarding
    m_EnableForwardMelCommand = initialForwarding;
}

std::vector<int> maya_MagmaFLUX::getSelectedNodes() {
    std::vector<int> nodes;

    foreach( QGraphicsItem* item, this->scene()->selectedItems() ) {
        Node* node = qgraphicsitem_cast<Node*>( item );
        if( node != NULL ) {
            if( m_NodeToID.count( node ) > 0 ) {
                int id = m_NodeToID[node];
                nodes.push_back( id );
            }
        }
    }

    return nodes;
}

std::vector<maya_MagmaFLUX::connection_data> maya_MagmaFLUX::getSelectedEdges() {
    std::vector<maya_MagmaFLUX::connection_data> edges;

    foreach( QGraphicsItem* item, this->scene()->selectedItems() ) {
        Connection* conn = qgraphicsitem_cast<Connection*>( item );
        if( conn != NULL ) {
            if( m_ConnectionToConnectionData.count( conn ) > 0 ) {
                connection_data data = m_ConnectionToConnectionData[conn];
                edges.push_back( data );
            }
        }
    }

    return edges;
}

void maya_MagmaFLUX::setSelection( std::vector<int>& nodes, std::vector<connection_data>& edges ) {
    this->scene()->clearSelection();

    for( std::vector<int>::const_iterator iter = nodes.begin(); iter != nodes.end(); ++iter ) {
        Node* node = this->findNode( *iter );
        if( node != NULL )
            node->setSelected( true );
    }

    for( std::vector<connection_data>::const_iterator iter = edges.begin(); iter != edges.end(); ++iter ) {
        Connection* conn = this->findConnection( iter->m_source, iter->m_source_index, iter->m_destination,
                                                 iter->m_destination_index );
        if( conn != NULL )
            conn->setSelected( true );
    }
}

MString maya_MagmaFLUX::getWindowName() const { return m_WindowName; }

MString maya_MagmaFLUX::getMayaNodeName() const { return m_MayaNodeName; }

void maya_MagmaFLUX::setNodeOutputSocketName( int id, int index, QString label ) {
    Node* node = findNode( id );
    if( node == NULL )
        return;
    if( index >= node->getOutputSocketCount() )
        return;

    node->getOutputSocket( index )->setSocketName( label );
}

void maya_MagmaFLUX::setNodeInputSocketName( int id, int index, QString label ) {
    Node* node = findNode( id );
    if( node == NULL )
        return;
    if( index >= node->getInputSocketCount() )
        return;

    node->getInputSocket( index )->setSocketName( label );
}

void maya_MagmaFLUX::setNodeName( int id, QString label ) {
    Node* node = findNode( id );
    if( node == NULL )
        return;

    node->setTitleText( label );
}

void maya_MagmaFLUX::getLastNodePosition( int& outX, int& outY ) const {
    outX = m_lastNodeX;
    outY = m_lastNodeY;
}

void maya_MagmaFLUX::setLastNodePosition( int x, int y ) {
    m_lastNodeX = x;
    m_lastNodeY = y;
}

void maya_MagmaFLUX::deleteNodeInformation( int id ) {
    m_IDToMetaInfo.erase( id );
    m_IDToIsError.erase( id );
}

void maya_MagmaFLUX::keyPressEvent( QKeyEvent* event ) {
    if( m_EnableForwardMelCommand ) {
        if( !this->m_melCommandOnKeyboardPressed.empty() ) {
            Qt::KeyboardModifiers mod = event->modifiers();
            bool shift = (bool)( mod & Qt::ShiftModifier );
            bool ctrl = (bool)( mod & Qt::ControlModifier );
            bool alt = (bool)( mod & Qt::AltModifier );

            frantic::tsstream::tostringstream cmd;
            cmd << m_melCommandOnKeyboardPressed << " " << frantic::strings::to_tstring( m_MayaNodeName.asChar() )
                << " " << event->key() << " " << ( shift ? "true" : "false" ) << " " << ( ctrl ? "true" : "false" )
                << " " << ( alt ? "true" : "false" ) << ";";

            MString result;
            MStatus stat = MGlobal::executeCommand( cmd.str().c_str(), result );
        }
    }
}

void maya_MagmaFLUX::onRightClickShowMenu( const QPoint& point ) {
    QPointF scenePoint = this->mapToScene( point );
    showRightClickMenu( static_cast<int>( scenePoint.x() ), static_cast<int>( scenePoint.y() ) );
}

MString maya_MagmaFLUX::showRightClickMenu( int x, int y ) {
    QPoint popupPoint = this->mapFromScene( x, y );
    QPoint scenePoint( x, y );
    QPoint windowPosition = mapToGlobal( popupPoint );
    MString result;

    QMenu* menu = m_rightClickMenu.getQMenu();

    QAction* selectedItem = menu->exec( windowPosition );
    if( selectedItem != NULL ) {
        MagmaFLUXQAction* action = dynamic_cast<MagmaFLUXQAction*>( selectedItem );
        if( action != NULL ) {
            action->runCommand( scenePoint, this->m_MayaNodeName, result );
        }
    }

    return result;
}

void maya_MagmaFLUX::addRightClickMenu( const std::vector<frantic::tstring>& menuPath, frantic::tstring menuCommand ) {
    m_rightClickMenu.insert( menuPath, menuCommand );
}

void maya_MagmaFLUX::clearRightClickMenu() { m_rightClickMenu.clear(); }

void maya_MagmaFLUX::onConnectedToEmpty( Socket* socket, QPointF pos ) {
    if( m_EnableForwardMelCommand ) {
        Node* node = qgraphicsitem_cast<Node*>( socket->parentItem() );
        if( node == NULL || m_NodeToID.count( node ) <= 0 )
            return;

        int x = static_cast<int>( pos.x() );
        int y = static_cast<int>( pos.y() );

        int nodeID = m_NodeToID[node];

        bool isSourceNode = ( socket->socketType() == Socket::Output );
        int socketID = -1;
        int index = 0;
        if( socket != NULL ) {
            foreach( Socket* sock, ( isSourceNode ? node->getOutputSockets() : node->getInputSockets() ) ) {
                if( socket == sock ) {
                    socketID = index;
                    break;
                }
                index++;
            }
        }

        frantic::tsstream::tostringstream cmd;
        cmd << m_melCommandOnConnectionToEmpty << " " << frantic::strings::to_tstring( m_MayaNodeName.asChar() ) << " "
            << x << " " << y << " " << nodeID << " " << ( isSourceNode ? "true" : "false" ) << " " << socketID << ";";

        MString result;
        MStatus stat = MGlobal::executeCommand( cmd.str().c_str(), result );
    }
}

void maya_MagmaFLUX::onAttributeChangedUpdateNodesInWindow( MPlug& plug ) {
    MStatus status;
    MString name = plug.name();

    std::string sbuffer = frantic::strings::to_string( frantic::maya::from_maya_t( name ) );

    // This matches node properties but not the input socket properties
    boost::regex propertyparser(
        "\\s*(\\S+?)\\s*\\.\\s*(\\S+?)\\s*_\\s*(\\S+?)\\s*_\\s*(\\S+?)\\s*_\\s*(\\S+?)\\s*_\\s*(\\S+?)\\s*" );
    boost::match_results<std::string::const_iterator> what;
    if( boost::regex_search( sbuffer, what, propertyparser, boost::match_default ) ) {

        frantic::tstring nodeName( frantic::tstring( what[1].first, what[1].second ) );

        frantic::tstring nodeType( frantic::tstring( what[2].first, what[2].second ) );
        int nodeID;
        try {
            nodeID = boost::lexical_cast<int>( frantic::tstring( what[3].first, what[3].second ) );
        } catch( boost::bad_lexical_cast& ) {
            // Invalid, no need to update anything
            return;
        }
        frantic::tstring propertyName( frantic::tstring( what[5].first, what[5].second ) );

        try {
            if( nodeType == _T("InputChannel") && propertyName == _T("channelName") ) {
                // Input channel name
                MString name;
                status = plug.getValue( name, MDGContext::fsNormal );
                this->setNodeOutputSocketName( nodeID, 0, name.asChar() );

            } else if( nodeType == _T("Output") && propertyName == _T("channelName") ) {
                // Output channel name
                MString name;
                status = plug.getValue( name, MDGContext::fsNormal );
                this->setNodeInputSocketName( nodeID, 0, name.asChar() );

            } else if( nodeType == _T("InputValue") ) {
                // Input value value
                // This code partially copied from maya_magma_mel_interface.cpp
                MStatus stat;
                MObject node = plug.node();
                MFnDependencyNode depNode( node, &stat );
                MPlug magmaPlug = depNode.findPlug( "inMayaMagma", &stat );

                MObject descMpxData;
                stat = magmaPlug.getValue( descMpxData );
                MFnPluginData fnData( descMpxData, &stat );
                frantic::magma::maya::maya_magma_desc_mpxdata* pdescMPxData =
                    static_cast<frantic::magma::maya::maya_magma_desc_mpxdata*>( fnData.data( &stat ) );

                if( pdescMPxData ) {
                    // TODO: simplify and merge all the cases together if needed for clarity
                    frantic::magma::maya::desc::maya_magma_desc_ptr desc = pdescMPxData->get_maya_magma_desc();

                    frantic::tsstream::tostringstream str;
                    bool intValue = ( propertyName == _T("iValue") );
                    bool fltValue = ( propertyName == _T("fValue") );
                    bool vecValue = ( propertyName == _T("vec3Value") );
                    bool enmValue = ( propertyName == _T("enumValue") );
                    if( intValue || fltValue || vecValue ) {

                        bool changeLabel = false;
                        frantic::tstring propertyAttrName =
                            desc->get_maya_attr_name_from_node_property( nodeID, _T("enumValue") );
                        MPlug propertyAttrPlug = depNode.findPlug( propertyAttrName.c_str(), &status );

                        int currentSelection;
                        status = propertyAttrPlug.getValue( currentSelection, MDGContext::fsNormal );
                        if( currentSelection == 2 ) {
                            if( intValue ) {
                                int currentValue;
                                status = plug.getValue( currentValue, MDGContext::fsNormal );
                                str << currentValue;
                                changeLabel = true;
                            }
                        } else if( currentSelection == 1 ) {
                            if( fltValue ) {
                                float currentValue;
                                status = plug.getValue( currentValue, MDGContext::fsNormal );
                                str << currentValue << "f";
                                changeLabel = true;
                            }
                        } else if( currentSelection == 3 ) {
                            if( vecValue ) {
                                // We need to get all 3 values.  The attribute update event just gives us one of the
                                // values
                                frantic::tstring propertyAttrName =
                                    desc->get_maya_attr_name_from_node_property( nodeID, _T("vec3Value") );
                                MPlug propertyPlug = depNode.findPlug( propertyAttrName.c_str(), &status );
                                MDataHandle currentValue;
                                status = propertyPlug.getValue( currentValue, MDGContext::fsNormal );
                                MFloatVector currentValueVec = currentValue.asFloatVector();
                                str.precision( 3 );
                                str << "(" << currentValueVec.x << "," << currentValueVec.y << "," << currentValueVec.z
                                    << ")";
                                changeLabel = true;
                            }
                        } else {
                            str << "Unknown";
                            changeLabel = true;
                        }
                        if( changeLabel )
                            this->setNodeOutputSocketName( nodeID, 0,
                                                           frantic::strings::to_string( str.str() ).c_str() );

                    } else if( enmValue ) {
                        int currentSelection = 0;
                        status = plug.getValue( currentSelection, MDGContext::fsNormal );

                        if( currentSelection == 2 ) {
                            frantic::tstring propertyAttrName =
                                desc->get_maya_attr_name_from_node_property( nodeID, _T("iValue") );
                            MPlug propertyAttrPlug = depNode.findPlug( propertyAttrName.c_str(), &status );
                            int currentValue = 0;
                            status = propertyAttrPlug.getValue( currentValue, MDGContext::fsNormal );
                            str << currentValue;

                        } else if( currentSelection == 1 ) {
                            frantic::tstring propertyAttrName =
                                desc->get_maya_attr_name_from_node_property( nodeID, _T("fValue") );
                            MPlug propertyAttrPlug = depNode.findPlug( propertyAttrName.c_str(), &status );
                            float currentValue = 0;
                            status = propertyAttrPlug.getValue( currentValue, MDGContext::fsNormal );
                            str << currentValue << "f";

                        } else if( currentSelection == 3 ) {

                            frantic::tstring propertyAttrName =
                                desc->get_maya_attr_name_from_node_property( nodeID, _T("vec3Value") );
                            MPlug propertyAttrPlug = depNode.findPlug( propertyAttrName.c_str(), &status );
                            MDataHandle currentValue;
                            status = propertyAttrPlug.getValue( currentValue, MDGContext::fsNormal );
                            MFloatVector currentValueVec = currentValue.asFloatVector();
                            str.precision( 3 );
                            str << "(" << currentValueVec.x << "," << currentValueVec.y << "," << currentValueVec.z
                                << ")";

                        } else {
                            str << "Unknown";
                        }
                        this->setNodeOutputSocketName( nodeID, 0, frantic::strings::to_string( str.str() ).c_str() );
                    }
                }

            } else if( propertyName == _T("channels") || propertyName == _T("properties") ) {
                MObject arraydataobject;
                status = plug.getValue( arraydataobject );
                MFnStringArrayData arraydata( arraydataobject, &status );
                MStringArray output = arraydata.array( &status );

                std::vector<frantic::tstring> outValue = frantic::maya::from_maya_t( output );

                if( propertyName == _T("channels") &&
                    ( nodeType == _T("VertexQuery") || nodeType == _T("FaceQuery") ) ) {
                    // Deal with "exposePosition" settings
                    MStatus stat;
                    MObject node = plug.node();
                    MFnDependencyNode depNode( node, &stat );
                    MPlug magmaPlug = depNode.findPlug( "inMayaMagma", &stat );

                    MObject descMpxData;
                    stat = magmaPlug.getValue( descMpxData );
                    MFnPluginData fnData( descMpxData, &stat );
                    frantic::magma::maya::maya_magma_desc_mpxdata* pdescMPxData =
                        static_cast<frantic::magma::maya::maya_magma_desc_mpxdata*>( fnData.data( &stat ) );

                    frantic::magma::maya::desc::maya_magma_desc_ptr desc = pdescMPxData->get_maya_magma_desc();
                    frantic::tstring propertyAttrName =
                        desc->get_maya_attr_name_from_node_property( nodeID, _T("exposePosition") );
                    MPlug propertyAttrPlug = depNode.findPlug( propertyAttrName.c_str(), &status );

                    bool enabled;
                    status = propertyAttrPlug.getValue( enabled, MDGContext::fsNormal );
                    if( status == MS::kSuccess ) {
                        if( enabled ) {
                            outValue.insert( outValue.begin(), _T("Position") );
                        }
                    }
                }

                this->setNodeExtraOutputSockets( nodeID, outValue, false );

            } else if( propertyName == _T("exposePosition") ) {

                bool enabled;
                status = plug.getValue( enabled, MDGContext::fsNormal );

                // Get the other channels
                MStatus stat;
                MObject node = plug.node();
                MFnDependencyNode depNode( node, &stat );
                MPlug magmaPlug = depNode.findPlug( "inMayaMagma", &stat );

                MObject descMpxData;
                stat = magmaPlug.getValue( descMpxData );
                MFnPluginData fnData( descMpxData, &stat );
                frantic::magma::maya::maya_magma_desc_mpxdata* pdescMPxData =
                    static_cast<frantic::magma::maya::maya_magma_desc_mpxdata*>( fnData.data( &stat ) );

                frantic::magma::maya::desc::maya_magma_desc_ptr desc = pdescMPxData->get_maya_magma_desc();
                frantic::tstring propertyAttrName =
                    desc->get_maya_attr_name_from_node_property( nodeID, _T("channels") );
                MPlug propertyAttrPlug = depNode.findPlug( propertyAttrName.c_str(), &status );

                MObject arraydataobject;
                status = propertyAttrPlug.getValue( arraydataobject );
                MFnStringArrayData arraydata( arraydataobject, &status );
                MStringArray output = arraydata.array( &status );

                std::vector<frantic::tstring> outValue = frantic::maya::from_maya_t( output );

                if( status == MS::kSuccess ) {
                    if( enabled ) {
                        outValue.insert( outValue.begin(), _T("Position") );
                    }
                }

                this->setNodeExtraOutputSockets( nodeID, outValue, true );

            } else if( nodeType == _T("Mux") && propertyName == _T("NumInputs") ) {

                int numInputs;
                status = plug.getValue( numInputs, MDGContext::fsNormal );

                std::vector<frantic::tstring> inputs;
                for( int i = 0; i < numInputs - 1; i++ ) {
                    frantic::tsstream::tostringstream str;
                    str << "Input " << i;
                    inputs.push_back( str.str() );
                }
                inputs.push_back( _T("Selector") );

                this->setNodeExtraInputSockets( nodeID, inputs, true );
            }

        } catch( frantic::magma::maya::maya_magma_exception& ) {
            // It's possible to trigger this when not all the properties have been set up yet.  Just ignore it
        }
    }
}

} // namespace maya
} // namespace magma
} // namespace frantic
