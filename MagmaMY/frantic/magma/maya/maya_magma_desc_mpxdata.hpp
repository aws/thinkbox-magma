// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <maya/MPxData.h>

#include "frantic/magma/maya/maya_magma_description.hpp"
#include <boost/shared_ptr.hpp>

namespace frantic {
namespace magma {
namespace maya {

class maya_magma_desc_mpxdata : public MPxData {
  private:
    desc::maya_magma_desc_ptr m_desc;

  public:
    static const MString typeName;
    static const MTypeId id;
    static void* creator();

  public:
    maya_magma_desc_mpxdata();

    ////////////////////////////////////////
    // override MPxData virtual function
    ////////////////////////////////////////
    virtual ~maya_magma_desc_mpxdata();

    virtual MStatus readASCII( const MArgList& argList, unsigned& endofTheLastParsedElement );

    virtual MStatus readBinary( std::istream& in, unsigned int length );

    virtual MStatus writeASCII( std::ostream& out );

    virtual MStatus writeBinary( std::ostream& out );

    // this only perform a shallow copy
    virtual void copy( const MPxData& src );

    MTypeId typeId() const;

    MString name() const;

    ////////////////////////////////////////
    // setter & getter
    ////////////////////////////////////////
    void set_maya_magma_desc( desc::maya_magma_desc_ptr inDesc );

    desc::maya_magma_desc_ptr get_maya_magma_desc() const;
};

} // namespace maya
} // namespace magma
} // namespace frantic
