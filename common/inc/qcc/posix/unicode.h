/**
 * @file
 * this file helps handle differences in wchar_t on different OSs
 */
/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/
#ifndef _PLATFORM_UNICODE_H
#define _PLATFORM_UNICODE_H

/// @cond ALLJOYN_DEV
#if __SIZEOF_WCHAR_T__ == 4
// GCC normally defines a 4-byte wchar_t
/**
 * If wchar_t is defined as 4-bytes this will convert UTF8 to wchar_t
 */
#define ConvertUTF8ToWChar ConvertUTF8toUTF32

/**
 * if wchar_t is defined as 4-bytes this will convert wchar_t to UTF8
 */
#define ConvertWCharToUTF8 ConvertUTF32toUTF8

/**
 * WideUTF is defined as a 4-byte container
 */
#define WideUTF UTF32
#else

// GCC will define a 2 byte wchar_t when running under windows or if given the
// -fshort-wchar option.
/**
 * If wchar_t is defined as 2-bytes this will convert UTF8 to wchar_t
 */
#define ConvertUTF8ToWChar ConvertUTF8toUTF16

/**
 * if wchar_t is defined as 2-bytes this will convert wchar_t to UTF8
 */
#define ConvertWCharToUTF8 ConvertUTF16toUTF8

/**
 * WideUTF is defined as a 2-byte container
 */
#define WideUTF UTF16
#endif
/// @endcond
#endif