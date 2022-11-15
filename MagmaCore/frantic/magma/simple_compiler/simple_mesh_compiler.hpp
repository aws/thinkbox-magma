// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <deque>
#include <map>
#include <vector>

#include <boost/any.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <frantic/geometry/mesh_interface.hpp>
#include <frantic/magma/nodes/magma_foreach_vertex_node.hpp>
#include <frantic/magma/nodes/magma_input_geometry_interface.hpp>
#include <frantic/magma/simple_compiler/base_compiler.hpp>

namespace tbb {
template <class T>
class blocked_range; // Forward decl
}

namespace frantic {
namespace magma {
namespace simple_compiler {

class buffered_mesh_channel;

class simple_mesh_compiler : public base_compiler {
  public:
    enum MeshIterationPattern { VERTEX, FACE, VERTEX_CUSTOM_FACES };

  public:
    simple_mesh_compiler();

    virtual ~simple_mesh_compiler();

    bool is_threading() const;

    frantic::geometry::mesh_interface& get_mesh_interface();

    const frantic::geometry::mesh_interface& get_mesh_interface() const;

    boost::shared_ptr<frantic::geometry::mesh_interface> get_mesh_interface_ptr() const;

    MeshIterationPattern get_iteration_pattern() const;

    void set_threading( bool enabled = true );

    void set_iteration_pattern( MeshIterationPattern iterPattern );

    void set_mesh_interface( boost::shared_ptr<frantic::geometry::mesh_interface> meshInterface );

    /**
     * This function will evaluate the code segments produced by compiling an AST,
     * using the mesh_channel_map supplied at constructed as input.  The output
     * of the channel operation will be in a channel of the input mesh.
     *
     */
    void eval();

    /**
     * This function will evaluate the code segments produced by compiling an AST,
     * using the mesh_channel_map supplied at constructed as input similar to eval().
     * The value produced by each AST node will be recorded into 'outValues'.
     */
    void eval_debug( std::vector<debug_data>& outValues,
                     std::size_t maxIterations = std::numeric_limits<std::size_t>::max() );

    virtual void build();

  public:
    virtual void compile( nodes::magma_input_channel_node* );

    virtual void compile( nodes::magma_loop_channel_node* );

    virtual void compile( nodes::magma_output_node* );

    virtual void compile( nodes::magma_vertex_query_node* );

    virtual void compile( nodes::magma_face_query_node* );

    virtual void compile( nodes::magma_intersection_node* );

    virtual void compile( nodes::magma_nearest_point_node* );

    /**
     * Registers an expression that extracts a particle channel ( ie. the one passed as a parameter to eval() ). Can
     * also verify that the channel is a specific type. \param exprID The id of the new expression to create. \param
     * channelName The name of the particle's channel to extract \param expectedType The type that the particle's
     * channel should be. An exception is thrown if it doesn't match. Can be NULL if you don't care.
     */
    virtual void compile_input_channel( expression_id exprID, const frantic::tstring& channelName,
                                        const magma_data_type* expectedType = NULL );

    /**
     * Registers an expression that writes its input value to a particle channel ( ie. the one passed as a parameter to
     * eval() ). Can also verify that the channel is a specific type. \param exprID The id of the new expression to
     * create. \param inputValue The id & value index of the expression to write to the particle. \param channelName The
     * name of the particle's channel to write to. \param expectedType The type that the particle's channel should be if
     * it isn't already in the native channel map.
     */
    virtual void compile_output( expression_id exprID, const std::pair<expression_id, int>& inputValue,
                                 const frantic::tstring& channelName, const magma_data_type& expectedType );

    /**
     * Implements an expression which packages the mesh currently being operated on (ie. this->get_mesh_interface_ptr())
     * into the format provided by InputGeometry. \param exprID The ID to associate with the resulting geometry data.
     */
    void compile_current_mesh( expression_id exprID );

  protected:
    void eval_vertices( const tbb::blocked_range<std::size_t>& range ) const;

    void eval_faces( const tbb::blocked_range<std::size_t>& range ) const;

    void eval_face_vertices( const tbb::blocked_range<std::size_t>& range ) const;

  private:
    bool m_isThreaded;

    MeshIterationPattern m_iterationPattern;

    boost::shared_ptr<frantic::geometry::mesh_interface> m_meshInterface;

    struct geometry_holder : public frantic::magma::nodes::magma_input_geometry_interface {
        frantic::magma::magma_geometry_ptr m_geom;

        virtual ~geometry_holder() {}

        virtual std::size_t size() const { return 1; }

        virtual magma_geometry_ptr get_geometry( std::size_t i ) const { return m_geom; }
    } m_currentGeometryHolder;

    bool m_geometryIsDirty;

    std::map<frantic::tstring, buffered_mesh_channel*> m_inputChannels;
};

inline void
simple_mesh_compiler::set_mesh_interface( boost::shared_ptr<frantic::geometry::mesh_interface> meshInterface ) {
    m_meshInterface = meshInterface;
}

inline void simple_mesh_compiler::set_threading( bool enabled ) { m_isThreaded = enabled; }

inline void simple_mesh_compiler::set_iteration_pattern( MeshIterationPattern iterPattern ) {
    m_iterationPattern = iterPattern;
}

inline bool simple_mesh_compiler::is_threading() const { return m_isThreaded; }

inline frantic::geometry::mesh_interface& simple_mesh_compiler::get_mesh_interface() { return *m_meshInterface; }

inline const frantic::geometry::mesh_interface& simple_mesh_compiler::get_mesh_interface() const {
    return *m_meshInterface;
}

inline boost::shared_ptr<frantic::geometry::mesh_interface> simple_mesh_compiler::get_mesh_interface_ptr() const {
    return m_meshInterface;
}

inline simple_mesh_compiler::MeshIterationPattern simple_mesh_compiler::get_iteration_pattern() const {
    return m_iterationPattern;
}

} // namespace simple_compiler
} // namespace magma
} // namespace frantic
