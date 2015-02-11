/**
 * @file
 * this file helps handle differences in wchar_t on different OSs
 */
/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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
