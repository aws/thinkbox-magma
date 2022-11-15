// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/nodes/magma_loop_node.hpp>

namespace frantic {
namespace magma {
namespace nodes {

class magma_foreach_particle_node : public magma_loop_node {
    std::pair<magma_interface::magma_id, int> m_geomInput, m_posInput, m_radiusOrCountInput;
    variant_t m_geomDefault, m_posDefault, m_radiusDefault, m_countDefault;
    std::vector<frantic::tstring> m_loopChannels;

  protected:
    virtual void do_clone( magma_node_base& dest, clone_callback& cb ) const;

  public:
    MAGMA_REQUIRED_METHODS( magma_foreach_particle_node );
    MAGMA_PROPERTY( searchMode, frantic::tstring );

    static const frantic::tstring& get_uiType() {
        static frantic::tstring typeString( _T("Loop") );
        return typeString;
    }
    static const int get_numControlInputs() { return 3; }
    const std::vector<frantic::tstring>& get_loopChannels() const;

    magma_foreach_particle_node();

    virtual int get_num_inputs() const;

    virtual std::pair<magma_interface::magma_id, int> get_input( int index ) const;

    virtual const variant_t& get_input_default_value( int i ) const;

    virtual void get_input_description( int i, frantic::tstring& outDescription );

    virtual void get_output_description( int i, frantic::tstring& outDescription ) const;

    virtual void set_input( int index, magma_interface::magma_id id, int socketIndex );

    virtual void set_input_default_value( int i, const variant_t& value );

    virtual void compile_as_extension_type( magma_compiler_interface& compiler );
};

class magma_foreach_particle_inputs_node : public magma_loop_inputs_node {
  public:
    MAGMA_REQUIRED_METHODS( magma_foreach_particle_inputs_node );

    magma_foreach_particle_inputs_node();
};

class magma_foreach_particle_outputs_node : public magma_loop_outputs_node {
  public:
    MAGMA_REQUIRED_METHODS( magma_foreach_particle_outputs_node );

    magma_foreach_particle_outputs_node();
};

} // namespace nodes
} // namespace magma
} // namespace frantic
