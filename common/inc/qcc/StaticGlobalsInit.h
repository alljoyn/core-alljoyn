#ifndef _QCC_STATICGLOBALSINIT_H
#define _QCC_STATICGLOBALSINIT_H
/**
 * @file
 * File for initializing global variables.
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

#ifndef __cplusplus
#error Only include StaticGlobalsInit.h in C++ code.
#endif

namespace qcc {

/**
 * Nifty counter used to ensure that common globals are initialized before any
 * other client code static or global variables.
 */
static struct StaticGlobalsInit {
    StaticGlobalsInit();
    ~StaticGlobalsInit();
    static void Cleanup();

  private:
    static bool cleanedup;

} staticGlobalsInit;

}

#endif // _QCC_STATICGLOBALSINIT_H
