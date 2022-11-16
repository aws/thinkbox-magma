// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/magma_compiler_interface.hpp>
#include <frantic/magma/max3d/MagmaMaxContext.hpp>
#include <frantic/magma/max3d/nodes/magma_input_particles_node.hpp>
#include <frantic/magma/nodes/magma_node_impl.hpp>

#include <frantic/max3d/convert.hpp>
#include <frantic/max3d/particles/particle_stream_factory.hpp>

#include <frantic/channels/channel_map.hpp>
#include <frantic/graphics/camera.hpp>

#include <boost/scope_exit.hpp>

#include <memory>

using namespace frantic::max3d::particles;

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

MSTR magma_input_particles_node::max_impl::s_ClassName( _T("MagmaInputParticlesNode") );
// Class_ID magma_input_particles_node::max_impl::s_ClassID(0x52ef7e6a, 0x20452f80); //This needs to be defined in the
// DLL, not in this project because it must be different for Krakatoa, Genome, etc.

void magma_input_particles_node::max_impl::DefineParameters( ParamBlockDesc2& paramDesc ) {
    paramDesc.AddParam( kNode, _T("node"), TYPE_INODE, 0, 0, p_end );
}

RefResult magma_input_particles_node::max_impl::NotifyRefChanged( const Interval& /*changeInt*/,
                                                                  RefTargetHandle hTarget, PartID& /*partID*/,
                                                                  RefMessage message, BOOL /*propagate*/ ) {
    if( hTarget == m_pblock ) {
        if( message == REFMSG_CHANGE ) {
            if( m_pblock->LastNotifyParamID() == kNode )
                m_validInterval.SetEmpty();
            return REF_SUCCEED;
        }
    }
    return REF_DONTCARE;
}

int magma_input_particles_node::max_impl::RenderBegin( TimeValue /*t*/, ULONG /*flags*/ ) {
    m_inRenderMode = true;
    m_validInterval = NEVER;
    return TRUE;
}

int magma_input_particles_node::max_impl::RenderEnd( TimeValue /*t*/ ) {
    m_inRenderMode = false;
    m_validInterval = NEVER;
    return TRUE;
}

Interval magma_input_particles_node::max_impl::get_validity( TimeValue t ) const {
    Interval iv = MagmaMaxNodeExtension<max_impl>::get_validity( t );

    iv &= m_validInterval;

    if( !iv.InInterval( t ) )
        return Interval( t, t );
    return iv;
}

magma_input_particles_node::max_impl::max_impl() {
    m_inRenderMode = false;
    m_validInterval = NEVER;
}

bool magma_input_particles_node::max_impl::is_valid( TimeValue t ) { return m_validInterval.InInterval( t ) != FALSE; }

magma_input_particles_node::particle_array_ptr
magma_input_particles_node::max_impl::get_particles( TimeValue t, IMaxKrakatoaPRTEvalContextPtr pEvalContext ) {
    if( m_cachedParticles && m_validInterval.InInterval( t ) )
        return m_cachedParticles;

    m_validInterval.SetInfinite();
    m_cachedParticles.reset( new frantic::particles::particle_array );

    INode* node = m_pblock->GetINode( kNode, t );
    if( !node )
        return m_cachedParticles;

    FF_LOG( debug ) << _T("Rebuilding InputParticles cache for: ") << node->GetName() << std::endl;

    particle_istream_ptr pin =
        frantic::max3d::particles::max_particle_istream_factory( node, pEvalContext, m_validInterval );

    if( pin ) {
        frantic::channels::channel_map nativeMap = pin->get_native_channel_map();

        // Some particle sources are stupid (Looking at you KMX PRT Loader!), and return a completely empty channel_map.
        // This tends to cause grief, so I force at least one channel.
        if( nativeMap.channel_count() == 0 ) {
            nativeMap.reset();
            nativeMap.define_channel<vec3>( _T("Position") );
            nativeMap.end_channel_definition();
        }

        pin->set_channel_map( nativeMap );
        m_cachedParticles->reset( nativeMap );
        m_cachedParticles->insert_particles( pin );
    }

    return m_cachedParticles;
}

void magma_input_particles_node::compile_as_extension_type( frantic::magma::magma_compiler_interface& compiler ) {
    INode* node = get_node();
    if( !node )
        throw magma_exception() << magma_exception::node_id( get_id() ) << magma_exception::property_name( _T("node") )
                                << magma_exception::error_name( _T("Particle source node was undefined or deleted") );

    using frantic::graphics::vector3f;
    using frantic::max3d::particles::IMaxKrakatoaPRTObject;

    frantic::channels::channel_map channelMap;
    channelMap.define_channel<vector3f>( _T("Position") );
    channelMap.end_channel_definition();

    frantic::graphics::camera<float> theCamera;

    TimeValue t = TIME_NegInfinity;

    IMaxKrakatoaPRTEvalContextPtr evalCtxPtr;
    if( compiler.get_context_data().get_property( _T("IMaxKrakatoaPRTObject::EvalContext"), evalCtxPtr ) ) {
        theCamera = evalCtxPtr->GetCamera();
        t = evalCtxPtr->GetTime();
    } else {
        compiler.get_context_data().get_property( _T("Time"), t );
        compiler.get_context_data().get_property( _T("Camera"), theCamera );

        evalCtxPtr = frantic::max3d::particles::CreateMaxKrakatoaPRTEvalContext( t, Class_ID( 0x52ef7e6a, 0x20452f80 ),
                                                                                 &theCamera, &channelMap );
    }

    // std::unique_ptr<IMaxKrakatoaPRTObject::IEvalContext> ctx( IMaxKrakatoaPRTObject::CreateDefaultEvalContext(
    // channelMap, theCamera, t ) );

    max_impl& impl = *static_cast<max_impl*>( this->get_max_object() );
    if( &impl == NULL )
        THROW_MAGMA_INTERNAL_ERROR();

    if( !m_cachedParticles || !impl.is_valid( t ) ) {
        m_cachedParticles = impl.get_particles( t, evalCtxPtr );
        m_cachedKDTree.reset();
    }

    compiler.register_interface( get_id(),
                                 static_cast<frantic::magma::nodes::magma_input_particles_interface*>( this ) );
    compiler.compile_constant( get_id(), frantic::magma::variant_t( (int)m_cachedParticles->size() ) );
}

magma_input_particles_node::const_particle_array_ptr magma_input_particles_node::get_particles() const {
    return m_cachedParticles;
}

magma_input_particles_node::const_particle_kdtree_ptr magma_input_particles_node::get_particle_kdtree() {
    if( !m_cachedKDTree ) {
        m_cachedKDTree.reset( new frantic::particles::particle_kdtree<detail::particle_standin> );

        frantic::channels::channel_accessor<frantic::graphics::vector3f> posAccessor =
            m_cachedParticles->get_channel_map().get_accessor<frantic::graphics::vector3f>( _T("Position") );

        boost::int64_t counter = 0;
        for( frantic::particles::particle_array::const_iterator it = m_cachedParticles->begin(),
                                                                itEnd = m_cachedParticles->end();
             it != itEnd; ++it, ++counter )
            m_cachedKDTree->add_particle( detail::particle_standin( posAccessor.get( *it ), counter ) );
        m_cachedKDTree->balance_kdtree();
    }

    return m_cachedKDTree;
}

} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic

using frantic::magma::max3d::MagmaMaxNodeExtension;

template <>
MagmaMaxNodeExtension<frantic::magma::nodes::max3d::magma_input_particles_node::max_impl>::MyClassDesc
    MagmaMaxNodeExtension<frantic::magma::nodes::max3d::magma_input_particles_node::max_impl>::s_classDesc;

ClassDesc2* GetMagmaInputParticlesNodeDesc() {
    return &frantic::magma::nodes::max3d::magma_input_particles_node::max_impl::s_classDesc;
}
