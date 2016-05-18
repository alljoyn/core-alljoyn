/**
 * @file
 * The implementation of the the default auth listener.
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

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Debug.h>
#include <qcc/Util.h>

#include <alljoyn/AuthListener.h>

#define QCC_MODULE "ALLJOYN_AUTH"

using namespace std;
using namespace qcc;

namespace ajn {

DefaultECDHEAuthListener::DefaultECDHEAuthListener() : AuthListener(), m_psk(nullptr), m_pskSize(0), m_password(nullptr), m_passwordSize(0)
{
}

DefaultECDHEAuthListener::~DefaultECDHEAuthListener()
{
    if (m_pskSize > 0) {
        ClearMemory(m_psk, m_pskSize);
        delete[] m_psk;
    }

    if (m_passwordSize > 0) {
        ClearMemory(m_password, m_passwordSize);
        delete[] m_password;
    }
}

/* Note that this constructor is deprecated because it only supports PSK. */
DefaultECDHEAuthListener::DefaultECDHEAuthListener(const uint8_t* psk, size_t pskSize) : AuthListener(), m_password(nullptr), m_passwordSize(0)
{
    /* A secret must be supplied, and have the minimum length. */
    QCC_ASSERT(pskSize >= 16);

    m_psk = new uint8_t[pskSize];
    memcpy(m_psk, psk, pskSize);
    m_pskSize = pskSize;
}



QStatus DefaultECDHEAuthListener::SetPSK(const uint8_t* psk, size_t pskSize)
{
    if ((psk != nullptr) && (pskSize < 16)) {
        return ER_BAD_ARG_2;
    }

    ClearMemory(m_psk, m_pskSize);
    delete[] m_psk;
    m_psk = NULL;
    m_pskSize = 0;

    if (psk != nullptr) {
        m_psk = new uint8_t[pskSize];
        memcpy(m_psk, psk, pskSize);
        m_pskSize = pskSize;
    }
    return ER_OK;
}

QStatus DefaultECDHEAuthListener::SetPassword(const uint8_t* password, size_t passwordSize)
{
    if ((password != nullptr) && (passwordSize < 4)) {
        return ER_BAD_ARG_2;
    }

    ClearMemory(m_password, m_passwordSize);
    delete[] m_password;
    m_password = NULL;
    m_passwordSize = 0;

    if (password != nullptr) {
        m_password = new uint8_t[passwordSize];
        memcpy(m_password, password, passwordSize);
        m_passwordSize = passwordSize;
    }
    return ER_OK;
}

bool DefaultECDHEAuthListener::RequestCredentials(const char* authMechanism, const char* peerName, uint16_t authCount, const char* userName, uint16_t credMask, Credentials& credentials) {
    QCC_DbgTrace(("%s", __FUNCTION__));
    QCC_UNUSED(peerName);
    QCC_UNUSED(userName);
    QCC_UNUSED(credMask);

    if (strcmp(authMechanism, "ALLJOYN_ECDHE_NULL") == 0) {
        return true;
    } else if (strcmp(authMechanism, "ALLJOYN_ECDHE_PSK") == 0) {     /* Marked deprecated in 16.04 */
        if (m_pskSize == 0) {
            QCC_DbgHLPrintf(("DefaultECDHEAuthListener::RequestCredentials called for ECDHE_PSK, but no PSK value is set, authentication will fail."));
            return false;
        }
        qcc::String outpsk;
        outpsk.assign_std(reinterpret_cast<const char*>(m_psk), m_pskSize);
        credentials.SetPassword(outpsk);    /* The credentials class has only one way to store pre-shared credentials. */
        outpsk.clear();
        return true;
    } else if (strcmp(authMechanism, "ALLJOYN_ECDHE_SPEKE") == 0) {
        if (m_passwordSize == 0) {
            QCC_DbgHLPrintf(("DefaultECDHEAuthListener::RequestCredentials called for ECDHE_SPEKE, but no password value is set, authentication will fail."));
            return false;
        }
        if (authCount > 10) {
            /* If the peer making a large number of attempts, they may be an attacking trying to guess the password. */
            QCC_DbgHLPrintf(("DefaultECDHEAuthListener::RequestCredentials called for ECDHE_SPEKE more than 10 times, authentication will fail."));
            return false;
        }
        qcc::String outpassword;
        outpassword.assign_std(reinterpret_cast<const char*>(m_password), m_passwordSize);
        credentials.SetPassword(outpassword);
        outpassword.clear();
        return true;
    } else if (strcmp(authMechanism, "ALLJOYN_ECDHE_ECDSA") == 0) {
        return true;
    }
    return false;
}

void DefaultECDHEAuthListener::AuthenticationComplete(const char* authMechanism, const char* peerName, bool success)
{
    QCC_DbgTrace(("%s: peerName = %s, success = %u", __FUNCTION__, peerName, (uint32_t)success));
    QCC_UNUSED(authMechanism);
    QCC_UNUSED(peerName);
    QCC_UNUSED(success);
}

}
