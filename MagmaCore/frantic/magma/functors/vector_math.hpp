// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <cmath>

#include <frantic/channels/channel_map.hpp>
#include <frantic/graphics/quat4f.hpp>
#include <frantic/graphics/transform4f.hpp>
#include <frantic/graphics/vector3f.hpp>

namespace frantic {
namespace magma {
namespace functors {

struct component_sum {
    float operator()( float val ) const { return val; }
    int operator()( int val ) const { return val; }
    float operator()( const frantic::graphics::vector3f& val ) const { return val.x + val.y + val.z; }
    float operator()( const frantic::graphics::quat4f& val ) const { return val.x + val.y + val.z + val.w; }
};

struct magnitude {
    float operator()( const frantic::graphics::vector3f& val ) const { return val.get_magnitude(); }
};

struct normalize {
    frantic::graphics::vector3f operator()( const frantic::graphics::vector3f& val ) const {
        return frantic::graphics::vector3f::normalize( val );
    }
};

struct dot {
    float operator()( const frantic::graphics::vector3f& lhs, const frantic::graphics::vector3f& rhs ) const {
        return frantic::graphics::vector3f::dot( lhs, rhs );
    }
};

struct cross {
    frantic::graphics::vector3f operator()( const frantic::graphics::vector3f& lhs,
                                            const frantic::graphics::vector3f& rhs ) const {
        return frantic::graphics::vector3f::cross( lhs, rhs );
    }
};

struct vectors_to_quat {
    frantic::graphics::quat4f operator()( const frantic::graphics::vector3f& x, const frantic::graphics::vector3f& y,
                                          const frantic::graphics::vector3f& z ) const {
        return frantic::graphics::quat4f::from_coord_sys( x, y, z );
    }
};

struct eulerangles_to_quat {
    frantic::graphics::quat4f operator()( float x, float y, float z ) const {
        return frantic::graphics::quat4f::from_euler_angles( x, y, z );
    }
};

struct angleaxis_to_quat {
    frantic::graphics::quat4f operator()( float angle, const frantic::graphics::vector3f& axis ) const {
        return frantic::graphics::quat4f::from_angle_axis( angle, frantic::graphics::vector3f::normalize( axis ) );
    }
};

class quat_to_vectors {
    static frantic::channels::channel_map s_staticMap; // Describes the output.

    typedef frantic::graphics::vector3f matrix3[3];

  public:
    const frantic::channels::channel_map& get_output_map() const {
        if( !s_staticMap.channel_definition_complete() ) { // HACK This could be a threading race-condition.
            s_staticMap.define_channel( _T("Column1"), 3, frantic::channels::data_type_float32, 0 );
            s_staticMap.define_channel( _T("Column2"), 3, frantic::channels::data_type_float32, 12 );
            s_staticMap.define_channel( _T("Column3"), 3, frantic::channels::data_type_float32, 24 );
            s_staticMap.end_channel_definition();
        }
        return s_staticMap;
    }

    void operator()( void* _out, const frantic::graphics::quat4f& val ) const {
        frantic::graphics::transform4f tm;

        val.as_transform4f( tm );

        matrix3& out = *reinterpret_cast<matrix3*>( _out );

        out[0] = tm.get_column( 0 );
        out[1] = tm.get_column( 1 );
        out[2] = tm.get_column( 2 );
    }
};

struct matrix_mul {
    typedef frantic::graphics::vector3f matrix3[3];

    frantic::graphics::vector3f operator()( const frantic::graphics::vector3f& val, const matrix3& m ) const {
        return val.x * m[0] + val.y * m[1] + val.z * m[2];
    }

    frantic::graphics::vector3f operator()( const frantic::graphics::vector3f& val,
                                            const frantic::graphics::vector3f& c1,
                                            const frantic::graphics::vector3f& c2,
                                            const frantic::graphics::vector3f& c3 ) const {
        return val.x * c1 + val.y * c2 + val.z * c3;
    }
};

struct quat_mul {
    frantic::graphics::quat4f operator()( const frantic::graphics::quat4f& lhs,
                                          const frantic::graphics::quat4f& rhs ) const {
        return lhs * rhs;
    }

    frantic::graphics::vector3f operator()( const frantic::graphics::quat4f& lhs,
                                            const frantic::graphics::vector3f& rhs ) const {
        frantic::graphics::transform4f xform;
        lhs.as_transform4f( xform );
        return xform * rhs;
    }
};

class transform_point {
    std::vector<frantic::graphics::transform4f> m_xform;

  public:
    transform_point( const std::vector<frantic::graphics::transform4f>& tm )
        : m_xform( tm ) {}
    transform_point( const frantic::graphics::transform4f& tm )
        : m_xform( std::vector<frantic::graphics::transform4f>( 1, tm ) ) {}

    frantic::graphics::vector3f operator()( const frantic::graphics::vector3f& val, const int index = 0 ) const {
        if( index < m_xform.size() ) {
            return m_xform[index] * val;
        } else {
            return frantic::graphics::vector3f();
        }
    }
};

class transform_direction {
    std::vector<frantic::graphics::transform4f> m_xform;

  public:
    transform_direction( const std::vector<frantic::graphics::transform4f>& tm )
        : m_xform( tm ) {}
    transform_direction( const frantic::graphics::transform4f& tm )
        : m_xform( std::vector<frantic::graphics::transform4f>( 1, tm ) ) {}

    frantic::graphics::vector3f operator()( const frantic::graphics::vector3f& val, const int index = 0 ) const {
        if( index < m_xform.size() ) {
            return m_xform[index].transform_no_translation( val );
        } else {
            return frantic::graphics::vector3f();
        }
    }
};

class transform_normal {
    std::vector<frantic::graphics::transform4f> m_xform;

  public:
    transform_normal( const std::vector<frantic::graphics::transform4f>& tm ) {
        for( std::size_t i = 0; i < tm.size(); ++i ) {
            m_xform.push_back( tm[i].to_inverse() );
        }
    }
    transform_normal( const frantic::graphics::transform4f& tm )
        : m_xform( std::vector<frantic::graphics::transform4f>( 1, tm.to_inverse() ) ) {}

    frantic::graphics::vector3f operator()( const frantic::graphics::vector3f& val, const int index = 0 ) const {
        if( index < m_xform.size() ) {
            return m_xform[index].transpose_transform_no_translation( val );
        } else {
            return frantic::graphics::vector3f();
        }
    }
};

} // namespace functors
} // namespace magma
} // namespace frantic
