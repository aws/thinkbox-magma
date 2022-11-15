// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/magma/magma_compiler_interface.hpp>
#include <frantic/magma/magma_interface.hpp>
#include <frantic/particles/streams/particle_istream.hpp>

#include <boost/any.hpp>
#include <boost/function.hpp>

namespace frantic {
namespace magma {
namespace simple_compiler {

typedef boost::shared_ptr<frantic::particles::streams::particle_istream> particle_istream_ptr;
typedef boost::function<void( const frantic::magma::magma_exception& )> exception_callback;

particle_istream_ptr
apply_simple_compiler_expression( particle_istream_ptr pin, boost::shared_ptr<magma_interface> magma,
                                  boost::shared_ptr<magma_compiler_interface::context_base> contextData,
                                  const exception_callback& errorCallback = exception_callback() );

} // namespace simple_compiler
} // namespace magma
} // namespace frantic
