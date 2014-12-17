#ifndef _ALLJOYN_KEYINFO_HELPER_H
#define _ALLJOYN_KEYINFO_HELPER_H
/**
 * @file
 * This file defines the helper function for KeyInfo data
 */

/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
#error Only include KeyInfoHelper.h in C++ code.
#endif

#include <qcc/KeyInfoECC.h>
#include <alljoyn/MsgArg.h>

namespace ajn {

class KeyInfoHelper {

  public:
    /**
     * Helper function to determine whether the keyInfo object is an instance of a KeyInfoNISTP256 object.
     * @param keyInfo the KeyInfoECC object
     * @true if the keyInfo object is a KeyInfoNISTP256 object; false, otherwise.
     */

    static bool InstanceOfKeyInfoNISTP256(const qcc::KeyInfoECC& keyInfo);

    /**
     * Helper function to generate a MsgArg for KeyInfoNISTP256 object.
     * @param keyInfo the KeyInfoNISTP256 object
     * @param variant[out] the output message arg.
     */

    static void KeyInfoNISTP256ToMsgArg(const qcc::KeyInfoNISTP256& keyInfo, MsgArg& variant);

    /**
     * Helper function to load a KeyInfoNISTP256 object using data from the message arg.
     * @param variant the input message arg.
     * @param keyInfo[out] the output KeyInfoNISTP256 object
     */
    static QStatus MsgArgToKeyInfoNISTP256(const MsgArg& variant, qcc::KeyInfoNISTP256& keyInfo);

};

}
#endif
