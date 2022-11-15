// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <boost/mpl/vector.hpp>

#include <frantic/magma/functors/objects.hpp>
#include <frantic/magma/nodes/magma_node_property.hpp>
#include <frantic/magma/nodes/magma_simple_operator.hpp>

#include <vector>

namespace frantic {
namespace magma {
namespace nodes {

class magma_object_query_node : public magma_simple_operator<2> {
    MAGMA_PROPERTY( properties, std::vector<frantic::tstring> )

  public:
    class functor;

    struct meta {
        enum { ARITY = 1 };
        typedef frantic::magma::functors::object_query type;
        typedef boost::mpl::vector<void( void*, int )> bindings;
    };

  public:
    static void create_type_definition( magma_node_type& outType );

    virtual void compile( magma_compiler_interface& compile );

    magma_object_query_node();

    virtual int get_num_outputs() const;

    virtual void get_output_description( int i, frantic::tstring& outDescription ) const;
};

} // namespace nodes
} // namespace magma
} // namespace frantic
