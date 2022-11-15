# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required( VERSION 3.15 FATAL_ERROR )

project( Magma )

find_package( thinkboxcmlibrary REQUIRED )
include( PrecompiledHeader )
include( ThinkboxCMLibrary )

option( BUILD_UNIT_TESTS "Build unit tests" OFF )
option( BUILD_EXAMPLES "Build the sample project" ON )

add_library( magma STATIC )

set_property( TARGET magma PROPERTY CXX_STANDARD 17 )

target_include_directories( magma PUBLIC
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                            $<INSTALL_INTERFACE:include> )

file( GLOB_RECURSE H_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
    "frantic/*.h"
    "frantic/*.hpp"
)

file( GLOB_RECURSE CXX_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "src/*.cpp"
)

target_sources( magma PRIVATE
    stdafx.cpp
    stdafx.h
    ${H_FILES}
    ${CXX_FILES}
)

set_target_properties( magma PROPERTIES PROJECT_LABEL "Magma" )

add_precompiled_header( magma stdafx.h SOURCE_CXX stdafx.cpp )

find_package( thinkboxlibrary REQUIRED )
find_package( Boost REQUIRED )
find_package( OpenEXR REQUIRED )
find_package( tinyxml2 REQUIRED )
find_package( ZLIB REQUIRED )
find_package( TBB REQUIRED )

target_include_directories( magma PUBLIC ${thinkboxlibrary_INCLUDE_DIRS} )
target_include_directories( magma PUBLIC ${Boost_INCLUDE_DIRS} )
target_include_directories( magma PUBLIC ${OpenEXR_INCLUDE_DIRS} )
target_include_directories( magma PUBLIC ${tinyxml2_INCLUDE_DIRS} )
target_include_directories( magma PUBLIC ${ZLIB_INCLUDE_DIRS} )
target_include_directories( magma PUBLIC ${TBB_INCLUDE_DIRS} )

target_link_libraries( magma INTERFACE thinkboxlibrary::thinkboxlibrary )
target_link_libraries( magma INTERFACE Boost::Boost )
target_link_libraries( magma INTERFACE OpenEXR::OpenEXR )
target_link_libraries( magma INTERFACE tinyxml2::tinyxml2 )
target_link_libraries( magma INTERFACE ZLIB::ZLIB )
target_link_libraries( magma INTERFACE TBB::tbb )

frantic_common_platform_setup( magma )
frantic_default_source_groups( magma HEADERDIR include SOURCEDIR src )

# The Conan version of Boost was built with this, and it changes the library names.
# As a result, we need to set this to tell Boost to look for the right libraries to
# link against.
target_compile_definitions( magma PUBLIC BOOST_AUTO_LINK_SYSTEM )

if( WIN32 )
    # Disable optimization for the RelWithDebInfo configuration on Windows.
    # This allows breakpoints to be hit reliably when debugging in Visual Studio.
	target_compile_options( magma PRIVATE "$<$<CONFIG:Release>:/O2>$<$<CONFIG:RelWithDebInfo>:/Od>" )
endif()

if( BUILD_UNIT_TESTS )
    add_subdirectory( UnitTests )
endif()

if( BUILD_EXAMPLES )
    add_subdirectory( Example )
endif()

install( DIRECTORY frantic
            DESTINATION include
            FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
install( TARGETS magma
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
)
