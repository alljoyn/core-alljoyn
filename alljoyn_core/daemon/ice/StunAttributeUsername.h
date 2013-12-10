#ifndef _STUNATTRIBUTEUSERNAME_H
#define _STUNATTRIBUTEUSERNAME_H
/**
 * @file
 *
 * This file defines the USERNAME STUN message attribute.
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
#error Only include StunAttributeBase.h in C++ code.
#endif

#include <string>
#include <qcc/platform.h>
#include <qcc/unicode.h>
#include <StunAttributeStringBase.h>
#include <types.h>
#include <alljoyn/Status.h>

using namespace qcc;

/** @internal */
#define QCC_MODULE "STUN_ATTRIBUTE"

/**
 * Username STUN attribute class.
 */
class StunAttributeUsername : public StunAttributeStringBase {
  public:
    /**
     * This constructor just sets the attribute type to STUN_ATTR_USERNAME.
     */
    StunAttributeUsername(void) : StunAttributeStringBase(STUN_ATTR_USERNAME, "USERNAME") { }

    /**
     * This constructor just sets the attribute type to STUN_ATTR_USERNAME.
     *
     * @param username  The username as std::string.
     */
    StunAttributeUsername(const String& username) :
        StunAttributeStringBase(STUN_ATTR_USERNAME, "USERNAME", username)
    { }

    /**
     * Retrieves the parsed UTF-8 username.
     *
     * @param username OUT: A reference to where to copy the username.
     */
    void GetUsername(String& username) const { GetStr(username); }

    /**
     * Sets the UTF-8 username.
     *
     * @param username A reference the username.
     */
    void SetUsername(const String& username) { SetStr(username); }
};


#undef QCC_MODULE
#endif
