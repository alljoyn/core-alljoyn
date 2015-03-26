#ifndef _QCC_PLATFORMCPP_H
#define _QCC_PLATFORMCPP_H
/**
 * @file
 *
 * This file contains common definitions for C++ binding.
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

#include <qcc/platform.h>

#if defined(QCC_OS_GROUP_WINDOWS)
/*
 * This pragma prevents Microsoft compiler warning C4407: cast between different pointer to member representations, compiler may generate incorrect code.
 * This is equivalent to /vmm and /vmg compiler options but without the burden of requiring all apps to specify these non-default options.
 */
#pragma pointers_to_members(full_generality, virtual_inheritance)
#endif /* Compiler type */
#endif /* _QCC_PLATFORMCPP_H */
