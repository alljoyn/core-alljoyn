/**
 * @file
 * Container class to hold the information about an icon for the About Icon
 * Object.
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
#ifndef _ALLJOYN_ABOUTICON_H
#define _ALLJOYN_ABOUTICON_H

#include <alljoyn/MsgArg.h>
#include <qcc/String.h>


namespace ajn {
/**
 * container to hold information about the Icon
 */
class AboutIcon {
  public:
    /**
     * Initialize an empty Icon
     */
    AboutIcon();

    /**
     * Set the content for an icon.  The content must fit into a MsgArg so the
     * largest size for the icon data is ALLJOYN_MAX_ARRAY_LEN bytes.
     *
     * Note as long as the MIME type matches it is possible to set both icon content
     * and icon URL
     *
     * If an error is returned the icon content will remain unchanged.
     *
     * @param[in] mimetype a MIME type indicating the icon image type. Typical
     *                     value will be `image/jpeg` or `image/png`
     * @param[in] data     pointer to an array of bytes that represent an icon
     * @param[in] csize    The number of bytes in data
     * @param[in] ownsData If true the AboutIcon contain gains ownership of the
     *                     data that data points to.  AboutIcon class will be
     *                     responsible for freeing the memory. If false user is
     *                     is responsible for freeing memory pointed to by data.
     * @return
     *  - #ER_OK on success
     *  - #ER_BUS_BAD_VALUE if the data is to large to be marshaled
     *  - other status indicating failure
     */
    QStatus SetContent(const char* mimetype, uint8_t* data, size_t csize, bool ownsData = false);

    /**
     * Set a url that contain the icon for the application.
     *
     * Note as long as the MIME type matches it is possible to set both icon content
     * and icon URL
     *
     * @param[in] mimetype a MIME type indicating the icon image type. Typical
     *                     value will be `image/jpeg` or `image/png`
     * @param[in] url      A URL that contain the location of the icon hosted in
     *                     the cloud.
     * @return
     *  - #ER_OK on success
     *  - other status indicating failure otherwise
     */
    QStatus SetUrl(const char* mimetype, const char* url);

    /**
     * Clear the AboutIcon.  This will change all strings to empty strings
     * set the content to NULL and the contentSize to zero.
     */
    void Clear();

    /**
     * Add the IconContent from a MsgArg. This will make a local copy of the
     * MsgArg passed in and expose the contents as the member variables content
     * and contentSize.
     *
     * @param arg the MsgArg containing the Icon
     *
     * @return
     *   - #ER_OK on success
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

    /**
     * The url for the Icon
     */
    qcc::String url;

  private:
    /**
     * MsgArg containing the Icon image
     */
    MsgArg m_arg;
};
}

#endif //_ALLJOYN_ABOUTICON_H
