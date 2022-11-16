// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/nodes/magma_simple_operator.hpp>

#include <frantic/magma/max3d/IMagmaHolder.hpp>

#include <frantic/max3d/GenericReferenceTarget.hpp>

#include <iparamb2.h>

#include <boost/scope_exit.hpp>

#include <memory>

extern HINSTANCE hInstance;

namespace frantic {
namespace magma {
namespace max3d {

class IMagmaNode : public ReferenceTarget {
  public:
    enum enumParamBlockIDs { kParamBlockID = 0 };

    enum enumParamIDs { kID = 0 };

    virtual ~IMagmaNode() {}

    virtual Interval get_validity( TimeValue t ) const = 0;
    virtual IMagmaHolder::magma_id get_id() const = 0;
    virtual void set_id( IMagmaHolder::magma_id id ) = 0;
};

inline IMagmaNode* GetMagmaNodeInterface( ReferenceTarget* rtarg ) { return static_cast<IMagmaNode*>( rtarg ); }

template <class BaseType>
class MagmaMaxNodeExtension : public GenericReferenceTarget<IMagmaNode, BaseType> {
  private:
    class MyClassDesc : public ClassDesc2 {
        ParamBlockDesc2 m_pbDesc;

      public:
        MyClassDesc( int version = 0 );

        int IsPublic() { return FALSE; }
        void* Create( BOOL /*loading*/ ) { return new BaseType; }
        const TCHAR* ClassName() { return BaseType::s_ClassName; }
#if MAX_VERSION_MAJOR >= 24
        const TCHAR* NonLocalizedClassName() { return BaseType::s_ClassName; }
#endif
        SClass_ID SuperClassID() { return REF_TARGET_CLASS_ID; }
        Class_ID ClassID() { return BaseType::s_ClassID; }
        const TCHAR* Category() { return _T(""); }

        const TCHAR* InternalName() {
            return BaseType::s_ClassName;
        } // returns fixed parsable name (scripter-visible name)
        HINSTANCE HInstance() { return hInstance; }
    };

  protected:
    virtual ClassDesc2* GetClassDesc() { return &s_classDesc; }

  public:
    static MyClassDesc s_classDesc;

  public:
    MagmaMaxNodeExtension() { s_classDesc.MakeAutoParamBlocks( this ); }

    virtual ~MagmaMaxNodeExtension() {}

    virtual Interval get_validity( TimeValue t ) const {
        Interval iv = FOREVER;

        m_pblock->GetValidity( t, iv );

        return iv;
    }

    virtual IMagmaHolder::magma_id get_id() const { return m_pblock->GetInt( kID ); }

    virtual void set_id( IMagmaHolder::magma_id id ) { m_pblock->SetValue( kID, 0, id ); }
};

template <class BaseType>
MagmaMaxNodeExtension<BaseType>::MyClassDesc::MyClassDesc( int version )
    : m_pbDesc( kParamBlockID, _T("Parameters"), /*IDS_PARAMETERS*/ NULL, NULL, P_VERSION | P_AUTO_CONSTRUCT, version,
                0, p_end ) {
    m_pbDesc.SetClassDesc( this );
    m_pbDesc.AddParam( kID, _T("id"), TYPE_MAGMA_ID, 0, 0, p_end );
    m_pbDesc.ParamOption( kID, p_default, magma_interface::INVALID_ID );

    BaseType::DefineParameters( m_pbDesc );
}
} // namespace max3d
} // namespace magma
} // namespace frantic

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

using frantic::magma::max3d::IMagmaNode;
using frantic::magma::max3d::MagmaMaxNodeExtension;

class magma_max_node_base : public magma_node_base {
  public:
    magma_max_node_base();

    virtual ~magma_max_node_base();

    /**
     * We need to know the class ID of the 3ds Max object associated with this node.
     * @return The Class_ID of the node that implements the 3dsMax interface for this node.
     */
    virtual Class_ID get_class_id() const = 0;

    /**
     * We may need to access the 3ds Max object associated with this node directly.
     * @return The object that implements the 3ds Max part of this node's implementation.
     */
    virtual IMagmaNode* get_max_object() const;

    /**
     * 3ds Max manages the lifetime of the object associated with this node (for example, saving and loading it in the
     * containing MagmaHolder object). We notify the associated magma node with this method.
     * @param node The 3ds Max object associated with this magma_max_node.
     */
    virtual void set_max_object( IMagmaNode* node );

    virtual int get_num_inputs() const = 0;
    virtual void set_num_inputs( int numInputs ) = 0;
    virtual std::pair<magma_interface::magma_id, int> get_input( int i ) const = 0;
    virtual void set_input( int i, magma_interface::magma_id id, int outputIndex = 0 ) = 0;
    virtual void compile( magma_compiler_interface& compiler ) = 0;

    // These methods are provided to support the existing property exposure mechanism.
    inline Class_ID get__maxImplClassID() const { return this->get_class_id(); }
    inline ReferenceTarget* const get__maxImpl() const { return this->get_max_object(); }
    inline void set__maxImpl( ReferenceTarget* const val ) { this->set_max_object( static_cast<IMagmaNode*>( val ) ); }

  private:
    // We aren't going to store a direct pointer, but instead access it through a AnimHandle which is safer and protects
    // us against accessing a deleted object.
    AnimHandle m_maxObjHandle;

    IParamBlock2* get_parameters() const;

  protected:
    template <class RefTargType>
    inline RefTargType* get_max_property( ParamID id, int tabIndex = 0 ) const;

    template <class RefTargType>
    inline void get_max_property( ParamID id, std::vector<RefTargType*>& outValues ) const;

    template <class RefTargType>
    inline bool set_max_property( ParamID id, RefTargType* val );

    template <class RefTargType>
    inline bool set_max_property( ParamID id, const std::vector<RefTargType*>& val );
};

template <class RefTargType>
RefTargType* magma_max_node_base::get_max_property( ParamID id, int tabIndex ) const {
    RefTargType* result = NULL;

    if( IParamBlock2* pb = get_parameters() ) {
        Interval resultValid = FOREVER;
        pb->GetValue( id, TIME_NegInfinity, result, resultValid, tabIndex );
    }

    return result;
}

template <class RefTargType>
void magma_max_node_base::get_max_property( ParamID id, std::vector<RefTargType*>& outValues ) const {
    outValues.clear();

    if( IParamBlock2* pb = get_parameters() ) {
        RefTargType* val;
        for( int i = 0, iEnd = pb->Count( id ); i < iEnd; ++i ) {
            Interval resultValid = FOREVER;
            if( pb->GetValue( id, TIME_NegInfinity, val, resultValid, i ) )
                outValues.push_back( val );
        }
    }
}

template <class RefTargType>
bool magma_max_node_base::set_max_property( ParamID id, RefTargType* val ) {
    if( IParamBlock2* pb = get_parameters() )
        return pb->SetValue( id, TIME_NegInfinity, val ) != FALSE;

    return false;
}

template <class RefTargType>
bool magma_max_node_base::set_max_property( ParamID id, const std::vector<RefTargType*>& val ) {
    if( IParamBlock2* pb = get_parameters() ) {
        int dest = 0;

        // We want to minimize the changes to the parameter block because each one will fire a separate PBAccessor
        // callback, and those have an unknown complexity.

        // Increase the container size to accomodate all items.
        if( pb->Count( id ) < (int)val.size() )
            pb->SetCount( id, (int)val.size() );

        // Set each element in the container, but don't advance the output location if SetValue returns false. This
        // means there was a circular reference or somesuch and it stored a NULL pointer which we don't want.
        for( std::vector<RefTargType*>::const_iterator it = val.begin(), itEnd = val.end(); it != itEnd; ++it ) {
            if( pb->SetValue( id, 0, *it, dest ) )
                ++dest;
        }

        // Resize the container to hold exactly the number of items we added.
        if( dest < pb->Count( id ) )
            pb->SetCount( id, dest );

        // Return true if any new values were set, otherwise return false. This doesn't report times when some values
        // failed to be added.
        return dest > 0 || val.empty();
    }

    return false;
}

typedef magma_simple_operator<0, magma_max_node_base> magma_max_input_node;

} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic

#define MAGMA_MAX_PROPERTY( name, type, paramID )                                                                      \
  public:                                                                                                              \
    inline type* const get_##name() const { return this->get_max_property<type>( paramID, 0 ); }                       \
    inline void set_##name( type* val ) { this->set_max_property( paramID, val ); }

#define MAGMA_MAX_ARRAY_PROPERTY( name, type, paramID )                                                                \
  public:                                                                                                              \
    inline void get_##name( std::vector<type*>& outVals ) const { this->get_max_property( paramID, outVals ); }        \
    inline void set_##name( const std::vector<type*>& vals ) { this->set_max_property( paramID, vals ); }

#define MAGMA_MAX_REQUIRED_METHODS( className )                                                                        \
    MAGMA_REQUIRED_METHODS( className );                                                                               \
                                                                                                                       \
    virtual Class_ID get_class_id() const;                                                                             \
    virtual void compile_as_extension_type( frantic::magma::magma_compiler_interface& compiler );
