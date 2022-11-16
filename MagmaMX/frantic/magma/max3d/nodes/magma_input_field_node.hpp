// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/max3d/MagmaMaxNodeExtension.hpp>
#undef base_type

#include <frantic/magma/nodes/magma_node_property.hpp>
#include <frantic/magma/nodes/magma_simple_operator.hpp>

#include <boost/shared_ptr.hpp>

namespace frantic {
namespace volumetrics {
class field_interface;
}
} // namespace frantic

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

class magma_field_input_node
    : public frantic::magma::nodes::magma_simple_operator<1, frantic::magma::nodes::max3d::magma_max_node_base> {
    typedef boost::shared_ptr<frantic::volumetrics::field_interface> field_ptr;

    std::vector<frantic::tstring> m_outputChannels; // Maps from output index to channel name.

    frantic::tstring m_fxdSequenceOverride;

  private:
    field_ptr get_field( TimeValue t ) const;

    void on_field_changed();

  public:
    enum { kNode = 1 };

  public:
    MAGMA_MAX_REQUIRED_METHODS( magma_field_input_node );
    // MAGMA_MAX_PROPERTY(node,INode,kNode);
    // MAGMA_PROPERTY(fxdSequenceOverride,frantic::tstring);

    inline INode* const get_node() const { return this->get_max_property<INode>( kNode, 0 ); }

    inline const frantic::tstring& get_fxdSequenceOverride() const { return m_fxdSequenceOverride; }

    // Expose the channels that can be accessed from the current field.
    void get_availableChannels( std::vector<frantic::tstring>& ) const;

    // Expose the names of channels currently exposed as outputs in the order of exposure. (ie. value[0] corresponds to
    // the first output socket)
    inline const std::vector<frantic::tstring>& get_channels() const { return m_outputChannels; }

    inline void set_node( INode* node );

    inline void set_fxdSequenceOverride( const frantic::tstring& val );

    // Set the outputs
    inline void set_channels( const std::vector<frantic::tstring>& channels );

    magma_field_input_node();

    virtual ~magma_field_input_node() {}

    virtual int get_num_outputs() const;

    virtual void get_output_description( int i, frantic::tstring& outDescription ) const;

  public:
    class max_impl : public frantic::magma::max3d::MagmaMaxNodeExtension<magma_field_input_node::max_impl> {
      public:
        static MSTR s_ClassName;
        static Class_ID s_ClassID;

        static void DefineParameters( ParamBlockDesc2& paramDesc );

        // Override this to capture changes to the parameter block
        virtual void SetReference( int i, RefTargetHandle rtarg );
        virtual int RenderBegin( TimeValue t, ULONG flags );
        virtual int RenderEnd( TimeValue t );
        virtual IOResult Load( ILoad* iload );

        // Want to know when parameters and references are changing
        virtual RefResult NotifyRefChanged( const Interval& changeInt, RefTargetHandle hTarget, PartID& partID,
                                            RefMessage message, BOOL propagate );

        max_impl();

        field_ptr get_field( TimeValue t );
        field_ptr get_field( TimeValue t, const frantic::tstring& fxdSequence );

        // Get validity of held meshes
        virtual Interval get_validity( TimeValue t ) const;

      private:
        std::pair<field_ptr, Interval> m_cache;

        friend class MyAccessor;
    };
};

} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic

ClassDesc2* GetFieldNodeDesc();
