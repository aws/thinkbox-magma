// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <maya/MStatus.h>

#include <frantic/maya/plugin_manager.hpp>

#include "frantic/magma/maya/maya_magma_desc_mpxdata.hpp"
#include "frantic/magma/maya/maya_magma_initializer.hpp"
#include "frantic/magma/maya/maya_magma_mel_command.hpp"
#include "frantic/magma/maya/maya_magma_mpxnode.hpp"
#include <frantic/maya/MPxParticleStream.hpp>
#include <frantic/maya/PRTMayaParticle.hpp>

namespace frantic {
namespace magma {
namespace maya {

using namespace frantic::maya;

void initialize( frantic::maya::plugin_manager& pluginManager ) {
    MStatus status;

    // REMOVE ME
    frantic::logging::set_logging_level( frantic::logging::LOG_DEBUG );

    // Common to all Maya
    status =
        pluginManager.register_data( MPxParticleStream::typeName, MPxParticleStream::id, MPxParticleStream::creator );
    CHECK_MSTATUS( status );
    status = pluginManager.register_node( PRTMayaParticle::typeName, PRTMayaParticle::typeId, PRTMayaParticle::creator,
                                          PRTMayaParticle::initialize, MPxNode::kDependNode );
    CHECK_MSTATUS( status );

    // Magma plugins
    status = pluginManager.register_data( maya_magma_desc_mpxdata::typeName, maya_magma_desc_mpxdata::id,
                                          maya_magma_desc_mpxdata::creator );
    CHECK_MSTATUS( status );
    status =
        pluginManager.register_node( maya_magma_mpxnode::typeName, maya_magma_mpxnode::id, maya_magma_mpxnode::creator,
                                     maya_magma_mpxnode::initialize, MPxNode::kDependNode );
    CHECK_MSTATUS( status );
    status = pluginManager.register_command( mel::maya_magma_mel_create_node::commandName,
                                             mel::maya_magma_mel_create_node::creator );
    CHECK_MSTATUS( status );
    status = pluginManager.register_command( mel::maya_magma_mel_delete_node::commandName,
                                             mel::maya_magma_mel_delete_node::creator );
    CHECK_MSTATUS( status );
    status = pluginManager.register_command( mel::maya_magma_mel_create_connection::commandName,
                                             mel::maya_magma_mel_create_connection::creator );
    CHECK_MSTATUS( status );
    status = pluginManager.register_command( mel::maya_magma_mel_delete_connection::commandName,
                                             mel::maya_magma_mel_delete_connection::creator );
    CHECK_MSTATUS( status );
    status = pluginManager.register_command( mel::maya_magma_mel_retrieve_node_info::commandName,
                                             mel::maya_magma_mel_retrieve_node_info::creator );
    CHECK_MSTATUS( status );
    status = pluginManager.register_command( mel::maya_magma_mel_debug_desc::commandName,
                                             mel::maya_magma_mel_debug_desc::creator );
    CHECK_MSTATUS( status );
    status =
        pluginManager.register_command( mel::maya_magma_mel_window::commandName, mel::maya_magma_mel_window::creator );
    CHECK_MSTATUS( status );
    status = pluginManager.register_command( mel::maya_magma_mel_window_retrieve::commandName,
                                             mel::maya_magma_mel_window_retrieve::creator );
    CHECK_MSTATUS( status );
    status = pluginManager.register_command( mel::maya_magma_mel_window_set::commandName,
                                             mel::maya_magma_mel_window_set::creator );
    CHECK_MSTATUS( status );
    status = pluginManager.register_command( mel::maya_magma_mel_retrieve_node_ids::commandName,
                                             mel::maya_magma_mel_retrieve_node_ids::creator );
    CHECK_MSTATUS( status );
    status = pluginManager.register_command( mel::maya_magma_mel_retrieve_node_instance_info::commandName,
                                             mel::maya_magma_mel_retrieve_node_instance_info::creator );
    CHECK_MSTATUS( status );
    status = pluginManager.register_command( mel::maya_magma_mel_set_node_instance_info::commandName,
                                             mel::maya_magma_mel_set_node_instance_info::creator );
    CHECK_MSTATUS( status );
    status = pluginManager.register_command( mel::maya_magma_mel_get_connections::commandName,
                                             mel::maya_magma_mel_get_connections::creator );
    CHECK_MSTATUS( status );
    status = pluginManager.register_command( mel::maya_magma_mel_load_setup::commandName,
                                             mel::maya_magma_mel_load_setup::creator );
    CHECK_MSTATUS( status );
    status = pluginManager.register_command( mel::maya_magma_mel_apply_setup::commandName,
                                             mel::maya_magma_mel_apply_setup::creator );
    CHECK_MSTATUS( status );
    status = pluginManager.register_command( mel::maya_magma_mel_set_connection_properties::commandName,
                                             mel::maya_magma_mel_set_connection_properties::creator );
    CHECK_MSTATUS( status );
    status = pluginManager.register_command( mel::maya_magma_mel_window_menu::commandName,
                                             mel::maya_magma_mel_window_menu::creator );
    CHECK_MSTATUS( status );
}

} // namespace maya
} // namespace magma
} // namespace frantic
