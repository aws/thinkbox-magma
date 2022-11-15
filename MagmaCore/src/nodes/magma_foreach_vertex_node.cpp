// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/functors/geometry.hpp>
#include <frantic/magma/nodes/magma_foreach_vertex_node.hpp>
#include <frantic/magma/nodes/magma_input_geometry_interface.hpp>
#include <frantic/magma/nodes/magma_node_impl.hpp>
#include <frantic/magma/simple_compiler/base_compiler_impl.hpp>

#include <boost/move/move.hpp>

#include <memory>
#include <utility>

namespace frantic {
namespace magma {
namespace nodes {

// MAGMA_DEFINE_TYPE( "VertexLoop", "System", magma_foreach_vertex_node )
namespace {
frantic::magma::magma_node_base* create_magma_foreach_vertex_node() { return new magma_foreach_vertex_node; }
}
void magma_foreach_vertex_node::compile(
    frantic::magma::magma_compiler_interface& compiler ) { // compiler.compile( this );
    this->compile_as_extension_type( compiler );
}
void magma_foreach_vertex_node::create_type_definition( frantic::magma::magma_node_type& _outType ) {
    frantic::magma::magma_node_type_impl& outType = dynamic_cast<frantic::magma::magma_node_type_impl&>( _outType );
    outType.set_name( _T( "VertexLoopByEdge" ) );
    outType.set_category( _T( "Loop" ) );
    outType.set_factory( &create_magma_foreach_vertex_node );
    {
        std::unique_ptr<frantic::magma::property_meta<bool, magma_node_base>> propMeta(
            new frantic::magma::property_meta<bool, magma_node_base> );
        propMeta->m_name = _T("enabled");
        propMeta->m_getter = &magma_node_base::get_enabled;
        propMeta->m_setter = NULL /*&magma_node_base::set_enabled*/;
        outType.add_property( std::unique_ptr<frantic::magma::property_meta_interface>( propMeta.release() ), true );
    }
    typedef magma_foreach_vertex_node this_type;
    MAGMA_DESCRIPTION( "Runs an operation over all connected vertices" )
    MAGMA_TYPE_ATTR( container, true );
    MAGMA_EXPOSE_PROPERTY( maxIterations, int )
    MAGMA_EXPOSE_PROPERTY( outputMask, std::vector<int> )
    MAGMA_EXPOSE_PROPERTY( numInputs, int )
    MAGMA_EXPOSE_PROPERTY( numOutputs, int )
    MAGMA_EXPOSE_PROPERTY( visibleOnly, bool )
    MAGMA_READONLY_PROPERTY( uiType, frantic::tstring )
    MAGMA_READONLY_PROPERTY( numControlInputs, int )
    MAGMA_READONLY_PROPERTY( loopChannels, std::vector<frantic::tstring> )
    // MAGMA_HIDDEN_READONLY_PROPERTY( _internal_input_id, magma_interface::magma_id )
    // MAGMA_HIDDEN_READONLY_PROPERTY( _internal_output_id, magma_interface::magma_id )
    MAGMA_DEFINE_TYPE_END

    MAGMA_DEFINE_TYPE( "VertexLoopByEdge__Input", "System", magma_foreach_vertex_inputs_node )
    MAGMA_TYPE_ATTR( disableable, false )
    MAGMA_TYPE_ATTR( public, false );
    MAGMA_DEFINE_TYPE_END

    MAGMA_DEFINE_TYPE( "VertexLoopByEdge__Output", "System", magma_foreach_vertex_outputs_node )
    MAGMA_TYPE_ATTR( disableable, false )
    MAGMA_TYPE_ATTR( public, false );
    MAGMA_DEFINE_TYPE_END

    const std::vector<frantic::tstring>& magma_foreach_vertex_node::get_loopChannels() const {
        if( m_loopChannels
                .empty() ) { // This needs to be lazy since 'this->get_type()' cannot be called during the constructor.
            std::vector<std::pair<frantic::tstring, frantic::magma::magma_data_type>> vertChannels;

            this->get_type().get_singleton().get_predefined_vertex_channels( vertChannels );

            for( std::vector<std::pair<frantic::tstring, frantic::magma::magma_data_type>>::const_iterator
                     it = vertChannels.begin(),
                     itEnd = vertChannels.end();
                 it != itEnd; ++it )
                const_cast<std::vector<frantic::tstring>&>( m_loopChannels )
                    .push_back( it->first + _T(" ") + it->second.to_string() );

            const_cast<std::vector<frantic::tstring>&>( m_loopChannels ).push_back( _T("EdgeVisibility Bool") );
            const_cast<std::vector<frantic::tstring>&>( m_loopChannels ).push_back( _T("EdgeIsBoundary Bool") );
        }

        return m_loopChannels;
    }

    magma_foreach_vertex_node::magma_foreach_vertex_node()
        : m_geomInput( magma_interface::INVALID_ID, 0 )
        , m_objIndexInput( magma_interface::INVALID_ID, 0 )
        , m_vertIndexInput( magma_interface::INVALID_ID, 0 )
        , m_geomDefault()
        , m_objIndexDefault( 0 )
        , m_vertIndexDefault( 0 ) {
        this->set_visibleOnly( true );
    }

    void magma_foreach_vertex_node::do_clone( magma_node_base & dest, clone_callback & cb ) const {
        magma_loop_node::do_clone( dest, cb );
    }

    int magma_foreach_vertex_node::get_num_inputs() const {
        return magma_loop_node::get_num_inputs() + 3; // Add one input for the geometry
    }
    std::pair<magma_interface::magma_id, int> magma_foreach_vertex_node::get_input( int index ) const {
        if( index == 0 )
            return m_geomInput;
        else if( index == 1 )
            return m_objIndexInput;
        else if( index == 2 )
            return m_vertIndexInput;
        else
            return magma_loop_node::get_input( index - 3 );
    }

    void magma_foreach_vertex_node::get_input_description( int i, frantic::tstring& outDescription ) {
        if( i == 0 )
            outDescription = _T("Geometry");
        else if( i == 1 )
            outDescription = _T("ObjIndex");
        else if( i == 2 )
            outDescription = _T("VertIndex");
        else
            magma_loop_node::get_input_description( i - 3, outDescription );
    }

    void magma_foreach_vertex_node::get_output_description( int i, frantic::tstring& outDescription ) const {
        const_cast<magma_foreach_vertex_node*>( this )->get_input_description( i + 3, outDescription );
    }

    void magma_foreach_vertex_node::set_input( int index, magma_interface::magma_id id, int socketIndex ) {
        if( index == 0 )
            m_geomInput = std::make_pair( id, socketIndex );
        else if( index == 1 )
            m_objIndexInput = std::make_pair( id, socketIndex );
        else if( index == 2 )
            m_vertIndexInput = std::make_pair( id, socketIndex );
        else
            magma_loop_node::set_input( index - 3, id, socketIndex );
    }

    const variant_t& magma_foreach_vertex_node::get_input_default_value( int index ) const {
        if( index == 0 )
            return m_geomDefault;
        else if( index == 1 )
            return m_objIndexDefault;
        else if( index == 2 )
            return m_vertIndexDefault;
        return magma_loop_node::get_input_default_value( index - 3 );
    }

    void magma_foreach_vertex_node::set_input_default_value( int index, const variant_t& value ) {
        if( index == 0 )
            m_geomDefault = boost::blank();
        else if( index == 1 )
            m_objIndexDefault = value;
        else if( index == 2 )
            m_vertIndexDefault = value;
        else
            magma_loop_node::set_input_default_value( index - 3, value );
    }

    magma_foreach_vertex_inputs_node::magma_foreach_vertex_inputs_node()
        : magma_loop_inputs_node( 3, 0 ) {}

    void magma_foreach_vertex_inputs_node::get_output_description( int i, frantic::tstring& outDescription ) const {
        // if( i == 1 ){
        //	outDescription = _T("Edge Visible");
        // }else{
        return magma_loop_inputs_node::get_output_description( i, outDescription );
        //}
    }

    magma_foreach_vertex_outputs_node::magma_foreach_vertex_outputs_node()
        : magma_loop_outputs_node( 0 ) {}

    using namespace frantic::magma::simple_compiler;

    class foreach_vertex_expression : public base_compiler::foreach_expression {
        std::vector<magma_geometry_ptr> m_geometry;
        std::ptrdiff_t m_controlInputs[2]; // Ptrs to input values (ObjIndex & VertIndex)
        std::size_t m_maxIterations;
        bool m_visibleOnly;

        frantic::channels::channel_map m_bodyVars; // Locations and types of the body variables, relative to
                                                   // m_bodyVarPtr
        std::vector<std::pair<std::ptrdiff_t, std::ptrdiff_t>>
            m_bodyVarUpdatePtrs;       // Ptrs for assigning initial and post-loop values for the body variables.
        std::ptrdiff_t m_bodyVarPtr;   // Ptr to store the body variables at.
        std::ptrdiff_t m_iterationPtr; // Ptr to store the iteration index before running the loop body.
        std::ptrdiff_t m_neighborIndexPtr;
        std::ptrdiff_t m_edgeIsVisPtr;
        std::ptrdiff_t m_edgeIsBoundaryPtr;

        frantic::channels::channel_map m_tempVars;

        std::vector<expression*> m_bodyExpr;

      public:
        foreach_vertex_expression() {
            m_iterationPtr = -1;
            m_maxIterations = 1000;
            m_visibleOnly = true;

            m_tempVars.define_channel( _T("Iteration"), 1, frantic::channels::data_type_int32, 0 );
            m_tempVars.define_channel( _T("IsVisible"), 1, frantic::channels::data_type_int32, 4 );
            m_tempVars.define_channel( _T("IsBoundary"), 1, frantic::channels::data_type_int32, 8 );
            m_tempVars.end_channel_definition( 4u, true, false );

            m_edgeIsVisPtr = m_edgeIsBoundaryPtr = -1;
        }

        virtual ~foreach_vertex_expression() {
            for( std::vector<expression*>::iterator it = m_bodyExpr.begin(), itEnd = m_bodyExpr.end(); it != itEnd;
                 ++it )
                delete *it;
        }

        void set_max_iterations( std::size_t maxIterations ) { m_maxIterations = maxIterations; }

        void set_geometry( magma_input_geometry_interface& geom ) {
            geom.get_all_geometry( std::back_inserter( m_geometry ) );

            for( std::vector<magma_geometry_ptr>::iterator it = m_geometry.begin(), itEnd = m_geometry.end();
                 it != itEnd; ++it )
                ( *it )->get_mesh().init_adjacency(); // Side effect will allocate this structure needed while executing
                                                      // this operation.
        }

        void enable_invisible_edges( bool enabled = true ) { m_visibleOnly = !enabled; }

        virtual void set_input( std::size_t inputIndex, std::ptrdiff_t relPtr ) {
            m_controlInputs[inputIndex] = relPtr;
        }

        virtual void set_output( std::ptrdiff_t relPtr ) { m_bodyVarPtr = relPtr; }

        virtual const frantic::channels::channel_map& get_output_map() const { return m_bodyVars; }

        virtual void apply( base_compiler::state& data ) const;

        virtual void set_loop_body( std::vector<expression*>::iterator itStart,
                                    std::vector<expression*>::iterator itEnd ) {
            m_bodyExpr.assign( itStart, itEnd );
        }

        virtual void set_body_variables( const frantic::channels::channel_map& varMap ) {
            m_bodyVars = varMap;
            m_bodyVarUpdatePtrs.resize( varMap.channel_count(), std::make_pair( -1, -1 ) );
        }

        virtual void set_body_variable_init( std::size_t varIndex, std::ptrdiff_t initRelPtr ) {
            m_bodyVarUpdatePtrs[varIndex].first = initRelPtr;
        }

        virtual void set_body_variable_update( std::size_t varIndex, std::ptrdiff_t postLoopRelPtr ) {
            m_bodyVarUpdatePtrs[varIndex].second = postLoopRelPtr;
        }

        virtual void set_iteration_output( std::ptrdiff_t iterationPtr ) { m_iterationPtr = iterationPtr; }

        virtual const frantic::channels::channel_map& get_temporary_variables() const {
            // return frantic::magma::simple_compiler::traits<int>::get_static_map();
            return m_tempVars;
        }

        virtual void set_temporary_variables( std::ptrdiff_t tempPtr ) { m_neighborIndexPtr = tempPtr; }

        virtual void handle_loop_input( base_compiler& compiler, base_compiler::expression_id exprID,
                                        const frantic::tstring& channelName ) {
            // TODO: Could make this work by first looking at the values in get_temporary_variables() ...
            if( channelName == _T("EdgeVisibility") ) {
                if( m_visibleOnly ) {
                    compiler.compile_constant( exprID, true );
                } else {
                    // Pass empty vectors for inputs and types.
                    // compiler.compile_expression( make_edge_expression<edge_vis_functor>(), exprID, std::vector<
                    // std::pair<magma_interface::magma_id,int> >(), std::vector< magma_data_type >() );
                    m_edgeIsVisPtr = m_neighborIndexPtr + 4;
                    compiler.register_temporary( exprID, frantic::magma::simple_compiler::traits<bool>::get_type(),
                                                 m_edgeIsVisPtr );
                }
            } else if( channelName == _T("EdgeIsBoundary") ) {
                // Pass empty vectors for inputs and types.
                // compiler.compile_expression( make_edge_expression<edge_boundary_functor>(), exprID, std::vector<
                // std::pair<magma_interface::magma_id,int> >(), std::vector< magma_data_type >() );
                m_edgeIsBoundaryPtr = m_neighborIndexPtr + 8;
                compiler.register_temporary( exprID, frantic::magma::simple_compiler::traits<bool>::get_type(),
                                             m_edgeIsBoundaryPtr );
            } else {
                typedef frantic::magma::nodes::magma_vertex_query_node::meta::type functor_type;

                functor_type fn;
                fn.set_geometry( m_geometry );
                fn.add_channel( channelName );

                std::unique_ptr<base_compiler::expression> result(
                    new frantic::magma::simple_compiler::detail::expression_impl<functor_type, void( void*, int, int )>(
                        fn ) );

                result->set_input( 0, m_controlInputs[0] );
                result->set_input( 1, m_neighborIndexPtr );

                // Pass empty vectors for inputs and types.
                compiler.compile_expression( std::move( result ), exprID,
                                             std::vector<std::pair<magma_interface::magma_id, int>>(),
                                             std::vector<magma_data_type>() );
            }
        }
    };

    void foreach_vertex_expression::apply( base_compiler::state & data ) const {
        // Copy the initial values to the output location.
        for( std::size_t i = 0, iEnd = m_bodyVars.channel_count(); i < iEnd; ++i ) {
            void* dest = m_bodyVars[i].get_channel_data_pointer( data.get_output_pointer( m_bodyVarPtr ) );
            void* src = data.get_output_pointer( m_bodyVarUpdatePtrs[i].first );
            memcpy( dest, src, m_bodyVars[i].primitive_size() );
        }

        std::size_t objIndex = static_cast<std::size_t>( data.get_temporary<int>( m_controlInputs[0] ) );
        std::size_t vertIndex = static_cast<std::size_t>( data.get_temporary<int>( m_controlInputs[1] ) );

        if( objIndex >= m_geometry.size() )
            return;

        const frantic::geometry::mesh_interface& theMesh = m_geometry[objIndex]->get_mesh();
        if( vertIndex >= theMesh.get_num_verts() )
            return;

        frantic::geometry::vertex_iterator vIt;
        if( theMesh.init_vertex_iterator( vIt, vertIndex ) ) {
            std::size_t counter = 0;
            do {
                if( !m_visibleOnly || theMesh.is_edge_visible( vIt ) ) {
                    data.set_temporary( m_neighborIndexPtr, static_cast<int>( theMesh.get_edge_endpoint( vIt ) ) );

                    if( m_iterationPtr >= 0 )
                        data.set_temporary( m_iterationPtr, static_cast<int>( counter ) );

                    if( m_edgeIsVisPtr >= 0 && !m_visibleOnly )
                        data.set_temporary( m_edgeIsVisPtr, theMesh.is_edge_visible( vIt ) );

                    if( m_edgeIsBoundaryPtr >= 0 )
                        data.set_temporary( m_edgeIsBoundaryPtr, theMesh.is_edge_boundary( vIt ) );

                    for( std::vector<expression*>::const_iterator it = m_bodyExpr.begin(), itEnd = m_bodyExpr.end();
                         it != itEnd; ++it )
                        ( *it )->apply( data );

                    // Copy the loop's updated values to the output location.
                    for( std::size_t i = 0, iEnd = m_bodyVars.channel_count(); i < iEnd; ++i ) {
                        void* dest = m_bodyVars[i].get_channel_data_pointer( data.get_output_pointer( m_bodyVarPtr ) );
                        void* src = data.get_output_pointer( m_bodyVarUpdatePtrs[i].second );
                        memcpy( dest, src, m_bodyVars[i].primitive_size() );
                    }
                }

                ++counter;
            } while( counter < m_maxIterations && theMesh.advance_vertex_iterator( vIt ) );
        }
    }

    void magma_foreach_vertex_node::compile_as_extension_type( magma_compiler_interface & compiler ) {
        if( base_compiler* bc = dynamic_cast<base_compiler*>( &compiler ) ) {
            std::unique_ptr<foreach_vertex_expression> loopExpr( new foreach_vertex_expression );

            loopExpr->enable_invisible_edges( !this->get_visibleOnly() );
            loopExpr->set_geometry(
                *compiler.get_geometry_interface( this->get_input( 0 ).first, this->get_input( 0 ).second ) );

            std::vector<std::pair<magma_interface::magma_id, int>> controlInputs;
            std::vector<magma_data_type> controlInputTypes;

            controlInputs.push_back( compiler.get_node_input( *this, 1 ) );
            controlInputTypes.push_back( traits<int>::get_type() );
            controlInputs.push_back( compiler.get_node_input( *this, 2 ) );
            controlInputTypes.push_back( traits<int>::get_type() );

            std::vector<std::pair<magma_interface::magma_id, int>> loopBodyInputs;
            for( int i = 3, iEnd = this->get_num_inputs(); i < iEnd; ++i )
                loopBodyInputs.push_back( compiler.get_node_input( *this, i ) );

            magma_interface::magma_id sourceID = this->get_contained_source()->get_id();
            magma_interface::magma_id sinkID = this->get_contained_sink()->get_id();

            bc->compile_foreach(
                static_cast<std::unique_ptr<base_compiler::foreach_expression>>( std::move( loopExpr ) ),
                this->get_id(), controlInputs, controlInputTypes, sourceID, sinkID, loopBodyInputs,
                this->get_outputMask() );
        } else
            magma_loop_node::compile_as_extension_type( compiler );
    }
}
}
}
