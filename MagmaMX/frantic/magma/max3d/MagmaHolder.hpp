// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/max3d/IMagmaHolder.hpp>
#include <frantic/max3d/GenericReferenceTarget.hpp>

#include <map>
#include <memory>

#define MagmaHolderDeprecated_INTERFACE Interface_ID( 0x6c8d6eeb, 0x6c803022 )

namespace frantic {
namespace magma {
namespace max3d {

/**
 * This interface is provided to expose old methods that are needed to support MAXScript code that is currently in use
 * by 3rd party developers, but we do not want used when creating new Magma scripts.
 */
class IMagmaHolderDeprecated : public FPMixinInterface {
  public:
    enum {
        CURRENT_BLOP = -2,
    };

    virtual ~IMagmaHolderDeprecated() {}

    // From FPMixinInterface
    virtual FPInterfaceDesc* GetDesc() { return GetDescByID( MagmaHolderDeprecated_INTERFACE ); }

    virtual FPInterfaceDesc* GetDescByID( Interface_ID id ) = 0;

    IMagmaHolder::magma_id get_current_blop_id() const { return CURRENT_BLOP; }

    /**
     * Typically used with BLOP nodes for determinging which node is connected to the N'th output socket.
     * @param id Which node to query. Should probably only accept IMagmaHolderDeprecated::CURRENT_BLOP
     * @param index Which output connection information to retrieve.
     * @return A pair containing the node ID and output socket index. The node ID can be magma_interface::INVALID_ID if
     * the output is not connected.
     */
    virtual Value* deprecated_get_output( IMagmaHolder::index_type index ) const = 0;

    /**
     * Typically used with BLOP nodes. Will connect the "internal" output node's N'th socket to the specified node.
     * @param id The ID of the BLOP to modify. Should probably only accept IMagmaHolderDeprecated::CURRENT_BLOP.
     * @param index The index of the implicit output socket/node to connect.
     * @param connectedID The ID of the node to connect to.
     * @param connectedIndex The index of the socket (of node w/ ID 'connectedID') to connect to.
     * @return False if the connection could not be made. This might happen if index or connectedIndex is out of range,
     * connectedID is invalid, or we are at the top-level imaginary BLOP (since it has its own outputs).
     */
    virtual bool deprecated_set_output( IMagmaHolder::index_type index, IMagmaHolder::magma_id connectedID,
                                        IMagmaHolder::index_type connectedIndex ) = 0;

    /**
     * An alias for IMagmaHolder::add_node_property
     */
    virtual bool deprecated_declare_extension_property( IMagmaHolder::magma_id id, const MCHAR* propName ) = 0;

  public:
    enum { kPropCurrentBLOP, kFnGetOutput, kFnSetOutput, kFnDeclareExtensionProperty };

#pragma warning( push )
#pragma warning( disable : 4238 4100 )
    BEGIN_FUNCTION_MAP
    RO_PROP_FN( kPropCurrentBLOP, get_current_blop_id, TYPE_MAGMA_ID )
    FN_1( kFnGetOutput, TYPE_VALUE, deprecated_get_output, TYPE_INDEX )
    FN_3( kFnSetOutput, TYPE_bool, deprecated_set_output, TYPE_INDEX, TYPE_MAGMA_ID, TYPE_INDEX )
    FN_2( kFnDeclareExtensionProperty, TYPE_bool, deprecated_declare_extension_property, TYPE_MAGMA_ID, TYPE_STRING )
    END_FUNCTION_MAP;
#pragma warning( pop )

    friend class MagmaHolderClassDesc;

    inline static void init_fpinterface_desc( FPInterfaceDesc& desc ) {
        desc.AppendProperty( kPropCurrentBLOP, FP_NO_FUNCTION, _T("CurrentBLOP"), 0, TYPE_MAGMA_ID, p_end );
        desc.AppendFunction( kFnGetOutput, _T("GetOutput"), 0, TYPE_VALUE, 0, 1, _T("InputIndex"), 0, TYPE_INDEX,
                             p_end );
        desc.AppendFunction( kFnSetOutput, _T("SetOutput"), 0, TYPE_bool, 0, 3, _T("InputIndex"), 0, TYPE_INDEX,
                             _T("ConnectedID"), 0, TYPE_MAGMA_ID, _T("ConnectedSocket"), 0, TYPE_INDEX, p_end );
        desc.AppendFunction( kFnDeclareExtensionProperty, _T("DeclareExtensionProperty"), 0, TYPE_bool, 0, 2,
                             _T("NodeID"), 0, TYPE_MAGMA_ID, _T("PropertyName"), 0, TYPE_STRING, p_end );
    }
};

class IMagmaNode;

/**
 * This class is a concrete implementation of IMagmaHolder. Implementors of DLLs that want to have Magma should subclass
 * this and provide a new ClassDesc that is a subclass of MagmaHolderClassDesc.
 */
class MagmaHolder : public GenericReferenceTarget<ReferenceTarget, MagmaHolder>,
                    public IMagmaHolder,
                    public IMagmaHolderDeprecated {
  public:
    MagmaHolder( std::unique_ptr<frantic::magma::magma_interface> magmaInterface );

    virtual ~MagmaHolder();

    // Resets 'this' as if a new object were created with the supplied magma.
    void reset( std::unique_ptr<frantic::magma::magma_interface> magmaInterface );

    // From FPMixinInterface
    virtual FPInterfaceDesc* GetDescByID( Interface_ID id );

    // From Animatable
    virtual BaseInterface* GetInterface( Interface_ID id );

    // From ReferenceMaker
    virtual IOResult Load( ILoad* iload ); // In MagmaHolderIO.cpp
    virtual IOResult Save( ISave* isave ); // In MagmaHolderIO.cpp
    virtual RefResult NotifyRefChanged( const Interval& changeInt, RefTargetHandle hTarget, PartID& partID,
                                        RefMessage message, BOOL propagate );

    // From ReferenceTarget
    virtual void BaseClone( ReferenceTarget* from, ReferenceTarget* to, RemapDir& remap );

    //*************************************************************************************************************
    // From IMagmaHolder
    //*************************************************************************************************************
#pragma region From IMagmaHolder
    virtual boost::shared_ptr<magma_interface> get_magma_interface();

    virtual Interval get_validity( TimeValue t ) const;

    virtual bool is_read_only() const;

    virtual bool is_loading() const;

    virtual void set_is_loading( bool loading = true );

    virtual void reset();

    virtual magma_id create_node( const MCHAR* typeName );

    virtual bool delete_node( magma_id id );

    virtual bool replace_node( magma_id idDest, magma_id idSrc );

    virtual const MCHAR* get_node_type( magma_id id ) const;

    virtual bool is_node_container( magma_id id ) const;

    virtual bool is_node_creatable( magma_id id ) const;

    virtual Tab<const MCHAR*> get_node_property_names( magma_id id,
                                                       TYPE_ENUM_TYPE propType = property_type::all ) const;

    virtual MSTR get_node_property_type( magma_id id, const MCHAR* propName ) const;

    virtual bool get_node_property_readonly( magma_id id, const MCHAR* propName ) const;

    virtual bool add_node_property( magma_id id, const MCHAR* propName );

    virtual FPValue get_node_property( magma_id id, const MCHAR* propName ) const;

    virtual bool set_node_property( magma_id id, const MCHAR* propName, const FPValue& val );

    virtual Tab<const MCHAR*> get_node_enum_values( magma_id id, const MCHAR* propName ) const;

    virtual int get_num_node_inputs( magma_id id ) const;

    virtual bool set_num_node_inputs( magma_id id, int numInputs );

    virtual FPValue get_node_input_default_value( magma_id id, index_type socketIndex ) const;

    virtual bool set_node_input_default_value( magma_id id, index_type socketIndex, const FPValue& val );

    virtual MSTR get_node_input_description( magma_id id, index_type socketIndex ) const;

    virtual Value* get_node_input( magma_id id, index_type index ) const;

    virtual bool set_node_input( magma_id id, index_type index, magma_id connectedID, index_type connectedIndex );

    virtual int get_num_node_outputs( magma_id id ) const;

    virtual bool set_num_node_outputs( magma_id id, int numOutputs );

    virtual MSTR get_node_output_description( magma_id id, index_type socketIndex ) const;

    virtual magma_id_tab get_nodes() const;

    virtual magma_id get_container_source() const;

    virtual magma_id get_container_sink() const;

    virtual bool push_editable_BLOP( magma_id id );

    virtual magma_id pop_editable_BLOP();

    virtual int num_editing_BLOPs() const;

    virtual magma_id_tab get_BLOP_stack() const;

    virtual bool explode_blop( magma_id id );

    virtual magma_id create_blop( const magma_id_tab& nodes );

    virtual Tab<const MCHAR*> get_type_names() const;

    virtual MSTR get_type_category( const MCHAR* typeName ) const;

    virtual MSTR get_type_description( const MCHAR* typeName ) const;
#pragma endregion

    //*************************************************************************************************************
    // From IMagmaHolderDeprecated
    //*************************************************************************************************************
#pragma region From IMagmaHolderDeprecated
    virtual Value* deprecated_get_output( index_type index ) const;

    virtual bool deprecated_set_output( index_type index, magma_id connectedID, index_type connectedIndex );

    virtual bool deprecated_declare_extension_property( IMagmaHolder::magma_id id, const MCHAR* propName );
#pragma endregion

  public:
    // There is one ID per parameter block. We should propbably only have this one.
    enum { kPbMagmaHolder };

    // These are the ids for the parameter block associated with a MagmaHolder. These are here mostly for legacy
    // reasons.
    enum {
        kMagmaFlow,
        kMagmaInternalFlow,
        kMagmaNote,
        kMagmaTrackID,
        kMagmaTextureMapSources,
        kMagmaCurrentPreset,
        kMagmaCurrentFolder,
        kMagmaAutoUpdate,
        kMagmaInteractiveMode,
        kMagmaAutomaticRenameOFF,
        kMagmaGeometryObjectsList,
        kMagmaNeedCAUpdate,
        kMagmaIsRenderElement,

        kMagmaLastErrorMessage,
        kMagmaLastErrorNode,

        kMagmaNodeMaxDataDEPRECATED, // MAX-specific data, stored as a sparse tab of sorted RefTargets of type
                                     // MagmaNodeMaxData
        kMagmaNodeMaxData
    };

  protected:
    // This class is left pure virtual so that application specific implmentors are forced to overload this class and
    // provide a ClassDesc for it.
    virtual ClassDesc2* GetClassDesc() = 0;

  private:
    typedef std::map<magma_id, std::map<M_STD_STRING, FPValue>> ext_prop_map_type;

    ext_prop_map_type m_extensionProps;

    boost::shared_ptr<magma_interface> m_magma;

    bool m_isLoading;

    int get_magma_max_node_index( magma_id id, bool forInsert = false );
    IMagmaNode* get_magma_max_node( magma_id id );
    void manage_magma_max_node( magma_id id );
    void replace_magma_max_node( magma_id idDest, magma_id idSrc );

    void delete_node_recursive( magma_id id );

    friend class FixRefsPLCB; // In MagmaHolderIO.cpp

    void FixRefsPLCBImpl();                         // In MagmaHolderIO.cpp
    IOResult LoadNextNode( ILoad* iload );          // In MagmaHolderIO.cpp
    IOResult SaveNode( ISave* isave, magma_id id ); // In MagmaHolderIO.cpp

    // TODO: This should be a boost::thread_specific_ptr< std::vector<std::string> > if we need to be thread-safe.
    mutable std::vector<frantic::tstring> m_cachedStrings;

    inline void get_cached_strings( Tab<const MCHAR*>& result ) const {
        result.SetCount( (int)m_cachedStrings.size() );

        int i = 0;
        for( std::vector<frantic::tstring>::const_iterator it = m_cachedStrings.begin(), itEnd = m_cachedStrings.end();
             it != itEnd; ++it, ++i )
            result[i] = it->c_str();
    }
};

/**
 * Subclass this when you need to add a MagmaHolder instance to your DLL. This cannot be shared since it would
 * cause ClassID conflicts if multiple DLLs used the same ClassDesc + ClassID
 */
class MagmaHolderClassDesc : public ClassDesc2 {
    ParamBlockDesc2 m_pbDesc;
    FPInterfaceDesc m_fpDesc, m_fpDescDeprecated;

  public:
    MagmaHolderClassDesc();

    virtual ~MagmaHolderClassDesc();

    virtual FPInterfaceDesc* GetDescByID( Interface_ID id );

    virtual const TCHAR* ClassName() = 0;
    virtual HINSTANCE HInstance() = 0;
    virtual Class_ID ClassID() = 0;
    virtual void* Create( BOOL loading ) = 0;

    virtual int IsPublic() { return FALSE; }
    virtual const TCHAR* Category() { return _T(""); }
    virtual SClass_ID SuperClassID() { return REF_TARGET_CLASS_ID; }
    virtual const TCHAR* InternalName() { return ClassName(); }
};

} // namespace max3d
} // namespace magma
} // namespace frantic
