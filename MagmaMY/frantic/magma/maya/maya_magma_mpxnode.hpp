// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <frantic/maya/PRTObject_base.hpp>
#include <maya/MPxNode.h>

namespace frantic {
namespace magma {
namespace maya {

class maya_magma_mpxnode : public MPxNode, public frantic::maya::PRTObjectBase {
  public:
    typedef void ( *AttributeChangeFunction )( maya_magma_mpxnode* caller, MNodeMessage::AttributeMessage msg,
                                               MPlug& plug, MPlug& otherPlug );

    // Maya rtti and object creation information
    static void* creator();
    static MStatus initialize();
    static const MTypeId id;
    static const MString typeName;

  private:
    // Input particles to be modified
    static MObject inParticleStream;

    // Magma
    static MObject inMayaMagma;

    // Output particles
    static MObject outParticleStream;

    // Enabled
    static MObject inEnabled;

  private:
    MCallbackId m_attribute_changed_event_id;
    std::vector<AttributeChangeFunction> m_event_handlers;

  public:
    maya_magma_mpxnode();
    virtual ~maya_magma_mpxnode();
    virtual void postConstructor();
    virtual MStatus compute( const MPlug& plug, MDataBlock& block );

    // inherited from base
    frantic::maya::PRTObjectBase::particle_istream_ptr
    getParticleStream( const frantic::graphics::transform4f& objectSpace, const MDGContext& context,
                       bool isViewport ) const;

    virtual frantic::maya::PRTObjectBase::particle_istream_ptr
    getRenderParticleStream( const frantic::graphics::transform4f& objectSpace, const MDGContext& context ) const {
        return getParticleStream( objectSpace, context, false );
    }

    virtual frantic::maya::PRTObjectBase::particle_istream_ptr
    getViewportParticleStream( const frantic::graphics::transform4f& objectSpace, const MDGContext& context ) const {
        return getParticleStream( objectSpace, context, true );
    }

    bool isEnabled( const MDGContext& context = MDGContext::fsNormal ) const;

    void addAttributeChangedEventHandler( AttributeChangeFunction func );

  private:
    static void attribute_changed_event( MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& other, void* data );
};

} // namespace maya
} // namespace magma
} // namespace frantic
