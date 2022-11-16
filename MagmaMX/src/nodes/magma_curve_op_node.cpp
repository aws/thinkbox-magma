// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/max3d/MagmaMaxContext.hpp>
#include <frantic/magma/max3d/nodes/magma_curve_op_node.hpp>

#include <frantic/magma/nodes/magma_node_impl.hpp>
//#include <frantic/magma/simple_compiler/simple_compiler.hpp>
//#include <frantic/magma/simple_compiler/constraints.hpp>
#include <frantic/magma/simple_compiler/base_compiler.hpp>
#include <frantic/magma/simple_compiler/base_compiler_impl.hpp> //for traits class...
#include <frantic/math/splines/bezier_spline.hpp>

#include <frantic/max3d/particles/IMaxKrakatoaPRTObject.hpp>

#include <memory>

// ClassDesc2* GetMagmaCurveNodeDesc();

using frantic::magma::max3d::MagmaMaxNodeExtension;
using frantic::magma::simple_compiler::base_compiler;

template <>
MagmaMaxNodeExtension<frantic::magma::nodes::max3d::magma_curve_op_node::max_impl>::MyClassDesc
    MagmaMaxNodeExtension<frantic::magma::nodes::max3d::magma_curve_op_node::max_impl>::s_classDesc;

ClassDesc2* GetMagmaCurveNodeDesc() {
    return &frantic::magma::nodes::max3d::magma_curve_op_node::max_impl::s_classDesc;
}

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

enum fn_ids {
    kFnSetNumPoints,
    kFnGetNumPoints,
    kFnSetPoint,
    kFnGetPointPos,
    kFnGetPointIn,
    kFnGetPointOut,
    kFnGetPointFlags,
    kFnGetDomain,
    kFnSetDomain
};

#pragma warning( push )
#pragma warning( disable : 4238 )

FPStatus IMagmaCurve::_dispatch_fn( FunctionID fid, TimeValue t, FPValue& result, FPParams* p ) {
    FPStatus status = FPS_OK;
    switch( fid ) {
        VFN_1( kFnSetNumPoints, SetNumPoints, TYPE_INT );
        FN_0( kFnGetNumPoints, TYPE_INT, GetNumPoints );
        VFNT_5( kFnSetPoint, SetPoint, TYPE_INDEX, TYPE_POINT2, TYPE_POINT2, TYPE_POINT2, TYPE_INT );
        FNT_1( kFnGetPointPos, TYPE_POINT2_BV, GetPointPos, TYPE_INDEX );
        FNT_1( kFnGetPointIn, TYPE_POINT2_BV, GetPointIn, TYPE_INDEX );
        FNT_1( kFnGetPointOut, TYPE_POINT2_BV, GetPointOut, TYPE_INDEX );
        FN_1( kFnGetPointFlags, TYPE_INT, GetPointFlags, TYPE_INDEX );
        FN_0( kFnGetDomain, TYPE_POINT2_BV, GetDomain );
        VFN_1( kFnSetDomain, SetDomain, TYPE_POINT2 );
        END_FUNCTION_MAP

#pragma warning( pop )

        Interface_ID IMagmaCurve::s_interfaceID( 0x12b65041, 0x440951e6 );
        IMagmaCurve::FPInterfaceDescHolder IMagmaCurve::s_interfaceDescHolder;

        IMagmaCurve::FPInterfaceDescHolder::FPInterfaceDescHolder()
            : desc( IMagmaCurve::s_interfaceID, _T("IMagmaCurve"), /*string table entry*/ 0, GetMagmaCurveNodeDesc(),
                    FP_MIXIN, p_end ) {
            desc.AppendFunction( kFnSetNumPoints, _T("setNumPoints"), /*IDS_COUNT*/ 0, /*return type*/ TYPE_VOID,
                                 /*flags*/ 0,
                                 /*param count*/ 1, _T("count"), /*string table entry*/ 0, TYPE_INT, p_end );
            desc.AppendFunction( kFnGetNumPoints, _T("getNumPoints"), /*IDS_COUNT*/ 0, /*return type*/ TYPE_INT,
                                 /*flags*/ 0,
                                 /*param count*/ 0, p_end );
            desc.AppendFunction( kFnSetPoint, _T("setPoint"), /*IDS_COUNT*/ 0, /*return type*/ TYPE_VOID, /*flags*/ 0,
                                 /*param count*/ 5, _T("index"), /*string table entry*/ 0, TYPE_INDEX, _T("pos"),
                                 /*string table entry*/ 0, TYPE_POINT2, _T("in"), /*string table entry*/ 0, TYPE_POINT2,
                                 _T("out"), /*string table entry*/ 0, TYPE_POINT2, _T("flags"),
                                 /*string table entry*/ 0, TYPE_INT, p_end );
            desc.AppendFunction( kFnGetPointPos, _T("getPointPos"), /*IDS_COUNT*/ 0, /*return type*/ TYPE_POINT2_BV,
                                 /*flags*/ 0,
                                 /*param count*/ 1, _T("index"), /*string table entry*/ 0, TYPE_INDEX, p_end );
            desc.AppendFunction( kFnGetPointIn, _T("getPointIn"), /*IDS_COUNT*/ 0, /*return type*/ TYPE_POINT2_BV,
                                 /*flags*/ 0,
                                 /*param count*/ 1, _T("index"), /*string table entry*/ 0, TYPE_INDEX, p_end );
            desc.AppendFunction( kFnGetPointOut, _T("getPointOut"), /*IDS_COUNT*/ 0, /*return type*/ TYPE_POINT2_BV,
                                 /*flags*/ 0,
                                 /*param count*/ 1, _T("index"), /*string table entry*/ 0, TYPE_INDEX, p_end );
            desc.AppendFunction( kFnGetPointFlags, _T("getPointFlags"), /*IDS_COUNT*/ 0, /*return type*/ TYPE_INT,
                                 /*flags*/ 0,
                                 /*param count*/ 1, _T("index"), /*string table entry*/ 0, TYPE_INDEX, p_end );

            desc.AppendFunction( kFnGetDomain, _T("getDomain"), /*IDS_COUNT*/ 0, /*return type*/ TYPE_POINT2_BV,
                                 /*flags*/ 0,
                                 /*param count*/ 0, p_end );
            desc.AppendFunction( kFnSetDomain, _T("setDomain"), /*IDS_COUNT*/ 0, /*return type*/ TYPE_VOID, /*flags*/ 0,
                                 /*param count*/ 1, _T("min/max"), /*string table entry*/ 0, TYPE_POINT2, p_end );
        }

        MSTR magma_curve_op_node::max_impl::s_ClassName( _T("MagmaCurveNode") );
        // Class_ID magma_curve_op_node::max_impl::s_ClassID(0x5fb062e7, 0x97b4b05); //This needs to be defined in the
        // DLL, not in this project because it must be different for Krakatoa, Genome, etc.

        void magma_curve_op_node::max_impl::DefineParameters( ParamBlockDesc2 & paramDesc ) {
            paramDesc.AddParam( kCurvePoint, _T("curvePoints"), TYPE_POINT3_TAB, 0, P_ANIMATABLE, 0, p_end );
            paramDesc.AddParam( kCurvePointIn, _T("curvePointsIn"), TYPE_POINT3_TAB, 0, P_ANIMATABLE, 0, p_end );
            paramDesc.AddParam( kCurvePointOut, _T("curvePointsOut"), TYPE_POINT3_TAB, 0, P_ANIMATABLE, 0, p_end );
            paramDesc.AddParam( kCurvePointFlags, _T("curvePointsFlags"), TYPE_INT_TAB, 0, 0, 0, p_end );
            paramDesc.AddParam( kCurveXMin, _T("curveXMin"), TYPE_FLOAT, 0, 0, p_end );
            paramDesc.AddParam( kCurveXMax, _T("curveXMax"), TYPE_FLOAT, 0, 0, p_end );
            paramDesc.ParamOption( kCurveXMin, p_default, 0.f, p_end );
            paramDesc.ParamOption( kCurveXMax, p_default, 1.f, p_end );
        }

        magma_curve_op_node::max_impl::max_impl() {
            Point3 pts[] = { Point3( 0, 0, 0 ), Point3( 1, 1, 0 ) };
            Point3 ptsIn[] = { Point3( 0, 0, 0 ), Point3( 0, 0, 0 ) };
            Point3 ptsOut[] = { Point3( 1, 1, 0 ), Point3( 1, 1, 0 ) };
            int ptsFlags[] = { CURVEP_CORNER | CURVEP_BEZIER | CURVEP_ENDPOINT,
                               CURVEP_CORNER | CURVEP_BEZIER | CURVEP_ENDPOINT };

            m_pblock->SetCount( kCurvePoint, 2 );
            m_pblock->SetValue( kCurvePoint, 0, pts[0], 0 );
            m_pblock->SetValue( kCurvePoint, 0, pts[1], 1 );

            m_pblock->SetCount( kCurvePointIn, 2 );
            m_pblock->SetValue( kCurvePointIn, 0, ptsIn[0], 0 );
            m_pblock->SetValue( kCurvePointIn, 0, ptsIn[1], 1 );

            m_pblock->SetCount( kCurvePointOut, 2 );
            m_pblock->SetValue( kCurvePointOut, 0, ptsOut[0], 0 );
            m_pblock->SetValue( kCurvePointOut, 0, ptsOut[1], 1 );

            m_pblock->Append( kCurvePointFlags, 2, ptsFlags );
        }

        BaseInterface* magma_curve_op_node::max_impl::GetInterface( Interface_ID id ) {
            if( id == IMagmaCurve::s_interfaceID )
                return static_cast<IMagmaCurve*>( this );
            if( BaseInterface* bf = IMagmaCurve::GetInterface( id ) )
                return bf;
            return ReferenceTarget::GetInterface( id );
        }

        void magma_curve_op_node::max_impl::SetNumPoints( int count ) {
            m_pblock->SetCount( magma_curve_op_node::kCurvePoint, count );
            m_pblock->SetCount( magma_curve_op_node::kCurvePointIn, count );
            m_pblock->SetCount( magma_curve_op_node::kCurvePointOut, count );
            m_pblock->SetCount( magma_curve_op_node::kCurvePointFlags, count );
        }

        int magma_curve_op_node::max_impl::GetNumPoints() const {
            return m_pblock->Count( magma_curve_op_node::kCurvePoint );
        }

        void magma_curve_op_node::max_impl::SetDomain( const Point2& range ) {
            m_pblock->SetValue( kCurveXMin, 0, range.x );
            m_pblock->SetValue( kCurveXMax, 0, std::max( range.x, range.y ) );
        }

        Point2 magma_curve_op_node::max_impl::GetDomain() const {
            return Point2( m_pblock->GetFloat( kCurveXMin ), m_pblock->GetFloat( kCurveXMax ) );
        }

        void magma_curve_op_node::max_impl::SetPoint( int i, const Point2& pos, const Point2& in, const Point2& out,
                                                      int flags, TimeValue t ) {
            if( i >= 0 ) {
                Point3 vals[] = { Point3( pos.x, pos.y, 0.f ), Point3( in.x, in.y, 0.f ), Point3( out.x, out.y, 0.f ) };

                if( i < m_pblock->Count( magma_curve_op_node::kCurvePoint ) )
                    m_pblock->SetValue( magma_curve_op_node::kCurvePoint, t, vals[0], i );

                if( i < m_pblock->Count( magma_curve_op_node::kCurvePointIn ) )
                    m_pblock->SetValue( magma_curve_op_node::kCurvePointIn, t, vals[1], i );

                if( i < m_pblock->Count( magma_curve_op_node::kCurvePointOut ) )
                    m_pblock->SetValue( magma_curve_op_node::kCurvePointOut, t, vals[2], i );

                if( i < m_pblock->Count( magma_curve_op_node::kCurvePointFlags ) )
                    m_pblock->SetValue( magma_curve_op_node::kCurvePointFlags, t, flags, i );
            }
        }

        Point2 magma_curve_op_node::max_impl::GetPointPos( int i, TimeValue t ) {
            Point3 p( 0, 0, 0 );
            if( i >= 0 && i < m_pblock->Count( magma_curve_op_node::kCurvePoint ) )
                p = m_pblock->GetPoint3( magma_curve_op_node::kCurvePoint, t, i );
            return Point2( p.x, p.y );
        }

        Point2 magma_curve_op_node::max_impl::GetPointIn( int i, TimeValue t ) {
            Point3 p( 0, 0, 0 );
            if( i >= 0 && i < m_pblock->Count( magma_curve_op_node::kCurvePointIn ) )
                p = m_pblock->GetPoint3( magma_curve_op_node::kCurvePointIn, t, i );
            return Point2( p.x, p.y );
        }

        Point2 magma_curve_op_node::max_impl::GetPointOut( int i, TimeValue t ) {
            Point3 p( 0, 0, 0 );
            if( i >= 0 && i < m_pblock->Count( magma_curve_op_node::kCurvePointOut ) )
                p = m_pblock->GetPoint3( magma_curve_op_node::kCurvePointOut, t, i );
            return Point2( p.x, p.y );
        }

        int magma_curve_op_node::max_impl::GetPointFlags( int i ) {
            if( i >= 0 && i < m_pblock->Count( magma_curve_op_node::kCurvePointFlags ) )
                return m_pblock->GetInt( magma_curve_op_node::kCurvePointFlags, 0, i );
            return 0;
        }

        RefResult magma_curve_op_node::max_impl::NotifyRefChanged( const Interval& /*changeInt*/,
                                                                   RefTargetHandle hTarget, PartID& /*partID*/,
                                                                   RefMessage message, BOOL /*propagate*/ ) {
            if( hTarget == m_pblock ) {
                if( message == REFMSG_CHANGE )
                    return REF_SUCCEED;
            }
            return REF_DONTCARE;
        }

        using frantic::graphics2d::vector2f;
        using frantic::math::bezier_curve_point;

        namespace {
        class curve_expression : public base_compiler::expression {
            std::ptrdiff_t m_inPtr, m_outPtr;
            std::vector<bezier_curve_point<vector2f>> m_points;

            static void internal_apply( const base_compiler::expression* _this, base_compiler::state& data );

          public:
            void add_curve_point( const bezier_curve_point<vector2f>& point ) { m_points.push_back( point ); }

            virtual void set_input( std::size_t inputIndex, std::ptrdiff_t relPtr ) {
                if( inputIndex == 0 )
                    m_inPtr = relPtr;
            }

            virtual void set_output( std::ptrdiff_t relPtr ) { m_outPtr = relPtr; }

            virtual const frantic::channels::channel_map& get_output_map() const {
                return frantic::magma::simple_compiler::traits<float>::get_static_map();
            }

            virtual void apply( base_compiler::state& data ) const {
                float x = data.get_temporary<float>( m_inPtr );
                float y = 0;

                try {
                    if( !m_points.empty() ) {
                        if( x <= m_points.front().position.x )
                            y = m_points.front().position.y;
                        else if( x >= m_points.back().position.x )
                            y = m_points.back().position.y;
                        else
                            y = frantic::math::bezier_curve_x_to_y( m_points, x );
                    }
                } catch( ... ) {
                    y = 0; // We don't want any exceptions in here.
                }

                data.set_temporary( m_outPtr, y );
            }

            virtual runtime_ptr get_runtime_ptr() const { return &internal_apply; }
        };

        void curve_expression::internal_apply( const base_compiler::expression* _this, base_compiler::state& data ) {
            static_cast<const curve_expression*>( _this )->curve_expression::apply( data );
        }
        }

        FPInterface* const magma_curve_op_node::get_curve() const {
            return (FPInterface* const)get_max_object()->GetInterface( IMagmaCurve::s_interfaceID );
        }

        void magma_curve_op_node::compile_as_extension_type( frantic::magma::magma_compiler_interface & compiler ) {
            IMagmaCurve* curve = static_cast<IMagmaCurve*>( get_curve() );
            if( !curve )
                THROW_MAGMA_INTERNAL_ERROR();

            TimeValue t = TIME_NegInfinity;
            compiler.get_context_data().get_property( _T("Time"), t );

            if( frantic::magma::simple_compiler::base_compiler* sc =
                    dynamic_cast<frantic::magma::simple_compiler::base_compiler*>( &compiler ) ) {
                std::unique_ptr<curve_expression> expr( new curve_expression );

                Point2 domain = curve->GetDomain();

                for( int i = 0, iEnd = curve->GetNumPoints(); i < iEnd; ++i ) {
                    bezier_curve_point<vector2f> pt;

                    pt.position = frantic::max3d::from_max_t( curve->GetPointPos( i, t ) );
                    pt.inTan = frantic::max3d::from_max_t( curve->GetPointIn( i, t ) );
                    pt.outTan = frantic::max3d::from_max_t( curve->GetPointOut( i, t ) );

                    // Apply the real domain stored in the curve.
                    float origX = pt.position.x;

                    pt.position.x = frantic::math::lerp( domain.x, domain.y, pt.position.x );
                    pt.inTan.x = frantic::math::lerp( domain.x, domain.y, origX + pt.inTan.x ) - pt.position.x;
                    pt.outTan.x = frantic::math::lerp( domain.x, domain.y, origX + pt.outTan.x ) - pt.position.x;

                    expr->add_curve_point( pt );
                }

                std::vector<std::pair<int, int>> inputs;
                std::vector<frantic::magma::magma_data_type> inputTypes;

                inputs.push_back( compiler.get_node_input( *this, 0 ) );
                inputTypes.push_back( *frantic::magma::magma_singleton::get_named_data_type( _T("Float") ) );
                std::unique_ptr<base_compiler::expression> uniqueExpression(
                    static_cast<base_compiler::expression*>( expr.release() ) );

                sc->compile_expression( std::move( uniqueExpression ), this->get_id(), inputs, inputTypes );
            } else {
                magma_max_node_base::compile_as_extension_type( compiler );
            }
        }
    }
}
}
}
