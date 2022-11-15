// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_exception.hpp>
#include <frantic/magma/nodes/magma_input_geometry_interface.hpp>
#include <frantic/magma/nodes/magma_node_property.hpp>
#include <frantic/magma/nodes/magma_simple_operator.hpp>

#include <frantic/channels/channel_map.hpp>

#include <boost/mpl/vector.hpp>

#include <vector>

namespace frantic {
namespace magma {
namespace functors {

class face_query;
class vertex_query;
class element_query;
class mesh_query;

} // namespace functors
} // namespace magma
} // namespace frantic

namespace frantic {
namespace magma {
namespace nodes {

class magma_face_query_node : public magma_simple_operator<4> {
    MAGMA_PROPERTY( exposePosition, bool )
    MAGMA_PROPERTY( channels, std::vector<frantic::tstring> )

  public:
    class functor;

    struct meta {
        enum { ARITY = 3 };
        typedef frantic::magma::functors::face_query type;
        typedef boost::mpl::vector<void( void*, int, int, vec3 )> bindings;
    };

    static void create_type_definition( magma_node_type& outType );

    virtual void compile( magma_compiler_interface& compile );

    magma_face_query_node();

    virtual int get_num_outputs() const;

    virtual void get_output_description( int index, frantic::tstring& outDescription ) const;
};

class magma_vertex_query_node : public magma_simple_operator<3> {
    MAGMA_PROPERTY( exposePosition, bool )
    MAGMA_PROPERTY( channels, std::vector<frantic::tstring> )

  public:
    class functor;

    struct meta {
        enum { ARITY = 2 };
        typedef frantic::magma::functors::vertex_query type;
        typedef boost::mpl::vector<void( void*, int, int )> bindings;
    };

    static void create_type_definition( magma_node_type& outType );

    virtual void compile( magma_compiler_interface& compile );

    magma_vertex_query_node();

    virtual int get_num_outputs() const;

    virtual void get_output_description( int index, frantic::tstring& outDescription ) const;
};

class magma_element_query_node : public magma_simple_operator<3> {
  public:
    class functor;

    struct meta {
        enum { ARITY = 2 };
        typedef frantic::magma::functors::element_query type;
        typedef boost::mpl::vector<void( void*, int, int )> bindings;
    };

    static void create_type_definition( magma_node_type& outType );

    virtual void compile( magma_compiler_interface& compile );

    virtual int get_num_outputs() const;
};

class magma_mesh_query_node : public magma_simple_operator<2> {
  public:
    class functor;

    struct meta {
        enum { ARITY = 1 };
        typedef frantic::magma::functors::mesh_query type;
        typedef boost::mpl::vector<void( void*, int )> bindings;
    };

    static void create_type_definition( magma_node_type& outType );

    virtual void compile( magma_compiler_interface& compile );

    virtual int get_num_outputs() const;

    // virtual void get_output_description( int index, std::string& outDescription ) const;
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
struct elem_cvt_and_copy {
    inline static void apply( char* dest, frantic::geometry::mesh_channel* src, std::size_t faceIndex,
                              const float( & ) /*weights*/[3] ) {
        cvt_and_copy<TDest, TSrc>::apply( dest, src, src->get_fv_index( faceIndex, 0 ) );
    }
};
} // namespace detail

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

template <class Traits>
class mesh_query_functor {
  protected:
    typedef typename Traits::impl_fn_t impl_fn_t;

    struct channel_accessor {
        frantic::channels::channel_general_accessor outAccessor;
        frantic::geometry::mesh_channel* srcChannel;
        impl_fn_t implFn;
    };

    typedef std::vector<channel_accessor> channel_list;
    typedef std::vector<std::pair<magma_geometry_ptr, channel_list>> mesh_list;

    mesh_list m_channels;

    frantic::channels::channel_map m_outMap;

  public:
    mesh_query_functor() { m_outMap.end_channel_definition(); }

    void set_geometry( magma_input_geometry_interface& geom ) {
        m_channels.resize( geom.size() );

        std::size_t i = 0;
        for( typename mesh_list::iterator it = m_channels.begin(), itEnd = m_channels.end(); it != itEnd; ++it, ++i )
            it->first = geom.get_geometry( i );
    }

    void add_channel( const frantic::tstring& channelName ) {
        if( m_channels.empty() )
            return;

        frantic::geometry::mesh_interface& theMesh = m_channels.front().first->get_mesh();

        frantic::geometry::mesh_channel* ch;
        if( theMesh.request_channel( channelName, true, false, false ) )
            ch = const_cast<frantic::geometry::mesh_channel*>(
                theMesh.get_vertex_channels().get_channel( channelName ) );
        else if( !theMesh.request_channel( channelName, false, false, false ) )
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

            if( !result.implFn )
                THROW_MAGMA_INTERNAL_ERROR();

            it->second.push_back( result );
        }
    }
};

class magma_face_query_node::functor : private mesh_query_functor<face_query_traits> {
  public:
    typedef void* return_type;
    typedef boost::mpl::vector<int, int, frantic::graphics::vector3f> param_types;

  public:
    void calculate_result_layout( frantic::channels::channel_map& map ) const { map = m_outMap; }

    void set_geometry( magma_input_geometry_interface& geom ) {
        mesh_query_functor<face_query_traits>::set_geometry( geom );
    }

    void add_channel( const frantic::tstring& channelName ) {
        mesh_query_functor<face_query_traits>::add_channel( channelName );
    }

    void operator()( void* out, int _objIndex, int _faceIndex, const frantic::graphics::vector3f& baryCoords ) const {
        std::size_t objIndex = static_cast<std::size_t>( _objIndex );
        std::size_t faceIndex = static_cast<std::size_t>( _faceIndex );

        if( objIndex < m_channels.size() ) {
            mesh_list::const_reference meshData = m_channels[objIndex];

            if( faceIndex < meshData.first->get_mesh().get_num_faces() ) {
                for( channel_list::const_iterator it = meshData.second.begin(), itEnd = meshData.second.end();
                     it != itEnd; ++it )
                    it->implFn( (char*)it->outAccessor.get_channel_data_pointer( (char*)out ), it->srcChannel,
                                faceIndex, *( ( float( * )[3] ) & baryCoords.x ) );
            } else {
                memset( out, 0, m_outMap.structure_size() );
            }
        } else {
            memset( out, 0, m_outMap.structure_size() );
        }
    }
};

class magma_vertex_query_node::functor : private mesh_query_functor<vertex_query_traits> {
  public:
    typedef void* return_type;
    typedef boost::mpl::vector<int, int> param_types;

  public:
    void calculate_result_layout( frantic::channels::channel_map& map ) const { map = m_outMap; }

    void set_geometry( magma_input_geometry_interface& geom ) {
        mesh_query_functor<vertex_query_traits>::set_geometry( geom );
    }

    void add_channel( const frantic::tstring& channelName ) {
        mesh_query_functor<vertex_query_traits>::add_channel( channelName );
    }

    void operator()( void* out, int _objIndex, int _vertIndex ) const {
        std::size_t objIndex = static_cast<std::size_t>( _objIndex );
        std::size_t vertIndex = static_cast<std::size_t>( _vertIndex );

        if( objIndex < m_channels.size() ) {
            mesh_list::const_reference meshData = m_channels[objIndex];

            if( vertIndex < meshData.first->get_mesh().get_num_verts() ) {
                for( channel_list::const_iterator it = meshData.second.begin(), itEnd = meshData.second.end();
                     it != itEnd; ++it )
                    it->implFn( (char*)it->outAccessor.get_channel_data_pointer( (char*)out ), it->srcChannel,
                                vertIndex );
            } else {
                memset( out, 0, m_outMap.structure_size() );
            }
        } else {
            memset( out, 0, m_outMap.structure_size() );
        }
    }
};

class magma_element_query_node::functor : private mesh_query_functor<element_query_traits> {
  public:
    typedef void* return_type;
    typedef boost::mpl::vector<int, int> param_types;

  public:
    void calculate_result_layout( frantic::channels::channel_map& map ) const { map = m_outMap; }

    void set_geometry( magma_input_geometry_interface& geom ) {
        mesh_query_functor<element_query_traits>::set_geometry( geom );
    }

    void add_channel( const frantic::tstring& channelName ) {
        mesh_query_functor<element_query_traits>::add_channel( channelName );
    }

    void operator()( void* out, int _objIndex, int _elemIndex ) const {
        std::size_t objIndex = static_cast<std::size_t>( _objIndex );
        std::size_t elemIndex = static_cast<std::size_t>( _elemIndex );

        if( objIndex < m_channels.size() ) {
            mesh_list::const_reference meshData = m_channels[objIndex];

            if( elemIndex < meshData.first->get_mesh().get_num_elements() ) {
                for( channel_list::const_iterator it = meshData.second.begin(), itEnd = meshData.second.end();
                     it != itEnd; ++it )
                    it->implFn( (char*)it->outAccessor.get_channel_data_pointer( (char*)out ), it->srcChannel,
                                elemIndex );
            } else {
                memset( out, 0, m_outMap.structure_size() );
            }
        } else {
            memset( out, 0, m_outMap.structure_size() );
        }
    }
};

class magma_mesh_query_node::functor {
    std::vector<magma_geometry_ptr> m_geom;

  public:
    typedef void* return_type;
    typedef boost::mpl::vector<int> param_types;

    struct result_type {
        int numFaces, numVerts, numElements;
    };

  public:
    void calculate_result_layout( frantic::channels::channel_map& map ) const {
        map.reset();
        map.define_channel( _T("FaceCount"), 1, frantic::channels::data_type_int32, offsetof( result_type, numFaces ) );
        map.define_channel( _T("VertexCount"), 1, frantic::channels::data_type_int32,
                            offsetof( result_type, numVerts ) );
        map.define_channel( _T("ElementCount"), 1, frantic::channels::data_type_int32,
                            offsetof( result_type, numElements ) );
        map.end_channel_definition( 4u, true, false );
    }

    void set_geometry( magma_input_geometry_interface& geom ) { geom.get_all_geometry( std::back_inserter( m_geom ) ); }

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

} // namespace nodes
} // namespace magma
} // namespace frantic
