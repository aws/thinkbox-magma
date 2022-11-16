// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#pragma warning( push, 3 )
#include <imtl.h>
#pragma warning( pop )

#include <memory>

namespace frantic {
namespace magma {
namespace max3d {

/**
 * This ShadeContext subclass is suitable for evaluating Texmaps and Mtls outside of an actual renderer. It can be used
 * with a variety of sources, and does not assume it is bound to a Mesh instance. It DOES require a pointer to an
 * external RenderGlobalContext object however.
 *
 * If you wish to expose further information associated with the object it is bound to, simply subclass this object and
 * override the relevent functions. A prime candidate is 'Point3 UVW(int channel);'
 *
 * @note All quatities are expected to be in camera-space, and the RenderGlobalContext::camToWorld matrix will be used
 * to produce world space quantities.
 */
class MagmaShadeContext : public ShadeContext {
  public:
    MagmaShadeContext();

    virtual ~MagmaShadeContext();

    void set_render_global_context( RenderGlobalContext* globContext );
    void set_render_instance( RenderInstance* inst, INode* node );

    virtual std::unique_ptr<MagmaShadeContext> clone() const;

#pragma warning( push )
#pragma warning( disable : 4100 )
    virtual BOOL InMtlEditor();

    virtual int Antialias();

    virtual int ProjType();

    virtual LightDesc* Light( int n );

    virtual TimeValue CurTime();

    virtual int NodeID();

    virtual INode* Node();

    virtual Object* GetEvalObject();

    // virtual Point3 BarycentricCoords() { return Point3(0,0,0);}

    virtual int FaceNumber();

    virtual Point3 Normal();

    virtual void SetNormal( Point3 p );

    virtual Point3 OrigNormal();

    virtual Point3 GNormal();

    // virtual float  Curve() { return 0.0f; }

    virtual Point3 V();

    virtual void SetView( Point3 p );

    virtual Point3 OrigView();

    virtual Point3 ReflectVector();

    virtual Point3 RefractVector( float ior );

    virtual void SetIOR( float ior );

    virtual float GetIOR();

    virtual Point3 CamPos();

    virtual Point3 P();

    virtual Point3 DP();

    // virtual void DP(Point3& dpdx, Point3& dpdy){}

    virtual Point3 PObj();

    virtual Point3 DPObj();

    virtual Box3 ObjectBox();

    virtual Point3 PObjRelBox();

    virtual Point3 DPObjRelBox();

    virtual void ScreenUV( Point2& uv, Point2& duv );

    virtual IPoint2 ScreenCoord();

    // virtual Point2 SurfacePtScreen(){ return Point2(0.0,0.0); }

    virtual Point3 UVW( int channel );

    virtual Point3 DUVW( int channel );

    virtual void DPdUVW( Point3 dP[3], int channel );

    // virtual int BumpBasisVectors(Point3 dP[2], int axis, int channel=0) { return 0; }

    // virtual BOOL IsSuperSampleOn(){ return FALSE; }
    // virtual BOOL IsTextureSuperSampleOn(){ return FALSE; }
    // virtual int GetNSuperSample(){ return 0; }
    // virtual float GetSampleSizeScale(){ return 1.0f; }

    // virtual Point3 UVWNormal(int channel=0) { return Point3(0,0,1); }

    // virtual float RayDiam() { return Length(DP()); }

    // virtual float RayConeAngle() { return 0.0f; }

    // virtual AColor EvalEnvironMap(Texmap *map, Point3 view);

    virtual AColor EvalGlobalEnvironMap( Point3 dir );

    virtual void GetBGColor( Color& bgcol, Color& transp, BOOL fogBG );

    // virtual float CamNearRange() {return 0.0f;}

    // virtual float CamFarRange() {return 0.0f;}

    virtual Point3 PointTo( const Point3& p, RefFrame ito );

    virtual Point3 PointFrom( const Point3& p, RefFrame ifrom );

    virtual Point3 VectorTo( const Point3& p, RefFrame ito );

    virtual Point3 VectorFrom( const Point3& p, RefFrame ifrom );

    virtual Point3 VectorToNoScale( const Point3& p, RefFrame ito );

    virtual Point3 VectorFromNoScale( const Point3& p, RefFrame ifrom );
#pragma warning( pop )

  public:
    Point3 m_pos, m_view, m_origView, m_normal, m_origNormal;
    float m_ior;

  private:
    Matrix3 m_toWorld, m_toObject, m_fromWorld, m_fromObject;

    RenderInstance* m_renderInstance;
    INode* m_node; // Only used if m_renderInstance == NULL;

  protected:
    static void do_clone( MagmaShadeContext& dest, const MagmaShadeContext& src );
};

} // namespace max3d
} // namespace magma
} // namespace frantic
