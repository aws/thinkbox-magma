// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/strings/tstring.hpp>

#include "frantic/magma/maya/maya_magma_common.hpp"
#include "frantic/magma/maya/maya_magma_info.hpp"

namespace frantic {
namespace magma {
namespace maya {
namespace info {

/// "propertyIndex:propertyName,propertyType,readOnly,acceptedValues(if any)"
frantic::tstring maya_magma_node_property_info::to_tstring() const {
    frantic::tstring outResult( frantic::convert::to_tstring( m_index ) + _T( ":" ) + m_name + _T( "," ) + m_type +
                                _T( "," ) + convert::to_tstring( m_isReadOnly ) + _T( "," ) );
    std::vector<frantic::tstring>::const_iterator cit;
    for( cit = m_acceptedValues.begin(); cit != m_acceptedValues.end(); cit++ )
        outResult += *cit + _T( "|" );
    return outResult;
}

////////////////////////////////////////////////////////////////////////////////

/// "inputSocket#inputSocketIndex:inputSocketDescription,currentInputSocketData,inputSocketSupportedDataType"
frantic::tstring maya_magma_node_input_socket_info::to_tstring() const {
    frantic::tstring outResult( _T( "inputsocket#" ) + frantic::convert::to_tstring( m_index ) + _T( ":" ) +
                                m_description + _T( "," ) + frantic::convert::to_tstring( m_data ) );
    outResult += _T( "," ) + input_socket_data_type_to_tstring( m_dataType );
    return outResult;
}

////////////////////////////////////////////////////////////////////////////////

/// "outputsocket#outputSocketIndex:outputSocketDescription"
frantic::tstring maya_magma_node_output_socket_info::to_tstring() const {
    frantic::tstring outResult( _T( "outputsocket#" ) + frantic::convert::to_tstring( m_index ) + _T( ":" ) +
                                m_description );
    return outResult;
}

////////////////////////////////////////////////////////////////////////////////

frantic::tstring maya_magma_node_info::to_tstring() const {
    frantic::tstring outResult( _T( "\"" ) + m_nodeType + _T( "," ) + m_nodeCategory + _T( "\"" ) );
    outResult += +_T( " " ) + this->properties_to_tstring();
    outResult += +_T( " " ) + this->input_socket_to_tstring();
    outResult += +_T( " " ) + this->output_socket_to_tstring();

    return outResult;
}

frantic::tstring maya_magma_node_info::properties_to_tstring( int index ) const {
    frantic::tstring outResult;
    std::vector<maya_magma_node_property_info>::const_iterator citP;
    if( index == -1 ) {
        for( citP = m_propertyInfos.begin(); citP != m_propertyInfos.end(); citP++ )
            outResult += _T( "\"" ) + citP->to_tstring() + _T( "\" " );
    } else {
        for( citP = m_propertyInfos.begin(); citP != m_propertyInfos.end(); citP++ )
            if( citP->m_index == index ) {
                outResult += _T( "\"" ) + citP->to_tstring() + _T( "\" " );
            }
    }
    return outResult;
}

frantic::tstring maya_magma_node_info::input_socket_to_tstring( int index ) const {
    frantic::tstring outResult;
    std::vector<maya_magma_node_input_socket_info>::const_iterator citInputSocket;
    if( index == -1 ) {
        for( citInputSocket = m_inputSocketInfos.begin(); citInputSocket != m_inputSocketInfos.end(); citInputSocket++ )
            outResult += _T( "\"" ) + citInputSocket->to_tstring() + _T( "\" " );
    } else {
        for( citInputSocket = m_inputSocketInfos.begin(); citInputSocket != m_inputSocketInfos.end(); citInputSocket++ )
            if( citInputSocket->m_index == index )
                outResult += _T( "\"" ) + citInputSocket->to_tstring() + _T( "\" " );
    }
    return outResult;
}

frantic::tstring maya_magma_node_info::output_socket_to_tstring( int index ) const {
    frantic::tstring outResult;
    std::vector<maya_magma_node_output_socket_info>::const_iterator citOutputSocket;

    if( index == -1 ) {
        for( citOutputSocket = m_outputSocketInfos.begin(); citOutputSocket != m_outputSocketInfos.end();
             citOutputSocket++ )
            outResult += _T( "\"" ) + citOutputSocket->to_tstring() + _T( "\" " );
    } else {
        for( citOutputSocket = m_outputSocketInfos.begin(); citOutputSocket != m_outputSocketInfos.end();
             citOutputSocket++ )
            if( citOutputSocket->m_index == index )
                outResult += _T( "\"" ) + citOutputSocket->to_tstring() + _T( "\" " );
    }
    return outResult;
}

void maya_magma_node_info::add_properties_to_list( MStringArray& arr, int index ) const {
    std::vector<maya_magma_node_property_info>::const_iterator citP;
    if( index == -1 ) {
        for( citP = m_propertyInfos.begin(); citP != m_propertyInfos.end(); citP++ )
            arr.append( citP->to_tstring().c_str() );
    } else {
        for( citP = m_propertyInfos.begin(); citP != m_propertyInfos.end(); citP++ )
            if( citP->m_index == index ) {
                arr.append( citP->to_tstring().c_str() );
            }
    }
}

void maya_magma_node_info::add_input_socket_to_list( MStringArray& arr, int index ) const {
    std::vector<maya_magma_node_input_socket_info>::const_iterator citInputSocket;
    if( index == -1 ) {
        for( citInputSocket = m_inputSocketInfos.begin(); citInputSocket != m_inputSocketInfos.end(); citInputSocket++ )
            arr.append( citInputSocket->to_tstring().c_str() );
    } else {
        for( citInputSocket = m_inputSocketInfos.begin(); citInputSocket != m_inputSocketInfos.end(); citInputSocket++ )
            if( citInputSocket->m_index == index )
                arr.append( citInputSocket->to_tstring().c_str() );
    }
}

void maya_magma_node_info::add_output_socket_to_list( MStringArray& arr, int index ) const {
    std::vector<maya_magma_node_output_socket_info>::const_iterator citOutputSocket;
    if( index == -1 ) {
        for( citOutputSocket = m_outputSocketInfos.begin(); citOutputSocket != m_outputSocketInfos.end();
             citOutputSocket++ )
            arr.append( citOutputSocket->to_tstring().c_str() );
    } else {
        for( citOutputSocket = m_outputSocketInfos.begin(); citOutputSocket != m_outputSocketInfos.end();
             citOutputSocket++ )
            if( citOutputSocket->m_index == index )
                arr.append( citOutputSocket->to_tstring().c_str() );
    }
}

void maya_magma_node_info::add_to_list( MStringArray& arr ) const {
    frantic::tstring outResult( m_nodeType + _T( "," ) + m_nodeCategory );
    arr.append( outResult.c_str() );

    this->add_properties_to_list( arr );
    this->add_input_socket_to_list( arr );
    this->add_output_socket_to_list( arr );
}

} // namespace info
} // namespace maya
} // namespace magma
} // namespace frantic
