// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_data_type.hpp>
#include <frantic/magma/nodes/magma_node_property.hpp>
#include <frantic/magma/nodes/magma_simple_operator.hpp>

namespace frantic {
namespace magma {
namespace nodes {

class magma_output_node : public magma_simple_operator<1> {
    MAGMA_PROPERTY( channelName, frantic::tstring );
    MAGMA_PROPERTY( channelType, magma_data_type );

  public:
    static void create_type_definition( magma_node_type& outType );

    virtual void compile( magma_compiler_interface& compiler );

    magma_output_node();

    virtual int get_num_outputs() const { return 0; }
};

} // namespace nodes
} // namespace magma
} // namespace frantic
