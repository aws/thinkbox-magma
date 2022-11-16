// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/nodes/magma_node_impl.hpp>

namespace frantic {
namespace magma {
namespace detail {

template <>
struct property_traits<Class_ID> {
    typedef Class_ID return_type;
    typedef const Class_ID& param_type;
};

} // namespace detail
} // namespace magma
} // namespace frantic

#define MAGMA_DEFINE_MAX_TYPE( name, category, type )                                                                  \
    Class_ID type::get_class_id() const { return max_impl::s_ClassID; }                                                \
    MAGMA_DEFINE_TYPE( name, category, type )                                                                          \
    MAGMA_HIDDEN_PROPERTY( _maxImpl, ReferenceTarget* )                                                                \
    MAGMA_HIDDEN_READONLY_PROPERTY( _maxImplClassID, Class_ID )
