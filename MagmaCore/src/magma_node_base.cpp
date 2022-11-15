// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/magma_exception.hpp>
#include <frantic/magma/magma_node_base.hpp>
#include <frantic/magma/magma_node_type.hpp>

#include <memory>

namespace frantic {
namespace magma {

void magma_node_base::get_input_description( int i, frantic::tstring& outDescription ) {
    if( i < m_type->get_num_inputs() )
        outDescription = m_type->get_input( i ).get_name();
    else
        outDescription = _T("Input ") + boost::lexical_cast<frantic::tstring>( i + 1 );
}

bool magma_node_base::get_input_visitable( int i ) const {
    return ( m_type && i >= 0 && i < m_type->get_num_inputs() ) ? m_type->get_input( i ).get_visitable() : true;
}

void magma_node_base::get_output_description( int i, frantic::tstring& outDescription ) const {
    if( i < m_type->get_num_outputs() )
        outDescription = m_type->get_output( i ).get_name();
    else if( get_num_outputs() == 1 )
        outDescription = m_type->get_name();
    else
        outDescription = m_type->get_name() + _T(" ") + boost::lexical_cast<frantic::tstring>( i + 1 );
}

void magma_node_base::do_clone( magma_node_base& dest, clone_callback& cb ) const {
    // Copy properties first because some properties affect a node's input count.
    for( int i = 0, iEnd = m_type->get_num_properties(); i < iEnd; ++i )
        m_type->get_property( i ).copy_value( &dest, this );

    dest.set_num_inputs( this->get_num_inputs() );
    dest.set_num_outputs( this->get_num_outputs() );

    for( int i = 0, iEnd = this->get_num_inputs(); i < iEnd; ++i ) {
        std::pair<magma_interface::magma_id, int> input = this->get_input( i );
        const variant_t& defaultValue = this->get_input_default_value( i );

        dest.set_input( i, input.first, input.second );
        if( defaultValue.type() != typeid( boost::blank ) ) // TODO Why is it not copying boost::blank default values?
            dest.set_input_default_value( i, defaultValue );
    }
}

magma_node_base* magma_node_base::clone( clone_callback& cb ) const {
    if( !m_type )
        THROW_MAGMA_INTERNAL_ERROR( this->get_id() );

    std::unique_ptr<magma_node_base> result( m_type->create_instance( get_id() ) );

    // TODO Should this be registered after cloning is complete?
    cb.register_clone( *result );

    this->do_clone( *result, cb );

    return result.release();
}

void magma_node_base::compile_as_extension_type( magma_compiler_interface& compiler ) {
    throw magma_exception() << magma_exception::node_id( get_id() )
                            << magma_exception::error_name( _T("**INTERNAL ERROR** ") + get_type().get_name() +
                                                            _T(" is not supported by this compiler") );
}

} // namespace magma
} // namespace frantic
