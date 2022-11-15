// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/strings/tstring.hpp>
#include <frantic/strings/utf8.hpp>
#include <istream>
#include <ostream>
#include <sstream>
#include <vector>

namespace frantic {
namespace convert {

template <typename T>
inline frantic::tstring to_tstring( T val ) {
#ifdef FRANTIC_USE_WCHAR
    std::wostringstream strOut;
#else
    std::ostringstream strOut;
#endif
    strOut << val;
    return strOut.str();
}

template <>
inline frantic::tstring to_tstring<bool>( bool val ) {
    if( val )
        return _T( "true" );
    return _T( "false" );
}

/// XXX check this function works when WCHAR used
inline frantic::tstring istream_to_tstring( std::istream& in, std::size_t length ) {
    std::vector<char> buffer( length );
    in.read( &buffer[0], length );
    if( !in.good() )
        throw std::runtime_error( "istream_to_tstring: can't get characters from istream" );
    std::string outbuffer( buffer.begin(), buffer.end() );
    return frantic::strings::to_tstring( outbuffer );
}

inline frantic::tstring utf8_istream_to_tstring( std::istream& in, std::size_t length ) {
    std::vector<char> buffer( length );
    in.read( &buffer[0], length );
    if( !in.good() )
        throw std::runtime_error( "istream_to_tstring: can't get characters from istream" );
    std::string outbuffer( buffer.begin(), buffer.end() );
#if defined( FRANTIC_USE_WCHAR )
    if( frantic::strings::is_valid_utf8( outbuffer ) ) {
        return frantic::strings::wstring_from_utf8( outbuffer );
    } else {
        return frantic::strings::to_tstring( outbuffer );
    }
#else
    return outbuffer;
#endif
}

} // namespace convert
} // namespace frantic
