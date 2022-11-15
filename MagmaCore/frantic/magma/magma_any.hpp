// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <boost/scoped_ptr.hpp>
#include <boost/type_traits/remove_reference.hpp>

#include <memory>
#include <typeinfo>

namespace frantic {
namespace magma {

// This class represents a variable that can store *any* value.
class magma_any {
    class holder_interface {
      public:
        virtual ~holder_interface() {}
        virtual const std::type_info& get_type() const = 0;
        virtual const void* get_ptr() const = 0;
        virtual holder_interface* clone() const = 0;
    };

    template <class T>
    class holder : public holder_interface {
        T m_val;

      public:
        explicit holder( const T& val )
            : m_val( val ) {}

#if _MSC_VER >= 1600
        explicit holder( T&& val )
            : m_val( std::forward<T>( val ) ) {}
#endif
        virtual ~holder() {}
        virtual const std::type_info& get_type() const { return typeid( T ); }
        virtual const void* get_ptr() const { return &m_val; }
        virtual holder_interface* clone() const { return new holder( m_val ); }
    };

    template <class T>
    class holder<const T>;
    template <class T>
    class holder<const T&>;
    template <class T>
    class holder<T&>;

    // boost::scoped_ptr<holder_interface> m_holder;
    std::unique_ptr<holder_interface> m_holder;

    template <class T>
    friend const T& any_cast( const magma_any& val );

  public:
    magma_any() {}

    template <class T>
    explicit magma_any( const T& val ) {
        m_holder.reset( new holder<T>( val ) );
    }
    template <class T>
    magma_any& operator=( const T& val ) {
        m_holder.reset( new holder<T>( val ) );
        return *this;
    }

    magma_any( const magma_any& rhs )
        : m_holder( rhs.m_holder.get() ? rhs.m_holder->clone() : NULL ) {}

    magma_any& operator=( const magma_any& rhs ) {
        m_holder.reset( rhs.m_holder.get() ? rhs.m_holder->clone() : NULL );
        return *this;
    }

#if _MSC_VER >= 1600
    // Need to disable this overload if it is trying to bind to a reference type. This is because T& && collapses to T&
    // so for some uses T will be a lvalue reference (ie. T = X&) which is not desired.

    // MSVC doesn't yet support default template parameter on function templates, so I need to use the dummy arg method.
    // Hopefully the compiler elides it. template <class T, typename
    // std::enable_if<!std::is_lvalue_reference<T>::value,int>::type = 0> explicit magma_any( T&& val ){ m_holder.reset(
    // new holder< T >( std::forward<T>(val) ) ); }

    template <class T>
    explicit magma_any( T&& val, typename std::enable_if<!std::is_lvalue_reference<T>::value, int>::type dummy = 0 ) {
        m_holder.reset( new holder<T>( std::move( val ) ) );
    }

    magma_any( magma_any&& rhs ) { m_holder = std::move( rhs.m_holder ); }
    magma_any& operator=( magma_any&& rhs ) {
        m_holder = std::move( rhs.m_holder );
        return *this;
    }
#endif

    const std::type_info& get_type() const { return m_holder.get() ? m_holder->get_type() : typeid( void ); }
    const void* get_ptr() const { return m_holder.get() ? m_holder->get_ptr() : NULL; }
};

template <>
class magma_any::holder<magma_any>;

template <class T>
const T& any_cast( const magma_any& val ) {
    if( val.get_type() != typeid( T ) )
        throw std::bad_cast();
    return static_cast<magma_any::holder<T>>( val.m_holder )->m_val;
}

} // namespace magma
} // namespace frantic
