// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <sstream>

#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MStringArray.h>

#include <frantic/logging/logging_level.hpp>

namespace frantic {
namespace maya {
namespace selection {

inline MStringArray get_current_selected_object_names() {
    std::ostringstream cmd;
    MStringArray resultArray;

    // please tell me if there is a better way to get current selected object name
    cmd << "ls -selection";
    MGlobal::executeCommand( cmd.str().c_str(), resultArray );
    return resultArray;
}

// Temporary working around since MFnDependencyNode's copy constructor changes visibility between maya versions
#define GET_DEPENDENCY_NODE_FROM_MSTRING( MFnDependencyNode_OUT, MString_IN_NAME )                                     \
    MSelectionList __TMPVAR_selected;                                                                                  \
    __TMPVAR_selected.add( MString_IN_NAME );                                                                          \
                                                                                                                       \
    if( __TMPVAR_selected.length() == 0 ) {                                                                            \
        FF_LOG( error ) << "get_dependency_node_from_mstring: zero selected object." << std::endl;                     \
        throw std::runtime_error( "get_dependency_node_from_mstring: zero selected object" );                          \
    } else if( __TMPVAR_selected.length() > 1 ) {                                                                      \
        FF_LOG( error ) << "get_dependency_node_from_mstring: multiple selected object." << std::endl;                 \
        throw std::runtime_error( "get_dependency_node_from_mstring: multiple selected object." );                     \
    }                                                                                                                  \
                                                                                                                       \
    MObject __TMPVAR_obj;                                                                                              \
    __TMPVAR_selected.getDependNode( 0, __TMPVAR_obj );                                                                \
    MFnDependencyNode MFnDependencyNode_OUT( __TMPVAR_obj );

#define GET_PLUG_FROM_MSTRING( MPlug_OUT, MString_IN_NAME )                                                            \
    MSelectionList __TMPVAR_selected;                                                                                  \
    __TMPVAR_selected.add( MString_IN_NAME );                                                                          \
                                                                                                                       \
    if( __TMPVAR_selected.length() == 0 ) {                                                                            \
        FF_LOG( error ) << "get_plug_from_mstring: zero selected object." << std::endl;                                \
        throw std::runtime_error( "get_plug_from_mstring: zero selected object" );                                     \
    } else if( __TMPVAR_selected.length() > 1 ) {                                                                      \
        FF_LOG( error ) << "get_plug_from_mstring: multiple selected object." << std::endl;                            \
        throw std::runtime_error( "get_plug_from_mstring: multiple selected object." );                                \
    }                                                                                                                  \
                                                                                                                       \
    MPlug MPlug_OUT;                                                                                                   \
    __TMPVAR_selected.getPlug( 0, MPlug_OUT );
/*
inline MFnDependencyNode get_dependency_node_from_mstring( const MString &name )
{
        MSelectionList selected;
        selected.add(name);

        if (selected.length() == 0) {
                FF_LOG(error) << "get_dependency_node_from_mstring: zero selected object." << std::endl;
                throw std::runtime_error("get_dependency_node_from_mstring: zero selected object");
        } else if (selected.length() > 1) {
                FF_LOG(error) << "get_dependency_node_from_mstring: multiple selected object." << std::endl;
                throw std::runtime_error("get_dependency_node_from_mstring: multiple selected object.");
        }

        MObject obj;
        selected.getDependNode(0, obj);
        MFnDependencyNode outDepNode( obj );
        return outDepNode;
}
*/

} // namespace selection
} // namespace maya
} // namespace frantic
