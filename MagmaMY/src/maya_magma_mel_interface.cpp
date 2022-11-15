// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "frantic/maya/parser_args.hpp"
#include <maya/MFnDependencyNode.h>
#include <maya/MFnPluginData.h>
#include <maya/MStringArray.h>

#include "frantic/maya/selection.hpp"
#include <frantic/logging/logging_level.hpp>

#include "frantic/magma/maya/maya_magma_desc_mpxdata.hpp"
#include "frantic/magma/maya/maya_magma_mel_interface.hpp"
#include <frantic/magma/maya/maya_magma_exception.hpp>

namespace frantic {
namespace magma {
namespace maya {
namespace mel {

maya_magma_mel_interface::~maya_magma_mel_interface() {}

MStatus maya_magma_mel_interface::doIt( const MArgList& args ) {
    MStatus outStatus = MS::kFailure;
    try {

        MString objectName;
        if( MS::kSuccess != frantic::maya::parser_args::get_arg( args, "-prtnode", "-prt", objectName ) ) {

            // get current selected object name (only one object should be selected)
            MStringArray objectNames = frantic::maya::selection::get_current_selected_object_names();
            if( objectNames.length() != 1 )
                throw maya_magma_exception(
                    "maya_magma_mel_interface::doIt no maya object or more than one maya object has been selected" );

            // get the pointer to PRTLoader * from maya dependency node
            objectName = objectNames[0];
        }

        // MFnDependencyNode depNode = frantic::maya::selection::get_dependency_node_from_mstring(objectName);
        GET_DEPENDENCY_NODE_FROM_MSTRING( depNode, objectName );

        MStatus stat;
        MPlug magmaPlug = depNode.findPlug( "inMayaMagma", &stat );
        throw_maya_magma_exception_if_not_success(
            stat, _T( "maya_magma_mel_interface::doIt could not find a plug in 'inMayaMagma'" ) );

        MObject descMpxData;
        stat = magmaPlug.getValue( descMpxData );
        throw_maya_magma_exception_if_not_success( stat,
                                                   _T( "maya_magma_mel_interface::doIt could get value from plug" ) );

        MFnPluginData fnData( descMpxData, &stat );
        throw_maya_magma_exception_if_not_success(
            stat, _T( "maya_magma_mel_interface::doIt could get MFnPluginData from MObject: descMpxData" ) );

        // pdescMPxData just point to the data in maya_magma_desc_mpxdata
        maya_magma_desc_mpxdata* pdescMPxData = static_cast<maya_magma_desc_mpxdata*>( fnData.data( &stat ) );
        throw_maya_magma_exception_if_not_success(
            stat, _T( "maya_magma_mel_interface::doIt could not get data from MFnPluginData(fnData)" ) );

        if( !pdescMPxData )
            throw maya_magma_exception(
                "maya_magma_mel_interface::doIt pdescMPxData is Null, possibly not initialized yet? " );

        desc::maya_magma_desc_ptr desc = pdescMPxData->get_maya_magma_desc();

        // let derived class to do the parseArgs
        outStatus = parseArgs( args, desc, depNode );

    } catch( std::exception& e ) {
        FF_LOG( error ) << e.what() << std::endl;
        MGlobal::displayError( e.what() );
    } catch( ... ) {
        FF_LOG( stats ) << "YOU should never get here" << std::endl;
        assert( false );
    }
    return outStatus;
}

} // namespace mel
} // namespace maya
} // namespace magma
} // namespace frantic
