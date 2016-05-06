/**
 * @file
 *
 * Defines QCC_ASSERT for Jenkins Windows builds
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

/**
 * Hitting _ASSERTE or a regular assert would display a UI window and prevent us
 * from creating automatic memory dumps in Jenkins for Windows builds. This version
 * of QCC_ASSERT forces a dereference of a null pointer, which does not display
 * the UI window, but immediately skips to the default debugger.
 */
#define QCC_ASSERT(expr) ((void) (                                                                                  \
                              (!!(expr)) ||                                                                         \
                              (printf("%s(%d) : Assertion failed for expression: %s.", __FILE__, __LINE__, # expr), \
                               fflush(stdout),                                                                      \
                               *((volatile int*)0))                                                                 \
                              ))

#define QCC_CRASH_DUMP_SUPPORT