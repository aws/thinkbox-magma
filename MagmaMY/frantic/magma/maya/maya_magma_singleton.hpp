// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "frantic/magma/maya/maya_magma_common.hpp"
#include <frantic/magma/magma_singleton.hpp>

namespace frantic {
namespace magma {
namespace maya {
namespace modifier {

class maya_magma_singleton : public frantic::magma::magma_singleton {
    maya_magma_singleton();

    virtual ~maya_magma_singleton() {}

  public:
    static maya_magma_singleton& get_instance();
};

} // namespace modifier
} // namespace maya
} // namespace magma
} // namespace frantic
