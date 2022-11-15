// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <boost/call_traits.hpp>

#define MAGMA_PROPERTY( name, type )                                                                                   \
  private:                                                                                                             \
    type m_##name;                                                                                                     \
                                                                                                                       \
  public:                                                                                                              \
    inline boost::call_traits<type>::param_type get_##name() const { return m_##name; }                                \
    inline void set_##name( boost::call_traits<type>::param_type val ) { m_##name = val; }
