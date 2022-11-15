// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/nodes/magma_node_property.hpp>
#include <frantic/magma/nodes/magma_simple_operator.hpp>

#include <frantic/magma/functors/arithmetic.hpp>
#include <frantic/magma/functors/logic.hpp>
#include <frantic/magma/functors/misc.hpp>
#include <frantic/magma/functors/trigonometry.hpp>
#include <frantic/magma/functors/vector_math.hpp>

#include <boost/mpl/vector.hpp>

#define MAGMA_HAS_CONSTRUCTOR_false( name )

#define MAGMA_HAS_CONSTRUCTOR_true( name )                                                                             \
  public:                                                                                                              \
    magma_##name##_node();

#define MAGMA_BEGIN_SIMPLE_OP_IMPL( name, num_inputs, hasContructor, /*fnBindings*/... )                               \
    class magma_##name##_node : public magma_simple_operator<num_inputs> {                                             \
        MAGMA_HAS_CONSTRUCTOR_##hasContructor( name ) public : struct meta {                                           \
            static const int ARITY = num_inputs;                                                                       \
            typedef frantic::magma::functors::name type;                                                               \
            typedef __VA_ARGS__ bindings;                                                                              \
        };                                                                                                             \
                                                                                                                       \
      private:

#define MAGMA_BEGIN_SIMPLE_OP( name, num_inputs, fnBindings )                                                          \
    MAGMA_BEGIN_SIMPLE_OP_IMPL( name, num_inputs, true, fnBindings )

#define MAGMA_END_SIMPLE_OP                                                                                            \
  public:                                                                                                              \
    static void create_type_definition( magma_node_type& outType );                                                    \
    virtual void compile( magma_compiler_interface& compile );                                                         \
    }                                                                                                                  \
    ;

#define MAGMA_DECLARE_SIMPLE_OP( name, num_inputs, fnBindings )                                                        \
    MAGMA_BEGIN_SIMPLE_OP_IMPL( name, num_inputs, false, fnBindings )                                                  \
    MAGMA_END_SIMPLE_OP

#define BINDINGS( ... ) boost::mpl::vector<__VA_ARGS__>

namespace frantic {
namespace magma {
namespace nodes {

namespace detail {
template <int Arity, class Functor, class Bindings>
struct declare_meta {
    static const int ARITY = Arity;
    typedef Functor type;
    typedef Bindings bindings;
};
} // namespace detail

// MAGMA_DECLARE_SIMPLE_OP( elbow, 1,  );

MAGMA_DECLARE_SIMPLE_OP( negate, 1, BINDINGS( float( float ), int( int ), vec3( vec3 ) ) );
MAGMA_DECLARE_SIMPLE_OP( abs, 1, BINDINGS( float( float ), int( int ), vec3( vec3 ) ) );
MAGMA_DECLARE_SIMPLE_OP( floor, 1, BINDINGS( float( float ), vec3( vec3 ) ) );
MAGMA_DECLARE_SIMPLE_OP( ceil, 1, BINDINGS( float( float ), vec3( vec3 ) ) );
MAGMA_DECLARE_SIMPLE_OP( sqrt, 1, BINDINGS( float( float ) ) );
MAGMA_DECLARE_SIMPLE_OP( log, 1, BINDINGS( float( float ) ) );
MAGMA_DECLARE_SIMPLE_OP( add, 2, BINDINGS( float( float, float ), int( int, int ), vec3( vec3, vec3 ) ) );
MAGMA_DECLARE_SIMPLE_OP( sub, 2, BINDINGS( float( float, float ), int( int, int ), vec3( vec3, vec3 ) ) );
MAGMA_DECLARE_SIMPLE_OP( mul, 2,
                         BINDINGS( float( float, float ), int( int, int ), vec3( float, vec3 ), vec3( vec3, float ),
                                   vec3( vec3, vec3 ) ) );
MAGMA_DECLARE_SIMPLE_OP( div, 2,
                         BINDINGS( float( float, float ), int( int, int ), vec3( vec3, float ), vec3( vec3, vec3 ) ) );
MAGMA_DECLARE_SIMPLE_OP( mod, 2,
                         BINDINGS( float( float, float ), int( int, int ), vec3( vec3, float ), vec3( vec3, vec3 ) ) );
MAGMA_DECLARE_SIMPLE_OP( pow, 2,
                         BINDINGS( float( float, float ), float( float, int ), int( int, int ), vec3( vec3, float ) ) );

MAGMA_DECLARE_SIMPLE_OP( component_sum, 1, BINDINGS( float( float ), int( int ), float( vec3 ), float( quat ) ) );
MAGMA_DECLARE_SIMPLE_OP( magnitude, 1, BINDINGS( float( vec3 ) ) );
MAGMA_DECLARE_SIMPLE_OP( normalize, 1, BINDINGS( vec3( vec3 ) ) );
MAGMA_DECLARE_SIMPLE_OP( dot, 2, BINDINGS( float( vec3, vec3 ) ) );
MAGMA_DECLARE_SIMPLE_OP( cross, 2, BINDINGS( vec3( vec3, vec3 ) ) );
MAGMA_DECLARE_SIMPLE_OP( quat_mul, 2, BINDINGS( vec3( quat, vec3 ), quat( quat, quat ) ) );
MAGMA_DECLARE_SIMPLE_OP( matrix_mul, 4, BINDINGS( vec3( vec3, vec3, vec3, vec3 ) ) );
MAGMA_DECLARE_SIMPLE_OP( vectors_to_quat, 3, BINDINGS( quat( vec3, vec3, vec3 ) ) );
MAGMA_DECLARE_SIMPLE_OP( eulerangles_to_quat, 3, BINDINGS( quat( float, float, float ) ) );
MAGMA_DECLARE_SIMPLE_OP( angleaxis_to_quat, 2, BINDINGS( quat( float, vec3 ) ) );

MAGMA_DECLARE_SIMPLE_OP( cos, 1, BINDINGS( float( float ) ) );
MAGMA_DECLARE_SIMPLE_OP( acos, 1, BINDINGS( float( float ) ) );
MAGMA_DECLARE_SIMPLE_OP( sin, 1, BINDINGS( float( float ) ) );
MAGMA_DECLARE_SIMPLE_OP( asin, 1, BINDINGS( float( float ) ) );
MAGMA_DECLARE_SIMPLE_OP( tan, 1, BINDINGS( float( float ) ) );
MAGMA_DECLARE_SIMPLE_OP( atan, 1, BINDINGS( float( float ) ) );
MAGMA_DECLARE_SIMPLE_OP( atan2, 2, BINDINGS( float( float, float ) ) );

MAGMA_DECLARE_SIMPLE_OP( logical_not, 1, BINDINGS( bool( bool ) ) );

// MAGMA_DECLARE_SIMPLE_OP( less, 2, BINDINGS(bool(float,float), bool(int,int)) );
// MAGMA_DECLARE_SIMPLE_OP( lesseq, 2, BINDINGS(bool(float,float), bool(int,int)) );
// MAGMA_DECLARE_SIMPLE_OP( greater, 2, BINDINGS(bool(float,float), bool(int,int)) );
// MAGMA_DECLARE_SIMPLE_OP( greatereq, 2, BINDINGS(bool(float,float), bool(int,int)) );
// MAGMA_DECLARE_SIMPLE_OP( equal, 2, BINDINGS(bool(float,float), bool(int,int), bool(bool,bool), bool(vec3,vec3),
// bool(quat,quat)) ); MAGMA_DECLARE_SIMPLE_OP( notequal, 2, BINDINGS(bool(float,float), bool(int,int), bool(bool,bool),
// bool(vec3,vec3)/*, bool(quat,quat)*/) );

class magma_less_node : public magma_simple_operator<2> {
  public:
    typedef detail::declare_meta<2, frantic::magma::functors::less, BINDINGS( bool( float, float ), bool( int, int ) )>
        meta;
    typedef detail::declare_meta<2, frantic::magma::functors::less_tol, BINDINGS( bool( float, float ) )> meta_tol;

    MAGMA_REQUIRED_METHODS( magma_less_node );
    MAGMA_PROPERTY( useTolerance, bool );
    MAGMA_PROPERTY( toleranceExp, int );

    magma_less_node() {
        this->set_useTolerance( false );
        this->set_toleranceExp( 4 );
    }
};

class magma_lesseq_node : public magma_simple_operator<2> {
  public:
    typedef detail::declare_meta<2, frantic::magma::functors::lesseq,
                                 BINDINGS( bool( float, float ), bool( int, int ) )>
        meta;
    typedef detail::declare_meta<2, frantic::magma::functors::lesseq_tol, BINDINGS( bool( float, float ) )> meta_tol;

    MAGMA_REQUIRED_METHODS( magma_lesseq_node );
    MAGMA_PROPERTY( useTolerance, bool );
    MAGMA_PROPERTY( toleranceExp, int );

    magma_lesseq_node() {
        this->set_useTolerance( false );
        this->set_toleranceExp( 4 );
    }
};

class magma_greater_node : public magma_simple_operator<2> {
  public:
    typedef detail::declare_meta<2, frantic::magma::functors::greater,
                                 BINDINGS( bool( float, float ), bool( int, int ) )>
        meta;
    typedef detail::declare_meta<2, frantic::magma::functors::greater_tol, BINDINGS( bool( float, float ) )> meta_tol;

    MAGMA_REQUIRED_METHODS( magma_greater_node );
    MAGMA_PROPERTY( useTolerance, bool );
    MAGMA_PROPERTY( toleranceExp, int );

    magma_greater_node() {
        this->set_useTolerance( false );
        this->set_toleranceExp( 4 );
    }
};

class magma_greatereq_node : public magma_simple_operator<2> {
  public:
    typedef detail::declare_meta<2, frantic::magma::functors::greatereq,
                                 BINDINGS( bool( float, float ), bool( int, int ) )>
        meta;
    typedef detail::declare_meta<2, frantic::magma::functors::greatereq_tol, BINDINGS( bool( float, float ) )> meta_tol;

    MAGMA_REQUIRED_METHODS( magma_greatereq_node );
    MAGMA_PROPERTY( useTolerance, bool );
    MAGMA_PROPERTY( toleranceExp, int );

    magma_greatereq_node() {
        this->set_useTolerance( false );
        this->set_toleranceExp( 4 );
    }
};

class magma_equal_node : public magma_simple_operator<2> {
  public:
    typedef detail::declare_meta<2, frantic::magma::functors::equal,
                                 BINDINGS( bool( float, float ), bool( int, int ), bool( bool, bool ),
                                           bool( vec3, vec3 ), bool( quat, quat ) )>
        meta;
    typedef detail::declare_meta<2, frantic::magma::functors::equal_tol,
                                 BINDINGS( bool( float, float ), bool( vec3, vec3 ), bool( quat, quat ) )>
        meta_tol;

    MAGMA_REQUIRED_METHODS( magma_equal_node );
    MAGMA_PROPERTY( useTolerance, bool );
    MAGMA_PROPERTY( toleranceExp, int );

    magma_equal_node() {
        this->set_useTolerance( false );
        this->set_toleranceExp( 4 );
    }
};

class magma_notequal_node : public magma_simple_operator<2> {
  public:
    typedef detail::declare_meta<2, frantic::magma::functors::notequal,
                                 BINDINGS( bool( float, float ), bool( int, int ), bool( bool, bool ),
                                           bool( vec3, vec3 ) /*, bool(quat,quat)*/ )>
        meta;
    typedef detail::declare_meta<2, frantic::magma::functors::notequal_tol,
                                 BINDINGS( bool( float, float ), bool( vec3, vec3 ) /*, bool(quat,quat)*/ )>
        meta_tol;

    MAGMA_REQUIRED_METHODS( magma_notequal_node );
    MAGMA_PROPERTY( useTolerance, bool );
    MAGMA_PROPERTY( toleranceExp, int );

    magma_notequal_node() {
        this->set_useTolerance( false );
        this->set_toleranceExp( 4 );
    }
};

MAGMA_DECLARE_SIMPLE_OP( logical_and, 2, BINDINGS( bool( bool, bool ) ) );
MAGMA_DECLARE_SIMPLE_OP( logical_or, 2, BINDINGS( bool( bool, bool ) ) );
MAGMA_DECLARE_SIMPLE_OP( logical_xor, 2, BINDINGS( bool( bool, bool ) ) );
MAGMA_DECLARE_SIMPLE_OP( logicswitch, 3,
                         BINDINGS( float( float, float, bool ), int( int, int, bool ), bool( bool, bool, bool ),
                                   vec3( vec3, vec3, bool ), quat( quat, quat, bool ), float( float, float, int ),
                                   int( int, int, int ), bool( bool, bool, int ), vec3( vec3, vec3, int ),
                                   quat( quat, quat, int ) ) );

MAGMA_DECLARE_SIMPLE_OP( to_float, 1, BINDINGS( float( float ), float( int ), float( bool ) ) );
MAGMA_DECLARE_SIMPLE_OP( to_int, 1, BINDINGS( int( float ), int( int ), int( bool ) ) );
MAGMA_DECLARE_SIMPLE_OP( to_vector, 3, BINDINGS( vec3( float, float, float ) ) );

MAGMA_DECLARE_SIMPLE_OP( blend, 3, BINDINGS( float( float, float, float ), vec3( vec3, vec3, float ) ) );
MAGMA_DECLARE_SIMPLE_OP( clamp, 3,
                         BINDINGS( float( float, float, float ), int( int, int, int ), vec3( vec3, float, float ) ) );

class magma_elbow_node : public magma_simple_operator<1> {
  public:
    MAGMA_REQUIRED_METHODS( magma_elbow_node );
};

struct transform_point_meta {
    enum { ARITY = 1 };
    typedef frantic::magma::functors::transform_point type;
    typedef boost::mpl::vector<vec3( vec3 )> bindings;
};

struct transform_direction_meta {
    enum { ARITY = 1 };
    typedef frantic::magma::functors::transform_direction type;
    typedef boost::mpl::vector<vec3( vec3 )> bindings;
};

struct transform_normal_meta {
    enum { ARITY = 1 };
    typedef frantic::magma::functors::transform_normal type;
    typedef boost::mpl::vector<vec3( vec3 )> bindings;
};

class magma_to_world_node : public magma_simple_operator<1> {
  public:
    typedef transform_point_meta point_meta;
    typedef transform_direction_meta direction_meta;
    typedef transform_normal_meta normal_meta;

    MAGMA_REQUIRED_METHODS( magma_to_world_node );
    MAGMA_PROPERTY( inputType, frantic::tstring );

    magma_to_world_node();
};

class magma_from_world_node : public magma_simple_operator<1> {
  public:
    typedef transform_point_meta point_meta;
    typedef transform_direction_meta direction_meta;
    typedef transform_normal_meta normal_meta;

    MAGMA_REQUIRED_METHODS( magma_from_world_node );
    MAGMA_PROPERTY( inputType, frantic::tstring );

    magma_from_world_node();
};

class magma_to_camera_node : public magma_simple_operator<1> {
  public:
    typedef transform_point_meta point_meta;
    typedef transform_direction_meta direction_meta;
    typedef transform_normal_meta normal_meta;

    MAGMA_REQUIRED_METHODS( magma_to_camera_node );
    MAGMA_PROPERTY( inputType, frantic::tstring );

    magma_to_camera_node();
};

class magma_from_camera_node : public magma_simple_operator<1> {
  public:
    typedef transform_point_meta point_meta;
    typedef transform_direction_meta direction_meta;
    typedef transform_normal_meta normal_meta;

    MAGMA_REQUIRED_METHODS( magma_from_camera_node );
    MAGMA_PROPERTY( inputType, frantic::tstring );

    magma_from_camera_node();
};

class magma_noise_node : public magma_simple_operator<2> {
  public:
    struct meta {
        static const int ARITY = 2;
        typedef frantic::magma::functors::noise1f type;
        typedef BINDINGS( float( float, float ), float( vec3, float ) ) bindings;
    };

    MAGMA_REQUIRED_METHODS( magma_noise_node );
    MAGMA_PROPERTY( numOctaves, int );   // TODO: Replace this with an optional input
    MAGMA_PROPERTY( lacunarity, float ); // TODO: Replace this with an optional input
    MAGMA_PROPERTY( normalize, bool );

    magma_noise_node();
};

class magma_vecnoise_node : public magma_simple_operator<2> {
  public:
    struct meta {
        static const int ARITY = 2;
        typedef frantic::magma::functors::noise3f type;
        typedef BINDINGS( vec3( float, float ), vec3( vec3, float ) ) bindings;
    };

    MAGMA_REQUIRED_METHODS( magma_vecnoise_node );
    MAGMA_PROPERTY( numOctaves, int );   // TODO: Replace this with an optional input
    MAGMA_PROPERTY( lacunarity, float ); // TODO: Replace this with an optional input
    MAGMA_PROPERTY( normalize, bool );

    magma_vecnoise_node();
};

class magma_random_node : public magma_simple_operator<3> {
  public:
    struct meta {
        static const int ARITY = 3;
        typedef frantic::magma::functors::random1f type;
        typedef BINDINGS( float( int, float, float ) ) bindings;
    };
    MAGMA_REQUIRED_METHODS( magma_random_node );
};

class magma_vecrandom_node : public magma_simple_operator<3> {
  public:
    struct meta {
        static const int ARITY = 3;
        typedef frantic::magma::functors::random3f type;
        typedef BINDINGS( vec3( int, float, float ), vec3( int, vec3, float ), vec3( int, float, vec3 ),
                          vec3( int, vec3, vec3 ) ) bindings;
    };
    MAGMA_REQUIRED_METHODS( magma_vecrandom_node );
};

class magma_exponential_random_node : public magma_simple_operator<2> {
  public:
    struct meta {
        static const int ARITY = 2;
        typedef frantic::magma::functors::exponential_random1f type;
        typedef BINDINGS( float( int, float ) ) bindings;
    };
    MAGMA_REQUIRED_METHODS( magma_exponential_random_node );
};

class magma_weibull_random_node : public magma_simple_operator<3> {
  public:
    struct meta {
        static const int ARITY = 3;
        typedef frantic::magma::functors::weibull_random1f type;
        typedef BINDINGS( float( int, float, float ) ) bindings;
    };
    MAGMA_REQUIRED_METHODS( magma_weibull_random_node );
};

class magma_gaussian_random_node : public magma_simple_operator<3> {
  public:
    struct meta {
        static const int ARITY = 3;
        typedef frantic::magma::functors::gaussian_random1f type;
        typedef BINDINGS( float( int, float, float ) ) bindings;
    };
    MAGMA_REQUIRED_METHODS( magma_gaussian_random_node );
};

class magma_triangle_random_node : public magma_simple_operator<4> {
  public:
    struct meta {
        static const int ARITY = 4;
        typedef frantic::magma::functors::triangle_random1f type;
        typedef BINDINGS( float( int, float, float, float ) ) bindings;
    };
    MAGMA_REQUIRED_METHODS( magma_triangle_random_node );
};

class magma_uniform_on_sphere_random_node : public magma_simple_operator<2> {
  public:
    struct meta {
        static const int ARITY = 2;
        typedef frantic::magma::functors::uniform_sphere_random3f type;
        typedef BINDINGS( vec3( int, int ) ) bindings;
    };
    MAGMA_REQUIRED_METHODS( magma_triangle_random_node );
};

class magma_breakout_node : public magma_simple_operator<1> {
  public:
    MAGMA_REQUIRED_METHODS( magma_breakout_node )

    virtual int get_num_outputs() const { return 3; }
};

class magma_quat_to_vectors_node : public magma_simple_operator<1> {
  public:
    struct meta {
        static const int ARITY = 1;
        typedef frantic::magma::functors::quat_to_vectors type;
        typedef BINDINGS( void( void*, quat ) ) bindings;
    };

    MAGMA_REQUIRED_METHODS( magma_quat_to_vectors_node )

    virtual int get_num_outputs() const { return 3; }
};

} // namespace nodes
} // namespace magma
} // namespace frantic

#undef BINDINGS
#undef MAGMA_DECLARE_SIMPLE_OP
#undef MAGMA_END_SIMPLE_OP
#undef MAGMA_BEGIN_SIMPLE_OP
#undef MAGMA_BEGIN_SIMPLE_OP_IMPL
#undef MAGMA_HAS_CONSTRUCTOR_false
#undef MAGMA_HAS_CONSTRUCTOR_true
