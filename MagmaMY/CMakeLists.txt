# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required( VERSION 3.20 FATAL_ERROR )

project( MagmaMY )

find_package( thinkboxcmlibrary REQUIRED )
include( PrecompiledHeader )
include( ThinkboxCMLibrary )

set( SUPPORTED_MAYA_VERSIONS 2022 2023 )

option( MAYA_VERSION "The version of the Maya SDK to build the library against." 2022 )

if( NOT MAYA_VERSION IN_LIST SUPPORTED_MAYA_VERSIONS )
  message( FATAL_ERROR "ERROR: Cannot build for unsupported Maya version ${MAYA_VERSION}" )
endif()

set( CMAKE_AUTOMOC ON )
set( CMAKE_AUTORCC ON )
set( CMAKE_AUTOUIC ON )

add_library( magmamy STATIC )

set_property( TARGET magmamy PROPERTY CXX_STANDARD 17 )

target_include_directories( magmamy PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:include>
)
 
file( GLOB_RECURSE H_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        "frantic/*.h"
        "frantic/*.hpp"
)

file( GLOB_RECURSE CXX_FILES ${CMAKE_CURRENT_SOURCE}
    "src/*.cpp"
)

target_sources( magmamy PRIVATE
    stdafx.cpp
    stdafx.h
    ${H_FILES}
    ${CXX_FILES}
)

# The Conan version of Boost was built with this, and it changes the library names.
# As a result, we need to set this to tell Boost to look for the right libraries to
# link against.
target_compile_definitions( magmamy PUBLIC BOOST_AUTO_LINK_SYSTEM )

find_package( thinkboxlibrary REQUIRED )
find_package( thinkboxmylibrary REQUIRED )
find_package( nodeview REQUIRED )
find_package( magma REQUIRED )
find_package( Qt5 COMPONENTS Core Gui Widgets REQUIRED )
find_package( Boost REQUIRED )
find_package( TBB REQUIRED )
find_package( tinyxml2 REQUIRED )
find_package( OpenEXR REQUIRED )
find_package( mayasdk REQUIRED )

target_include_directories( magmamy PUBLIC ${thinkboxlibrary_INCLUDE_DIRS} )
target_include_directories( magmamy PUBLIC ${thinkboxmylibrary_INCLUDE_DIRS} )
target_include_directories( magmamy PUBLIC ${nodeview_INCLUDE_DIRS} )
target_include_directories( magmamy PUBLIC ${magma_INCLUDE_DIRS} )
target_include_directories( magmamy PUBLIC ${Qt5_INCLUDE_DIRS} )
target_include_directories( magmamy PUBLIC ${Boost_INCLUDE_DIRS} )
target_include_directories( magmamy PUBLIC ${TBB_INCLUDE_DIRS} )
target_include_directories( magmamy PUBLIC ${tinyxml2_INCLUDE_DIRS} )
target_include_directories( magmamy PUBLIC ${OpenEXR_INCLUDE_DIRS} )
target_include_directories( magmamy PUBLIC ${mayasdk_INCLUDE_DIRS} )

target_link_libraries( magmamy INTERFACE thinkboxlibrary::thinkboxlibrary )
target_link_libraries( magmamy INTERFACE thinkboxmylibrary::thinkboxmylibrary )
target_link_libraries( magmamy INTERFACE nodeview::nodeview )
target_link_libraries( magmamy INTERFACE magma::magma )
target_link_libraries( magmamy INTERFACE Qt5::Core )
target_link_libraries( magmamy INTERFACE Qt5::Gui )
target_link_libraries( magmamy INTERFACE Qt5::Widgets )
target_link_libraries( magmamy INTERFACE Boost::Boost )
target_link_libraries( magmamy INTERFACE TBB::tbb )
target_link_libraries( magmamy INTERFACE tinyxml2::tinyxml2 )
target_link_libraries( magmamy INTERFACE OpenEXR::OpenEXR )
target_link_libraries( magmamy INTERFACE mayasdk::mayasdk )

frantic_common_platform_setup( magmamy )
frantic_default_source_groups( magmamy HEADERDIR frantic SOURCEDIR src )

# Disable optimization for the RelWithDebInfo configuration on Windows.
# This allows breakpoints to be hit reliably when debugging in Visual Studio.
if( WIN32 )
	target_compile_options( magmamy PRIVATE "$<$<CONFIG:Release>:/O2>$<$<CONFIG:RelWithDebInfo>:/Od>" )
endif()

install( DIRECTORY frantic
          DESTINATION include
          FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
install( TARGETS magmamy
          RUNTIME DESTINATION bin
          LIBRARY DESTINATION lib
          ARCHIVE DESTINATION lib
)
