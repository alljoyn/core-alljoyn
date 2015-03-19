/**
 * @file
 * Implementation of a ProxyBusObject used to interact with a org.alljoyn.About
 * interface
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

#ifndef ABOUTCLIENT_H_
#define ABOUTCLIENT_H_

#include <map>
#include <vector>
#include <string>
#include <alljoyn/BusAttachment.h>

namespace ajn {
namespace services {

/**
 * AboutClient is a helper class used by an AllJoyn IoE client application to discover services
 * being offered by nearby AllJoyn IoE service applications.
 * AboutClient enables the user of the class to interact with the remote AboutService instance exposing the following methods:
 *  GetObjectDescriptions
 *  GetAboutData
 *  GetVersion
 *
 * @deprecated The AboutClient class has been deprecated please see the
 * AboutProxy class for similar functionality as the AboutClient class.
 */
class AboutClient : public ajn::MessageReceiver {
  public:
    /**
     *	map of AboutData using qcc::String as key and ajn::MsgArg as value.
     */
    typedef std::map<qcc::String, ajn::MsgArg> AboutData;

    /**
     * map of ObjectDescriptions using qcc::String as key std::vector<qcc::String>   as value, describing interfaces
     *
     */
    typedef std::map<qcc::String, std::vector<qcc::String> > ObjectDescriptions;

    /**
     * AboutClient Constructor
     *
     * @deprecated The AboutClient class has been deprecated please see the
     * AboutProxy class for similar functionality as the AboutClient class.
     *
     * @param  bus reference to BusAttachment
     */
    QCC_DEPRECATED(AboutClient(ajn::BusAttachment& bus));

    /**
     * Destruct AboutClient
     *
     * @deprecated The AboutClient class has been deprecated please see the
     * AboutProxy class for similar functionality as the AboutClient class.
     */
    QCC_DEPRECATED(virtual ~AboutClient());

    /**
     * Get the ObjectDescription array for specified bus name.
     *
     * @deprecated The AboutClient class and its member functions have been
     * deprecated please see the AboutProxy::GetObjectDescription.
     *
     * @param[in] busName Unique or well-known name of AllJoyn bus
     * @param[in,out] objectDescs  objectDescs  Description of busName's remote objects.
     * @param[in] sessionId the session received  after joining AllJoyn session
     * @return ER_OK if successful.
     */
    QCC_DEPRECATED(QStatus GetObjectDescriptions(const char* busName, ObjectDescriptions & objectDescs, ajn::SessionId sessionId = 0));

    /**
     * Get the AboutData  for specified bus name.
     *
     * @deprecated The AboutClient class and its member functions have been
     * deprecated please see the AboutProxy::GetAboutData function.
     *
     * @param[in] busName Unique or well-known name of AllJoyn bus
     * @param[in] languageTag is the language used to request the AboutData.
     * @param[in,out] data is reference of AboutData that is filled by the function
     * @param[in] sessionId the session received  after joining AllJoyn session
     * @return ER_OK if successful.
     */
    QCC_DEPRECATED(QStatus GetAboutData(const char* busName, const char* languageTag, AboutData & data, ajn::SessionId sessionId = 0));

    /**
     * GetVersion get the About version
     *
     * @deprecated The AboutClient class and its member functions have been
     * deprecated please see the AboutProxy::GetVersion function.
     *
     * @param[in] busName Unique or well-known name of AllJoyn bus
     * @param[in] version of the service.
     * @param[in] sessionId the session received  after joining AllJoyn session
     * @return
     */
    QCC_DEPRECATED(QStatus GetVersion(const char* busName, int& version, ajn::SessionId sessionId = 0));

  private:

    /**
     *	pointer to  BusAttachment
     */
    ajn::BusAttachment* m_BusAttachment;

};

}
}

#endif /* ABOUTCLIENT_H_ */
