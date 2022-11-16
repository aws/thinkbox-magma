// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_geometry_interface.hpp>
#include <frantic/magma/nodes/magma_input_geometry_interface.hpp>
#include <frantic/magma/nodes/magma_input_objects_interface.hpp>
#include <frantic/magma/nodes/magma_node_property.hpp>
#include <frantic/max3d/geometry/max_mesh_interface.hpp>

#include <frantic/magma/max3d/MagmaMaxNodeExtension.hpp>

#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

class magma_input_geometry_node : public magma_max_input_node,
                                  public magma_input_geometry_interface,
                                  public magma_input_objects_interface {
  public:
    enum {
        kNode = 1, // TODO: Make this multiple nodes
        kNodes
    };

    class max_impl : public MagmaMaxNodeExtension<max_impl> {
        Interval m_cachedValidity;

      public:
        typedef boost::shared_ptr<frantic::max3d::geometry::MaxMeshInterface> mesh_ptr;

      private:
        // We are keeping a parallel array of cached meshes that must match the count of m_pblock->Count( kNodes )
        // We use SetReference(), NotifyRefChanged() and a PBAccessor to manage this.
        std::vector<boost::tuple<Interval, Interval, mesh_ptr>> m_cachedMeshes;

        friend class MyAccessor;

      public:
        static MSTR s_ClassName;
        static Class_ID s_ClassID;

        static void DefineParameters( ParamBlockDesc2& paramDesc );

        // Override this to capture changes to the parameter block
        virtual void SetReference( int i, RefTargetHandle rtarg );
        virtual IOResult Load( ILoad* iload );

        // Want to know when parameters and references are changing
        virtual RefResult NotifyRefChanged( const Interval& changeInt, RefTargetHandle hTarget, PartID& partID,
                                            RefMessage message, BOOL propagate );

        max_impl();

        // Gets the number of meshes held by this object
        std::size_t get_count() const;

        // Updates the mesh ptr, returns false if the associated node was null
        bool get_mesh( TimeValue t, std::size_t i, magma_geometry_ptr& inoutMeshPtr );

        // Get validity of held meshes
        virtual Interval get_validity( TimeValue t ) const;

        virtual void reset_validity();

        virtual void update_validity( Interval iv );
    };

  private:
    // std::vector< std::pair< trimesh3_ptr, trimesh3_kdtree_ptr > > m_meshData;
    // typedef std::pair<trimesh3_ptr, trimesh3_kdtree_ptr> mesh_data;
    // typedef std::vector< mesh_data > mesh_data_list;

    // mesh_data_list m_meshData;
    std::vector<magma_geometry_ptr> m_meshData;

    TimeValue m_cachedTime;

  protected:
    virtual bool get_property_internal( std::size_t index, const frantic::tstring& propName,
                                        const std::type_info& typeInfo, void* outValue );

  public:
    MAGMA_MAX_REQUIRED_METHODS( magma_input_geometry_node );
    MAGMA_MAX_ARRAY_PROPERTY( nodes, INode, kNodes );

    virtual ~magma_input_geometry_node() {}

    virtual int get_num_outputs() const { return 3; }

    virtual std::size_t size() const;

    virtual magma_geometry_ptr get_geometry( std::size_t index ) const;

    /*virtual const_trimesh3_ptr get_mesh() const;

    virtual const_trimesh3_kdtree_ptr get_mesh_kdtree();

    virtual std::size_t size() const;

    virtual const_trimesh3_ptr get_mesh( std::size_t index ) const;

    virtual const_trimesh3_kdtree_ptr get_mesh_kdtree( std::size_t index );*/

    virtual void get_property( std::size_t index, const frantic::tstring& propName, variant_t& outValue );
};

} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic
