// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// In Visual Studio 2003, MS's C++ standard library generates this warning (unreachable code)
#if _MSC_VER == 1300
#pragma warning( disable : 4702 )
#endif

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>
#pragma warning( push )
#pragma warning( disable : 4267 ) // Bizarre that MS's standard headers have such warnings...
#include <algorithm>
#pragma warning( pop )

#pragma warning( push, 3 )
#pragma warning( disable : 4512 4610 )
#include <boost/config.hpp>
#include <boost/function.hpp>
#include <boost/integer_fwd.hpp>
#include <boost/random.hpp>
#include <boost/smart_ptr.hpp>
#pragma warning( pop )

// Use warning level 3 for lexical cast
#pragma warning( push, 3 )
#pragma warning( disable : 4701 4702 4267 )
#include <boost/lexical_cast.hpp>
#pragma warning( pop )

// Include the Windows headers with minimal fuss, and without the min and max macros
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Bring the std::min and std::max algorithms into the global namespace so the 3ds Max SDK shuts
// up about the macros missing and uses these instead.
using std::max;
using std::min;

// the 3ds Max headers are really noisy at level 4
#pragma warning( push, 3 )
#pragma warning( disable : 4100 4702 )

// 3ds max headers
#if !defined(NO_INIUTIL_USING)
#define NO_INIUTIL_USING
#endif
#include <Max.h>

#if MAX_RELEASE < 9000 // Only do this for the max 8 and earlier builds
#include <max_mem.h>
#define _INC_CRTDBG // Pretend to already have included crtdbg.h
#endif

#include <Notify.h>
#include <Simpobj.h>
#include <icurvctl.h>
#include <imtl.h>
#include <iparamm2.h>
#include <modstack.h>
#include <ref.h>
#include <shaders.h>
#include <simpmod.h>
#include <splshape.h>
#include <texutil.h>
#if MAX_RELEASE >= 25000
#include <geom/point3.h>
#else
#include <point3.h>
#endif
#include <strclass.h>

// The renderer capability system was added in 3ds Max 6.
#if MAX_RELEASE >= 6000
#include <IMtlRender_Compatibility.h>
#endif

// 3ds max maxscript headers
#if MAX_RELEASE >= 14000
#include <maxscript/compiler/parser.h>
#include <maxscript/foundation/3dmath.h>
#include <maxscript/foundation/arrays.h>
#include <maxscript/foundation/colors.h>
#include <maxscript/foundation/numbers.h>
#include <maxscript/foundation/strings.h>
#include <maxscript/kernel/value.h>
#include <maxscript/maxscript.h>
#include <maxscript/maxwrapper/bitmaps.h>
#include <maxscript/maxwrapper/mxsmaterial.h>
#include <maxscript/maxwrapper/mxsobjects.h>
#else
#include <MAXScrpt/3dmath.h>
#include <MAXScrpt/MAXScrpt.h>
#include <MAXScrpt/arrays.h>
#include <MAXScrpt/bitmaps.h>
#include <MAXScrpt/maxmats.h>
#include <MAXScrpt/maxobj.h>
#include <MAXScrpt/numbers.h>
#include <MAXScrpt/strings.h>
#include <MAXScrpt/value.h>
#include <Maxscrpt/colorval.h>
#include <Maxscrpt/parser.h>
#endif

// Particle Flow headers
// TODO: We may want to isolate these to a different file.
#include <IParticleObjectExt.h>
#include <ParticleFlow/IChannelContainer.h>
#include <ParticleFlow/IPFActionList.h>
#include <ParticleFlow/IPFActionState.h>
#include <ParticleFlow/IPFOperator.h>
#include <ParticleFlow/IPViewManager.h>
#include <ParticleFlow/IParticleChannels.h>
#include <ParticleFlow/IParticleGroup.h>
#include <ParticleFlow/PFClassIDs.h>
#include <ParticleFlow/PFMessages.h>
#include <ParticleFlow/PFSimpleOperator.h>

// Include all the necessary static libraries. TODO: Perhaps we should split
// these up on a "as needed basis". For example, make an include per-namespace
// that includes the specific libraries. Ie. frantic::particles would include pflow.
#pragma comment( lib, "core.lib" )
#pragma comment( lib, "maxutil.lib" )
#pragma comment( lib, "comctl32.lib" )

#pragma comment( lib, "bmm.lib" )
#pragma comment( lib, "geom.lib" )
#pragma comment( lib, "mesh.lib" )
#pragma comment( lib, "Paramblk2.lib" )
#pragma comment( lib, "gfx.lib" )
#pragma comment( lib, "maxscrpt.lib" )
#pragma comment( lib, "MNMath.lib" )
#pragma comment( lib, "ParticleFlow.lib" )
#pragma comment( lib, "Poly.lib" )
#if MAX_VERSION_MAJOR < 24
#pragma comment( lib, "zlibdll.lib" )
#endif

#pragma warning( pop )
