// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cmath>

#include <boost/math/constants/constants.hpp>

namespace frantic {
namespace magma {
namespace functors {

struct cos {
    float operator()( float val ) const { return std::cos( val ); }
};

struct acos {
    float operator()( float val ) const {
        // Since acos gets angry when you aren't in [-1,1] (even by a little bit) I am clamping the
        // value so that unsuspecting Magma users get more friendly results.
        if( val <= -1.f )
            return boost::math::constants::pi<float>();
        else if( val >= 1.f )
            return 0;
        return std::acos( val );
    }
};

struct sin {
    float operator()( float val ) const { return std::sin( val ); }
};

struct asin {
    float operator()( float val ) const {
        // Since asin gets angry when you aren't in [-1,1] (even by a little bit) I am clamping the
        // value so that unsuspecting Magma users get more friendly results.
        if( val <= -1.f )
            return -boost::math::constants::half_pi<float>();
        else if( val >= 1.f )
            return boost::math::constants::half_pi<float>();
        return std::asin( val );
    }
};

struct tan {
    float operator()( float val ) const { return std::tan( val ); }
};

struct atan {
    float operator()( float val ) const { return std::atan( val ); }
};

struct atan2 {
    float operator()( float numerator, float denominator ) const { return std::atan2( numerator, denominator ); }
};

} // namespace functors
} // namespace magma
} // namespace frantic
