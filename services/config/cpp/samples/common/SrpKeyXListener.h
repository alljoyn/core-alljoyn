/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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

