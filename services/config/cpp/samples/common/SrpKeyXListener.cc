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

#include "SrpKeyXListener.h"
#include <iostream>

SrpKeyXListener::SrpKeyXListener() : m_PassCode("000000"), m_GetPassCode(0)
{


}

SrpKeyXListener::~SrpKeyXListener()
{

}

void SrpKeyXListener::setPassCode(qcc::String const& passCode)
{
    m_PassCode = passCode;
}

void SrpKeyXListener::setGetPassCode(void (*getPassCode)(qcc::String&))
{
    m_GetPassCode = getPassCode;
}

bool SrpKeyXListener::RequestCredentials(const char* authMechanism, const char* authPeer,
                                         uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds)
{
    QCC_UNUSED(userId);
    std::cout << "RequestCredentials for authenticating " << authPeer << " using mechanism " << authMechanism << std::endl;
    if (strcmp(authMechanism, "ALLJOYN_SRP_KEYX") == 0 || strcmp(authMechanism, "ALLJOYN_ECDHE_PSK") == 0) {
        if (credMask & AuthListener::CRED_PASSWORD) {
            if (authCount <= 3) {
                qcc::String passCodeFromGet;
                if (m_GetPassCode) {
                    m_GetPassCode(passCodeFromGet);
                }
                std::cout << "RequestCredentials setPasscode to " << (m_GetPassCode ? passCodeFromGet.c_str() : m_PassCode.c_str()) << std::endl;
                creds.SetPassword(m_GetPassCode ? passCodeFromGet.c_str() : m_PassCode.c_str());
                return true;
            } else {
                return false;
            }
        }
    }
    return false;
}

void SrpKeyXListener::AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success)
{
    QCC_UNUSED(authPeer);
    std::cout << "Authentication with " << authMechanism << (success ? " was successful" : " failed") << std::endl;
}
