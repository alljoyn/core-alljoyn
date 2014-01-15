#ifndef _STUNATTRIBUTESOFTWARE_H
#define _STUNATTRIBUTESOFTWARE_H
/**
 * @file
 *
 * This file defines the SOFTWARE STUN message attribute.
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
#error Only include StunAttributeSoftware.h in C++ code.
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
 * Software STUN attribute class.
 */
class StunAttributeSoftware : public StunAttributeStringBase {
  public:
    /**
     * This constructor just sets the attribute type to STUN_ATTR_SOFTWARE.
     */
    StunAttributeSoftware(void) : StunAttributeStringBase(STUN_ATTR_SOFTWARE, "SOFTWARE") { }

    /**
     * This constructor just sets the attribute type to STUN_ATTR_SOFTWARE.
     *
     * @param software  The software description as std::string.
     */
    StunAttributeSoftware(const String& software) :
        StunAttributeStringBase(STUN_ATTR_SOFTWARE, "SOFTWARE", software)
    { }

    /**
     * Retrieve the software.
     *
     * @param software  OUT: A copy the software std::string.
     */
    void GetSoftware(String& software) const { GetStr(software); }

    /**
     * Set the software.
     *
     * @param software A reference to the software std::string.
     */
    void SetSoftware(const String& software) { SetStr(software); }
};


#undef QCC_MODULE
#endif
