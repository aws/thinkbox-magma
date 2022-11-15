// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_singleton.hpp>
#include <frantic/magma/nodes/magma_input_geometry_interface.hpp>

#include <frantic/channels/channel_map.hpp>
#include <frantic/math/eigen.hpp>

namespace frantic {
namespace magma {
namespace functors {

class in_volume {
    std::vector<magma_geometry_ptr> m_meshes;

  public:
    void set_geometry( const frantic::magma::nodes::magma_input_geometry_interface& geomInterface ) {
        geomInterface.get_all_geometry( std::back_inserter( m_meshes ) );

        for( std::vector<magma_geometry_ptr>::iterator it = m_meshes.begin(), itEnd = m_meshes.end(); it != itEnd;
             ++it )
            ( *it )->allocate_kdtree();
    }

    bool operator()( const frantic::graphics::vector3f& pos ) const throw() {
        bool result = false;

        try {
            for( std::vector<magma_geometry_ptr>::const_iterator it = m_meshes.begin(), itEnd = m_meshes.end();
                 !result && it != itEnd; ++it ) {
                if( ( *it )->in_volume( pos ) )
                    result = true;
            }
        } catch( ... ) {
            result = false;
        }

        return result;
    }
};

class nearest_point {
    std::vector<magma_geometry_ptr> m_meshes;
    frantic::channels::channel_map m_outMap; // TODO: Could be static

    struct results_holder {
        frantic::graphics::vector3f pos;
        bool valid;
        int objIndex;
        int faceIndex;
        float distance;
        frantic::graphics::vector3f faceNormal;
        frantic::graphics::vector3f baryCoords;
    };

  public:
    nearest_point() {
        m_outMap.define_channel( _T("Position"), 3, frantic::channels::data_type_float32,
                                 offsetof( results_holder, pos ) );
        m_outMap.define_channel( _T("IsValid"), 1, magma_singleton::get_named_data_type( _T("Bool") )->m_elementType,
                                 offsetof( results_holder, valid ) );
        m_outMap.define_channel( _T("ObjIndex"), 1, frantic::channels::data_type_int32,
                                 offsetof( results_holder, objIndex ) );
        m_outMap.define_channel( _T("FaceIndex"), 1, frantic::channels::data_type_int32,
                                 offsetof( results_holder, faceIndex ) );
        m_outMap.define_channel( _T("Distance"), 1, frantic::channels::data_type_float32,
                                 offsetof( results_holder, distance ) );
        m_outMap.define_channel( _T("FaceNormal"), 3, frantic::channels::data_type_float32,
                                 offsetof( results_holder, faceNormal ) );
        m_outMap.define_channel( _T("BaryCoords"), 3, frantic::channels::data_type_float32,
                                 offsetof( results_holder, baryCoords ) );
        m_outMap.end_channel_definition( 4u, true, false );
    }

    const frantic::channels::channel_map& get_output_map() const { return m_outMap; }

    void set_geometry( const frantic::magma::nodes::magma_input_geometry_interface& geomInterface ) {
        geomInterface.get_all_geometry( std::back_inserter( m_meshes ) );

        for( std::vector<magma_geometry_ptr>::iterator it = m_meshes.begin(), itEnd = m_meshes.end(); it != itEnd;
             ++it )
            ( *it )->allocate_kdtree();
    }

    bool operator()( const frantic::graphics::vector3f& pos, magma_geometry_interface::query_result& outResult,
                     bool ignoreBackfaces ) const throw() {
        bool result = false;

        // TODO: We could use the bounding boxes of the individual primitves to accelerate the determination of closest
        // primitives.
        try {
            int curPrim = 0;
            for( std::vector<magma_geometry_ptr>::const_iterator it = m_meshes.begin(), itEnd = m_meshes.end();
                 it != itEnd; ++it, ++curPrim ) {
                bool isValid = ( *it )->find_nearest_point( pos, outResult.distance, outResult, ignoreBackfaces );
                if( isValid ) {
                    result = true;
                    outResult.objIndex = curPrim;
                }
            }
        } catch( ... ) {
            result = false;
        }

        return result;
    }

    inline void operator()( void* _out, const frantic::graphics::vector3f& pos, bool ignoreBackfaces ) const throw() {
        results_holder& out = *static_cast<results_holder*>( _out );

        frantic::magma::magma_geometry_interface::query_result qr;

        qr.distance = std::numeric_limits<float>::max();

        if( operator()( pos, qr, ignoreBackfaces ) ) {
            out.pos = qr.pos;
            out.valid = true;
            out.objIndex = qr.objIndex;
            out.faceIndex = qr.faceIndex;
            out.distance = frantic::graphics::vector3f::dot( ( qr.pos - pos ), qr.normal ) <= 0 ? (float)qr.distance
                                                                                                : -(float)qr.distance;
            out.faceNormal = qr.normal;
            out.baryCoords = qr.baryCoord;
        } else {
            memset( &out, 0, sizeof( results_holder ) );
        }
    }
};

class intersect_ray {
    std::vector<magma_geometry_ptr> m_meshes;
    frantic::channels::channel_map m_outMap; // TODO Could be static

    struct results_holder {
        frantic::graphics::vector3f pos;
        bool valid;
        int objIndex;
        int faceIndex;
        float distance;
        frantic::graphics::vector3f faceNormal;
        frantic::graphics::vector3f baryCoords;
    };

  public:
    intersect_ray() {
        m_outMap.define_channel( _T("Position"), 3, frantic::channels::data_type_float32,
                                 offsetof( results_holder, pos ) );
        m_outMap.define_channel( _T("IsValid"), 1, magma_singleton::get_named_data_type( _T("Bool") )->m_elementType,
                                 offsetof( results_holder, valid ) );
        m_outMap.define_channel( _T("ObjIndex"), 1, frantic::channels::data_type_int32,
                                 offsetof( results_holder, objIndex ) );
        m_outMap.define_channel( _T("FaceIndex"), 1, frantic::channels::data_type_int32,
                                 offsetof( results_holder, faceIndex ) );
        m_outMap.define_channel( _T("Distance"), 1, frantic::channels::data_type_float32,
                                 offsetof( results_holder, distance ) );
        m_outMap.define_channel( _T("FaceNormal"), 3, frantic::channels::data_type_float32,
                                 offsetof( results_holder, faceNormal ) );
        m_outMap.define_channel( _T("BaryCoords"), 3, frantic::channels::data_type_float32,
                                 offsetof( results_holder, baryCoords ) );
        m_outMap.end_channel_definition( 4u, true, false );
    }

    const frantic::channels::channel_map& get_output_map() const { return m_outMap; }

    void set_geometry( const frantic::magma::nodes::magma_input_geometry_interface& geomInterface ) {
        geomInterface.get_all_geometry( std::back_inserter( m_meshes ) );

        for( std::vector<magma_geometry_ptr>::iterator it = m_meshes.begin(), itEnd = m_meshes.end(); it != itEnd;
             ++it )
            ( *it )->allocate_kdtree();
    }

    bool operator()( const frantic::graphics::vector3f& pos, const frantic::graphics::vector3f& dir,
                     magma_geometry_interface::query_result& outResult, bool ignoreBackfaces ) const throw() {
        bool result = false;

        // TODO: We could use the bounding boxes of the individual primitves to accelerate the determination of closest
        // primitives.
        try {
            int curPrim = 0;
            for( std::vector<magma_geometry_ptr>::const_iterator it = m_meshes.begin(), itEnd = m_meshes.end();
                 it != itEnd; ++it, ++curPrim ) {
                bool isValid = ( *it )->intersect_ray( pos, dir, outResult.distance, outResult, ignoreBackfaces );
                if( isValid ) {
                    result = true;
                    outResult.objIndex = curPrim;
                }
            }
        } catch( ... ) {
            result = false;
        }

        return result;
    }

    inline void operator()( void* _out, const frantic::graphics::vector3f& pos, const frantic::graphics::vector3f& dir,
                            bool ignoreBackfaces ) const throw() {
        results_holder& out = *static_cast<results_holder*>( _out );

        frantic::magma::magma_geometry_interface::query_result qr;

        qr.distance = std::numeric_limits<float>::max();

        if( operator()( pos, dir, qr, ignoreBackfaces ) ) {
            out.pos = qr.pos;
            out.valid = true;
            out.objIndex = qr.objIndex;
            out.faceIndex = qr.faceIndex;
            out.distance = frantic::graphics::vector3f::dot( ( qr.pos - pos ), qr.normal ) <= 0 ? (float)qr.distance
                                                                                                : -(float)qr.distance;
            out.faceNormal = qr.normal;
            out.baryCoords = qr.baryCoord;
        } else {
            memset( &out, 0, sizeof( results_holder ) );
        }
    }
};

namespace detail {
template <class TDest, class TSrc>
struct baryc_cvt_and_copy {
    inline static void apply( char* dest, frantic::geometry::mesh_channel* src, std::size_t faceIndex,
                              const float ( &weights )[3] ) {
        char* tempBuffer = (char*)alloca( src->get_element_size() );

        src->get_value( src->get_fv_index( faceIndex, 0 ), tempBuffer );
        for( std::size_t i = 0, iEnd = src->get_data_arity(); i < iEnd; ++i )
            reinterpret_cast<TDest*>( dest )[i] =
                weights[0] * static_cast<TDest>( reinterpret_cast<TSrc*>(
                                 tempBuffer )[i] ); // Not plus equals since we didn't initialize 'dest'

        src->get_value( src->get_fv_index( faceIndex, 1 ), tempBuffer );
        for( std::size_t i = 0, iEnd = src->get_data_arity(); i < iEnd; ++i )
            reinterpret_cast<TDest*>( dest )[i] +=
                weights[1] * static_cast<TDest>( reinterpret_cast<TSrc*>( tempBuffer )[i] );

        src->get_value( src->get_fv_index( faceIndex, 2 ), tempBuffer );
        for( std::size_t i = 0, iEnd = src->get_data_arity(); i < iEnd; ++i )
            reinterpret_cast<TDest*>( dest )[i] +=
                weights[2] * static_cast<TDest>( reinterpret_cast<TSrc*>( tempBuffer )[i] );
    }
};

template <class TDest, class TSrc>
struct cvt_and_copy {
    inline static void apply( char* dest, frantic::geometry::mesh_channel* src, std::size_t index ) {
        char* tempBuffer = (char*)alloca( src->get_element_size() );

        src->get_value( index, tempBuffer );
        for( std::size_t i = 0, iEnd = src->get_data_arity(); i < iEnd; ++i )
            reinterpret_cast<TDest*>( dest )[i] = static_cast<TDest>( reinterpret_cast<TSrc*>( tempBuffer )[i] );
    }

    inline static void apply( char* dest, frantic::geometry::mesh_channel* src, std::size_t faceIndex,
                              const float( & ) /*weights*/[3] ) {
        apply( dest, src, faceIndex );
    }
};

template <class TDest, class TSrc>
struct facevert_cvt_and_copy {
    inline static void apply( char* dest, frantic::geometry::mesh_channel* src, std::size_t faceIndex,
                              std::size_t fvertIndex ) {
        char* tempBuffer = (char*)alloca( src->get_element_size() );

        src->get_value( src->get_fv_index( faceIndex, fvertIndex ), tempBuffer );
        for( std::size_t i = 0, iEnd = src->get_data_arity(); i < iEnd; ++i )
            reinterpret_cast<TDest*>( dest )[i] = static_cast<TDest>( reinterpret_cast<TSrc*>( tempBuffer )[i] );
    }
};

template <class TDest, class TSrc>
struct elem_cvt_and_copy {
    inline static void apply( char* dest, frantic::geometry::mesh_channel* src, std::size_t faceIndex,
                              const float( & ) /*weights*/[3] ) {
        cvt_and_copy<TDest, TSrc>::apply( dest, src, src->get_fv_index( faceIndex, 0 ) );
    }
};

struct face_query_traits {
    typedef void ( *impl_fn_t )( char*, frantic::geometry::mesh_channel*, std::size_t, const float ( & )[3] );

    inline static bool is_valid_channel( frantic::geometry::mesh_channel* /*ch*/ ) { return true; }

    inline static impl_fn_t get_impl_fn( const frantic::magma::magma_data_type& outType,
                                         frantic::geometry::mesh_channel* srcCh ) {
        if( frantic::geometry::mesh_channel::is_stored_at_vertex( srcCh->get_channel_type() ) ) {
            if( !frantic::channels::is_channel_data_type_float( outType.m_elementType ) )
                throw frantic::magma::magma_exception() << frantic::magma::magma_exception::error_name(
                    _T("Cannot interpolate an integer channel: \"") + srcCh->get_name() + _T("\"") );

            switch( srcCh->get_data_type() ) {
            case frantic::channels::data_type_float16:
                return &detail::baryc_cvt_and_copy<float, half>::apply;
            case frantic::channels::data_type_float32:
                return &detail::baryc_cvt_and_copy<float, float>::apply;
            case frantic::channels::data_type_float64:
                return &detail::baryc_cvt_and_copy<float, double>::apply;
            };
        } else if( srcCh->get_channel_type() == frantic::geometry::mesh_channel::element ) {
            switch( outType.m_elementType ) {
            case frantic::channels::data_type_float32:
                switch( srcCh->get_data_type() ) {
                case frantic::channels::data_type_float16:
                    return &detail::elem_cvt_and_copy<float, half>::apply;
                case frantic::channels::data_type_float32:
                    return &detail::elem_cvt_and_copy<float, float>::apply;
                case frantic::channels::data_type_float64:
                    return &detail::elem_cvt_and_copy<float, double>::apply;
                }
                break;
            case frantic::channels::data_type_int32:
                switch( srcCh->get_data_type() ) {
                case frantic::channels::data_type_int8:
                    return &detail::elem_cvt_and_copy<int, char>::apply;
                case frantic::channels::data_type_int16:
                    return &detail::elem_cvt_and_copy<int, short>::apply;
                case frantic::channels::data_type_int32:
                    return &detail::elem_cvt_and_copy<int, int>::apply;
                case frantic::channels::data_type_int64:
                    return &detail::elem_cvt_and_copy<int, long long>::apply;
                }
                break;
            }
        } else { // if( ch->get_channel_type() == frantic::geometry::mesh_channel::face )
            switch( outType.m_elementType ) {
            case frantic::channels::data_type_float32:
                switch( srcCh->get_data_type() ) {
                case frantic::channels::data_type_float16:
                    return &detail::cvt_and_copy<float, half>::apply;
                case frantic::channels::data_type_float32:
                    return &detail::cvt_and_copy<float, float>::apply;
                case frantic::channels::data_type_float64:
                    return &detail::cvt_and_copy<float, double>::apply;
                }
                break;
            case frantic::channels::data_type_int32:
                switch( srcCh->get_data_type() ) {
                case frantic::channels::data_type_int8:
                    return &detail::cvt_and_copy<int, char>::apply;
                case frantic::channels::data_type_int16:
                    return &detail::cvt_and_copy<int, short>::apply;
                case frantic::channels::data_type_int32:
                    return &detail::cvt_and_copy<int, int>::apply;
                case frantic::channels::data_type_int64:
                    return &detail::cvt_and_copy<int, long long>::apply;
                }
                break;
            }
        }

        return NULL;
    }
};

struct face_only_query_traits {
    typedef void ( *impl_fn_t )( char*, frantic::geometry::mesh_channel*, std::size_t );

    inline static bool is_valid_channel( frantic::geometry::mesh_channel* ch ) {
        return ch->get_channel_type() == frantic::geometry::mesh_channel::face;
    }

    inline static impl_fn_t get_impl_fn( const frantic::magma::magma_data_type& outType,
                                         frantic::geometry::mesh_channel* srcCh ) {
        if( srcCh->get_channel_type() == frantic::geometry::mesh_channel::face ) {
            switch( outType.m_elementType ) {
            case frantic::channels::data_type_float32:
                switch( srcCh->get_data_type() ) {
                case frantic::channels::data_type_float16:
                    return &detail::cvt_and_copy<float, half>::apply;
                case frantic::channels::data_type_float32:
                    return &detail::cvt_and_copy<float, float>::apply;
                case frantic::channels::data_type_float64:
                    return &detail::cvt_and_copy<float, double>::apply;
                }
                break;
            case frantic::channels::data_type_int32:
                switch( srcCh->get_data_type() ) {
                case frantic::channels::data_type_int8:
                    return &detail::cvt_and_copy<int, char>::apply;
                case frantic::channels::data_type_int16:
                    return &detail::cvt_and_copy<int, short>::apply;
                case frantic::channels::data_type_int32:
                    return &detail::cvt_and_copy<int, int>::apply;
                case frantic::channels::data_type_int64:
                    return &detail::cvt_and_copy<int, long long>::apply;
                }
                break;
            }
        }

        return NULL;
    }
};

struct face_vertex_query_traits {
    typedef void ( *impl_fn_t )( char*, frantic::geometry::mesh_channel*, std::size_t, std::size_t );

    inline static bool is_valid_channel( frantic::geometry::mesh_channel* /*ch*/ ) { return true; }

    inline static impl_fn_t get_impl_fn( const frantic::magma::magma_data_type& outType,
                                         frantic::geometry::mesh_channel* srcCh ) {
        switch( outType.m_elementType ) {
        case frantic::channels::data_type_float32:
            switch( srcCh->get_data_type() ) {
            case frantic::channels::data_type_float16:
                return &detail::facevert_cvt_and_copy<float, half>::apply;
            case frantic::channels::data_type_float32:
                return &detail::facevert_cvt_and_copy<float, float>::apply;
            case frantic::channels::data_type_float64:
                return &detail::facevert_cvt_and_copy<float, double>::apply;
            };
            break;
        case frantic::channels::data_type_int32:
            switch( srcCh->get_data_type() ) {
            case frantic::channels::data_type_int8:
                return &detail::facevert_cvt_and_copy<int, char>::apply;
            case frantic::channels::data_type_int16:
                return &detail::facevert_cvt_and_copy<int, short>::apply;
            case frantic::channels::data_type_int32:
                return &detail::facevert_cvt_and_copy<int, int>::apply;
            case frantic::channels::data_type_int64:
                return &detail::facevert_cvt_and_copy<int, long long>::apply;
            }
            break;
        };

        return NULL;
    }
};

struct vertex_query_traits {
    typedef void ( *impl_fn_t )( char*, frantic::geometry::mesh_channel*, std::size_t );

    inline static bool is_valid_channel( frantic::geometry::mesh_channel* ch ) {
        return ch->get_channel_type() == frantic::geometry::mesh_channel::vertex;
    }

    inline static impl_fn_t get_impl_fn( const frantic::magma::magma_data_type& outType,
                                         frantic::geometry::mesh_channel* srcCh ) {
        switch( outType.m_elementType ) {
        case frantic::channels::data_type_float32:
            switch( srcCh->get_data_type() ) {
            case frantic::channels::data_type_float16:
                return &detail::cvt_and_copy<float, half>::apply;
            case frantic::channels::data_type_float32:
                return &detail::cvt_and_copy<float, float>::apply;
            case frantic::channels::data_type_float64:
                return &detail::cvt_and_copy<float, double>::apply;
            }
            break;
        case frantic::channels::data_type_int32:
            switch( srcCh->get_data_type() ) {
            case frantic::channels::data_type_int8:
                return &detail::cvt_and_copy<int, char>::apply;
            case frantic::channels::data_type_int16:
                return &detail::cvt_and_copy<int, short>::apply;
            case frantic::channels::data_type_int32:
                return &detail::cvt_and_copy<int, int>::apply;
            case frantic::channels::data_type_int64:
                return &detail::cvt_and_copy<int, long long>::apply;
            }
            break;
        }

        return NULL;
    }
};

struct element_query_traits {
    typedef void ( *impl_fn_t )( char*, frantic::geometry::mesh_channel*, std::size_t );

    inline static bool is_valid_channel( frantic::geometry::mesh_channel* ch ) {
        return ch->get_channel_type() == frantic::geometry::mesh_channel::element;
    }

    inline static impl_fn_t get_impl_fn( const frantic::magma::magma_data_type& outType,
                                         frantic::geometry::mesh_channel* srcCh ) {
        switch( outType.m_elementType ) {
        case frantic::channels::data_type_float32:
            switch( srcCh->get_data_type() ) {
            case frantic::channels::data_type_float16:
                return &detail::cvt_and_copy<float, half>::apply;
            case frantic::channels::data_type_float32:
                return &detail::cvt_and_copy<float, float>::apply;
            case frantic::channels::data_type_float64:
                return &detail::cvt_and_copy<float, double>::apply;
            }
            break;
        case frantic::channels::data_type_int32:
            switch( srcCh->get_data_type() ) {
            case frantic::channels::data_type_int8:
                return &detail::cvt_and_copy<int, char>::apply;
            case frantic::channels::data_type_int16:
                return &detail::cvt_and_copy<int, short>::apply;
            case frantic::channels::data_type_int32:
                return &detail::cvt_and_copy<int, int>::apply;
            case frantic::channels::data_type_int64:
                return &detail::cvt_and_copy<int, long long>::apply;
            }
            break;
        }

        return NULL;
    }
};

template <class Type>
struct transform_point {
    inline static void apply( void* pData, const frantic::graphics::transform4f& tm ) {
        Type& p = *static_cast<Type*>( pData );
        p = tm * p;
    }
};

template <class Type>
struct transform_vector {
    inline static void apply( void* pData, const frantic::graphics::transform4f& tm ) {
        Type& p = *static_cast<Type*>( pData );
        p = tm.transform_no_translation( p );
    }
};

template <class Type>
struct transform_normal {
    inline static void apply( void* pData, const frantic::graphics::transform4f& tm ) {
        Type& p = *static_cast<Type*>( pData );
        p = tm.transpose_transform_no_translation( p );
    }
};

template <class Type>
struct transform_scalar {
    inline static void apply( void* pData, const frantic::graphics::transform4f& tm ) {
        *static_cast<Type*>( pData ) *=
            tm.get( 0, 0 ); // Arbitrarily use the [0,0] element to store the required scale.
    }
};

template <class Traits>
class mesh_query_functor {
  protected:
    typedef typename Traits::impl_fn_t impl_fn_t;

    struct channel_accessor {
        frantic::channels::channel_general_accessor outAccessor;
        frantic::geometry::mesh_channel* srcChannel;
        impl_fn_t implFn;

        frantic::graphics::transform4f toWorldTM; // For 'point', 'vector' types this is the normal transform. For
                                                  // 'normal' types this is the inverse.
        void ( *transformFn )( void*, const frantic::graphics::transform4f& );
    };

    typedef std::vector<channel_accessor> channel_list;
    typedef std::vector<std::pair<magma_geometry_ptr, channel_list>> mesh_list;

    mesh_list m_channels;

    frantic::channels::channel_map m_outMap;

  protected:
    inline void init_transform( channel_accessor& ch, const frantic::graphics::transform4f& toWorldTM ) {
        switch( ch.srcChannel->get_transform_type() ) {
        case frantic::geometry::mesh_channel::transform_type::point:
            ch.toWorldTM = toWorldTM;
            ch.transformFn = &transform_point<frantic::graphics::vector3f>::apply;
            break;
        case frantic::geometry::mesh_channel::transform_type::vector:
            ch.toWorldTM = toWorldTM;
            ch.transformFn = &transform_vector<frantic::graphics::vector3f>::apply;
            break;
        case frantic::geometry::mesh_channel::transform_type::normal:
            ch.toWorldTM = toWorldTM.to_inverse();
            ch.transformFn = &transform_normal<frantic::graphics::vector3f>::apply;
            break;
        case frantic::geometry::mesh_channel::transform_type::scale: {
            frantic::graphics::transform4f outPersp, outStretch, outRotate;
            frantic::graphics::vector3f outTrans, eigenvalues;

            toWorldTM.decompose( outPersp, outTrans, outRotate, outStretch );

            float stretchUpperTriangle[6] = { outStretch.get( 0, 0 ), outStretch.get( 1, 0 ), outStretch.get( 2, 0 ),
                                              outStretch.get( 1, 1 ), outStretch.get( 2, 1 ), outStretch.get( 2, 2 ) };

            frantic::math::linearalgebra::get_eigenvalues_symmetric_3x3( stretchUpperTriangle, eigenvalues[0],
                                                                         eigenvalues[1], eigenvalues[2] );

            ch.toWorldTM = frantic::graphics::transform4f::from_scale( eigenvalues.max_abs_component() );
            ch.transformFn = &transform_scalar<float>::apply;
        } break;
        default:
            ch.toWorldTM.set_to_identity();
            ch.transformFn = NULL;
            break;
        }
    }

  public:
    mesh_query_functor() { m_outMap.end_channel_definition(); }

    const frantic::channels::channel_map& get_output_map() const { return m_outMap; }

    void set_geometry( const frantic::magma::nodes::magma_input_geometry_interface& geom ) {
        m_channels.resize( geom.size() );

        std::size_t i = 0;
        for( typename mesh_list::iterator it = m_channels.begin(), itEnd = m_channels.end(); it != itEnd; ++it, ++i )
            it->first = geom.get_geometry( i );
    }

    void set_geometry( const std::vector<magma_geometry_ptr>& geom ) {
        m_channels.resize( geom.size() );

        typename mesh_list::iterator itDest = m_channels.begin();
        for( std::vector<magma_geometry_ptr>::const_iterator it = geom.begin(), itEnd = geom.end(); it != itEnd;
             ++it, ++itDest )
            itDest->first = *it;
    }

    void add_channel( const frantic::tstring& channelName ) {
        if( m_channels.empty() )
            return;

        frantic::geometry::mesh_interface& theMesh = m_channels.front().first->get_mesh();

        frantic::geometry::mesh_channel* ch;
        if( theMesh.request_channel( channelName, true, false, false ) )
            ch = const_cast<frantic::geometry::mesh_channel*>(
                theMesh.get_vertex_channels().get_channel( channelName ) );
        else if( theMesh.request_channel( channelName, false, false, false ) )
            ch = const_cast<frantic::geometry::mesh_channel*>( theMesh.get_face_channels().get_channel( channelName ) );
        else
            throw magma_exception() << magma_exception::error_name(
                _T("The InputGeometry #0 has no channel named: \"") + channelName + _T("\"") );

        if( !Traits::is_valid_channel( ch ) )
            throw magma_exception();

        frantic::magma::magma_data_type chType;
        chType.m_elementType = ch->get_data_type();
        chType.m_elementCount = ch->get_data_arity();

        if( frantic::channels::is_channel_data_type_float( chType.m_elementType ) )
            chType.m_elementType = frantic::channels::data_type_float32;
        else if( frantic::channels::is_channel_data_type_int( chType.m_elementType ) )
            chType.m_elementType = frantic::channels::data_type_int32;
        else
            throw magma_exception() << magma_exception::found_type( chType )
                                    << magma_exception::error_name( _T("Channel: \"") + channelName +
                                                                    _T("\" has invalid type") );

        m_outMap.append_channel( channelName, chType.m_elementCount, chType.m_elementType );

        frantic::channels::channel_general_accessor chAcc = m_outMap.get_general_accessor( channelName );

        std::size_t counter = 0;
        for( typename mesh_list::iterator it = m_channels.begin(), itEnd = m_channels.end(); it != itEnd;
             ++it, ++counter ) {
            frantic::geometry::mesh_interface& curMesh = it->first->get_mesh();
            frantic::geometry::mesh_channel* curCh;

            if( frantic::geometry::mesh_channel::is_stored_at_vertex( ch->get_channel_type() ) ) {
                if( !curMesh.request_channel( channelName, true, false, false ) )
                    throw magma_exception() << magma_exception::error_name(
                        _T("The InputGeometry #") + boost::lexical_cast<frantic::tstring>( counter ) +
                        _T(" has no vertex channel named: \"") + channelName + _T("\"") );
                curCh = const_cast<frantic::geometry::mesh_channel*>(
                    curMesh.get_vertex_channels().get_channel( channelName ) );
            } else {
                if( !curMesh.request_channel( channelName, false, false, false ) )
                    throw magma_exception() << magma_exception::error_name(
                        _T("The InputGeometry #") + boost::lexical_cast<frantic::tstring>( counter ) +
                        _T(" has no face channel named: \"") + channelName + _T("\"") );
                curCh = const_cast<frantic::geometry::mesh_channel*>(
                    curMesh.get_face_channels().get_channel( channelName ) );
            }

            frantic::magma::magma_data_type curType;
            curType.m_elementType = curCh->get_data_type();
            curType.m_elementCount = curCh->get_data_arity();

            if( frantic::channels::is_channel_data_type_float( curType.m_elementType ) )
                curType.m_elementType = frantic::channels::data_type_float32;
            else if( frantic::channels::is_channel_data_type_int( curType.m_elementType ) )
                curType.m_elementType = frantic::channels::data_type_int32;

            if( curCh->get_channel_type() != ch->get_channel_type() )
                throw magma_exception() << magma_exception::error_name(
                    _T("The InputGeometry #") + boost::lexical_cast<frantic::tstring>( counter ) +
                    _T(" did not have the same type of channel named: \"") + channelName + _T("\" as #0") );

            if( curType.m_elementType != chType.m_elementType || curType.m_elementCount != chType.m_elementCount )
                throw magma_exception() << magma_exception::found_type( curType )
                                        << magma_exception::expected_type( chType )
                                        << magma_exception::error_name(
                                               _T("The InputGeometry #") +
                                               boost::lexical_cast<frantic::tstring>( counter ) +
                                               _T(" did not have the same data type for channel named: \"") +
                                               channelName + _T("\" as #0") );

            channel_accessor result;
            result.outAccessor = chAcc;
            result.srcChannel = curCh;
            result.implFn = Traits::get_impl_fn( chType, curCh );

            init_transform( result, it->first->get_toworld_transform() );

            if( !result.implFn )
                THROW_MAGMA_INTERNAL_ERROR();

            it->second.push_back( result );
        }
    }
};
} // namespace detail

class face_query : public detail::mesh_query_functor<detail::face_query_traits> {
  public:
    void operator()( void* out, int _objIndex, int _faceIndex, const frantic::graphics::vector3f& baryCoords ) const {
        std::size_t objIndex = static_cast<std::size_t>( _objIndex );
        std::size_t faceIndex = static_cast<std::size_t>( _faceIndex );

        if( objIndex < m_channels.size() ) {
            mesh_list::const_reference meshData = m_channels[objIndex];

            if( faceIndex < meshData.first->get_mesh().get_num_faces() ) {
                for( channel_list::const_iterator it = meshData.second.begin(), itEnd = meshData.second.end();
                     it != itEnd; ++it ) {
                    it->implFn( (char*)it->outAccessor.get_channel_data_pointer( (char*)out ), it->srcChannel,
                                faceIndex, *( ( float( * )[3] ) & baryCoords.x ) );
                    if( it->transformFn )
                        it->transformFn( it->outAccessor.get_channel_data_pointer( (char*)out ), it->toWorldTM );
                }
            } else {
                memset( out, 0, m_outMap.structure_size() );
            }
        } else {
            memset( out, 0, m_outMap.structure_size() );
        }
    }
};

class face_only_query : public detail::mesh_query_functor<detail::face_only_query_traits> {
  public:
    void operator()( void* out, int _objIndex, int _faceIndex ) const {
        std::size_t objIndex = static_cast<std::size_t>( _objIndex );
        std::size_t faceIndex = static_cast<std::size_t>( _faceIndex );

        if( objIndex < m_channels.size() ) {
            mesh_list::const_reference meshData = m_channels[objIndex];

            if( faceIndex < meshData.first->get_mesh().get_num_faces() ) {
                for( channel_list::const_iterator it = meshData.second.begin(), itEnd = meshData.second.end();
                     it != itEnd; ++it ) {
                    it->implFn( (char*)it->outAccessor.get_channel_data_pointer( (char*)out ), it->srcChannel,
                                faceIndex );
                    if( it->transformFn )
                        it->transformFn( it->outAccessor.get_channel_data_pointer( (char*)out ), it->toWorldTM );
                }
            } else {
                memset( out, 0, m_outMap.structure_size() );
            }
        } else {
            memset( out, 0, m_outMap.structure_size() );
        }
    }
};

class face_vertex_query : public detail::mesh_query_functor<detail::face_vertex_query_traits> {
  public:
    void operator()( void* out, int _objIndex, int _faceIndex, int _fvertIndex ) const {
        std::size_t objIndex = static_cast<std::size_t>( _objIndex );
        std::size_t faceIndex = static_cast<std::size_t>( _faceIndex );
        std::size_t fvertIndex = static_cast<std::size_t>( _fvertIndex );

        if( objIndex < m_channels.size() ) {
            mesh_list::const_reference meshData = m_channels[objIndex];

            if( faceIndex < meshData.first->get_mesh().get_num_faces() ) {
                for( channel_list::const_iterator it = meshData.second.begin(), itEnd = meshData.second.end();
                     it != itEnd; ++it ) {
                    it->implFn( (char*)it->outAccessor.get_channel_data_pointer( (char*)out ), it->srcChannel,
                                faceIndex, fvertIndex );
                    if( it->transformFn )
                        it->transformFn( it->outAccessor.get_channel_data_pointer( (char*)out ), it->toWorldTM );
                }
            } else {
                memset( out, 0, m_outMap.structure_size() );
            }
        } else {
            memset( out, 0, m_outMap.structure_size() );
        }
    }
};

class vertex_query : public detail::mesh_query_functor<detail::vertex_query_traits> {
  public:
    void operator()( void* out, int _objIndex, int _vertIndex ) const {
        std::size_t objIndex = static_cast<std::size_t>( _objIndex );
        std::size_t vertIndex = static_cast<std::size_t>( _vertIndex );

        if( objIndex < m_channels.size() ) {
            mesh_list::const_reference meshData = m_channels[objIndex];

            if( vertIndex < meshData.first->get_mesh().get_num_verts() ) {
                for( channel_list::const_iterator it = meshData.second.begin(), itEnd = meshData.second.end();
                     it != itEnd; ++it ) {
                    it->implFn( (char*)it->outAccessor.get_channel_data_pointer( (char*)out ), it->srcChannel,
                                vertIndex );
                    if( it->transformFn )
                        it->transformFn( it->outAccessor.get_channel_data_pointer( (char*)out ), it->toWorldTM );
                }
            } else {
                memset( out, 0, m_outMap.structure_size() );
            }
        } else {
            memset( out, 0, m_outMap.structure_size() );
        }
    }
};

class element_query : public detail::mesh_query_functor<detail::element_query_traits> {
  public:
    void operator()( void* out, int _objIndex, int _elemIndex ) const {
        std::size_t objIndex = static_cast<std::size_t>( _objIndex );
        std::size_t elemIndex = static_cast<std::size_t>( _elemIndex );

        if( objIndex < m_channels.size() ) {
            mesh_list::const_reference meshData = m_channels[objIndex];

            if( elemIndex < meshData.first->get_mesh().get_num_elements() ) {
                for( channel_list::const_iterator it = meshData.second.begin(), itEnd = meshData.second.end();
                     it != itEnd; ++it ) {
                    it->implFn( (char*)it->outAccessor.get_channel_data_pointer( (char*)out ), it->srcChannel,
                                elemIndex );
                    if( it->transformFn )
                        it->transformFn( it->outAccessor.get_channel_data_pointer( (char*)out ), it->toWorldTM );
                }
            } else {
                memset( out, 0, m_outMap.structure_size() );
            }
        } else {
            memset( out, 0, m_outMap.structure_size() );
        }
    }
};

class mesh_query {
    std::vector<magma_geometry_ptr> m_geom;
    frantic::channels::channel_map m_outMap; // TODO Could be static

  public:
    struct result_type {
        int numFaces, numVerts, numElements;
    };

  public:
    mesh_query() {
        m_outMap.define_channel( _T("NumFaces"), 1, frantic::channels::data_type_int32,
                                 offsetof( result_type, numFaces ) );
        m_outMap.define_channel( _T("NumVerts"), 1, frantic::channels::data_type_int32,
                                 offsetof( result_type, numVerts ) );
        m_outMap.define_channel( _T("NumElements"), 1, frantic::channels::data_type_int32,
                                 offsetof( result_type, numElements ) );
        m_outMap.end_channel_definition( 4u, true, false );
    }

    const frantic::channels::channel_map& get_output_map() const { return m_outMap; }

    void set_geometry( const frantic::magma::nodes::magma_input_geometry_interface& geom ) {
        geom.get_all_geometry( std::back_inserter( m_geom ) );
    }

    void operator()( void* _out, int _objIndex ) const {
        result_type& out = *static_cast<result_type*>( _out );
        std::size_t objIndex = static_cast<std::size_t>( _objIndex );

        if( objIndex < m_geom.size() ) {
            frantic::geometry::mesh_interface& theMesh = m_geom[objIndex]->get_mesh();
            out.numFaces = static_cast<int>( theMesh.get_num_faces() );
            out.numVerts = static_cast<int>( theMesh.get_num_verts() );
            out.numElements = static_cast<int>( theMesh.get_num_elements() );
        } else {
            memset( &out, 0, sizeof( result_type ) );
        }
    }
};

} // namespace functors
} // namespace magma
} // namespace frantic
