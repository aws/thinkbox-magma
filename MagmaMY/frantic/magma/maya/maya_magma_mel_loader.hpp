// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "frantic/magma/maya/maya_magma_mel_interface.hpp"

#include <map>
#include <vector>

namespace frantic {
namespace magma {
namespace maya {
namespace mel {

class maya_magma_mel_load_setup : public maya_magma_mel_interface {
  public:
    static const MString commandName;

  public:
    static void* creator();

  private:
    MStatus parseArgs( const MArgList& args, frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                       MFnDependencyNode& depNode );
};

class maya_magma_mel_apply_setup : public maya_magma_mel_interface {
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
