#ifndef _QCC_KEYINFO_ECC_H
#define _QCC_KEYINFO_ECC_H
/**
 * @file
 *
 * This file provide ECC public key info
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

#include <qcc/platform.h>
#include <qcc/KeyInfo.h>
#include <qcc/CryptoECC.h>

namespace qcc {

/**
 * ECC KeyInfo
 */
class KeyInfoECC : public KeyInfo {

  public:

    /**
     * The ECC key type
     */
    static const size_t KEY_TYPE = 0;

    /**
     * The ECC algorithm
     */
    static const size_t ALGORITHM_ECDSA_SHA_256 = 0;

    /**
     * The size of the ECC coordinate
     */
    static const size_t ECC_COORDINATE_SZ = ECC_BIGVAL_SZ * sizeof(uint32_t);

    /**
     * Default constructor.
     */
    KeyInfoECC() : KeyInfo(FORMAT_ALLJOYN), curve(Crypto_ECC::ECC_NIST_P256)
    {
    }

    /**
     * constructor.
     */
    KeyInfoECC(uint8_t curve) : KeyInfo(FORMAT_ALLJOYN), curve(curve)
    {
    }

    /**
     * Default destructor.
     */
    virtual ~KeyInfoECC()
    {
    }

    /**
     * Retrieve the ECC algorithm
     * @return the ECC algorithm
     */
    const uint8_t GetAlgorithm()
    {
        return ALGORITHM_ECDSA_SHA_256;
    }

    /**
     * Retrieve the ECC curve type.
     * @return the ECC curve type
     */
    const uint8_t GetCurve()
    {
        return curve;
    }

  private:
    uint8_t curve;
};

/**
 * NIST P-256 ECC KeyInfo
 */
class KeyInfoNISTP256 : public KeyInfoECC {

  public:

    /**
     * Default constructor.
     */
    KeyInfoNISTP256() : KeyInfoECC(Crypto_ECC::ECC_NIST_P256)
    {
    }


    /**
     * Default destructor.
     */
    virtual ~KeyInfoNISTP256()
    {
    }

    /**
     * Assign the X coordinate
     * @param xCoord the X coordinate value to copy
     */
    void SetXCoord(const uint8_t* xCoord)
    {
        memcpy(x, xCoord, ECC_COORDINATE_SZ);
    }

    /**
     * Retrieve the X coordinate value
     * @return the ECC X coordinate value.  It's a pointer to an internal buffer. Its lifetime is the same as the object's lifetime.
     */
    const uint8_t* GetXCoord()
    {
        return x;
    }

    /**
     * Assign the Y coordinate
     * @param yCoord the Y coordinate value to copy
     */
    void SetYCoord(const uint8_t* yCoord)
    {
        memcpy(y, yCoord, ECC_COORDINATE_SZ);
    }

    /**
     * Retrieve the Y coordinate value
     * @return the ECC Y coordinate value.  It's a pointer to an internal buffer. Its lifetime is the same as the object's lifetime.
     */
    const uint8_t* GetYCoord()
    {
        return y;
    }

    /**
     * Export data to an ECCPublicKey object
     * @param[out] pubKey the output ECCPublicKey object
     * @return
     *      ER_OK if the export succeeds
     *      ER_FAIL otherwise
     */
    QStatus Export(ECCPublicKey* pubKey);

    /**
     * Import data from an ECCPublicKey object
     * @param pubKey the ECCPublicKey object
     * @return
     *      ER_OK if the import succeeds
     *      ER_FAIL otherwise
     */
    QStatus Import(const ECCPublicKey* pubKey);

  private:
    uint8_t x[ECC_COORDINATE_SZ];
    uint8_t y[ECC_COORDINATE_SZ];
};

} /* namespace qcc */


#endif
