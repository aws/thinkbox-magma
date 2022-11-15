// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef Q_MOC_RUN
// There's a bug with Qt and Boost that may cause compiler errors while generating the moc files
// Thus ignore them when generating the moc file
#include "frantic/magma/maya/maya_magma_datatypes.hpp"
#include "frantic/magma/maya/maya_magma_description.hpp"
#endif

#include "NodeView/connection.h"
#include "NodeView/node.h"
#include "NodeView/nodeview.h"
#include <map>
#include <set>

#include <QAction>
#include <QWidget>

// Qt and Maya do not work well together in Linux so just declare that we need this class rather
// than include the header file
#if !( __APPLE__ && MAYA_API_VERSION >= 201800 )
class MFnDependencyNode;
#endif

namespace frantic {
namespace magma {
namespace maya {

class maya_MagmaFLUX : public NodeView {
    Q_OBJECT

  public:
    // Helper Class for storing an edge information (which 2 nodes are connected and which sockets)
    struct connection_data {
        int m_source;
        int m_source_index;
        int m_destination;
        int m_destination_index;

        connection_data();
        connection_data( int src, int srcSocket, int dst, int dstSocket );
        connection_data( frantic::tstring fromString );

        bool operator==( const connection_data& rhs ) const;
        bool operator!=( const connection_data& rhs ) const;
        bool operator>=( const connection_data& rhs ) const;
        bool operator<=( const connection_data& rhs ) const;
        bool operator>( const connection_data& rhs ) const;
        bool operator<( const connection_data& rhs ) const;

        std::string toStdString( char separator = ',' ) const;
    };

  private:
    // Helper Class for storing information about a node that should affect its appearance
    struct node_meta_data {
        unsigned char m_rgba[4];
        unsigned char m_title_rgba[4];
        unsigned char m_outline_rgba[4];
        unsigned char m_title_text_rgba[4];

        int m_base_output_socket_count;
        int m_base_input_socket_count;
        frantic::tstring m_type;

        node_meta_data();
        node_meta_data( Node* node, const frantic::tstring& type, int inputCount, int outputCount );

        void setNodeColor( const QColor& color );
        QColor getNodeColor() const;
        void setTitleColor( const QColor& color );
        QColor getTitleColor() const;
        void setOutlineColor( const QColor& color );
        QColor getOutlineColor() const;
        void setTitleTextColor( const QColor& color );
        QColor getTitleTextColor() const;
    };

    // Helper Class for dealing with menus
    class MagmaFLUXQAction : public QAction {

      private:
        frantic::tstring m_command;
        std::vector<frantic::tstring> m_path;

      public:
        MagmaFLUXQAction( QString& text, QObject* parent, const std::vector<frantic::tstring>& path,
                          const frantic::tstring& command );
        virtual ~MagmaFLUXQAction();
        bool runCommand( const QPoint& position, const MString& nodeName, MString& outResult );
    };

    // Helper Class for dealing with menus
    class MenuLayout {
        typedef boost::shared_ptr<MenuLayout> MenuLayout_ptr;

      private:
        frantic::tstring m_name;
        frantic::tstring m_command;
        std::vector<MenuLayout_ptr> m_submenu_items;
        bool m_menuHasChanged;
        QMenu* m_qmenu;
        QWidget* m_parent;

      private:
        MenuLayout_ptr getSubmenu( const frantic::tstring& item, bool autocreate = true );
        QMenu* createMenuDetail( std::vector<frantic::tstring>& path );

      public:
        MenuLayout( const frantic::tstring& m_name, QWidget* parent = NULL );
        ~MenuLayout();

        QMenu* getQMenu();
        bool insert( const std::vector<frantic::tstring>& path, const frantic::tstring& command, int index = 0 );
        void clear();
    };

  private:
    void deleteConnection( Connection* conn );
    void deleteNodeInformation( int id );
    void updateNodeAppearance( int id );

  private slots:
    void onConnectionCreated( Socket* sock );
    void onConnectionToBeDestroyed( QObject* obj = NULL );
    void onConnectedToEmpty( Socket* socket, QPointF pos );

    void onNodeAdded( Node* node );
    void onNodeToBeDestroyed( QObject* obj = NULL );

    void onSelectionChanged();
    void onMousePress( QMouseEvent* event );
    void onMouseRelease( QMouseEvent* event );

    void onRightClickShowMenu( const QPoint& point );

  public:
    maya_MagmaFLUX( MString& window, MString& node, frantic::magma::maya::desc::maya_magma_desc_ptr currentDescription,
                    MFnDependencyNode& mayaNode, QWidget* parent = NULL );
    ~maya_MagmaFLUX();

    Node* findNode( int id ) const;
    Node* addNode( int id, const char* type, const char* name, int xPosition = 0, int yPosition = 0 );
    Node* addNode( int id, frantic::magma::maya::desc::maya_magma_desc_ptr currentDescription,
                   MFnDependencyNode& mayaNode );
    bool removeNode( int id );

    Connection* findConnection( int srcID, int srcSocketIndex, int dstID, int dstSocketIndex ) const;
    bool addConnection( int srcID, int srcSocketIndex, int dstID, int dstSocketIndex );
    bool removeConnection( int srcID, int srcSocketIndex, int dstID, int dstSocketIndex );

    std::vector<int> getSelectedNodes();
    std::vector<connection_data> getSelectedEdges();
    void setSelection( std::vector<int>& nodes, std::vector<connection_data>& edges );

    void setNodePosition( int id, int xPosition, int yPosition );

    void setNodeError( int id, bool isError = true );

    void setNodeColor( int id, const unsigned char rgba[4] );
    void setNodeTitleColor( int id, const unsigned char rgba[4] );
    void setNodeOutlineColor( int id, const unsigned char rgba[4] );
    void setNodeTitleTextColor( int id, const unsigned char rgba[4] );

    void setNodeSocketColor( int id, int socket, bool isOutput, const unsigned char rgba[4] );
    void setNodeSocketTextColor( int id, int socket, bool isOutput, const unsigned char rgba[4] );
    void setNodeSocketOutlineColor( int id, int socket, bool isOutput, const unsigned char rgba[4] );

    void setConnectionColor( int srcID, int srcSocketIndex, int dstID, int dstSocketIndex,
                             const unsigned char rgba[4] );

    bool getNodeColor( int id, unsigned char rgba[4] ) const;
    bool getNodeTitleColor( int id, unsigned char rgba[4] ) const;
    bool getNodeOutlineColor( int id, unsigned char rgba[4] ) const;
    bool getNodeTitleTextColor( int id, unsigned char rgba[4] ) const;
    bool getNodeConnectionColor( int id, unsigned char rgba[4] ) const;

    bool getNodeSocketColor( int id, int socket, bool isOutput, unsigned char rgba[4] ) const;
    bool getNodeSocketTextColor( int id, int socket, bool isOutput, unsigned char rgba[4] ) const;
    bool getNodeSocketOutlineColor( int id, int socket, bool isOutput, unsigned char rgba[4] ) const;

    bool getConnectionColor( int srcID, int srcSocketIndex, int dstID, int dstSocketIndex,
                             unsigned char rgba[4] ) const;

    void setNodeExtraOutputSockets( int id, const std::vector<frantic::tstring>& socketNames, bool tryShift );
    void setNodeExtraInputSockets( int id, const std::vector<frantic::tstring>& socketNames, bool tryShift );

    void clearNodeErrors();

    MString getWindowName() const;
    MString getMayaNodeName() const;

    void setNodeOutputSocketName( int id, int index, QString label );
    void setNodeInputSocketName( int id, int index, QString label );
    void setNodeName( int id, QString label );

    void getLastNodePosition( int& outX, int& outY ) const;
    void setLastNodePosition( int x, int y );

    void onAttributeChangedUpdateNodesInWindow( MPlug& plug );

    MString showRightClickMenu( int x, int y );
    void addRightClickMenu( const std::vector<frantic::tstring>& menuPath, frantic::tstring menuCommand );
    void clearRightClickMenu();

  protected:
    // Override
    void keyPressEvent( QKeyEvent* event );

  private:
    frantic::magma::maya::desc::maya_magma_desc_ptr m_description;

    MString m_WindowName;
    MString m_MayaNodeName;
    MObject m_MayaObject;

    bool m_EnableForwardMelCommand;
    frantic::tstring
        m_melCommandOnNodeSelectionChanged; // function( string $windowName, string $nodeName, int $selectedIDs[] )
    frantic::tstring m_melCommandOnConnectionSelectionChanged; // function( string $windowName, string $nodeName, string
                                                               // $selectedEdgesTuple[] ) "src,srcSocket,dst,dstSocket"
    frantic::tstring m_melCommandOnConnectionCreated; // function( string $windowName, string $nodeName, int $srcID, int
                                                      // $srcSocketIndex, int $dstID, int $dstSocketIndex )
    frantic::tstring m_melCommandOnConnectionDeleted; // function( string $windowName, string $nodeName, int $srcID, int
                                                      // $srcSocketIndex, int $dstID, int $dstSocketIndex )
    frantic::tstring m_melCommandOnKeyboardPressed; // function( string $windowName, string $nodeName, int $keyCode,
                                                    // bool $shift, bool $control, bool $alt )
    frantic::tstring m_melCommandOnConnectionToEmpty; // function( string $windowName, string $nodeName, int $releaseX,
                                                      // int $releaseY, int $nodeID, int $socketID, bool $isOutput )

    std::map<Node*, int> m_NodeToID;
    std::map<int, Node*> m_IDToNode;

    std::map<int, node_meta_data> m_IDToMetaInfo;
    std::set<int> m_IDToIsError;

    std::map<connection_data, Connection*> m_ConnectionDataToConnection;
    std::map<Connection*, connection_data> m_ConnectionToConnectionData;

    int m_lastNodeX;
    int m_lastNodeY;

    int m_lastMousePressX;
    int m_lastMousePressY;

    MenuLayout m_rightClickMenu;
};

} // namespace maya
} // namespace magma
} // namespace frantic
