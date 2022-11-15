// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/magma_interface.hpp>

#include "frantic/magma/maya/maya_magma_datatypes.hpp"

namespace frantic {
namespace magma {
namespace maya {

namespace desc {

const desc_id kInvalidDescID = -1;
const desc_index_type kInvalidDescSocketIndex = -1;
const frantic::tstring kInvalidNodeType( _T( "INVALID_NODE_TYPE" ) );
const frantic::tstring kInvalidEnumAttrName( _T( "INVALID_MAYA_ENUM_ATTR_NAME" ) );

} // namespace desc

namespace holder {

const magma_id kInvalidMagmaID = magma_interface::INVALID_ID;

}

} // namespace maya
} // namespace magma
} // namespace frantic
