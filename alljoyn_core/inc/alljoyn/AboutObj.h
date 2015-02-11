/**
 * @file
 * This contains the About class
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
#ifndef _ALLJOYN_ABOUT_H
#define _ALLJOYN_ABOUT_H

#include <alljoyn/AboutDataListener.h>
#include <alljoyn/AboutListener.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>

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
     * This will also register the About BusObject on the passed in BusAttachment
     *
     * The AboutObj class is responsible for transmitting information about the
     * interfaces that are available for other applications to use. It also
     * provides application specific information that is contained in the
     * AboutDataListener class
     *
     * It also provides means for applications to respond to certain requests
     * concerning the interfaces.
     *
     * By default the org.alljoyn.About interface is excluded from the list of
     * announced interfaces. Since simply receiving the announce signal tells
     * the client that the service implements the org.alljoyn.About interface.
     * There are some legacy applications that expect the org.alljoyn.About
     * interface to be part of the announcement. Changing the isAnnounced flag
     * from UNANNOUNCED, its default, to ANNOUNCED will cause the org.alljoyn.About
     * interface to be part of the announce signal. Unless your application is
     * talking with a legacy application that expects the org.alljoyn.About
     * interface to be part of the announce signal it is best to leave the
     * isAnnounced to use its default value.
     *
     * @param[in] bus the BusAttachment that will contain the about information
     * @param[in] isAboutIntfAnnounced will the org.alljoyn.About interface be
     *                                 part of the announced interfaces.
     */
    AboutObj(BusAttachment& bus, AnnounceFlag isAboutIntfAnnounced = UNANNOUNCED);

    virtual ~AboutObj();

    /**
     * This is used to send the Announce signal.  It announces the list of all
     * interfaces available at given object paths as well as the announced
     * fields from the AboutData.
     *
     * This method will automatically obtain the Announced ObjectDescription from the
     * BusAttachment that was used to create the AboutObj. Only BusObjects that have
     * marked their interfaces as announced and are registered with the
     * BusAttachment will be announced.
     *
     * @see BusAttachment::RegisterBusObject
     * @see BusObject::AddInterface
     *
     * @param sessionPort the session port the interfaces can be connected with
     * @param aboutData   the AboutDataListener that contains the AboutData for
     *                    this announce signal.
     *
     * @return
     *  - ER_OK on success
     *  - ER_ABOUT_SESSIONPORT_NOT_BOUND if the SessionPort given is not bound
     */
    QStatus Announce(SessionPort sessionPort, AboutDataListener& aboutData);

    /**
     * Cancel the last announce signal sent. If no signals have been sent this
     * method call will return.
     *
     * @return
     *     - ER_OK on success
     *     - another status indicating failure.
     */
    QStatus Unannounce();
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
     * check that the MsgArg returned from AboutDataListener.GetAboutData
     * contains all the required fields
     * Fields are:
     *  - AppId
     *  - DefaultLanguage
     *  - DeviceId
     *  - AppName
     *  - Manufacture
     *  - ModelNumber
     *  - SupportedLanguages
     *  - Description
     *  - SoftwareVersion
     *  - AJSoftwareVersion
     *
     *  @param aboutDataArg MsgArg containing the AboutData fields.
     *
     *  return true if it contains all the required fields
     */
    bool HasAllRequiredFields(MsgArg& aboutDataArg);

    /**
     *
     * check that the MsgArg returned from AboutDataListener.GetAnnouncedAboutData
     * contains all the required fields
     * Fields are:
     *  - AppId
     *  - DefaultLanguage
     *  - DeviceId
     *  - AppName
     *  - Manufacture
     *  - ModelNumber
     *
     *  @param announcedDataArg MsgArg containging the announced AboutData fields.
     *
     *  return true if it contains all the required fields
     */
    bool HasAllAnnouncedFields(MsgArg& announcedDataArg);

    /**
     * Check that the values in the Announced Fields and the Requried Fields
     * agree.
     *
     * @param aboutDataArg MsgArg containing the AboutData fields.
     * @param announcedDataArg MsgArg containging the announced AboutData fields.
     *
     * @return true if the announced AboutData and the AboutData fields match
     */
    bool AnnouncedDataAgreesWithAboutData(MsgArg& aboutDataArg, MsgArg& announcedDataArg);

    /**
     * Validate individual AboutData fields to make sure they meet requirements.
     *
     *  @param aboutDataArg MsgArg containing the AboutData fields.
     *
     * @return
     * - #ER_OK if all checked fields are good
     * - Error status indicating otherwise
     */
    QStatus ValidateAboutDataFields(MsgArg& aboutDataArg);

    /**
     *  pointer to BusAttachment
     */
    BusAttachment* m_busAttachment;

    MsgArg m_objectDescription;
    AboutDataListener* m_aboutDataListener;
    uint32_t m_announceSerialNumber;
};
}
#endif
