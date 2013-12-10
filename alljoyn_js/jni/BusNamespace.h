/*
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
 */
#ifndef _BUSNAMESPACE_H
#define _BUSNAMESPACE_H

#include "BusAttachmentInterface.h"
#include "BusErrorInterface.h"
#include "CredentialsInterface.h"
#include "MessageInterface.h"
#include "Plugin.h"
#include "ScriptableObject.h"
#include "IfcSecurityInterface.h"
#include "SessionLostReasonInterface.h"
#include "SessionOptsInterface.h"
#include "SocketFdInterface.h"
#include "VersionInterface.h"
#include <map>
#include <qcc/ManagedObj.h>

class _BusNamespace : public ScriptableObject {
  public:
    static std::map<qcc::String, int32_t>& Constants();
    _BusNamespace(Plugin& plugin);
    virtual ~_BusNamespace();

  private:
    static std::map<qcc::String, int32_t> constants;

    BusAttachmentInterface busAttachmentInterface;
    BusErrorInterface busErrorInterface;
    CredentialsInterface credentialsInterface;
    IfcSecurityInterface ifcSecurityInterface;
    MessageInterface messageInterface;
    SessionLostReasonInterface sessionLostReasonInterface;
    SessionOptsInterface sessionOptsInterface;
    SocketFdInterface socketFdInterface;
    VersionInterface versionInterface;
    bool getBusAttachment(NPVariant* result);
    bool getBusError(NPVariant* result);
    bool getCredentials(NPVariant* result);
    bool getIfcSecurity(NPVariant* result);
    bool getMessage(NPVariant* result);
    bool getSessionOpts(NPVariant* result);
    bool getSessionLostReason(NPVariant* result);
    bool getSocketFd(NPVariant* result);
    bool getVersion(NPVariant* result);

    /*
     * Fallback for FeaturePermissions APIs.
     * (http://dev.w3.org/2009/dap/perms/FeaturePermissions.html).
     */
    bool getPrivilegedFeatures(NPVariant* result);
    bool permissionLevel(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool requestPermission(const NPVariant* args, uint32_t argCount, NPVariant* result);
};

typedef qcc::ManagedObj<_BusNamespace> BusNamespace;

#endif // _BUSNAMESPACE_H
