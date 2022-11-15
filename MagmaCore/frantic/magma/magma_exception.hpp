// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <exception>

#pragma warning( push )
#pragma warning( disable : 4512 4996 4706 )
#include <boost/exception/all.hpp>
#pragma warning( pop )

#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>

#pragma warning( push )
#pragma warning( disable : 4100 )
#include <boost/tuple/tuple_io.hpp>
#pragma warning( pop )

#include <frantic/magma/magma_interface.hpp>

namespace frantic {
namespace magma {

class magma_exception : virtual public boost::exception, virtual public std::exception {
  public:
    typedef boost::error_info<struct tag_error_name, frantic::tstring> error_name;
    typedef boost::error_info<struct tag_node_id, magma_interface::magma_id> node_id;
    typedef boost::error_info<struct tag_input_index, int> input_index;
    typedef boost::error_info<struct tag_output_index, int> output_index;
    typedef boost::error_info<struct tag_connected_id, magma_interface::magma_id> connected_id;
    typedef boost::error_info<struct tag_connected_output_index, int> connected_output_index;
    typedef boost::error_info<struct tag_property_name, frantic::tstring> property_name;
    typedef boost::error_info<struct tag_found_type, magma_data_type> expected_type;
    typedef boost::error_info<struct tag_expected_type, magma_data_type> found_type;
    typedef boost::error_info<struct tag_magma_interface, boost::uint64_t> source_expression_id;
    typedef boost::error_info<struct tag_input_types, std::vector<magma_data_type>> found_inputs;
    typedef boost::error_info<struct tag_input_types, std::vector<std::vector<magma_data_type>>> expected_inputs;

    inline magma_interface::magma_id get_node_id() const {
        if( const magma_interface::magma_id* id = boost::get_error_info<node_id>( *this ) )
            return *id;
        return magma_interface::INVALID_ID;
    }

    inline boost::uint64_t get_source_expression_id() const {
        if( const boost::uint64_t* exprId = boost::get_error_info<source_expression_id>( *this ) )
            return *exprId;
        return std::numeric_limits<boost::uint64_t>::max();
    }

    inline frantic::tstring get_message( bool oneBasedIndex = false ) const throw() {
        try {
            int indexOffset = oneBasedIndex ? 1 : 0;

            std::basic_stringstream<frantic::tchar> ss;
            ss << "Magma ";
            if( const frantic::tstring* name = boost::get_error_info<error_name>( *this ) )
                ss << *name << ":\n";
            else
                ss << "error:\n";

            if( const magma_interface::magma_id* id = boost::get_error_info<node_id>( *this ) )
                ss << "\tID: " << *id << "\n";
            if( const int* index = boost::get_error_info<input_index>( *this ) )
                ss << "\tInput Index: " << ( *index + indexOffset ) << "\n";
            if( const int* index = boost::get_error_info<output_index>( *this ) )
                ss << "\tOutput Index: " << ( *index + indexOffset ) << "\n";
            if( const magma_interface::magma_id* id = boost::get_error_info<connected_id>( *this ) )
                ss << "\tConnected To ID: " << *id << "\n";
            if( const int* index = boost::get_error_info<connected_output_index>( *this ) )
                ss << "\tConnected To Output Index: " << ( *index + indexOffset ) << "\n";
            if( const frantic::tstring* propName = boost::get_error_info<property_name>( *this ) )
                ss << "\tProperty Name: " << *propName << "\n";
            if( const magma_data_type* foundType = boost::get_error_info<found_type>( *this ) )
                ss << "\tFound Type: " << foundType->to_string() << "\n";
            if( const magma_data_type* expectedType = boost::get_error_info<expected_type>( *this ) )
                ss << "\tExpected Type: " << expectedType->to_string() << "\n";
            if( const std::vector<magma_data_type>* foundInputs = boost::get_error_info<found_inputs>( *this ) ) {
                if( !foundInputs->empty() ) {
                    ss << _T("\tFound Inputs: (") << foundInputs->front().to_string();
                    for( std::vector<magma_data_type>::const_iterator it = foundInputs->begin() + 1,
                                                                      itEnd = foundInputs->end();
                         it != itEnd; ++it )
                        ss << _T(", ") << it->to_string();
                    ss << ")\n";
                }
            }
            if( const std::vector<std::vector<magma_data_type>>* expectedInputs =
                    boost::get_error_info<expected_inputs>( *this ) ) {
                ss << _T("\tValid Inputs:\n");
                for( std::vector<std::vector<magma_data_type>>::const_iterator itOuter = expectedInputs->begin(),
                                                                               itOuterEnd = expectedInputs->end();
                     itOuter != itOuterEnd; ++itOuter ) {
                    if( !itOuter->empty() ) {
                        ss << _T("\t\t(") << itOuter->front().to_string();
                        for( std::vector<magma_data_type>::const_iterator it = itOuter->begin() + 1,
                                                                          itEnd = itOuter->end();
                             it != itEnd; ++it )
                            ss << _T(", ") << it->to_string();
                        ss << ")\n";
                    }
                }
            }

            return ss.str();
        } catch( ... ) {
            return frantic::tstring();
        }
    }

    virtual const char* what() const throw() { return "Magma error"; }
};

class internal_error : public std::logic_error {
    std::string m_message;

    // overload to work around lexical_cast() build error in VS2008
    std::string to_string( const boost::tuple<>& ) { return "()"; }

    template <class TupleType>
    std::string to_string( const TupleType& tuple ) {
        return boost::lexical_cast<std::string>( tuple );
    }

  public:
    internal_error( const std::string& msg )
        : std::logic_error( msg )
        , m_message( msg ) {}

    ~internal_error() throw() {}

    template <class TupleType>
    inline internal_error& operator<<( const TupleType& tuple ) {
        m_message += to_string( tuple );
        return *this;
    }

    virtual const char* what() const throw() { return m_message.c_str(); }
};

#define THROW_MAGMA_INTERNAL_ERROR_IMPL2( line, file )                                                                 \
    throw frantic::magma::internal_error( "Magma internal error at line " #line " of file \"" file "\"" )
#define THROW_MAGMA_INTERNAL_ERROR_IMPL1( line, file ) THROW_MAGMA_INTERNAL_ERROR_IMPL2( line, file )
#define THROW_MAGMA_INTERNAL_ERROR( ... )                                                                              \
    THROW_MAGMA_INTERNAL_ERROR_IMPL1( __LINE__, __FILE__ ) << boost::make_tuple( __VA_ARGS__ )

} // namespace magma
} // namespace frantic
