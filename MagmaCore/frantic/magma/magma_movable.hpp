// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace frantic {
namespace magma {

/**
 * This class is used as a wrapper to simulate an rvalue-reference for the purposes of implementing move assignment and
 * construction.
 */
template <class T>
struct movable {
    T* m_val;

    explicit movable( T& val )
        : m_val( &val ) {}

    T& get() const { return const_cast<T&>( *m_val ); }
};

template <class T>
inline movable<T> make_movable( T& ref ) {
    return movable<T>( ref );
}

} // namespace magma
} // namespace frantic
