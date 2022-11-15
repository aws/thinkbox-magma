// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <maya/MArgList.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <frantic/logging/logging_level.hpp>

namespace frantic {
namespace maya {
namespace parser_args {

namespace detail {

template <typename T>
inline MStatus get_arg( const MArgList& args, const char* shortFlag, const char* longFlag, T& outArg ) {
    MStatus outStatus = MS::kFailure;
    unsigned int index = args.flagIndex( shortFlag, longFlag );
    if( MArgList::kInvalidArgIndex != index ) {
        unsigned int element = index + 1;
        outStatus = args.get( element, outArg );
    }
    return outStatus;
}

} // namespace detail

/// the reason I only expose some overload functions instead of template because of that the MArgList::get only
/// support a certain amount of data types
inline MStatus get_arg( const MArgList& args, const char* shortFlag, const char* longFlag, MString& outArg ) {
    return detail::get_arg( args, shortFlag, longFlag, outArg );
}

inline MStatus get_arg( const MArgList& args, const char* shortFlag, const char* longFlag, int& outArg ) {
    return detail::get_arg( args, shortFlag, longFlag, outArg );
}

// return true if this argument has been presented in MArgList otherwise returns false
inline bool get_int_arg( const MArgList& args, const char* shortFlag, const char* longFlag, int& outArg ) {
    bool outResult;

    if( MS::kSuccess == parser_args::get_arg( args, shortFlag, longFlag, outArg ) )
        outResult = true;
    else
        outResult = false;
    return outResult;
}

/// 'on' in mel script means 'true', the corresponding integer value is 1
/// 'off' in mel script means 'false', the corresponding integer value is 0
inline bool get_bool_arg( const MArgList& args, const char* shortFlag, const char* longFlag ) {
    bool outResult = false;

    int argVal;
    if( MS::kSuccess == detail::get_arg( args, shortFlag, longFlag, argVal ) ) {
        if( argVal != 0 )
            outResult = true;
    }
    return outResult;
}

inline bool get_bool_arg( const MArgList& args, const char* shortFlag, const char* longFlag, bool& outArg ) {
    bool outResult = false;

    int argVal;
    if( MS::kSuccess == detail::get_arg( args, shortFlag, longFlag, argVal ) ) {
        outArg = ( argVal != 0 );
        outResult = true;
    }
    return outResult;
}

// return true if this argument has been presented in MArgList otherwise returns false
inline bool get_float_arg( const MArgList& args, const char* shortFlag, const char* longFlag, double& outArg ) {
    bool outResult;

    if( MS::kSuccess == detail::get_arg( args, shortFlag, longFlag, outArg ) )
        outResult = true;
    else
        outResult = false;
    return outResult;
}

// MStatus			get( unsigned int& index, MIntArray& ret ) const;
////! \noscript
// MStatus			get( unsigned int& index, MDoubleArray& ret ) const;
////! \noscript
// MStatus         get( unsigned int& index, MStringArray& ret ) const;
inline bool get_int_array_arg( const MArgList& args, const char* shortFlag, const char* longFlag, MIntArray& outArg ) {
    bool outResult;

    if( MS::kSuccess == detail::get_arg( args, shortFlag, longFlag, outArg ) )
        outResult = true;
    else
        outResult = false;
    return outResult;
}

inline bool get_string_array_arg( const MArgList& args, const char* shortFlag, const char* longFlag,
                                  MStringArray& outArg ) {
    bool outResult;

    if( MS::kSuccess == detail::get_arg( args, shortFlag, longFlag, outArg ) )
        outResult = true;
    else
        outResult = false;
    return outResult;
}

/// Just checks if a flag/argument exists
inline bool has_flag( const MArgList& args, const char* shortFlag, const char* longFlag ) {
    unsigned int index = args.flagIndex( shortFlag, longFlag );
    if( MArgList::kInvalidArgIndex != index )
        return true;
    return false;
}

} // namespace parser_args
} // namespace maya
} // namespace frantic
