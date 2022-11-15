// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <maya/MFnDependencyNode.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MMatrix.h>

#include <frantic/graphics/transform4f.hpp>
#include <frantic/logging/logging_level.hpp>
#include <frantic/magma/magma_compiler_interface.hpp>
#include <frantic/maya/convert.hpp>

#include "frantic/magma/maya/maya_magma_datatypes.hpp"
#include "frantic/magma/maya/maya_magma_exception.hpp"

namespace frantic {
namespace magma {
namespace maya {
namespace modifier {

class maya_magma_context_interface : public frantic::magma::magma_compiler_interface::context_base {
  private:
    frantic::graphics::transform4f m_worldTransform;
    frantic::graphics::transform4f m_worldTransformInverse;
    frantic::graphics::transform4f m_cameraTransform;
    frantic::graphics::transform4f m_cameraTransformInverse;

  public:
    explicit maya_magma_context_interface( const frantic::graphics::transform4f& worldTransform,
                                           const frantic::graphics::transform4f& cameraTransform )
        : m_worldTransform( worldTransform )
        , m_cameraTransform( cameraTransform ) {
        m_worldTransformInverse = m_worldTransform.to_inverse();
        m_cameraTransformInverse = m_cameraTransform.to_inverse();
    }

    virtual frantic::tstring get_name() const { return _T("Maya"); }

    virtual frantic::graphics::transform4f get_world_transform( bool inverse ) const {
        return !inverse ? m_worldTransform : m_worldTransformInverse;
    }

    virtual frantic::graphics::transform4f get_camera_transform( bool inverse ) const {
        return !inverse ? m_cameraTransform : m_cameraTransformInverse;
    }
};

} // namespace modifier
} // namespace maya
} // namespace magma
} // namespace frantic
