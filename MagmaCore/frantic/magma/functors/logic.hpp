// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <cmath>

#include <frantic/graphics/quat4f.hpp>
#include <frantic/graphics/vector3f.hpp>

namespace frantic {
namespace magma {
namespace functors {

struct logical_not {
    bool operator()( bool val ) const { return !val; }
};

struct logical_and {
    bool operator()( bool lhs, bool rhs ) const { return lhs && rhs; }
};

struct logical_or {
    bool operator()( bool lhs, bool rhs ) const { return lhs || rhs; }
};

struct logical_xor {
    bool operator()( bool lhs, bool rhs ) const { return ( lhs && !rhs ) || ( !lhs && rhs ); }
};

struct less {
    template <class T>
    bool operator()( const T& lhs, const T& rhs ) const {
        return lhs < rhs;
    }
};

struct less_tol {
    float m_tol;

    less_tol( float tol = 1e-5f )
        : m_tol( tol ) {}

    template <class T>
    bool operator()( const T& lhs, const T& rhs ) const {
        return ( rhs - lhs ) > m_tol * ( 1.f + std::abs( rhs ) );
    } // (lhs - rhs) < -tol * |rhs| w/ -1 multiplied on each side.
};

struct lesseq {
    template <class T>
    bool operator()( const T& lhs, const T& rhs ) const {
        return lhs <= rhs;
    }
};

struct lesseq_tol {
    float m_tol;

    lesseq_tol( float tol = 1e-5f )
        : m_tol( tol ) {}

    template <class T>
    bool operator()( const T& lhs, const T& rhs ) const {
        return ( lhs - rhs ) <= m_tol * ( 1.f + std::abs( rhs ) );
    }
};

struct greater {
    template <class T>
    bool operator()( const T& lhs, const T& rhs ) const {
        return lhs > rhs;
    }
};

struct greater_tol {
    float m_tol;

    greater_tol( float tol = 1e-5f )
        : m_tol( tol ) {}

    template <class T>
    bool operator()( const T& lhs, const T& rhs ) const {
        return ( lhs - rhs ) > m_tol * ( 1.f + std::abs( rhs ) );
    }
};

struct greatereq {
    template <class T>
    bool operator()( const T& lhs, const T& rhs ) const {
        return lhs >= rhs;
    }
};

struct greatereq_tol {
    float m_tol;

    greatereq_tol( float tol = 1e-5f )
        : m_tol( tol ) {}

    template <class T>
    bool operator()( const T& lhs, const T& rhs ) const {
        return ( rhs - lhs ) <= m_tol * ( 1.f + std::abs( rhs ) );
    } // (lhs - rhs) > -tol * |rhs| w/ -1 multiplied on each side.
};

struct equal {
    template <class T>
    bool operator()( const T& lhs, const T& rhs ) const {
        return lhs == rhs;
    }
};

struct equal_tol {
    float m_tol;

    equal_tol( float tol = 1e-5f )
        : m_tol( tol ) {}

    bool operator()( float lhs, float rhs ) const { return std::abs( lhs - rhs ) <= m_tol * ( 1.f + std::abs( rhs ) ); }
    bool operator()( const frantic::graphics::vector3f& lhs, const frantic::graphics::vector3f& rhs ) const {
        return ( *this )( lhs.x, rhs.x ) && ( *this )( lhs.y, rhs.y ) && ( *this )( lhs.z, rhs.z );
    }
    bool operator()( const frantic::graphics::quat4f& lhs, const frantic::graphics::quat4f& rhs ) const {
        return ( *this )( lhs.x, rhs.x ) && ( *this )( lhs.y, rhs.y ) && ( *this )( lhs.z, rhs.z ) &&
               ( *this )( lhs.w, rhs.w );
    }
};

struct notequal {
    template <class T>
    bool operator()( const T& lhs, const T& rhs ) const {
        return lhs != rhs;
    }
};

struct notequal_tol {
    float m_tol;

    notequal_tol( float tol = 1e-5f )
        : m_tol( tol ) {}

    bool operator()( float lhs, float rhs ) const { return std::abs( lhs - rhs ) > m_tol * ( 1.f + std::abs( rhs ) ); }
    bool operator()( const frantic::graphics::vector3f& lhs, const frantic::graphics::vector3f& rhs ) const {
        return ( *this )( lhs.x, rhs.x ) || ( *this )( lhs.y, rhs.y ) || ( *this )( lhs.z, rhs.z );
    }
    bool operator()( const frantic::graphics::quat4f& lhs, const frantic::graphics::quat4f& rhs ) const {
        return ( *this )( lhs.x, rhs.x ) || ( *this )( lhs.y, rhs.y ) || ( *this )( lhs.z, rhs.z ) ||
               ( *this )( lhs.w, rhs.w );
    }
};

struct logicswitch {
    template <class T>
    T operator()( const T& lhs, const T& rhs, bool val ) const {
        return val ? lhs : rhs;
    }

    template <class T>
    T operator()( const T& lhs, const T& rhs, int intBoolVal ) const {
        return ( intBoolVal != 0 ) ? lhs : rhs;
    }
};

} // namespace functors
} // namespace magma
} // namespace frantic
