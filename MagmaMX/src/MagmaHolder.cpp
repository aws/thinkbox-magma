// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/max3d/MagmaHolder.hpp>
#include <frantic/magma/max3d/MagmaMaxNodeExtension.hpp>

#include <frantic/magma/magma_data_type.hpp>
#include <frantic/magma/magma_exception.hpp>
#include <frantic/magma/magma_singleton.hpp>

#include <frantic/logging/logging_level.hpp>

#include <frantic/max3d/convert.hpp>

#pragma warning( push, 3 )
#if MAX_VERSION_MAJOR >= 14
#include <maxscript/maxscript.h>
#else
#include <MAXScrpt/MAXScrpt.h>
#endif
#pragma warning( pop )

#include <boost/noncopyable.hpp>
#include <boost/variant.hpp>

#include <memory>

namespace frantic {
namespace magma {
namespace max3d {

namespace {

/**
 * Helper function for returning a MAXScript value that cannot be represented properly via. Tab<T>
 */
inline Value* make_result( const std::vector<M_STD_STRING>& vals ) {
    one_typed_value_local( Array * result );

    try {
        vl.result = new Array( (int)vals.size() );

        for( std::vector<M_STD_STRING>::const_iterator it = vals.begin(), itEnd = vals.end(); it != itEnd; ++it )
            vl.result->append( new String( it->c_str() ) );

        return_value( vl.result );
    } catch( ... ) {
#if MAX_VERSION_MAJOR < 19
        pop_value_locals();
#endif
        throw;
    }
}

/**
 * Helper function for returning a MAXScript value that cannot be represented properly via. Tab<T>
 */
inline Value* make_result( const std::pair<IMagmaHolder::magma_id, int>& val ) {
    one_typed_value_local( Array * result );

    try {
        vl.result = new Array( 2 );
        vl.result->append( Integer::intern( val.first ) );      // Not plus one because it is an opaque ID.
        vl.result->append( Integer::intern( val.second + 1 ) ); // Plus one because its an index and MXS is 1-based.

        return_value( vl.result );
    } catch( ... ) {
#if MAX_VERSION_MAJOR < 19
        pop_value_locals();
#endif
        throw;
    }
}

} // namespace

class MagmaHolderPBAccessor : public PBAccessor {
  public:
#if MAX_VERSION_MAJOR >= 24
    virtual MSTR GetLocalName( ReferenceMaker* owner, ParamID id, int tabIndex, bool localized )
#else
    virtual MSTR GetLocalName( ReferenceMaker* owner, ParamID id, int tabIndex )
#endif
    {
        if( id == MagmaHolder::kMagmaNodeMaxData ) {
            MSTR result;

            IParamBlock2* pblock = static_cast<MagmaHolder*>( owner )->GetParamBlockByID( MagmaHolder::kPbMagmaHolder );
            if( !pblock )
                return result;

            IMagmaNode* nodeExt = GetMagmaNodeInterface( pblock->GetReferenceTarget( id, 0, tabIndex ) );
            if( !nodeExt )
                return result;

            magma_interface::magma_id id = nodeExt->get_id();

            MSTR typeName;

            try {
                typeName = static_cast<MagmaHolder*>( owner )->get_node_type( id );
            } catch( const frantic::magma::magma_exception& e ) {
                FF_LOG( error ) << e.get_message( true ) << std::flush;
            } catch( const std::exception& e ) {
                FF_LOG( error ) << e.what() << std::endl;
            }

            result.printf( _T("%s %d"), typeName.data(), id );

            return result;
        }
        return PBAccessor::GetLocalName( owner, id, tabIndex );
    }

    static MagmaHolderPBAccessor* GetInstance() {
        static MagmaHolderPBAccessor theMagmaHolderPBAccessor;
        return &theMagmaHolderPBAccessor;
    }
};

MagmaHolderClassDesc::MagmaHolderClassDesc()
    : m_pbDesc( MagmaHolder::kPbMagmaHolder, _T("Parameters"), 0, NULL, P_AUTO_CONSTRUCT + P_VERSION, 0, 0, p_end )
    , m_fpDesc( MagmaHolder_INTERFACE, _T("Magma"), 0, NULL, FP_MIXIN, p_end )
    , m_fpDescDeprecated( MagmaHolderDeprecated_INTERFACE, _T("MagmaDeprecated"), 0, NULL, FP_MIXIN, p_end ) {
    m_pbDesc.SetClassDesc( this );
    m_fpDesc.SetClassDesc( this );
    m_fpDescDeprecated.SetClassDesc( this );

    IMagmaHolder::init_fpinterface_desc( m_fpDesc );
    IMagmaHolderDeprecated::init_fpinterface_desc( m_fpDescDeprecated );

    m_pbDesc.AddParam( MagmaHolder::kMagmaFlow, _T("flow"), TYPE_STRING, P_RESET_DEFAULT, 0, p_end );
    m_pbDesc.ParamOption( MagmaHolder::kMagmaFlow, p_default, _T("#()"), p_end );

    m_pbDesc.AddParam( MagmaHolder::kMagmaInternalFlow, _T("internalFlow"), TYPE_STRING, P_RESET_DEFAULT, 0, p_end );
    m_pbDesc.ParamOption( MagmaHolder::kMagmaInternalFlow, p_default, _T("#()"), p_end );

    m_pbDesc.AddParam( MagmaHolder::kMagmaNote, _T("note"), TYPE_STRING, P_RESET_DEFAULT, 0, p_end );
    m_pbDesc.ParamOption( MagmaHolder::kMagmaNote, p_default, _T(""), p_end );

    m_pbDesc.AddParam( MagmaHolder::kMagmaTrackID, _T("trackID"), TYPE_STRING, P_RESET_DEFAULT, 0, p_end );

    m_pbDesc.AddParam( MagmaHolder::kMagmaTextureMapSources, _T("TextureMapSources"), TYPE_TEXMAP_TAB, 0,
                       P_RESET_DEFAULT | P_VARIABLE_SIZE, 0, p_end );

    m_pbDesc.AddParam( MagmaHolder::kMagmaCurrentPreset, _T("currentPreset"), TYPE_STRING, P_RESET_DEFAULT, 0, p_end );

    m_pbDesc.AddParam( MagmaHolder::kMagmaCurrentFolder, _T("currentFolder"), TYPE_STRING, P_RESET_DEFAULT, 0, p_end );

    m_pbDesc.AddParam( MagmaHolder::kMagmaAutoUpdate, _T("autoUpdate"), TYPE_BOOL, P_RESET_DEFAULT, 0, p_end );

    m_pbDesc.AddParam( MagmaHolder::kMagmaInteractiveMode, _T("interactiveMode"), TYPE_BOOL, P_RESET_DEFAULT, 0,
                       p_end );

    m_pbDesc.AddParam( MagmaHolder::kMagmaAutomaticRenameOFF, _T("AutomaticRenameOFF"), TYPE_BOOL, P_RESET_DEFAULT, 0,
                       p_end );

    m_pbDesc.AddParam( MagmaHolder::kMagmaGeometryObjectsList, _T("GeometryObjectsList"), TYPE_INODE_TAB, 0,
                       P_RESET_DEFAULT | P_VARIABLE_SIZE, 0, p_end );

    m_pbDesc.AddParam( MagmaHolder::kMagmaNeedCAUpdate, _T("needCAupdate"), TYPE_BOOL, P_RESET_DEFAULT, 0, p_end );

    m_pbDesc.AddParam( MagmaHolder::kMagmaIsRenderElement, _T("isRenderElement"), TYPE_BOOL, P_RESET_DEFAULT, 0,
                       p_end );

    m_pbDesc.AddParam( MagmaHolder::kMagmaLastErrorMessage, _T("lastErrorMessage"), TYPE_STRING, P_RESET_DEFAULT, 0,
                       p_end );
    m_pbDesc.ParamOption( MagmaHolder::kMagmaLastErrorMessage, p_default, _T(""), p_end );

    m_pbDesc.AddParam( MagmaHolder::kMagmaLastErrorNode, _T("lastErrorNode"), TYPE_INDEX, P_RESET_DEFAULT, 0, p_end );
    m_pbDesc.ParamOption( MagmaHolder::kMagmaLastErrorNode, p_default,
                          -2 /*minus 2 so it'll look like -1 to MXS. Fix this if we change to TYPE_INT*/, p_end );

    m_pbDesc.AddParam( MagmaHolder::kMagmaNodeMaxData, _T(""), TYPE_REFTARG_TAB, 0, P_SUBANIM | P_COMPUTED_NAME, 0,
                       p_end );
    m_pbDesc.ParamOption( MagmaHolder::kMagmaNodeMaxData, p_accessor, MagmaHolderPBAccessor::GetInstance(), p_end );
}

MagmaHolderClassDesc::~MagmaHolderClassDesc() {}

FPInterfaceDesc* MagmaHolderClassDesc::GetDescByID( Interface_ID id ) {
    if( id == MagmaHolder_INTERFACE )
        return &m_fpDesc;
    if( id == MagmaHolderDeprecated_INTERFACE )
        return &m_fpDescDeprecated;
    return NULL;
}

MagmaHolder::MagmaHolder( std::unique_ptr<frantic::magma::magma_interface> magmaInterface )
    : m_magma( magmaInterface.release() )
    , m_isLoading( false ) {}

MagmaHolder::~MagmaHolder() {}

void MagmaHolder::reset( std::unique_ptr<frantic::magma::magma_interface> magmaInterface ) {
    m_magma.reset( magmaInterface.release() );
    m_extensionProps.clear();
}

FPInterfaceDesc* MagmaHolder::GetDescByID( Interface_ID id ) {
    MagmaHolderClassDesc* cd = static_cast<MagmaHolderClassDesc*>( this->GetClassDesc() );
    FPInterfaceDesc* desc = cd->GetDescByID( id );
    if( desc ) {
        return desc;
    }
    return static_cast<FPInterfaceDesc*>( FPMixinInterface::GetDescByID( id ) );
}

BaseInterface* MagmaHolder::GetInterface( Interface_ID id ) {
    if( id == MagmaHolder_INTERFACE )
        return static_cast<IMagmaHolder*>( this );
    else if( id == MagmaHolderDeprecated_INTERFACE )
        return static_cast<IMagmaHolderDeprecated*>( this );
    else if( BaseInterface* bi = FPMixinInterface::GetInterface( id ) )
        return bi;

    return GenericReferenceTarget<ReferenceTarget, MagmaHolder>::GetInterface( id );
}

RefResult MagmaHolder::NotifyRefChanged( const Interval& /*changeInt*/, RefTargetHandle hTarget, PartID& partID,
                                         RefMessage message, BOOL /*propagate*/ ) {
    if( hTarget == m_pblock ) {
        int tabIndex;
        ParamID param = m_pblock->LastNotifyParamID( tabIndex );

        switch( param ) {
        case kMagmaAutoUpdate:
            if( message == REFMSG_CHANGE ) {
                NotifyDependents( FOREVER, (PartID)param, REFMSG_MAGMALEGACYCHANGE );
                return m_pblock->GetInt( kMagmaAutoUpdate ) ? REF_SUCCEED : REF_STOP;
            }
            break;
        case kMagmaNodeMaxData:
            if( tabIndex < 0 )
                return REF_FAIL;

            if( message == REFMSG_CHANGE && ( partID & ( PART_OBJ | PART_TM ) ) )
                return m_pblock->GetInt( kMagmaAutoUpdate ) ? REF_SUCCEED : REF_STOP;

            break;
        case kMagmaLastErrorMessage:
        case kMagmaLastErrorNode:
            if( message == REFMSG_CHANGE )
                return REF_STOP;
            break;
        case kMagmaAutomaticRenameOFF:
            if( message == REFMSG_CHANGE )
                NotifyDependents( FOREVER, (PartID)param, REFMSG_MAGMALEGACYCHANGE );
            break;
        };
    }

    return REF_DONTCARE;
}

void MagmaHolder::BaseClone( ReferenceTarget* from, ReferenceTarget* to, RemapDir& remap ) {
    if( !to || !from || to == from )
        return;

    if( !this->is_read_only() ) {
        // This will clone the parameter block and all the associated data.
        GenericReferenceTarget<ReferenceTarget, MagmaHolder>::BaseClone( from, to, remap );

        // Clone the node heirarchy
        static_cast<MagmaHolder*>( to )->m_magma = static_cast<MagmaHolder*>( from )->m_magma->clone();

        // Clone any scripter added properties
        static_cast<MagmaHolder*>( to )->m_extensionProps = static_cast<MagmaHolder*>( from )->m_extensionProps;

        // Remap each IMagmaNode to the associated node in the new node heirarchy.
        static_cast<MagmaHolder*>( to )->FixRefsPLCBImpl();
    } else {
        // Skip base cloning everything except the superclass stuff
        ReferenceTarget::BaseClone( from, to, remap );

        FF_LOG( error ) << _T("Genome modifier could not be cloned") << std::endl;
    }
}

int MagmaHolder::get_magma_max_node_index( magma_id id, bool forInsert ) {
    int i = 0, iEnd = m_pblock->Count( kMagmaNodeMaxData );

    for( ; i < iEnd; ++i ) {
        IMagmaNode* node = GetMagmaNodeInterface( m_pblock->GetReferenceTarget( kMagmaNodeMaxData, 0, i ) );
        if( node && node->get_id() == id )
            return i;
    }

    if( forInsert ) {
        if( i != iEnd )
            THROW_MAGMA_INTERNAL_ERROR();
        return iEnd;
    } else {
        return -1;
    }
}

IMagmaNode* MagmaHolder::get_magma_max_node( magma_id id ) {
    int index = get_magma_max_node_index( id, false );
    if( index >= 0 )
        return GetMagmaNodeInterface( m_pblock->GetReferenceTarget( kMagmaNodeMaxData, 0, index ) );
    return NULL;
}

void MagmaHolder::manage_magma_max_node( magma_id id ) {
    ReferenceTarget* maxNode = NULL;
    if( m_magma->get_property<ReferenceTarget*>( id, _T("_maxImpl"), maxNode ) ) {
        if( !maxNode ) {
            Class_ID classID;
            if( !m_magma->get_property<Class_ID>( id, _T("_maxImplClassID"), classID ) )
                THROW_MAGMA_INTERNAL_ERROR();

            maxNode = (ReferenceTarget*)CreateInstance( REF_TARGET_CLASS_ID, classID );

            if( !m_magma->set_property<ReferenceTarget*>( id, _T("_maxImpl"), maxNode ) )
                THROW_MAGMA_INTERNAL_ERROR();
        }

        GetMagmaNodeInterface( maxNode )->set_id( id );

        int index = get_magma_max_node_index( id, true );

        m_pblock->Insert( kMagmaNodeMaxData, index, 1, &maxNode );
    }
}

void MagmaHolder::replace_magma_max_node( magma_id idDest, magma_id idSrc ) {
    assert( idDest >= 0 );
    assert( idDest != idSrc );

    int indexSrc = get_magma_max_node_index( idSrc );
    if( indexSrc < 0 ) {
        int indexDest = get_magma_max_node_index( idDest );
        if( indexDest >= 0 )
            m_pblock->Delete( kMagmaNodeMaxData, indexDest, 1 );
    } else {
        ReferenceTarget* rtarg = m_pblock->GetReferenceTarget( kMagmaNodeMaxData, 0, indexSrc );

        assert( rtarg != NULL );

        int indexDest = get_magma_max_node_index( idDest );
        if( indexDest >= 0 ) {
            m_pblock->SetValue( kMagmaNodeMaxData, 0, rtarg, indexDest );
        } else {
            indexDest = get_magma_max_node_index( idDest, true );
            if( indexDest <= indexSrc )
                ++indexSrc;

            m_pblock->Insert( kMagmaNodeMaxData, indexDest, 1, &rtarg );
        }

        // Make sure to delete after inserting it elsewhere so the ref count doesn't go to 0.
        m_pblock->Delete( kMagmaNodeMaxData, indexSrc, 1 );

        // Update the id stored in the moved node.
        GetMagmaNodeInterface( rtarg )->set_id( idDest );
    }
}

boost::shared_ptr<magma_interface> MagmaHolder::get_magma_interface() { return m_magma; }

Interval MagmaHolder::get_validity( TimeValue t ) const {
    Interval result = FOREVER;

    int numRefs = m_pblock->Count( kMagmaNodeMaxData );
    for( int i = 0; i < numRefs; ++i ) {
        if( ReferenceTarget* rtarg = m_pblock->GetReferenceTarget( kMagmaNodeMaxData, 0, i ) )
            result &= GetMagmaNodeInterface( rtarg )->get_validity( t );
    }

    return result;
}

/**
 * These are done as macros so I could quickly change the exception handling of every method without having to
 * individually modify each one.
 * @note We don't catch MXS Exceptions, so those will be reported (for example, if a Value* is not the expected type).
 * This could potentially leave us in an undefined state. I haven't proven that all MXS exceptions happen before
 * modifying m_magma, but I suspect that is the case.
 */
#define MAGMA_TRY try
#ifdef _UNICODE
#define MAGMA_CATCH                                                                                                    \
    catch( const frantic::magma::magma_exception& e ) {                                                                \
        FF_LOG( error ) << e.get_message( true ) << std::flush;                                                        \
    }                                                                                                                  \
    catch( const std::exception& e ) {                                                                                 \
        std::wstring msg = frantic::strings::to_wstring( e.what() );                                                   \
        throw MAXException( const_cast<MCHAR*>( msg.c_str() ) );                                                       \
    }
#else
#define MAGMA_CATCH                                                                                                    \
    catch( const frantic::magma::magma_exception& e ) {                                                                \
        FF_LOG( error ) << e.get_message( true ) << std::flush;                                                        \
    }                                                                                                                  \
    catch( const std::exception& e ) {                                                                                 \
        throw MAXException( const_cast<MCHAR*>( e.what() ) );                                                          \
    }
#endif

bool MagmaHolder::is_read_only() const { return false; }

bool MagmaHolder::is_loading() const { return m_isLoading; }

void MagmaHolder::set_is_loading( bool loading ) { m_isLoading = loading; }

void MagmaHolder::reset() {
    if( !this->is_read_only() ) {
        MAGMA_TRY {
            m_extensionProps.clear();
            m_cachedStrings.clear();
            m_pblock->ZeroCount( kMagmaNodeMaxData );
            m_magma->clear();
        }
        MAGMA_CATCH;
    }
}

MagmaHolder::magma_id MagmaHolder::create_node( const MCHAR* typeName ) {
    magma_id result = magma_interface::INVALID_ID;

    if( this->is_read_only() )
        return result;

    MAGMA_TRY {
        result = m_magma->create_node( typeName );

        if( result != magma_interface::INVALID_ID )
            manage_magma_max_node( result );
        else if( this->is_loading() ) {
            result = m_magma->create_node(
                _T("Missing") ); // Make a missing node to stand in place of the node we couldn't create.
            m_magma->set_num_outputs( result, 1 );
        }
    }
    MAGMA_CATCH;

    return result;
}

bool MagmaHolder::delete_node( magma_id id ) { return replace_node( id, magma_interface::INVALID_ID ); }

void MagmaHolder::delete_node_recursive( magma_id nodeID ) {
    for( int i = 0, iEnd = m_magma->get_num_nodes( nodeID ); i < iEnd; ++i )
        delete_node_recursive( m_magma->get_id( nodeID, i ) );

    replace_magma_max_node( nodeID, frantic::magma::magma_interface::INVALID_ID );

    m_extensionProps.erase( nodeID );
}

bool MagmaHolder::replace_node( magma_id idDest, magma_id idSrc ) {
    bool result = false;

    if( this->is_read_only() )
        return result;

    MAGMA_TRY {
        if( idDest != idSrc && idDest >= 0 ) {
            if( idSrc == frantic::magma::magma_interface::INVALID_ID ) {
                /*m_magma->delete_node( idDest );
                replace_magma_max_node( idDest, frantic::magma::magma_interface::INVALID_ID );
                m_extensionProps.erase( idDest );*/
                delete_node_recursive( idDest );

                m_magma->delete_node( idDest );
            } else {
                delete_node_recursive( idDest );

                m_magma->replace_node( idDest, idSrc );

                replace_magma_max_node( idDest, idSrc );

                ext_prop_map_type::iterator itSrc = m_extensionProps.find( idSrc );
                if( itSrc != m_extensionProps.end() ) {
                    m_extensionProps[idDest].swap( itSrc->second );
                    m_extensionProps.erase( itSrc );
                } else {
                    m_extensionProps.erase( idDest );
                }
            }

            result = true;
        }
    }
    MAGMA_CATCH;

    return result;
}

const MCHAR* MagmaHolder::get_node_type( magma_id id ) const {
    const MCHAR* result = _T("");

    MAGMA_TRY { result = m_magma->get_type( id ).c_str(); }
    MAGMA_CATCH;

    return result;
}

bool MagmaHolder::is_node_container( magma_id id ) const {
    bool result = false;

    MAGMA_TRY { result = m_magma->is_container_node( id ); }
    MAGMA_CATCH;

    return result;
}

bool MagmaHolder::is_node_creatable( magma_id id ) const {
    bool result = false;

    MAGMA_TRY {
        if( const magma_node_type* nodeType = m_magma->get_singleton()->get_named_node_type( m_magma->get_type( id ) ) )
            result = nodeType->is_public() || nodeType->get_name() == _T("Missing");
    }
    MAGMA_CATCH;

    return result;
}

Tab<const MCHAR*> MagmaHolder::get_node_property_names( magma_id id, TYPE_ENUM_TYPE propType ) const {
    Tab<const MCHAR*> result;

    // NOTE: We may need to change the implementation if magma_interface::get_property_name() is changed to return a
    // copy instead of a reference.
    MAGMA_TRY {
        if( propType == property_type::all || propType == property_type::builtin ) {
            for( int i = 0, iEnd = m_magma->get_num_properties( id ); i < iEnd; ++i ) {
                const MCHAR* name = m_magma->get_property_name( id, i ).c_str();

                result.Append( 1, &name );
            }
        }

        if( propType == property_type::all || propType == property_type::userdefined ) {
            if( id == CURRENT_BLOP && m_magma->getBLOPStackDepth() > 0 ) {
                // A bit of a hack here ... whoops.
                id = m_magma->popBLOP();
                m_magma->pushBLOP( id );
            }

            ext_prop_map_type::const_iterator it = m_extensionProps.find( id );
            if( it != m_extensionProps.end() ) {
                for( ext_prop_map_type::mapped_type::const_iterator itProps = it->second.begin(),
                                                                    itPropsEnd = it->second.end();
                     itProps != itPropsEnd; ++itProps ) {
                    const MCHAR* name = itProps->first.c_str();

                    result.Append( 1, &name );
                }
            }
        }
    }
    MAGMA_CATCH;

    return result;
}

MSTR MagmaHolder::get_node_property_type( magma_id id, const MCHAR* propName ) const {
    MSTR result;

    MAGMA_TRY {
        const std::type_info& typeInfo = m_magma->get_property_type( id, propName );

        if( typeInfo == typeid( frantic::tstring ) )
            result = _T("String");
        else if( typeInfo == typeid( frantic::graphics::vector3f ) )
            result = _T("Point3");
        else if( typeInfo == typeid( Control* ) )
            result = _T("Control");
        else if( typeInfo == typeid( INode* ) )
            result = _T("Node");
        else if( typeInfo == typeid( Object* ) )
            result = _T("Object");
        else if( typeInfo == typeid( Texmap* ) )
            result = _T("Texmap");
        else if( typeInfo == typeid( Mtl* ) )
            result = _T("Material");
        else if( typeInfo == typeid( ReferenceTarget* ) )
            result = _T("ReferenceTarget");
        else if( typeInfo == typeid( frantic::magma::magma_data_type ) )
            result = _T("MagmaDataType");
        else if( typeInfo == typeid( std::vector<frantic::tstring> ) )
            result = _T("Tab<String>");
        else if( typeInfo == typeid( std::vector<INode*> ) )
            result = _T("Tab<Node>");
        else
            result = frantic::strings::to_tstring( typeInfo.name() ).c_str();
    }
    MAGMA_CATCH;

    return result;
}

bool MagmaHolder::get_node_property_readonly( magma_id id, const MCHAR* propName ) const {
    bool result = false;

    MAGMA_TRY { result = !m_magma->get_property_writable( id, propName ); }
    MAGMA_CATCH;

    return result;
}

bool MagmaHolder::add_node_property( magma_id id, const MCHAR* _propName ) {
    bool result = false;

    if( this->is_read_only() )
        return result;

    MAGMA_TRY {
        frantic::tstring propName( _propName );

        if( m_magma->is_valid_node( id ) && m_magma->get_property_type( id, propName ) == typeid( void ) ) {
            ext_prop_map_type::iterator it = m_extensionProps.find( id );
            if( it == m_extensionProps.end() )
                it = m_extensionProps.insert( std::make_pair( id, ext_prop_map_type::mapped_type() ) ).first;

            // Make sure there isn't aleady an extension property with this name.
            if( it->second.find( propName ) == it->second.end() ) {
                FPValue defaultValue;
                defaultValue.LoadPtr( TYPE_VALUE, &undefined );

                it->second.insert( std::make_pair( propName, defaultValue ) );

                result = true;
            }
        }
    }
    MAGMA_CATCH;

    return result;
}

FPValue MagmaHolder::get_node_property( magma_id id, const MCHAR* propName ) const {
    FPValue result;
    result.LoadPtr( TYPE_VALUE, &undefined );

    if( !propName || propName[0] == _T( '_' ) )
        return result;

    MAGMA_TRY {
        const std::type_info& type = m_magma->get_property_type( id, propName );

        if( type == typeid( void ) ) {
            ext_prop_map_type::const_iterator it = m_extensionProps.find( id );
            if( it != m_extensionProps.end() ) {
                ext_prop_map_type::mapped_type::const_iterator itProp = it->second.find( propName );
                if( itProp != it->second.end() )
                    result = itProp->second;
            }
        } else if( type == typeid( float ) ) {
            float f;
            m_magma->get_property( id, propName, f );

            result.LoadPtr( TYPE_FLOAT, f );
            // frantic::max3d::fpwrapper::MaxTypeTraits<float>::set_fpvalue( f, result );
        } else if( type == typeid( int ) ) {
            int i;
            m_magma->get_property( id, propName, i );

            result.LoadPtr( TYPE_INT, i );
            // frantic::max3d::fpwrapper::MaxTypeTraits<int>::set_fpvalue( i, result );
        } else if( type == typeid( std::vector<int> ) ) {
            std::vector<int> vals;
            m_magma->get_property( id, propName, vals );

            Tab<int> valsTab;
            if( !vals.empty() )
                valsTab.Append( (int)vals.size(), &vals.front() );

            result.LoadPtr( TYPE_INT_TAB_BV, &valsTab );
        } else if( type == typeid( bool ) ) {
            bool b;
            m_magma->get_property( id, propName, b );

            // frantic::max3d::fpwrapper::MaxTypeTraits<bool>::set_fpvalue( b, result );
            result.LoadPtr( TYPE_bool, b );
        } else if( type == typeid( frantic::tstring ) ) {
            frantic::tstring s;
            m_magma->get_property( id, propName, s );

            MSTR ts( s.c_str() );
            result.LoadPtr( TYPE_TSTR_BV, &ts );
        } else if( type == typeid( std::vector<frantic::tstring> ) ) {
            // std::vector<frantic::tstring> vals;

            // if( m_magma->get_property( id, propName, vals ) )
            // result.LoadPtr( TYPE_VALUE, make_result( vals ) );
            m_cachedStrings.clear();
            m_magma->get_property( id, propName, m_cachedStrings );

            Tab<const MCHAR*> vals;
            get_cached_strings( vals );

            result.LoadPtr( TYPE_STRING_TAB_BV, &vals );
        } else if( type == typeid( frantic::graphics::vector3f ) ) {
            frantic::graphics::vector3f p;
            m_magma->get_property( id, propName, p );

            Point3 pMax = frantic::max3d::to_max_t( p );

            result.LoadPtr( TYPE_POINT3_BV, &pMax );
            // frantic::max3d::fpwrapper::MaxTypeTraits<Point3>::set_fpvalue( frantic::max3d::to_max_t(p), result );
        } else if( type == typeid( frantic::magma::magma_data_type ) ) {
            frantic::magma::magma_data_type type;
            m_magma->get_property( id, propName, type );

            TSTR ts( type.to_string().c_str() );
            result.LoadPtr( TYPE_TSTR_BV, &ts );
        } else if( type == typeid( Control* ) ) {
            Control* c = NULL;

            if( m_magma->get_property( id, propName, c ) )
                result.LoadPtr( TYPE_CONTROL, c );
        } else if( type == typeid( INode* ) ) {
            INode* n = NULL;

            if( m_magma->get_property( id, propName, n ) )
                result.LoadPtr( TYPE_INODE, n );
        } else if( type == typeid( std::vector<INode*> ) ) {
            std::vector<INode*> rawNodes;

            if( m_magma->get_property( id, propName, rawNodes ) ) {
                Tab<INode*> nodes;

                if( !rawNodes.empty() )
                    nodes.Append( (int)rawNodes.size(), &rawNodes.front() );

                result.LoadPtr( TYPE_INODE_TAB_BV, &nodes );
            }
        } else if( type == typeid( Mtl* ) ) {
            Mtl* m = NULL;

            if( m_magma->get_property( id, propName, m ) )
                result.LoadPtr( TYPE_MTL, m );
        } else if( type == typeid( Texmap* ) ) {
            Texmap* t = NULL;

            if( m_magma->get_property( id, propName, t ) )
                result.LoadPtr( TYPE_TEXMAP, t );
        } else if( type == typeid( Object* ) ) {
            Object* o = NULL;

            if( m_magma->get_property( id, propName, o ) )
                result.LoadPtr( TYPE_OBJECT, o );
        } else if( type == typeid( ReferenceTarget* ) ) {
            ReferenceTarget* r = NULL;

            if( m_magma->get_property( id, propName, r ) )
                result.LoadPtr( TYPE_REFTARG, r );
        } else if( type == typeid( Value* ) ) {
            Value* v = &undefined;

            if( m_magma->get_property( id, propName, v ) )
                result.LoadPtr( TYPE_VALUE, v );
        } else if( type == typeid( FPInterface* ) ) {
            FPInterface* fpi = NULL;

            if( m_magma->get_property( id, propName, fpi ) )
                result.LoadPtr( TYPE_INTERFACE, fpi );
        } else if( type == typeid( IObject* ) ) {
            IObject* obj = NULL;

            if( m_magma->get_property( id, propName, obj ) )
                result.LoadPtr( TYPE_IOBJECT, obj );
        }
    }
    MAGMA_CATCH;

    return result;
}

bool MagmaHolder::set_node_property( magma_id id, const MCHAR* propName, const FPValue& _val ) {
    bool result = false;

    // We allow property changes in read-only mode for now. In reality we probably want selective property changes.
    // if( this->is_read_only() )
    //	return result;

    FPValue val = _val;
    if( val.type == TYPE_FPVALUE )
        val = TYPE_FPVALUE_BV_FIELD( val );

    MAGMA_TRY {
        const std::type_info& type = m_magma->get_property_type( id, propName );

        if( type == typeid( void ) ) {
            ext_prop_map_type::iterator it = m_extensionProps.find( id );
            if( it != m_extensionProps.end() ) {
                ext_prop_map_type::mapped_type::iterator itProp = it->second.find( propName );
                if( itProp != it->second.end() ) {
                    if( val.type == TYPE_STRING ) {
                        // TYPE_STRING values need to be converted to TSTR because we need a copy. Should do this for
                        // other ptr-passed types too I think (like Point3*, etc.)
                        MSTR s( TYPE_STRING_FIELD( val ) );
                        val.LoadPtr( TYPE_TSTR_BV, &s );
                    }

                    itProp->second = val;
                    result = true;
                }
            }
        } else if( type == typeid( std::vector<frantic::tstring> ) && ( val.type & ( TYPE_STRING | TYPE_TSTR ) ) &&
                   !( val.type & TYPE_TAB ) ) {
            // Handle special case of assigning CSV values as a string (this is old, deprecated behavior).
            // We used to save string tabs as CSV, but now we actually want a string tab so I'm splitting them here.
            const MCHAR* str = ( val.type & TYPE_STRING ) ? TYPE_STRING_FIELD( val ) : TYPE_TSTR_FIELD( val ).data();
            const MCHAR* next = str;

            FF_LOG( warning ) << _T("MagmaHolder.SetProperty ") << id << _T(" ") << propName << _T(" \"") << str
                              << _T("\" is deprecated. Use an array of strings") << std::endl;

            std::vector<frantic::tstring> stringArray;
            if( str && *str != _T( '\0' ) ) {
            loopBegin : {
                while( *next != _T( '\0' ) && *next != _T( ',' ) )
                    ++next;
                stringArray.push_back( frantic::tstring( str, next ) );

                if( *next != _T( '\0' ) ) {
                    str = ++next;
                    goto loopBegin;
                }
            }
            }

            result = m_magma->set_property<std::vector<frantic::tstring>>( id, propName, stringArray );
        } else {
            // Receiving undefined can mean a number of things depending on the type.
            if( val.type == TYPE_VALUE && TYPE_VALUE_FIELD( val ) == &undefined ) {
                if( type == typeid( Control* ) )
                    val.LoadPtr( TYPE_CONTROL, NULL );
                else if( type == typeid( INode* ) )
                    val.LoadPtr( TYPE_INODE, NULL );
                else if( type == typeid( ReferenceTarget* ) )
                    val.LoadPtr( TYPE_REFTARG, NULL );
                else if( type == typeid( Texmap* ) )
                    val.LoadPtr( TYPE_TEXMAP, NULL );
            }

            // An empty array gets converted to an empty int array, so we look for that and set a NULL ptr of the
            // correct type.
            if( val.type == TYPE_INT_TAB &&
                ( TYPE_INT_TAB_FIELD( val ) == NULL || TYPE_INT_TAB_FIELD( val )->Count() == 0 ) ) {
                if( type == typeid( std::vector<INode*> ) )
                    val.LoadPtr( TYPE_INODE_TAB, NULL );
                if( type == typeid( std::vector<frantic::tstring> ) )
                    val.LoadPtr( TYPE_STRING_TAB, NULL );
            }

            switch( (int)val.type ) {
            case TYPE_FLOAT:
                result = m_magma->set_property<float>( id, propName, TYPE_FLOAT_FIELD( val ) );
                break;
            case TYPE_INT:
                result = m_magma->set_property<int>( id, propName, TYPE_INT_FIELD( val ) );
                break;
            case TYPE_INT_TAB: {
                std::vector<int> vals;

                TYPE_INT_TAB_TYPE tab = TYPE_INT_TAB_FIELD( val );

                if( tab )
                    vals.assign( tab->Addr( 0 ), tab->Addr( 0 ) + tab->Count() );

                result = m_magma->set_property<std::vector<int>>( id, propName, vals );
            } break;
            case TYPE_BOOL:
                result = m_magma->set_property<bool>( id, propName, TYPE_BOOL_FIELD( val ) != FALSE );
                break;
            case TYPE_bool:
                result = m_magma->set_property<bool>( id, propName, TYPE_bool_FIELD( val ) );
                break;
            case TYPE_POINT3_BV:
            case TYPE_POINT3:
                result = m_magma->set_property<frantic::graphics::vector3f>(
                    id, propName, frantic::max3d::from_max_t( TYPE_POINT3_FIELD( val ) ) );
                break;
            case TYPE_STRING:
                if( type == typeid( frantic::magma::magma_data_type ) ) {
                    frantic::magma::magma_data_type type;
                    frantic::tstring typeString = TYPE_STRING_FIELD( val );

                    if( const frantic::magma::magma_data_type* namedType =
                            m_magma->get_singleton()->get_named_data_type( typeString ) )
                        type = *namedType;
                    else if( typeString[0] != _T( '\0' ) )
                        boost::tie( type.m_elementType, type.m_elementCount ) =
                            frantic::channels::channel_data_type_and_arity_from_string(
                                frantic::strings::to_string( typeString ) );

                    result = m_magma->set_property<frantic::magma::magma_data_type>( id, propName, type );
                } else {
                    result = m_magma->set_property<frantic::tstring>( id, propName, TYPE_STRING_FIELD( val ) );
                }
                break;
            case TYPE_STRING_TAB:
            case TYPE_STRING_TAB_BV: {
#if MAX_VERSION_MAJOR < 15
                Tab<MCHAR*>* tab = TYPE_STRING_TAB_FIELD( val );
#else
                Tab<const MCHAR*>* tab = TYPE_STRING_TAB_FIELD( val );
#endif

                std::vector<frantic::tstring> vals;
                if( tab ) {
                    for( int i = 0, iEnd = tab->Count(); i < iEnd; ++i )
                        vals.push_back( frantic::tstring( *tab->Addr( i ) ) );
                }
                result = m_magma->set_property<std::vector<frantic::tstring>>( id, propName, vals );
                break;
            }
            case TYPE_TSTR_BV:
            case TYPE_TSTR:
                if( type == typeid( frantic::magma::magma_data_type ) ) {
                    frantic::magma::magma_data_type type;
                    frantic::tstring typeString = TYPE_TSTR_FIELD( val ).data();

                    if( const frantic::magma::magma_data_type* namedType =
                            m_magma->get_singleton()->get_named_data_type( typeString ) )
                        type = *namedType;
                    else if( typeString[0] != _T( '\0' ) )
                        boost::tie( type.m_elementType, type.m_elementCount ) =
                            frantic::channels::channel_data_type_and_arity_from_string(
                                frantic::strings::to_string( typeString ) );

                    result = m_magma->set_property<frantic::magma::magma_data_type>( id, propName, type );
                } else {
                    result = m_magma->set_property<frantic::tstring>( id, propName, TYPE_TSTR_FIELD( val ).data() );
                }
                break;
            case TYPE_CONTROL:
                result = m_magma->set_property<Control*>( id, propName, TYPE_CONTROL_FIELD( val ) );
                break;
            case TYPE_INODE:
                result = m_magma->set_property<INode*>( id, propName, TYPE_INODE_FIELD( val ) );
                break;
            case TYPE_INODE_TAB:
            case TYPE_INODE_TAB_BV: {
                std::vector<INode*> nodes;
                Tab<INode*>* tab = TYPE_INODE_TAB_FIELD( val );

                if( tab && tab->Count() > 0 )
                    nodes.assign( tab->Addr( 0 ), tab->Addr( 0 ) + tab->Count() );

                result = m_magma->set_property<std::vector<INode*>>( id, propName, nodes );
                break;
            }
            case TYPE_MTL:
                result = m_magma->set_property<Mtl*>( id, propName, TYPE_MTL_FIELD( val ) );
                break;
            case TYPE_TEXMAP:
                result = m_magma->set_property<Texmap*>( id, propName, TYPE_TEXMAP_FIELD( val ) );
                break;
            case TYPE_OBJECT:
                result = m_magma->set_property<Object*>( id, propName, TYPE_OBJECT_FIELD( val ) );
                break;
            case TYPE_REFTARG:
                result = m_magma->set_property<ReferenceTarget*>( id, propName, TYPE_REFTARG_FIELD( val ) );
                break;
            case TYPE_VALUE:
                result = m_magma->set_property<Value*>( id, propName, TYPE_VALUE_FIELD( val ) );
                break;
            }

            // We only notify stuff if a REAL (non-extension) prop is changed
            if( result && m_pblock->GetInt( kMagmaAutoUpdate ) )
                NotifyDependents( FOREVER, (PartID)PART_ALL, REFMSG_CHANGE );
        }
    }
    MAGMA_CATCH;

    return result;
}

Tab<const MCHAR*> MagmaHolder::get_node_enum_values( magma_id id, const MCHAR* propName ) const {
    m_cachedStrings.clear();

    MAGMA_TRY { m_magma->get_property_accepted_values( id, propName, m_cachedStrings ); }
    MAGMA_CATCH;

    Tab<const MCHAR*> result;
    get_cached_strings( result );

    return result;
}

int MagmaHolder::get_num_node_inputs( magma_id id ) const {
    int result = 0;

    MAGMA_TRY { result = m_magma->get_num_inputs( id ); }
    MAGMA_CATCH;

    return result;
}

bool MagmaHolder::set_num_node_inputs( magma_id id, int numInputs ) {
    bool result = false;

    if( this->is_read_only() )
        return result;

    MAGMA_TRY {
        result = ( m_magma->set_num_inputs( id, numInputs ), true ); // Change when set_num_inputs returns true/false.

        if( result && m_pblock->GetInt( kMagmaAutoUpdate ) )
            NotifyDependents( FOREVER, (PartID)PART_ALL, REFMSG_CHANGE );
    }
    MAGMA_CATCH;

    return result;
}

namespace {
class to_fpvalue : public boost::static_visitor<>, boost::noncopyable {
    FPValue result;

  public:
    FPValue& get_result() { return result; }

    void operator()( boost::blank ) { result.LoadPtr( TYPE_VALUE, &undefined ); }

    void operator()( int val ) { result.LoadPtr( TYPE_INT, TYPE_INT_RSLT val ); }

    void operator()( float val ) { result.LoadPtr( TYPE_FLOAT, TYPE_FLOAT_RSLT val ); }

    void operator()( bool val ) { result.LoadPtr( TYPE_bool, TYPE_bool_RSLT val ); }

    void operator()( const frantic::graphics::vector3f& val ) {
        Point3 p = frantic::max3d::to_max_t( val );
        result.LoadPtr( TYPE_POINT3_BV, &p );
    }

    void operator()( const frantic::graphics::quat4f& val ) {
        Quat q = frantic::max3d::to_max_t( val );
        result.LoadPtr( TYPE_QUAT_BV, &q );
    }
};
} // namespace

FPValue MagmaHolder::get_node_input_default_value( magma_id id, index_type socketIndex ) const {
    to_fpvalue visitor;

    MAGMA_TRY { boost::apply_visitor( visitor, m_magma->get_input_default_value( id, socketIndex ) ); }
    MAGMA_CATCH;

    return visitor.get_result();
}

bool MagmaHolder::set_node_input_default_value( magma_id id, index_type socketIndex, const FPValue& _val ) {
    bool result = false;

    // We allow default value changes in read-only mode ... This might be wrongish.
    // if( this->is_read_only() )
    //	return result;

    FPValue val = _val;
    if( val.type == TYPE_FPVALUE )
        val = *TYPE_FPVALUE_FIELD( val );

    MAGMA_TRY {
        // Strip out the ByValue flag since it doesn't affect us in any way.
        switch( ( val.type & ~TYPE_BY_VAL ) ) {
        case TYPE_INT:
            m_magma->set_input_default_value( id, socketIndex, frantic::magma::variant_t( TYPE_INT_FIELD( val ) ) );
            break;
        case TYPE_FLOAT:
            m_magma->set_input_default_value( id, socketIndex, frantic::magma::variant_t( TYPE_FLOAT_FIELD( val ) ) );
            break;
        case TYPE_BOOL:
            m_magma->set_input_default_value( id, socketIndex,
                                              frantic::magma::variant_t( TYPE_BOOL_FIELD( val ) != FALSE ) );
            break;
        case TYPE_bool:
            m_magma->set_input_default_value( id, socketIndex, frantic::magma::variant_t( TYPE_bool_FIELD( val ) ) );
            break;
        case TYPE_POINT3:
            m_magma->set_input_default_value(
                id, socketIndex, frantic::magma::variant_t( frantic::max3d::from_max_t( TYPE_POINT3_FIELD( val ) ) ) );
            break;
        case TYPE_QUAT:
            m_magma->set_input_default_value(
                id, socketIndex, frantic::magma::variant_t( frantic::max3d::from_max_t( TYPE_QUAT_FIELD( val ) ) ) );
            break;
        case TYPE_VALUE:
            if( TYPE_VALUE_FIELD( val ) == &undefined )
                m_magma->set_input_default_value( id, socketIndex, frantic::magma::variant_t() );
            break;
        }

        // TODO: Once set_input_default_value() returns true/false, capture that result.
        result = true;

        if( result && m_pblock->GetInt( kMagmaAutoUpdate ) )
            NotifyDependents( FOREVER, (PartID)PART_ALL, REFMSG_CHANGE );
    }
    MAGMA_CATCH;

    return result;
}

MSTR MagmaHolder::get_node_input_description( magma_id id, index_type socketIndex ) const {
    MSTR result;

    MAGMA_TRY {
        frantic::tstring desc;
        m_magma->get_input_description( id, socketIndex, desc );

        result = desc.c_str();
    }
    MAGMA_CATCH;

    return result;
}

Value* MagmaHolder::get_node_input( magma_id id, index_type index ) const {
    magma_id connID = magma_interface::INVALID_ID;
    int connSocket = 0;

    MAGMA_TRY { boost::tie( connID, connSocket ) = m_magma->get_input( id, index ); }
    MAGMA_CATCH;

    return make_result( std::make_pair( connID, connSocket ) );
}

bool MagmaHolder::set_node_input( magma_id id, index_type index, magma_id connectedID, index_type connectedIndex ) {
    bool result = false;

    if( this->is_read_only() )
        return result;

    MAGMA_TRY {
        result = ( m_magma->set_input( id, index, connectedID, connectedIndex ),
                   true ); // Change once set_input returns true/false.

        if( result && m_pblock->GetInt( kMagmaAutoUpdate ) )
            NotifyDependents( FOREVER, (PartID)PART_ALL, REFMSG_CHANGE );
    }
    MAGMA_CATCH;

    return result;
}

int MagmaHolder::get_num_node_outputs( magma_id id ) const {
    int result = 0;

    MAGMA_TRY { result = m_magma->get_num_outputs( id ); }
    MAGMA_CATCH;

    return result;
}

bool MagmaHolder::set_num_node_outputs( magma_id id, int numOutputs ) {
    bool result = false;

    if( this->is_read_only() )
        return result;

    MAGMA_TRY {
        result =
            ( m_magma->set_num_outputs( id, numOutputs ), true ); // Change once set_num_outputs() returns true/false.

        if( result && m_pblock->GetInt( kMagmaAutoUpdate ) )
            NotifyDependents( FOREVER, (PartID)PART_ALL, REFMSG_CHANGE );
    }
    MAGMA_CATCH;

    return result;
}

MSTR MagmaHolder::get_node_output_description( magma_id id, index_type socketIndex ) const {
    MSTR result;

    MAGMA_TRY {
        frantic::tstring desc;
        m_magma->get_output_description( id, socketIndex, desc );

        result = desc.c_str();
    }
    MAGMA_CATCH;

    return result;
}

MagmaHolder::magma_id_tab MagmaHolder::get_nodes() const {
    magma_id_tab result;

    MAGMA_TRY {
        // It seems more natural to have these go first.
        /*if( m_magma->getBLOPStackDepth() > 0 ){
                magma_id inputNode, outputNode;

                if( m_magma->get_property<magma_interface::magma_id>( magma_interface::CURRENT_EDITING,
        _T("_internal_input_id"), inputNode ) ) result.Append( 1, &inputNode );

                if( m_magma->get_property<magma_interface::magma_id>( magma_interface::CURRENT_EDITING,
        _T("_internal_output_id"), outputNode ) ) result.Append( 1, &outputNode );
        }*/

        int baseIndex = result.Count();
        int numNodes = m_magma->get_num_nodes( magma_interface::CURRENT_EDITING );

        result.SetCount( baseIndex + numNodes );

        for( int i = 0; i < numNodes; ++i )
            *result.Addr( baseIndex + i ) = m_magma->get_id( magma_interface::CURRENT_EDITING, i );
    }
    MAGMA_CATCH;

    return result;
}

MagmaHolder::magma_id MagmaHolder::get_container_source() const {
    magma_id result = magma_interface::INVALID_ID;

    MAGMA_TRY {
        result = m_magma->get_id( magma_interface::CURRENT_EDITING,
                                  -1 ); // get_id() with -1 returns the source node for a container.
    }
    MAGMA_CATCH;

    return result;
}

MagmaHolder::magma_id MagmaHolder::get_container_sink() const {
    magma_id result = magma_interface::INVALID_ID;

    MAGMA_TRY {
        result = m_magma->get_id( magma_interface::CURRENT_EDITING,
                                  -2 ); // get_id() with -2 returns the source node for a container.
    }
    MAGMA_CATCH;

    return result;
}

bool MagmaHolder::push_editable_BLOP( magma_id id ) {
    bool result = false;

    // NOTE: Not affected by read-only status.

    MAGMA_TRY {
        result = ( m_magma->pushBLOP( id ), true ); // Change once pushBLOP returns true/false.
    }
    MAGMA_CATCH;

    return result;
}

MagmaHolder::magma_id MagmaHolder::pop_editable_BLOP() {
    magma_id result = magma_interface::INVALID_ID;

    // NOTE: Not affected by read-only status.

    MAGMA_TRY { result = m_magma->popBLOP(); }
    MAGMA_CATCH;

    return result;
}

int MagmaHolder::num_editing_BLOPs() const {
    int result = 0;

    MAGMA_TRY { result = m_magma->getBLOPStackDepth(); }
    MAGMA_CATCH;

    return result;
}

MagmaHolder::magma_id_tab MagmaHolder::get_BLOP_stack() const {
    magma_id_tab result;

    MAGMA_TRY {
        for( int i = 0, iEnd = m_magma->getBLOPStackDepth(); i < iEnd; ++i ) {
            magma_id id = m_magma->getBLOPStackAtDepth( i );
            result.Append( 1, &id );
        }
    }
    MAGMA_CATCH;

    return result;
}

bool MagmaHolder::explode_blop( magma_id id ) {
    bool result = false;

    if( this->is_read_only() )
        return result;

    MAGMA_TRY {
        result = ( m_magma->explode_blop( id ), true ); // Change once explode_blop returns true/false.

        // Don't need to do this cause BLOPs don't have 3dsMax specific parts.
        // replace_magma_max_node( id, frantic::magma::magma_interface::INVALID_ID );

        m_extensionProps.erase( id );

        // Don't need to NotifyDependents because the modified flow should be equivalent.
    }
    MAGMA_CATCH;

    return result;
}

MagmaHolder::magma_id MagmaHolder::create_blop( const magma_id_tab& nodes ) {
    magma_id result = magma_interface::INVALID_ID;

    if( this->is_read_only() )
        return result;
    MAGMA_TRY {
        std::set<magma_id> nodeSet( nodes.Addr( 0 ), nodes.Addr( 0 ) + nodes.Count() );

        result = m_magma->create_blop( nodeSet );

        // Don't need to NotifyDependents because the modified flow should be equivalent
    }
    MAGMA_CATCH;

    return result;
}

Tab<const MCHAR*> MagmaHolder::get_type_names() const {
    Tab<const MCHAR*> result;

    MAGMA_TRY {
        const magma_singleton& global = *m_magma->get_singleton();

        m_cachedStrings.clear();
        for( std::size_t i = 0, iEnd = global.get_num_node_types(); i < iEnd; ++i ) {
            const magma_node_type& nodeType = *global.get_named_node_type( global.get_node_type_name( i ) );
            if( nodeType.is_public() )
                m_cachedStrings.push_back( m_magma->get_singleton()->get_node_type_name( i ) );
        }

        get_cached_strings( result );
    }
    MAGMA_CATCH;

    return result;
}

MSTR MagmaHolder::get_type_category( const MCHAR* typeName ) const {
    MSTR result;

    MAGMA_TRY {
        if( const frantic::magma::magma_node_type* type = m_magma->get_singleton()->get_named_node_type( typeName ) )
            result = type->get_category().c_str();
    }
    MAGMA_CATCH;

    return result;
}

MSTR MagmaHolder::get_type_description( const MCHAR* typeName ) const {
    MSTR result;

    MAGMA_TRY {
        if( const frantic::magma::magma_node_type* type = m_magma->get_singleton()->get_named_node_type( typeName ) )
            result = type->get_description().c_str();
    }
    MAGMA_CATCH;

    return result;
}

Value* MagmaHolder::deprecated_get_output( index_type index ) const {
    magma_id connID = magma_interface::INVALID_ID;
    int connSocket = 0;

    MAGMA_TRY {
        if( m_magma->getBLOPStackDepth() > 0 ) {
            boost::tie( connID, connSocket ) = m_magma->get_output( magma_interface::CURRENT_EDITING, index );

            // magma_interface::magma_id outputHolder;
            // if( m_magma->get_property<magma_interface::magma_id>( magma_interface::CURRENT_EDITING,
            // _T("_internal_output_id"), outputHolder ) ) 	boost::tie( connID, connSocket ) = m_magma->get_input(
            //outputHolder, index );
        }
    }
    MAGMA_CATCH;

    return make_result( std::make_pair( connID, connSocket ) );
}

bool MagmaHolder::deprecated_set_output( index_type index, magma_id connectedID, index_type connectedIndex ) {
    bool result = false;

    if( this->is_read_only() )
        return result;

    MAGMA_TRY {
        if( m_magma->getBLOPStackDepth() > 0 ) {
            magma_interface::magma_id outputHolder = m_magma->get_output( magma_interface::CURRENT_EDITING, -1 ).first;

            if( outputHolder != magma_interface::INVALID_ID )
                result = ( m_magma->set_input( outputHolder, index, connectedID, connectedIndex ), true );

            // magma_interface::magma_id outputHolder;
            // if( m_magma->get_property<magma_interface::magma_id>( magma_interface::CURRENT_EDITING,
            // _T("_internal_output_id"), outputHolder ) ) 	result = (m_magma->set_input( outputHolder, index,
            //connectedID, connectedIndex ), true);
        }

        if( result && m_pblock->GetInt( kMagmaAutoUpdate ) )
            NotifyDependents( FOREVER, (PartID)PART_ALL, REFMSG_CHANGE );
    }
    MAGMA_CATCH;

    return result;
}

bool MagmaHolder::deprecated_declare_extension_property( IMagmaHolder::magma_id id, const MCHAR* propName ) {
    return this->add_node_property( id, propName );
}

} // namespace max3d
} // namespace magma
} // namespace frantic
