/**
 * @file
 * This contains the AboutListener class
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
#ifndef _ALLJOYN_ABOUTLISTENER_H
#define _ALLJOYN_ABOUTLISTENER_H

#include <alljoyn/Session.h>
#include <alljoyn/AboutData.h>
#include <alljoyn/AboutObjectDescription.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/Message.h>
#include <alljoyn/MessageReceiver.h>

namespace ajn {

/**
 * Abstract base class implemented by AllJoyn users and called by AllJoyn to inform
 * users of About interface related events.
 */
class AboutListener {
  public:

    /**
     * AboutListener constructor
     */
    AboutListener() { }

    /**
     * AboutListener destructor
     */
    virtual ~AboutListener() { }

    /**
     * handler for the org.alljoyn.About.Anounce sessionless signal
     *
     * @param[in] busName            well know name of the service
     * @param[in] version            version of the AboutService
     * @param[in] port               SessionPort used by the AboutService
     * @param[in] objectDescription  map of ObjectDescriptions using qcc::String as key std::vector<qcc::String>   as value, describing interfaces
     * @param[in] aboutData          AboutData
     */
    virtual void Announced(const char* busName, uint16_t version, SessionPort port, AboutObjectDescription& objectDescription, AboutData& aboutData) = 0;
};
}

#endif //_ALLJOYN_ABOUTLISTENER_H
