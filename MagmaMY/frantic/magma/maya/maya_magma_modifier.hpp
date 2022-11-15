// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>

#include <frantic/graphics/transform4f.hpp>
#include <frantic/particles/streams/particle_istream.hpp>

#include <frantic/magma/magma_exception.hpp>

#include "frantic/magma/maya/maya_magma_datatypes.hpp"
#include "frantic/magma/maya/maya_magma_desc_mpxdata.hpp"
#include "frantic/magma/maya/maya_magma_description.hpp"
#include "frantic/magma/maya/maya_magma_exception.hpp"

namespace frantic {
namespace magma {
namespace maya {
namespace modifier {

void maya_magma_error_callback( const frantic::magma::magma_exception& e );

class maya_magma_modifier {
  private:
    typedef particles::streams::particle_istream_ptr particle_istream_ptr;

    // XXX use weak_ptr ?
    desc::maya_magma_desc_ptr m_desc;

    MObject& m_surShapeNode;

    frantic::graphics::transform4f m_worldTransform;

    frantic::graphics::transform4f m_cameraTransform;

  public:
    maya_magma_modifier( MObject& obj, const frantic::tstring& mayaMagmaPlugName,
                         const frantic::graphics::transform4f& worldTransform,
                         const frantic::graphics::transform4f& cameraTransform );

    ~maya_magma_modifier();

    particle_istream_ptr get_modified_particle_istream( particle_istream_ptr inStream ) const;
};

} // namespace modifier
} // namespace maya
} // namespace magma
} // namespace frantic
