// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/fnpublish/MixinInterface.hpp>
#include <frantic/max3d/maxscript/shared_value_ptr.hpp>

#include <frantic/magma/magma_exception.hpp>

#include <frantic/logging/logging_level.hpp>

#include <boost/function.hpp>

#pragma warning( push, 3 )
#define NOMINMAX
#include <ifnpub.h>
#include <paramtype.h>

#if MAX_VERSION_MAJOR < 15
#define p_end end
#endif

#pragma warning( pop )

#define ErrorReporter_INTERFACE Interface_ID( 0x5ba36c3b, 0x110e3873 )

namespace frantic {
namespace magma {
namespace max3d {

class IErrorReporter : public FPMixinInterface {
  public:
    virtual ~IErrorReporter() {}

    virtual BaseInterface* GetInterface( Interface_ID id ) {
        if( id == ErrorReporter_INTERFACE )
            return this;
        return FPMixinInterface::GetInterface( id );
    }

    virtual FPInterfaceDesc* GetDesc() { return GetDescByID( ErrorReporter_INTERFACE ); }

    virtual FPInterfaceDesc* GetDescByID( Interface_ID id ) = 0;

    virtual bool HasError() const = 0;
    virtual TYPE_TSTR_BV_TYPE GetErrorMessage() const = 0;
    virtual TYPE_REFTARG_TYPE GetErrorExpression() const = 0;
    virtual TYPE_INT_TYPE GetErrorExpressionNodeID() const = 0;

    // Deprecated
    // virtual bool GetLastError( TYPE_TSTR_BR_TYPE outMessage, TYPE_REFTARG_BR_TYPE outHolder, TYPE_INT_BR_TYPE
    // outMagmaID ) = 0;

    // Proposed observer interface for errors...
    // virtual int RegisterErrorListener( TYPE_VALUE_TYPE callbackObject ) = 0;

  protected:
    enum {
        // kFnGetLastError,
        kFnHasError,
        kFnErrorMessage,
        kFnErrorExpression,
        kFnErrorNodeID
    };

#pragma warning( push )
#pragma warning( disable : 4238 4100 )
    BEGIN_FUNCTION_MAP
    // FN_3( kFnGetLastError, TYPE_bool, GetLastError, TYPE_TSTR_BR, TYPE_REFTARG_BR, TYPE_INT_BR )
    RO_PROP_FN( kFnHasError, HasError, TYPE_bool )
    RO_PROP_FN( kFnErrorMessage, GetErrorMessage, TYPE_TSTR_BV )
    RO_PROP_FN( kFnErrorExpression, GetErrorExpression, TYPE_REFTARG )
    RO_PROP_FN( kFnErrorNodeID, GetErrorExpressionNodeID, TYPE_INT )
    END_FUNCTION_MAP;
#pragma warning( pop )

    inline static void init_fpinterface_desc( FPInterfaceDesc& desc ) {
        /*desc.AppendFunction( kFnGetLastError, _M("GetLastError"), 0, TYPE_bool, 0, 3,
                _M("OutMessage"), 0, TYPE_TSTR_BR,
                f_inOut, FPP_OUT_PARAM,
                f_keyArgDefault, NULL,
                _M("OutExpressionHolder"), 0, TYPE_REFTARG_BR,
                f_inOut, FPP_OUT_PARAM,
                f_keyArgDefault, NULL,
                _M("OutNodeID"), 0, TYPE_INT_BR,
                f_inOut, FPP_OUT_PARAM,
                f_keyArgDefault, NULL,
                p_end );*/
        desc.AppendProperty( kFnHasError, FP_NO_FUNCTION, _M( "HasError" ), 0, TYPE_bool, p_end );
        desc.AppendProperty( kFnErrorMessage, FP_NO_FUNCTION, _M( "ErrorMessage" ), 0, TYPE_TSTR_BV, p_end );
        desc.AppendProperty( kFnErrorExpression, FP_NO_FUNCTION, _M( "ErrorSource" ), 0, TYPE_REFTARG, p_end );
        desc.AppendProperty( kFnErrorNodeID, FP_NO_FUNCTION, _M( "ErrorSourceNodeID" ), 0, TYPE_INT, p_end );
    }
};

class error_reporter {
  public:
    typedef void( callback_signature )( bool, const frantic::tstring&, boost::uint64_t,
                                        frantic::magma::magma_interface::magma_id );

  private:
    bool present;                                     // Is there currently an error held?
    frantic::tstring message;                         // Error message
    boost::uint64_t sourceExprID;                     // Id associated with the holder that threw this error.
    frantic::magma::magma_interface::magma_id nodeID; // Id associated with the node that threw this error.

    boost::function<callback_signature> m_callback;

  public:
    error_reporter()
        : present( false )
        , sourceExprID( std::numeric_limits<boost::uint64_t>::max() )
        , nodeID( -43 )
        , message( _M( "No Error" ) ) {}

    inline void set_callback( const boost::function<callback_signature>& callback ) { m_callback = callback; }

    inline bool has_error() const { return present; }

    inline const frantic::tstring& get_message() const { return message; }

    inline boost::uint64_t get_expression_id() const { return sourceExprID; }

    inline frantic::magma::magma_interface::magma_id get_id() const { return nodeID; }

    inline bool reset() {
        if( !present )
            return false;
        present = false;
        if( m_callback )
            m_callback( false, _T(""), std::numeric_limits<boost::uint64_t>::max(), -43 );
        return true;
    }

    inline bool reset( const std::exception& e ) {
        frantic::tstring newMessage = frantic::strings::to_tstring( e.what() );
        if( this->has_error() && newMessage == message )
            return false;

        present = true;
        sourceExprID = std::numeric_limits<boost::uint64_t>::max();
        nodeID = -1;
        message = newMessage;
        if( m_callback )
            m_callback( true, message, sourceExprID, nodeID );
        return true;
    }

    inline bool reset( const frantic::magma::magma_exception& e ) {
        frantic::tstring newMessage = e.get_message( true );
        if( this->has_error() && e.get_source_expression_id() == sourceExprID && e.get_node_id() == nodeID &&
            newMessage == message )
            return false;

        present = true;
        sourceExprID = e.get_source_expression_id();
        nodeID = e.get_node_id();
        message = newMessage;
        if( m_callback )
            m_callback( true, message, sourceExprID, nodeID );
        return true;
    }
};

#define ErrorReport2_INTERFACE Interface_ID( 0x511d0328, 0x58df2325 )

class IErrorReporter2 : public frantic::max3d::fnpublish::MixinInterface<IErrorReporter2> {
  public:
    virtual void SetCallback( Value* callbackFn ) = 0;

    virtual void SetError( const frantic::magma::magma_exception& e ) = 0;

    virtual void ClearError() = 0;

    static ThisInterfaceDesc* GetStaticDesc() {
        static ThisInterfaceDesc theDesc( ErrorReport2_INTERFACE, _T("MagmaErrorReporter"), 0 );

        if( theDesc.empty() ) { // ie. Check if we haven't initialized the descriptor yet
            theDesc.function( _T("SetCallback"), &IErrorReporter2::SetCallback ).param( _T("CallbackFunction") );
        }

        return &theDesc;
    }

    virtual ThisInterfaceDesc* GetDesc() { return IErrorReporter2::GetStaticDesc(); }
};

inline void ReportError( Value* callbackFn, const frantic::magma::magma_exception& e ) {
    Value** mxsArgs;
    value_local_array( mxsArgs, 6 );

    std::basic_stringstream<MCHAR> ss;

    ss << e.get_message( true ) << std::endl;

    mxsArgs[0] = new String( ss.str().c_str() );
    mxsArgs[1] = &keyarg_marker;
    mxsArgs[2] = Name::intern( _T("NodeID") );
    mxsArgs[3] = Integer::intern( static_cast<int>( e.get_node_id() ) );
    mxsArgs[4] = Name::intern( _T("ExpressionID") );
    mxsArgs[5] = Integer::intern( static_cast<int>( e.get_source_expression_id() ) );

    try {
        callbackFn->apply( mxsArgs, 6 );
    } catch( MAXScriptException& /*e*/ ) {
#if MAX_VERSION_MAJOR < 19
        pop_value_local_array( mxsArgs );
#endif

        throw;
    }
#if MAX_VERSION_MAJOR < 19
    pop_value_local_array( mxsArgs );
#endif
}

inline void ClearError( Value* callbackFn ) {
    Value** mxsArgs;
    value_local_array( mxsArgs, 1 );

    mxsArgs[0] = &undefined;

    try {
        callbackFn->apply( mxsArgs, 1 );
    } catch( MAXScriptException& /*e*/ ) {
#if MAX_VERSION_MAJOR < 19
        pop_value_local_array( mxsArgs );
#endif

        throw;
    }
#if MAX_VERSION_MAJOR < 19
    pop_value_local_array( mxsArgs );
#endif
}

class ErrorReporter : public IErrorReporter2 {
  public:
    void CopyCallback( ErrorReporter* other );

    virtual void SetCallback( Value* callbackFn );

    virtual void SetError( const frantic::magma::magma_exception& e );

    virtual void ClearError();

  private:
    boost::shared_ptr<Value> m_pErrorCallback;
};

inline void ErrorReporter::SetCallback( Value* callbackFn ) {
    if( !callbackFn || !is_function( callbackFn ) )
        m_pErrorCallback.reset();

    m_pErrorCallback = frantic::max3d::mxs::make_shared_value( callbackFn );
}

inline void ErrorReporter::CopyCallback( ErrorReporter* other ) { m_pErrorCallback = other->m_pErrorCallback; }

inline void ErrorReporter::SetError( const frantic::magma::magma_exception& e ) {
    if( m_pErrorCallback ) {
        frantic::magma::max3d::ReportError( m_pErrorCallback.get(), e );
    } else {
        FF_LOG( warning ) << _T("No error handler is registered") << std::endl;
    }

    FF_LOG( error ) << e.get_message( true ) << std::endl;
}

inline void ErrorReporter::ClearError() {
    if( m_pErrorCallback ) {
        frantic::magma::max3d::ClearError( m_pErrorCallback.get() );
    } else {
        FF_LOG( warning ) << _T("No error handler is registered") << std::endl;
    }
}

} // namespace max3d
} // namespace magma
} // namespace frantic
