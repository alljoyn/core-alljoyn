// ****************************************************************************
// Copyright (c) 2012, AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
// ******************************************************************************

#pragma once

// Enable logging in Debug mode. No logging in Release mode
#define ENABLE_DEBUG_LOG

#if !defined(NDEBUG) && defined(ENABLE_DEBUG_LOG)
  #define QCC_DbgHLPrintf(x)            \
    do                                  \
    {                                   \
        PrintDebugOutput x;             \
    } while (0);

#else
  #define QCC_DbgHLPrintf(x) do { } while (0)
#endif

// Errors always logged even int Release mode
#define QCC_LogError(_status, _msg)     \
    do                                  \
    {                                   \
        PrintDebugOutput _msg;          \
    } while (0);

#if !defined(NDEBUG) && defined(ENABLE_DEBUG_LOG)
  #define QCC_DbgPrintf(x)              \
    do                                  \
    {                                   \
        PrintDebugOutput x;             \
    } while (0);
#else
  #define QCC_DbgPrintf(x) do { } while (0)
#endif

void PrintDebugOutput(const char* szFormat, ...);
