// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/max3d/DebugInformation.hpp>
#include <frantic/max3d/maxscript/maxscript.hpp>

#pragma warning( push, 3 )
#pragma warning( disable : 4510 4512 4244 )
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#pragma warning( pop )

namespace frantic {
namespace magma {
namespace max3d {

FPInterfaceDesc* IDebugInformation::GetDesc() {
    static FPInterfaceDesc theDesc( DebugInformation_INTERFACE, _T("DebugInformation"), 0, NULL, FP_MIXIN, p_end );

    if( theDesc.functions.Count() == 0 ) {
        theDesc.AppendProperty( kFnGetNumIterations, FP_NO_FUNCTION, _T("Count"), 0, TYPE_INT, p_end );
        theDesc.AppendFunction( kFnGetNodeValue, _T("GetNodeValue"), 0, TYPE_VALUE, 0, 3, _T("IterationIndex"), 0,
                                TYPE_INDEX, _T("NodeID"), 0, TYPE_INT, _T("NodeOutputIndex"), 0, TYPE_INDEX, p_end );
        theDesc.AppendFunction( kFnGetNodeMinMaxMeanValue, _T("GetNodeMinMaxMeanValue"), 0, TYPE_VALUE, 0, 2,
                                _T("NodeID"), 0, TYPE_INT, _T("NodeOutputIndex"), 0, TYPE_INDEX, p_end );
    }

    return &theDesc;
}

namespace {
class to_mxsvalue : public boost::static_visitor<Value*>, boost::noncopyable {
  public:
    Value* operator()( boost::blank ) const { return &undefined; }

    Value* operator()( int val ) const {
        one_value_local( result );
        vl.result = Integer::heap_intern( val );
        return_value( vl.result );
    }

    Value* operator()( float val ) const {
        one_value_local( result );
        vl.result = Float::heap_intern( val );
        return_value( vl.result );
    }

    Value* operator()( bool val ) const {
        one_value_local( result );
        vl.result = val ? &true_value : &false_value;
        return_value( vl.result );
    }

    Value* operator()( const frantic::graphics::vector3f& val ) const {
        Point3 p = frantic::max3d::to_max_t( val );

        one_value_local( result );
        vl.result = new( GC_IN_HEAP ) Point3Value( p );
        return_value( vl.result );
    }

    Value* operator()( const frantic::graphics::quat4f& val ) const {
        Quat q = frantic::max3d::to_max_t( val );

        one_value_local( result );
        vl.result = new( GC_IN_HEAP ) QuatValue( q );
        return_value( vl.result );
    }
};
} // namespace

int DebugInformation::GetNumIterations() const { return (int)m_data.size(); }

Value* DebugInformation::GetNodeValue( int iteration, int nodeID, int outputIndex ) const {
    one_value_local( result );
    vl.result = &undefined;

    if( (std::size_t)iteration < m_data.size() ) {
        frantic::magma::debug_data::const_iterator itNode = m_data[iteration].find( nodeID );
        if( itNode != m_data[iteration].end() && (std::size_t)outputIndex < itNode->second.size() )
            vl.result = boost::apply_visitor( to_mxsvalue(), itNode->second[outputIndex] );
    }

    return_value( vl.result );
}

Array* DebugInformation::GetNodeMinMaxMeanValue( int nodeID, int outputIndex ) const {
    using namespace boost::accumulators;

    four_typed_value_locals( Array * result, Value * minVal, Value * maxVal, Value * meanVal );
    vl.result = NULL;

    if( !m_data.empty() ) {
        frantic::magma::debug_data::const_iterator itNode = m_data.front().find( nodeID );
        if( itNode != m_data.front().end() && (std::size_t)outputIndex < itNode->second.size() ) {
            if( itNode->second[outputIndex].type() == typeid( int ) ) {
                accumulator_set<int, features<tag::min, tag::max, tag::mean>> acc;

                // We've determined the type, now for each row, find the node and relevant value and accumulate it.
                for( storage_type::const_iterator it = m_data.begin(), itEnd = m_data.end(); it != itEnd; ++it ) {
                    frantic::magma::debug_data::const_iterator itCurNode = it->find( nodeID );
                    if( itCurNode != it->end() && (std::size_t)outputIndex < itCurNode->second.size() )
                        acc( boost::get<int>( itCurNode->second[outputIndex] ) );
                }

                vl.minVal = Integer::heap_intern( boost::accumulators::min( acc ) );
                vl.maxVal = Integer::heap_intern( boost::accumulators::max( acc ) );
                vl.meanVal = Float::heap_intern( (float)boost::accumulators::mean( acc ) );
            } else if( itNode->second[outputIndex].type() == typeid( float ) ) {
                accumulator_set<float, features<tag::min, tag::max, tag::mean>> acc;

                // We've determined the type, now for each row, find the node and relevant value and accumulate it.
                for( storage_type::const_iterator it = m_data.begin(), itEnd = m_data.end(); it != itEnd; ++it ) {
                    frantic::magma::debug_data::const_iterator itCurNode = it->find( nodeID );
                    if( itCurNode != it->end() && (std::size_t)outputIndex < itCurNode->second.size() )
                        acc( boost::get<float>( itCurNode->second[outputIndex] ) );
                }

                vl.minVal = Float::heap_intern( boost::accumulators::min( acc ) );
                vl.maxVal = Float::heap_intern( boost::accumulators::max( acc ) );
                vl.meanVal = Float::heap_intern( boost::accumulators::mean( acc ) );
            } else if( itNode->second[outputIndex].type() == typeid( frantic::graphics::vector3f ) ) {
                accumulator_set<float, features<tag::min, tag::max, tag::mean>> acc1, acc2, acc3;

                // We've determined the type, now for each row, find the node and relevant value and accumulate it.
                for( storage_type::const_iterator it = m_data.begin(), itEnd = m_data.end(); it != itEnd; ++it ) {
                    frantic::magma::debug_data::const_iterator itCurNode = it->find( nodeID );
                    if( itCurNode != it->end() && (std::size_t)outputIndex < itCurNode->second.size() ) {
                        frantic::graphics::vector3f v =
                            boost::get<frantic::graphics::vector3f>( itCurNode->second[outputIndex] );
                        acc1( v.x );
                        acc2( v.y );
                        acc3( v.z );
                    }
                }

                Point3 pmin( boost::accumulators::min( acc1 ), boost::accumulators::min( acc2 ),
                             boost::accumulators::min( acc3 ) );
                Point3 pmax( boost::accumulators::max( acc1 ), boost::accumulators::max( acc2 ),
                             boost::accumulators::max( acc3 ) );
                Point3 pmean( boost::accumulators::mean( acc1 ), boost::accumulators::mean( acc2 ),
                              boost::accumulators::mean( acc3 ) );

                vl.minVal = new( GC_IN_HEAP ) Point3Value( pmin );
                vl.maxVal = new( GC_IN_HEAP ) Point3Value( pmax );
                vl.meanVal = new( GC_IN_HEAP ) Point3Value( pmean );
            } else {
                vl.minVal = &undefined;
                vl.maxVal = &undefined;
                vl.meanVal = &undefined;
            }

            vl.result = new Array( 3 );
            vl.result->append( vl.minVal );
            vl.result->append( vl.maxVal );
            vl.result->append( vl.meanVal );
        }
    }

    return_value( vl.result );
}

BaseInterface* DebugInformation::GetInterface( Interface_ID id ) {
    if( id == DebugInformation_INTERFACE )
        return static_cast<IDebugInformation*>( this );
    if( BaseInterface* result = IDebugInformation::GetInterface( id ) )
        return result;
    return IObject::GetInterface( id );
}

int DebugInformation::NumInterfaces() const { return 1 + IObject::NumInterfaces(); }

BaseInterface* DebugInformation::GetInterfaceAt( int i ) const {
    return ( i == 0 ) ? (BaseInterface*)static_cast<const IDebugInformation*>( this )
                      : IObject::GetInterfaceAt( i - 1 );
}

} // namespace max3d
} // namespace magma
} // namespace frantic
