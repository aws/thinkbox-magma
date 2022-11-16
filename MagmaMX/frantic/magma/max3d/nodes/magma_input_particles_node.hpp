// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/nodes/magma_input_particles_interface.hpp>
#include <frantic/magma/nodes/magma_node_property.hpp>

#include <frantic/max3d/particles/IMaxKrakatoaPRTObject.hpp>

#include <frantic/magma/max3d/MagmaMaxNodeExtension.hpp>

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

class magma_input_particles_node : public magma_max_input_node, public magma_input_particles_interface {
  public:
    enum {
        kNode = 1 // TODO: Make this multiple nodes
    };

    class max_impl : public MagmaMaxNodeExtension<max_impl> {
        Interval m_validInterval;
        particle_array_ptr m_cachedParticles;

        bool m_inRenderMode;

      public:
        static MSTR s_ClassName;
        static Class_ID s_ClassID;

        static void DefineParameters( ParamBlockDesc2& paramDesc );

        virtual RefResult NotifyRefChanged( const Interval& changeInt, RefTargetHandle hTarget, PartID& partID,
                                            RefMessage message, BOOL propagate );

        virtual int RenderBegin( TimeValue t, ULONG flags );
        virtual int RenderEnd( TimeValue t );

        max_impl();

        // From IMagmaNode
        virtual Interval get_validity( TimeValue ) const;

        bool is_valid( TimeValue t );
        particle_array_ptr get_particles( TimeValue t,
                                          frantic::max3d::particles::IMaxKrakatoaPRTEvalContextPtr pEvalContext );
    };

  private:
    particle_array_ptr m_cachedParticles;
    particle_kdtree_ptr m_cachedKDTree;

  public:
    MAGMA_MAX_REQUIRED_METHODS( magma_input_particles_node );
    MAGMA_MAX_PROPERTY( node, INode, kNode );

    virtual int get_num_outputs() const { return 2; }

    virtual const_particle_array_ptr get_particles() const;

    virtual const_particle_kdtree_ptr get_particle_kdtree();
};

} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic
