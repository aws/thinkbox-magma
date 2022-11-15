// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <deque>
#include <map>
#include <vector>

#include <boost/any.hpp>

#include <frantic/channels/channel_map.hpp>
#include <frantic/magma/simple_compiler/base_compiler.hpp>

namespace frantic {
namespace magma {
namespace simple_compiler {

class simple_particle_compiler : public base_compiler {
    // List of all possible input channels. New output channels should append themselves
    // to this channel map so that external consumers are aware of the results of this operation.
    frantic::channels::channel_map m_nativeMap;

    // List of all current input channels. Can be modified by appending extra channels if they
    // are required as inputs. Channels should only be appended if they exist in the native map.
    frantic::channels::channel_map m_channelMap;

    // If the index channel is requested by the stream, this accessor will be populated.
    frantic::channels::channel_cvt_accessor<int>
        m_indexAccessor; // TODO: This should probably be an int64 at the very least.

  public:
    simple_particle_compiler() {
        m_nativeMap.end_channel_definition();
        m_channelMap.end_channel_definition();
    }

    virtual ~simple_particle_compiler() {}

    frantic::channels::channel_map& get_channel_map() { return m_channelMap; }

    frantic::channels::channel_map& get_native_channel_map() { return m_nativeMap; }

    const frantic::channels::channel_map& get_channel_map() const { return m_channelMap; }

    const frantic::channels::channel_map& get_native_channel_map() const { return m_nativeMap; }

    /**
     * Resets the compiler to the same state as if a new object was created.
     * @param pcm Same as in channel_operation_compiler( pcm, nativePcm )
     * @param nativePcm Same as in channel_operation_compiler( pcm, nativePcm )
     */
    void reset( const frantic::channels::channel_map& pcm, const frantic::channels::channel_map& nativePcm );

    /**
     * @deprecated
     */
    void reset( magma_interface& mi, const frantic::channels::channel_map& pcm,
                const frantic::channels::channel_map& nativePcm, boost::shared_ptr<context_base> contextData );

    /**
     * This function will evaluate the code segments produced by compiling an AST, using the
     * supplied pointer as input values laid out according to the internal pcm. The output
     * of the channel operation will be in a particle channel.
     *
     * @param particle A pointer to a particle with memory layout as described by internalPcm.
     */
    void eval( char* particle, std::size_t index ) const;

    /**
     * This function will evaluate each code segment, extracting its output value and storing it into the supplied
     * vector. This is used for debugging purposes to inspect the value at each stage. The results are per-node,
     * per-output, and are stored as a vector of boost::any objects.
     * @param particle The data external to the compiled expression.
     * @param index The index of the currently evaluated particle.
     * @param[out] outResultsPerNode Debug data is stored into this object.
     */
    void eval_debug( char* particle, std::size_t index, debug_data& outValues ) const;

  public:
    /**
     * Registers an expression that extracts a particle channel ( ie. from the particle passed as a parameter to eval()
     * ). Can also verify that the channel is a specific type.
     * @exprID The id of the new expression to create.
     * @channelName The name of the particle's channel to extract
     * @expectedType The type that the particle's channel should be. An exception is thrown if it doesn't match. Can be
     * NULL if you don't care.
     */
    virtual void compile_input_channel( expression_id exprID, const frantic::tstring& channelName,
                                        const magma_data_type* expectedType = NULL );

    /**
     * Registers an expression that extracts a particle channel. Assigns a non-conflicting expression ID and and value
     * index.
     * @param channelName The particle channel to extract
     * @channelName The name of the particle's channel to extract
     * @expectedType The type that the particle's channel should be. An exception is thrown if it doesn't match. Can be
     * NULL if you don't care.
     * @return The expression ID and value index of the created expression.
     */
    virtual std::pair<expression_id, int> compile_default_input_channel( const frantic::tstring& channelName,
                                                                         const magma_data_type* expectedType = NULL );

    /**
     * Registers an expression that writes its input value to a particle channel ( ie. the one passed as a parameter to
     * eval() ). Can also verify that the channel is a specific type.
     * @exprID The id of the new expression to create.
     * @inputValue The id & value index of the expression to write to the particle.
     * @channelName The name of the particle's channel to write to.
     * @expectedType The type that the particle's channel should be if it isn't already in the native channel map.
     */
    virtual void compile_output( expression_id exprID, const std::pair<expression_id, int>& inputValue,
                                 const frantic::tstring& channelName, const magma_data_type& expectedType );

    virtual void compile( nodes::magma_input_channel_node* );

    virtual void compile( nodes::magma_output_node* );

  private:
    void compile_index_channel( expression_id exprID, const magma_data_type* expectedType = NULL );
};

} // namespace simple_compiler
} // namespace magma
} // namespace frantic
