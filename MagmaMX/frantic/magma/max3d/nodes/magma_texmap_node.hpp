// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_node_base.hpp>
#include <frantic/magma/nodes/magma_node_property.hpp>

#include <frantic/magma/max3d/MagmaMaxNodeExtension.hpp>

#include <boost/array.hpp>

namespace frantic {
namespace magma {
namespace nodes {
namespace max3d {

class magma_texmap_node : public magma_max_node_base {
  public:
    class max_impl : public MagmaMaxNodeExtension<max_impl> {
      public:
        static MSTR s_ClassName;
        static Class_ID s_ClassID;

        static void DefineParameters( ParamBlockDesc2& paramDesc );

        virtual Interval get_validity( TimeValue t ) const;

        virtual RefResult NotifyRefChanged( const Interval& changeInt, RefTargetHandle hTarget, PartID& partID,
                                            RefMessage message, BOOL propagate );
    };

    enum {
        kTexmap = 1,
    };

    struct map_channel_data {
        int channel;
        std::pair<magma_interface::magma_id, int> connection;
        variant_t defaultValue;

        explicit map_channel_data( int ch )
            : channel( ch )
            , connection( std::make_pair( magma_interface::INVALID_ID, 0 ) )
            , defaultValue( frantic::graphics::vector3f( 0.f ) ) {}

        bool operator<( int _channel ) const { return this->channel < _channel; }
    };

    boost::array<std::pair<magma_interface::magma_id, int>, 6> m_baseConnections;
    boost::array<variant_t, 6> m_baseDefaults;

    std::vector<map_channel_data> m_mapChannelConnections;

  public:
    MAGMA_MAX_REQUIRED_METHODS( magma_texmap_node );
    MAGMA_PROPERTY( resultType, M_STD_STRING );
    inline void get_mapChannels( std::vector<int>& outChannels ) const;
    inline void set_mapChannels( const std::vector<int>& channels );
    MAGMA_MAX_PROPERTY( texmap, Texmap, kTexmap );

    magma_texmap_node() {
        set_resultType( _T("Color") );

        for( boost::array<std::pair<magma_interface::magma_id, int>, 6>::iterator it = m_baseConnections.begin(),
                                                                                  itEnd = m_baseConnections.end();
             it != itEnd; ++it ) {
            it->first = magma_interface::INVALID_ID;
            it->second = 0;
        }
    }

    virtual int get_num_inputs() const { return (int)m_baseConnections.size() + (int)m_mapChannelConnections.size(); }

    virtual void set_num_inputs( int ) {}

    virtual std::pair<magma_interface::magma_id, int> get_input( int i ) const {
        if( i < m_baseConnections.size() ) {
            return m_baseConnections[i];
        } else {
            return m_mapChannelConnections[i - m_baseConnections.size()].connection;
        }
    }

    virtual void set_input( int i, magma_interface::magma_id id, int outputIndex = 0 ) {
        if( i < m_baseConnections.size() ) {
            m_baseConnections[i].first = id;
            m_baseConnections[i].second = outputIndex;
        } else {
            m_mapChannelConnections[i - m_baseConnections.size()].connection = std::make_pair( id, outputIndex );
        }
    }

    virtual const variant_t& get_input_default_value( int i ) const {
        if( i < m_baseDefaults.size() ) {
            return m_baseDefaults[i];
        } else {
            return m_mapChannelConnections[i - m_baseDefaults.size()].defaultValue;
        }
    }

    virtual void set_input_default_value( int i, const variant_t& value ) {
        if( i < m_baseDefaults.size() ) {
            m_baseDefaults[i] = value;
        } else {
            m_mapChannelConnections[i - m_baseDefaults.size()].defaultValue = value;
        }
    }

    virtual void get_input_description( int i, frantic::tstring& outDescription ) {
        if( i < m_baseDefaults.size() ) {
            magma_max_node_base::get_input_description( i, outDescription );
        } else {
            outDescription = _T("Mapping") + boost::lexical_cast<frantic::tstring>(
                                                 m_mapChannelConnections[i - m_baseDefaults.size()].channel );
        }
    }
};

void magma_texmap_node::get_mapChannels( std::vector<int>& outChannels ) const {
    for( std::vector<map_channel_data>::const_iterator it = m_mapChannelConnections.begin(),
                                                       itEnd = m_mapChannelConnections.end();
         it != itEnd; ++it )
        outChannels.push_back( it->channel );
}

void magma_texmap_node::set_mapChannels( const std::vector<int>& channels ) {
    // Build a new sorted vector of channel information and later swap it into
    std::vector<map_channel_data> newConnections;

    for( std::vector<int>::const_iterator it = channels.begin(), itEnd = channels.end(); it != itEnd; ++it ) {
        // We want to ignore channels 0 and 1 because they are handled separately.
        if( *it > 1 ) {
            map_channel_data newData( *it );

            // See if we previously held connection data for this, and copy it if so.
            std::vector<map_channel_data>::const_iterator itOld =
                std::lower_bound( m_mapChannelConnections.begin(), m_mapChannelConnections.end(), *it );
            if( itOld != m_mapChannelConnections.end() && itOld->channel == *it )
                newData = *itOld;

            // Insert this new channel into the list in the properly sorted location.
            std::vector<map_channel_data>::iterator itNew =
                std::lower_bound( newConnections.begin(), newConnections.end(), *it );
            if( itNew != newConnections.end() && itNew->channel == *it )
                *itNew = newData;
            else
                newConnections.insert( itNew, newData );
        }
    }

    m_mapChannelConnections.swap( newConnections );
}

} // namespace max3d
} // namespace nodes
} // namespace magma
} // namespace frantic
