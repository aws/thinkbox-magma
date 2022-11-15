// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include <frantic/magma/magma_interface.hpp>
#include <frantic/magma/magma_singleton.hpp>
#include <frantic/magma/simple_compiler/simple_particle_compiler.hpp>

/**
 * Magma's main object for creating, managing and modifying nodes that you will be working with is class
 * magma_interface. It is a pure virtual interface that creates the abstraction for how you interact with the collection
 * of nodes that make up the magma 'flow'.
 *
 * The class heirarchy of magma_compiler_interface, base_compiler, simple_particle_compiler and simple_mesh_compiler are
 * the main objects for taking the collection of magma nodes (managed via the magma_interface object) and converting
 * them into executable code. Each base_compiler subclass is responsible for connecting the executable code with a
 * differernt for of data that it operates on (hence the the particle and mesh subclasses).
 *
 * You cannot directly create an instance of magma_interface (because it is pure virtual) and you should not be directly
 * creating any derived concrete classes. Instead you need to create an instance by using a global singleton object of
 * class magma_singleton. The instance of class magma_singleton is created for each use case of magma you might have.
 * For example if you had magma expressions for modifying particles and you also had magma expressions for shading
 * geometry at render time you would create a different subclass of magma_singleton for each. When you override
 * magma_singleton that is where you can hook up any nodes that are customized for the specific context (ex. a node that
 * does some rendering calculations) as well as nodes that are specific to the host application (ex. 3ds Max specific
 * nodes vs. Maya specific nodes). Once you have subclasses magma_singleton you can use it to create a new
 * magma_interface object via magma_singleton::create_magma_instance().
 *
 * You will not be directly working with node object when you are using magma. All access to creating, deleting, and
 * modifying nodes is done through the owning magma_interface object using the unique id associated with each node.
 *
 * Summary:
 * 1. Subclass magma_singleton for your data context and host application.
 * 2. Create a magma_interface object that manages you nodes by calling magma_singleton::create_magma_interface()
 * 3. Create some nodes in the magma_interface using magma_interface::create_node( const frantic::tstring& nodeTypeName
 * )
 * 4. Set node properties using template <class T> magma_interface::set_property( magma_id nodeID, const
 * frantic::tstring& propertyName, const T& propertyValue )
 * 5. Connect nodes in the graph using magma_interface::set_input( magma_id nodeID, int inputIndex, magma_id
 * incomingNodeID, int incomingNodeOutputIndex )
 * 6. Create an instance of an appropriate base_compiler subclass (ex. simple_particle_compiler for working on particle
 * data)
 * 7. Configure the compiler as appropriate
 * 8. Build (ie. compile) the executable expression
 * 9. Using the base_compiler subclass specific mechanism, run the expression on your data (ex.
 * simple_particle_compiler::eval( char* particleData ) )
 */

/**
 * I am defining a subclass of magma_singleton here that is specific to my console application. I just use the default
 * collection of nodes for now, but I could add extra nodes by using define_node_type<MyNodeType>() in the constructor.
 */
class sample_magma_singleton : public frantic::magma::magma_singleton {
  public:
    static sample_magma_singleton& get_instance() {
        static sample_magma_singleton theSingleton;
        return theSingleton;
    }

  private:
    sample_magma_singleton()
        : frantic::magma::magma_singleton(
              true ) // Passing true here causes all of the common/default nodes to be defined automatically.
    {
        // TODO: Define any non-default node types here.
    }
};

/**
 * This context object is passed to the magma_compiler_interface to provide access to contextual information or global
 * values during the compilation process. It is useful for passing information like a transform to worldspace, the
 * bounding box of a mesh, rendering settings, etc.
 */
class sample_magma_context : public frantic::magma::magma_compiler_interface::context_base {
  public:
    virtual frantic::tstring get_name() const { return _T("SampleContext"); }

    virtual frantic::graphics::transform4f get_world_transform( bool /*inverse = false*/ ) const {
        return frantic::graphics::transform4f();
    }

    virtual frantic::graphics::transform4f get_camera_transform( bool /*inverse = false*/ ) const {
        return frantic::graphics::transform4f();
    }

    virtual boost::any get_property( const frantic::tstring& name ) const {
        // TODO: Any custom properties can be searched name here. Make sure to call the base class implementation if one
        // isn't found.
        if( name == _T("SampleProperty") )
            return boost::any( frantic::tstring( _T("This is a sample property") ) );

        return frantic::magma::magma_compiler_interface::context_base::get_property( name );
    }
};

/**
 * Create some arbitrary particle data for purposes of modification.
 */
void init_particle_data( frantic::particles::particle_array& outParticleData ) {
    frantic::channels::channel_map prtMap;
    prtMap.define_channel<frantic::graphics::vector3f>( _T("Position") );
    prtMap.define_channel<frantic::graphics::vector3f>( _T("Color") );
    prtMap.define_channel<float>( _T("Density") );
    prtMap.end_channel_definition();

    outParticleData.reset( prtMap );

    frantic::channels::channel_accessor<frantic::graphics::vector3f> posAccessor =
        prtMap.get_accessor<frantic::graphics::vector3f>( _T("Position") );
    frantic::channels::channel_accessor<frantic::graphics::vector3f> colorAccessor =
        prtMap.get_accessor<frantic::graphics::vector3f>( _T("Color") );
    frantic::channels::channel_accessor<float> densityAccessor = prtMap.get_accessor<float>( _T("Density") );

    char* tempParticle = static_cast<char*>( alloca( prtMap.structure_size() ) );
    for( int i = 0; i < 1000; ++i ) {
        posAccessor.get( tempParticle ) = frantic::graphics::vector3f::from_random();
        colorAccessor.get( tempParticle ) = frantic::graphics::vector3f( 0, 0, 1.f ); // Blue
        densityAccessor.get( tempParticle ) = 1.f;

        outParticleData.push_back( tempParticle );
    }
}

int main( int argc, char* argv[] ) {
    // Let's assume we have a collection of particle data that we want to modify.
    boost::shared_ptr<frantic::particles::particle_array> particles( new frantic::particles::particle_array );
    init_particle_data( *particles );

    // Let's create a magma expression that modifies the Color channel of the particle data.
    boost::shared_ptr<frantic::magma::magma_interface> magma =
        sample_magma_singleton::get_instance().create_magma_instance();

    // Create an Output node (that writes to particle channels) and configure its properties.
    frantic::magma::magma_interface::magma_id outId = magma->create_node( _T("Output") );
    magma->set_property<frantic::tstring>( outId, _T("channelName"), _T("Color") );
    magma->set_property<frantic::magma::magma_data_type>(
        outId, _T("channelType"), *frantic::magma::magma_singleton::get_named_data_type( _T("Vec3") ) );

    // Create an InputChannel node (that reads from particle channels) and configure its properties.
    frantic::magma::magma_interface::magma_id inputId = magma->create_node( _T("InputChannel") );
    magma->set_property<frantic::tstring>( inputId, _T("channelName"), _T("Color") );
    magma->set_property<frantic::magma::magma_data_type>(
        inputId, _T("channelType"), *frantic::magma::magma_singleton::get_named_data_type( _T("Vec3") ) );

    // Create an Add node (that adds two values) and configure its inputs
    frantic::magma::magma_interface::magma_id addId = magma->create_node( _T("Add") );
    magma->set_input( addId, 0, inputId, 0 );
    magma->set_input_default_value( addId, 1, frantic::graphics::vector3f( 0.25f, 0, 0 ) );

    // Connect the Add node to the Output node.
    magma->set_input( outId, 0, addId, 0 );

    // We now have a graph like so:
    //
    // InputChannel (Color float32[3]) -              -> Output (Color float32[3])
    //                                  \    Add     /
    //                                   -> in|out --
    //                        [0.25,0,0] -> in|
    //
    // For some hypothetical particle with a color property, this is equivalent to:
    //   particle.Color = particle.Color + [0.25, 0, 0]

    // In order for this magma 'flow' to be turned into executable code we need to feed it to a magma_compiler. We also
    // need a new subclass of magma_compiler_interface::context_base which is an object passed to the compiler to
    // provide contextual information (ex. the current scene time, or the transformation for the incoming data to
    // worldspace, etc.). The context object is passed alongside the magma_interface.

    boost::shared_ptr<sample_magma_context> ctx( new sample_magma_context );

    frantic::magma::simple_compiler::simple_particle_compiler compiler;

    // Assign the magma_interface into the compiler
    compiler.set_abstract_syntax_tree( magma );

    // Assign the context object
    compiler.set_compilation_context( ctx );

    // Set the current and potentially available channels for the particle data. The two maps can be the same if there
    // is no mechanism for requesting channels not currently stored.
    compiler.reset( particles->get_channel_map(), particles->get_channel_map() );

    // Build the expression
    try {
        compiler.build();
    } catch( const frantic::magma::magma_exception& e ) {
        std::cerr << frantic::strings::to_string( e.get_message( false ) ) << std::endl;
        return -1;
    }

    // Assuming no exceptions were thrown, we now have a valid expression to apply to particles!
    std::size_t index = 0;
    for( frantic::particles::particle_array::iterator it = particles->begin(), itEnd = particles->end(); it != itEnd;
         ++it, ++index )
        compiler.eval( *it, index );

    // TODO: You could write this particle array to a PRT file and inspect its data.
    std::cout << "Done" << std::endl;
    return 0;
}