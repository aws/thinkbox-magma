// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "frantic/magma/maya/nodes/maya_magma_input_object_node.hpp"

#include <frantic/maya/convert.hpp>

#include <boost/noncopyable.hpp>

#include <frantic/magma/nodes/magma_node_impl.hpp>

#include <maya/MDistance.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTransform.h>
#include <maya/MFnUnitAttribute.h>

namespace frantic {
namespace magma {
namespace maya {
namespace nodes {

namespace {
class get_property_visitor : public boost::static_visitor<bool>, boost::noncopyable {
    void* m_outValue;
    const std::type_info& m_outType;

  public:
    get_property_visitor( const std::type_info& outType, void* outValue )
        : m_outType( outType )
        , m_outValue( outValue ) {}

    bool operator()( boost::blank ) { return false; }

    template <class T>
    bool operator()( T val ) {
        if( m_outType != typeid( T ) )
            return false;

        *reinterpret_cast<T*>( m_outValue ) = val;

        return true;
    }
};
} // namespace

MAGMA_DEFINE_TYPE( "InputObject", "Input", maya_magma_input_object_node )
MAGMA_EXPOSE_PROPERTY( objectName, frantic::tstring )
MAGMA_OUTPUT_NAMES( "Object" )
MAGMA_DESCRIPTION( "Exposes the properties of an object for other operators." )
MAGMA_DEFINE_TYPE_END

bool maya_magma_input_object_node::get_property_internal( std::size_t /*index*/, const frantic::tstring& propName,
                                                          const std::type_info& typeInfo, void* outValue ) {

    if( propName == _T( "worldMatrix" ) && typeid( frantic::graphics::transform4f ) == typeInfo ) {
        frantic::graphics::transform4f xform;
        bool ok = false;
        get_transform( 0, xform, ok );
        if( ok ) {
            *reinterpret_cast<frantic::graphics::transform4f*>( outValue ) = xform;
            return true;
        } else {
            throw magma_exception() << magma_exception::node_id( get_id() )
                                    << magma_exception::property_name( _T( "geometryNames" ) )
                                    << magma_exception::error_name(
                                           _T("maya_magma_input_object_node::get_property_internal error: Could not ")
                                           _T("find transform matrix for object.") );
        }
    }

    variant_t result;
    this->get_property( 0, propName, result );

    get_property_visitor visitor( typeInfo, outValue );

    return boost::apply_visitor( visitor, result );
}

void maya_magma_input_object_node::get_property( std::size_t /*index*/, const frantic::tstring& propName,
                                                 variant_t& outValue ) {
    MStatus status;
    MFnDependencyNode object( m_sourceNode, &status );
    if( status != MStatus::kSuccess ) {
        return;
    }

    MPlug prop = object.findPlug( frantic::maya::to_maya_t( propName ), &status );
    if( status != MStatus::kSuccess ) {
        throw magma_exception() << magma_exception::node_id( get_id() )
                                << magma_exception::property_name( _T( "objectName" ) )
                                << magma_exception::error_name(
                                       _T("maya_magma_input_object_node::get_property error: Attribute \"") + propName +
                                       _T("\" not found.") );
    }

    bool ok = false;
    MObject propType = prop.attribute();
    if( propType.hasFn( MFn::kNumericAttribute ) ) {
        MFnNumericAttribute propSubType( propType );

        // Boolean
        if( propSubType.unitType() == MFnNumericData::kBoolean ) {
            bool value;
            status = prop.getValue( value );
            if( status == MS::kSuccess ) {
                outValue = value;
                ok = true;
            }

            // Int
        } else if( propSubType.unitType() == MFnNumericData::kByte ) {
            int value;
            status = prop.getValue( value );
            if( status == MS::kSuccess ) {
                outValue = value;
                ok = true;
            }
        } else if( propSubType.unitType() == MFnNumericData::kInt || propSubType.unitType() == MFnNumericData::kLong ) {
            int value;
            status = prop.getValue( value );
            if( status == MS::kSuccess ) {
                outValue = value;
                ok = true;
            }
        } else if( propSubType.unitType() == MFnNumericData::kShort ) {
            short value;
            status = prop.getValue( value );
            if( status == MS::kSuccess ) {
                outValue = (int)value;
                ok = true;
            }
        } else if( propSubType.unitType() == MFnNumericData::kChar ) {
            char value;
            status = prop.getValue( value );
            if( status == MS::kSuccess ) {
                outValue = (int)value;
                ok = true;
            }

            // Float
        } else if( propSubType.unitType() == MFnNumericData::kFloat ) {
            float value;
            status = prop.getValue( value );
            if( status == MS::kSuccess ) {
                outValue = value;
                ok = true;
            }
        } else if( propSubType.unitType() == MFnNumericData::kDouble ) {
            float value;
            status = prop.getValue( value );
            if( status == MS::kSuccess ) {
                outValue = (float)value;
                ok = true;
            }

            // Vec3
        } else if( propSubType.unitType() == MFnNumericData::k3Float ) {
            MStatus s1, s2, s3;
            float x, y, z;
            s1 = prop.child( 0 ).getValue( x );
            s2 = prop.child( 1 ).getValue( y );
            s3 = prop.child( 2 ).getValue( z );

            if( s1 == MS::kSuccess && s2 == MS::kSuccess && s3 == MS::kSuccess ) {
                outValue = frantic::magma::vec3( x, y, z );
                ok = true;
            }
        } else if( propSubType.unitType() == MFnNumericData::k3Double ) {
            MStatus s1, s2, s3;
            double x, y, z;
            s1 = prop.child( 0 ).getValue( x );
            s2 = prop.child( 1 ).getValue( y );
            s3 = prop.child( 2 ).getValue( z );

            if( s1 == MS::kSuccess && s2 == MS::kSuccess && s3 == MS::kSuccess ) {
                outValue = frantic::magma::vec3( (float)x, (float)y, (float)z );
                ok = true;
            }
        } else if( propSubType.unitType() == MFnNumericData::k3Int ||
                   propSubType.unitType() == MFnNumericData::k3Long ) {
            MStatus s1, s2, s3;
            int x, y, z;
            s1 = prop.child( 0 ).getValue( x );
            s2 = prop.child( 1 ).getValue( y );
            s3 = prop.child( 2 ).getValue( z );

            if( s1 == MS::kSuccess && s2 == MS::kSuccess && s3 == MS::kSuccess ) {
                outValue = frantic::magma::vec3( (float)x, (float)y, (float)z );
                ok = true;
            }
        } else if( propSubType.unitType() == MFnNumericData::k3Short ) {
            MStatus s1, s2, s3;
            short x, y, z;
            s1 = prop.child( 0 ).getValue( x );
            s2 = prop.child( 1 ).getValue( y );
            s3 = prop.child( 2 ).getValue( z );

            if( s1 == MS::kSuccess && s2 == MS::kSuccess && s3 == MS::kSuccess ) {
                outValue = frantic::magma::vec3( (float)x, (float)y, (float)z );
                ok = true;
            }
        } else if( propSubType.unitType() == MFnNumericData::k2Float ) {
            MStatus s1, s2;
            float x, y;
            s1 = prop.child( 0 ).getValue( x );
            s2 = prop.child( 1 ).getValue( y );

            if( s1 == MS::kSuccess && s2 == MS::kSuccess ) {
                outValue = frantic::magma::vec3( x, y, 0 );
                ok = true;
            }
        } else if( propSubType.unitType() == MFnNumericData::k2Double ) {
            MStatus s1, s2;
            double x, y;
            s1 = prop.child( 0 ).getValue( x );
            s2 = prop.child( 1 ).getValue( y );

            if( s1 == MS::kSuccess && s2 == MS::kSuccess ) {
                outValue = frantic::magma::vec3( (float)x, (float)y, 0 );
                ok = true;
            }
        } else if( propSubType.unitType() == MFnNumericData::k2Int ||
                   propSubType.unitType() == MFnNumericData::k2Long ) {
            MStatus s1, s2;
            int x, y;
            s1 = prop.child( 0 ).getValue( x );
            s2 = prop.child( 1 ).getValue( y );

            if( s1 == MS::kSuccess && s2 == MS::kSuccess ) {
                outValue = frantic::magma::vec3( (float)x, (float)y, 0 );
                ok = true;
            }
        } else if( propSubType.unitType() == MFnNumericData::k2Short ) {
            MStatus s1, s2;
            short x, y;
            s1 = prop.child( 0 ).getValue( x );
            s2 = prop.child( 1 ).getValue( y );

            if( s1 == MS::kSuccess && s2 == MS::kSuccess ) {
                outValue = frantic::magma::vec3( (float)x, (float)y, 0 );
                ok = true;
            }

            // Quat
        } else if( propSubType.unitType() == MFnNumericData::k4Double ) {
            // TODO: does this make sense?
            MStatus s1, s2, s3, s4;
            double w, x, y, z;
            s1 = prop.child( 0 ).getValue( w );
            s2 = prop.child( 1 ).getValue( x );
            s3 = prop.child( 2 ).getValue( y );
            s4 = prop.child( 3 ).getValue( z );

            if( s1 == MS::kSuccess && s2 == MS::kSuccess && s3 == MS::kSuccess && s4 == MS::kSuccess ) {
                outValue = frantic::magma::quat( (float)w, (float)x, (float)y, (float)z );
                ok = true;
            }
        }
    } else if( propType.hasFn( MFn::kUnitAttribute ) ) {
        MFnUnitAttribute propSubType( propType );

        if( propSubType.unitType() == MFnUnitAttribute::kDistance ) {
            MDistance value;
            status = prop.getValue( value );
            if( status == MS::kSuccess ) {
                outValue = (float)value.asCentimeters();
                ok = true;
            }
        } else if( propSubType.unitType() == MFnUnitAttribute::kAngle ) {
            MAngle value;
            status = prop.getValue( value );
            if( status == MS::kSuccess ) {
                outValue = (float)value.asRadians();
                ok = true;
            }
        } else if( propSubType.unitType() == MFnUnitAttribute::kTime ) {
            MTime value;
            status = prop.getValue( value );
            if( status == MS::kSuccess ) {
                outValue = (float)value.as( MTime::kSeconds );
                ok = true;
            }
        }
    }

    if( !ok ) {
        throw magma_exception() << magma_exception::node_id( get_id() )
                                << magma_exception::property_name( _T( "objectName" ) )
                                << magma_exception::error_name(
                                       _T("maya_magma_input_object_node::get_property error: Attribute \"") + propName +
                                       _T("\" is neither a bool, int, float, vec3, or quat.") );
    }
}

void maya_magma_input_object_node::compile_as_extension_type( frantic::magma::magma_compiler_interface& compiler ) {

    m_sourceNode = MObject::kNullObj;

    MStatus status;
    MString objectName( frantic::strings::to_string( m_objectName ).c_str() );

    // GET_DEPENDENCY_NODE_FROM_MSTRING( depNode, objectName );
    MSelectionList currentPar;
    currentPar.add( objectName );
    if( currentPar.length() != 1 )
        throw magma_exception() << magma_exception::node_id( get_id() )
                                << magma_exception::property_name( _T( "objectName" ) )
                                << magma_exception::error_name(
                                       _T("maya_magma_input_object_node::compile_as_extension_type error: Object \"") +
                                       m_objectName + _T("\" not found.") );
    MObject foundParObject;
    currentPar.getDependNode( 0, foundParObject );

    MFnDependencyNode object( foundParObject, &status );
    if( status != MStatus::kSuccess )
        throw magma_exception()
            << magma_exception::node_id( get_id() ) << magma_exception::property_name( _T( "objectName" ) )
            << magma_exception::error_name(
                   _T("maya_magma_input_object_node::compile_as_extension_type error: Unable to read node \"") +
                   m_objectName + _T(".") );

    m_sourceNode = foundParObject;
    compiler.register_interface( get_id(), static_cast<frantic::magma::nodes::magma_input_objects_interface*>( this ) );
}

void maya_magma_input_object_node::get_transform( std::size_t index, frantic::graphics::transform4f& outTransform,
                                                  bool& foundOutTransform ) {

    MFnDependencyNode depNode( m_sourceNode );
    MObject depObject = depNode.object();

    MObject transform;
    // bool foundMesh;
    foundOutTransform = false;

    if( depObject.hasFn( MFn::kTransform ) ) {
        transform = depNode.object(); // MFnTransform
        foundOutTransform = true;
    }

    MStatus status;
    MMatrix trans;
    if( foundOutTransform ) {
        if( transform.hasFn( MFn::kTransform ) ) {
            MFnTransform mtrans( transform, &status );
            MTransformationMatrix mmtrans = mtrans.transformation( &status );
            trans = mmtrans.asMatrix();
        } else if( transform.hasFn( MFn::kMatrixData ) ) {
            MFnMatrixData mtrans( transform, &status );
            trans = mtrans.matrix();
        } else { // transform.hasFn( MFn::kMeshData )
            MFnMatrixData mtrans( transform, &status );
            if( status == MStatus::kSuccess )
                trans = mtrans.matrix();
        }
        outTransform = frantic::maya::from_maya_t( trans );
    }
}

} // namespace nodes
} // namespace maya
} // namespace magma
} // namespace frantic
