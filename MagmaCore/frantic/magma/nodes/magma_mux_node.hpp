// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_node_base.hpp>
#include <frantic/magma/nodes/magma_node_property.hpp>

#include <vector>

namespace frantic {
namespace magma {
namespace nodes {

class magma_mux_node : public magma_node_base {
    std::vector<std::pair<magma_interface::magma_id, int>> m_inputs;
    std::vector<variant_t> m_defaultValues;

  public:
    static void create_type_definition( magma_node_type& outType );

    virtual void compile( magma_compiler_interface& compile );

    magma_mux_node();

    virtual int get_num_inputs() const { return (int)m_inputs.size(); }

    virtual void set_num_inputs( int numInputs ) {
        m_inputs.resize( std::max( 3, numInputs ), std::make_pair( magma_interface::INVALID_ID, 0 ) );
        m_defaultValues.resize( m_inputs.size(), variant_t() );
    }

    virtual std::pair<magma_interface::magma_id, int> get_input( int index ) const { return m_inputs[index]; }

    virtual const variant_t& get_input_default_value( int i ) const { return m_defaultValues[i]; }

    virtual void set_input( int index, magma_interface::magma_id id, int socketIndex ) {
        m_inputs[index].first = id;
        m_inputs[index].second = socketIndex;
    }

    virtual void set_input_default_value( int i, const variant_t& value ) { m_defaultValues[i] = value; }

    virtual void get_input_description( int i, frantic::tstring& outDescription ) {
        if( i == (int)m_inputs.size() - 1 )
            outDescription = _T("Selector");
        else
            outDescription = _T("Input ") + boost::lexical_cast<frantic::tstring>( i );
    }

    virtual bool get_input_visitable( int /*i*/ ) const {
        // if( m_inputs.size() > 4 && i != (int)m_inputs.size() - 1 )
        //	return false;
        return true;
    }
};

} // namespace nodes
} // namespace magma
} // namespace frantic
