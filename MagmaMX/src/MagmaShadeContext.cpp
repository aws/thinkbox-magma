// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"
#include <frantic/magma/max3d/MagmaShadeContext.hpp>

#include <frantic/max3d/node_transform.hpp>

#pragma warning( push, 3 )
#include <render.h>
#pragma warning( pop )

#include <memory>

namespace frantic {
namespace magma {
namespace max3d {

// Like VectorTransform( Point3, Matrix3 ) expect it transposes the matrix first.
inline Point3 VectorTransposeTransform( const Point3& p, const Matrix3& m ) {
    return Point3( DotProd( p, m[0] ), DotProd( p, m[1] ), DotProd( p, m[2] ) );
}

MagmaShadeContext::MagmaShadeContext() {
    /*mode = SCMODE_NORMAL; nLights = 0; shadow = TRUE;  rayLevel = 0;
    globContext = NULL; atmosSkipLight = NULL;*/

    this->doMaps = TRUE;
    this->filterMaps = TRUE;
    this->backFace = FALSE;
    this->mtlNum = 0;
    this->ambientLight.White();
    this->xshadeID = 0;

    m_ior = 1.f;
    m_renderInstance = NULL;
    m_node = NULL;
}

MagmaShadeContext::~MagmaShadeContext() {}

void MagmaShadeContext::set_render_global_context( RenderGlobalContext* globContext ) {
    this->globContext = globContext;

    m_toWorld = globContext->camToWorld;
    m_fromWorld = globContext->worldToCam;
}

void MagmaShadeContext::set_render_instance( RenderInstance* inst, INode* node ) {
    m_renderInstance = inst;
    m_node = node;

    if( inst ) {
        this->nLights = m_renderInstance->NumLights();

        m_fromObject = m_renderInstance->objToCam;
        m_toObject = m_renderInstance->camToObj;
    } else if( node ) {
        Interval valid = FOREVER;
        Matrix3 nodeTM = frantic::max3d::get_node_transform( node, globContext->time, valid );

        m_fromObject = nodeTM * m_fromWorld;
        m_toObject = m_toWorld * Inverse( nodeTM );
    } else {
        m_fromObject.IdentityMatrix();
        m_toObject.IdentityMatrix();
    }
}

std::unique_ptr<MagmaShadeContext> MagmaShadeContext::clone() const {
    std::unique_ptr<MagmaShadeContext> result( new MagmaShadeContext );

    do_clone( *result, *this );

    return result;
}

void MagmaShadeContext::do_clone( MagmaShadeContext& dest, const MagmaShadeContext& src ) {
    dest.mode = src.mode;
    dest.nLights = src.nLights;
    dest.shadow = src.shadow;
    dest.rayLevel = src.rayLevel;
    dest.globContext = src.globContext;
    dest.atmosSkipLight = src.atmosSkipLight;
    dest.doMaps = src.doMaps;
    dest.filterMaps = src.filterMaps;
    dest.backFace = src.backFace;
    dest.mtlNum = src.mtlNum;
    dest.ambientLight = src.ambientLight;
    dest.xshadeID = src.xshadeID;

    dest.m_fromObject = src.m_fromObject;
    dest.m_fromWorld = src.m_fromWorld;
    dest.m_toObject = src.m_toObject;
    dest.m_toWorld = src.m_toWorld;
    dest.m_ior = src.m_ior;
    dest.m_node = src.m_node;
    dest.m_normal = src.m_normal;
    dest.m_view = src.m_view;
    dest.m_origNormal = src.m_origNormal;
    dest.m_origView = src.m_origView;
    dest.m_pos = src.m_pos;
    dest.m_renderInstance = src.m_renderInstance;
}

#pragma warning( push )
#pragma warning( disable : 4100 )
BOOL MagmaShadeContext::InMtlEditor() { return this->globContext->inMtlEdit; }

int MagmaShadeContext::Antialias() { return globContext->antialias; }

int MagmaShadeContext::ProjType() { return globContext->projType; }

LightDesc* MagmaShadeContext::Light( int n ) { return m_renderInstance ? m_renderInstance->Light( n ) : NULL; }

TimeValue MagmaShadeContext::CurTime() { return globContext->time; }

// I used to return m_node->GetRenderID(), but that value is set by the renderer and it usually is bunk. The Raytrace
// texmap seems to this as an index into RenderGlobalContext::GetRenderInstance() which causes a crash when it doesn't
// actually correspond.
int MagmaShadeContext::NodeID() { return m_renderInstance ? m_renderInstance->nodeID : -1; }

INode* MagmaShadeContext::Node() { return m_renderInstance ? m_renderInstance->GetINode() : m_node; }

Object* MagmaShadeContext::GetEvalObject() { return m_renderInstance ? m_renderInstance->GetEvalObject() : NULL; }

// virtual Point3 BarycentricCoords() { return Point3(0,0,0);}

int MagmaShadeContext::FaceNumber() {
    return 0; // Can't return -1 since some Texmaps (thinking of Raytrace texmap) will blindly access the associated
              // RenderInstance's data using this index.
}

Point3 MagmaShadeContext::Normal() { return m_normal; }

void MagmaShadeContext::SetNormal( Point3 p ) { m_normal = p; }

Point3 MagmaShadeContext::OrigNormal() { return m_origNormal; }

Point3 MagmaShadeContext::GNormal() { return m_normal; }

// virtual float  Curve() { return 0.0f; }

Point3 MagmaShadeContext::V() { return m_view; }

void MagmaShadeContext::SetView( Point3 p ) { m_view = p; }

Point3 MagmaShadeContext::OrigView() { return m_origView; }

Point3 MagmaShadeContext::ReflectVector() { return m_view - 2 * DotProd( m_view, m_normal ) * m_normal; }

Point3 MagmaShadeContext::RefractVector( float ior ) {
    // Taken from Max SDK cjrender sample.
    float VN, nur, k;
    VN = DotProd( -m_view, m_normal );
    if( backFace )
        nur = ior;
    else
        nur = ( ior != 0.0f ) ? 1.0f / ior : 1.0f;
    k = 1.0f - nur * nur * ( 1.0f - VN * VN );
    if( k <= 0.0f ) {
        // Total internal reflection:
        return ReflectVector();
    } else {
        return ( nur * VN - (float)sqrt( k ) ) * m_normal + nur * m_view;
    }
}

void MagmaShadeContext::SetIOR( float ior ) { m_ior = ior; }

float MagmaShadeContext::GetIOR() { return m_ior; }

Point3 MagmaShadeContext::CamPos() { return Point3( 0, 0, 0 ); }

Point3 MagmaShadeContext::P() { return m_pos; }

Point3 MagmaShadeContext::DP() { return Point3( 0, 0, 0 ); }

// virtual void DP(Point3& dpdx, Point3& dpdy){}

Point3 MagmaShadeContext::PObj() { return this->PointTo( P(), REF_OBJECT ); }

Point3 MagmaShadeContext::DPObj() { return Point3( 0, 0, 0 ); }

Box3 MagmaShadeContext::ObjectBox() { return m_renderInstance ? m_renderInstance->obBox : Box3(); }

Point3 MagmaShadeContext::PObjRelBox() {
    if( m_renderInstance ) {
        Point3 pRel = ( PObj() - m_renderInstance->obBox.pmin );
        pRel.x /= ( m_renderInstance->obBox.pmax.x - m_renderInstance->obBox.pmin.x );
        pRel.x = 2.f * ( pRel.x - 0.5f );
        pRel.y /= ( m_renderInstance->obBox.pmax.y - m_renderInstance->obBox.pmin.y );
        pRel.y = 2.f * ( pRel.y - 0.5f );
        pRel.z /= ( m_renderInstance->obBox.pmax.z - m_renderInstance->obBox.pmin.z );
        pRel.z = 2.f * ( pRel.z - 0.5f );
        return pRel;
    } else {
        return Point3( 0, 0, 0 );
    }
}

Point3 MagmaShadeContext::DPObjRelBox() { return Point3( 0, 0, 0 ); }

void MagmaShadeContext::ScreenUV( Point2& uv, Point2& duv ) {}

IPoint2 MagmaShadeContext::ScreenCoord() { return IPoint2( 0, 0 ); }

// virtual Point2 SurfacePtScreen(){ return Point2(0.0,0.0); }

Point3 MagmaShadeContext::UVW( int channel ) { return Point3( 0, 0, 0 ); }

Point3 MagmaShadeContext::DUVW( int channel ) { return Point3( 0, 0, 0 ); }

void MagmaShadeContext::DPdUVW( Point3 dP[3], int channel ) {}

// virtual int BumpBasisVectors(Point3 dP[2], int axis, int channel=0) { return 0; }

// virtual BOOL IsSuperSampleOn(){ return FALSE; }
// virtual BOOL IsTextureSuperSampleOn(){ return FALSE; }
// virtual int GetNSuperSample(){ return 0; }
// virtual float GetSampleSizeScale(){ return 1.0f; }

// virtual Point3 UVWNormal(int channel=0) { return Point3(0,0,1); }

// virtual float RayDiam() { return Length(DP()); }

// virtual float RayConeAngle() { return 0.0f; }

// virtual AColor EvalEnvironMap(Texmap *map, Point3 view);

AColor MagmaShadeContext::EvalGlobalEnvironMap( Point3 dir ) {
    Ray r;

    // I'm really unsure of these quantities. Should they be in world space?!
    // r.dir = this->VectorTo( dir, REF_WORLD );
    // r.p = this->PointTo( this->CamPos(), REF_WORLD );
    r.dir = dir;
    r.p = this->CamPos();

    return globContext->EvalGlobalEnvironMap( *this, r, FALSE );
}

void MagmaShadeContext::GetBGColor( Color& bgcol, Color& transp, BOOL fogBG ) {
    // TODO Use globContext->atmos & globContext->EvalGlobalEnvironMap() to generate a color ...
}

// virtual float CamNearRange() {return 0.0f;}

// virtual float CamFarRange() {return 0.0f;}

Point3 MagmaShadeContext::PointTo( const Point3& p, RefFrame ito ) {
    switch( ito ) {
    case REF_WORLD:
        return m_toWorld.PointTransform( p );
    case REF_OBJECT:
        return m_toObject.PointTransform( p );
    default:
    case REF_CAMERA:
        return p;
    }
}

Point3 MagmaShadeContext::PointFrom( const Point3& p, RefFrame ifrom ) {
    switch( ifrom ) {
    case REF_WORLD:
        return m_fromWorld.PointTransform( p );
    case REF_OBJECT:
        return m_fromObject.PointTransform( p );
    default:
    case REF_CAMERA:
        return p;
    }
}

Point3 MagmaShadeContext::VectorTo( const Point3& p, RefFrame ito ) {
    switch( ito ) {
    case REF_WORLD:
        return m_toWorld.VectorTransform( p );
    case REF_OBJECT:
        return m_toObject.VectorTransform( p );
    default:
    case REF_CAMERA:
        return p;
    }
}

Point3 MagmaShadeContext::VectorFrom( const Point3& p, RefFrame ifrom ) {
    switch( ifrom ) {
    case REF_WORLD:
        return m_fromWorld.VectorTransform( p );
    case REF_OBJECT:
        return m_fromObject.VectorTransform( p );
    default:
    case REF_CAMERA:
        return p;
    }
}

Point3 MagmaShadeContext::VectorToNoScale( const Point3& p, RefFrame ito ) {
    switch( ito ) {
    case REF_WORLD:
        return VectorTransposeTransform( p, m_fromWorld ); // Multiply by the transpose of the inverse
    case REF_OBJECT:
        return VectorTransposeTransform( p, m_fromObject ); // Multiply by the transpose of the inverse
    default:
    case REF_CAMERA:
        return p;
    }
}

Point3 MagmaShadeContext::VectorFromNoScale( const Point3& p, RefFrame ifrom ) {
    switch( ifrom ) {
    case REF_WORLD:
        return VectorTransposeTransform( p, m_toWorld ); // Multiply by the transpose of the inverse
    case REF_OBJECT:
        return VectorTransposeTransform( p, m_toObject ); // Multiply by the transpose of the inverse
    default:
    case REF_CAMERA:
        return p;
    }
}
#pragma warning( pop )

} // namespace max3d
} // namespace magma
} // namespace frantic
