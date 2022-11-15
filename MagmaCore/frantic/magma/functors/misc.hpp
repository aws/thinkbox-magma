// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <cmath>

#include <boost/random/exponential_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/triangle_distribution.hpp>
#include <boost/random/uniform_on_sphere.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#include <boost/random/weibull_distribution.hpp>

#include <frantic/magma/magma_data_type.hpp>

#include <frantic/graphics/quat4f.hpp>
#include <frantic/graphics/vector3f.hpp>
#include <frantic/math/perlin_noise_generator.hpp>

namespace frantic {
namespace magma {
namespace functors {

struct to_float {
    template <class T>
    float operator()( const T& val ) const {
        return static_cast<float>( val );
    }
    float operator()( bool val ) const { return val ? 1.f : 0.f; }
};

struct to_int {
    template <class T>
    int operator()( const T& val ) const {
        return static_cast<int>( val );
    }
    int operator()( bool val ) const { return val ? 1 : 0; }
};

struct to_vector {
    frantic::graphics::vector3f operator()( float x, float y, float z ) const {
        return frantic::graphics::vector3f( x, y, z );
    }
};

struct clamp {
    template <class T>
    T operator()( const T& val, const T& min, const T& max ) const {
        if( val < min ) {
            return min;
        } else if( val > max ) {
            return max;
        } else {
            return val;
        }
    }

    frantic::graphics::vector3f operator()( const frantic::graphics::vector3f& val, float min, float max ) const {
        return frantic::graphics::vector3f( ( *this )( val.x, min, max ), ( *this )( val.y, min, max ),
                                            ( *this )( val.z, min, max ) );
    }
};

struct blend {
    template <class T>
    T operator()( const T& lhs, const T& rhs, float alpha ) const {
        return lhs + alpha * ( rhs - lhs );
    }
};

struct blend_clamped {
    template <class T>
    T operator()( const T& lhs, const T& rhs, float alpha ) const {
        if( alpha <= 0.f )
            return lhs;
        else if( alpha >= 1.f )
            return rhs;
        else
            return lhs + alpha * ( rhs - lhs );
    }
};

struct blend_smooth {
    template <class T>
    T operator()( const T& lhs, const T& rhs, float alpha ) const {
        if( alpha <= 0.f )
            return lhs;
        else if( alpha >= 1.f )
            return rhs;
        else {
            float a2 = alpha * alpha;
            float a3 = alpha * a2;
            return lhs + ( 3 * a2 - 2 * a3 ) * ( rhs - lhs );
        }
    }
};

class noise1f {
    mutable frantic::math::perlin_noise_generator m_noiseGen;
    float m_freq, m_scale;

  public:
    noise1f( float freq = 1.f, int numLevels = 1, float persistence = 0.5f, bool normalize = true )
        : m_freq( freq )
        , m_noiseGen( numLevels, persistence ) {
        m_scale = 1.f;
        if( normalize && numLevels > 1 ) {
            if( persistence == 1.f )
                m_scale = 1.f / (float)numLevels;
            else
                m_scale = ( persistence - 1.f ) / ( powf( persistence, (float)numLevels ) - 1.f );
        }
    }

    inline float operator()( const vec3& pos, float phase ) const {
        return m_scale * m_noiseGen.noise( m_freq * pos.x + phase, m_freq * pos.y + phase, m_freq * pos.z + phase );
    }

    inline float operator()( float pos, float phase ) const {
        return m_scale * m_noiseGen.noise( m_freq * pos + phase );
    }
};

class noise3f {
    mutable frantic::math::perlin_noise_generator m_noiseGenX, m_noiseGenY, m_noiseGenZ;
    float m_freq, m_scale;

  public:
    noise3f( float freq = 1.f, int numLevels = 1, float persistence = 0.5f, bool normalize = true )
        : m_freq( freq )
        , m_noiseGenX( numLevels, persistence )
        , m_noiseGenY( numLevels, persistence )
        , m_noiseGenZ( numLevels, persistence ) {
        m_noiseGenX.generate_permutations( 256, 2 );
        m_noiseGenY.generate_permutations( 256, 13 );
        m_noiseGenZ.generate_permutations( 256, 1023 );

        m_scale = 1.f;
        if( normalize && numLevels > 1 ) {
            if( persistence == 1.f )
                m_scale = 1.f / (float)numLevels;
            else
                m_scale = ( persistence - 1.f ) / ( powf( persistence, (float)numLevels ) - 1.f );
        }
    }

    inline frantic::graphics::vector3f operator()( float val, float phase ) const {
        val *= m_freq;
        val += phase;

        return frantic::graphics::vector3f( m_scale * m_noiseGenX.noise( val ), m_scale * m_noiseGenY.noise( val ),
                                            m_scale * m_noiseGenZ.noise( val ) );
    }

    inline frantic::graphics::vector3f operator()( const frantic::graphics::vector3f& pos, float phase ) const {
        float x = m_freq * pos.x + phase, y = m_freq * pos.y + phase, z = m_freq * pos.z + phase;

        return frantic::graphics::vector3f( m_scale * m_noiseGenX.noise( x, y, z ),
                                            m_scale * m_noiseGenY.noise( x, y, z ),
                                            m_scale * m_noiseGenZ.noise( x, y, z ) );
    }
};

// Although the mt19937 (Mersenne Twist) engine would likely produce better psueudo-random numbers,
// it's not particularly well suited for this usage case. Because we allow per-particle seeding (which is necessary
// to ensure that the output remains deterministic when particles are run through the Magma in parallel), we need to
// initialize the engine for each particle individually. For mt19937 this means generating 625*sizeof(uint32_t) of data
// per-particle per-operator. rand48 (linear_congruential) only needs to generate sizeof(uint32_t) per-particle
// per-operator, so is much faster in this case.
typedef boost::random::mt11213b random_engine_t;

class random1f {
  public:
    inline float operator()( int seed, float min, float max ) const {
        if( min >= max ) {
            return min;
        }
        random_engine_t gen( seed );
        boost::random::uniform_real_distribution<float> dist( min, max );
        return dist( gen );
    }
};

class random3f {
  public:
    inline vec3 operator()( int seed, float min, float max ) const {
        if( min >= max ) {
            return vec3( min, min, min );
        }
        random_engine_t gen( seed );
        boost::random::uniform_real_distribution<float> dist( min, max );
        return vec3( dist( gen ), dist( gen ), dist( gen ) );
    }

    inline vec3 operator()( int seed, vec3 min, float max ) const {
        vec3 result;
        random_engine_t gen( seed );
        boost::random::uniform_real_distribution<float> dist( 0.0f, 1.0f );

        if( min.x >= max ) {
            result.x = min.x;
        } else {
            dist = boost::random::uniform_real_distribution<float>( min.x, max );
            result.x = dist( gen );
        }

        if( min.y >= max ) {
            result.y = min.y;
        } else {
            dist = boost::random::uniform_real_distribution<float>( min.y, max );
            result.y = dist( gen );
        }

        if( min.z >= max ) {
            result.z = min.z;
        } else {
            dist = boost::random::uniform_real_distribution<float>( min.z, max );
            result.z = dist( gen );
        }

        return result;
    }

    inline vec3 operator()( int seed, float min, vec3 max ) const {
        vec3 result;
        random_engine_t gen( seed );
        boost::random::uniform_real_distribution<float> dist( 0.0f, 1.0f );
        if( min >= max.x ) {
            result.x = min;
        } else {
            dist = boost::random::uniform_real_distribution<float>( min, max.x );
            result.x = dist( gen );
        }

        if( min >= max.y ) {
            result.y = min;
        } else {
            dist = boost::random::uniform_real_distribution<float>( min, max.y );
            result.y = dist( gen );
        }

        if( min >= max.z ) {
            result.z = min;
        } else {
            dist = boost::random::uniform_real_distribution<float>( min, max.z );
            result.z = dist( gen );
        }

        return result;
    }

    inline vec3 operator()( int seed, vec3 min, vec3 max ) const {
        vec3 result;
        random_engine_t gen( seed );
        boost::random::uniform_real_distribution<float> dist( 0.0f, 1.0f );
        if( min.x >= max.x ) {
            result.x = min.x;
        } else {
            dist = boost::random::uniform_real_distribution<float>( min.x, max.x );
            result.x = dist( gen );
        }

        if( min.y >= max.y ) {
            result.y = min.y;
        } else {
            dist = boost::random::uniform_real_distribution<float>( min.y, max.y );
            result.y = dist( gen );
        }

        if( min.z >= max.z ) {
            result.z = min.z;
        } else {
            dist = boost::random::uniform_real_distribution<float>( min.z, max.z );
            result.z = dist( gen );
        }

        return result;
    }
};

inline float ensure_positive( float x ) { return std::max( x, std::numeric_limits<float>::min() ); }

class exponential_random1f {
  public:
    inline float operator()( int seed, float lambda ) const {
        random_engine_t gen( seed );
        boost::random::exponential_distribution<float> dist( ensure_positive( lambda ) );
        return dist( gen );
    }
};

class weibull_random1f {
  public:
    inline float operator()( int seed, float lambda, float kappa ) const {
        random_engine_t gen( seed );
        boost::random::weibull_distribution<float> dist( ensure_positive( lambda ), ensure_positive( kappa ) );
        return dist( gen );
    }
};

class gaussian_random1f {
  public:
    inline float operator()( int seed, float mean, float sigma ) const {
        random_engine_t gen( seed );
        boost::random::normal_distribution<float> dist( mean, ensure_positive( sigma ) );
        return dist( gen );
    }
};

class triangle_random1f {
  public:
    inline float operator()( int seed, float min, float probable, float max ) const {
        if( min >= probable ) {
            return min;
        }
        if( probable >= max ) {
            return max;
        }

        random_engine_t gen( seed );
        boost::random::triangle_distribution<float> dist( min, probable, max );
        return dist( gen );
    }
};

inline float get_component( std::size_t component, const std::vector<float>& components ) {
    if( component >= components.size() ) {
        return 0.0f;
    } else {
        return components[component];
    }
}

class uniform_sphere_random3f {
  public:
    inline vec3 operator()( int seed, int dimensions ) const {
        random_engine_t gen( seed );
        boost::random::uniform_on_sphere<float> dist( dimensions < 0 ? 0 : dimensions );
        const std::vector<float> result = dist( gen );
        return vec3( get_component( 0, result ), get_component( 1, result ), get_component( 2, result ) );
    }
};

} // namespace functors
} // namespace magma
} // namespace frantic
