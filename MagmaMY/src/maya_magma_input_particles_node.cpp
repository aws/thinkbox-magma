// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "frantic/magma/maya/nodes/maya_magma_input_particles_node.hpp"

#include "frantic/maya/selection.hpp"
#include <frantic/magma/nodes/magma_node_impl.hpp>
#include <frantic/maya/particles/particles.hpp>
#include <maya/MFnParticleSystem.h>

// Required for getting PRTObject particles
#include <frantic/graphics/transform4f.hpp>
#include <frantic/maya/PRTMayaParticle.hpp>
#include <frantic/maya/PRTObject_base.hpp>
#include <frantic/maya/maya_util.hpp>
#include <frantic/maya/util.hpp>
#include <frantic/particles/streams/particle_istream.hpp>
#include <frantic/particles/streams/transformed_particle_istream.hpp>
#include <maya/MDagPath.h>

#include "frantic/magma/maya/maya_magma_attr_manager.hpp"
#include "frantic/magma/maya/maya_magma_datatypes.hpp"
#include "frantic/magma/maya/maya_magma_desc_mpxdata.hpp"
#include "frantic/magma/maya/maya_magma_description.hpp"
#include "frantic/magma/maya/maya_magma_mpxnode.hpp"
#include <maya/MFnPluginData.h>
#include <set>

using namespace frantic::maya;

namespace frantic {
namespace magma {
namespace maya {
namespace nodes {

namespace detail {
bool check_for_loops( MFnDependencyNode& endOfChain, std::map<frantic::tstring, size_t>& elementsCurrentInPathMap,
                      std::vector<frantic::tstring>& elementsCurrentInPath,
                      std::set<frantic::tstring>& checkedElements ) {

    MStatus status;
    MObject startingObject = endOfChain.object();
    MObject currentObject = startingObject;

    std::vector<frantic::magma::maya::desc::maya_magma_desc_ptr> mayaCheck;
    std::vector<MObject> mayaObjects;
    do {
        // Check if there's an inMayaMagma object and add it to the list to check
        MFnDependencyNode depNode( currentObject );
        MPlug plug = depNode.findPlug( "inMayaMagma", &status );

        MObject toCheck = currentObject;
        currentObject = frantic::maya::PRTObjectBase::previousElementInChain( depNode );

        MObject mayaMagmaDataObj;
        status = plug.getValue( mayaMagmaDataObj );
        if( status != MS::kSuccess )
            continue;

        MFnPluginData pluginData( mayaMagmaDataObj, &status );
        if( status != MS::kSuccess )
            continue;

        MPxData* data = pluginData.data( &status );
        if( status != MS::kSuccess )
            continue;

        const frantic::magma::maya::maya_magma_desc_mpxdata* pMayaMagmaMPxData =
            frantic::maya::mpx_cast<const frantic::magma::maya::maya_magma_desc_mpxdata*>( data );
        if( pMayaMagmaMPxData == NULL )
            continue;

        // Check if this modifier is enabled.  If it's disabled, it's safe to skip over.
        MPlug enabledPlug = depNode.findPlug( "inEnabled", &status );
        if( status == MS::kSuccess ) {
            bool enabled = enabledPlug.asBool( MDGContext::fsNormal, &status );
            if( status == MS::kSuccess && !enabled )
                continue;
        }

        frantic::magma::maya::desc::maya_magma_desc_ptr desc = pMayaMagmaMPxData->get_maya_magma_desc();
        mayaCheck.push_back( desc );
        mayaObjects.push_back( toCheck );
    } while( currentObject != MObject::kNullObj );

    if( mayaCheck.empty() )
        return true;

    // Check if they refer to another particle
    std::vector<frantic::magma::maya::desc::maya_magma_desc_ptr>::const_iterator mayaIter;
    std::vector<MObject>::const_iterator nodeIter;
    for( mayaIter = mayaCheck.begin(), nodeIter = mayaObjects.begin();
         mayaIter != mayaCheck.end() && nodeIter != mayaObjects.end(); ++mayaIter, ++nodeIter ) {

        frantic::magma::maya::desc::maya_magma_desc_ptr desc = *mayaIter;
        const std::map<frantic::magma::maya::desc::desc_id, frantic::magma::maya::desc::maya_magma_desc_node> nodes =
            desc->get_nodes();
        for( std::map<frantic::magma::maya::desc::desc_id,
                      frantic::magma::maya::desc::maya_magma_desc_node>::const_iterator idIter = nodes.begin();
             idIter != nodes.end(); ++idIter ) {

            frantic::magma::maya::desc::desc_id id = idIter->first;
            if( desc->get_node_type( id ) == _T("InputParticles") ) {
                bool ok;
                MStatus status;

                // get value for property
                frantic::tstring mayaAttr = desc->get_maya_attr_name_from_node_property( id, _T( "particleName" ) );
                MFnDependencyNode currentNode( *nodeIter, &status );
                frantic::magma::maya::holder::property_variant_t value =
                    frantic::magma::maya::attr::maya_magma_attr_manager::get_property_value_from_maya_attrs(
                        mayaAttr, currentNode );
                frantic::tstring particleName = boost::get<frantic::tstring>( value );

                // Figure out where it's coming from
                // Errors can be caught right here but I'm just interested in circular dependencies
                MString objectName( frantic::strings::to_string( particleName ).c_str() );

                MSelectionList currentPar;
                currentPar.add( objectName );
                if( currentPar.length() != 1 )
                    continue;
                MObject foundParObject;
                currentPar.getDependNode( 0, foundParObject );

                // If input is a maya particle system, check if there's a wrapper we can use
                bool useMayaParticle = false;
                MFnParticleSystem mayaParticleSystem( foundParObject, &status );
                MObject sourceObject = foundParObject;
                if( status == MS::kSuccess ) {
                    MObject prtObj =
                        frantic::maya::PRTMayaParticle::getPRTMayaParticleFromMayaParticleStreamCheckDeformed(
                            mayaParticleSystem, &status );
                    if( status == MS::kSuccess && sourceObject != MObject::kNullObj ) {
                        sourceObject = prtObj;
                        useMayaParticle = false;
                    } else {
                        useMayaParticle = true;
                    }
                }

                if( useMayaParticle ) {
                    // Dealing with Maya nParticles
                    continue;
                }

                // Dealing with PRTObjects
                MFnDependencyNode testDepNode( sourceObject );
                ok = PRTObjectBase::hasParticleStreamMPxData( testDepNode );
                if( !ok )
                    continue;

                MObject endOfChain = PRTObjectBase::getEndOfStreamChain( testDepNode );
                MFnDependencyNode nextDepNode( endOfChain );

                // Check if we've reached a loop
                if( elementsCurrentInPathMap.count( particleName ) > 0 ) {
                    elementsCurrentInPath.push_back( particleName );
                    return false;
                }

                // Did we already check this branch?
                if( checkedElements.count( particleName ) > 0 )
                    continue;

                // Check this branch now
                elementsCurrentInPath.push_back( particleName );
                checkedElements.insert( particleName );
                elementsCurrentInPathMap[particleName] = elementsCurrentInPath.size() - 1;
                ok = check_for_loops( nextDepNode, elementsCurrentInPathMap, elementsCurrentInPath, checkedElements );
                if( !ok )
                    return false;
                elementsCurrentInPath.pop_back();
                elementsCurrentInPathMap.erase( particleName );
            }
        }
    }

    // This branch ok
    return true;
}

bool check_for_loops( MFnDependencyNode& endOfChain, std::vector<frantic::tstring>& elementsCurrentInPath ) {
    std::set<frantic::tstring> seenIt;
    std::map<frantic::tstring, size_t> elementsMap;
    int i = 0;
    for( std::vector<frantic::tstring>::const_iterator iter = elementsCurrentInPath.begin();
         iter != elementsCurrentInPath.end(); ++iter ) {
        seenIt.insert( *iter );
        elementsMap[*iter] = i;
        i++;
    }
    return check_for_loops( endOfChain, elementsMap, elementsCurrentInPath, seenIt );
}

} // namespace detail

MAGMA_DEFINE_TYPE( "InputParticles", "Input", maya_magma_input_particles_node )
MAGMA_EXPOSE_PROPERTY( particleName, frantic::tstring )
MAGMA_OUTPUT_NAMES( "Particles", "Particle Count" )
MAGMA_DESCRIPTION( "Exposes the particles of a node for other operators." )
MAGMA_DEFINE_TYPE_END

frantic::magma::nodes::magma_input_particles_interface::const_particle_array_ptr
maya_magma_input_particles_node::get_particles() const {
    return m_cachedParticles;
}

frantic::magma::nodes::magma_input_particles_interface::const_particle_kdtree_ptr
maya_magma_input_particles_node::get_particle_kdtree() {
    // Copied from MagmaProject/3DSMaxProject/src/nodes/magma_input_particles_node.cpp
    if( !m_cachedKDTree ) {
        m_cachedKDTree.reset(
            new frantic::particles::particle_kdtree<frantic::magma::nodes::detail::particle_standin> );

        frantic::channels::channel_accessor<frantic::graphics::vector3f> posAccessor =
            m_cachedParticles->get_channel_map().get_accessor<frantic::graphics::vector3f>( _T("Position") );

        boost::int64_t counter = 0;
        for( frantic::particles::particle_array::const_iterator it = m_cachedParticles->begin(),
                                                                itEnd = m_cachedParticles->end();
             it != itEnd; ++it, ++counter )
            m_cachedKDTree->add_particle(
                frantic::magma::nodes::detail::particle_standin( posAccessor.get( *it ), counter ) );
        m_cachedKDTree->balance_kdtree();
    }

    return m_cachedKDTree;
}

void maya_magma_input_particles_node::compile_as_extension_type( frantic::magma::magma_compiler_interface& compiler ) {
    bool ok;
    MStatus status;
    MString objectName( frantic::strings::to_string( m_particleName ).c_str() );

    // GET_DEPENDENCY_NODE_FROM_MSTRING( depNode, objectName );
    MSelectionList currentPar;
    currentPar.add( objectName );
    if( currentPar.length() != 1 )
        throw magma_exception()
            << magma_exception::node_id( get_id() ) << magma_exception::property_name( _T( "particleName" ) )
            << magma_exception::error_name(
                   _T("maya_magma_input_particles_node::compile_as_extension_type error: Object \"") + m_particleName +
                   _T("\" not found.") );
    MObject foundParObject;
    currentPar.getDependNode( 0, foundParObject );

    // If input is a maya particle system, check if there's a wrapper we can use
    bool useMayaParticle = false;
    MFnParticleSystem mayaParticleSystem( foundParObject, &status );
    MObject sourceObject = foundParObject;
    if( status == MS::kSuccess ) {
        MObject prtObj = frantic::maya::PRTMayaParticle::getPRTMayaParticleFromMayaParticleStreamCheckDeformed(
            mayaParticleSystem, &status );
        if( status == MS::kSuccess && sourceObject != MObject::kNullObj ) {
            sourceObject = prtObj;
            useMayaParticle = false;
        } else {
            useMayaParticle = true;
        }
    }

    if( useMayaParticle ) {
        // Dealing with Maya nParticles
        throw magma_exception()
            << magma_exception::node_id( get_id() ) << magma_exception::property_name( _T( "particleName" ) )
            << magma_exception::error_name(
                   _T("maya_magma_input_particles_node::compile_as_extension_type error: Maya Particles \"") +
                   m_particleName + _T("\" no longer supported.  Please wrap with PRTMayaParticle.") );
    }

    // Dealing with PRTObjects
    MFnDependencyNode testDepNode( sourceObject );
    ok = PRTObjectBase::hasParticleStreamMPxData( testDepNode );
    if( !ok )
        throw magma_exception()
            << magma_exception::node_id( get_id() ) << magma_exception::property_name( _T( "particleName" ) )
            << magma_exception::error_name(
                   _T("maya_magma_input_particles_node::compile_as_extension_type error: Object \"") + m_particleName +
                   _T("\" does not contain a particle system.") );

    MObject endOfChain = PRTObjectBase::getEndOfStreamChain( testDepNode );
    MFnDependencyNode depNode( endOfChain );

    // Check for circular dependencies
    std::vector<frantic::tstring> loopItems;
    loopItems.push_back( m_particleName );
    ok = detail::check_for_loops( depNode, loopItems );
    if( !ok ) {
        std::stringstream str;
        bool first = true;
        for( std::vector<frantic::tstring>::const_iterator iter = loopItems.begin(); iter != loopItems.end(); ++iter ) {
            str << ( first ? _T( "" ) : _T( "->" ) ) << frantic::strings::to_string( *iter );
            first = false;
        }
        throw magma_exception() << magma_exception::node_id( get_id() )
                                << magma_exception::property_name( _T( "particleName" ) )
                                << magma_exception::error_name(
                                       _T("maya_magma_input_particles_node::compile_as_extension_type error: magma ")
                                       _T("circular dependency detected.  Path Found: ") +
                                       frantic::strings::to_tstring( str.str() ) );
    }

    MDagPath exportObjectPath;
    currentPar.getDagPath( 0, exportObjectPath );

    const MTime currentTime = frantic::maya::maya_util::get_current_time();
    MDGContext currentContext( currentTime );

    frantic::graphics::transform4f transform;
    ok = frantic::maya::maya_util::get_object_world_matrix( exportObjectPath, currentContext, transform );

    boost::shared_ptr<frantic::particles::streams::particle_istream> inputStream;
    try {
        inputStream = PRTObjectBase::getParticleStreamFromMPxData( depNode, transform, currentContext, false );
    } catch( frantic::magma::magma_exception& e ) {
        // Change the origin so that there's no confusion over which object is causing it
        throw magma_exception()
            << magma_exception::node_id( get_id() ) << magma_exception::property_name( _T( "particleName" ) )
            << magma_exception::error_name(
                   _T("maya_magma_input_particles_node::compile_as_extension_type error: PRTObject \"") +
                   m_particleName + _T("\" contains an error in its magma setup.  Internal Message: [") +
                   e.get_message() + _T("]") );
    }

    // transform the the particle stream into world-space co-ordinates
    frantic::graphics::transform4f objectPosition;
    maya_util::get_object_world_matrix( exportObjectPath, currentContext, objectPosition );
    inputStream = boost::shared_ptr<frantic::particles::streams::particle_istream>(
        new frantic::particles::streams::transformed_particle_istream<float>( inputStream, objectPosition ) );

    frantic::channels::channel_map channels = inputStream->get_native_channel_map();
    m_cachedParticles.reset( new frantic::particles::particle_array );
    m_cachedParticles->set_channel_map( channels );
    m_cachedParticles->insert_particles( inputStream );

    compiler.register_interface( get_id(),
                                 static_cast<frantic::magma::nodes::magma_input_particles_interface*>( this ) );
    compiler.compile_constant( get_id(), frantic::magma::variant_t( (int)m_cachedParticles->size() ) );
}

} // namespace nodes
} // namespace maya
} // namespace magma
} // namespace frantic
