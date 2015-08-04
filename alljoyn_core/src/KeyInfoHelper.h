#ifndef _ALLJOYN_KEYINFO_HELPER_H
#define _ALLJOYN_KEYINFO_HELPER_H
/**
 * @file
 * This file defines the helper function for KeyInfo data
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
     * @return true if the keyInfo object is a KeyInfoNISTP256 object; false, otherwise.
     */

    static bool InstanceOfKeyInfoNISTP256(const qcc::KeyInfoECC& keyInfo);

    /**
     * Helper function to generate a MsgArg for KeyInfoNISTP256 object.
     * @param keyInfo the KeyInfoNISTP256 object
     * @param[out] variant the output message arg.
     */

    static void KeyInfoNISTP256ToMsgArg(const qcc::KeyInfoNISTP256& keyInfo, MsgArg& variant);

    /**
     * Helper function to load a KeyInfoNISTP256 object using data from the message arg.
     * @param variant the input message arg.
     * @param[out] keyInfo the output KeyInfoNISTP256 object
     * @return ER_OK if successful; otherwise, error code.
     */
    static QStatus MsgArgToKeyInfoNISTP256(const MsgArg& variant, qcc::KeyInfoNISTP256& keyInfo);

    /**
     * Helper function to generate a MsgArg for KeyInfoNISTP256 public key.
     * @param keyInfo the KeyInfoNISTP256 object
     * @param[out] msgArg the output message arg.
     * @param setKeyId set the keyId value
     */
    static void KeyInfoNISTP256PubKeyToMsgArg(const qcc::KeyInfoNISTP256& keyInfo, MsgArg& msgArg, bool setKeyId = false);

    /**
     * Helper function to load a KeyInfoNISTP256 public key using data from the
     * message arg.
     * @param msgArg the input message arg.
     * @param[out] keyInfo the output KeyInfoNISTP256 object
     * @param retrieveKeyId retrieve the keyId value
     * @return ER_OK if successful; otherwise, error code.
     */
    static QStatus MsgArgToKeyInfoNISTP256PubKey(const MsgArg& msgArg, qcc::KeyInfoNISTP256& keyInfo, bool retrieveKeyId = false);

    /**
     * Helper function to load an authority key id using data from the
     * message arg.
     * @param msgArg the input message arg.
     * @param[in,out] keyInfo the keyInfo object
     * @return ER_OK if successful; otherwise, error code.
     */
    static QStatus MsgArgToKeyInfoKeyId(const MsgArg& msgArg, qcc::KeyInfoNISTP256& keyInfo);

    /**
     * Helper function to generate a MsgArg for authority key id.
     * @param keyInfo the keyInfo object
     * @param[out] arg the output message arg.
     */
    static void KeyInfoKeyIdToMsgArg(const qcc::KeyInfoNISTP256& keyInfo, MsgArg& msgArg);

    /**
     * Helper function to generate the key id using the authority
     * key id generation algorithm.
     * @param[in,out] keyInfo the key info object
     * @return ER_OK if successfull; otherwise, error code.
     */
    static QStatus GenerateKeyId(qcc::KeyInfoNISTP256& keyInfo);

    /**
     * Helper function to export the ECC public key to coordinates.
     * @param[in] publicKey the ECC public key
     * @param[out] xData the buffer to hold the x coordinate
     * @param[in] xSize the size of the x coordinate buffer
     * @param[out] yData the buffer to hold the y coordinate
     * @param[in] ySize the size of the y coordinate buffer
     * @return ER_OK if successfull; otherwise, error code.
     */
    static QStatus ExportCoordinates(const qcc::ECCPublicKey& publicKey, uint8_t* xData, const size_t xSize, uint8_t* yData, const size_t ySize);
};

}
#endif
