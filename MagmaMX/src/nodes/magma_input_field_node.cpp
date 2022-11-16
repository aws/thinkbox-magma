// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/max3d/nodes/magma_input_field_node.hpp>

#include <boost/function.hpp>

#pragma warning( push, 3 )
#include <max.h>
#pragma warning( pop )

// I hate whoever made this macro with the passion of 10 000 000 supernovae
#undef base_type

#include <frantic/magma/max3d/magma_max_node_impl.hpp>

#include <frantic/max3d/volumetrics/IEmberField.hpp>
#include <frantic/max3d/volumetrics/force_field_adapter.hpp>
#include <frantic/max3d/volumetrics/fumefx_field_factory.hpp>
#include <frantic/max3d/volumetrics/phoenix_field.hpp>

#include <frantic/files/filename_sequence.hpp>

#pragma warning( push, 3 )
#include <maxtypes.h>
#include <units.h>
#pragma warning( pop )

#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/tss.hpp>

#include <memory>

MSTR frantic::magma::nodes::max3d::magma_field_input_node::max_impl::s_ClassName( _T("EmberInputFieldNode") );

// This must be moved to the hosting plugin (and made unique!)
// Class_ID magma_field_input_node::max_impl::s_ClassID( 0x627a02bb, 0x13a2ea9 );

void frantic::magma::nodes::max3d::magma_field_input_node::max_impl::DefineParameters( ParamBlockDesc2& paramDesc ) {
    paramDesc.AddParam( kNode, _T("node"), TYPE_INODE, 0, 0, p_end );
}

void frantic::magma::nodes::max3d::magma_field_input_node::max_impl::SetReference( int i, RefTargetHandle rtarg ) {
    MagmaMaxNodeExtension<max_impl>::SetReference( i, rtarg );
    if( i == 0 && m_pblock ) // New pblock, so invalidate cache.
        m_cache.second.SetEmpty();
}

int frantic::magma::nodes::max3d::magma_field_input_node::max_impl::RenderBegin( TimeValue /*t*/, ULONG /*flags*/ ) {
    m_cache.second.SetEmpty();

    return TRUE;
}

int frantic::magma::nodes::max3d::magma_field_input_node::max_impl::RenderEnd( TimeValue /*t*/ ) {
    m_cache.second.SetEmpty();

    return TRUE;
}

IOResult frantic::magma::nodes::max3d::magma_field_input_node::max_impl::Load( ILoad* iload ) {
    IOResult r = MagmaMaxNodeExtension<max_impl>::Load( iload );

    m_cache.second.SetEmpty();

    return r;
}

RefResult frantic::magma::nodes::max3d::magma_field_input_node::max_impl::NotifyRefChanged(
    const Interval& /*changeInt*/, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL /*propagate*/ ) {
    if( hTarget == m_pblock && m_pblock != NULL ) {

        int index;
        ParamID paramID = m_pblock->LastNotifyParamID( index );

        if( message == REFMSG_CHANGE ) {
            if( paramID == kNode ) {
                if( partID & PART_GEOM )
                    m_cache.second.SetEmpty();
                return REF_SUCCEED;
            }
        }
    }

    return REF_DONTCARE;
}

frantic::magma::nodes::max3d::magma_field_input_node::max_impl::max_impl() {}

HMODULE GetCurrentModule() {
    HMODULE currHModule = NULL;
    GetModuleHandleEx( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)GetCurrentModule, &currHModule );
    return currHModule;
}

frantic::magma::nodes::max3d::magma_field_input_node::field_ptr
frantic::magma::nodes::max3d::magma_field_input_node::max_impl::get_field( TimeValue t ) {
    if( m_cache.first && m_cache.second.InInterval( t ) )
        return m_cache.first;

    m_cache.first.reset();
    m_cache.second.SetEmpty();

    INode* pNode = m_pblock->GetINode( kNode );
    if( pNode ) {
        // Do a manual conversion because then I have guaranteed access to the pipeline result's validity.
        ObjectState os = pNode->EvalWorldState( t );
        if( os.obj && os.obj->CanConvertToType( EmberPipeObject_CLASSID ) ) {
            Object* pPipeObj = os.obj->ConvertToType( t, EmberPipeObject_CLASSID );
            if( frantic::max3d::volumetrics::IEmberField* ember =
                    frantic::max3d::volumetrics::GetEmberFieldInterface( pPipeObj ) ) {
                m_cache.second = FOREVER; // Init to FOREVER so that create_field() will update it appropriately.
                m_cache.first = ember->create_field( pNode, t, m_cache.second );
            }

            if( pPipeObj != os.obj )
                pPipeObj->MaybeAutoDelete();
        } else if( frantic::max3d::volumetrics::is_forcefield_node( pNode, t ) ) {
            m_cache.first = frantic::max3d::volumetrics::get_force_field_adapter( pNode, t );
            m_cache.second.SetInstant( t );
#if defined( FUMEFX_SDK_AVAILABLE )
        } else if( frantic::max3d::volumetrics::is_fumefx_node( pNode, t ) ) {
            std::unique_ptr<frantic::max3d::volumetrics::fumefx_field_interface> fumeField =
                frantic::max3d::volumetrics::get_fumefx_field( pNode, t );

            m_cache.first.reset( fumeField.release() );
            m_cache.second.SetInstant( t );
#endif
#if defined( PHOENIX_SDK_AVAILABLE )
        } else if( frantic::max3d::volumetrics::is_phoenix_node( pNode, t ) ) {
            m_cache.first = frantic::max3d::volumetrics::get_phoenix_field( pNode, t );
            m_cache.second.SetInstant( t );
#endif
        }
    }

    return m_cache.first;
}

Interval frantic::magma::nodes::max3d::magma_field_input_node::max_impl::get_validity( TimeValue t ) const {
    if( !m_cache.second.InInterval( t ) )
        return Interval( t, t );
    return m_cache.second;
}

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {
MAGMA_DEFINE_MAX_TYPE( "InputField", "Input", magma_field_input_node )
MAGMA_EXPOSE_PROPERTY( node, INode* )
// MAGMA_EXPOSE_PROPERTY( fxdSequenceOverride, frantic::tstring )
MAGMA_EXPOSE_PROPERTY( channels, std::vector<frantic::tstring> )
MAGMA_READONLY_ARRAY_PROPERTY( availableChannels, frantic::tstring )
MAGMA_INPUT( "Position", boost::blank() )
MAGMA_DESCRIPTION( "Exposes a field from another scene object. Supported object types are: FumeFX, PhoenixFD, Stoke, "
                   "and WSM force fields." )
MAGMA_DEFINE_TYPE_END;
} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic

frantic::magma::nodes::max3d::magma_field_input_node::magma_field_input_node() {}

void frantic::magma::nodes::max3d::magma_field_input_node::set_channels(
    const std::vector<frantic::tstring>& channels ) {
    std::vector<frantic::tstring> nextChannels( channels );

    for( std::vector<frantic::tstring>::iterator it = nextChannels.begin(), itEnd = nextChannels.end(); it != itEnd;
         ++it ) {
        // If this channel exists in m_outputChannels we want to make sure to place it at the same index (if possible).
        std::vector<frantic::tstring>::const_iterator itOld =
            std::find( m_outputChannels.begin(), m_outputChannels.end(), *it );
        if( itOld != m_outputChannels.end() && ( it - nextChannels.begin() ) != ( itOld - m_outputChannels.begin() ) &&
            ( itOld - m_outputChannels.begin() ) < static_cast<std::ptrdiff_t>( nextChannels.size() ) )
            std::swap( *it, *( nextChannels.begin() + ( itOld - m_outputChannels.begin() ) ) );
    }

    m_outputChannels.swap( nextChannels );
}

void frantic::magma::nodes::max3d::magma_field_input_node::set_node( INode* node ) {
    INode* oldNode = this->get_node();

    this->set_max_property<INode>( kNode, node );

    if( oldNode != node && m_fxdSequenceOverride.empty() )
        on_field_changed();
}

void frantic::magma::nodes::max3d::magma_field_input_node::set_fxdSequenceOverride( const frantic::tstring& val ) {
    if( m_fxdSequenceOverride != val ) {
        m_fxdSequenceOverride = val;

        on_field_changed();
    }
}

frantic::magma::nodes::max3d::magma_field_input_node::field_ptr
frantic::magma::nodes::max3d::magma_field_input_node::get_field( TimeValue t ) const {
    magma_field_input_node::field_ptr theField;

    if( max_impl* impl = static_cast<max_impl*>( this->get_max_object() ) )
        theField = impl->get_field( t );

    return theField;
}

void frantic::magma::nodes::max3d::magma_field_input_node::get_availableChannels(
    std::vector<frantic::tstring>& outChannels ) const {
    field_ptr theField = get_field( GetCOREInterface()->GetTime() );

    if( theField ) {
        for( std::size_t i = 0, iEnd = theField->get_channel_map().channel_count(); i < iEnd; ++i )
            outChannels.push_back( theField->get_channel_map()[i].name() );
    }
}

void frantic::magma::nodes::max3d::magma_field_input_node::on_field_changed() {
    std::vector<frantic::tstring> availableChannels;
    this->get_availableChannels( availableChannels );
    this->set_channels( availableChannels );
}

int frantic::magma::nodes::max3d::magma_field_input_node::get_num_outputs() const {
    /*field_ptr theField = get_field( GetCOREInterface()->GetTime() );

    if( theField ){
            return std::max( 1, static_cast<int>( theField->get_channel_map().channel_count() ) );
    }
    return 1;*/
    return std::max( 1, static_cast<int>( m_outputChannels.size() ) );
}

void frantic::magma::nodes::max3d::magma_field_input_node::get_output_description(
    int i, frantic::tstring& outDescription ) const {
    /*field_ptr theField = get_field( GetCOREInterface()->GetTime() );

    if( theField && static_cast<std::size_t>( i ) < theField->get_channel_map().channel_count() )
            outDescription = theField->get_channel_map()[ static_cast<std::size_t>( i ) ].name();
    else
            outDescription = _T( "Invalid" );*/
    if( static_cast<std::size_t>( i ) < m_outputChannels.size() )
        outDescription = m_outputChannels[static_cast<std::size_t>( i )];
    else
        outDescription = _T("Invalid");
}

void frantic::magma::nodes::max3d::magma_field_input_node::compile_as_extension_type(
    frantic::magma::magma_compiler_interface& compiler ) {
    TimeValue t = TIME_NegInfinity;
    compiler.get_context_data().get_property( _T("Time"), t );

    field_ptr theField = get_field( t );

    if( !theField )
        throw frantic::magma::magma_exception()
            << frantic::magma::magma_exception::node_id( this->get_id() )
            << frantic::magma::magma_exception::error_name( _T("Not a valid field") );

    compiler.compile_field( this->get_id(), theField, this->get_input( 0 ), this->get_channels() );
}

using frantic::magma::max3d::MagmaMaxNodeExtension;

template <>
MagmaMaxNodeExtension<frantic::magma::nodes::max3d::magma_field_input_node::max_impl>::MyClassDesc
    MagmaMaxNodeExtension<frantic::magma::nodes::max3d::magma_field_input_node::max_impl>::s_classDesc;

namespace ember {
namespace max3d {

ClassDesc2* GetFieldNodeDesc() { return &frantic::magma::nodes::max3d::magma_field_input_node::max_impl::s_classDesc; }

} // namespace max3d
} // namespace ember
