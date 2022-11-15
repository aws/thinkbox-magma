// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_node_base.hpp>

namespace frantic {
namespace magma {
namespace nodes {

template <int N, class BaseType = frantic::magma::magma_node_base>
class magma_simple_operator : public BaseType {
    std::pair<magma_interface::magma_id, int> m_inputs[N];
    variant_t m_defaults[N];

  public:
    magma_simple_operator() {
        for( int i = 0; i < N; ++i ) {
            m_inputs[i].first = magma_interface::INVALID_ID;
            m_inputs[i].second = 0;
        }
    }

    virtual ~magma_simple_operator() {}

    virtual int get_num_inputs() const { return N; }
    virtual void set_num_inputs( int /*numInputs*/ ) {}
    virtual std::pair<magma_interface::magma_id, int> get_input( int index ) const { return m_inputs[index]; }
    virtual void set_input( int index, magma_interface::magma_id id, int outputIndex ) {
        m_inputs[index].first = id;
        m_inputs[index].second = outputIndex;
    }

    virtual const variant_t& get_input_default_value( int i ) const { return m_defaults[i]; }
    virtual void set_input_default_value( int i, const variant_t& value ) { m_defaults[i] = value; }
};

template <class BaseType>
class magma_simple_operator<0, BaseType> : public BaseType {
  public:
    virtual ~magma_simple_operator() {}

    virtual int get_num_inputs() const { return 0; }
    virtual void set_num_inputs( int /*numInputs*/ ) {}
    virtual std::pair<magma_interface::magma_id, int> get_input( int /*index*/ ) const {
        return std::make_pair( magma_interface::INVALID_ID, 0 );
    }
    virtual void set_input( int /*index*/, magma_interface::magma_id /*id*/, int /*outputIndex*/ ) {}
};

typedef magma_simple_operator<0> magma_input_node;

} // namespace nodes
} // namespace magma
} // namespace frantic
