/**
 * @file
 * Implement a permission verification class
 */

/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/
#ifndef _ALLJOYN_PERMISSION_DB_H
#define _ALLJOYN_PERMISSION_DB_H

#include "BusEndpoint.h"
#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <set>
#include <map>
namespace ajn {

class PermissionDB {
  public:

    /**
     * Get the singleton instance of PermissionDB
     */
    static PermissionDB& GetDB();

    /**
     * Check whether the endpoint is allowed to use WIFI
     * @param uid    UserId to be checked
     * @return true if allowed
     */
    bool IsWifiAllowed(uint32_t uid);

    /**
     * Check whether the endpoint owns the required permissions
     * @param uid      The user id of the endpoint to be verified
     * @param permsReq The list of permissions to be verified
     * @return true if the endpoint passes the verification
     */
    bool VerifyPeerPermissions(const uint32_t uid, const std::set<qcc::String>& permsReq);

    /**
     * Remove the permission information cache of an enpoint before it exits.
     * @param endponit The endpoint that will exits
     * @return ER_OK if successful
     */
    QStatus RemovePermissionCache(BusEndpoint& endpoint);

    /**
     * Add an alias ID to a UnixEndpoint User ID
     * @param origUID   The unique User ID
     * @param aliasUID  The alias User ID
     * @return ER_OK if successfully
     */
    QStatus AddAliasUnixUser(uint32_t origUID, uint32_t aliasUID);

  private:
    /**
     * Check whether the uid owns the required permissions on Android
     * @param uid        The user Id of an Android app
     * @param permsReq   The required permissions
     * @return true if the uid meets the permission requirement
     */
    bool VerifyPermsOnAndroid(const uint32_t uid, const std::set<qcc::String>& permsReq);

    /**
     * Get the unique user ID of an alias user ID
     * @param userID      The user ID of the endpoint
     * @return the unique user ID if it exists otherwise keep unchanged
     */
    uint32_t UniqueUserID(uint32_t userID);

    qcc::Mutex permissionDbLock;
    std::map<uint32_t, std::set<qcc::String> > uidPermsMap;          /**< cache the permissions owned by an endpoint identified by user id */
    std::map<uint32_t, uint32_t> uidAliasMap;                        /**< map of alias user id to the unique user id. */
    std::set<uint32_t> unknownApps;                                  /**< apps whose permission info is unknown */
};

}
#endif