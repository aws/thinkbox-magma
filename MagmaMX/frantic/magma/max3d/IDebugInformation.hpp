// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#define DebugInformation_INTERFACE Interface_ID( 0x30853579, 0x686878e7 )

// Forward decl.
class Array;
class Value;

namespace frantic {
namespace magma {
namespace max3d {

class IDebugInformation : public FPMixinInterface {
  public:
    virtual ~IDebugInformation() {}

    virtual FPInterfaceDesc* GetDesc();

    virtual int GetNumIterations() const = 0;
    virtual Value* GetNodeValue( int iteration, int nodeID, int outputIndex ) const = 0;
    virtual Array* GetNodeMinMaxMeanValue( int nodeID, int outputIndex ) const = 0;

  protected:
    enum {
        kFnGetNumIterations,
        kFnGetNodeValue,
        kFnGetNodeMinMaxMeanValue,
    };

#pragma warning( push )
#pragma warning( disable : 4238 4100 )
    BEGIN_FUNCTION_MAP
    RO_PROP_FN( kFnGetNumIterations, GetNumIterations, TYPE_INT )
    FN_3( kFnGetNodeValue, TYPE_VALUE, GetNodeValue, TYPE_INDEX, TYPE_INT, TYPE_INDEX )
    FN_2( kFnGetNodeMinMaxMeanValue, TYPE_VALUE, GetNodeMinMaxMeanValue, TYPE_INT, TYPE_INDEX )
    END_FUNCTION_MAP;
#pragma warning( pop )
};

} // namespace max3d
} // namespace magma
} // namespace frantic
