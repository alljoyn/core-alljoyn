/**
 * @file STLContainer.h
 *
 *
 *
 */

/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 ******************************************************************************/

#ifndef _STLCONTAINER_H
#define _STLCONTAINER_H

#include <qcc/platform.h>


/***************************
 * STANDARD C++11
 ***************************/
#if (__cplusplus >= 201100L) && !defined(QCC_OS_DARWIN)
/*
 * The compiler is conformant to the C++11 standard.  Use unordered_map,
 * etc. directly.
 */
#include <unordered_map>
#include <unordered_set>

#define _BEGIN_NAMESPACE_CONTAINER_FOR_HASH namespace std {
#define _END_NAMESPACE_CONTAINER_FOR_HASH }


#else
/*
 * Compiling with a C++ compiler that predates the C++11 standard.  Need to
 * map compiler specific arrangements to match the standard arrangement.
 */



/***************************
 * DARWIN
 ***************************/
#if defined(QCC_OS_DARWIN)
/*
 * Darwin (Mac OSX and iOS) currently put unordered_map, etc. under the tr1
 * subdirectory and place their template classes in the std::tr1 namespace.
 */
#include <tr1/unordered_map>
#include <tr1/unordered_set>

#define _BEGIN_NAMESPACE_CONTAINER_FOR_HASH namespace std { namespace tr1 {
#define _END_NAMESPACE_CONTAINER_FOR_HASH } }

namespace std {
/*
 * Map everything in the std::tr1 namespace to the std namespace.
 */
using namespace std::tr1;
}



/***************************
 * MSVC 2008, 2010
 ***************************/
#elif defined(_MSC_VER)
#if (_MSC_VER >= 1500)
/*
 * MSVC 2008 and later provide unordered_map, etc. in the standard location.
 */
#include <unordered_map>
#include <unordered_set>

#if (_MSC_VER >= 1600)
/*
 * MSVC 2010 actually follows the C++11 standard for unordered_map, etc. but
 * we are here because it still defined __cplusplus to 199711.
 */
#define _BEGIN_NAMESPACE_CONTAINER_FOR_HASH namespace std {
#define _END_NAMESPACE_CONTAINER_FOR_HASH }

#elif (_MSC_VER >= 1500)
/*
 * MSVC 2008 puts unordered_map, etc. in the std::tr1 namespace.
 */
#define _BEGIN_NAMESPACE_CONTAINER_FOR_HASH namespace std { namespace tr1 {
#define _END_NAMESPACE_CONTAINER_FOR_HASH } }

namespace std {
/*
 * Map everything in the std::tr1 namespace to the std namespace.
 */
using namespace std::tr1;
}

#endif  // MSVC versions

#endif  // MSVC version >= 2008


/***************************
 * GCC
 ***************************/
#elif defined(__GNUC__)
/*
 * Older versions of GCC use hash_map, etc. instead of unordered_map,
 * etc. respectively.  Thus we need to map the class names to unordered_*.
 * Additionally, the hash_* classes are in the __gnu_cxx namespace.
 */
#include <ext/hash_map>
#include <ext/hash_set>

#define _BEGIN_NAMESPACE_CONTAINER_FOR_HASH namespace __gnu_cxx {
#define _END_NAMESPACE_CONTAINER_FOR_HASH }

#define unordered_map hash_map
#define unordered_multimap hash_multimap
#define unordered_set hash_set
#define unordered_multiset hash_multiset

namespace std {
/*
 * Map everything in the __gnu_cxx namespace to the std namespace.
 */
using namespace __gnu_cxx;
}


#else
#error Unsupported Compiler/Platform

#endif  // Platforms for C++ prior to C++11
#endif  // Standard C++11
#endif  // _STLCONTAINER_H
