#ifndef _STUNATTRIBUTEUSECANDIDATE_H
#define _STUNATTRIBUTEUSECANDIDATE_H
/**
 * @file
 *
 * This file defines the USE-CANDIDATE STUN message attribute.
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
#error Only include StunAddr/StunAttributeUseCandidate.h in C++ code.
#endif

#include <string>
#include <qcc/platform.h>
#include <StunAttributeBase.h>
#include <types.h>
#include <alljoyn/Status.h>

/** @internal */
#define QCC_MODULE "STUN_ATTRIBUTE"

/**
 * Use Candidate STUN attribute class.
 */
class StunAttributeUseCandidate : public StunAttribute {
  private:

  public:
    /**
     * This constructor just sets the attribute type to STUN_ATTR_USE_CANDIDATE.
     */
    StunAttributeUseCandidate(void) :
        StunAttribute(STUN_ATTR_USE_CANDIDATE, "USE-CANDIDATE") { }

    uint16_t AttrSize(void) const { return 0; /* Attribute has no data */ }
};


#undef QCC_MODULE
#endif
