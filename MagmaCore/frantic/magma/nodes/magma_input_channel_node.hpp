// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_data_type.hpp>
#include <frantic/magma/nodes/magma_node_property.hpp>
#include <frantic/magma/nodes/magma_simple_operator.hpp>

namespace frantic {
namespace magma {
namespace nodes {

class magma_input_channel_node : public nodes::magma_input_node {
    MAGMA_PROPERTY( channelName, frantic::tstring );
    MAGMA_PROPERTY( channelType, magma_data_type );

  public:
    magma_input_channel_node()
        : m_channelName( _T("<undefined>") )
        , m_channelType() {}

    static void create_type_definition( magma_node_type& outType );

    virtual void compile( magma_compiler_interface& compiler );

    virtual void get_output_description( int i, frantic::tstring& outDescription ) const {
        if( i == 0 )
            outDescription = get_channelName();
    }
};

} // namespace nodes
} // namespace magma
} // namespace frantic
