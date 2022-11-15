// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <maya/MStatus.h>
#include <maya/MString.h>

#include "frantic/magma/maya/maya_magma_common.hpp"
#include <frantic/logging/logging_level.hpp>

namespace frantic {
namespace magma {
namespace maya {

class maya_magma_exception : public std::runtime_error {
  public:
    explicit maya_magma_exception( const char* inVal );

    explicit maya_magma_exception( const frantic::tstring& inVal );
};

inline void throw_maya_magma_exception_if_not_success( const MStatus& inStatus, const frantic::tstring& inMessage ) {
    if( inStatus != MStatus::kSuccess ) {
#if defined( FRANTIC_USE_WCHAR )
        FF_LOG( stats ) << "DEBUG:" << inMessage << ":" << inStatus.errorString().asWChar() << std::endl;
        throw maya_magma_exception( inMessage + inStatus.errorString().asWChar() );
#else
        FF_LOG( stats ) << "DEBUG:" << inMessage << ":" << inStatus.errorString().asChar() << std::endl;
        throw maya_magma_exception( inMessage + inStatus.errorString().asChar() );
#endif
    }
}

} // namespace maya
} // namespace magma
} // namespace frantic
