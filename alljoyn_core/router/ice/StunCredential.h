#ifndef _STUNCREDENTIAL_H
#define _STUNCREDENTIAL_H
/**
 * @file
 *
 * This file defines the STUN credential class, used for long and short-term
 * credentials.
 *
 */

/******************************************************************************
 * Copyright (c) 2009,2012 AllSeen Alliance. All rights reserved.
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
#error Only include StunCredential.h in C++ code.
#endif

#include <string>
#include <qcc/platform.h>
#include <alljoyn/Status.h>

using namespace std;
using namespace qcc;

/** @internal */
#define QCC_MODULE "STUNCREDENTIAL"

/**
 * @class StunCredential
 *
 * This class computes the HMAC key for the MESSAGE-INTEGRITY
 * attribute, per RFC 5389, based on whether the credential
 * is long-term or short-term.
 *
 */
class StunCredential {
  private:

    /* Just defined to make klocwork happy. Should never be used */
    StunCredential(const StunCredential& other) {
        assert(false);
    }

    /* Just defined to make klocwork happy. Should never be used */
    StunCredential& operator=(const StunCredential& other) {
        assert(false);
        return *this;
    }

    const String password;

    uint8_t* hmacKey;
    uint8_t keyLength;

    void ComputeShortTermKey(void);

    String SASLprep(const String& in) const;

  public:

    StunCredential(const String& password) :
        password(password), hmacKey(NULL), keyLength(0) { ComputeShortTermKey(); }

    /**
     * Destructor for the StunCredential class.  This will delete the key, if any.
     *
     */
    ~StunCredential(void) { free(hmacKey); }


    QStatus GetKey(uint8_t* keyOut, size_t& len) const;

};


#undef QCC_MODULE
#endif
