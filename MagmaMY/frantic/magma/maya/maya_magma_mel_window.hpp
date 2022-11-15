// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "frantic/magma/maya/maya_magma_mel_interface.hpp"

namespace frantic {
namespace magma {
namespace maya {
// In Linux, Maya and Qt do not play nicely with each other.  Rather than include
// frantic/magma/maya/maya_magma_gui.hpp, avoid it so the compiler does not have to
// deal with Qt unless it really has to.
class maya_MagmaFLUX;
} // namespace maya
} // namespace magma
} // namespace frantic

#include <map>
#include <vector>

namespace frantic {
namespace magma {
namespace maya {
namespace mel {

class maya_magma_mel_window : public maya_magma_mel_interface {
  public:
    static const MString commandName;

  private:
    static std::map<frantic::tstring, maya_MagmaFLUX*> m_WindowNameToMagmaFLUX;
    static std::map<frantic::tstring, maya_MagmaFLUX*> m_NodeNameToMagmaFLUX;

  public:
    static maya_MagmaFLUX* findMagmaFLUXWidgetByWindowName( const MString& windowName );
    static maya_MagmaFLUX* findMagmaFLUXWidgetByNode( const MFnDependencyNode& node );
    static maya_MagmaFLUX* findMagmaFLUXWidgetByNodeName( const MString& nodeName );
    static maya_MagmaFLUX* findMagmaFLUXWidgetByNodeName( const frantic::tstring& nodeName );
    static std::vector<maya_MagmaFLUX*> findAllMagmaFLUXWidgets();

    // Check the property change given in the plug and update the UI appropriately
    static void onAttributeChangedUpdateNodesInWindow( MPlug& plug );

    static void addErrorNode( const MObject& prtNode, int id );
    static void clearErrorNodes( const MObject& prtNode );

    static void removeMagmaFLUX( maya_MagmaFLUX* window );
    static void addMagmaFLUX( maya_MagmaFLUX* window );

    static void* creator();

  private:
    MStatus parseArgs( const MArgList& args, frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                       MFnDependencyNode& depNode );
};

////////////////////////////////////////////////////////////////////////////////

class maya_magma_mel_window_retrieve : public maya_magma_mel_interface // MPxCommand
{
  public:
    static const MString commandName;

  public:
    static void* creator();

  private:
    MStatus parseArgs( const MArgList& args, frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                       MFnDependencyNode& depNode );
};

////////////////////////////////////////////////////////////////////////////////

class maya_magma_mel_window_set : public maya_magma_mel_interface // MPxCommand
{
  public:
    static const MString commandName;

  public:
    static void* creator();

  private:
    MStatus parseArgs( const MArgList& args, frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                       MFnDependencyNode& depNode );
};

////////////////////////////////////////////////////////////////////////////////

class maya_magma_mel_window_menu : public maya_magma_mel_interface // MPxCommand
{
  public:
    static const MString commandName;

  public:
    static void* creator();

  private:
    MStatus parseArgs( const MArgList& args, frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                       MFnDependencyNode& depNode );
};

} // namespace mel
} // namespace maya
} // namespace magma
} // namespace frantic
