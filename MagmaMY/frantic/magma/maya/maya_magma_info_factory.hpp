// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/strings/tstring.hpp>

#include "frantic/magma/maya/maya_magma_common.hpp"
#include "frantic/magma/maya/maya_magma_holder.hpp"
#include "frantic/magma/maya/maya_magma_info.hpp"

namespace frantic {
namespace magma {
namespace maya {
namespace factory {

class maya_magma_node_info_factory {
  public:
    ~maya_magma_node_info_factory() {}

    // this is factory method
    static info::maya_magma_node_info create_node_infos( const frantic::tstring& m_nodeType );

  private:
    maya_magma_node_info_factory() {}
};

} // namespace factory
} // namespace maya
} // namespace magma
} // namespace frantic
