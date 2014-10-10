/*
 * @file
 * Pure Virtual class that contains the interface needed by anyone one wanting
 * to use
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

#ifndef _ALLJOYN_ABOUTDATALISTENER_H
#define _ALLJOYN_ABOUTDATALISTENER_H

#include <alljoyn/MsgArg.h>
#include <alljoyn/Status.h>

namespace ajn {
class AboutDataListener {
  public:
    /**
     * Constructor
     */
    AboutDataListener() { };

    /**
     * Destructor
     */
    virtual ~AboutDataListener() { };

    /**
     * Creating the MsgArg that is returned when a user calls
     * org.alljoyn.About.GetAboutData. The returned MsgArg must contain the
     * AboutData dictionary for the Language specified.
     *
     * The MsgArg will contain the signature `a{sv}`.
     *
     * TODO add more documentation for the Key/Value pair requirements here.
     *
     * @param[out] msgArg a the dictionary containing all of the AboutData fields for
     *                    the specified language.  If language is not specified the default
     *                    language will be returned
     * @param[in] language IETF language tags specified by RFC 5646 if the string
     *                     is NULL or an empty string the MsgArg for the default
     *                     language will be returned
     *
     * @return ER_OK on successful
     */
    virtual QStatus GetAboutData(MsgArg* msgArg, const char* language) = 0;

    /**
     * Return a MsgArg pointer containing dictionary containing the AboutData that
     * is announced with the org.alljoyn.About.announce signal.
     * This will always be the default language and will only contain the fields
     * that are announced.
     *
     * The fields required to be part of the announced MsgArg are:
     *  - AppId
     *  - DefaultLanguage
     *  - DeviceName
     *  - DeviceId
     *  - AppName
     *  - Manufacture
     *  - ModelNumber
     *
     * If you require other fields or need the localized AboutData
     *   The org.alljoyn.About.GetAboutData method can be used.
     *
     * Or the GetMsgArg member function.
     *
     * @param[out] msgArg a MsgArg dictionary with the a{sv} that contains the Announce
     *                    data.
     * @return ER_OK if successful
     */
    virtual QStatus GetAnnouncedAboutData(MsgArg* msgArg) = 0;
};
}
#endif /* _ALLJOYN_ABOUTDATALISTENER_H */
