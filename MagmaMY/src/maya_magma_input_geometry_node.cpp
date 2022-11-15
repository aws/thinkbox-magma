// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "frantic/magma/maya/nodes/maya_magma_input_geometry_node.hpp"

#include <frantic/magma/nodes/magma_node_impl.hpp>
#include <frantic/maya/convert.hpp>
#include <frantic/maya/selection.hpp>

#include <maya/MDistance.h>
#include <maya/MFnTransform.h>
#include <maya/MTransformationMatrix.h>

#include <frantic/geometry/dcel_construction.hpp>

#include <boost/config.hpp>

namespace frantic {
namespace magma {
namespace maya {
namespace nodes {

namespace geom_property_visitor {
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
} // namespace geom_property_visitor

MAGMA_DEFINE_TYPE( "InputGeometry", "Input", maya_magma_input_geometry_node )
MAGMA_EXPOSE_PROPERTY( geometryNames, std::vector<frantic::tstring> )
MAGMA_OUTPUT_NAMES( "Geometry", "Object Count", "Objects" )
MAGMA_DESCRIPTION( "Exposes the geometry of a node for other operators." )
MAGMA_DEFINE_TYPE_END

std::size_t maya_magma_input_geometry_node::size() const { return m_meshData.size(); }

magma_geometry_ptr maya_magma_input_geometry_node::get_geometry( std::size_t index ) const { return m_meshData[index]; }

static inline void get_mesh_and_transform_matrix( MFnDependencyNode& depNode, MObject& outMesh, MObject& outTransform,
                                                  bool& outFoundMesh, bool& outFoundTransform ) {
    MStatus status;
    MObject depObject = depNode.object();

    if( depObject.hasFn( MFn::kTransform ) ) {
        outTransform = depNode.object(); // MFnTransform
        outFoundTransform = true;

        // TODO: NOT IMPLEMENTED YET
        // outMesh = depNode.attribute( "geometry", &status );
        MPlug depPlug = depNode.findPlug( "geometry", &status );
        outMesh = depPlug.elementByLogicalIndex( 1 ).asMObject();
        outFoundMesh = ( status == MStatus::kSuccess );
    } else if( depObject.hasFn( MFn::kMesh ) || depObject.hasFn( MFn::kMeshGeom ) ) {
        outMesh = depNode.object(); // MFnMesh
        outFoundMesh = true;

        MPlug depPlug = depNode.findPlug( "worldMatrix", &status );
        outTransform = depPlug.elementByLogicalIndex( 0 ).asMObject(); // MFnMatrixData
        outFoundTransform = ( status == MStatus::kSuccess );
    }
}

void maya_magma_input_geometry_node::compile_as_extension_type( frantic::magma::magma_compiler_interface& compiler ) {
    if( m_geometryNames.empty() )
        // No input geometry
        throw magma_exception()
            << magma_exception::node_id( get_id() ) << magma_exception::property_name( _T( "geometryNames" ) )
            << magma_exception::error_name(
                   _T("maya_magma_input_geometry_node::compile_as_extension_type error: No mesh given.") );

    // TODO: update this to make use of Libraries/FranticMayaLibrary/frantic/maya/geometry/mesh.hpp if needed

    if( m_meshData.size() == 0 ) {

        std::set<frantic::tstring> added;

        for( std::vector<frantic::tstring>::const_iterator iter = m_geometryNames.begin();
             iter != m_geometryNames.end(); ++iter ) {
            // Maya 2014:
            //   mesh object of type kMeshGeom, but contains kMesh
            //   transform object of type kMeshData, but contains kMatrixData
            // Maya all others:
            //   mesh object of type kMesh
            //   transform object of type kMatrixData

            // Ignore duplicates
            if( added.count( *iter ) > 0 ) {
                continue;
            }
            added.insert( *iter );

            // Get the object by name
            MString objectName( iter->c_str() );
            // GET_DEPENDENCY_NODE_FROM_MSTRING( depNode, objectName );
            MSelectionList currentGeo;
            currentGeo.add( objectName );

            if( currentGeo.length() != 1 )
                throw magma_exception()
                    << magma_exception::node_id( get_id() ) << magma_exception::property_name( _T( "geometryNames" ) )
                    << magma_exception::error_name(
                           _T("maya_magma_input_geometry_node::compile_as_extension_type error: Object \"") + *iter +
                           _T("\" not found.") );

            MObject foundGeoObject;
            currentGeo.getDependNode( 0, foundGeoObject );
            MFnDependencyNode depNode( foundGeoObject );

            MObject selectedObject, transform;
            bool foundMesh, foundTrans;
            get_mesh_and_transform_matrix( depNode, selectedObject, transform, foundMesh, foundTrans );

            m_objects.push_back( foundGeoObject );

            MStatus status;

            MFnMesh mesh( selectedObject, &status );
            if( status != MStatus::kSuccess ) {
                if( !foundMesh || !( selectedObject.hasFn( MFn::kMesh ) || selectedObject.hasFn( MFn::kMeshGeom ) ) )
                    throw magma_exception()
                        << magma_exception::node_id( get_id() )
                        << magma_exception::property_name( _T( "geometryNames" ) )
                        << magma_exception::error_name(
                               _T("maya_magma_input_geometry_node::compile_as_extension_type error: Object \"") +
                               *iter + _T(" does not contain a mesh.") );
                else
                    throw magma_exception()
                        << magma_exception::node_id( get_id() )
                        << magma_exception::property_name( _T( "geometryNames" ) )
                        << magma_exception::error_name( _T("maya_magma_input_geometry_node::compile_as_extension_type ")
                                                        _T("error: Unable to read mesh from \"") +
                                                        *iter + _T(".") );
            }

            MMatrix trans;
            if( foundTrans ) {
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
            }

            frantic::magma::magma_geometry_ptr meshptr = frantic::magma::magma_geometry_interface::create_instance(
                boost::shared_ptr<maya_magma_mesh>( new maya_magma_mesh( mesh ) ),
                frantic::maya::from_maya_t( trans ) );
            m_meshData.push_back( meshptr );
        }
    }

    if( m_objects.empty() ) {
        throw magma_exception()
            << magma_exception::node_id( get_id() ) << magma_exception::property_name( _T( "geometryNames" ) )
            << magma_exception::error_name(
                   _T("maya_magma_input_geometry_node::compile_as_extension_type error: No objects at source.") );
    }

    compiler.register_interface( get_id(),
                                 static_cast<frantic::magma::nodes::magma_input_geometry_interface*>( this ) );
    compiler.compile_constant( get_id(), (int)m_meshData.size() );
    compiler.register_interface( get_id(), static_cast<frantic::magma::nodes::magma_input_objects_interface*>( this ) );
}

bool maya_magma_input_geometry_node::get_property_internal( std::size_t index, const frantic::tstring& propName,
                                                            const std::type_info& typeInfo, void* outValue ) {
    MObject obj = m_objects[index];
    if( obj.isNull() ) {
        return false;
    }

    if( propName == _T( "worldMatrix" ) && typeid( frantic::graphics::transform4f ) == typeInfo ) {
        frantic::graphics::transform4f xform;
        bool ok = false;
        get_transform( index, xform, ok );
        if( ok ) {
            *reinterpret_cast<frantic::graphics::transform4f*>( outValue ) = xform;
            return true;
        } else {
            throw magma_exception() << magma_exception::node_id( get_id() )
                                    << magma_exception::property_name( _T( "geometryNames" ) )
                                    << magma_exception::error_name(
                                           _T("maya_magma_input_geometry_node::get_property_internal error: Could not ")
                                           _T("find transform matrix for object.") );
        }
    }

    variant_t result;
    this->get_property( index, propName, result );

    geom_property_visitor::get_property_visitor visitor( typeInfo, outValue );

    return boost::apply_visitor( visitor, result );
}

void maya_magma_input_geometry_node::get_property( std::size_t index, const frantic::tstring& propName,
                                                   variant_t& outValue ) {
    MStatus status;
    if( index >= m_objects.size() ) {
        return;
    }

    if( m_objects[index].isNull() ) {
        return;
    }

    MObject source = m_objects[index];
    MFnDependencyNode object( source, &status );
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

void maya_magma_input_geometry_node::get_transform( std::size_t index, frantic::graphics::transform4f& outTransform,
                                                    bool& foundOutTransform ) {
    if( index >= 0 && index < m_geometryNames.size() ) {
        MString objectName( frantic::strings::to_string( m_geometryNames[index] ).c_str() );
        MSelectionList currentGeo;
        currentGeo.add( objectName );

        if( currentGeo.length() != 1 ) {
            foundOutTransform = false;
            throw magma_exception()
                << magma_exception::node_id( get_id() ) << magma_exception::property_name( _T( "geometryNames" ) )
                << magma_exception::error_name(
                       _T("maya_magma_input_geometry_node::compile_as_extension_type error: Object \"") +
                       m_geometryNames[index] + _T("\" not found.") );
        }

        MObject foundGeoObject;
        currentGeo.getDependNode( 0, foundGeoObject );
        MFnDependencyNode depNode( foundGeoObject );

        MObject selectedObject, transform;
        bool foundMesh;
        get_mesh_and_transform_matrix( depNode, selectedObject, transform, foundMesh, foundOutTransform );

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

    } else {
        foundOutTransform = false;
        throw magma_exception() << magma_exception::node_id( get_id() )
                                << magma_exception::property_name( _T( "geometryNames" ) )
                                << magma_exception::error_name(
                                       _T("maya_magma_input_geometry_node::compile_as_extension_type error: Index \"") +
                                       boost::lexical_cast<frantic::tstring>( index ) + _T("\" out of range.") );
    }
}

} // namespace nodes
} // namespace maya
} // namespace magma
} // namespace frantic

namespace frantic {
namespace magma {
namespace maya {
namespace nodes {

#pragma region maya_magma_mesh
maya_magma_mesh::maya_magma_mesh( MFnMesh& mesh ) {
    m_EnableCache = true;
    m_mesh_object = mesh.object();

    // Set up the mapping between points/polygon IDs/vertex IDs/Triangles
    // This complex mapping is needed for the following reasons:
    //   Code elsewhere assumes all faces are triangles (see below TODO)
    //   Properties such as color and texture coordinates are not just vertex specific in Maya, but polygon specific
    //   Properties accessors only pass in one index value
    MIntArray triangleCount;
    MIntArray triangleVerts;
    mesh.getTriangles( triangleCount, triangleVerts );

    int ourVertexID = 0;
    for( unsigned int polygonID = 0; polygonID < triangleCount.length(); polygonID++ ) {

        MIntArray perPolygonVerts;
        mesh.getPolygonVertices( polygonID, perPolygonVerts );

        for( unsigned int triangleID = 0; triangleID < (unsigned int)triangleCount[polygonID]; triangleID++ ) {
            int triangleVerts[3];
            mesh.getPolygonTriangleVertices( polygonID, triangleID, triangleVerts );

            int triangleIDs[3] = { -1, -1, -1 };
            // Figure out which triangle vertex refers to which polygon vertex
            // Assuming there are no duplicates
            for( unsigned int perPolygonVertexID = 0; perPolygonVertexID < perPolygonVerts.length();
                 perPolygonVertexID++ ) {
                if( triangleVerts[0] == perPolygonVerts[perPolygonVertexID] )
                    triangleIDs[0] = perPolygonVertexID;
                if( triangleVerts[1] == perPolygonVerts[perPolygonVertexID] )
                    triangleIDs[1] = perPolygonVertexID;
                if( triangleVerts[2] == perPolygonVerts[perPolygonVertexID] )
                    triangleIDs[2] = perPolygonVertexID;
                if( triangleIDs[0] >= 0 && triangleIDs[1] >= 0 && triangleIDs[2] >= 0 )
                    break;
            }

            maya_magma_mesh::polygon_map_data data;
            m_FaceIDToPolygonData.push_back( data );
            m_FaceIDToPolygonData.back().polygonID = polygonID;
            m_FaceIDToPolygonData.back().triangleID = triangleID;
            m_FaceIDToPolygonData.back().triangleVertex[0] = triangleVerts[0];
            m_FaceIDToPolygonData.back().triangleVertex[1] = triangleVerts[1];
            m_FaceIDToPolygonData.back().triangleVertex[2] = triangleVerts[2];
            m_FaceIDToPolygonData.back().polygonVertex[0] = triangleIDs[0];
            m_FaceIDToPolygonData.back().polygonVertex[1] = triangleIDs[1];
            m_FaceIDToPolygonData.back().polygonVertex[2] = triangleIDs[2];

            m_FaceIDToPolygonData.back().ourVertexID[0] = ourVertexID + 0;
            m_FaceIDToPolygonData.back().ourVertexID[1] = ourVertexID + 1;
            m_FaceIDToPolygonData.back().ourVertexID[2] = ourVertexID + 2;

            m_IDToPolygonVertex.push_back( std::make_pair( polygonID, triangleIDs[0] ) );
            m_IDToVertex.push_back( triangleVerts[0] );
            m_IDToPolygonVertex.push_back( std::make_pair( polygonID, triangleIDs[1] ) );
            m_IDToVertex.push_back( triangleVerts[1] );
            m_IDToPolygonVertex.push_back( std::make_pair( polygonID, triangleIDs[2] ) );
            m_IDToVertex.push_back( triangleVerts[2] );

            ourVertexID += 3;
        }
    }

    initialize_default_channels();

    if( m_EnableCache ) {
        mesh.getPoints( m_points );
    }
}

bool maya_magma_mesh::is_valid() const { return true; }

void maya_magma_mesh::initialize_default_channels() {
    std::unique_ptr<maya_magma_mesh_vertex_position_accessor> pos(
        new maya_magma_mesh_vertex_position_accessor( this ) );
    this->append_vertex_channel( std::move( pos ) );
}

bool maya_magma_mesh::populate_channel( const frantic::tstring& channelName, bool vertexChannel ) {
    if( vertexChannel ) {
        if( this->get_vertex_channels().get_channel( channelName ) != NULL )
            return true;

        if( channelName == _T("Position") ) {
            std::unique_ptr<maya_magma_mesh_vertex_position_accessor> pos(
                new maya_magma_mesh_vertex_position_accessor( this ) );
            this->append_vertex_channel( std::move( pos ) );
            return true;

        } else if( channelName == _T("Normal") ) {
            std::unique_ptr<maya_magma_mesh_vertex_normal_accessor> norm(
                new maya_magma_mesh_vertex_normal_accessor( false, this ) );
            this->append_vertex_channel( std::move( norm ) );
            return true;

        } else if( channelName == _T("SmoothNormal") ) {
            std::unique_ptr<maya_magma_mesh_vertex_normal_accessor> norm(
                new maya_magma_mesh_vertex_normal_accessor( true, this ) );
            this->append_vertex_channel( std::move( norm ) );
            return true;

        } else if( channelName == _T("Color") ) {
            std::unique_ptr<maya_magma_mesh_vertex_color_accessor> col(
                new maya_magma_mesh_vertex_color_accessor( this ) );
            this->append_vertex_channel( std::move( col ) );
            return true;

        } else if( channelName == _T("TextureCoord") ) {
            std::unique_ptr<maya_magma_mesh_vertex_texture_accessor> txt(
                new maya_magma_mesh_vertex_texture_accessor( this ) );
            this->append_vertex_channel( std::move( txt ) );
            return true;

        } else {
            // Unknown name could refer to a Maya Color Set or Texture Set
            MFnMesh mesh( m_mesh_object );

            MString mayaChannel = frantic::maya::to_maya_t( channelName );
            MStringArray values;

            // Color Set
            values.clear();
            mesh.getColorSetNames( values );
            for( unsigned int i = 0; i < values.length(); i++ ) {
                if( mayaChannel == values[i] ) {
                    std::unique_ptr<maya_magma_mesh_vertex_color_accessor> custcol(
                        new maya_magma_mesh_vertex_color_accessor( channelName, this ) );
                    this->append_vertex_channel( std::move( custcol ) );
                    return true;
                }
            }

            // Texture Set
            values.clear();
            mesh.getUVSetNames( values );
            for( unsigned int i = 0; i < values.length(); i++ ) {
                if( mayaChannel == values[i] ) {
                    std::unique_ptr<maya_magma_mesh_vertex_texture_accessor> custcol(
                        new maya_magma_mesh_vertex_texture_accessor( channelName, this ) );
                    this->append_vertex_channel( std::move( custcol ) );
                    return true;
                }
            }
        }
    } else {
        if( this->get_face_channels().get_channel( channelName ) != NULL )
            return true;

        if( channelName == _T("FaceNormal") ) {
            std::unique_ptr<maya_magma_mesh_face_normal_accessor> accessor(
                new maya_magma_mesh_face_normal_accessor( this ) );
            this->append_face_channel( std::move( accessor ) );
            return true;
        }
    }
    return false;
}

bool maya_magma_mesh::request_channel( const frantic::tstring& channelName, bool vertexChannel, bool forOutput,
                                       bool throwOnError ) {
    const frantic::geometry::mesh_channel* ch;

    // Check if the channel is already there
    if( vertexChannel )
        ch = this->get_vertex_channels().get_channel( channelName );
    else
        ch = this->get_face_channels().get_channel( channelName );
    if( ch != NULL ) {
        if( !forOutput )
            return true;
        if( ch->is_writeable() )
            return true;
        if( throwOnError )
            throw std::runtime_error( "maya_magma_mesh::request_channel(): Failed to request channel: \"" +
                                      frantic::strings::to_string( channelName ) + "\" is readonly" );
        return false;
    }

    // If not, try to add it and check again
    if( populate_channel( channelName, vertexChannel ) ) {
        if( vertexChannel )
            ch = this->get_vertex_channels().get_channel( channelName );
        else
            ch = this->get_face_channels().get_channel( channelName );

        if( ch != NULL ) {
            if( !forOutput )
                return true;
            if( ch->is_writeable() )
                return true;
            if( throwOnError )
                throw std::runtime_error( "maya_magma_mesh::request_channel(): Failed to request channel: \"" +
                                          frantic::strings::to_string( channelName ) + "\" is readonly" );
            return false;
        }
    }

    if( throwOnError )
        throw std::runtime_error( "maya_magma_mesh::request_channel(): Failed to request channel: \"" +
                                  frantic::strings::to_string( channelName ) + "\" does not exist" );
    return false;
}

std::size_t maya_magma_mesh::get_num_verts() const {
    std::size_t count;
    if( m_EnableCache ) {
        count = m_points.length();
    } else {
        MFnMesh mesh( m_mesh_object );
        count = (std::size_t)mesh.numVertices();
    }
    return count;
}

void maya_magma_mesh::get_vert( std::size_t index, float ( &outValues )[3] ) const {
    MPoint position;
    if( m_EnableCache ) {
        position = m_points[(int)index];
    } else {
        MFnMesh mesh( m_mesh_object );
        mesh.getPoint( (int)index, position );
    }
    outValues[0] = (float)position.x;
    outValues[1] = (float)position.y;
    outValues[2] = (float)position.z;
}

std::size_t maya_magma_mesh::get_num_faces() const { return m_FaceIDToPolygonData.size(); }

std::size_t maya_magma_mesh::get_num_face_verts( std::size_t faceIndex ) const {
    // TODO: WARNING: MagmaProject\src\magma_geometry_interface.cpp IS ASSUMING TRIANGLES
    return 3;
}

std::size_t maya_magma_mesh::get_face_vert_index( std::size_t faceIndex, std::size_t fvertIndex ) const {
    const polygon_map_data& data = m_FaceIDToPolygonData[faceIndex];
    return data.triangleVertex[fvertIndex];
}

std::size_t maya_magma_mesh::get_face_vert_id( std::size_t faceIndex, std::size_t fvertIndex ) const {
    const polygon_map_data& data = m_FaceIDToPolygonData[faceIndex];
    return data.ourVertexID[fvertIndex];
}

bool maya_magma_mesh::get_maya_face_vert_from_id( std::size_t id, std::size_t& outPolygonIndex,
                                                  std::size_t& outFVertIndex ) const {
    if( id < m_IDToPolygonVertex.size() ) {
        const std::pair<int, int>& faceAndVert = m_IDToPolygonVertex[id];
        outPolygonIndex = faceAndVert.first;
        outFVertIndex = faceAndVert.second;
        return true;
    }
    outPolygonIndex = 0;
    outFVertIndex = 0;
    return false;
}

std::size_t maya_magma_mesh::get_maya_vertex_from_id( std::size_t id ) const { return m_IDToVertex[id]; }

std::size_t maya_magma_mesh::get_maya_polygon_from_face_id( std::size_t faceIndex ) const {
    const polygon_map_data& data = m_FaceIDToPolygonData[faceIndex];
    return data.polygonID;
}

void maya_magma_mesh::get_face_vert_indices( std::size_t faceIndex, std::size_t outValues[] ) const {
    for( std::size_t i = 0; i < get_num_face_verts( faceIndex ); i++ ) {
        outValues[i] = get_face_vert_index( faceIndex, i );
    }
}

void maya_magma_mesh::get_face_verts( std::size_t faceIndex, float outValues[][3] ) const {
    for( std::size_t i = 0; i < get_num_face_verts( faceIndex ); i++ ) {
        std::size_t j = get_face_vert_index( faceIndex, i );
        get_vert( j, outValues[i] );
    }
}

std::size_t maya_magma_mesh::get_num_elements() const { return 1; }

std::size_t maya_magma_mesh::get_face_element_index( std::size_t faceIndex ) const { return 0; }

MObject maya_magma_mesh::get_maya_mesh_object() const { return m_mesh_object; }

void maya_magma_mesh::init_adjacency() {
    if( !has_adjacency() ) {
#if( defined( __linux__ ) || defined( __APPLE__ ) ) && !defined( BOOST_NO_CXX11_HDR_SMART_PTR )
        // For the version of Boost we use, Boost.Move doesn't seem to work correctly in C++11
        frantic::geometry::dcel* adjacency = new frantic::geometry::dcel();
        frantic::geometry::mesh_interface_to_dcel( this, *adjacency );
        m_adjacencyDelegate.reset( new frantic::geometry::dcel_mesh_interface( adjacency, true ) );
#else
        frantic::geometry::dcel adjacency;
        frantic::geometry::mesh_interface_to_dcel( this, adjacency );
        m_adjacencyDelegate.reset( new frantic::geometry::dcel_mesh_interface( boost::move( adjacency ) ) );
#endif
    }
}

bool maya_magma_mesh::has_adjacency() const { return m_adjacencyDelegate.get() != NULL; }

bool maya_magma_mesh::init_vertex_iterator( frantic::geometry::vertex_iterator& vIt, std::size_t vertexIndex ) const {
    assert( has_adjacency() && "init_adjacency() must be called before maya_magma_mesh::init_vertex_iterator()" );
    return m_adjacencyDelegate->init_vertex_iterator( vIt, vertexIndex );
}

bool maya_magma_mesh::advance_vertex_iterator( frantic::geometry::vertex_iterator& vIt ) const {
    assert( has_adjacency() );
    return m_adjacencyDelegate->advance_vertex_iterator( vIt );
}

std::size_t maya_magma_mesh::get_edge_endpoint( frantic::geometry::vertex_iterator& vIt ) const {
    assert( has_adjacency() );
    return m_adjacencyDelegate->get_edge_endpoint( vIt );
}

std::size_t maya_magma_mesh::get_edge_left_face( frantic::geometry::vertex_iterator& vIt ) const {
    assert( has_adjacency() );
    return m_adjacencyDelegate->get_edge_left_face( vIt );
}

std::size_t maya_magma_mesh::get_edge_right_face( frantic::geometry::vertex_iterator& vIt ) const {
    assert( has_adjacency() );
    return m_adjacencyDelegate->get_edge_right_face( vIt );
}

bool maya_magma_mesh::is_edge_visible( frantic::geometry::vertex_iterator& vIt ) const {
    assert( has_adjacency() );
    return m_adjacencyDelegate->is_edge_visible( vIt );
}

bool maya_magma_mesh::is_edge_boundary( frantic::geometry::vertex_iterator& vIt ) const {
    assert( has_adjacency() );
    return m_adjacencyDelegate->is_edge_boundary( vIt );
}

void maya_magma_mesh::init_face_iterator( frantic::geometry::face_iterator& fIt, std::size_t faceIndex ) const {
    assert( has_adjacency() && "init_adjacency() must be called before maya_magma_mesh::init_face_iterator()" );
    m_adjacencyDelegate->init_face_iterator( fIt, faceIndex );
}

bool maya_magma_mesh::advance_face_iterator( frantic::geometry::face_iterator& fIt ) const {
    assert( has_adjacency() );
    return m_adjacencyDelegate->advance_face_iterator( fIt );
}

std::size_t maya_magma_mesh::get_face_neighbor( frantic::geometry::face_iterator& fIt ) const {
    assert( has_adjacency() );
    return m_adjacencyDelegate->get_face_neighbor( fIt );
}

std::size_t maya_magma_mesh::get_face_next_vertex( frantic::geometry::face_iterator& fIt ) const {
    assert( has_adjacency() );
    return m_adjacencyDelegate->get_face_next_vertex( fIt );
}

std::size_t maya_magma_mesh::get_face_prev_vertex( frantic::geometry::face_iterator& fIt ) const {
    assert( has_adjacency() );
    return m_adjacencyDelegate->get_face_prev_vertex( fIt );
}

#pragma endregion

#pragma region Geometry Accessors

void maya_magma_mesh_vertex_position_accessor::get_value( std::size_t index, void* outValue ) const {
    std::size_t mayaVertIndex = m_mesh->get_maya_vertex_from_id( index );

    float positions[3];
    m_mesh->get_vert( mayaVertIndex, positions );
    float* outVal = reinterpret_cast<float*>( outValue );
    outVal[0] = positions[0];
    outVal[1] = positions[1];
    outVal[2] = positions[2];
}

void maya_magma_mesh_vertex_normal_accessor::get_value( std::size_t index, void* outValue ) const {
    MObject meshObj = m_mesh->get_maya_mesh_object();
    std::size_t mayaVertIndex = m_mesh->get_maya_vertex_from_id( index );
    MFnMesh mesh( meshObj );

    MVector norm;
    mesh.getVertexNormal( (int)mayaVertIndex, true, norm );
    float* outVal = reinterpret_cast<float*>( outValue );
    outVal[0] = (float)norm.x;
    outVal[1] = (float)norm.y;
    outVal[2] = (float)norm.z;
}

void maya_magma_mesh_vertex_color_accessor::get_value( std::size_t index, void* outValue ) const {
    MObject meshObj = m_mesh->get_maya_mesh_object();
    std::size_t face, faceVert;
    m_mesh->get_maya_face_vert_from_id( index, face, faceVert );
    MFnMesh mesh( meshObj );

    int colorid;
    if( m_set_name.length() > 0 ) {
        MString mayaSetName = frantic::maya::to_maya_t( m_set_name );
        mesh.getColorIndex( (int)face, (int)faceVert, colorid, &mayaSetName );
    } else {
        mesh.getColorIndex( (int)face, (int)faceVert, colorid );
    }
    MColor color;
    mesh.getColor( colorid, color );

    float* outVal = reinterpret_cast<float*>( outValue );
    outVal[0] = color.r;
    outVal[1] = color.g;
    outVal[2] = color.b;
    // outVal[3] = color.a;
}

void maya_magma_mesh_vertex_texture_accessor::get_value( std::size_t index, void* outValue ) const {
    MObject meshObj = m_mesh->get_maya_mesh_object();
    std::size_t face, faceVert;
    m_mesh->get_maya_face_vert_from_id( index, face, faceVert );
    MFnMesh mesh( meshObj );

    float u, v;
    if( m_set_name.length() > 0 ) {
        MString mayaSetName = frantic::maya::to_maya_t( m_set_name );
        mesh.getPolygonUV( (int)face, (int)faceVert, u, v, &mayaSetName );
    } else {
        mesh.getPolygonUV( (int)face, (int)faceVert, u, v );
    }

    float* outVal = reinterpret_cast<float*>( outValue );
    outVal[0] = u;
    outVal[1] = v;
    outVal[2] = 0;
}

void maya_magma_mesh_face_normal_accessor::get_value( std::size_t index, void* outValue ) const {
    MObject meshObj = m_mesh->get_maya_mesh_object();
    std::size_t mayaPolygon = m_mesh->get_maya_polygon_from_face_id( index );
    MFnMesh mesh( meshObj );

    MVector norm;
    mesh.getPolygonNormal( (int)mayaPolygon, norm );
    float* outVal = reinterpret_cast<float*>( outValue );
    outVal[0] = (float)norm.x;
    outVal[1] = (float)norm.y;
    outVal[2] = (float)norm.z;
}

#pragma endregion

} // namespace nodes
} // namespace maya
} // namespace magma
} // namespace frantic
