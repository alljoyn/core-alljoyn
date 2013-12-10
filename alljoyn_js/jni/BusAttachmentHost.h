/*
 * Copyright (c) 2011-2013, AllSeen Alliance. All rights reserved.
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
#ifndef _BUSATTACHMENTHOST_H
#define _BUSATTACHMENTHOST_H

#include "BusAttachment.h"
#include "ProxyBusObjectHost.h"
#include "ScriptableObject.h"
#include <qcc/ManagedObj.h>
#include <qcc/String.h>
class AuthListener;
class BusListener;
class BusObjectListener;
class InterfaceDescription;
class SessionListener;
class SessionPortListener;
class SignalReceiver;

class _BusAttachmentHost : public ScriptableObject {
    friend class JoinSessionAsyncCB;
    friend class SessionPortListener;
  public:
    _BusAttachmentHost(Plugin& plugin);
    virtual ~_BusAttachmentHost();

  private:
    BusAttachment* busAttachment;
    AuthListener* authListener;
    qcc::String applicationName;
    qcc::String connectSpec;
    std::list<SignalReceiver*> signalReceivers;
    std::list<BusListener*> busListeners;
    std::map<ajn::SessionPort, SessionPortListener*> sessionPortListeners;
    std::map<ajn::SessionId, SessionListener*> sessionListeners;
    std::map<qcc::String, BusObjectListener*> busObjectListeners;
    std::map<qcc::String, ProxyBusObjectHost> proxyBusObjects;

    bool getGlobalGUIDString(NPVariant* result);
    bool getUniqueName(NPVariant* result);

    bool addLogonEntry(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool addMatch(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool advertiseName(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool bindSessionPort(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool cancelAdvertiseName(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool cancelFindAdvertisedName(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool cancelFindAdvertisedNameByTransport(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool clearKeyStore(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool clearKeys(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool connect(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool create(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool createInterface(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool createInterfacesFromXML(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool destroy(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool disconnect(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool enablePeerSecurity(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool findAdvertisedName(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool findAdvertisedNameByTransport(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool getInterface(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool getInterfaces(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool getKeyExpiration(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool getPeerGUID(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool getPeerSecurityEnabled(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool getProxyBusObject(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool getTimestamp(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool joinSession(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool leaveSession(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool removeSessionMember(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool getSessionFd(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool nameHasOwner(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool registerBusListener(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool registerBusObject(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool registerSignalHandler(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool releaseName(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool reloadKeyStore(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool removeMatch(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool requestName(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool setDaemonDebug(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool setLinkTimeout(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool setKeyExpiration(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool setSessionListener(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool unbindSessionPort(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool unregisterBusListener(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool unregisterBusObject(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool unregisterSignalHandler(const NPVariant* args, uint32_t argCount, NPVariant* result);

    QStatus GetSignal(const qcc::String& signalName, const ajn::InterfaceDescription::Member*& signal);
    qcc::String MatchRule(const ajn::InterfaceDescription::Member* signal, const qcc::String& sourcePath);
    QStatus Connect(Plugin& plugin, const char* connectSpec);
    /**
     * Parse the ProxyBusObject name string into its components.
     *
     * @param[in] name a proxy bus object name of the form "<serviceName><objectPath><args>"
     * @param[out] serviceName a D-Bus bus name
     * @param[out] path a D-Bus object path
     * @param[out] argMap a map of args from the args component of name: ":<name>=<value>[,<name>=<value>]"
     */
    void ParseName(const qcc::String& name, qcc::String& serviceName, qcc::String& path, std::map<qcc::String, qcc::String>& argMap);
    void stopAndJoin();
};

typedef qcc::ManagedObj<_BusAttachmentHost> BusAttachmentHost;

#endif // _BUSATTACHMENTHOST_H
