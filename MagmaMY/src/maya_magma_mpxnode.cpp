// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "frantic/magma/maya/maya_magma_mpxnode.hpp"
#include "frantic/magma/maya/maya_magma_desc_mpxdata.hpp"
#include "frantic/magma/maya/maya_magma_mel_window.hpp"
#include "frantic/magma/maya/maya_magma_modifier.hpp"
#include "stdafx.h"

#include <frantic/channels/channel_map.hpp>
#include <frantic/magma/magma_exception.hpp>
#include <frantic/maya/MPxParticleStream.hpp>
#include <frantic/maya/PRTMayaParticle.hpp>
#include <frantic/maya/maya_util.hpp>
#include <frantic/maya/util.hpp>
#include <frantic/particles/streams/empty_particle_istream.hpp>

#include <maya/MFnNumericAttribute.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

using namespace frantic::maya;

namespace {

inline frantic::maya::PRTObjectBase::particle_istream_ptr getEmptyStream() {
    frantic::channels::channel_map lsChannelMap;
    lsChannelMap.define_channel( _T("Position"), 3, frantic::channels::data_type_float32 );
    lsChannelMap.end_channel_definition();

    return frantic::particles::streams::particle_istream_ptr(
        new frantic::particles::streams::empty_particle_istream( lsChannelMap ) );
}

} // namespace

namespace frantic {
namespace magma {
namespace maya {

const MTypeId maya_magma_mpxnode::id( 0x0011748e );
const MString maya_magma_mpxnode::typeName = "PRTMagma";

MObject maya_magma_mpxnode::inParticleStream;
MObject maya_magma_mpxnode::inMayaMagma;
MObject maya_magma_mpxnode::outParticleStream;
MObject maya_magma_mpxnode::inEnabled;

#pragma region Attribute Changed Event related
static void updateMagmaNodeAppearanceEvent( maya_magma_mpxnode* caller, MNodeMessage::AttributeMessage msg, MPlug& plug,
                                            MPlug& otherPlug ) {
    if( msg & MNodeMessage::kAttributeSet ) {
        try {
            frantic::magma::maya::mel::maya_magma_mel_window::onAttributeChangedUpdateNodesInWindow( plug );
        } catch( std::exception& e ) {
            MGlobal::displayWarning( e.what() );
        }
    }
}

void maya_magma_mpxnode::addAttributeChangedEventHandler( AttributeChangeFunction func ) {
    if( m_event_handlers.size() == 0 ) {
        MStatus status;
        MObject thismobject = this->thisMObject();
        m_attribute_changed_event_id = MNodeMessage::addAttributeChangedCallback(
            thismobject, maya_magma_mpxnode::attribute_changed_event, this, &status );
    }
    m_event_handlers.push_back( func );
}

void maya_magma_mpxnode::attribute_changed_event( MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& other,
                                                  void* data ) {
    maya_magma_mpxnode* mpxobj = static_cast<maya_magma_mpxnode*>( data );
    if( mpxobj != NULL ) {
        for( std::vector<AttributeChangeFunction>::const_iterator iter = mpxobj->m_event_handlers.begin();
             iter != mpxobj->m_event_handlers.end(); ++iter ) {
            ( *iter )( mpxobj, msg, plug, other );
        }
    }
}
#pragma endregion

maya_magma_mpxnode::maya_magma_mpxnode() {}

maya_magma_mpxnode::~maya_magma_mpxnode() {
    if( m_event_handlers.size() > 0 )
        MMessage::removeCallback( m_attribute_changed_event_id );
}

void* maya_magma_mpxnode::creator() { return new maya_magma_mpxnode; }

MStatus maya_magma_mpxnode::initialize() {
    MStatus status;

    // Particle Stream Input
    {
        MFnTypedAttribute fnTypedAttribute;
        inParticleStream = fnTypedAttribute.create( "inParticleStream", "inParticleStream",
                                                    frantic::maya::MPxParticleStream::id, MObject::kNullObj );
        fnTypedAttribute.setHidden( true );
        fnTypedAttribute.setStorable( false );
        status = addAttribute( inParticleStream );
        CHECK_MSTATUS_AND_RETURN_IT( status );
    }

    // MayaMagma
    {
        MFnTypedAttribute fnTypedAttribute;
        inMayaMagma =
            fnTypedAttribute.create( "inMayaMagma", "MayaMagma", maya_magma_desc_mpxdata::id, MObject::kNullObj );
        fnTypedAttribute.setHidden( true );
        status = addAttribute( inMayaMagma );
        CHECK_MSTATUS_AND_RETURN_IT( status );
    }

    // Particle Stream Output
    {
        MFnTypedAttribute fnTypedAttribute;
        outParticleStream = fnTypedAttribute.create( "outParticleStream", "outParticleStream",
                                                     frantic::maya::MPxParticleStream::id, MObject::kNullObj );
        fnTypedAttribute.setHidden( true );
        fnTypedAttribute.setStorable( false );
        status = addAttribute( outParticleStream );
        CHECK_MSTATUS_AND_RETURN_IT( status );
    }

    // inEnabled
    {
        MFnNumericAttribute fnNumericAttribute;
        inEnabled = fnNumericAttribute.create( "inEnabled", "enabled", MFnNumericData::kBoolean, 1 );
        status = addAttribute( inEnabled );
        CHECK_MSTATUS_AND_RETURN_IT( status );
    }

    attributeAffects( inParticleStream, outParticleStream );
    attributeAffects( inEnabled, outParticleStream );

    return MS::kSuccess;
}

void maya_magma_mpxnode::postConstructor() {
    MStatus stat;

    // With the default node deletion behaviour, magma nodes are deleted whenever the last node with an attribute
    // connected to an input attribute is created, or when the last node with an attribute connected to an output
    // attribute is deleted. This behaviour is desireable for input attributes, since we want magmas to be deleted when
    // the PRTObject they modify is deleted, however, we don't want them to be deleted when another node ( such as a
    // Frost ) connected to an output attribute is deleted. The two following calls ensure that we get the desired
    // behaviour. Note that these calls are only respected if made in postConstructor().
    setExistWithoutInConnections( false );
    setExistWithoutOutConnections( true );

    // Magma
    {
        // prepare the default data
        MFnPluginData fnData;
        MObject pluginMpxData = fnData.create( MTypeId( frantic::magma::maya::maya_magma_desc_mpxdata::id ), &stat );
        if( stat != MStatus::kSuccess )
            throw std::runtime_error( ( "DEBUG: maya_magma_mpxnode: fnData.create()" + stat.errorString() ).asChar() );
        frantic::magma::maya::maya_magma_desc_mpxdata* mpxData =
            frantic::maya::mpx_cast<frantic::magma::maya::maya_magma_desc_mpxdata*>( fnData.data( &stat ) );
        if( !mpxData || stat != MStatus::kSuccess )
            throw std::runtime_error(
                ( "DEBUG: maya_magma_mpxnode: dynamic_cast<frantic::magma::maya::maya_magma_desc_mpxdata*>(...) " +
                  stat.errorString() )
                    .asChar() );
        mpxData->set_maya_magma_desc( boost::make_shared<frantic::magma::maya::desc::maya_magma_desc>() );

        // get plug
        MObject obj = thisMObject();
        MFnDependencyNode depNode( obj, &stat );
        if( stat != MStatus::kSuccess )
            throw std::runtime_error(
                ( "DEBUG: maya_magma_mpxnode: could not get dependencyNode from thisMObject():" + stat.errorString() )
                    .asChar() );

        MPlug plug = depNode.findPlug( "inMayaMagma", &stat );
        if( stat != MStatus::kSuccess )
            throw std::runtime_error(
                ( "DEBUG: maya_magma_mpxnode: could not find plug 'inMayaMagma' from depNode: " + stat.errorString() )
                    .asChar() );

        // set the default data on the plug
        FF_LOG( debug ) << "maya_magma_mpxnode::postConstructor(): setValue for inMayaMagma" << std::endl;
        plug.setValue( pluginMpxData );
    }

    // Output Particles
    {
        // prepare the default data
        MFnPluginData fnData;
        MObject pluginMpxData = fnData.create( MTypeId( frantic::maya::MPxParticleStream::id ), &stat );
        if( stat != MStatus::kSuccess )
            throw std::runtime_error( ( "DEBUG: maya_magma_mpxnode: fnData.create()" + stat.errorString() ).asChar() );
        frantic::maya::MPxParticleStream* mpxData =
            frantic::maya::mpx_cast<frantic::maya::MPxParticleStream*>( fnData.data( &stat ) );
        if( !mpxData || stat != MStatus::kSuccess )
            throw std::runtime_error(
                ( "DEBUG: maya_magma_mpxnode: dynamic_cast<frantic::maya::MPxParticleStream*>(...) " +
                  stat.errorString() )
                    .asChar() );
        mpxData->setParticleSource( this );

        // get plug
        MObject obj = thisMObject();
        MFnDependencyNode depNode( obj, &stat );
        if( stat != MStatus::kSuccess )
            throw std::runtime_error(
                ( "DEBUG: maya_magma_mpxnode: could not get dependencyNode from thisMObject():" + stat.errorString() )
                    .asChar() );

        MPlug plug = depNode.findPlug( "outParticleStream", &stat );
        if( stat != MStatus::kSuccess )
            throw std::runtime_error(
                ( "DEBUG: maya_magma_mpxnode: could not find plug 'outParticleStream' from depNode: " +
                  stat.errorString() )
                    .asChar() );

        // set the default data on the plug
        FF_LOG( debug ) << "maya_magma_mpxnode::postConstructor(): setValue for outParticleStream" << std::endl;
        plug.setValue( pluginMpxData );
    }

    // Attribute Changed Event Handler
    addAttributeChangedEventHandler( updateMagmaNodeAppearanceEvent );
}

MStatus maya_magma_mpxnode::compute( const MPlug& plug, MDataBlock& block ) {
    MStatus status = MPxNode::compute( plug, block );
    try {
        if( plug == outParticleStream ) {
            MDataHandle outputData = block.outputValue( outParticleStream );

            MFnPluginData fnData;
            MObject pluginMpxData = fnData.create( MTypeId( frantic::maya::MPxParticleStream::id ), &status );
            if( status != MStatus::kSuccess )
                return status;
            frantic::maya::MPxParticleStream* mpxData =
                frantic::maya::mpx_cast<MPxParticleStream*>( fnData.data( &status ) );
            if( mpxData == NULL )
                return MS::kFailure;
            mpxData->setParticleSource( this );

            outputData.set( mpxData );
            status = MS::kSuccess;
        }
    } catch( std::exception& e ) {
        MGlobal::displayError( e.what() );
        return MS::kFailure;
    }

    return status;
}

frantic::maya::PRTObjectBase::particle_istream_ptr
maya_magma_mpxnode::getParticleStream( const frantic::graphics::transform4f& objectTransform, const MDGContext& context,
                                       bool isViewport ) const {
    // Clear error nodes
    frantic::magma::maya::mel::maya_magma_mel_window::clearErrorNodes( thisMObject() );

    MStatus stat;

    // Get the input particle stream
    MObject obj = thisMObject();
    MFnDependencyNode depNode( obj, &stat );
    if( stat != MStatus::kSuccess ) {
        FF_LOG( debug ) << ( ( "DEBUG: maya_magma_mpxnode: could not get dependencyNode from thisMObject():" +
                               stat.errorString() )
                                 .asChar() )
                        << std::endl;
        return getEmptyStream();
    }

    MPlug plug = depNode.findPlug( "inParticleStream", &stat );
    if( stat != MStatus::kSuccess ) {
        FF_LOG( debug ) << ( ( "DEBUG: maya_magma_mpxnode: could not find plug 'inParticleStream' from depNode: " +
                               stat.errorString() )
                                 .asChar() )
                        << std::endl;
        return getEmptyStream();
    }

    MObject prtMpxData;
    plug.getValue( prtMpxData );
    MFnPluginData fnData( prtMpxData );
    frantic::maya::MPxParticleStream* streamMPxData =
        frantic::maya::mpx_cast<MPxParticleStream*>( fnData.data( &stat ) );
    if( streamMPxData == NULL ) {
        FF_LOG( debug ) << ( ( "DEBUG: maya_magma_mpxnode: could not find plug 'inParticleStream' from depNode: " +
                               stat.errorString() )
                                 .asChar() )
                        << std::endl;
        return getEmptyStream();
    }

    particle_istream_ptr outStream;
    if( isViewport )
        outStream = streamMPxData->getViewportParticleStream( objectTransform, context );
    else
        outStream = streamMPxData->getRenderParticleStream( objectTransform, context );

    // Get the magma transform
    if( isEnabled( context ) ) {
        try {

            MDagPath cameraDag;
            if( MRenderView::doesRenderEditorExist() ) {
                M3dView::active3dView().getCamera( cameraDag );
            } else {
                // Maya Batch mode render fails to get the appropriate camera object.
                // Pick one of the active cameras and just use that.
                std::vector<MDagPath> renderableCameras;
                maya_util::find_all_renderable_cameras( renderableCameras );
                if( !renderableCameras.empty() )
                    cameraDag = renderableCameras.front();
            }

            frantic::graphics::transform4f cameraPosition;
            maya_util::get_object_world_matrix( cameraDag, context, cameraPosition );
            frantic::graphics::transform4f cameraTransform = cameraPosition.to_inverse();

            frantic::tstring magmaAttr( _T( "inMayaMagma" ) );
            frantic::magma::maya::modifier::maya_magma_modifier modifier( obj, magmaAttr, objectTransform,
                                                                          cameraTransform );
            outStream = modifier.get_modified_particle_istream( outStream );

        } catch( frantic::magma::magma_exception& e ) {
            frantic::magma::magma_interface::magma_id id = e.get_node_id();
            frantic::magma::maya::mel::maya_magma_mel_window::addErrorNode( obj, id );
            throw std::runtime_error( frantic::strings::to_string( e.get_message().c_str() ) );
        }
    }

    return outStream;
}

bool maya_magma_mpxnode::isEnabled( const MDGContext& context ) const {
    MStatus outStatus;
    MPlug plug( thisMObject(), inEnabled );
    return plug.asBool( const_cast<MDGContext&>( context ), &outStatus );
}

} // namespace maya
} // namespace magma
} // namespace frantic
