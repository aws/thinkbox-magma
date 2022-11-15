// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_interface.hpp>
#include <frantic/magma/magma_standard_nodes.hpp>

#include <boost/any.hpp>

namespace frantic {
namespace volumetrics {
class field_interface;
}
} // namespace frantic

namespace frantic {
namespace magma {

namespace nodes {
class magma_input_geometry_interface;  // Fwd decl.
class magma_input_particles_interface; // Fwd decl.
class magma_input_objects_interface;   // Fwd decl.
} // namespace nodes

class magma_compiler_interface {
  public:
    virtual ~magma_compiler_interface() {}

    // TODO: Add every node in the standard set to this class.
    virtual void compile( nodes::magma_elbow_node* ) = 0;
    virtual void compile( nodes::magma_negate_node* ) = 0;
    virtual void compile( nodes::magma_abs_node* ) = 0;
    virtual void compile( nodes::magma_component_sum_node* ) = 0;
    virtual void compile( nodes::magma_magnitude_node* ) = 0;
    virtual void compile( nodes::magma_normalize_node* ) = 0;
    virtual void compile( nodes::magma_floor_node* ) = 0;
    virtual void compile( nodes::magma_ceil_node* ) = 0;
    virtual void compile( nodes::magma_sqrt_node* ) = 0;
    virtual void compile( nodes::magma_log_node* ) = 0;
    virtual void compile( nodes::magma_cos_node* ) = 0;
    virtual void compile( nodes::magma_acos_node* ) = 0;
    virtual void compile( nodes::magma_sin_node* ) = 0;
    virtual void compile( nodes::magma_asin_node* ) = 0;
    virtual void compile( nodes::magma_tan_node* ) = 0;
    virtual void compile( nodes::magma_atan_node* ) = 0;
    virtual void compile( nodes::magma_logical_not_node* ) = 0;
    virtual void compile( nodes::magma_to_float_node* ) = 0;
    virtual void compile( nodes::magma_to_int_node* ) = 0;
    virtual void compile( nodes::magma_breakout_node* ) = 0;
    virtual void compile( nodes::magma_noise_node* ) = 0;
    virtual void compile( nodes::magma_vecnoise_node* ) = 0;
    virtual void compile( nodes::magma_random_node* ) = 0;
    virtual void compile( nodes::magma_vecrandom_node* ) = 0;
    virtual void compile( nodes::magma_exponential_random_node* ) = 0;
    virtual void compile( nodes::magma_weibull_random_node* ) = 0;
    virtual void compile( nodes::magma_gaussian_random_node* ) = 0;
    virtual void compile( nodes::magma_triangle_random_node* ) = 0;
    virtual void compile( nodes::magma_uniform_on_sphere_random_node* ) = 0;
    virtual void compile( nodes::magma_to_world_node* ) = 0;
    virtual void compile( nodes::magma_from_world_node* ) = 0;
    virtual void compile( nodes::magma_to_camera_node* ) = 0;
    virtual void compile( nodes::magma_from_camera_node* ) = 0;
    virtual void compile( nodes::magma_quat_to_vectors_node* ) = 0;

    virtual void compile( nodes::magma_add_node* ) = 0;
    virtual void compile( nodes::magma_sub_node* ) = 0;
    virtual void compile( nodes::magma_mul_node* ) = 0;
    virtual void compile( nodes::magma_div_node* ) = 0;
    virtual void compile( nodes::magma_mod_node* ) = 0;
    virtual void compile( nodes::magma_pow_node* ) = 0;
    virtual void compile( nodes::magma_atan2_node* ) = 0;
    virtual void compile( nodes::magma_dot_node* ) = 0;
    virtual void compile( nodes::magma_cross_node* ) = 0;
    virtual void compile( nodes::magma_less_node* ) = 0;
    virtual void compile( nodes::magma_lesseq_node* ) = 0;
    virtual void compile( nodes::magma_greater_node* ) = 0;
    virtual void compile( nodes::magma_greatereq_node* ) = 0;
    virtual void compile( nodes::magma_equal_node* ) = 0;
    virtual void compile( nodes::magma_notequal_node* ) = 0;
    virtual void compile( nodes::magma_logical_and_node* ) = 0;
    virtual void compile( nodes::magma_logical_or_node* ) = 0;
    virtual void compile( nodes::magma_logical_xor_node* ) = 0;
    virtual void compile( nodes::magma_nearest_point_node* ) = 0;
    virtual void compile( nodes::magma_intersection_node* ) = 0;
    virtual void compile( nodes::magma_in_volume_node* ) = 0;
    virtual void compile( nodes::magma_nearest_particle_node* ) = 0;
    virtual void compile( nodes::magma_nearest_particles_avg_node* ) = 0;
    virtual void compile( nodes::magma_particle_kernel_node* ) = 0;
    virtual void compile( nodes::magma_quat_mul_node* ) = 0;

    virtual void compile( nodes::magma_blend_node* ) = 0;
    virtual void compile( nodes::magma_clamp_node* ) = 0;
    virtual void compile( nodes::magma_logicswitch_node* ) = 0;
    virtual void compile( nodes::magma_to_vector_node* ) = 0;
    virtual void compile( nodes::magma_face_query_node* ) = 0;
    virtual void compile( nodes::magma_vertex_query_node* ) = 0;
    virtual void compile( nodes::magma_element_query_node* ) = 0;
    virtual void compile( nodes::magma_mesh_query_node* ) = 0;
    virtual void compile( nodes::magma_particle_query_node* ) = 0;
    virtual void compile( nodes::magma_object_query_node* ) = 0;
    virtual void compile( nodes::magma_vectors_to_quat_node* ) = 0;
    virtual void compile( nodes::magma_eulerangles_to_quat_node* ) = 0;
    virtual void compile( nodes::magma_angleaxis_to_quat_node* ) = 0;

    virtual void compile( nodes::magma_matrix_mul_node* ) = 0;

    virtual void compile( nodes::magma_mux_node* ) = 0;

    virtual void compile( nodes::magma_input_channel_node* ) = 0;
    virtual void compile( nodes::magma_output_node* ) = 0;

    virtual void compile( nodes::magma_blop_node* ) = 0;
    virtual void compile( nodes::magma_blop_input_node* ) = 0;
    virtual void compile( nodes::magma_blop_output_node* ) = 0;
    virtual void compile( nodes::magma_loop_node* ) = 0;
    virtual void compile( nodes::magma_loop_inputs_node* ) = 0;
    virtual void compile( nodes::magma_loop_outputs_node* ) = 0;
    virtual void compile( nodes::magma_loop_channel_node* ) = 0;

    // Handle the generic case for non-standard nodes.
    virtual void compile( magma_node_base* extensionNode ) = 0;

    virtual void compile_constant( magma_interface::magma_id id, const variant_t& value ) = 0;

    /**
     * Compiles a constant valued expression and returns an ID & index pair that allows other nodes to reference this
     * value. The ID will be one not used in the associated node graph. This is used to implement a node input's default
     * values where an unconnected socket can provide a default, constant value to be used instead.
     * @param value The constant value to use.
     * @return An expression ID and value index that can be used to reference this constant.
     */
    virtual std::pair<magma_interface::magma_id, int> compile_constant( const variant_t& value ) = 0;

    virtual void register_interface( magma_interface::magma_id id, nodes::magma_input_geometry_interface* ifPtr ) = 0;
    virtual void register_interface( magma_interface::magma_id id, nodes::magma_input_particles_interface* ifPtr ) = 0;
    virtual void register_interface( magma_interface::magma_id id, nodes::magma_input_objects_interface* ifPtr ) = 0;

    enum transform_type { transform_point, transform_direction, transform_normal };

    virtual void compile_transforms( magma_interface::magma_id id,
                                     const std::vector<frantic::graphics::transform4f>& tm, transform_type tmType,
                                     const std::pair<frantic::magma::magma_interface::magma_id, int>& vectorInput,
                                     const std::pair<frantic::magma::magma_interface::magma_id, int>& indexInput ) = 0;

    /**
     * Compile a node which evaluates a field at a runtime-determined location.
     * \param id The ID to associate with the compiled operation
     * \param fieldPtr The field to evaluate at runtime. The field must be thread-safe since it can be evaluated in a
     * multithreaded context. \param posInput The node & socket that are connected as the position to evaluate the field
     * at. \param reorderedChannels The outputs of this operation are determined from the channel map of the
     * field_interface. This parameters allows an alternative ordering to be specified.
     */
    virtual void
    compile_field( magma_interface::magma_id id,
                   const boost::shared_ptr<frantic::volumetrics::field_interface>& fieldPtr,
                   const std::pair<magma_interface::magma_id, int>& posInput,
                   const std::vector<frantic::tstring>& reorderedChannels = std::vector<frantic::tstring>() ) = 0;

    // Extracts the interface from the specified node's output socket. Node must have registered this interface via.
    // register_interface().
    virtual nodes::magma_input_geometry_interface* get_geometry_interface( magma_interface::magma_id id,
                                                                           int outputIndex ) = 0;
    virtual nodes::magma_input_particles_interface* get_particles_interface( magma_interface::magma_id id,
                                                                             int outputIndex ) = 0;
    virtual nodes::magma_input_objects_interface* get_objects_interface( magma_interface::magma_id id,
                                                                         int outputIndex ) = 0;

    // Helper function that gets a node's input connectivity info, generating a new one if is unconnected
    // but a default value is provided for that socket.
    inline std::pair<magma_interface::magma_id, int> get_node_input( const magma_node_base& node, int inputIndex ) {
        std::pair<magma_interface::magma_id, int> result = node.get_input( inputIndex );
        if( result.first == magma_interface::INVALID_ID ) {
            const variant_t& defaultVal = node.get_input_default_value( inputIndex );
            if( defaultVal.type() == typeid( boost::blank ) )
                throw magma_exception() << magma_exception::node_id( node.get_id() )
                                        << magma_exception::input_index( inputIndex )
                                        << magma_exception::error_name( _T("Unconnected input socket") );

            result = this->compile_constant( defaultVal );
        }

        return result;
    }

  public:
    /**
     * Subclasses of this class will be queried for via dynamic_cast<> to get context specific data for non-standard
     * nodes.
     */
    class context_base {
      public:
        virtual ~context_base() {}

        virtual frantic::tstring get_name() const = 0;

        /**
         * Returns a matrix that transforms from the current object's space into world space.
         * @param inverse If true, the matrix is inverted, transforming from worldspace into object space.
         */
        virtual frantic::graphics::transform4f get_world_transform( bool inverse = false ) const = 0;

        /**
         * Returns a matrix that transforms from world space into the current camera's space.
         * @param inverse If true, the matrix is inverted, transforming from cameraspace into world space.
         */
        virtual frantic::graphics::transform4f get_camera_transform( bool inverse = false ) const = 0;

        /**
         * Returns a property that nodes might need, in a type agnostic container.
         */
        virtual boost::any get_property( const frantic::tstring& name ) const {
            if( name == _T("WorldTransform") )
                return boost::any( this->get_world_transform( false ) );
            else if( name == _T("WorldTransformInverse") )
                return boost::any( this->get_world_transform( true ) );
            else if( name == _T("CameraTransform") )
                return boost::any( this->get_camera_transform( false ) );
            else if( name == _T("CameraTransformInverse") )
                return boost::any( this->get_camera_transform( true ) );
            return boost::any();
        }

        // Helper function template for working with boost::any properties when you know an exact type you want.
        template <class T>
        inline T get_property( const frantic::tstring& name ) const {
            return boost::any_cast<T>( this->get_property( name ) );
        }

        // Helper function template for working with boost::any properties. This overload allows for default values.
        template <class T>
        inline bool get_property( const frantic::tstring& name, T& outVal ) const {
            boost::any result = this->get_property( name );
            if( !result.empty() && typeid( T ) == result.type() )
                return outVal = boost::any_cast<T&>( result ), true;
            return false;
        }
    };

    /**
     * Nodes that expect a specific subclass of context_base, will dynamic_cast<> this reference to the appropriate type
     * to get information that is not explicitly part of magma, but is associated with the context magma is being used
     * in. For example, in 3ds Max we provide a TimeValue that everything is evaluated at among other things.
     */
    virtual const context_base& get_context_data() const = 0;
};

/**
 * This class can be used to extract debugging information. It records the temporary values output from each node during
 * execution.
 */
class debug_data {
    // Contains a vector of values for each node in the flow.
    std::map<magma_interface::magma_id, std::vector<variant_t>> m_data;

  public:
    void append_value( magma_interface::magma_id nodeID, const variant_t& value ) { m_data[nodeID].push_back( value ); }

    std::size_t size() const { return m_data.size(); }

    typedef std::map<magma_interface::magma_id, std::vector<variant_t>>::const_iterator const_iterator;
    typedef std::vector<variant_t>::const_iterator const_value_iterator;

    const_iterator begin() const { return m_data.begin(); }
    const_iterator end() const { return m_data.end(); }
    const_iterator find( magma_interface::magma_id id ) const { return m_data.find( id ); }
};

} // namespace magma
} // namespace frantic
