// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <maya/MArgList.h>
#include <maya/MIOStream.h>
#include <maya/MPxCommand.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include "frantic/magma/maya/maya_magma_mel_interface.hpp"

namespace frantic {
namespace magma {
namespace maya {
namespace mel {

class maya_magma_mel_create_connection : public maya_magma_mel_interface {
  public:
    static const MString commandName;

  public:
    static void* creator();

  private:
    MStatus parseArgs( const MArgList& args, frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                       MFnDependencyNode& /*depNode*/ );
};

////////////////////////////////////////////////////////////////////////////////

class maya_magma_mel_delete_connection : public maya_magma_mel_interface {
  public:
    static const MString commandName;

  public:
    static void* creator();

  private:
    MStatus parseArgs( const MArgList& args, frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                       MFnDependencyNode& /*depNode*/ );
};

////////////////////////////////////////////////////////////////////////////////

class maya_magma_mel_get_connections : public maya_magma_mel_interface {
  public:
    static const MString commandName;

  public:
    static void* creator();

  private:
    MStatus parseArgs( const MArgList& args, frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                       MFnDependencyNode& /*depNode*/ );
};

////////////////////////////////////////////////////////////////////////////////

class maya_magma_mel_set_connection_properties : public maya_magma_mel_interface {
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
