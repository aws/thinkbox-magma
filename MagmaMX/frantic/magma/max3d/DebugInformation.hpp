// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_compiler_interface.hpp>
#include <frantic/magma/max3d/IDebugInformation.hpp>

namespace frantic {
namespace magma {
namespace max3d {

class DebugInformation : public IObject, public IDebugInformation {
    int m_refCount;

    typedef std::vector<frantic::magma::debug_data> storage_type;

    storage_type m_data;

  public:
    DebugInformation()
        : m_refCount( 0 ) {}

    virtual ~DebugInformation() {}

    inline storage_type& get_storage() { return m_data; }

    // From IObject
    virtual
#if MAX_VERSION_MAJOR >= 15
        const
#endif
        MCHAR*
        GetIObjectName() {
        return _T("DebugInformation");
    }
    virtual void AcquireIObject() { ++m_refCount; }
    virtual void ReleaseIObject() {
        if( --m_refCount == 0 )
            delete this;
    }
    virtual void DeleteIObject() {}

    // From IDebugInformation
    virtual int GetNumIterations() const;
    virtual Value* GetNodeValue( int iteration, int nodeID, int outputIndex ) const;
    virtual Array* GetNodeMinMaxMeanValue( int nodeID, int outputIndex ) const;

    // From BaseInterfaceServer
    virtual int NumInterfaces() const;
    virtual BaseInterface* GetInterfaceAt( int i ) const;

    // From InterfaceServer
    virtual BaseInterface* GetInterface( Interface_ID id );
};

} // namespace max3d
} // namespace magma
} // namespace frantic
