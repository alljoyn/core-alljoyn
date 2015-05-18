/**
 * @file
 * This file defines the implementation of the KeyInfoHelper
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

#include "KeyInfoHelper.h"
#include <qcc/CryptoECC.h>
#include <qcc/CertificateECC.h>
#include <qcc/Debug.h>

#define QCC_MODULE "PERMISSION_MGMT"

using namespace std;
using namespace qcc;

namespace ajn {

bool KeyInfoHelper::InstanceOfKeyInfoNISTP256(const qcc::KeyInfoECC& keyInfo)
{
    return keyInfo.GetCurve() == Crypto_ECC::ECC_NIST_P256;
}

void KeyInfoHelper::KeyInfoNISTP256ToMsgArg(const KeyInfoNISTP256& keyInfo, MsgArg& variant)
{
    MsgArg coordArg("(ayay)", ECC_COORDINATE_SZ, keyInfo.GetXCoord(), ECC_COORDINATE_SZ, keyInfo.GetYCoord());

    variant.Set("(yv)", KeyInfo::FORMAT_ALLJOYN,
                new MsgArg("(ayyyv)", keyInfo.GetKeyIdLen(), keyInfo.GetKeyId(), KeyInfo::USAGE_SIGNING, KeyInfoECC::KEY_TYPE,
                           new MsgArg("(yyv)", keyInfo.GetAlgorithm(), keyInfo.GetCurve(), new MsgArg(coordArg))));
    variant.SetOwnershipFlags(MsgArg::OwnsArgs, true);
}

void KeyInfoHelper::KeyInfoNISTP256PubKeyToMsgArg(const KeyInfoNISTP256& keyInfo, MsgArg& msgArg)
{
    MsgArg localArg;
    QStatus status = localArg.Set("(yyayay)", keyInfo.GetAlgorithm(),
                                  keyInfo.GetCurve(), ECC_COORDINATE_SZ, keyInfo.GetXCoord(),
                                  ECC_COORDINATE_SZ, keyInfo.GetYCoord());
    if (ER_OK != status) {
        QCC_LogError(status, ("KeyInfoHelper::KeyInfoNISTP256PubKeyToMsgArg failed"));
        assert(false);
    }
    /* copy the message arg for a deep copy of the array arguments */
    msgArg = localArg;
}

QStatus KeyInfoHelper::MsgArgToKeyInfoNISTP256PubKey(const MsgArg& msgArg, KeyInfoNISTP256& keyInfo)
{
    QStatus status;
    uint8_t algorithm;
    uint8_t curve;
    uint8_t* xCoord;
    size_t xLen;
    uint8_t* yCoord;
    size_t yLen;
    status = msgArg.Get("(yyayay)", &algorithm, &curve, &xLen, &xCoord, &yLen, &yCoord);
    if (ER_OK != status) {
        return ER_INVALID_DATA;
    }
    if (algorithm != SigInfo::ALGORITHM_ECDSA_SHA_256) {
        return ER_INVALID_DATA;
    }
    if (curve != Crypto_ECC::ECC_NIST_P256) {
        return ER_INVALID_DATA;
    }
    if ((xLen != ECC_COORDINATE_SZ) || (yLen != ECC_COORDINATE_SZ)) {
        return ER_INVALID_DATA;
    }
    keyInfo.SetXCoord(xCoord);
    keyInfo.SetYCoord(yCoord);
    return ER_OK;
}

QStatus KeyInfoHelper::MsgArgToKeyInfoKeyId(const MsgArg& msgArg, KeyInfoNISTP256& keyInfo)
{
    uint8_t* buf;
    size_t len;
    QStatus status = msgArg.Get("ay", &len, &buf);
    if (ER_OK != status) {
        return ER_INVALID_DATA;
    }
    keyInfo.SetKeyId(buf, len);
    return ER_OK;
}

void KeyInfoHelper::KeyInfoKeyIdToMsgArg(const KeyInfoNISTP256& keyInfo, MsgArg& msgArg)
{
    MsgArg localArg;
    QStatus status = localArg.Set("ay", keyInfo.GetKeyIdLen(), keyInfo.GetKeyId());
    if (ER_OK != status) {
        QCC_LogError(status, ("KeyInfoHelper::KeyInfoKeyIdToMsgArg failed"));
        assert(false);
    }
    /* copy the message arg for a deep copy of the array arguments */
    msgArg = localArg;
}

QStatus KeyInfoHelper::MsgArgToKeyInfoNISTP256(const MsgArg& variant, KeyInfoNISTP256& keyInfo)
{
    QStatus status;
    uint8_t keyFormat;
    MsgArg* variantArg;
    status = variant.Get("(yv)", &keyFormat, &variantArg);
    if (ER_OK != status) {
        return ER_INVALID_DATA;
    }
    if (keyFormat != KeyInfo::FORMAT_ALLJOYN) {
        return ER_INVALID_DATA;
    }
    uint8_t* kid;
    size_t kidLen;
    uint8_t keyUsageType;
    uint8_t keyType;
    MsgArg* keyVariantArg;
    status = variantArg->Get("(ayyyv)", &kidLen, &kid, &keyUsageType, &keyType, &keyVariantArg);
    if (ER_OK != status) {
        return ER_INVALID_DATA;
    }
    if ((keyUsageType != KeyInfo::USAGE_SIGNING) && (keyUsageType != KeyInfo::USAGE_ENCRYPTION)) {
        return ER_INVALID_DATA;
    }
    if (keyType != KeyInfoECC::KEY_TYPE) {
        return ER_INVALID_DATA;
    }
    uint8_t algorithm;
    uint8_t curve;
    MsgArg* curveVariant;
    status = keyVariantArg->Get("(yyv)", &algorithm, &curve, &curveVariant);
    if (ER_OK != status) {
        return ER_INVALID_DATA;
    }
    if (curve != Crypto_ECC::ECC_NIST_P256) {
        return ER_INVALID_DATA;
    }

    uint8_t* xCoord;
    size_t xLen;
    uint8_t* yCoord;
    size_t yLen;
    status = curveVariant->Get("(ayay)", &xLen, &xCoord, &yLen, &yCoord);
    if (ER_OK != status) {
        return ER_INVALID_DATA;
    }
    if ((xLen != ECC_COORDINATE_SZ) || (yLen != ECC_COORDINATE_SZ)) {
        return ER_INVALID_DATA;
    }
    keyInfo.SetXCoord(xCoord);
    keyInfo.SetYCoord(yCoord);
    keyInfo.SetKeyId(kid, kidLen);
    return ER_OK;
}

QStatus KeyInfoHelper::GenerateKeyId(KeyInfoNISTP256& keyInfo)
{
    String aki;
    QStatus status = CertificateX509::GenerateAuthorityKeyId(keyInfo.GetPublicKey(), aki);
    if (ER_OK != status) {
        return status;
    }
    keyInfo.SetKeyId((const uint8_t*) aki.data(), aki.size());
    return ER_OK;
}

}
