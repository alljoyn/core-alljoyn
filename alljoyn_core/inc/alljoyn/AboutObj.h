/**
 * @file
 * This contains the About class
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
#ifndef _ALLJOYN_ABOUT_H
#define _ALLJOYN_ABOUT_H

#include <alljoyn/AboutData.h>
#include <alljoyn/AboutListener.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/AboutObjectDescription.h>

namespace ajn {
/**
 * An AllJoyn BusObject that implements the org.alljoyn.About interface.
 *
 * This BusObject is used to announce the capabilities and other identifying details
 * of the application or device.
 */
class AboutObj : public BusObject {
  public:
    /**
     * version of the org.alljoyn.About interface
     */
    static const uint16_t VERSION;

    /**
     * create a new About class
     *
     * The about class is responsible for transmitting information about the interfaces
     * that are avalible for other applications to use. It also provides application specific
     * information that is contained in the AboutData class
     *
     * It also provides mean for applications to respond to certain requests concerning the
     * interfaces.
     *
     * @param bus the BusAttachment that will contain the about information
     */
    AboutObj(BusAttachment& bus);

    virtual ~AboutObj() { }

    /**
     * This is used to send the Announce signal.  It announces the list of all
     * interfaces available at given object paths as well as the announced
     * fields from the AboutData.
     *
     * This method will automatically obtain the AboutObjectDescription from the
     * BusAttachment member function GetAboutObjectDescription. Only BusObjects that have
     * marked there interfaces as announced and are registered with the
     * BusAttachmentwill be announced.
     *
     * @see AboutObjectDescription
     * @see BusAttachment::GetAboutObjectDescription
     * @see BusAttachment::RegisterBusObject
     * @see BusObject::AddInterface
     *
     * @param sessionPort the session port the interfaces can be connected with
     *
     * @return ER_OK on success
     */
    QStatus Announce(SessionPort sessionPort, AboutData& aboutData);

    /**
     * This is used to send the Announce signal.  It announces the list of all
     * interfaces available at given object paths as well as the announced
     * fields from the AboutData.
     *
     * This method will automatically obtain the AboutObjectDescription from the
     * BusAttachment member function GetAboutObjectDescription. It will then merge
     * the interfaces provided in `objectDescription` with that AboutObjectDescription.
     * The resulting Announce signal will contain interfaces from RegisteredBusObjects
     * and interfaces specified in the user provided AboutObjectDescription.
     *
     * @see AboutObjectDescription::Merge
     * @see BusAttachment::GetAboutObjectDescription
     * @see BusAttachment::RegisterBusObject
     * @see BusObject::AddInterface
     *
     * @param sessionPort the session port the interfaces can be connected with
     *
     * @return ER_OK on success
     */
    QStatus Announce(SessionPort sessionPort, AboutObjectDescription& objectDescription, AboutData& aboutData);

  private:
    /**
     * Handles  GetAboutData method
     * @param[in]  member
     * @param[in]  msg reference of AllJoyn Message
     */
    void GetAboutData(const InterfaceDescription::Member* member, Message& msg);

    /**
     *  Handles  GetObjectDescription method
     * @param[in]  member
     * @param[in]  msg reference of AllJoyn Message
     */
    void GetObjectDescription(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Handles the GetPropery request
     * @param[in]  ifcName  interface name
     * @param[in]  propName the name of the properly
     * @param[in]  val reference of MsgArg out parameter.
     * @return ER_OK if successful.
     */
    QStatus Get(const char* ifcName, const char* propName, MsgArg& val);

    /**
     *  pointer to BusAttachment
     */
    BusAttachment* m_busAttachment;

    AboutObjectDescription m_objectDescription;
    AboutData m_aboutData;
};
}
#endif
