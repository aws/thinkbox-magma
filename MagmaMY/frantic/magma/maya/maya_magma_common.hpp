// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "frantic/magma/maya/maya_magma_datatypes.hpp"

namespace frantic {
namespace magma {
namespace maya {

inline bool is_input_socket_data_type_support_bool( maya_magma_input_socket_data_type_t data ) {
    return ( data & MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_BOOL ) != 0;
}

inline bool is_input_socket_data_type_support_float( maya_magma_input_socket_data_type_t data ) {
    return ( data & MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT ) != 0;
}

inline bool is_input_socket_data_type_support_int( maya_magma_input_socket_data_type_t data ) {
    return ( data & MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT ) != 0;
}

inline bool is_input_socket_data_type_support_vec3( maya_magma_input_socket_data_type_t data ) {
    return ( data & MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 ) != 0;
}

inline bool is_input_socket_data_type_support_quat( maya_magma_input_socket_data_type_t data ) {
    return ( data & MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_QUAT ) != 0;
}

inline frantic::tstring input_socket_data_type_to_tstring( maya_magma_input_socket_data_type_t data ) {
    frantic::tstring outResult;
    if( is_input_socket_data_type_support_bool( data ) )
        outResult += _T( "Bool" );
    if( is_input_socket_data_type_support_float( data ) )
        outResult += _T( " Float" );
    if( is_input_socket_data_type_support_int( data ) )
        outResult += _T( " Int" );
    if( is_input_socket_data_type_support_vec3( data ) )
        outResult += _T( " Vec3" );
    if( is_input_socket_data_type_support_quat( data ) )
        outResult += _T( " Quat" );
    return outResult;
}

} // namespace maya
} // namespace magma
} // namespace frantic
