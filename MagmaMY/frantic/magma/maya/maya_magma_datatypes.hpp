// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <boost/cstdint.hpp>

#include <frantic/graphics/quat4f.hpp>
#include <frantic/graphics/vector3f.hpp>

#include <frantic/maya/animation/animation_data.hpp>

#include <frantic/logging/logging_level.hpp>
#include <frantic/magma/magma_interface.hpp>
#include <frantic/strings/tstring.hpp>

// Required to support variant of vectors
#include <ostream>
namespace std {
template <typename T>
ostream& operator<<( ostream& out, const vector<T>& v ) {
    bool first = true;
    out << "[";
    typename vector<T>::const_iterator iter;
    for( iter = v.begin(); iter != v.end(); ++iter ) {
        if( first ) {
            first = false;
        } else {
            out << ",";
        }
        out << ( *iter );
    }
    out << "]";
    return out;
}

template <typename T>
wostream& operator<<( wostream& out, const vector<T>& v ) {
    bool first = true;
    out << _T( "[" );
    typename vector<T>::const_iterator iter;
    for( iter = v.begin(); iter != v.end(); ++iter ) {
        if( first ) {
            first = false;
        } else {
            out << _T( "," );
        }
        out << ( *iter );
    }
    out << _T( "]" );
    return out;
}

} // namespace std

namespace frantic {
namespace magma {
namespace maya {

enum maya_attribute_input_socket_data_type {
    MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_NONE = 0,
    MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_BOOL,
    MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_FLOAT,
    MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_INT,
    MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_VEC3,
    MAYA_ATTR_INPUT_SOCKET_DATA_TYPE_QUAT,
};

/// this enum type define available input socket data type of a magma node
/// this data corresponding to the magma_standard_operators.hpp in Magma Project;
///
/// in fact, this will be passed to maya as a int, so that make sure this less or equal
/// to the num of bits of integer type
enum maya_magma_input_socket_data_type {
    MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_NONE = 0x00000000,
    MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_BOOL = 0x00000001,
    MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_FLOAT = 0x00000002,
    MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_INT = 0x00000004,
    MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_VEC3 = 0x00000008,
    MAYA_MAGMA_INPUT_SOCKET_DATA_TYPE_QUAT = 0x00000010,
};
typedef boost::uint32_t maya_magma_input_socket_data_type_t;

enum maya_magma_serializer_t {
    MAYA_MAGMA_SERIALIZER_STANDARD,
    NUM_MAYA_MAGMA_SERIALIZER,
};

class maya_magma_exception;

namespace desc {

class maya_magma_desc_input_socket;
class maya_magma_desc_node;
class maya_magma_desc_connection;
class maya_magma_desc;

typedef int desc_id;
typedef int desc_index_type;
typedef boost::shared_ptr<maya_magma_desc> maya_magma_desc_ptr;

extern const desc_id kInvalidDescID;
extern const desc_index_type kInvalidDescSocketIndex;
extern const frantic::tstring kInvalidNodeType;

extern const frantic::tstring kInvalidEnumAttrName;

} // namespace desc

namespace info {

struct maya_magma_info_interface;
struct maya_magma_node_property_info;
struct maya_magma_node_input_socket_info;
struct maya_magma_node_output_socket_info;
struct maya_magma_node_info;

} // namespace info

namespace holder {

class maya_magma_holder;

typedef frantic::magma::magma_interface::magma_id magma_id;

// IMagmaHolder::index_type
typedef int index_type;

// boost::variant<boost::blank,float,int,bool,vec3,quat>;
typedef frantic::magma::variant_t input_socket_variant_t;

// all the data types that property might take
typedef boost::variant<boost::blank, frantic::tstring, float, int, bool, frantic::graphics::vector3f,
                       frantic::graphics::quat4f, std::vector<frantic::tstring>, std::vector<int>>
    property_variant_t;

// data types for animation
typedef boost::variant<boost::blank, frantic::maya::animation::animation_data,
                       frantic::maya::animation::animation_data_vector3,
                       frantic::maya::animation::animation_data_vector4>
    property_animation_variant_t;

typedef property_animation_variant_t input_socket_animation_variant_t;

extern const magma_id kInvalidMagmaID;

} // namespace holder

} // namespace maya
} // namespace magma
} // namespace frantic
