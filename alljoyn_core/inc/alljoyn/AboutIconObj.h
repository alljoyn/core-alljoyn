/**
 * @file
 * An Alljoyn BusObject that implements the org.alljoyn.Icon interface
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

#ifndef _ALLJOYN_ABOUTICONOBJ_H
#define _ALLJOYN_ABOUTICONOBJ_H

#include <alljoyn/AboutIcon.h>
#include <alljoyn/BusObject.h>

namespace ajn {

/**
 * AboutIconObj is an AllJoyn BusObject that implements the org.alljoyn.Icon standard interface.
 * Applications that provide AllJoyn IoE services to receive info about the Icon of the service.
 */
class AboutIconObj : public BusObject {
  public:
    /**
     * version of the org.alljoyn.Icon interface
     */
    static const uint16_t VERSION;

    /**
     * Construct an About Icon BusObject.
     *
     * @param[in] bus  BusAttachment instance associated with this BusObject
     * @param[in] icon instance of an AboutIcon which holds the icon image
     */
    AboutIconObj(BusAttachment& bus, AboutIcon& icon);

    /**
     *  Destructor of the About Icon BusObject
     */
    virtual ~AboutIconObj();

  private:
    /**
     * Handles  GetUrl method call for the org.alljoyn.Icon interface
     *
     * @param[in]  member
     * @param[in]  msg reference of AllJoyn Message
     */
    void GetUrl(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Handles  GetContent method call for the org.alljoyn.Icon interface
     *
     * @param[in]  member
     * @param[in]  msg reference of AllJoyn Message
     */
    void GetContent(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Handles the GetPropery request for the org.alljoyn.Icon interface
     *
     * @param[in]  ifcName  interface name
     * @param[in]  propName the name of the properly
     * @param[in]  val reference of MsgArg out parameter.
     * @return
     */
    QStatus Get(const char* ifcName, const char* propName, MsgArg& val);

    /**
     * pointer to BusAttachment
     */
    BusAttachment* m_busAttachment;

    /**
     * pointer to AboutIcon
     */
    AboutIcon* m_icon;
};

}

#endif /* _ALLJOYN_ABOUTICONOBJ_H */
