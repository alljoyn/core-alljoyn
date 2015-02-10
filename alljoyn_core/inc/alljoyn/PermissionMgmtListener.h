/**
 * @file
 * Contains the PermissionMgmtListener class
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
#ifndef _ALLJOYN_PERMISSIONMGMTLISTENER_H
#define _ALLJOYN_PERMISSIONMGMTLISTENER_H

#include <alljoyn/PermissionConfigurator.h>

namespace ajn {

class PermissionMgmtListener {
  public:
    /**
     * PermissionMgmtListener constructor
     */
    PermissionMgmtListener() { }

    /**
     * PermissionMgmtListener destructor
     */
    virtual ~PermissionMgmtListener() { }

    /**
     * Handler for the org.allseen.Security.PermissionMgmt.Notification
     * NotifyConfig sessionless signal.
     *
     * The publicKeyInfoArg contains an array with a signature of `a(yv)`
     * this is an array of public KeyInfo structures.
     * @param[in] busName          unique name of the remote BusAttachment that
     *                             sent the NotifyConfig signal
     * @param[in] version          version of the org.allseen.Security.PermissionMgmt.Notification
     *                             interface
     * @param[in] publicKeyArg     public key of the application
     * @param[in] claimableState   enum indicating the claimable state of the remote
     *                             remote device
     * @param[in] trustAnchorsArg  array of <trust anchor usage type, keyInfo> pairs
     * @param[in] serialNumber     the policy serial number
     * @param[in] membershipsArg   array of <guildID, serialNumber> pairs
     */
    virtual void NotifyConfig(const char* busName, uint16_t version, MsgArg publicKeyArg, PermissionConfigurator::ClaimableState claimableState, MsgArg trustAnchorsArg, uint32_t serialNumber, MsgArg membershipsArg) = 0;
};
}
#endif //_ALLJOYN_PERMISSIONMGMTLISTENER_H
