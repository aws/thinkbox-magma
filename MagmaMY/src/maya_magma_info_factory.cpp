// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "frantic/convert/tstring.hpp"
#include "frantic/magma/maya/maya_magma_exception.hpp"
#include "frantic/magma/maya/maya_magma_info.hpp"
#include "frantic/magma/maya/maya_magma_info_factory.hpp"
#include "frantic/magma/maya/maya_magma_singleton.hpp"

#include <frantic/logging/logging_level.hpp>

#include <memory>
#include <utility>

namespace frantic {
namespace magma {
namespace maya {
namespace factory {

namespace detail {

class maya_magma_input_socket_info_manager {
  public:
    struct meta {
        std::vector<maya_magma_input_socket_data_type_t> m_inputSocketInfos;

        void push_back( maya_magma_input_socket_data_type_t data ) { m_inputSocketInfos.push_back( data ); }

        std::size_t get_num_input_sockets() const { return m_inputSocketInfos.size(); }
    };

  private:
    std::map<frantic::tstring, meta> m_metaData;

  public:
    static const maya_magma_input_socket_info_manager& get_instance();

    const meta* get_meta( const frantic::tstring& dataType ) const;

  private:
    maya_magma_input_socket_info_manager() { initialize(); }

    ~maya_magma_input_socket_info_manager() {}

    void initialize();
};

const maya_magma_input_socket_info_manager& maya_magma_input_socket_info_manager::get_instance() {
    static maya_magma_input_socket_info_manager theMayaMagmaInputSocketInfoManager;
    return theMayaMagmaInputSocketInfoManager;
}

const maya_magma_input_socket_info_manager::meta*
maya_magma_input_socket_info_manager::get_meta( const frantic::tstring& dataType ) const {
    std::map<frantic::tstring, maya_magma_input_socket_info_manager::meta>::const_iterator it =
        m_metaData.find( dataType );
    if( it == m_metaData.end() ) {
        FF_LOG( debug ) << "maya_magma_input_socket_info_manager::get_meta the meta information for " << dataType
                        << " is not found " << std::endl;
        return NULL;
    }
    return &( it->second );
}

void maya_magma_input_socket_info_manager::initialize() {
    // at the moment, the initialization only initialize the meta info for standard operator nodes;
    // if we want to add some new nodes to maya magma, we have to add their input socket meta informations to here as
    // well usually, the header file tells you what kinds of data type node's input socket supports and, its source file
    // tells you whether a magma node accepts a default input or not  (see macro MAGMA_INPUT) for example:
    // magma_standard_operators.cpp tells you whether a magma node accepts a default input or not, MAGMA_INPUT
    // magma_standard_operators.hpp tells you what kinds of data type that input socket supports

    // MAGMA_DECLARE_SIMPLE_OP( add, 2, BINDINGS(float(float,float), int(int,int), vec3(vec3,vec3)) );
    m_metaData[_T( "Add" )] = meta();
    m_metaData[_T( "Add" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT | MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT |
                                       MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "Add" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT | MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT |
                                       MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );

    // MAGMA_DECLARE_SIMPLE_OP( sub, 2, BINDINGS(float(float,float), int(int,int), vec3(vec3,vec3)) );
    m_metaData[_T( "Subtract" )] = meta();
    m_metaData[_T( "Subtract" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                            MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT |
                                            MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "Subtract" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                            MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT |
                                            MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );

    // MAGMA_DECLARE_SIMPLE_OP( mul, 2, BINDINGS(float(float,float), int(int,int), vec3(float,vec3), vec3(vec3,float),
    // vec3(vec3,vec3)) );
    m_metaData[_T( "Multiply" )] = meta();
    m_metaData[_T( "Multiply" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                            MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT |
                                            MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "Multiply" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                            MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT |
                                            MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );

    // MAGMA_DECLARE_SIMPLE_OP( div, 2, BINDINGS(float(float,float), int(int,int), vec3(vec3,float), vec3(vec3,vec3)) );
    m_metaData[_T( "Divide" )] = meta();
    m_metaData[_T( "Divide" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                          MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT |
                                          MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "Divide" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                          MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT |
                                          MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );

    // MAGMA_DECLARE_SIMPLE_OP( mod, 2, BINDINGS(float(float,float), int(int,int), vec3(vec3,float), vec3(vec3,vec3)) );
    m_metaData[_T( "Modulo" )] = meta();
    m_metaData[_T( "Modulo" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                          MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT |
                                          MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "Modulo" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                          MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT |
                                          MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );

    // MAGMA_DECLARE_SIMPLE_OP( pow, 2, BINDINGS(float(float,float), float(float,int), int(int,int), vec3(vec3,float))
    // );
    m_metaData[_T( "Power" )] = meta();
    m_metaData[_T( "Power" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                         MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT |
                                         MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "Power" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                         MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );

    // MAGMA_DECLARE_SIMPLE_OP( less, 2, BINDINGS(bool(float,float), bool(int,int)) );
    m_metaData[_T( "Less" )] = meta();
    m_metaData[_T( "Less" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                        MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );
    m_metaData[_T( "Less" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                        MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );

    // MAGMA_DECLARE_SIMPLE_OP( lesseq, 2, BINDINGS(bool(float,float), bool(int,int)) );
    m_metaData[_T( "LessOrEqual" )] = meta();
    m_metaData[_T( "LessOrEqual" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                               MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );
    m_metaData[_T( "LessOrEqual" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                               MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );

    // MAGMA_DECLARE_SIMPLE_OP( greater, 2, BINDINGS(bool(float,float), bool(int,int)) );
    m_metaData[_T( "Greater" )] = meta();
    m_metaData[_T( "Greater" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                           MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );
    m_metaData[_T( "Greater" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                           MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );

    // MAGMA_DECLARE_SIMPLE_OP( greatereq, 2, BINDINGS(bool(float,float), bool(int,int)) );
    m_metaData[_T( "GreaterOrEqual" )] = meta();
    m_metaData[_T( "GreaterOrEqual" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                                  MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );
    m_metaData[_T( "GreaterOrEqual" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                                  MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );

    // MAGMA_DECLARE_SIMPLE_OP( equal, 2, BINDINGS(bool(float,float), bool(int,int), bool(bool,bool), bool(vec3,vec3),
    // bool(quat,quat)) );
    m_metaData[_T( "Equal" )] = meta();
    m_metaData[_T( "Equal" )].push_back(
        MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT | MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT |
        MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 | MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_QUAT );
    m_metaData[_T( "Equal" )].push_back(
        MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT | MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT |
        MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 | MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_QUAT );

    // MAGMA_DECLARE_SIMPLE_OP( notequal, 2, BINDINGS(bool(float,float), bool(int,int), bool(bool,bool),
    // bool(vec3,vec3)/*, bool(quat,quat)*/) );
    m_metaData[_T( "NotEqual" )] = meta();
    m_metaData[_T( "NotEqual" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                            MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT |
                                            MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "NotEqual" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                            MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT |
                                            MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );

    // MAGMA_DECLARE_SIMPLE_OP( quat_mul, 2, BINDINGS(vec3(quat,vec3), quat(quat,quat)) );
    m_metaData[_T( "TransformByQuat" )] = meta();
    m_metaData[_T( "TransformByQuat" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_QUAT );
    m_metaData[_T( "TransformByQuat" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 |
                                                   MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_QUAT );

    m_metaData[_T( "FromSpace" )] = meta();
    m_metaData[_T( "FromSpace" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "FromSpace" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_NONE );
    m_metaData[_T( "FromSpace" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );

    m_metaData[_T( "ToSpace" )] = meta();
    m_metaData[_T( "ToSpace" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "ToSpace" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_NONE );
    m_metaData[_T( "ToSpace" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );

    // MAGMA_DECLARE_SIMPLE_OP( blend, 3, BINDINGS(float(float,float,float), vec3(vec3,vec3,float)) );
    m_metaData[_T( "Blend" )] = meta();
    m_metaData[_T( "Blend" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                         MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "Blend" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                         MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "Blend" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );

    // MAGMA_DECLARE_SIMPLE_OP( clamp, 3, BINDINGS(float(float,float,float), int(int,int,int), vec3(vec3,float,float))
    // );
    m_metaData[_T( "Clamp" )] = meta();
    m_metaData[_T( "Clamp" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                         MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT |
                                         MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "Clamp" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                         MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );
    m_metaData[_T( "Clamp" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                         MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );

    // MAGMA_DECLARE_SIMPLE_OP( logicswitch, 3, BINDINGS(
    //	float(float,float,bool), int(int,int,bool), bool(bool,bool,bool), vec3(vec3,vec3,bool), quat(quat,quat,bool),
    //	float(float,float,int), int(int,int,int), bool(bool,bool,int), vec3(vec3,vec3,int), quat(quat,quat,int)
    //) );
    m_metaData[_T( "Switch" )] = meta();
    m_metaData[_T( "Switch" )].push_back(
        MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_BOOL | MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
        MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT | MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 |
        MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_QUAT );
    m_metaData[_T( "Switch" )].push_back(
        MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_BOOL | MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
        MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT | MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 |
        MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_QUAT );
    m_metaData[_T( "Switch" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_BOOL |
                                          MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );

    // MAGMA_DECLARE_SIMPLE_OP( to_vector, 3, BINDINGS(vec3(float,float,float)) );
    m_metaData[_T( "ToVector" )] = meta();
    m_metaData[_T( "ToVector" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );
    m_metaData[_T( "ToVector" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );
    m_metaData[_T( "ToVector" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );

    // MAGMA_DECLARE_SIMPLE_OP( matrix_mul, 4, BINDINGS(vec3(vec3,vec3,vec3,vec3)) );
    m_metaData[_T( "MatrixMulVec" )] = meta();
    m_metaData[_T( "MatrixMulVec" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "MatrixMulVec" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "MatrixMulVec" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "MatrixMulVec" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );

    // MAGMA_DECLARE_SIMPLE_OP( vectors_to_quat, 3, BINDINGS(quat(vec3,vec3,vec3)) );
    m_metaData[_T( "VectorsToQuat" )] = meta();
    m_metaData[_T( "VectorsToQuat" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "VectorsToQuat" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "VectorsToQuat" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );

    // MAGMA_DECLARE_SIMPLE_OP( eulerangles_to_quat, 3, BINDINGS(quat(float,float,float)) );
    m_metaData[_T( "EulerAnglesToQuat" )] = meta();
    m_metaData[_T( "EulerAnglesToQuat" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );
    m_metaData[_T( "EulerAnglesToQuat" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );
    m_metaData[_T( "EulerAnglesToQuat" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );

    // MAGMA_DECLARE_SIMPLE_OP( angleaxis_to_quat, 2, BINDINGS(quat(float,vec3)) );
    m_metaData[_T( "AngleAxisToQuat" )] = meta();
    m_metaData[_T( "AngleAxisToQuat" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );
    m_metaData[_T( "AngleAxisToQuat" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );

    // struct meta{ static const int ARITY = 1; typedef frantic::magma::functors::quat_to_vectors type; typedef
    // BINDINGS(void(void*,quat)) bindings; };
    m_metaData[_T( "QuatToVectors" )] = meta();
    m_metaData[_T( "QuatToVectors" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_QUAT );

    // MAGMA_DECLARE_SIMPLE_OP( magma_nearest_particles_avg_node, 4, ... );
    m_metaData[_T( "ParticleSumCount" )] = meta();
    m_metaData[_T( "ParticleSumCount" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_NONE );
    m_metaData[_T( "ParticleSumCount" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_NONE );
    m_metaData[_T( "ParticleSumCount" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );
    m_metaData[_T( "ParticleSumCount" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );

    // MAGMA_DECLARE_SIMPLE_OP( magma_particle_kernel_node, 4, ... );
    m_metaData[_T( "ParticleSumRadius" )] = meta();
    m_metaData[_T( "ParticleSumRadius" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_NONE );
    m_metaData[_T( "ParticleSumRadius" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_NONE );
    m_metaData[_T( "ParticleSumRadius" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );
    m_metaData[_T( "ParticleSumRadius" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );

    // MAGMA_DECLARE_SIMPLE_OP( magma_face_query_node, 4, ... );
    m_metaData[_T( "FaceQuery" )] = meta();
    m_metaData[_T( "FaceQuery" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_NONE );
    m_metaData[_T( "FaceQuery" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );
    m_metaData[_T( "FaceQuery" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );
    m_metaData[_T( "FaceQuery" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );

    // MAGMA_DECLARE_SIMPLE_OP( magma_vertex_query_node, 3, ... );
    m_metaData[_T( "VertexQuery" )] = meta();
    m_metaData[_T( "VertexQuery" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_NONE );
    m_metaData[_T( "VertexQuery" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );
    m_metaData[_T( "VertexQuery" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );

    // MAGMA_DECLARE_SIMPLE_OP( magma_element_query_node, 3, ... );
    m_metaData[_T( "ElementQuery" )] = meta();
    m_metaData[_T( "ElementQuery" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_NONE );
    m_metaData[_T( "ElementQuery" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );
    m_metaData[_T( "ElementQuery" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );

    // MAGMA_DECLARE_SIMPLE_OP( magma_mesh_query_node, 2, ... );
    m_metaData[_T( "MeshQuery" )] = meta();
    m_metaData[_T( "MeshQuery" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_NONE );
    m_metaData[_T( "MeshQuery" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );

    // TODO: This will break old files
    m_metaData[_T( "PropertyQuery" )] = meta();
    m_metaData[_T( "PropertyQuery" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_NONE );
    m_metaData[_T( "PropertyQuery" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );

    m_metaData[_T( "IntersectRay" )] = meta();
    m_metaData[_T( "IntersectRay" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_NONE );
    m_metaData[_T( "IntersectRay" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "IntersectRay" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "IntersectRay" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_BOOL );

    m_metaData[_T( "NearestPoint" )] = meta();
    m_metaData[_T( "NearestPoint" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_NONE );
    m_metaData[_T( "NearestPoint" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "NearestPoint" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_BOOL );

    m_metaData[_T( "UniformRandom" )] = meta();
    m_metaData[_T( "UniformRandom" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );
    m_metaData[_T( "UniformRandom" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );
    m_metaData[_T( "UniformRandom" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );

    m_metaData[_T( "VecUniformRandom" )] = meta();
    m_metaData[_T( "VecUniformRandom" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );
    m_metaData[_T( "VecUniformRandom" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                                    MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "VecUniformRandom" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );

    m_metaData[_T( "ExponentialRandom" )] = meta();
    m_metaData[_T( "ExponentialRandom" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );
    m_metaData[_T( "ExponentialRandom" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );

    m_metaData[_T( "WeibullRandom" )] = meta();
    m_metaData[_T( "WeibullRandom" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );
    m_metaData[_T( "WeibullRandom" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );
    m_metaData[_T( "WeibullRandom" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );

    m_metaData[_T( "GaussianRandom" )] = meta();
    m_metaData[_T( "GaussianRandom" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );
    m_metaData[_T( "GaussianRandom" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );
    m_metaData[_T( "GaussianRandom" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );

    m_metaData[_T( "TriangleRandom" )] = meta();
    m_metaData[_T( "TriangleRandom" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );
    m_metaData[_T( "TriangleRandom" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );
    m_metaData[_T( "TriangleRandom" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );
    m_metaData[_T( "TriangleRandom" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );

    m_metaData[_T( "UniformOnSphere" )] = meta();
    m_metaData[_T( "UniformOnSphere" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );
    m_metaData[_T( "UniformOnSphere" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT );

    m_metaData[_T( "Noise" )] = meta();
    m_metaData[_T( "Noise" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                         MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "Noise" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT );

    m_metaData[_T( "VecNoise" )] = meta();
    m_metaData[_T( "VecNoise" )].push_back( MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT |
                                            MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 );
    m_metaData[_T( "VecNoise" )].push_back( MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_FLOAT );
}

////////////////////////////////////////////////////////////////////////////////

void build_property_info( holder::maya_magma_holder& holder, info::maya_magma_node_info& outNodeInfo,
                          holder::magma_id id ) {
    int numProperties = holder.get_node_num_properties( id );
    for( int i = 0; i < numProperties; i++ ) {
        info::maya_magma_node_property_info currentProperty;
        frantic::tstring propertyName = holder.get_node_property_name( id, i );

        currentProperty.m_index = i;
        currentProperty.m_name = propertyName;
        currentProperty.m_type = holder.get_node_property_type( id, propertyName );
        currentProperty.m_isReadOnly = holder.get_node_property_readonly( id, propertyName );
        currentProperty.m_acceptedValues = holder.get_node_enum_values( id, propertyName );

        outNodeInfo.push_property_info( currentProperty );
    }
}

void build_input_socket_info( holder::maya_magma_holder& holder, info::maya_magma_node_info& outNodeInfo,
                              holder::magma_id id,
                              const detail::maya_magma_input_socket_info_manager::meta* metaData ) {
    int numInputSocket = holder.get_num_node_inputs( id );
    for( int i = 0; i < numInputSocket; i++ ) {
        info::maya_magma_node_input_socket_info currentInputSocketInfo;

        currentInputSocketInfo.m_index = i;
        currentInputSocketInfo.m_description = holder.get_node_input_description( id, i );
        currentInputSocketInfo.m_data = holder.get_node_input_default_value( id, i );

        if( metaData != NULL )
            currentInputSocketInfo.m_dataType = metaData->m_inputSocketInfos[i];
        else
            currentInputSocketInfo.m_dataType = MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_NONE;
        outNodeInfo.push_input_socket_info( currentInputSocketInfo );
    }
}

void build_output_socket_info( holder::maya_magma_holder& holder, info::maya_magma_node_info& outNodeInfo,
                               holder::magma_id id ) {
    int numOutputSocket = holder.get_num_node_outputs( id );
    for( int i = 0; i < numOutputSocket; i++ ) {
        info::maya_magma_node_output_socket_info currentOutputSocketInfo;
        currentOutputSocketInfo.m_index = i;
        currentOutputSocketInfo.m_description = holder.get_node_output_description( id, i );
        outNodeInfo.push_output_socket_info( currentOutputSocketInfo );
    }
}

} // namespace detail

/// TODO use RAII
info::maya_magma_node_info maya_magma_node_info_factory::create_node_infos( const frantic::tstring& nodeType ) {
    info::maya_magma_node_info outNodeInfo;
    // get a maya magma holder used for retrieving a node info
    std::unique_ptr<magma_interface> mayaMagmaSingleton =
        modifier::maya_magma_singleton::get_instance().create_magma_instance();
    holder::maya_magma_holder mayaMagmaHolder( std::move( mayaMagmaSingleton ) );
    holder::magma_id id = mayaMagmaHolder.create_node( nodeType );

    if( id == holder::kInvalidMagmaID )
        throw maya_magma_exception( _T( "maya_magma_node_info_factory::create_node_infos invalid nodeType: \"" ) +
                                    nodeType + _T( "\"" ) );

    // get a maya_magma_input_socket_info_manager used for retrieving a input socket data type information
    const detail::maya_magma_input_socket_info_manager& inputSocketManager =
        detail::maya_magma_input_socket_info_manager::get_instance();
    const detail::maya_magma_input_socket_info_manager::meta* metaData = inputSocketManager.get_meta( nodeType );

    outNodeInfo.m_nodeType = nodeType;
    outNodeInfo.m_typeDescription = mayaMagmaHolder.get_type_description( nodeType );
    outNodeInfo.m_nodeCategory = mayaMagmaHolder.get_type_category( nodeType );

    detail::build_property_info( mayaMagmaHolder, outNodeInfo, id );
    detail::build_input_socket_info( mayaMagmaHolder, outNodeInfo, id, metaData );
    detail::build_output_socket_info( mayaMagmaHolder, outNodeInfo, id );

    // Extra properties
    if( nodeType == _T("Mux") ) {
        info::maya_magma_node_property_info muxNumInputProperty;
        muxNumInputProperty.m_index = mayaMagmaHolder.get_node_num_properties( id );
        muxNumInputProperty.m_name = _T("NumInputs");
        muxNumInputProperty.m_type = _T("Int");
        muxNumInputProperty.m_isReadOnly = false;
        muxNumInputProperty.m_acceptedValues = std::vector<frantic::tstring>();
        outNodeInfo.push_property_info( muxNumInputProperty );
    }

    mayaMagmaHolder.delete_node( id );
    return outNodeInfo;
}

} // namespace factory
} // namespace maya
} // namespace magma
} // namespace frantic
