// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/magma/magma_compiler_interface.hpp>
#include <frantic/magma/max3d/MagmaMaxContext.hpp>
#include <frantic/magma/max3d/nodes/magma_geometry_input_node.hpp>
#include <frantic/magma/nodes/magma_node_impl.hpp>

#include <frantic/max3d/convert.hpp>
#include <frantic/max3d/geometry/mesh.hpp>
#include <frantic/max3d/node_transform.hpp>
#include <frantic/max3d/particles/IMaxKrakatoaPRTObject.hpp>

#include <boost/scope_exit.hpp>

namespace frantic {
namespace max3d {
namespace geometry {
namespace detail {
void copy_mesh_normals( frantic::geometry::trimesh3& dest, Mesh& source );
}
} // namespace geometry
} // namespace max3d
} // namespace frantic

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

MSTR magma_input_geometry_node::max_impl::s_ClassName( _T("MagmaInputGeometryNode") );
// Class_ID magma_input_geometry_node::max_impl::s_ClassID(0x5cfc3c74, 0x67c95180);

class MyAccessor : public PBAccessor {
  public:
    static MyAccessor* GetMyAccessor() {
        static MyAccessor theAccessor;
        return &theAccessor;
    }

    // virtual void  Set (PB2Value &v, ReferenceMaker *owner, ParamID id, int tabIndex, TimeValue t);
    virtual void TabChanged( tab_changes changeCode, Tab<PB2Value>* tab, ReferenceMaker* owner, ParamID id,
                             int tabIndex, int count );
};

void magma_input_geometry_node::max_impl::DefineParameters( ParamBlockDesc2& paramDesc ) {
    paramDesc.AddParam( kNode, _T("node"), TYPE_INODE, 0, 0, p_end );
    paramDesc.AddParam( kNodes, _T("nodes"), TYPE_INODE_TAB, 0, 0, 0, p_end );

    paramDesc.ParamOption( kNodes, p_accessor, MyAccessor::GetMyAccessor(), p_end );
}

void MyAccessor::TabChanged( tab_changes changeCode, Tab<PB2Value>* /*tab*/, ReferenceMaker* owner, ParamID id,
                             int tabIndex, int count ) {
    magma_input_geometry_node::max_impl* impl = static_cast<magma_input_geometry_node::max_impl*>( owner );

    if( id == magma_input_geometry_node::kNodes ) {
        switch( changeCode ) {
        case tab_insert:
            impl->m_cachedMeshes.insert(
                impl->m_cachedMeshes.begin() + tabIndex, count,
                boost::make_tuple( NEVER, NEVER, magma_input_geometry_node::max_impl::mesh_ptr() ) );
            break;
        case tab_append:
            impl->m_cachedMeshes.insert(
                impl->m_cachedMeshes.end(), count,
                boost::make_tuple( NEVER, NEVER, magma_input_geometry_node::max_impl::mesh_ptr() ) );
            break;
        case tab_delete:
            impl->m_cachedMeshes.erase( impl->m_cachedMeshes.begin() + tabIndex,
                                        impl->m_cachedMeshes.begin() + tabIndex + count );
            break;
        case tab_ref_deleted:
            impl->m_cachedMeshes[tabIndex].get<0>() = NEVER;
            impl->m_cachedMeshes[tabIndex].get<1>() = NEVER;
            impl->m_cachedMeshes[tabIndex].get<2>().reset();
            break;
        case tab_setcount:
            impl->m_cachedMeshes.resize( count );
            break;
        }
    }
}

void magma_input_geometry_node::max_impl::SetReference( int i, RefTargetHandle rtarg ) {
    MagmaMaxNodeExtension<max_impl>::SetReference( i, rtarg );

    if( i == 0 && m_pblock )
        m_cachedMeshes.resize( m_pblock ? m_pblock->Count( kNodes ) : 0,
                               boost::make_tuple( NEVER, NEVER, mesh_ptr() ) );
}

IOResult magma_input_geometry_node::max_impl::Load( ILoad* iload ) {
    class PLC : public PostLoadCallback {
        AnimHandle handle;

      public:
        PLC( AnimHandle _handle )
            : handle( _handle ) {}

        virtual int Priority() { return 6; }

        virtual void proc( ILoad* /*iload*/ ) {
            if( Animatable* anim = Animatable::GetAnimByHandle( handle ) ) {
                if( anim->ClassID() == magma_input_geometry_node::max_impl::s_ClassID ) {
                    magma_input_geometry_node::max_impl* impl =
                        static_cast<magma_input_geometry_node::max_impl*>( anim );
                    if( impl->m_pblock && impl->m_pblock->Count( kNodes ) == 0 ) {
                        INode* node = impl->m_pblock->GetINode( kNode );
                        if( node )
                            impl->m_pblock->Append( kNodes, 1, &node );
                    }
                }
            }

            delete this;
        }
    };

    IOResult r = MagmaMaxNodeExtension<max_impl>::Load( iload );

    if( r != IO_ERROR )
        iload->RegisterPostLoadCallback( new PLC( Animatable::GetHandleByAnim( this ) ) );

    return r;
}

RefResult magma_input_geometry_node::max_impl::NotifyRefChanged( const Interval& /*changeInt*/, RefTargetHandle hTarget,
                                                                 PartID& partID, RefMessage message,
                                                                 BOOL /*propagate*/ ) {
    if( hTarget == m_pblock ) {
        int index;
        ParamID paramID = m_pblock->LastNotifyParamID( index );

        if( message == REFMSG_CHANGE ) {
            if( paramID == kNodes ) {
                if( index < 0 )
                    return REF_SUCCEED;

                if( (std::size_t)index < m_cachedMeshes.size() ) {
                    if( ( partID &
                          ( PART_GEOM | PART_TOPO | PART_TEXMAP |
                            PART_SELECT ) ) ) { // Apparently TM controllers send PART_ALL so this gets hit every time.
                        m_cachedMeshes[index].get<0>() = NEVER;
                        m_cachedMeshes[index].get<1>() = NEVER;
                    } else if( partID & PART_TM ) {
                        m_cachedMeshes[index].get<0>() = NEVER;
                    }
                }
            }

            return REF_SUCCEED;
        }
    }
    return REF_DONTCARE;
}

magma_input_geometry_node::max_impl::max_impl() {}

std::size_t magma_input_geometry_node::max_impl::get_count() const { return (std::size_t)m_pblock->Count( kNodes ); }

Interval magma_input_geometry_node::max_impl::get_validity( TimeValue t ) const {
    Interval result = MagmaMaxNodeExtension<max_impl>::get_validity( t );

    for( std::vector<boost::tuple<Interval, Interval, mesh_ptr>>::const_iterator it = m_cachedMeshes.begin(),
                                                                                 itEnd = m_cachedMeshes.end();
         it != itEnd; ++it ) {
        result &= it->get<0>();
        result &= it->get<1>();
    }

    return result;
}

void magma_input_geometry_node::max_impl::reset_validity() { m_cachedValidity = FOREVER; }

void magma_input_geometry_node::max_impl::update_validity( Interval iv ) { m_cachedValidity &= iv; }

// magma_input_geometry_node::max_impl::mesh_ptr magma_input_geometry_node::max_impl::get_mesh( TimeValue t, std::size_t
// index, Matrix3* pOutTM ){
bool magma_input_geometry_node::max_impl::get_mesh( TimeValue t, std::size_t index, magma_geometry_ptr& inoutMeshPtr ) {
    if( m_cachedMeshes[index].get<0>().InInterval( t ) && m_cachedMeshes[index].get<1>().InInterval( t ) &&
        m_cachedMeshes[index].get<2>().get() == &inoutMeshPtr->get_mesh() )
        return true; // m_cachedMeshes[index].get<2>();

    INode* node = m_pblock->GetINode( kNodes, t, (int)index );

    if( m_cachedMeshes[index].get<1>().InInterval( t ) &&
        m_cachedMeshes[index].get<2>().get() == &inoutMeshPtr->get_mesh() ) {
        m_cachedMeshes[index].get<0>().SetInfinite();

        if( !node )
            return false; // m_cachedMeshes[index].get<2>();

        Interval tmValid = FOREVER;
        Matrix3 tm = node->GetObjTMAfterWSM( t, &tmValid );
        // if( pOutTM )
        //	*pOutTM = tm;

        inoutMeshPtr->set_toworld_transform( frantic::max3d::from_max_t( tm ) );

        m_cachedMeshes[index].get<0>() &= tmValid;
    } else {
        m_cachedMeshes[index].get<0>().SetInfinite();
        m_cachedMeshes[index].get<1>().SetInfinite();
        m_cachedMeshes[index].get<2>().reset( new frantic::max3d::geometry::MaxMeshInterface );

        inoutMeshPtr = magma_geometry_interface::create_instance( m_cachedMeshes[index].get<2>() );

        if( !node )
            return false; // m_cachedMeshes[index].get<2>();

        FF_LOG( debug ) << _T("Rebuilding InputGeometry cache for: ") << node->GetName() << std::endl;

        ObjectState os = node->EvalWorldState( t );
        if( !os.obj || !os.obj->CanConvertToType( triObjectClassID ) )
            return false; // m_cachedMeshes[index].get<2>();

        TriObject* geomObj = (TriObject*)os.obj->ConvertToType( t, triObjectClassID );
        BOOL deleteObj = ( geomObj != os.obj );

        BOOST_SCOPE_EXIT( (deleteObj)( geomObj ) ) {
            if( deleteObj )
                geomObj->MaybeAutoDelete(); // This is the SDK described appropriate way to delete a ReferenceTarget
                                            // subclass.
        }
        BOOST_SCOPE_EXIT_END

        Interval tmValid = FOREVER;
        Matrix3 tm = node->GetObjTMAfterWSM( t, &tmValid );
        // if( pOutTM )
        //	*pOutTM = tm;

        inoutMeshPtr->set_toworld_transform( frantic::max3d::from_max_t( tm ) );

        m_cachedMeshes[index].get<0>() &= tmValid;
        m_cachedMeshes[index].get<1>() &= os.obj->ObjectValidity( t );
        m_cachedMeshes[index].get<2>()->set_mesh( new Mesh( geomObj->GetMesh() ), true );
    }

    // return m_cachedMeshes[index].get<2>();
    return true;
}

void magma_input_geometry_node::compile_as_extension_type( frantic::magma::magma_compiler_interface& compiler ) {
    m_cachedTime = TIME_NegInfinity;
    compiler.get_context_data().get_property( _T("Time"), m_cachedTime );

    max_impl& impl = *static_cast<max_impl*>( this->get_max_object() );

    m_meshData.resize( impl.get_count() );

    if( m_meshData.empty() )
        throw magma_exception() << magma_exception::node_id( get_id() ) << magma_exception::property_name( _T("nodes") )
                                << magma_exception::error_name( _T("InputGeometry source list cannot be empty") );

    Matrix3 tm;

    for( std::size_t i = 0, iEnd = impl.get_count(); i < iEnd; ++i ) {
        if( !impl.get_mesh( m_cachedTime, i, m_meshData[i] ) || !m_meshData[i] || &m_meshData[i]->get_mesh() == NULL ||
            !m_meshData[i]->get_mesh().is_valid() )
            throw magma_exception() << magma_exception::node_id( get_id() )
                                    << magma_exception::property_name( _T("nodes") )
                                    << magma_exception::error_name( _T("InputGeometry source #") +
                                                                    boost::lexical_cast<frantic::tstring>( i ) +
                                                                    _T(" is invalid") );
    }

    std::vector<INode*> objects;
    get_nodes( objects );
    if( objects.empty() )
        throw magma_exception() << magma_exception::node_id( get_id() ) << magma_exception::property_name( _T("node") )
                                << magma_exception::error_name( _T("Object source list cannot be empty") );

    static_cast<max_impl*>( this->get_max_object() )->reset_validity();

    compiler.get_context_data().get_property( _T("Time"), m_cachedTime );
    compiler.register_interface( get_id(),
                                 static_cast<frantic::magma::nodes::magma_input_geometry_interface*>( this ) );
    compiler.compile_constant( get_id(), (int)m_meshData.size() );
    compiler.register_interface( get_id(), static_cast<frantic::magma::nodes::magma_input_objects_interface*>( this ) );
}

std::size_t magma_input_geometry_node::size() const { return m_meshData.size(); }

magma_geometry_ptr magma_input_geometry_node::get_geometry( std::size_t index ) const { return m_meshData[index]; }

namespace {
class get_property_visitor : public boost::static_visitor<bool>, boost::noncopyable {
    void* m_outValue;
    const std::type_info& m_outType;

  public:
    get_property_visitor( const std::type_info& outType, void* outValue )
        : m_outType( outType )
        , m_outValue( outValue ) {}

    bool operator()( boost::blank ) { return false; }

    template <class T>
    bool operator()( T val ) {
        if( m_outType != typeid( T ) )
            return false;

        *reinterpret_cast<T*>( m_outValue ) = val;

        return true;
    }
};
} // namespace

bool magma_input_geometry_node::get_property_internal( std::size_t index, const frantic::tstring& propName,
                                                       const std::type_info& typeInfo, void* outValue ) {
    std::vector<INode*> objects;
    get_nodes( objects );
    INode* node = objects[index];
    if( !node )
        return false;

    Interval iv = FOREVER;

    if( propName == _T("Transform") && typeid( frantic::graphics::transform4f ) == typeInfo ) {
        *reinterpret_cast<frantic::graphics::transform4f*>( outValue ) =
            frantic::max3d::from_max_t( frantic::max3d::get_node_transform( node, m_cachedTime, iv ) );

        static_cast<max_impl*>( this->get_max_object() )->update_validity( iv );

        return true;
    }

    // Fall back on the variant based approach if we don't have a specific one in mind.

    variant_t result;
    this->get_property( index, propName, result );

    get_property_visitor visitor( typeInfo, outValue );

    return boost::apply_visitor( visitor, result );
}

void magma_input_geometry_node::get_property( std::size_t index, const frantic::tstring& propName,
                                              variant_t& outValue ) {
    std::vector<INode*> objects;
    get_nodes( objects );
    ReferenceTarget* rtarg = objects[index];
    if( !rtarg )
        return;

    Value* result = frantic::max3d::mxs::expression( _T("try(theObj.") + propName + _T(")catch(undefined)") )
                        .bind( _T("theObj"), rtarg )
                        .at_time( m_cachedTime )
                        .evaluate<Value*>();
    if( !result || result == &undefined )
        return;

    // We currently have no idea what the validity of the property we just grabbed is, so we have to assume its instant.
    static_cast<max_impl*>( this->get_max_object() )->update_validity( Interval( m_cachedTime, m_cachedTime ) );

    if( is_int( result ) ) {
        outValue = result->to_int();
    } else if( is_number( result ) ) {
        outValue = result->to_float();
    } else if( is_bool( result ) ) {
        outValue = result->to_bool();
    } else if( is_point3( result ) ) {
        outValue = frantic::max3d::from_max_t( result->to_point3() );
    } else if( is_color( result ) ) {
        outValue = frantic::max3d::from_max_t( (Point3)result->to_acolor() );
    } else if( is_quat( result ) ) {
        outValue = frantic::max3d::from_max_t( result->to_quat() );
    }
}
} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic

using frantic::magma::max3d::MagmaMaxNodeExtension;

template <>
MagmaMaxNodeExtension<frantic::magma::nodes::max3d::magma_input_geometry_node::max_impl>::MyClassDesc
    MagmaMaxNodeExtension<frantic::magma::nodes::max3d::magma_input_geometry_node::max_impl>::s_classDesc;

ClassDesc2* GetMagmaInputGeometryNodeDesc() {
    return &frantic::magma::nodes::max3d::magma_input_geometry_node::max_impl::s_classDesc;
}
