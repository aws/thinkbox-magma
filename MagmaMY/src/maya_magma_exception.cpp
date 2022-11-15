// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "frantic/magma/maya/maya_magma_exception.hpp"

namespace frantic {
namespace magma {
namespace maya {

maya_magma_exception::maya_magma_exception( const char* inVal )
    : std::runtime_error( inVal ) {}

maya_magma_exception::maya_magma_exception( const frantic::tstring& inVal )
    : std::runtime_error( frantic::strings::to_string( inVal ).c_str() ) {}

} // namespace maya
} // namespace magma
} // namespace frantic
