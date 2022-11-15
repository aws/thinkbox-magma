// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <maya/MStringArray.h>

#include "frantic/maya/selection.hpp"
#include <frantic/logging/logging_level.hpp>

#include "frantic/magma/maya/maya_magma_description.hpp"
#include "frantic/magma/maya/maya_magma_mel_debug_desc.hpp"

namespace frantic {
namespace magma {
namespace maya {
namespace mel {

const MString maya_magma_mel_debug_desc::commandName = "debug_magma";

void* maya_magma_mel_debug_desc::creator() { return new maya_magma_mel_debug_desc; }

MStatus maya_magma_mel_debug_desc::parseArgs( const MArgList& args,
                                              frantic::magma::maya::desc::maya_magma_desc_ptr desc,
                                              MFnDependencyNode& /*depNode*/ ) {
    FF_LOG( debug ) << "maya_magma_mel_debug_desc::parseArgs:" << std::endl;

    MStatus outStatus = MS::kFailure;
    FF_LOG( stats ) << "current magma desc:" << desc->debug().c_str() << std::endl;
    setResult( desc->debug().c_str() );
    outStatus = MS::kSuccess;
    return outStatus;
}

} // namespace mel
} // namespace maya
} // namespace magma
} // namespace frantic
