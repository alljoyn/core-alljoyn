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

package org.alljoyn.bus;

import java.util.Map;


public interface AboutDataListener {
    /**
     * Get the Dictionary that is returned when a user calls
     * org.alljoyn.About.GetAboutData. The returned Dictionary must contain the
     * AboutData dictionary for the Language specified.
     *
     * TODO add more documentation for the Key/Value pair requirements here.
     *
     * @param language IETF language tags specified by RFC 5646 if the string
     *                 is null or an empty string the MsgArg for the default
     *                 language will be returned
     *
     * @return ER_OK on successful
     */
    public Map<String, Variant> getAboutData(String language) throws ErrorReplyBusException;

    /**
     * Return a Dictionary containing the AboutData that
     * is announced with the org.alljoyn.About.announce signal.
     * The Dictionary will always be the default language and will only contain
     * the fields that are announced.
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
     * Or the GetAboutData member function.
     *
     * @return a dictionary that contains the Announce data.
     */
    public Map<String, Variant> getAnnouncedAboutData() throws ErrorReplyBusException;
}
