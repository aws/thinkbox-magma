// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/nodes/magma_node_property.hpp>
#include <frantic/magma/nodes/magma_simple_operator.hpp>

#include <frantic/magma/max3d/MagmaMaxNodeExtension.hpp>

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

class IMagmaCurve : public FPMixinInterface {
  public:
    virtual void SetNumPoints( int count ) = 0;
    virtual int GetNumPoints() const = 0;

    virtual Point2 GetDomain() const = 0;
    virtual void SetDomain( const Point2& range ) = 0;

    virtual void SetPoint( int i, const Point2& pos, const Point2& in, const Point2& out, int flags, TimeValue t ) = 0;
    virtual Point2 GetPointPos( int i, TimeValue t ) = 0;
    virtual Point2 GetPointIn( int i, TimeValue t ) = 0;
    virtual Point2 GetPointOut( int i, TimeValue t ) = 0;
    virtual int GetPointFlags( int i ) = 0;

    virtual FPStatus _dispatch_fn( FunctionID fid, TimeValue t, FPValue& result, FPParams* p );

  public:
    static Interface_ID s_interfaceID;

    struct FPInterfaceDescHolder {
        FPInterfaceDesc desc;
        FPInterfaceDescHolder();
    } static s_interfaceDescHolder;

    virtual FPInterfaceDesc* GetDesc() { return &s_interfaceDescHolder.desc; }
};

class magma_curve_op_node : public magma_simple_operator<1, magma_max_node_base> {
  public:
    enum { kCurvePoint = IMagmaNode::kID + 1, kCurvePointIn, kCurvePointOut, kCurvePointFlags, kCurveXMin, kCurveXMax };

    class max_impl : public MagmaMaxNodeExtension<max_impl>, public IMagmaCurve {
      public:
        static MSTR s_ClassName;
        static Class_ID s_ClassID;

        static void DefineParameters( ParamBlockDesc2& paramDesc );

        max_impl();

        virtual void SetNumPoints( int count );
        virtual int GetNumPoints() const;

        virtual Point2 GetDomain() const;
        virtual void SetDomain( const Point2& range );

        virtual void SetPoint( int i, const Point2& pos, const Point2& in, const Point2& out, int flags, TimeValue t );
        virtual Point2 GetPointPos( int i, TimeValue t );
        virtual Point2 GetPointIn( int i, TimeValue t );
        virtual Point2 GetPointOut( int i, TimeValue t );
        virtual int GetPointFlags( int i );

        virtual RefResult NotifyRefChanged( const Interval& changeInt, RefTargetHandle hTarget, PartID& partID,
                                            RefMessage message, BOOL propagate );

        virtual BaseInterface* GetInterface( Interface_ID id );
    };

  public:
    MAGMA_MAX_REQUIRED_METHODS( magma_curve_op_node );

    FPInterface* const get_curve() const;
};

} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic
