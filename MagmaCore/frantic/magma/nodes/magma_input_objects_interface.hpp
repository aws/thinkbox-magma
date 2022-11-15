// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_data_type.hpp>
#include <frantic/magma/nodes/magma_simple_operator.hpp>

namespace frantic {
namespace magma {
namespace nodes {

/**
 * Abstract interface for supplying object properties at runtime.
 */
class magma_input_objects_interface {
    template <class T>
    struct fix_type {
        typedef T type;
    };

    template <class T>
    struct fix_type<const T*> {
        typedef T* type;
    };

  protected:
    virtual bool get_property_internal( std::size_t index, const frantic::tstring& propName,
                                        const std::type_info& typeInfo, void* outValue ) = 0;

  public:
    /**
     * @return The number of objects held.
     */
    virtual std::size_t size() const = 0;

    virtual void get_property( std::size_t index, const frantic::tstring& propName, variant_t& outValue ) = 0;

    template <class T>
    inline bool get_property( std::size_t index, const frantic::tstring& propName, T& outValue ) {
        return this->get_property_internal( index, propName, typeid( typename fix_type<T>::type ), &outValue );
    }

    /**
     * Here is an example implementation of how one might get a property
     */
    // inline frantic::graphics::transform4f get_toworld_transform(){
    //	frantic::graphics::transform4f result;
    //	this->get_property_internal( 0, "Transform", typeid(frantic::graphics::transform4f), &result );
    //	return result;
    // }
};

} // namespace nodes
} // namespace magma
} // namespace frantic
