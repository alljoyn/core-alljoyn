/**
 * @file
 * This contains the AboutProxy class
 */
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

#ifndef _ALLJOYN_ABOUTPROXY_H_
#define _ALLJOYN_ABOUTPROXY_H_

#include <alljoyn/AboutObjectDescription.h>
#include <alljoyn/AboutData.h>
#include <alljoyn/BusAttachment.h>


namespace ajn {

/**
 * AboutProxy give proxy access to the org.alljoyn.About interface AboutProxy
 * class enables the user to interact with the remote About BusObject instance
 * exposing the following methods:
 * - GetObjectDescriptions
 * - GetAboutData
 * - GetVersion
 *
 */
class AboutProxy : public ProxyBusObject {
  public:
    /**
     * AboutProxy Constructor
     *
     * @param  bus reference to BusAttachment
     * @param[in] busName Unique or well-known name of remote AllJoyn bus
     * @param[in] sessionId the session received after joining AllJoyn session
     */
    AboutProxy(BusAttachment& bus, const char* busName, SessionId sessionId = 0);

    /**
     * Destruct AboutProxy
     */
    virtual ~AboutProxy();

    /**
     * Get the ObjectDescription array for the specified bus name.
     *
     * @param[out] objectDesc Description of busName's remote objects.
     *
     * @return
     *   - ER_OK if successful.
     *   - ER_BUS_REPLY_IS_ERROR_MESSAGE on unknown failure.
     */
    QStatus GetObjectDescription(MsgArg& objectDesc);

    /**
     * Get the AboutData for the specified bus name for a requested language.
     * The language tag will be matched against a set of supported languages
     * by the remote object using the algorithm specified in RFC 4647
     * section 3.4 to find the best matching language or use the default
     * language if no match exists.  This algorithm requires that the
     * "supported" languages be the least specific they can (e.g., "en" in
     * order to match both "en" and "en-US" if requested), and the "requested"
     * language be the most specific it can (e.g., "en-US" in order to match
     * either "en-US" or "en" if supported).
     *
     * @param[in] languageTag The requested language.
     * @param[out] data Reference of AboutData that is filled by the function.
     *
     * @return
     *    - ER_OK if successful.
     *    - ER_BUS_REPLY_IS_ERROR_MESSAGE on unknown failure.
     */
    QStatus GetAboutData(const char* languageTag, MsgArg& data);

    /**
     * GetVersion get the About version
     *
     * @param[out] version of the service.
     *
     * @return ER_OK on success
     */
    QStatus GetVersion(uint16_t& version);
};

}

#endif /* _ALLJOYN_ABOUTPROXY_H_ */