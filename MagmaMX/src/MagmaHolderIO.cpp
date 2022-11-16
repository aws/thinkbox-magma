// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/max3d/MagmaHolder.hpp>
#include <frantic/magma/max3d/MagmaMaxNodeExtension.hpp>

#include <frantic/magma/magma_exception.hpp>
#include <frantic/magma/magma_singleton.hpp>

#include <frantic/max3d/iostream.hpp>

#include <frantic/logging/logging_level.hpp>

#include <boost/scoped_array.hpp>

#define MAGMAHOLDER_SAVE_VERSION 0

namespace frantic {
namespace magma {
namespace max3d {

namespace magma_io {
enum {
    kChunkVersion,
    kChunkNodes,
    kChunkNodeType,
    kChunkNodeID,
    kChunkNodeProps,
    kChunkNodePropName,
    kChunkNodePropValue,
    kChunkNodeConnections,
    kChunkNodeExtProps,
    kChunkNodeSockets,
    kChunkNodeOutputs,
    kChunkNodeInputDefaults,
    kChunkNodeWType, // We switched to wchar_t based strings later, so this is a different chunk ID.
};
}

namespace {
inline bool is_reftarg_type( int paramType ) {
    int baseType = ( paramType & ~( TYPE_TAB | TYPE_BY_REF | TYPE_BY_VAL | TYPE_BY_PTR ) );

    return ( baseType == TYPE_MTL || baseType == TYPE_TEXMAP || baseType == TYPE_INODE || baseType == TYPE_REFTARG ||
             baseType == TYPE_PBLOCK2 || baseType == TYPE_OBJECT || baseType == TYPE_CONTROL );
}
} // namespace

IOResult MagmaHolder::SaveNode( ISave* isave, magma_id id ) {
    ULONG numWritten = 0;
    IOResult result = IO_OK;

    frantic::tstring type = m_magma->get_type( id );

    isave->BeginChunk( magma_io::kChunkNodeType );
    result = isave->WriteCString( type.c_str() );
    if( result != IO_OK )
        return IO_ERROR;
    isave->EndChunk();

    isave->BeginChunk( magma_io::kChunkNodeID );
    result = isave->Write( &id, sizeof( int ), &numWritten );
    if( result != IO_OK || numWritten != sizeof( int ) )
        return IO_ERROR;
    isave->EndChunk();

    int numConnections = m_magma->get_num_inputs( id );
    if( numConnections > 0 ) {
        boost::scoped_array<std::pair<magma_id, int>> connections( new std::pair<magma_id, int>[numConnections] );
        for( int j = 0; j < numConnections; ++j )
            connections[j] = m_magma->get_input( id, j );

        isave->BeginChunk( magma_io::kChunkNodeConnections );
#if MAX_VERSION_MAJOR < 15
        result = isave->Write( connections.get(), sizeof( std::pair<magma_id, int> ) * numConnections, &numWritten );
#else
        result =
            isave->WriteVoid( connections.get(), sizeof( std::pair<magma_id, int> ) * numConnections, &numWritten );
#endif
        if( result != IO_OK || numWritten != ( sizeof( std::pair<magma_id, int> ) * numConnections ) )
            return IO_ERROR;
        isave->EndChunk();

        std::vector<FPValue> defaultValues;
        for( int j = 0; j < numConnections; ++j )
            defaultValues.push_back( this->get_node_input_default_value( id, index_type( j ) ) );

        isave->BeginChunk( magma_io::kChunkNodeInputDefaults );
        result = frantic::max3d::isave_write_propmap( isave, defaultValues.begin(), defaultValues.end() );
        if( result != IO_OK )
            return IO_ERROR;
        isave->EndChunk();
    }

    // Use a vector instead of a map since we are only iterating on all values, not doing key-value lookups.
    std::vector<std::pair<M_STD_STRING, FPValue>> props;
    for( int j = 0, jEnd = m_magma->get_num_properties( id ); j < jEnd; ++j ) {
        M_STD_STRING propName = m_magma->get_property_name( id, j );

        if( propName.empty() || propName[0] == _T( '_' ) || !m_magma->get_property_writable( id, propName ) )
            continue;

        FPValue value = this->get_node_property( id, propName.c_str() );
        if( value.type == TYPE_VALUE ||
            is_reftarg_type( value.type ) ) // Don't bother saving Max RefTarg based types or MXS only types.
            continue;

        props.push_back( std::make_pair( propName, value ) );
    }

    if( !props.empty() ) {
        isave->BeginChunk( magma_io::kChunkNodeProps );
        result = frantic::max3d::isave_write_propmap( isave, props.begin(), props.end() );
        if( result != IO_OK )
            return IO_ERROR;
        isave->EndChunk();
    }

    ext_prop_map_type::const_iterator itExtNode = m_extensionProps.find( id );
    if( itExtNode != m_extensionProps.end() && !itExtNode->second.empty() ) {
        isave->BeginChunk( magma_io::kChunkNodeExtProps );
        result = frantic::max3d::isave_write_propmap( isave, itExtNode->second.begin(), itExtNode->second.end() );
        isave->EndChunk();
    }

    int numContainedNodes = m_magma->get_num_nodes( id );
    if( numContainedNodes > 0 ) {
        m_magma->pushBLOP( id );

        isave->BeginChunk( magma_io::kChunkNodeSockets );

        std::pair<magma_id, int> internalIDs[2];

        magma_node_base* node = m_magma->get_node( id );
        if( !node )
            return IO_ERROR;

        if( magma_node_base* sourceNode = node->get_contained_source() )
            internalIDs[0].first = sourceNode->get_id();
        internalIDs[0].second = -1;

        if( magma_node_base* sinkNode = node->get_contained_sink() )
            internalIDs[1].first = sinkNode->get_id();
        internalIDs[1].second = -1;

        /*if( !m_magma->get_property<magma_interface::magma_id>( id, _T("_internal_input_id"), internalIDs[0].first ) )
                internalIDs[0].first = magma_interface::INVALID_ID;
        internalIDs[0].second = -1;

        if( !m_magma->get_property<magma_interface::magma_id>( id, _T("_internal_output_id"), internalIDs[1].first ) )
                internalIDs[1].first = magma_interface::INVALID_ID;
        internalIDs[1].second = -1;*/

#if MAX_VERSION_MAJOR < 15
        result = isave->Write( &internalIDs, (ULONG)sizeof( std::pair<magma_id, int> ) * 2, &numWritten );
#else
        result = isave->WriteVoid( &internalIDs, (ULONG)sizeof( std::pair<magma_id, int> ) * 2, &numWritten );
#endif
        if( result != IO_OK || numWritten != (ULONG)sizeof( std::pair<magma_id, int> ) * 2 )
            return IO_ERROR;

        isave->EndChunk();

        // TODO: This could be eliminated by just adding _internal_output_id to the list of contained nodes.
        // int numOutputs = m_magma->get_num_outputs( frantic::magma::magma_interface::CURRENT_EDITING );
        int numOutputs = m_magma->get_num_inputs( internalIDs[1].first );
        if( numOutputs > 0 ) {
            isave->BeginChunk( magma_io::kChunkNodeOutputs );

            boost::scoped_array<std::pair<magma_id, int>> outputIDs( new std::pair<magma_id, int>[numOutputs] );
            for( int j = 0; j < numOutputs; ++j )
                // outputIDs[j] = m_magma->get_output( frantic::magma::magma_interface::CURRENT_EDITING, j );
                outputIDs[j] = m_magma->get_input( internalIDs[1].first, j );

#if MAX_VERSION_MAJOR < 15
            result = isave->Write( outputIDs.get(), (ULONG)( sizeof( std::pair<magma_id, int> ) * numOutputs ),
                                   &numWritten );
#else
            result = isave->WriteVoid( outputIDs.get(), (ULONG)( sizeof( std::pair<magma_id, int> ) * numOutputs ),
                                       &numWritten );
#endif
            if( result != IO_OK || numWritten != (ULONG)( sizeof( std::pair<magma_id, int> ) * numOutputs ) )
                return IO_ERROR;

            isave->EndChunk();
        }

        isave->BeginChunk( magma_io::kChunkNodes );

        for( int j = 0; j < numContainedNodes; ++j ) {
            magma_id nextId = m_magma->get_id( id, j );

            // Don't save the special nodes.
            // TODO I'd like to remove the special casing from here. We will need to allow BLOPSocket nodes to be
            // createable. Maybe with a LOADING flag ...
            if( nextId == internalIDs[0].first || nextId == internalIDs[1].first )
                continue;

            isave->BeginChunk( (USHORT)j );

            result = SaveNode( isave, nextId );
            if( result != IO_OK )
                return IO_ERROR;

            isave->EndChunk();
        }

        isave->EndChunk();

        if( id != m_magma->popBLOP() )
            return IO_ERROR;
    }

    return IO_OK;
}

IOResult MagmaHolder::Save( ISave* isave ) {
    ULONG numWritten = 0;
    IOResult result = IO_OK;

    DWORD fv = isave->SavingVersion();
    FF_LOG( debug ) << "Saving Magma\n\tSavingVersion() = " << fv;
#if MAX_VERSION_MAJOR >= 15
    UINT cp = isave->CodePage();
    FF_LOG( debug ) << "\n\tCodePage() = " << cp << std::endl;
#endif

    isave->BeginChunk( magma_io::kChunkVersion );
    int version = MAGMAHOLDER_SAVE_VERSION;
    result = isave->Write( &version, sizeof( int ), &numWritten );
    if( result != IO_OK || numWritten != sizeof( int ) )
        return IO_ERROR;
    isave->EndChunk();

    isave->BeginChunk( magma_io::kChunkNodes );
    for( int i = 0, iEnd = m_magma->get_num_nodes( frantic::magma::magma_interface::TOPMOST_LEVEL ); i < iEnd; ++i ) {
        isave->BeginChunk( (USHORT)i );

        magma_id id = m_magma->get_id( frantic::magma::magma_interface::TOPMOST_LEVEL, i );
        result = SaveNode( isave, id );
        if( result != IO_OK )
            return IO_ERROR;

        isave->EndChunk();
    }
    isave->EndChunk(); // kChunkNodes

    return IO_OK;
}

class FixRefsPLCB : public PostLoadCallback {
    MagmaHolder* m_holder;

  public:
    FixRefsPLCB( MagmaHolder* holder )
        : m_holder( holder ) {}

    virtual void proc( ILoad* /*iload*/ ) {
        m_holder->FixRefsPLCBImpl();
        delete this;
    }

    virtual int Priority() { return 6; }
};

void MagmaHolder::FixRefsPLCBImpl() {
    // Remove any null entries that have somehow made it in here.
    int dest = 0;
    for( int i = 0, iEnd = m_pblock->Count( kMagmaNodeMaxData ); i < iEnd; ++i ) {
        ReferenceTarget* rtarg = m_pblock->GetReferenceTarget( kMagmaNodeMaxData, 0, i );
        if( rtarg ) {
            if( dest != i )
                m_pblock->SetValue( kMagmaNodeMaxData, 0, rtarg, dest );
            ++dest;
        }
    }

    if( dest != m_pblock->Count( kMagmaNodeMaxData ) )
        m_pblock->SetCount( kMagmaNodeMaxData, dest );

    // For each max node that was loaded, connect it to the appropriate magma node.
    for( int i = 0, iEnd = m_pblock->Count( kMagmaNodeMaxData ); i < iEnd; ++i ) {
        ReferenceTarget* rtarg = m_pblock->GetReferenceTarget( kMagmaNodeMaxData, 0, i );

        magma_id id = GetMagmaNodeInterface( rtarg )->get_id();

        try {
            ReferenceTarget* oldTarg = NULL;
            if( m_magma->get_property( id, _T("_maxImpl"), oldTarg ) ) {
                RefResult r;
                if( oldTarg )
                    r = oldTarg->MaybeAutoDelete();
                m_magma->set_property<ReferenceTarget*>( id, _T("_maxImpl"), rtarg );
            }
        } catch( const frantic::magma::magma_exception& e ) {
            FF_LOG( error ) << e.get_message( true ) << std::endl;

            rtarg = NULL;

            m_pblock->SetValue( kMagmaNodeMaxData, 0, rtarg, i );
        } catch( const std::exception& e ) {
            FF_LOG( error ) << e.what() << std::endl;

            rtarg = NULL;

            m_pblock->SetValue( kMagmaNodeMaxData, 0, rtarg, i );
        }
    }

    std::vector<frantic::magma::magma_interface::magma_id> blopStack;
    blopStack.push_back( frantic::magma::magma_interface::TOPMOST_LEVEL );

    // Visit every node, checking to make sure it has a valid _maxImpl if it needs one.
    while( !blopStack.empty() ) {
        frantic::magma::magma_interface::magma_id curBlop = blopStack.back();
        blopStack.pop_back();

        for( int i = 0, iEnd = m_magma->get_num_nodes( curBlop ); i < iEnd; ++i ) {
            frantic::magma::magma_interface::magma_id curNode = m_magma->get_id( curBlop, i );
            if( m_magma->get_num_nodes( curNode ) > 0 ) {
                blopStack.push_back( curNode );
            } else {
                ReferenceTarget* maxNode = NULL;
                if( m_magma->get_property<ReferenceTarget*>( curNode, _T("_maxImpl"), maxNode ) && !maxNode )
                    manage_magma_max_node( curNode );
            }
        }
    }
}

IOResult MagmaHolder::LoadNextNode( ILoad* iload ) {
    IOResult result;
    ULONG numRead;

    M_STD_STRING type;
    MCHAR* szType;
    magma_id id = frantic::magma::magma_interface::INVALID_ID;
    std::vector<std::pair<M_STD_STRING, FPValue>> props;
    std::map<M_STD_STRING, FPValue> extProps;
    std::vector<FPValue> inputDefaults;

    int numConnections = 0, numSockets = 0, numOutputs = 0;
    boost::scoped_array<std::pair<magma_id, int>> connections, sockets, outputs;

    bool alreadyCreated = false;

    result = iload->OpenChunk();
    while( result == IO_OK ) {
        switch( iload->CurChunkID() ) {
        case magma_io::kChunkNodeType:
            result = iload->ReadCStringChunk( &szType );
            if( result != IO_OK )
                return IO_ERROR;
            type = szType;
            if( type == _T("Loop::Input") ) // TODO remove this soon
                type = _T("Loop__Input");
            else if( type == _T("Loop::Output") )
                type = _T("Loop__Output");
            else if( type == _T("VertexLoop") )
                type = _T("VertexLoopByEdge");
            else if( type == _T("VertexLoop__Input") )
                type = _T("VertexLoopByEdge__Input");
            else if( type == _T("VertexLoop__Output") )
                type = _T("VertexLoopByEdge__Output");
            else if( type == _T("FaceLoop") )
                type = _T("FaceLoopByEdge");
            else if( type == _T("FaceLoop__Input") )
                type = _T("FaceLoopByEdge__Input");
            else if( type == _T("FaceLoop__Output") )
                type = _T("FaceLoopByEdge__Output");

            break;
        case magma_io::kChunkNodeID:
            result = iload->Read( &id, sizeof( int ), &numRead );
            if( result != IO_OK || numRead != sizeof( int ) )
                return IO_ERROR;
            break;
        case magma_io::kChunkNodeProps:
            result = frantic::max3d::iload_read_propmap( iload, props );
            if( result != IO_OK )
                return IO_ERROR;
            break;
        case magma_io::kChunkNodeExtProps:
            result = frantic::max3d::iload_read_propmap( iload, extProps );
            if( result != IO_OK )
                return IO_ERROR;
            break;
        case magma_io::kChunkNodeConnections:
            numConnections = static_cast<int>( iload->CurChunkLength() / sizeof( std::pair<magma_id, int> ) );
            connections.reset( new std::pair<magma_id, int>[numConnections] );

#if MAX_VERSION_MAJOR < 15
            result = iload->Read( connections.get(), sizeof( std::pair<magma_id, int> ) * numConnections, &numRead );
#else
            result =
                iload->ReadVoid( connections.get(), sizeof( std::pair<magma_id, int> ) * numConnections, &numRead );
#endif
            if( result != IO_OK || numRead != sizeof( std::pair<magma_id, int> ) * numConnections )
                return IO_ERROR;
            break;
        case magma_io::kChunkNodeInputDefaults:
            result = frantic::max3d::iload_read_propmap( iload, inputDefaults );
            if( result != IO_OK )
                return IO_ERROR;
            break;
        case magma_io::kChunkNodeOutputs:
            numOutputs = static_cast<int>( iload->CurChunkLength() / sizeof( std::pair<magma_id, int> ) );
            outputs.reset( new std::pair<magma_id, int>[numOutputs] );

#if MAX_VERSION_MAJOR < 15
            result = iload->Read( outputs.get(), sizeof( std::pair<magma_id, int> ) * numOutputs, &numRead );
#else
            result = iload->ReadVoid( outputs.get(), sizeof( std::pair<magma_id, int> ) * numOutputs, &numRead );
#endif
            if( result != IO_OK || numRead != sizeof( std::pair<magma_id, int> ) * numOutputs )
                return IO_ERROR;
            break;
        case magma_io::kChunkNodeSockets:
            numSockets = static_cast<int>( iload->CurChunkLength() / sizeof( std::pair<magma_id, int> ) );
            sockets.reset( new std::pair<magma_id, int>[numSockets] );

#if MAX_VERSION_MAJOR < 15
            result = iload->Read( sockets.get(), sizeof( std::pair<magma_id, int> ) * numSockets, &numRead );
#else
            result = iload->ReadVoid( sockets.get(), sizeof( std::pair<magma_id, int> ) * numSockets, &numRead );
#endif
            if( result != IO_OK || numRead != sizeof( std::pair<magma_id, int> ) * numSockets )
                return IO_ERROR;
            break;
        case magma_io::kChunkNodes:
            if( !alreadyCreated ) {
                alreadyCreated = true;

                if( m_magma->get_singleton()->get_named_node_type( type ) == NULL )
                    type = _T("Missing");

                if( id != m_magma->create_node( type, id ) )
                    return IO_ERROR;
            }

            m_magma->pushBLOP( id );
            {
                // Need to set the id of this compound node's internal stand-in node in case the auto-generated one
                // conflicts with one of the contained nodes.
                if( numSockets > 0 ) {
                    magma_interface::magma_id internalInputID = m_magma->get_id( id, -1 );
                    magma_interface::magma_id internalOutputID = m_magma->get_id( id, -2 );

                    m_magma->replace_node( sockets[0].first, internalInputID );

                    if( numSockets > 1 )
                        m_magma->replace_node( sockets[1].first, internalOutputID );
                    else {
                        // This is an old file, where we didn't have a node here. Since the auto-chosen ID will likely
                        // collide with an ID that was saved into the file, I am generating an ID that is likely free.
                        // There is no guarantee of course, but it is unlikely that someone has over a million nodes in
                        // a scene. Another option is setting it to a negative ID and looping over it later and
                        // replacing it.
                        m_magma->replace_node( id + 1000000, internalOutputID );
                    }
                }

                // HACK This fixes loading since I changed Loop based nodes to default to 1 output, but the socket
                // re-order code screws up sockets when
                //      numOutputs is not 0. This hack will compensate for that.
                m_magma->set_property<int>( id, _T("numOutputs"), 0 );

                result = iload->OpenChunk();
                while( result == IO_OK ) {
                    result = LoadNextNode( iload );
                    if( result != IO_OK )
                        return IO_ERROR;
                    iload->CloseChunk();
                    result = iload->OpenChunk();
                }

                if( result != IO_END )
                    return IO_ERROR;
            }
            if( id != m_magma->popBLOP() )
                return IO_ERROR;
            break;
        }

        iload->CloseChunk();
        result = iload->OpenChunk();
    } // for each chunk in a node.

    if( result != IO_END )
        return IO_ERROR;

    // If the type is not known, have it create a missing node instead.
    if( m_magma->get_singleton()->get_named_node_type( type ) == NULL )
        type = _T("Missing");

    if( !alreadyCreated && ( id != m_magma->create_node( type, id ) ) )
        return IO_ERROR;

    // Set the number of inputs and outputs since at least one node uses this to affect the property setting.
    m_magma->set_num_inputs( id, numConnections );
    m_magma->set_num_outputs( id, numOutputs );

    for( std::vector<std::pair<M_STD_STRING, FPValue>>::iterator it = props.begin(), itEnd = props.end(); it != itEnd;
         ++it ) {
        if( is_reftarg_type(
                it->second.type ) ) // These snuck in here in older builds but hopefully it won't happen anymore
            continue;

        if( !m_magma->get_property_writable( id, it->first ) )
            continue;

        set_node_property( id, it->first.c_str(), it->second );

        // I'm not sure if this is strictly necessary, but it seems like a good idea to free the memory if it held some.
        // We don't have to worry about extension props here, since we are only setting built-in props.
        it->second.Free();
    }

    if( !extProps.empty() )
        m_extensionProps[id].swap( extProps );

    if( numConnections > 0 ) {
        m_magma->set_num_inputs( id, numConnections );
        for( int j = 0; j < numConnections; ++j )
            m_magma->set_input( id, j, connections[j].first, connections[j].second );

        if( inputDefaults.size() != numConnections && !inputDefaults.empty() )
            return IO_ERROR;

        int counter = 0;
        for( std::vector<FPValue>::iterator it = inputDefaults.begin(), itEnd = inputDefaults.end(); it != itEnd;
             ++it, ++counter ) {
            this->set_node_input_default_value( id, index_type( counter ), *it );

            it->Free();
        }
    }

    if( numOutputs > 0 ) {
        magma_interface::magma_id internalOutputID = m_magma->get_id( id, -2 );

        m_magma->set_num_inputs( internalOutputID, numOutputs );

        for( int j = 0; j < numOutputs; ++j )
            m_magma->set_input( internalOutputID, j, outputs[j].first, outputs[j].second );
    }

    return IO_OK;
}

IOResult MagmaHolder::Load( ILoad* iload ) {
    IOResult r = GenericReferenceTarget<ReferenceTarget, MagmaHolder>::Load( iload );
    if( r != IO_OK )
        return r;

    DWORD fv = HIWORD( iload->GetFileSaveVersion() );
    FF_LOG( debug ) << "Loading Magma\n\tGetFileSaveVersion() = " << fv;
#if MAX_VERSION_MAJOR >= 15
    DWORD fsav = iload->FileSaveAsVersion();
    UINT fcp = iload->FileActiveCodePage();
    FF_LOG( debug ) << "\n\tFileSaveAsVersion() = " << fsav << "\n\tFileActiveCodePage() = " << fcp << std::endl;
#endif

    try {
        ULONG numRead = 0;
        IOResult result = iload->OpenChunk();

        int version = -1;

        while( result == IO_OK ) {
            switch( iload->CurChunkID() ) {
            case magma_io::kChunkVersion:
                result = iload->Read( &version, sizeof( int ), &numRead );
                if( result != IO_OK || numRead != sizeof( int ) )
                    return IO_ERROR;
                break;
            case magma_io::kChunkNodes:
                if( version ==
                    -1 ) // If the version is not set, then we can't know how to read the rest of this shenanigans.
                    return IO_ERROR;

                if( version > MAGMAHOLDER_SAVE_VERSION )
                    FF_LOG( warning ) << _T("Magma file version: ") << version
                                      << _T(" is newer than this plugin. Please upgrade your product.") << std::endl;

                result = iload->OpenChunk();
                while( result == IO_OK ) {
                    // TODO, if newer versions change the formatting significantly we should make a separate method.
                    result = LoadNextNode( iload );
                    if( result != IO_OK )
                        return IO_ERROR;
                    iload->CloseChunk();
                    result = iload->OpenChunk();
                } // for each node

                if( result != IO_END )
                    return IO_ERROR;
                break;
            }

            iload->CloseChunk();
            result = iload->OpenChunk();
        }

        if( result != IO_END )
            return IO_ERROR;

        iload->RegisterPostLoadCallback( new FixRefsPLCB( this ) );

        return IO_OK;
    } catch( const frantic::magma::magma_exception& e ) {
        FF_LOG( error ) << e.get_message( true ) << std::flush;
    } catch( const std::exception& e ) {
        FF_LOG( error ) << e.what() << std::endl;
    }

    return IO_ERROR;
}

} // namespace max3d
} // namespace magma
} // namespace frantic
