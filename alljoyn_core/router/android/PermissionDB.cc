/**
 * @file
 * AllJoyn permission database class
 */

/******************************************************************************
 * Copyright (c) 2010-2011, 2014, AllSeen Alliance. All rights reserved.
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
#include <qcc/platform.h>

#include <set>
#include <map>

#include <qcc/Debug.h>
#include <qcc/Logger.h>
#include <qcc/XmlElement.h>
#include <qcc/Log.h>
#include <qcc/FileStream.h>

#include "PermissionDB.h"
#include "BusEndpoint.h"

#define QCC_MODULE "PERMISSION_MGR"

using namespace std;
using namespace qcc;

namespace ajn {

static PermissionDB permissionDB;

/**
 * Get the assigned permissions of the installed Android package with specific user id
 * @param uid          The user Id
 * @param permissions  Container to store the permission entries
 * @return true if permission information for the user id is found
 */
static bool GetPermsAssignedByAndroid(const uint32_t uid, std::set<qcc::String>& permissions);

/**
 * Get the assigned permissions of the installed Android package with specific shared user id
 * @param sharedUid    The shared user Id
 * @param permissions  Container to store the permission entries
 * @param root         The root element of a parsed xml file
 * @return true if permission information for the shared user id is found
 */
static bool GetPermsBySharedUserId(const char* sharedUid, std::set<qcc::String>& permissions, const XmlElement* root);

PermissionDB& PermissionDB::GetDB()
{
    return permissionDB;
}

bool PermissionDB::VerifyPermsOnAndroid(const uint32_t userId, const std::set<qcc::String>& permsReq)
{
    bool pass = true;
    /* Root user with id 0 can do anything.
     */
    if (userId == 0) {
        return true;
    }
    permissionDbLock.Lock(MUTEX_CONTEXT);
    if (unknownApps.find(userId) != unknownApps.end()) {
        permissionDbLock.Unlock(MUTEX_CONTEXT);
        return true;
    }
    std::map<uint32_t, std::set<qcc::String> >::const_iterator uidPermsIt = uidPermsMap.find(userId);
    std::set<qcc::String> permsOwned;

    if (uidPermsIt == uidPermsMap.end()) {
        /* If no permission info is found because of failure to read the "/data/system/packages.xml" file, then ignore the permission check */
        if (!GetPermsAssignedByAndroid(userId, permsOwned)) {
            unknownApps.insert(userId);
            permissionDbLock.Unlock(MUTEX_CONTEXT);
            return true;
        }
        uidPermsMap[userId] = permsOwned;
    } else {
        permsOwned = uidPermsIt->second;
    }

    std::set<qcc::String>::const_iterator permsIt;
    for (permsIt = permsReq.begin(); permsIt != permsReq.end(); permsIt++) {
        QCC_DbgHLPrintf(("PermissionDB::Check permission %s at user %d", (*permsIt).c_str(), userId));
        set<qcc::String>::iterator permsOwnedIt = permsOwned.find(*permsIt);
        if (permsOwnedIt == permsOwned.end()) {
            pass = false;
            QCC_DbgHLPrintf(("PermissionDB::Permission %s is not granted for user %d", (*permsIt).c_str(), userId));
            break;
        }
    }
    permissionDbLock.Unlock(MUTEX_CONTEXT);
    return pass;
}

bool PermissionDB::IsWifiAllowed(uint32_t uid)
{
    QCC_DbgTrace(("PermissionDB::IsWifiAllowed(uid = %u)", uid));
    bool allowed = true;

    // For Android app, permissions "android.permission.INTERNET" and "android.permission.CHANGE_WIFI_MULTICAST_STATE" are required for usage of wifi.
    uint32_t userId = UniqueUserID(uid);
    std::set<qcc::String> permsReq;
    permsReq.insert("android.permission.INTERNET");
    permsReq.insert("android.permission.CHANGE_WIFI_MULTICAST_STATE");
    allowed = VerifyPermsOnAndroid(userId, permsReq);
    return allowed;
}

bool PermissionDB::VerifyPeerPermissions(const uint32_t uid, const std::set<qcc::String>& permsReq) {
    bool pass = true;
    uint32_t userId = UniqueUserID(uid);
    pass = VerifyPermsOnAndroid(userId, permsReq);
    return pass;
}

QStatus PermissionDB::AddAliasUnixUser(uint32_t origUID, uint32_t aliasUID)
{
    QCC_DbgTrace(("PermissionDB::AddAliasUnixUser(origUID = %d -> aliasUID = %d)", origUID, aliasUID));
    QStatus status = ER_OK;
    /* It is not allowed to use user id 0 (root user) as alias */
    if (aliasUID == 0) {
        status = ER_FAIL;
    }

    /* If the same, then do nothing*/
    if (status == ER_OK && UniqueUserID(aliasUID) != origUID) {
        permissionDbLock.Lock(MUTEX_CONTEXT);
        uidPermsMap.erase(UniqueUserID(aliasUID));
        uidAliasMap[aliasUID] = origUID;
        permissionDbLock.Unlock(MUTEX_CONTEXT);
    }
    return status;
}

uint32_t PermissionDB::UniqueUserID(uint32_t userID)
{
    uint32_t uid = userID;
    permissionDbLock.Lock(MUTEX_CONTEXT);
    std::map<uint32_t, uint32_t>::const_iterator it = uidAliasMap.find(userID);
    if (it != uidAliasMap.end()) {
        uid = uidAliasMap[userID];
    }
    permissionDbLock.Unlock(MUTEX_CONTEXT);
    return uid;
}

QStatus PermissionDB::RemovePermissionCache(BusEndpoint& endpoint)
{
    QCC_DbgTrace(("PermissionDB::RemovePermissionCache(endpoint = %s)", endpoint->GetUniqueName().c_str()));
    permissionDbLock.Lock(MUTEX_CONTEXT);
    uint32_t userId = endpoint->GetUserId();
    uidAliasMap.erase(userId);
    uidPermsMap.erase(UniqueUserID(userId));
    unknownApps.erase(UniqueUserID(userId));
    permissionDbLock.Unlock(MUTEX_CONTEXT);
    return ER_OK;
}

static bool GetPermsAssignedByAndroid(uint32_t uid, std::set<qcc::String>& permissions)
{
    QCC_DbgTrace(("PermissionDB::GetPermsAssignedByAndroid(uid = %d)", uid));
    const char* xml = "/data/system/packages.xml"; // The file contains information about all installed Android packages including Permissions

    const uint32_t MAX_ID_SIZE = 32;
    char userId [MAX_ID_SIZE];
    snprintf(userId, MAX_ID_SIZE, "%d", uid);
    FileSource source(xml);
    if (!source.IsValid()) {
        QCC_LogError(ER_FAIL, ("Failed to open %s", xml));
        return false;
    }

    XmlParseContext xmlParseCtx(source);
    bool success = XmlElement::Parse(xmlParseCtx) == ER_OK;
    bool found  = false;
    bool isSharedUserId = false;
    const XmlElement* root = xmlParseCtx.GetRoot();

    if (success) {
        if (root->GetName().compare("packages") == 0) {
            QCC_DbgPrintf(("Xml Tag %s", "packages"));
            const vector<XmlElement*>& elements = root->GetChildren();
            vector<XmlElement*>::const_iterator it;

            bool isSystemApp = false;
            for (it = elements.begin(); it != elements.end() && !found; ++it) {
                if ((*it)->GetName().compare("package") == 0) {
                    QCC_DbgPrintf(("Xml Tag %s", "package"));
                    const map<qcc::String, qcc::String>& attrs((*it)->GetAttributes());
                    map<qcc::String, qcc::String>::const_iterator attrIt;

                    isSystemApp = false;
                    for (attrIt = attrs.begin(); attrIt != attrs.end(); ++attrIt) {
                        if (attrIt->first.compare("codePath") == 0) {
                            uint32_t cmpLen = attrIt->second.size() < strlen("/system/app") ? attrIt->second.size() : strlen("/system/app");
                            if (attrIt->second.compare(0, cmpLen, "/system/app") == 0) {
                                QCC_DbgPrintf(("Xml Tag %s = %s", "codePath", attrIt->second.c_str()));
                                isSystemApp = true;
                            }
                        } else if (attrIt->first.compare("userId") == 0) {
                            QCC_DbgPrintf(("Xml Tag %s = %s", "userId", attrIt->second.c_str()));
                            if (attrIt->second.compare(userId) == 0) {
                                QCC_DbgHLPrintf(("PermissionDB::GetPermsAssignedByAndroid() entry for userId %d is found", uid));
                                found = true;
                                break;
                            }
                        } else if (attrIt->first.compare("sharedUserId") == 0) {
                            if (attrIt->second.compare(userId) == 0) {
                                QCC_DbgHLPrintf(("PermissionDB::GetPermsAssignedByAndroid() entry for userId %d is found", uid));
                                found = true;
                                isSharedUserId = true;
                                break;
                            }
                        }
                    }
                    if (found) {
                        /* If this is a pre-installed system app, then we trust it without checking the permissions becasue the permissions for system apps is not listed in packages.xml */
                        if (isSystemApp) {
                            return false;
                        }
                        /* If this is a shared user Id, then we should read the permissions from <shared-user> element*/
                        if (isSharedUserId) {
                            found = GetPermsBySharedUserId(userId, permissions, root);
                            break;
                        }
                        const vector<XmlElement*>& childElements = (*it)->GetChildren();
                        for (vector<XmlElement*>::const_iterator childIt = childElements.begin(); childIt != childElements.end(); ++childIt) {
                            if ((*childIt)->GetName().compare("perms") == 0) {
                                QCC_DbgPrintf(("Xml Tag %s", "perms"));
                                const vector<XmlElement*>& permElements = (*childIt)->GetChildren();
                                for (vector<XmlElement*>::const_iterator permIt = permElements.begin(); permIt != permElements.end(); ++permIt) {
                                    if ((*permIt)->GetName().compare("item") == 0) {
                                        QCC_DbgPrintf(("Xml Tag %s", "item"));
                                        const map<qcc::String, qcc::String>& itemAttrs((*permIt)->GetAttributes());
                                        map<qcc::String, qcc::String>::const_iterator itemAttrIt;
                                        for (itemAttrIt = itemAttrs.begin(); itemAttrIt != itemAttrs.end(); ++itemAttrIt) {
                                            if (itemAttrIt->first.compare("name") == 0) {
                                                QCC_DbgPrintf(("Xml Tag %s = (%s)", "name", itemAttrIt->second.c_str()));
                                                permissions.insert(itemAttrIt->second);
                                            }
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    if (!found) {
        QCC_LogError(ER_ALLJOYN_ACCESS_PERMISSION_WARNING, ("Cannot find permission info for userId %d in File %s", uid, xml));
    }
    return found;
}

static bool GetPermsBySharedUserId(const char* sharedUid, std::set<qcc::String>& permissions, const XmlElement* root)
{
    QCC_DbgTrace(("PermissionDB::GetPermsBySharedUserId(sharedUid = %s)", sharedUid));
    bool found = false;
    const vector<XmlElement*>& elements = root->GetChildren();
    vector<XmlElement*>::const_iterator it;
    for (it = elements.begin(); it != elements.end() && !found; ++it) {
        if ((*it)->GetName().compare("shared-user") == 0) {
            QCC_DbgPrintf(("Xml Tag %s", "shared-user"));
            const map<qcc::String, qcc::String>& attrs((*it)->GetAttributes());

            map<qcc::String, qcc::String>::const_iterator attrIt;
            for (attrIt = attrs.begin(); attrIt != attrs.end(); ++attrIt) {
                if (attrIt->first.compare("userId") == 0) {
                    QCC_DbgPrintf(("Xml Tag %s = %s", "userId", attrIt->second.c_str()));
                    if (attrIt->second.compare(sharedUid) == 0) {
                        QCC_DbgHLPrintf(("PermissionDB::GetPermsBySharedUserId() entry for sharedUid %s is found", sharedUid));
                        found = true;
                    }
                }
            }
        }
        if (found) {
            const vector<XmlElement*>& childElements = (*it)->GetChildren();
            for (vector<XmlElement*>::const_iterator childIt = childElements.begin(); childIt != childElements.end(); ++childIt) {
                if ((*childIt)->GetName().compare("perms") == 0) {
                    QCC_DbgPrintf(("Xml Tag %s", "perms"));
                    const vector<XmlElement*>& permElements = (*childIt)->GetChildren();
                    for (vector<XmlElement*>::const_iterator permIt = permElements.begin(); permIt != permElements.end(); ++permIt) {
                        if ((*permIt)->GetName().compare("item") == 0) {
                            QCC_DbgPrintf(("Xml Tag %s", "item"));
                            const map<qcc::String, qcc::String>& itemAttrs((*permIt)->GetAttributes());
                            map<qcc::String, qcc::String>::const_iterator itemAttrIt;
                            for (itemAttrIt = itemAttrs.begin(); itemAttrIt != itemAttrs.end(); ++itemAttrIt) {
                                if (itemAttrIt->first.compare("name") == 0) {
                                    QCC_DbgPrintf(("Xml Tag %s = (%s)", "name", itemAttrIt->second.c_str()));
                                    permissions.insert(itemAttrIt->second);
                                }
                            }
                        }
                    }
                    break;
                }
            }
        }
    }
    return found;
}

} // namespace ajn
