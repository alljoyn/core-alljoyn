/**
 * @file
 *
 * This file implements the BusAttachmentC class.
 */

/******************************************************************************
 * Copyright (c) 2012-2013, AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_C_MSGARGC_H
#define _ALLJOYN_C_MSGARGC_H

#include <alljoyn/Message.h>
#include <alljoyn/MsgArg.h>
#include <MsgArgUtils.h>
#include <alljoyn_c/Status.h>
namespace ajn {
/**
 * MsgArgC is used to map 'C' code to the 'C++' MsgArg code where additional handling
 * is needed to convert from a 'C' type to a 'C++' type.
 */
class MsgArgC : public MsgArg {
    friend class MsgArgUtils;
  public:
    /**
     * constructor
     */
    MsgArgC() : MsgArg() { }

    /**
     * copy constructor
     */
    MsgArgC(const MsgArgC& other) : MsgArg(static_cast<MsgArg>(other)) { }

    /**
     * Constructor
     *
     * @param typeId  The type for the MsgArg
     */
    MsgArgC(AllJoynTypeId typeId) : MsgArg(typeId) { }

    static QStatus MsgArgUtilsSetVC(MsgArg* args, size_t& numArgs, const char* signature, va_list* argp);

    static QStatus VBuildArgsC(const char*& signature, size_t sigLen, MsgArg* arg, size_t maxArgs, va_list* argp, size_t* count = NULL);
    static QStatus VParseArgsC(const char*& signature, size_t sigLen, const MsgArg* argList, size_t numArgs, va_list* argp);
};
}
#endif
