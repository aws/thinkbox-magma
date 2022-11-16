// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/max3d/MagmaShadeContext.hpp>
#include <frantic/magma/simple_compiler/base_compiler.hpp>

namespace frantic {
namespace magma {
namespace max3d {

class MagmaTexmapExpression : public simple_compiler::base_compiler::expression {
  public:
    struct result_type {
        enum enum_t { color, mono, perturb, error };

        static enum_t from_string( const frantic::tstring& val );
    };

  private:
    std::ptrdiff_t m_outPtr;
    std::ptrdiff_t m_inputs[6];

    Texmap* m_texmap;
    RenderGlobalContext* m_globContext;
    RenderInstance* m_rendInst;
    INode* m_node;
    Matrix3 m_toCamera, m_fromCamera;

    result_type::enum_t m_resultType;

    std::vector<std::pair<std::size_t, int>> m_inputBindings; // Maps from input socket index to map channel.
    std::vector<std::pair<int, std::ptrdiff_t>> m_uvwInfo;

    static void internal_apply( const simple_compiler::base_compiler::expression* _this,
                                simple_compiler::base_compiler::state& data );

  public:
    MagmaTexmapExpression( Texmap* theMap, RenderGlobalContext* globContext, RenderInstance* rendInst, INode* node,
                           bool inWorldSpace, result_type::enum_t resultType );

    virtual ~MagmaTexmapExpression();

    void bind_input_to_channel( std::size_t input, int mapChannel );

    virtual void set_input( std::size_t inputIndex, std::ptrdiff_t relPtr );

    virtual void set_output( std::ptrdiff_t relPtr );

    virtual const frantic::channels::channel_map& get_output_map() const;

    virtual void apply( simple_compiler::base_compiler::state& data ) const;

    virtual runtime_ptr get_runtime_ptr() const;
};

} // namespace max3d
} // namespace magma
} // namespace frantic
