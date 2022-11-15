// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <maya/MArgList.h>
#include <maya/MString.h>

#include "frantic/magma/maya/maya_magma_mel_interface.hpp"

namespace frantic {
namespace magma {
namespace maya {
namespace mel {

class maya_magma_mel_create_node : public maya_magma_mel_interface {
  public:
    static const MString commandName;

  public:
    static void* creator();

  private:
    MStatus parseArgs( const MArgList& args, frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                       MFnDependencyNode& depNode );
};

////////////////////////////////////////////////////////////////////////////////

class maya_magma_mel_delete_node : public maya_magma_mel_interface {
  public:
    static const MString commandName;

  public:
    static void* creator();

  private:
    MStatus parseArgs( const MArgList& args, frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                       MFnDependencyNode& /*depNode*/ );
};

////////////////////////////////////////////////////////////////////////////////

class maya_magma_mel_retrieve_node_instance_info : public maya_magma_mel_interface {
  public:
    static const MString commandName;

  public:
    static void* creator();

  private:
    MStatus parseArgs( const MArgList& args, frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                       MFnDependencyNode& /*depNode*/ );
};

////////////////////////////////////////////////////////////////////////////////

class maya_magma_mel_set_node_instance_info : public maya_magma_mel_interface {
  public:
    static const MString commandName;

  public:
    static void* creator();

  private:
    MStatus parseArgs( const MArgList& args, frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                       MFnDependencyNode& /*depNode*/ );
};

////////////////////////////////////////////////////////////////////////////////

class maya_magma_mel_retrieve_node_ids : public maya_magma_mel_interface {
  public:
    static const MString commandName;

  public:
    static void* creator();

  private:
    MStatus parseArgs( const MArgList& args, frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                       MFnDependencyNode& /*depNode*/ );
};

////////////////////////////////////////////////////////////////////////////////

/// NOTE maya_magma_mel_retrieve_node_info does not inherit from maya_magma_mel_interface
/// because users can retrieve node_info without selecting PRTLoader object beforehand
class maya_magma_mel_retrieve_node_info : public MPxCommand {
  public:
    static const MString commandName;

  public:
    static void* creator();

  public:
    MStatus doIt( const MArgList& args );
};

} // namespace mel
} // namespace maya
} // namespace magma
} // namespace frantic
