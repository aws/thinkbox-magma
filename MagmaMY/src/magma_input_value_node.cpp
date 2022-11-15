// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/graphics/vector3f.hpp>
#include <frantic/logging/logging_level.hpp>
#include <frantic/magma/nodes/magma_node_impl.hpp>

#include "frantic/magma/maya/maya_magma_common.hpp"
#include "frantic/magma/maya/maya_magma_datatypes.hpp"
#include "frantic/magma/maya/nodes/magma_input_value_node.hpp"

namespace frantic {
namespace magma {
namespace maya {
namespace nodes {

/// XXX
/// here is a hack; I have to declare m_value type to be frantic::magma::variant_t.
/// I was trying to create a new boost::variant for inputValue nodes. (because inputValue should
/// only accept float, int and vector3f value; and frantic::magma::variant_t has a lot extra data types)
/// However, I could not compile it because compilers complains data type mismatches. (the MACROs does not make sense to
/// me)
///
/// in ordr to prevent users to do stupid things, I shall only expose three maya attributes to the users (float, int,
/// vector3f) for m_value property.
MAGMA_DEFINE_TYPE( "InputValue", "Input", magma_input_value_node )
MAGMA_EXPOSE_PROPERTY( enumValue, int )
MAGMA_EXPOSE_PROPERTY( fValue, float )
MAGMA_EXPOSE_PROPERTY( iValue, int )
MAGMA_EXPOSE_PROPERTY( vec3Value, frantic::graphics::vector3f )
MAGMA_DESCRIPTION( "Input value node" )
MAGMA_DEFINE_TYPE_END

void magma_input_value_node::compile_as_extension_type( frantic::magma::magma_compiler_interface& compiler ) {
    FF_LOG( debug ) << "magma_input_value::compile_as_extension_type: m_Val" << m_enumValue << std::endl;

    frantic::magma::variant_t outValue;

    if( m_enumValue == 1 )
        outValue = m_fValue;
    else if( m_enumValue == 2 )
        outValue = m_iValue;
    else if( m_enumValue == 3 )
        outValue = m_vec3Value;
    else
        throw std::runtime_error( "magma_input_value_node::compile_as_extension_type an unknown data type" );
    compiler.compile_constant( this->get_id(), outValue );
}

} // namespace nodes
} // namespace maya
} // namespace magma
} // namespace frantic
