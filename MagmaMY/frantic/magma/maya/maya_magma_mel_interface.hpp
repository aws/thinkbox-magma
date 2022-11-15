// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <boost/shared_ptr.hpp>

#include <maya/MArgList.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MIOStream.h>
#include <maya/MPxCommand.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include "frantic/magma/maya/maya_magma_description.hpp"

namespace frantic {
namespace magma {
namespace maya {
namespace mel {

///
///  Template Method Pattern
///
class maya_magma_mel_interface : public MPxCommand {
  public:
    virtual ~maya_magma_mel_interface() = 0;

    MStatus doIt( const MArgList& args );

  private:
    // TODO check do we support python command ? TODO change MFnDependencyNode to `const MFnDependencyNode &`
    virtual MStatus parseArgs( const MArgList& args, frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                               MFnDependencyNode& depNode ) = 0;
};

} // namespace mel
} // namespace maya
} // namespace magma
} // namespace frantic
