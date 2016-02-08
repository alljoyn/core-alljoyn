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

#ifndef COMMONSAMPLEUTIL_H_
#define COMMONSAMPLEUTIL_H_

#include <alljoyn/BusAttachment.h>
#include <qcc/String.h>
#include <alljoyn/AboutData.h>
#include <alljoyn/AboutObj.h>
#include "AboutObjApi.h"
#include "CommonBusListener.h"
#include <map>

typedef std::map<qcc::String, qcc::String> DeviceNamesType;

/**
 * Util class that creates a busAttachment, starts it and connects it
 */
class CommonSampleUtil {
  public:

    /**
     * static method that prepares the BusAttachment
     * @param authListener - authListener if want to run secured
     * @return the BusAttachment created. NULL if failed
     */
    static ajn::BusAttachment* prepareBusAttachment(ajn::AuthListener* authListener = 0);

    /**
     * static method fillAboutData
     * @param aboutData
     * @param appIdHex
     * @param appName
     * @param deviceId
     * @param deviceNames
     * @param defaultLanguage
     * @return the property store created. NULL if failed
     */
    static QStatus fillAboutData(ajn::AboutData* aboutData, qcc::String const& appIdHex,
                                 qcc::String const& appName, qcc::String const& deviceId, DeviceNamesType const& deviceNames,
                                 qcc::String const& defaultLanguage = "en");

    /**
     * static method prepareAboutService
     * @param bus
     * @param aboutData
     * @param aboutObj
     * @param busListener
     * @param port
     * @return Qstatus
     */
    static QStatus prepareAboutService(ajn::BusAttachment* bus, ajn::AboutData* aboutData, ajn::AboutObj* aboutObj,
                                       CommonBusListener* busListener, uint16_t port);

    /**
     * static method aboutServiceAnnounce
     * @return Qstatus
     */
    static QStatus aboutServiceAnnounce();

    /**
     * static method aboutServiceDestory
     * @param bus
     * @param busListener
     */
    static void aboutServiceDestroy(ajn::BusAttachment* bus, CommonBusListener* busListener);

  private:

    /**
     * EnableSecurity
     * @param bus
     * @param authListener
     * @return success/failure
     */
    static QStatus EnableSecurity(ajn::BusAttachment* bus, ajn::AuthListener* authListener);
};

#endif /* COMMONSAMPLEUTIL_H_ */
