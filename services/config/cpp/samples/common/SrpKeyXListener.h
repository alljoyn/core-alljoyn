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

#ifndef SRPKEYLISTENER_H_
#define SRPKEYLISTENER_H_

#include <alljoyn/AuthListener.h>

#if defined(QCC_OS_GROUP_WINDOWS)
/* Disabling warning C 4100. Function doesnt use all passed in parameters */
#pragma warning(push)
#pragma warning(disable: 4100)
#endif


/**
 * class SrpKeyXListener
 * A listener for Authentication
 */
class SrpKeyXListener : public ajn::AuthListener {
  public:
    /**
     * SrpKeyXListener
     */
    SrpKeyXListener();

    /**
     * ~SrpKeyXListener
     */
    virtual ~SrpKeyXListener();

    /**
     * setPassCode
     * @param passCode to set
     */
    void setPassCode(qcc::String const& passCode);

    /**
     * setGetPassCode
     * @param getPassCode - callback function to set
     */
    void setGetPassCode(void (*getPassCode)(qcc::String&));

    /**
     * RequestCredentials
     * @param authMechanism
     * @param authPeer
     * @param authCount
     * @param userId
     * @param credMask
     * @param creds
     * @return boolean
     */
    bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId,
                            uint16_t credMask, Credentials& creds);

    /**
     * AuthenticationComplete
     * @param authMechanism
     * @param authPeer
     * @param success
     */
    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success);

  private:
    qcc::String m_PassCode;

    void (*m_GetPassCode)(qcc::String&);
};

#endif /* SRPKEYLISTENER_H_ */


