#ifndef _ALLJOYN_PERMISSION_MANAGER_H
#define _ALLJOYN_PERMISSION_MANAGER_H
/**
 * @file
 * This file defines the hook to validate whether a message is authorized by the Permission DB.
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

#ifndef __cplusplus
#error Only include PermissionManager.h in C++ code.
#endif

#include <alljoyn/PermissionPolicy.h>
#include "PermissionMgmtObj.h"

namespace ajn {

class PermissionManager {

  public:

    /**
     * Constructor
     *
     */
    PermissionManager() : policy(NULL), permissionMgmtObj(NULL)
    {
    }

    /**
     * virtual destructor
     */
    virtual ~PermissionManager()
    {
        delete policy;
    }

    /**
     * Set the permission policy.
     * @param   policy   the permission policy.  It must be new'd by the caller and and will be deleted by this object.
     */

    void SetPolicy(PermissionPolicy* policy)
    {
        delete this->policy;
        this->policy = policy;
    }

    /**
     * Retrieve the permission policy.
     * @return the permission policy.
     */
    const PermissionPolicy* GetPolicy() const
    {
        return policy;
    }

    /**
     * Authorize a message.  Make sure there is a proper permission is setup for this type of message.
     * @param outgoing indicating whether is a outgoing or incoming
     * @param msg the target message
     * @param peerState the peer's PeerState object
     * @return
     *  - ER_OK: authorized
     *  - ER_PERMISSION_DENIED: permission denied
     */
    QStatus AuthorizeMessage(bool outgoing, Message& msg, PeerState& peerState);

    /**
     * Authorize a Get Property message.  Make sure there is a proper permission is setup for this type of message.
     * @param objPath the object path
     * @param ifcName the interface name
     * @param propName the property name
     * @param peerState the peer's PeerState object
     * @return
     *  - ER_OK: authorized
     *  - ER_PERMISSION_DENIED: permission denied
     */
    QStatus AuthorizeGetProperty(const char* objPath, const char* ifcName, const char* propName, PeerState& peerState);

    void SetPermissionMgmtObj(PermissionMgmtObj* permissionMgmtObj)
    {
        this->permissionMgmtObj = permissionMgmtObj;
    }

    PermissionMgmtObj* GetPermissionMgmtObj()
    {
        return permissionMgmtObj;
    }

  private:
    /* Private assigment operator to prevent double freeing of memory */
    PermissionManager& operator=(const PermissionManager& src);
    /* Private copy constructor to prevent double freeing of memory */
    PermissionManager(const PermissionManager& src);

    bool AuthorizePermissionMgmt(bool outgoing, const char* iName, const char* mbrName, bool& authorized);

    PermissionPolicy* policy;
    PermissionMgmtObj* permissionMgmtObj;
};

}
#endif
