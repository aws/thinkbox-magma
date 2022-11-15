// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <maya/MFnDependencyNode.h>

#include "frantic/magma/maya/maya_magma_common.hpp"
#include "frantic/magma/maya/maya_magma_datatypes.hpp"
#include "frantic/magma/maya/maya_magma_info.hpp"

namespace frantic {
namespace magma {
namespace maya {
namespace attr {

class maya_magma_attr_manager {
  public:
    ~maya_magma_attr_manager() {}

    // create corresponding maya attributes when a desc node get created
    static void create_maya_attr( const info::maya_magma_node_info& nodeInfo, desc::maya_magma_desc_ptr desc,
                                  desc::desc_id descID, MFnDependencyNode& depNode );

    // delete corresponding maya attributes when a desc node get deleted
    static void delete_maya_attr( desc::maya_magma_desc_ptr desc, desc::desc_id descID, MFnDependencyNode& depNode );

    // get default value for input socket
    static holder::input_socket_variant_t
    get_input_socket_value_from_maya_attrs( const frantic::tstring& enumAttrName,
                                            const std::vector<frantic::tstring>& inputSocketAttrNames,
                                            const MFnDependencyNode& depNode );

    // get default value for input socket at the given type index
    static holder::input_socket_variant_t
    get_input_socket_value_from_maya_attrs( const maya_attribute_input_socket_data_type index,
                                            const std::vector<frantic::tstring>& inputSocketAttrNames,
                                            const MFnDependencyNode& depNode );

    // get the current enum selection for input socket
    static maya_attribute_input_socket_data_type
    get_input_socket_value_enum_from_maya_attrs( const frantic::tstring& enumAttrName,
                                                 const MFnDependencyNode& depNode );

    // set the value for input socket
    static void set_input_socket_value_from_maya_attrs( const maya_attribute_input_socket_data_type index,
                                                        const std::vector<frantic::tstring>& inputSocketAttrNames,
                                                        const MFnDependencyNode& depNode,
                                                        holder::input_socket_variant_t& value );

    // set the current enum selection for input socket
    static void set_input_socket_value_enum_from_maya_attrs( const frantic::tstring& enumAttrName,
                                                             const MFnDependencyNode& depNode,
                                                             const maya_attribute_input_socket_data_type value );

    // get value for property
    static holder::property_variant_t get_property_value_from_maya_attrs( const frantic::tstring& mayaAttrName,
                                                                          const MFnDependencyNode& depNode );

    // set the value for property
    static void set_property_value_from_maya_attrs( const frantic::tstring& mayaAttrName,
                                                    const MFnDependencyNode& depNode,
                                                    holder::property_variant_t& value );

    // set the animation for input socket
    static void set_input_socket_animation_from_maya_attrs( const maya_attribute_input_socket_data_type index,
                                                            const std::vector<frantic::tstring>& inputSocketAttrNames,
                                                            const MFnDependencyNode& depNode,
                                                            holder::input_socket_animation_variant_t& value );

    // get the animation for input socket at the given type index
    static holder::input_socket_animation_variant_t
    get_input_socket_animation_from_maya_attrs( const maya_attribute_input_socket_data_type index,
                                                const std::vector<frantic::tstring>& inputSocketAttrNames,
                                                const MFnDependencyNode& depNode );

    // set the animation for property
    static void set_property_animation_from_maya_attrs( const frantic::tstring& mayaAttrName,
                                                        const MFnDependencyNode& depNode,
                                                        holder::property_animation_variant_t& value );

    // get the animation for property
    static holder::property_animation_variant_t
    get_property_animation_from_maya_attrs( const frantic::tstring& mayaAttrName, const MFnDependencyNode& depNode );

    // change attributes to the latest version (support backwards compatibility)
    static void update_to_latest_version( desc::maya_magma_desc_ptr desc, MFnDependencyNode& depNode );

  private:
    maya_magma_attr_manager() {}
};

} // namespace attr
} // namespace maya
} // namespace magma
} // namespace frantic
