/**
 * @file
 * Contains the ApplicationStateListener class
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
#ifndef _ALLJOYN_APPLICATIONSTATELISTENER_H
#define _ALLJOYN_APPLICATIONSTATELISTENER_H

#include <alljoyn/PermissionConfigurator.h>

namespace ajn {

/**
 * Listener used to handle the security State signal.
 */
class ApplicationStateListener {
  public:
    /**
     * ApplicationStateListener constructor
     */
    ApplicationStateListener() { }

    /**
     * ApplicationStateListener destructor
     */
    virtual ~ApplicationStateListener() { }

    /**
     * Handler for the org.allseen.Bus.Application's State sessionless signal.
     *
     * @param[in] busName          unique name of the remote BusAttachment that
     *                             sent the State signal
     * @param[in] publicKeyInfo the application public key
     * @param[in] state the application state
     */
    virtual void State(const char* busName, const qcc::KeyInfoNISTP256& publicKeyInfo, PermissionConfigurator::ApplicationState state) = 0;
};
}
#endif //_ALLJOYN_APPLICATIONSTATELISTENER_H
