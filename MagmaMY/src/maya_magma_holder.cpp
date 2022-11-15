// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "frantic/magma/maya/maya_magma_description.hpp"
#include "frantic/magma/maya/maya_magma_exception.hpp"
#include "frantic/magma/maya/maya_magma_holder.hpp"

#include <frantic/magma/magma_exception.hpp>
#include <frantic/magma/magma_node_type.hpp>
#include <frantic/magma/magma_singleton.hpp>

#include <frantic/graphics/vector3f.hpp>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <memory>
#include <vector>

#define MAGMA_TRY try
#define MAGMA_CATCH                                                                                                    \
    catch( const frantic::magma::magma_exception& e ) {                                                                \
        FF_LOG( error ) << e.get_message( true ) << std::flush;                                                        \
    }                                                                                                                  \
    catch( const std::exception& e ) {                                                                                 \
        throw maya_magma_exception( e.what() );                                                                        \
    }

namespace frantic {
namespace magma {
namespace maya {
namespace holder {

maya_magma_holder::maya_magma_holder( std::unique_ptr<frantic::magma::magma_interface> magmaInterface )
    : m_magma( magmaInterface.release() ) {}

maya_magma_holder::~maya_magma_holder() {}

boost::shared_ptr<magma_interface> maya_magma_holder::get_magma_interface() { return m_magma; }

void maya_magma_holder::clear() {
    FF_LOG( debug ) << "maya_magma_holder: clear all nodes & connections in maya_magma_holder" << std::endl;
    m_magma->clear();
}

magma_id maya_magma_holder::create_node( const frantic::tstring& typeName ) {
    magma_id outId = frantic::magma::magma_interface::INVALID_ID;
    MAGMA_TRY { outId = m_magma->create_node( typeName ); }
    MAGMA_CATCH;
    return outId;
}

bool maya_magma_holder::delete_node( magma_id id ) {
    assert( id != frantic::magma::magma_interface::INVALID_ID );
    bool outResult = false;
    MAGMA_TRY {
        m_magma->delete_node( id );
        outResult = true;
    }
    MAGMA_CATCH;
    return outResult;
}

/*
void maya_magma_holder::delete_node_recursive( magma_id id )
{
        for( int i = 0, iEnd = m_magma->get_num_nodes( id ); i < iEnd; ++i )
                delete_node_recursive( m_magma->get_id( id, i ) );
        m_magma->delete_node( id );
}

bool maya_magma_holder::replace_node( magma_id idDest, magma_id idSrc )
{
        assert(idSrc != frantic::magma::magma_interface::INVALID_ID);
        assert(idDest != frantic::magma::magma_interface::INVALID_ID);

        bool outResult = false;
        MAGMA_TRY {
                if( idDest != idSrc && idDest >= 0 && idSrc >= 0) {
                        delete_node_recursive( idDest );
                        m_magma->replace_node( idDest, idSrc );
                        outResult = true;
                }
        } MAGMA_CATCH;
        return outResult;
}
*/

frantic::tstring maya_magma_holder::get_type_description( const frantic::tstring& typeName ) const {
    frantic::tstring outTypeDescription;

    MAGMA_TRY {
        if( const frantic::magma::magma_node_type* type = m_magma->get_singleton()->get_named_node_type( typeName ) )
            outTypeDescription = type->get_description();
    }
    MAGMA_CATCH;

    return outTypeDescription;
}

frantic::tstring maya_magma_holder::get_type_category( const frantic::tstring& typeName ) const {
    frantic::tstring outTypeCategory;

    MAGMA_TRY {
        if( const frantic::magma::magma_node_type* type = m_magma->get_singleton()->get_named_node_type( typeName ) )
            outTypeCategory = type->get_category();
    }
    MAGMA_CATCH;

    return outTypeCategory;
}

frantic::tstring maya_magma_holder::get_node_type( magma_id id ) const {
    frantic::tstring outResult = _T("");
    MAGMA_TRY { outResult = m_magma->get_type( id ); }
    MAGMA_CATCH;
    return outResult;
}

frantic::tstring maya_magma_holder::get_node_property_type( magma_id id, const frantic::tstring& propName ) const {
    frantic::tstring outTypeName( _T( "" ) );

    MAGMA_TRY {
        const std::type_info& typeInfo = m_magma->get_property_type( id, propName );
        if( typeInfo == typeid( int ) ) {
            outTypeName = _T("Int");
        } else if( typeInfo == typeid( float ) ) {
            outTypeName = _T("Float");
        } else if( typeInfo == typeid( frantic::tstring ) ) {
            outTypeName = _T("String");
        } else if( typeInfo == typeid( frantic::graphics::vector3f ) ) {
            outTypeName = _T("Vector3f");
        } else if( typeInfo == typeid( frantic::magma::magma_data_type ) ) {
            outTypeName = _T("MagmaDataType");
        } else if( typeInfo == typeid( bool ) ) {
            outTypeName = _T("Bool");
        } else if( typeInfo == typeid( std::vector<frantic::tstring> ) ) {
            outTypeName = _T("StringList");
        } else if( typeInfo == typeid( std::vector<int> ) ) {
            outTypeName = _T("IntList");
        } else {
            assert( false );
            throw maya_magma_exception( _T( "maya_magma_holder::get_node_property_type an unknown property type:" ) +
                                        frantic::strings::to_tstring( typeInfo.name() ) + _T( " on " ) + propName );
        }
    }
    MAGMA_CATCH;

    return outTypeName;
}

int maya_magma_holder::get_num_node_inputs( magma_id id ) const {
    int outNumNodeInputs = -1;
    MAGMA_TRY { outNumNodeInputs = m_magma->get_num_inputs( id ); }
    MAGMA_CATCH;
    return outNumNodeInputs;
}

void maya_magma_holder::set_num_node_inputs( magma_id id, int numInputs ) {
    MAGMA_TRY { m_magma->set_num_inputs( id, numInputs ); }
    MAGMA_CATCH;
}

int maya_magma_holder::get_num_node_outputs( magma_id id ) const {
    int outNumNodeOutputs = -1;
    MAGMA_TRY { outNumNodeOutputs = m_magma->get_num_outputs( id ); }
    MAGMA_CATCH;
    return outNumNodeOutputs;
}

input_socket_variant_t maya_magma_holder::get_node_input_default_value( magma_id id, index_type socketIndex ) const {
    input_socket_variant_t outDefaultValue;
    MAGMA_TRY { outDefaultValue = m_magma->get_input_default_value( id, socketIndex ); }
    MAGMA_CATCH;
    return outDefaultValue;
}

bool maya_magma_holder::set_node_input_default_value( magma_id id, index_type socketIndex,
                                                      input_socket_variant_t val ) {
    bool result = false;

    MAGMA_TRY {
        // XXX when m_magma->set_input_default_value returns true, change this as well
        m_magma->set_input_default_value( id, socketIndex, val );
        result = true;
    }
    MAGMA_CATCH;
    return result;
}

bool maya_magma_holder::set_node_input( magma_id id, index_type index, magma_id connectedID,
                                        index_type connectedIndex ) {
    bool result = false;
    MAGMA_TRY {
        // be careful with interface of m_magma here, see the definition of set_input
        result = ( m_magma->set_input( connectedID, connectedIndex, id, index ), true );
    }
    MAGMA_CATCH;
    return result;
}

/// return how many nodes in magma ?
std::vector<magma_id> maya_magma_holder::get_nodes() const {
    std::vector<magma_id> outCurrentNodes;

    MAGMA_TRY {
        int numNodes = m_magma->get_num_nodes( magma_interface::CURRENT_EDITING );
        for( int i = 0; i < numNodes; i++ )
            outCurrentNodes.push_back( m_magma->get_id( magma_interface::CURRENT_EDITING, i ) );
    }
    MAGMA_CATCH;

    return outCurrentNodes;
}

std::vector<frantic::tstring> maya_magma_holder::get_node_enum_values( magma_id id,
                                                                       const frantic::tstring& propName ) const {
    std::vector<frantic::tstring> outEnumValues;
    MAGMA_TRY { m_magma->get_property_accepted_values( id, propName, outEnumValues ); }
    MAGMA_CATCH;
    return outEnumValues;
}

bool maya_magma_holder::get_node_property_readonly( magma_id id, const frantic::tstring& propName ) const {
    bool result = false;
    MAGMA_TRY { result = !( m_magma->get_property_writable( id, propName ) ); }
    MAGMA_CATCH;
    return result;
}

bool maya_magma_holder::set_node_property( magma_id id, const frantic::tstring& propName, property_variant_t val ) {
    bool outResult = false;

    MAGMA_TRY {
        // NOTICE how we get the type information from magma flow rather than the `val` (this is IMPORTANT)
        // we can't really know what is the real data type of the property based on `val`
        // because `val` just hold property value. we have to get data type information from magma
        const std::type_info& type = m_magma->get_property_type( id, propName );
        FF_LOG( debug ) << "DEBUG: " << type.name() << ":" << val.type().name() << std::endl;

        if( type == typeid( void ) ) {
            ;
        } else if( type == typeid( float ) ) {
            outResult = m_magma->set_property<float>( id, propName, boost::get<float>( val ) );
        } else if( type == typeid( int ) ) {
            outResult = m_magma->set_property<int>( id, propName, boost::get<int>( val ) );
        } else if( type == typeid( bool ) ) {
            outResult = m_magma->set_property<bool>( id, propName, boost::get<bool>( val ) );
        } else if( type == typeid( frantic::tstring ) ) {
            outResult = m_magma->set_property<frantic::tstring>( id, propName, boost::get<frantic::tstring>( val ) );
        } else if( type == typeid( frantic::graphics::vector3f ) ) {
            outResult = m_magma->set_property<frantic::graphics::vector3f>(
                id, propName, boost::get<frantic::graphics::vector3f>( val ) );
        } else if( type == typeid( frantic::magma::magma_data_type ) ) {
            // hack here
            const frantic::magma::magma_data_type* magmaDataType =
                frantic::magma::magma_singleton::get_named_data_type( boost::get<frantic::tstring>( val ) );
            if( !magmaDataType )
                throw maya_magma_exception( _T( "maya_magma_holder::set_node_property a invalid magma_data_type " ) +
                                            boost::get<frantic::tstring>( val ) );
            else
                outResult = m_magma->set_property<frantic::magma::magma_data_type>( id, propName, *magmaDataType );
        } else if( type == typeid( frantic::magma::variant_t ) ) {
            // hack this is for inputValue
            if( val.type() == typeid( float ) ) {
                outResult = m_magma->set_property<float>( id, propName, boost::get<float>( val ) );
            } else if( val.type() == typeid( int ) ) {
                outResult = m_magma->set_property<int>( id, propName, boost::get<int>( val ) );
            } else if( val.type() == typeid( frantic::graphics::vector3f ) ) {
                outResult = m_magma->set_property<frantic::graphics::vector3f>(
                    id, propName, boost::get<frantic::graphics::vector3f>( val ) );
            }
        } else if( type == typeid( std::vector<frantic::tstring> ) ) {
            outResult = m_magma->set_property<std::vector<frantic::tstring>>(
                id, propName, boost::get<std::vector<frantic::tstring>>( val ) );
        } else if( type == typeid( std::vector<int> ) ) {
            // TODO:
        } else {
            assert( false );
            throw maya_magma_exception( "maya_magma_holder::set_node_property you should never get here" );
        }
    }
    MAGMA_CATCH;
    return outResult;
}

frantic::tstring maya_magma_holder::get_node_input_description( magma_id id, index_type socketIndex ) const {
    frantic::tstring outDescription;
    MAGMA_TRY { m_magma->get_input_description( id, socketIndex, outDescription ); }
    MAGMA_CATCH;
    return outDescription;
}

frantic::tstring maya_magma_holder::get_node_output_description( magma_id id, index_type socketIndex ) const {
    frantic::tstring outDescription;
    MAGMA_TRY { m_magma->get_output_description( id, socketIndex, outDescription ); }
    MAGMA_CATCH;
    return outDescription;
}

std::vector<frantic::tstring> maya_magma_holder::get_node_property_names( magma_id id ) const {
    std::vector<frantic::tstring> outResult;
    MAGMA_TRY {
        for( int i = 0, iEnd = m_magma->get_num_properties( id ); i < iEnd; ++i )
            outResult.push_back( m_magma->get_property_name( id, i ) );
    }
    MAGMA_CATCH;
    return outResult;
}

int maya_magma_holder::get_node_num_properties( magma_id id ) const {
    int outResult;
    MAGMA_TRY { outResult = m_magma->get_num_properties( id ); }
    MAGMA_CATCH;
    return outResult;
}

frantic::tstring maya_magma_holder::get_node_property_name( magma_id id, index_type index ) const {
    frantic::tstring outResult;
    MAGMA_TRY { outResult = m_magma->get_property_name( id, index ); }
    MAGMA_CATCH;
    return outResult;
}

bool maya_magma_holder::push_editable_BLOP( magma_id id ) {
    bool outResult;
    MAGMA_TRY {
        outResult = ( m_magma->pushBLOP( id ), true ); // Change once pushBLOP returns true/false.
    }
    MAGMA_CATCH;
    return outResult;
}

magma_id maya_magma_holder::pop_editable_BLOP() {
    magma_id outResult;
    MAGMA_TRY { outResult = m_magma->popBLOP(); }
    MAGMA_CATCH;
    return outResult;
}

magma_id maya_magma_holder::get_BLOP_source_id( magma_id id ) const {
    MAGMA_TRY {
        frantic::magma::magma_node_base* node = m_magma->get_node( id );
        if( node != NULL ) {
            frantic::magma::magma_node_base* sourceNode = node->get_contained_source();
            if( sourceNode != NULL ) {
                return sourceNode->get_id();
            }
        }
    }
    MAGMA_CATCH;

    return frantic::magma::magma_interface::INVALID_ID;
}
magma_id maya_magma_holder::get_BLOP_sink_id( magma_id id ) const {
    MAGMA_TRY {
        frantic::magma::magma_node_base* node = m_magma->get_node( id );
        if( node != NULL ) {
            frantic::magma::magma_node_base* sourceNode = node->get_contained_sink();
            if( sourceNode != NULL ) {
                return sourceNode->get_id();
            }
        }
    }
    MAGMA_CATCH;

    return frantic::magma::magma_interface::INVALID_ID;
}

} // namespace holder
} // namespace maya
} // namespace magma
} // namespace frantic
