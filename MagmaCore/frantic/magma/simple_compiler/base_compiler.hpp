// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_compiler_interface.hpp>

#include <memory>

namespace frantic {
namespace magma {
namespace nodes {
class foreach_vertex_expression;
}
} // namespace magma
} // namespace frantic

namespace frantic {
namespace magma {
namespace simple_compiler {

class base_compiler : public frantic::magma::magma_compiler_interface {
  public:
    class expression;
    class state;

    typedef std::pair<magma_data_type, std::ptrdiff_t>
        temporary_meta; // Type that stores metadata about a temporary value.

    typedef frantic::magma::magma_interface::magma_id
        expression_id; // When referring to an expression, use this ID.Usually corresponds with a magma_node_base
                       // instance's id.

  public:
    const context_base& get_context_data() const;

    const frantic::magma::magma_interface& get_abstract_syntax_tree() const;

    boost::shared_ptr<context_base> get_context_ptr() const;

    boost::shared_ptr<frantic::magma::magma_interface> get_abstract_syntax_tree_ptr() const;

    void set_abstract_syntax_tree( boost::shared_ptr<frantic::magma::magma_interface> magma );

    void set_compilation_context( boost::shared_ptr<context_base> context );

    // Returns true if this expression does nothing.
    bool empty() const;

    /**
     * Clears the generated expression from previous calls to build.
     */
    virtual void reset_expression();

    /**
     * Builds the entire tree by calling build_subtree() on each of the nodes marked by m_magma->get_output(i,
     * TOPMOST_LEVEL)
     */
    virtual void build();

    /**
     * We can extend the types available to the compiler by directly subclassing expression ourselves and implementing
     * something crazy go nuts!
     * @param expr The new expression instance
     * @param exprID The ID for the new expression
     * @param inputs The input connections required for this expression
     * @param expectedTypes The expected types for each input connection. It is an error if expectedType.size() !=
     * inputs.size()
     */
    virtual void compile_expression( std::unique_ptr<expression> expr, expression_id exprID,
                                     const std::vector<std::pair<expression_id, int>>& inputs,
                                     const std::vector<magma_data_type>& expectedTypes );

  public:
#pragma region Node handlers
    // Arithmetic
    virtual void compile( nodes::magma_negate_node* );
    virtual void compile( nodes::magma_abs_node* );
    virtual void compile( nodes::magma_floor_node* );
    virtual void compile( nodes::magma_ceil_node* );
    virtual void compile( nodes::magma_sqrt_node* );
    virtual void compile( nodes::magma_log_node* );
    virtual void compile( nodes::magma_add_node* );
    virtual void compile( nodes::magma_sub_node* );
    virtual void compile( nodes::magma_mul_node* );
    virtual void compile( nodes::magma_div_node* );
    virtual void compile( nodes::magma_mod_node* );
    virtual void compile( nodes::magma_pow_node* );

    // Vectors
    virtual void compile( nodes::magma_component_sum_node* );
    virtual void compile( nodes::magma_normalize_node* );
    virtual void compile( nodes::magma_magnitude_node* );
    virtual void compile( nodes::magma_dot_node* );
    virtual void compile( nodes::magma_cross_node* );
    virtual void compile( nodes::magma_matrix_mul_node* );

    // Trigonometry
    virtual void compile( nodes::magma_cos_node* );
    virtual void compile( nodes::magma_acos_node* );
    virtual void compile( nodes::magma_sin_node* );
    virtual void compile( nodes::magma_asin_node* );
    virtual void compile( nodes::magma_tan_node* );
    virtual void compile( nodes::magma_atan_node* );
    virtual void compile( nodes::magma_atan2_node* );

    // Logic
    virtual void compile( nodes::magma_logical_not_node* );
    virtual void compile( nodes::magma_less_node* );
    virtual void compile( nodes::magma_lesseq_node* );
    virtual void compile( nodes::magma_greater_node* );
    virtual void compile( nodes::magma_greatereq_node* );
    virtual void compile( nodes::magma_equal_node* );
    virtual void compile( nodes::magma_notequal_node* );
    virtual void compile( nodes::magma_logical_and_node* );
    virtual void compile( nodes::magma_logical_or_node* );
    virtual void compile( nodes::magma_logical_xor_node* );
    virtual void compile( nodes::magma_logicswitch_node* );
    virtual void compile( nodes::magma_mux_node* );

    // Conversion
    virtual void compile( nodes::magma_to_float_node* );
    virtual void compile( nodes::magma_to_int_node* );
    virtual void compile( nodes::magma_breakout_node* );
    virtual void compile( nodes::magma_to_vector_node* );
    virtual void compile( nodes::magma_quat_to_vectors_node* );
    virtual void compile( nodes::magma_vectors_to_quat_node* );
    virtual void compile( nodes::magma_eulerangles_to_quat_node* );
    virtual void compile( nodes::magma_angleaxis_to_quat_node* );

    // Functions
    virtual void compile( nodes::magma_noise_node* );
    virtual void compile( nodes::magma_vecnoise_node* );
    virtual void compile( nodes::magma_blend_node* );
    virtual void compile( nodes::magma_clamp_node* );
    virtual void compile( nodes::magma_random_node* );
    virtual void compile( nodes::magma_vecrandom_node* );
    virtual void compile( nodes::magma_exponential_random_node* );
    virtual void compile( nodes::magma_weibull_random_node* );
    virtual void compile( nodes::magma_gaussian_random_node* );
    virtual void compile( nodes::magma_triangle_random_node* );
    virtual void compile( nodes::magma_uniform_on_sphere_random_node* );

    // Transforms
    virtual void compile( nodes::magma_to_world_node* );
    virtual void compile( nodes::magma_from_world_node* );
    virtual void compile( nodes::magma_to_camera_node* );
    virtual void compile( nodes::magma_from_camera_node* );
    virtual void compile( nodes::magma_quat_mul_node* );

    // Object queries
    virtual void compile( nodes::magma_nearest_point_node* );
    virtual void compile( nodes::magma_intersection_node* );
    virtual void compile( nodes::magma_in_volume_node* );
    virtual void compile( nodes::magma_nearest_particle_node* );
    virtual void compile( nodes::magma_nearest_particles_avg_node* );
    virtual void compile( nodes::magma_particle_kernel_node* );
    virtual void compile( nodes::magma_face_query_node* );
    virtual void compile( nodes::magma_vertex_query_node* );
    virtual void compile( nodes::magma_element_query_node* );
    virtual void compile( nodes::magma_mesh_query_node* );
    virtual void compile( nodes::magma_particle_query_node* );
    virtual void compile( nodes::magma_object_query_node* );

    // Misc.
    virtual void compile( nodes::magma_elbow_node* );
    virtual void compile( nodes::magma_blop_node* );
    virtual void compile( nodes::magma_blop_input_node* );
    virtual void compile( nodes::magma_blop_output_node* );
    virtual void compile( nodes::magma_loop_node* );
    virtual void compile( nodes::magma_loop_inputs_node* );
    virtual void compile( nodes::magma_loop_outputs_node* );
    virtual void compile( nodes::magma_loop_channel_node* );
    virtual void compile( magma_node_base* );
#pragma endregion

    virtual void compile_constant( magma_interface::magma_id id, const variant_t& value );

    virtual std::pair<magma_interface::magma_id, int> compile_constant( const variant_t& value );

    // Associates a compile-time interface with the next output of the specified node.
    virtual void register_interface( magma_interface::magma_id id, nodes::magma_input_geometry_interface* ifPtr );
    virtual void register_interface( magma_interface::magma_id id, nodes::magma_input_particles_interface* ifPtr );
    virtual void register_interface( magma_interface::magma_id id, nodes::magma_input_objects_interface* ifPtr );

    virtual frantic::magma::nodes::magma_input_geometry_interface*
    get_geometry_interface( frantic::magma::magma_interface::magma_id id, int outputIndex );
    virtual frantic::magma::nodes::magma_input_particles_interface*
    get_particles_interface( frantic::magma::magma_interface::magma_id id, int outputIndex );
    virtual frantic::magma::nodes::magma_input_objects_interface*
    get_objects_interface( frantic::magma::magma_interface::magma_id id, int outputIndex );

    // This is how we should expose functionality going forward. It doesn't create tight coupling w/ the node types and
    // also allows implementation re-use when we just pass the relevent parameters like this.
    virtual void compile_transforms( magma_interface::magma_id id,
                                     const std::vector<frantic::graphics::transform4f>& tm, transform_type tmType,
                                     const std::pair<frantic::magma::magma_interface::magma_id, int>& vectorInput,
                                     const std::pair<frantic::magma::magma_interface::magma_id, int>& indexInput );

    virtual void
    compile_field( magma_interface::magma_id id,
                   const boost::shared_ptr<frantic::volumetrics::field_interface>& fieldPtr,
                   const std::pair<magma_interface::magma_id, int>& posInput,
                   const std::vector<frantic::tstring>& reorderedChannels = std::vector<frantic::tstring>() );

    class foreach_expression; // Forward decl of expression subclass for loops.

    virtual void compile_foreach(
        std::unique_ptr<foreach_expression> expr, expression_id exprID,
        const std::vector<std::pair<expression_id, int>>& loopControlInputs,
        const std::vector<magma_data_type>& loopControlExpectedTypes,
        magma_interface::magma_id loopBodyInputNode, // TODO This is kind of weird design, since we are passing a nodeID
                                                     // here but avoiding referring to a node by not passing one ...
        magma_interface::magma_id
            loopBodyOutputNode, // TODO This is kind of weird design, since we are passing a nodeID here but avoiding
                                // referring to a node by not passing one ...
        const std::vector<std::pair<expression_id, int>>& loopBodyInputs, const std::vector<int>& loopBodyOutputMask );

  protected:
    class expression_meta;        // Stores metadata about the output(s) of an expression.
    friend class expression_meta; // Workaround for C++ 03 badness
    friend class frantic::magma::nodes::foreach_vertex_expression; // Workaround. TODO: Fix this.

    base_compiler();

    virtual ~base_compiler();

    void do_move( base_compiler& other );

    /**
     * Builds a portion of the tree rooted at the specified node.
     * @param nodeID The node to start evaluating at.
     */
    virtual void build_subtree( frantic::magma::magma_interface::magma_id nodeID );

    /**
     * Traverses the AST, calling compile on each node in a post-fix manner.
     * @param node The node to visit. Non-const since magma_node_base::compile() is called, which is not const to handle
     * nodes that cache (but that might be bad. TODO: Should magma_node_base::compile() be const?).
     */
    void visit( frantic::magma::magma_node_base& node );

    // True the specified node has been visited already.
    bool is_visited( frantic::magma::magma_interface::magma_id nodeID );

    const temporary_meta& get_value( expression_id exprID, int index );

    template <class Interface>
    Interface* get_interface( expression_id exprID, int index, bool allowNull = false );

    magma_data_type get_input_type( const frantic::magma::magma_node_base& node, int inputIndex,
                                    bool allowUnconnected = false );

    const temporary_meta* get_input_value( const frantic::magma::magma_node_base& node, int inputIndex,
                                           bool allowUnconnected = false );

    template <class Interface>
    Interface* get_input_interface( frantic::magma::magma_node_base& node, int inputIndex,
                                    bool allowUnconnected = false );

    // Allocate temporary space for a single ouput for the indicated expression. Can be called multiple times to
    // allocate multiple results.
    const temporary_meta& allocate_temporary( expression_id exprID, const frantic::magma::magma_data_type& dt );

    // Allocates all the temporaries for an expression, as defined by its channel map. Should only be called once
    // per-expression.
    std::ptrdiff_t allocate_temporaries( expression_id exprID, const frantic::channels::channel_map& outMap );

    // Overload. This version uses a shuffle array to have a different order of expression outputs relative to the
    // channel map's memory layout.
    std::ptrdiff_t allocate_temporaries( expression_id exprID, const frantic::channels::channel_map& outMap,
                                         const std::vector<std::size_t>& shuffleIndices );

    const temporary_meta& allocate_constant( expression_id exprID, const frantic::magma::variant_t& val );

    std::pair<expression_id, int> get_arbitrary_expression_id();

    void register_temporary( expression_id exprID, const frantic::magma::magma_data_type& dt, std::ptrdiff_t relPtr );

    void register_expression( std::unique_ptr<expression> expr );

    virtual void do_eval( state& theState ) const;

    virtual void do_eval_debug( state& theState, debug_data& outDebugData ) const;

    /**
     * Instantiates a subclass of expression that contains a copy of fn that is used to generate the output(s) from the
     * inputs when evaluated.
     * @tparam ExpressionMetaData A wrapper class that exposes metadata about the functor. Must define at least:
     *          static const int ARITY;                    //The number of inputs required by the functor
     *          typedef ... type;                          //The concrete functor class that implements this expression.
     *          typedef boost::mpl::vector< ... > bindings //An MPL vector of function types that indicate the valid
     * input bindings and the associated output type. This
     *                                                     //may have the form void(void*,...) for expressions with
     * multiple return types. In this case the functor must
     *                                                     //expose a function const channel_map& get_output_map()
     * describing the values returned.
     *
     * @param exprID The ID to associate with the produced expression
     * @param fn The instance of the functor used to implement the expression. This will be *copied* into an expression
     * subclass. TODO: Make it movable
     * @param inputs The array of values used as inputs to this expression. The appropriate overloaded (or template)
     * function operator() in 'fn' will be chosen based on the input type combinations.
     * @param numInputs The number of values in the 'inputs' array.
     * @param expectedNumOutputs The number of outputs expected from this expression.
     */
    template <class ExpressionMetaData>
    void compile_impl( expression_id exprID, const typename ExpressionMetaData::type& fn, const temporary_meta inputs[],
                       std::size_t numInputs, std::size_t expectedNumOutputs = 1 );

    /**
     * @overload
     * Automates the input evaluation then passes over control to the other compiler implementation.
     */
    template <class ExpressionMetaData>
    void compile_impl( const frantic::magma::magma_node_base& node, const typename ExpressionMetaData::type& fn );

  private:
    typedef std::vector<expression*> expression_list;
    typedef std::map<expression_id, expression_meta> expression_meta_map;

    boost::shared_ptr<frantic::magma::magma_interface>
        m_magma; // Node collection that represents the overall expression to compile.
    boost::shared_ptr<context_base>
        m_context; // Extra data providing contextual information to nodes during compilation. Ex. current time.

    // Storage of metadata about temporary values computed as part of the overall expression.
    expression_meta_map m_temporaryResults;

    // Results are combined here.
    expression_list m_currentExpression;

    // Size of the buffer needed to store temporary values.
    std::size_t m_temporaryStorageSize;

    foreach_expression*
        m_currentLoop; // The current loop being compiled. Will be NULL if we aren't compiling inside a loop.
};

// This class encapsulates the state used during the evaluation of expressions. Subclass compilers are expected to
// extend this with data required by their unique expressions. Those expression will know which subclass of state can be
// safely cast to in order to access the extra data.
class base_compiler::state {
    char* m_memory; // All temporary values used by expression are written to this location.

    friend class base_compiler;

  public:
    // Retrieves a value from the temporary storage.
    template <class T>
    inline const T& get_temporary( std::ptrdiff_t relPtr ) const {
        return *reinterpret_cast<const T*>( m_memory + relPtr );
    }

    // Assigns a value to a temporary storage location
    template <class T>
    inline void set_temporary( std::ptrdiff_t relPtr, const T& val ) {
        *reinterpret_cast<T*>( m_memory + relPtr ) = val;
    }

    // Gets a pointer to a temporary storage location. This should be used in conjunction with a
    // frantic::channels::channel_map for populating complex return types.
    inline void* get_output_pointer( std::ptrdiff_t relPtr ) { return m_memory + relPtr; }
};

// This class encapsulates the basic unit of execution in magma.
class base_compiler::expression {
  public:
    virtual ~expression() {}

    /**
     * Binds the specified input to this expression to a given temporary value location.
     * @param inputIndex Which input to bind. No bounds checking is done on this.
     * @param relPtr The temporary value location to bind to. This will be later passed to
     * base_compiler::state::get_temporary() to access the value at runtime.
     */
    virtual void set_input( std::size_t inputIndex, std::ptrdiff_t relPtr ) = 0;

    /**
     * Binds the output of the expression to the given temporary value location. The memory layout of the output is
     * described by get_output_map().
     * @param relPtr The temporary value location to bind to. This will be later passed to
     * base_compiler::state::set_temporary() to set the value at runtime.
     */
    virtual void set_output( std::ptrdiff_t relPtr ) = 0;

    virtual const frantic::channels::channel_map& get_output_map() const = 0;

    /**
     * This function will produce the result of this expression by reading its required input values and store it back
     * into the state object.
     * @param data Inputs to this expression are retrieved from this object. The output is stored
     */
    virtual void apply( base_compiler::state& data ) const = 0;

    // Type of a C function ptr that mimics apply()
    typedef void ( *runtime_ptr )( const expression*, base_compiler::state& data );

    // We can avoid virtual function calls by extracting a direct function ptr and storing that alongside the ptr to the
    // subexpression. This might be quicker since it eliminates an indirection, but it also might not matter much
    // relative to the actual overhead of calling the function. Gotta test, test, test
    //
    // Another way this might get used is if we switch to a LLVM compiled approach. Since some nodes will still be
    // implemented in MSVC++ we need to use C ptrs in order to cross between LLVM & MSVC since their C++ ABI will not
    // match (ie. Cannot directly call virtual member functions from LLVM that were compiled in MSVC++).
    virtual runtime_ptr get_runtime_ptr() const { return NULL; }
};

// Subclass of expression that handles executing expressions in a loop.
class base_compiler::foreach_expression : public base_compiler::expression {
  public:
    virtual void handle_loop_input( base_compiler& compiler, base_compiler::expression_id exprID,
                                    const frantic::tstring& channelName ) = 0;

    // The expression that is executed on each loop iteration
    virtual void set_loop_body( std::vector<expression*>::iterator itStart,
                                std::vector<expression*>::iterator itEnd ) = 0;

    // The variables (input subset) that are changed by the loop
    virtual void set_body_variables( const frantic::channels::channel_map& varMap ) = 0;

    /**
     * Binds the initial values for body variables declared via set_body_variables().
     * @param varIndex The index of the variable declard via set_body_variables()
     * @param initRelPtr Ptr to the initial value (ie. before loop begins) for this variable
     */
    virtual void set_body_variable_init( std::size_t varIndex, std::ptrdiff_t initRelPtr ) = 0;

    /**
     * Binds the post-iteration values for body variables declared via set_body_variables().
     * @param varIndex The index of the variable declard via set_body_variables()
     * @param initRelPtr Ptr to the post-iteration value (ie. before loop begins) for this variable.
     */
    virtual void set_body_variable_update( std::size_t varIndex, std::ptrdiff_t postLoopRelPtr ) = 0;

    // The loop exposes the current iteration index by writing it to this location.
    virtual void set_iteration_output( std::ptrdiff_t iterationPtr ) = 0;

    virtual const frantic::channels::channel_map& get_temporary_variables() const = 0;

    virtual void set_temporary_variables( std::ptrdiff_t tempPtr ) = 0;
};

// Stores metadata about the output(s) of an expression.
class base_compiler::expression_meta {
    std::deque<base_compiler::temporary_meta> m_temporaries; // A deque so that appending doesn't invalidate pointers.

  public:
    // Record a new runtime temporary value that is generated by the associated expression.
    void add_temporary_value( const frantic::magma::magma_data_type& type, std::ptrdiff_t relPtr ) {
        m_temporaries.push_back( base_compiler::temporary_meta( type, relPtr ) );
    }

    // Record a new compile value that is exposed by the associated expression. This typically indicates data that is
    // passed around like a geometry collection or particle collection. TODO: We should record the actual pointer value
    // for later retrieval.
    void add_compile_time_value( void* /*ifPtr*/ ) {
        // We are reserving data_type_invalid to indicate this is a compile time only value. It has no meaning at
        // runtime.
        frantic::magma::magma_data_type dt;
        dt.m_elementCount = 0;
        dt.m_elementType = frantic::channels::data_type_invalid;
        dt.m_typeName = _T("");

        m_temporaries.push_back(
            std::make_pair( dt, -1 ) ); // The -1 value also indicates that this is not valid at runtime.
    }

    // Returns the number of values generated by this expression.
    inline std::size_t get_num_values() const { return m_temporaries.size(); }

    // Indicates if this expression produces no values.
    inline bool empty() const { return m_temporaries.empty(); }

    // True if the expression generates a value with the given index. TODO: Can this be deleted?
    bool has_temporary_value( std::size_t i ) const { return m_temporaries.size() > i; }

    // Returns the i'th temporary value's metadata.
    const base_compiler::temporary_meta& get_temporary_value( std::size_t i ) const { return m_temporaries[i]; }
};

inline const base_compiler::context_base& base_compiler::get_context_data() const { return *m_context; }

inline const magma_interface& base_compiler::get_abstract_syntax_tree() const { return *m_magma; }

} // namespace simple_compiler
} // namespace magma
} // namespace frantic
