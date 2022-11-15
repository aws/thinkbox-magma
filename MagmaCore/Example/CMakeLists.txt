# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required( VERSION 3.15 FATAL_ERROR )

project( MagmaExamples )

include( ../../../ThinkboxCMLibrary/ThinkboxCMLibrary.cmake )

add_executable( magmaexample "main.cpp" )
target_link_libraries( magmaexample magma )

frantic_common_platform_setup( magmaexample )

# The Conan version of Boost was built with this, and it changes the library names.
# As a result, we need to set this to tell Boost to look for the right libraries to
# link against.
target_compile_definitions( magmaexample PRIVATE BOOST_AUTO_LINK_SYSTEM )
