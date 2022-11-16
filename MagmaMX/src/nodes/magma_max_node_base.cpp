// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/max3d/MagmaMaxNodeExtension.hpp>

#include <frantic/magma/magma_exception.hpp>

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

magma_max_node_base::magma_max_node_base() { m_maxObjHandle = NULL; }

magma_max_node_base::~magma_max_node_base() {}

IMagmaNode* magma_max_node_base::get_max_object() const {
    if( m_maxObjHandle != NULL ) { // Make sure the handle isn't NULL, since it throws a Max assertion in
                                   // Animatable::GetAnimByHandle() but its ok to call get_max_object() in this case.
        if( Animatable* anim = Animatable::GetAnimByHandle( m_maxObjHandle ) ) {
            // TODO: Could check that the class ID is valid...
            return static_cast<IMagmaNode*>( anim );
        }
    }

    return NULL;
}

void magma_max_node_base::set_max_object( IMagmaNode* node ) {
    if( node->ClassID() == this->get_class_id() ) {
        m_maxObjHandle = Animatable::GetHandleByAnim( node );
    } else {
        m_maxObjHandle = NULL;

        throw magma_exception() << magma_exception::node_id( get_id() )
                                << magma_exception::error_name(
                                       _T("**INTERNAL ERROR** Node was linked to incorrect 3ds Max object type: #(") +
                                       boost::lexical_cast<frantic::tstring>( node->ClassID().PartA() ) + _T(", ") +
                                       boost::lexical_cast<frantic::tstring>( node->ClassID().PartB() ) + _T(")") );
    }
}

IParamBlock2* magma_max_node_base::get_parameters() const {
    if( m_maxObjHandle != NULL ) { // Make sure the handle isn't NULL, since it throws a Max assertion in
                                   // Animatable::GetAnimByHandle() but its ok to call get_max_object() in this case.
        if( Animatable* anim = Animatable::GetAnimByHandle( m_maxObjHandle ) ) {
            return anim->GetParamBlockByID( IMagmaNode::kParamBlockID );
        }
    }

    return NULL;
}

} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic
