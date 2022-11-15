// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <assert.h>

#include <maya/MArgList.h>
#include <maya/MPxData.h>
#include <maya/MString.h>

#include <boost/shared_ptr.hpp>

#include "frantic/maya/parser_args.hpp"
#include <frantic/logging/logging_level.hpp>

#include "frantic/magma/maya/maya_magma_desc_mpxdata.hpp"
#include "frantic/magma/maya/maya_magma_description.hpp"

#include "frantic/magma/maya/maya_magma_exception.hpp"

namespace frantic {
namespace magma {
namespace maya {

/// make sure this ID matches the ID in wiki
const MTypeId maya_magma_desc_mpxdata::id( 0x0011748a );
const MString maya_magma_desc_mpxdata::typeName( "MagmaDescMPxData" );

void* maya_magma_desc_mpxdata::creator() { return new maya_magma_desc_mpxdata; }

maya_magma_desc_mpxdata::maya_magma_desc_mpxdata()
    : m_desc() {}

maya_magma_desc_mpxdata::~maya_magma_desc_mpxdata() {}

void maya_magma_desc_mpxdata::set_maya_magma_desc( desc::maya_magma_desc_ptr inDesc ) { m_desc = inDesc; }

desc::maya_magma_desc_ptr maya_magma_desc_mpxdata::get_maya_magma_desc() const { return m_desc; }

MStatus maya_magma_desc_mpxdata::readASCII( const MArgList& argList, unsigned int& endofTheLastParsedElement ) {
    MStatus outStatus( MS::kFailure );
    MString inputData;
    if( MArgList::kInvalidArgIndex != endofTheLastParsedElement )
        outStatus = argList.get( endofTheLastParsedElement, inputData );

    std::istringstream inputStream( inputData.asChar() );
    readBinary( inputStream, inputData.length() );
    return outStatus;
}

MStatus maya_magma_desc_mpxdata::writeASCII( std::ostream& out ) {
    out << "\"";
    FF_LOG( debug ) << "maya_magma_desc_mpxdata::writeASCII()" << std::endl;
    m_desc->to_stream( out, true );
    out << "\"";
    return MS::kSuccess;
}

MStatus maya_magma_desc_mpxdata::readBinary( std::istream& in, unsigned int length ) {
    MStatus outStatus( MS::kFailure );

    FF_LOG( debug ) << "maya_magma_desc_mpxdata::readBinary()" << std::endl;
    assert( m_desc == NULL );
    // because m_desc haven't constructor yet, we have to create a one for it
    m_desc = boost::make_shared<frantic::magma::maya::desc::maya_magma_desc>();
    m_desc->from_stream( in, length );
    outStatus = MS::kSuccess;
    return outStatus;
}

MStatus maya_magma_desc_mpxdata::writeBinary( std::ostream& out ) {
    FF_LOG( debug ) << "maya_magma_desc_mpxdata::writeBinary()" << std::endl;
    m_desc->to_stream( out, false );
    return MS::kSuccess;
}

void maya_magma_desc_mpxdata::copy( const MPxData& src ) {
    if( src.typeId() != maya_magma_desc_mpxdata::id )
        throw maya_magma_exception(
            "maya_magma_desc_mpxdata::copy failed. src MPxData is not maya_magma_desc_mpxdata" );

    const maya_magma_desc_mpxdata* psrc = static_cast<const maya_magma_desc_mpxdata*>( &src );
    if( psrc ) {
        m_desc = ( psrc->m_desc );
    } else
        throw maya_magma_exception(
            "maya_magma_desc_mpxdata::copy failed. src MPxData is not maya_magma_desc_mpxdata" );
}

MTypeId maya_magma_desc_mpxdata::typeId() const { return maya_magma_desc_mpxdata::id; }

MString maya_magma_desc_mpxdata::name() const { return maya_magma_desc_mpxdata::typeName; }

} // namespace maya
} // namespace magma
} // namespace frantic
