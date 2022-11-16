// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/max3d/MagmaMaxContext.hpp>

#include <frantic/magma/max3d/nodes/magma_script_op_node.hpp>
#include <frantic/magma/nodes/magma_node_impl.hpp>

#include <frantic/max3d/maxscript/maxscript.hpp>
#include <frantic/max3d/particles/IMaxKrakatoaPRTObject.hpp>

//#include <boost/thread/mutex.hpp>

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

MSTR magma_script_op_node::max_impl::s_ClassName( _T("MagmaScriptNode") );
// Class_ID magma_script_op_node::max_impl::s_ClassID(); //This needs to be defined in the DLL, not in this project
// because it must be different for Krakatoa, Genome, etc.

void magma_script_op_node::max_impl::DefineParameters( ParamBlockDesc2& /*paramDesc*/ ) {}

RefResult magma_script_op_node::max_impl::NotifyRefChanged( const Interval& /*changeInt*/, RefTargetHandle /*hTarget*/,
                                                            PartID& /*partID*/, RefMessage /*message*/,
                                                            BOOL /*propagate*/ ) {
    return REF_DONTCARE;
}

Interval magma_script_op_node::max_impl::get_validity( TimeValue t ) const { return Interval( t, t ); }

namespace {
// using frantic::magma::simple_compiler::detail::temporary_result;

// template <class T>
// struct script_op : frantic::magma::simple_compiler::detail::magma_opaque{
//	boost::scoped_array<temporary_result> inputs;
//	temporary_result output;
//	int numInputs;
//
//	//boost::mutex m_evalLock;

//	input_op( boost::scoped_array<int>& _inputs,temporary_result _output, int _numInputs ){
//		inputs.swap(_inputs);
//		output = _output;
//		numInputs = _numInputs;
//	}

//	static void execute( char* stack, void*, frantic::magma::simple_compiler::detail::magma_opaque* opaqueSelf ){
//		script_op& self = *reinterpret_cast<script_op*>(opaqueSelf);
//
//		T* dest = reinterpret_cast<T*>( stack + self.output.offset );
//
//	}
//};
} // namespace

using frantic::magma::magma_exception;

void magma_script_op_node::compile_as_extension_type( frantic::magma::magma_compiler_interface& compiler ) {
    TimeValue t = TIME_NegInfinity;
    compiler.get_context_data().get_property( _T("Time"), t );

    Value* val = frantic::max3d::mxs::expression( _T("try(") + get_script() + _T(")catch(undefined)") )
                     .at_time( t )
                     .evaluate<Value*>();

    if( !val || val == &undefined )
        throw magma_exception() << magma_exception::node_id( get_id() )
                                << magma_exception::error_name(
                                       _T("Cannot convert script result (undefined) to a Magma data type") );

    if( is_bool( val ) ) {
        compiler.compile_constant( get_id(), variant_t( val->to_bool() ) );
    } else if( is_integer( val ) ) {
        compiler.compile_constant( get_id(), variant_t( val->to_int() ) );
    } else if( is_number( val ) ) {
        compiler.compile_constant( get_id(), variant_t( val->to_float() ) );
    } else if( is_point3( val ) ) {
        compiler.compile_constant( get_id(), variant_t( frantic::max3d::from_max_t( val->to_point3() ) ) );
    } else if( is_color( val ) ) {
        compiler.compile_constant( get_id(), variant_t( frantic::max3d::from_max_t( (Point3)val->to_acolor() ) ) );
    } else if( is_quat( val ) ) {
        compiler.compile_constant( get_id(), variant_t( frantic::max3d::from_max_t( val->to_quat() ) ) );
    } else
        throw magma_exception() << magma_exception::node_id( get_id() )
                                << magma_exception::error_name( _T("Cannot convert script result (") +
                                                                frantic::max3d::mxs::to_string( val ) +
                                                                _T(") to a Magma data type") );
}

} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic

using frantic::magma::max3d::MagmaMaxNodeExtension;

template <>
MagmaMaxNodeExtension<frantic::magma::nodes::max3d::magma_script_op_node::max_impl>::MyClassDesc
    MagmaMaxNodeExtension<frantic::magma::nodes::max3d::magma_script_op_node::max_impl>::s_classDesc;

ClassDesc2* GetMagmaScriptNodeDesc() {
    return &frantic::magma::nodes::max3d::magma_script_op_node::max_impl::s_classDesc;
}
