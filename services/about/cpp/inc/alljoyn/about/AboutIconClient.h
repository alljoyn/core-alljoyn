/**
 * @file
 * Implementation of a ProxyBusObject used to interact with a org.alljoyn.Icon
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
#ifndef ABOUTICONCLIENT_H_
#define ABOUTICONCLIENT_H_

#include <alljoyn/BusAttachment.h>

namespace ajn {
namespace services {
/**
 * AboutIconClient enables the user of the class to interact with the remote AboutServiceIcon instance exposing the following methods:
 *  GetUrl
 *  GetContent
 *  GetVersion
 *  GetMimeType
 *  GetSize
 */
class AboutIconClient {
  public:

    /**
     * container to hold information about the Icon
     */
    class Icon {
      public:
        /**
         * Initialize an empty Icon
         */
        Icon() : content(NULL), contentSize(0), m_arg() { }

        /**
         * Add the IconContent from a MsgArg
         *
         * @param arg the MsgArg containing the Icon
         *
         * @return
         *   - ER_OK on success
         *   - status indicating failure otherwise
         */
        QStatus SetContent(const MsgArg& arg);

        /**
         * an array of bytes containing the image
         */
        uint8_t* content;
        /**
         * the number of bytes in the content array
         */
        size_t contentSize;
        /**
         * the mimetype of the image
         */
        qcc::String mimetype;
      private:
        /**
         * MsgArg containing the Icon image
         */
        MsgArg m_arg;
    };
    /**
     * Construct an AboutIconClient.
     * @param bus reference to BusAttachment
     */
    AboutIconClient(ajn::BusAttachment& bus);
    /**
     * Destruct AboutIconClient.
     */
    virtual ~AboutIconClient() {
    }
    /**
     *
     * @param[in] busName Unique or well-known name of AllJoyn bus
     * @param[in] url of the icon
     * @param[in] sessionId the session received  after joining AllJoyn session
     * @return ER_OK if successful
     */
    QStatus GetUrl(const char* busName, qcc::String& url, ajn::SessionId sessionId = 0);

    /**
     * @deprecated This function has been deprecated in favor of the @see GetIcon
     * member function.
     *
     * Use the GetIcon function to Get content of the icon.
     *
     * @warning the content must be freed using `delete [] content` once it is no
     *          longer needed
     *
     * @code
       size_t contentSize;
       uint8_t* content = NULL;
       status = iconClient->GetContent(busName, &content, contentSize, sessionId);
       ...
       delete [] content;
       @endcode
     *
     * @param[in] busName Unique or well-known name of AllJoyn bus
     * @param[out] content retrieves the icons payload
     * @param[out] contentSize is the size of the content payload
     * @param[in] sessionId the session received  after joining AllJoyn session
     * @return ER_OK if successful
     */
    QCC_DEPRECATED(QStatus GetContent(const char* busName, uint8_t * *content, size_t & contentSize, ajn::SessionId sessionId = 0));

    /**
     * @param[in] busName Unique or well-known name of AllJoyn bus
     * @param[out] icon class that holds icon content
     * @param[in] sessionId the session received  after joining AllJoyn session
     * @return ER_OK if successful
     */
    QStatus GetIcon(const char* busName, Icon& icon, ajn::SessionId sessionId = 0);
    /**
     *
     * @param[in] busName Unique or well-known name of AllJoyn bus
     * @param[out] version of the AboutIcontClient
     * @param[in] sessionId the session received  after joining AllJoyn session
     * @return ER_OK if successful
     */
    QStatus GetVersion(const char* busName, int& version, ajn::SessionId sessionId = 0);

    /**
     *
     * @param[in] busName Unique or well-known name of AllJoyn bus
     * @param[out] mimeType of the icon
     * @param[in] sessionId the session received  after joining AllJoyn session
     * @return ER_OK if successful
     */
    QStatus GetMimeType(const char* busName, qcc::String& mimeType, ajn::SessionId sessionId = 0);

    /**
     *
     * @param[in] busName Unique or well-known name of AllJoyn bus
     * @param[out] size of the icon
     * @param[in] sessionId the session received  after joining AllJoyn session
     * @return ER_OK if successful
     */
    QStatus GetSize(const char* busName, size_t& size, ajn::SessionId sessionId = 0);

  private:
    /**
     * pointer to BusAttachment
     */
    ajn::BusAttachment* m_BusAttachment;

};

}
}

#endif /* ABOUTICONCLIENT_H_ */
