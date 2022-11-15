// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/magma_node_type.hpp>
#include <frantic/magma/nodes/magma_node_impl.hpp>

#include <memory>

namespace frantic {
namespace magma {

magma_node_type_impl::magma_node_type_impl( magma_singleton& singleton )
    : m_creatorFn( NULL )
    , m_singleton( &singleton )
    , m_numHiddenProps( 0 )
    , m_isPublic( true )
    , m_isContainer( false ) {}

magma_node_type_impl::~magma_node_type_impl() {
    for( std::vector<property_meta_interface*>::iterator it = m_props.begin(), itEnd = m_props.end(); it != itEnd;
         ++it )
        delete *it;
    for( std::vector<input_meta_interface*>::iterator it = m_inputs.begin(), itEnd = m_inputs.end(); it != itEnd; ++it )
        delete *it;
    for( std::vector<output_meta_interface*>::iterator it = m_outputs.begin(), itEnd = m_outputs.end(); it != itEnd;
         ++it )
        delete *it;
}

void magma_node_type_impl::add_property( std::unique_ptr<property_meta_interface> propMeta, bool isVisible ) {
    m_propNameToIndices[propMeta->get_name()] = (int)m_props.size();
    m_props.push_back( propMeta.release() );

    if( isVisible && m_numHiddenProps > 0 ) {
        std::size_t target = m_props.size() - m_numHiddenProps - 1;
        std::swap( m_propNameToIndices[m_props.back()->get_name()], m_propNameToIndices[m_props[target]->get_name()] );
        std::swap( m_props.back(), m_props[target] );
    } else if( !isVisible )
        ++m_numHiddenProps;
}

magma_node_base* magma_node_type_impl::create_instance( magma_interface::magma_id id ) const {
    std::unique_ptr<magma_node_base> result( m_creatorFn() );
    result->m_type = this;
    result->m_id = id;

    // Initialize the default values for the input sockets.
    for( int i = 0, iEnd = (int)m_inputs.size(); i < iEnd; ++i ) {
        const variant_t& val = m_inputs[i]->get_default_value();
        if( val.type() != typeid( boost::blank ) )
            result->set_input_default_value( i, val );
    }

    return result.release();
}

void magma_node_type_impl::set_disableable( bool isDisableable ) {
    std::map<frantic::tstring, std::size_t>::iterator it = m_propNameToIndices.find( _T("enabled") );
    if( it != m_propNameToIndices.end() ) {
        if( frantic::magma::property_meta<bool, magma_node_base>* pProp =
                dynamic_cast<frantic::magma::property_meta<bool, magma_node_base>*>( m_props[it->second] ) )
            pProp->m_setter = isDisableable ? &magma_node_base::set_enabled : NULL;
    }
}

const property_meta_interface* magma_node_type_impl::get_property_by_name( const frantic::tstring& propName ) const {
    std::map<frantic::tstring, std::size_t>::const_iterator it = m_propNameToIndices.find( propName );
    if( it == m_propNameToIndices.end() )
        return NULL;
    return m_props[it->second];
}

} // namespace magma
} // namespace frantic
