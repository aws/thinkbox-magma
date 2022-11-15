// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_exception.hpp>
#include <frantic/magma/magma_node_base.hpp>
#include <vector>

namespace frantic {
namespace magma {
namespace nodes {

class magma_missing_node : public magma_node_base {
    std::vector<std::pair<magma_interface::magma_id, int>> m_inputs;
    std::size_t m_numOutputs;

  public:
    static void create_type_definition( magma_node_type& outType );

    virtual void compile( magma_compiler_interface& compiler );

    magma_missing_node()
        : m_numOutputs( 0 ) {}

    virtual ~magma_missing_node() {}

    virtual int get_num_inputs() const { return (int)m_inputs.size(); }

    virtual void set_num_inputs( int numInputs ) {
        m_inputs.resize( numInputs, std::make_pair( magma_interface::INVALID_ID, 0 ) );
    }

    virtual std::pair<magma_interface::magma_id, int> get_input( int i ) const { return m_inputs[i]; }

    virtual void set_input( int i, magma_interface::magma_id id, int outputIndex = 0 ) {
        m_inputs[i].first = id;
        m_inputs[i].second = outputIndex;
    }

    virtual int get_num_outputs() const { return (int)m_numOutputs; }

    virtual void set_num_outputs( int numOutputs ) { m_numOutputs = (std::size_t)numOutputs; }

    virtual void compile_as_extension_type( magma_compiler_interface& /*compiler*/ ) {
        throw magma_exception() << magma_exception::node_id( get_id() )
                                << magma_exception::error_name(
                                       _T("This node failed to load from disk. Please fix it.") );
    }
};

} // namespace nodes
} // namespace magma
} // namespace frantic
