// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/graphics/quat4f.hpp>
#include <frantic/graphics/vector3f.hpp>

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <cmath>

namespace frantic {
namespace magma {
namespace functors {

struct negate {
    template <class T>
    T operator()( const T& val ) const {
        return -val;
    }
};

struct abs {
    template <class T>
    T operator()( const T& val ) const {
        return std::abs( val );
    }

    frantic::graphics::vector3f operator()( const frantic::graphics::vector3f& val ) const {
        return frantic::graphics::vector3f( std::abs( val.x ), std::abs( val.y ), std::abs( val.z ) );
    }
};

struct floor {
    template <class T>
    T operator()( const T& val ) const {
        return std::floor( val );
    }

    frantic::graphics::vector3f operator()( const frantic::graphics::vector3f& val ) const {
        return frantic::graphics::vector3f( std::floor( val.x ), std::floor( val.y ), std::floor( val.z ) );
    }
};

struct ceil {
    template <class T>
    T operator()( const T& val ) const {
        return std::ceil( val );
    }

    frantic::graphics::vector3f operator()( const frantic::graphics::vector3f& val ) const {
        return frantic::graphics::vector3f( std::ceil( val.x ), std::ceil( val.y ), std::ceil( val.z ) );
    }
};

struct sqrt {
    template <class T>
    T operator()( const T& val ) const {
        return std::sqrt( val );
    }
};

struct log {
    template <class T>
    T operator()( const T& val ) const {
        return std::log( val );
    }
};

struct add {
    template <class T>
    T operator()( const T& lhs, const T& rhs ) const {
        return lhs + rhs;
    }
};

struct sub {
    template <class T>
    T operator()( const T& lhs, const T& rhs ) const {
        return lhs - rhs;
    }
};

struct mul {
    template <class T>
    T operator()( const T& lhs, const T& rhs ) const {
        return lhs * rhs;
    }

    frantic::graphics::vector3f operator()( float lhs, const frantic::graphics::vector3f& rhs ) const {
        return lhs * rhs;
    }
    frantic::graphics::vector3f operator()( const frantic::graphics::vector3f& lhs, float rhs ) const {
        return lhs * rhs;
    }

    frantic::graphics::vector3f operator()( const frantic::graphics::vector3f& lhs,
                                            const frantic::graphics::vector3f& rhs ) const {
        return frantic::graphics::vector3f::component_multiply( lhs, rhs );
    }
};

struct div {
    template <class T>
    T operator()( const T& lhs, const T& rhs ) const {
        return lhs / rhs;
    }

    int operator()( int lhs, int rhs ) const { return rhs != 0 ? ( lhs / rhs ) : INT_MAX; }

    frantic::graphics::vector3f operator()( const frantic::graphics::vector3f& lhs, float rhs ) const {
        return lhs / rhs;
    }

    frantic::graphics::vector3f operator()( const frantic::graphics::vector3f& lhs,
                                            const frantic::graphics::vector3f& rhs ) const {
        return frantic::graphics::vector3f::component_divide( lhs, rhs );
    }
};

struct mod {
    int operator()( int lhs, int rhs ) const { return rhs != 0 ? ( lhs % rhs ) : 0; }

    float operator()( float lhs, float rhs ) const { return std::fmod( lhs, rhs ); }

    frantic::graphics::vector3f operator()( const frantic::graphics::vector3f& lhs, float rhs ) const {
        return frantic::graphics::vector3f( std::fmod( lhs.x, rhs ), std::fmod( lhs.y, rhs ), std::fmod( lhs.z, rhs ) );
    }

    frantic::graphics::vector3f operator()( const frantic::graphics::vector3f& lhs,
                                            const frantic::graphics::vector3f& rhs ) const {
        return frantic::graphics::vector3f( std::fmod( lhs.x, rhs.x ), std::fmod( lhs.y, rhs.y ),
                                            std::fmod( lhs.z, rhs.z ) );
    }
};

struct pow {
    float operator()( float lhs, float rhs ) const { return std::pow( lhs, rhs ); }
    float operator()( float lhs, int rhs ) const { return std::pow( lhs, rhs ); }
    int operator()( int lhs, int rhs ) const {
        if( rhs < 0 ) {
            return lhs == 1 ? 1 : 0;
        } else {
            int out = 1;
            for( ; rhs > 0; --rhs )
                out *= lhs;
            return out;
        }
    }

    frantic::graphics::vector3f operator()( const frantic::graphics::vector3f& lhs, float rhs ) const {
        return frantic::graphics::vector3f( std::pow( lhs.x, rhs ), std::pow( lhs.y, rhs ), std::pow( lhs.z, rhs ) );
    }
};

} // namespace functors
} // namespace magma
} // namespace frantic
