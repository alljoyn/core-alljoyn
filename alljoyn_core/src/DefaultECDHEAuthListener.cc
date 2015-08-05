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

#include <alljoyn/AuthListener.h>

#define QCC_MODULE "ALLJOYN_AUTH"

using namespace std;
using namespace qcc;

namespace ajn {

DefaultECDHEAuthListener::DefaultECDHEAuthListener() : AuthListener(), psk(NULL), pskSize(0)
{
}

DefaultECDHEAuthListener::DefaultECDHEAuthListener(const uint8_t* psk, size_t pskSize) : AuthListener()
{
    assert(pskSize >= 16);  /* psk size should be 128 bits or longer */
    this->psk = new uint8_t[pskSize];
    assert(this->psk);
    memcpy(this->psk, psk, pskSize);
    this->pskSize = pskSize;
}

QStatus DefaultECDHEAuthListener::SetPSK(const uint8_t* _psk, size_t _pskSize)
{
    delete[] psk;
    if (_psk) {
        if (_pskSize < 16) {
            return ER_BAD_ARG_2;
        }
        psk = new uint8_t[_pskSize];
        memcpy(psk, _psk, _pskSize);
        pskSize = _pskSize;
    } else {
        pskSize = 0;
        psk = NULL;
    }
    return ER_OK;
}


bool DefaultECDHEAuthListener::RequestCredentials(const char* authMechanism, const char* peerName, uint16_t authCount, const char* userName, uint16_t credMask, Credentials& credentials) {
    QCC_DbgTrace(("DefaultECDHEAuthListener::%s", __FUNCTION__));
    QCC_UNUSED(peerName);
    QCC_UNUSED(authCount);
    QCC_UNUSED(userName);
    QCC_UNUSED(credMask);
    QCC_UNUSED(credentials);
    if (strcmp(authMechanism, "ALLJOYN_ECDHE_NULL") == 0) {
        return true;
    } else if (strcmp(authMechanism, "ALLJOYN_ECDHE_PSK") == 0) {
        if (pskSize == 0) {
            return false;
        }
        qcc::String outPsk;
        outPsk.assign(reinterpret_cast<const char*>(psk), pskSize);
        credentials.SetPassword(outPsk);
        return true;
    } else if (strcmp(authMechanism, "ALLJOYN_ECDHE_ECDSA") == 0) {
        return true;
    }
    return false;
}

void DefaultECDHEAuthListener::AuthenticationComplete(const char* authMechanism, const char* peerName, bool success)
{
    QCC_DbgTrace(("DefaultECDHEAuthListener::%s", __FUNCTION__));
    QCC_UNUSED(authMechanism);
    QCC_UNUSED(peerName);
    QCC_UNUSED(success);
}

}
