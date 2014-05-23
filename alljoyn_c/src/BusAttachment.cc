/**
 * @file
 * BusAttachment is the top-level object responsible for connecting to and optionally managing a message bus.
 */

/******************************************************************************
 * Copyright (c) 2009-2014, AllSeen Alliance. All rights reserved.
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
#include <assert.h>

#include <alljoyn_c/BusAttachment.h>
#include <alljoyn/InterfaceDescription.h>
#include "BusAttachmentC.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

struct _alljoyn_busattachment_handle {
    /* Empty by design, this is just to allow the type restrictions to save coders from themselves */
};

alljoyn_busattachment alljoyn_busattachment_create(const char* applicationName, QCC_BOOL allowRemoteMessages)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    bool allowRemoteMessagesBool = (allowRemoteMessages == QCC_TRUE ? true : false);
    return ((alljoyn_busattachment) new ajn::BusAttachmentC(applicationName, allowRemoteMessagesBool));
}

alljoyn_busattachment alljoyn_busattachment_create_concurrency(const char* applicationName, QCC_BOOL allowRemoteMessages, uint32_t concurrency)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    bool allowRemoteMessagesBool = (allowRemoteMessages == QCC_TRUE ? true : false);
    return ((alljoyn_busattachment) new ajn::BusAttachmentC(applicationName, allowRemoteMessagesBool, concurrency));
}


void alljoyn_busattachment_destroy(alljoyn_busattachment bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    assert(bus != NULL && "NULL parameter passed to alljoyn_destroy_busattachment.");

    delete (ajn::BusAttachmentC*)bus;
}

QStatus alljoyn_busattachment_start(alljoyn_busattachment bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->Start();
}

QStatus alljoyn_busattachment_stop(alljoyn_busattachment bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->Stop();
}

QStatus alljoyn_busattachment_join(alljoyn_busattachment bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->Join();
}

uint32_t alljoyn_busattachment_getconcurrency(alljoyn_busattachment bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->GetConcurrency();
}

const char* alljoyn_busattachment_getconnectspec(alljoyn_busattachment bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->GetConnectSpec().c_str();
}

void alljoyn_busattachment_enableconcurrentcallbacks(alljoyn_busattachment bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->EnableConcurrentCallbacks();
}

QStatus alljoyn_busattachment_createinterface(alljoyn_busattachment bus,
                                              const char* name,
                                              alljoyn_interfacedescription* iface)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ajn::InterfaceDescription* ifaceObj = NULL;
    QStatus ret = ((ajn::BusAttachmentC*)bus)->CreateInterface(name, ifaceObj);
    *iface = (alljoyn_interfacedescription)ifaceObj;

    return ret;
}

QStatus alljoyn_busattachment_createinterface_secure(alljoyn_busattachment bus,
                                                     const char* name,
                                                     alljoyn_interfacedescription* iface,
                                                     alljoyn_interfacedescription_securitypolicy secPolicy)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ajn::InterfaceDescription* ifaceObj = NULL;
    QStatus ret = ((ajn::BusAttachmentC*)bus)->CreateInterface(name, ifaceObj, static_cast<ajn::InterfaceSecurityPolicy>(secPolicy));
    *iface = (alljoyn_interfacedescription)ifaceObj;

    return ret;
}

QStatus alljoyn_busattachment_connect(alljoyn_busattachment bus, const char* connectSpec)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (connectSpec == NULL) {
        return ((ajn::BusAttachmentC*)bus)->Connect();
    }
    // Because the second parameter to Connect is only used internally it is not exposed to the C interface.
    return ((ajn::BusAttachmentC*)bus)->Connect(connectSpec);
}

void alljoyn_busattachment_registerbuslistener(alljoyn_busattachment bus, alljoyn_buslistener listener)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ((ajn::BusAttachmentC*)bus)->RegisterBusListener((*(ajn::BusListener*)listener));
}

void alljoyn_busattachment_unregisterbuslistener(alljoyn_busattachment bus, alljoyn_buslistener listener)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ((ajn::BusAttachmentC*)bus)->UnregisterBusListener((*(ajn::BusListener*)listener));
}

QStatus alljoyn_busattachment_findadvertisedname(alljoyn_busattachment bus, const char* namePrefix)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->FindAdvertisedName(namePrefix);
}

QStatus alljoyn_busattachment_findadvertisednamebytransport(alljoyn_busattachment bus, const char* namePrefix, alljoyn_transportmask transports)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->FindAdvertisedNameByTransport(namePrefix, transports);
}

QStatus alljoyn_busattachment_cancelfindadvertisedname(alljoyn_busattachment bus, const char* namePrefix)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->CancelFindAdvertisedName(namePrefix);
}

QStatus alljoyn_busattachment_cancelfindadvertisednamebytransport(alljoyn_busattachment bus, const char* namePrefix, alljoyn_transportmask transports)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->CancelFindAdvertisedNameByTransport(namePrefix, transports);
}

const alljoyn_interfacedescription alljoyn_busattachment_getinterface(alljoyn_busattachment bus, const char* name)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_interfacedescription)((ajn::BusAttachmentC*)bus)->GetInterface(name);
}

QStatus alljoyn_busattachment_joinsession(alljoyn_busattachment bus, const char* sessionHost,
                                          alljoyn_sessionport sessionPort, alljoyn_sessionlistener listener,
                                          alljoyn_sessionid* sessionId, alljoyn_sessionopts opts)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->JoinSession(sessionHost, (ajn::SessionPort)sessionPort,
                                                    (ajn::SessionListener*)listener, *((ajn::SessionId*)sessionId),
                                                    *((ajn::SessionOpts*)opts));
}

QStatus alljoyn_busattachment_joinsessionasync(alljoyn_busattachment bus,
                                               const char* sessionHost,
                                               alljoyn_sessionport sessionPort,
                                               alljoyn_sessionlistener listener,
                                               const alljoyn_sessionopts opts,
                                               alljoyn_busattachment_joinsessioncb_ptr callback,
                                               void* context)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    /*
     * the instance of the ajn::JoinsessionCallbackContext is freed when the
     * JoinSessionCB is called.
     */
    return ((ajn::BusAttachmentC*)bus)->JoinSessionAsync(sessionHost, (ajn::SessionPort)sessionPort,
                                                         (ajn::SessionListener*)listener,
                                                         *((ajn::SessionOpts*)opts),
                                                         (ajn::BusAttachmentC*)bus,
                                                         (void*)new ajn::JoinsessionCallbackContext(callback, context));
}

QStatus alljoyn_busattachment_registerbusobject(alljoyn_busattachment bus, alljoyn_busobject obj)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->RegisterBusObject(*((ajn::BusObject*)obj));
}

QStatus alljoyn_busattachment_registerbusobject_secure(alljoyn_busattachment bus, alljoyn_busobject obj)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->RegisterBusObject(*((ajn::BusObject*)obj), true);
}

void alljoyn_busattachment_unregisterbusobject(alljoyn_busattachment bus, alljoyn_busobject object)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ((ajn::BusAttachmentC*)bus)->UnregisterBusObject(*((ajn::BusObject*)object));
}

QStatus alljoyn_busattachment_requestname(alljoyn_busattachment bus, const char* requestedName, uint32_t flags)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->RequestName(requestedName, flags);
}

QStatus alljoyn_busattachment_bindsessionport(alljoyn_busattachment bus, alljoyn_sessionport* sessionPort,
                                              const alljoyn_sessionopts opts, alljoyn_sessionportlistener listener)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->BindSessionPort(*((ajn::SessionPort*)sessionPort),
                                                        *((const ajn::SessionOpts*)opts),
                                                        *((ajn::SessionPortListener*)listener));
}

QStatus alljoyn_busattachment_unbindsessionport(alljoyn_busattachment bus, alljoyn_sessionport sessionPort)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->UnbindSessionPort(sessionPort);
}

QStatus alljoyn_busattachment_advertisename(alljoyn_busattachment bus, const char* name, alljoyn_transportmask transports)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->AdvertiseName(name, transports);
}

QStatus alljoyn_busattachment_canceladvertisename(alljoyn_busattachment bus, const char* name, alljoyn_transportmask transports)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->CancelAdvertiseName(name, transports);
}

QStatus alljoyn_busattachment_enablepeersecurity(alljoyn_busattachment bus, const char* authMechanisms,
                                                 alljoyn_authlistener listener, const char* keyStoreFileName,
                                                 QCC_BOOL isShared)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->EnablePeerSecurity(authMechanisms, (ajn::AuthListener*)listener, keyStoreFileName,
                                                           (isShared == QCC_TRUE ? true : false));
}

QCC_BOOL alljoyn_busattachment_ispeersecurityenabled(alljoyn_busattachment bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (((ajn::BusAttachmentC*)bus)->IsPeerSecurityEnabled() == true ? QCC_TRUE : QCC_FALSE);
}

QStatus alljoyn_busattachment_createinterfacesfromxml(alljoyn_busattachment bus, const char* xml)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->CreateInterfacesFromXml(xml);
}

size_t alljoyn_busattachment_getinterfaces(const alljoyn_busattachment bus,
                                           const alljoyn_interfacedescription* ifaces, size_t numIfaces)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->GetInterfaces((const ajn::InterfaceDescription**)ifaces, numIfaces);
}

QStatus alljoyn_busattachment_deleteinterface(alljoyn_busattachment bus, alljoyn_interfacedescription iface)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->DeleteInterface(*((ajn::InterfaceDescription*)iface));
}

QCC_BOOL alljoyn_busattachment_isstarted(alljoyn_busattachment bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (((ajn::BusAttachmentC*)bus)->IsStarted() == true ? QCC_TRUE : QCC_FALSE);
}

QCC_BOOL alljoyn_busattachment_isstopping(alljoyn_busattachment bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (((ajn::BusAttachmentC*)bus)->IsStopping() == true ? QCC_TRUE : QCC_FALSE);
}

QCC_BOOL alljoyn_busattachment_isconnected(const alljoyn_busattachment bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (((const ajn::BusAttachmentC*)bus)->IsConnected() == true ? QCC_TRUE : QCC_FALSE);
}

QStatus alljoyn_busattachment_disconnect(alljoyn_busattachment bus, const char* connectSpec)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (connectSpec == NULL) {
        return ((ajn::BusAttachmentC*)bus)->Disconnect(((ajn::BusAttachmentC*)bus)->GetConnectSpec().c_str());
    }
    return ((ajn::BusAttachmentC*)bus)->Disconnect(connectSpec);
}

const alljoyn_proxybusobject alljoyn_busattachment_getdbusproxyobj(alljoyn_busattachment bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (const alljoyn_proxybusobject)(&((ajn::BusAttachmentC*)bus)->GetDBusProxyObj());
}

const alljoyn_proxybusobject alljoyn_busattachment_getalljoynproxyobj(alljoyn_busattachment bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (const alljoyn_proxybusobject)(&((ajn::BusAttachmentC*)bus)->GetAllJoynProxyObj());
}

const alljoyn_proxybusobject alljoyn_busattachment_getalljoyndebugobj(alljoyn_busattachment bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (const alljoyn_proxybusobject)(&((ajn::BusAttachmentC*)bus)->GetAllJoynDebugObj());
}

const char* alljoyn_busattachment_getuniquename(const alljoyn_busattachment bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((const ajn::BusAttachmentC*)bus)->GetUniqueName().c_str();
}

const char* alljoyn_busattachment_getglobalguidstring(const alljoyn_busattachment bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((const ajn::BusAttachmentC*)bus)->GetGlobalGUIDString().c_str();
}

QStatus alljoyn_busattachment_registerkeystorelistener(alljoyn_busattachment bus, alljoyn_keystorelistener listener)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->RegisterKeyStoreListener(*((ajn::KeyStoreListener*)listener));
}

QStatus alljoyn_busattachment_reloadkeystore(alljoyn_busattachment bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->ReloadKeyStore();
}

void alljoyn_busattachment_clearkeystore(alljoyn_busattachment bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ((ajn::BusAttachmentC*)bus)->ClearKeyStore();
}

QStatus alljoyn_busattachment_clearkeys(alljoyn_busattachment bus, const char* guid)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->ClearKeys(guid);
}

QStatus alljoyn_busattachment_setkeyexpiration(alljoyn_busattachment bus, const char* guid, uint32_t timeout)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->SetKeyExpiration(guid, timeout);
}

QStatus alljoyn_busattachment_getkeyexpiration(alljoyn_busattachment bus, const char* guid, uint32_t* timeout)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->GetKeyExpiration(guid, *timeout);
}

QStatus alljoyn_busattachment_addlogonentry(alljoyn_busattachment bus, const char* authMechanism,
                                            const char* userName, const char* password)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->AddLogonEntry(authMechanism, userName, password);
}

QStatus alljoyn_busattachment_releasename(alljoyn_busattachment bus, const char* name)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->ReleaseName(name);
}

QStatus alljoyn_busattachment_addmatch(alljoyn_busattachment bus, const char* rule)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->AddMatch(rule);
}

QStatus alljoyn_busattachment_removematch(alljoyn_busattachment bus, const char* rule)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->RemoveMatch(rule);
}

QStatus alljoyn_busattachment_setsessionlistener(alljoyn_busattachment bus, alljoyn_sessionid sessionId,
                                                 alljoyn_sessionlistener listener)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->SetSessionListener(sessionId, (ajn::SessionListener*)listener);
}

QStatus alljoyn_busattachment_leavesession(alljoyn_busattachment bus, alljoyn_sessionid sessionId)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->LeaveSession(sessionId);
}

QStatus alljoyn_busattachment_removesessionmember(alljoyn_busattachment bus, alljoyn_sessionid sessionId, const char* memberName)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->RemoveSessionMember(sessionId, memberName);
}

QStatus alljoyn_busattachment_setlinktimeout(alljoyn_busattachment bus, alljoyn_sessionid sessionid, uint32_t* linkTimeout)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->SetLinkTimeout(sessionid, *linkTimeout);
}

QStatus alljoyn_busattachment_setlinktimeoutasync(alljoyn_busattachment bus,
                                                  alljoyn_sessionid sessionid,
                                                  uint32_t linkTimeout,
                                                  alljoyn_busattachment_setlinktimeoutcb_ptr callback,
                                                  void* context)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    /*
     * the instance of the ajn::SetLinkTimeoutContext is freed when the
     * SetLinkTimeoutCB is called.
     */
    return ((ajn::BusAttachmentC*)bus)->SetLinkTimeoutAsync(sessionid, linkTimeout,
                                                            (ajn::BusAttachmentC*)bus,
                                                            (void*)new ajn::SetLinkTimeoutContext(callback, context));
}

QStatus alljoyn_busattachment_namehasowner(alljoyn_busattachment bus, const char* name, QCC_BOOL* hasOwner)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    bool result;
    QStatus ret = ((ajn::BusAttachmentC*)bus)->NameHasOwner(name, result);
    *hasOwner = (result == true ? QCC_TRUE : QCC_FALSE);
    return ret;
}

QStatus alljoyn_busattachment_getpeerguid(alljoyn_busattachment bus, const char* name, char* guid, size_t* guidSz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String guidStr;
    QStatus ret = ((ajn::BusAttachmentC*)bus)->GetPeerGUID(name, guidStr);
    if (guid != NULL) {
        strncpy(guid, guidStr.c_str(), *guidSz);
        //prevent sting not being null terminated.
        guid[*guidSz - 1] = '\0';
    }
    //size of the string plus the nul character
    *guidSz = guidStr.length() + 1;
    return ret;
}

QStatus alljoyn_busattachment_registersignalhandler(alljoyn_busattachment bus,
                                                    alljoyn_messagereceiver_signalhandler_ptr signal_handler,
                                                    const alljoyn_interfacedescription_member member,
                                                    const char* srcPath)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->RegisterSignalHandlerC(signal_handler,
                                                               member,
                                                               srcPath);
}

QStatus alljoyn_busattachment_unregistersignalhandler(alljoyn_busattachment bus,
                                                      alljoyn_messagereceiver_signalhandler_ptr signal_handler,
                                                      const alljoyn_interfacedescription_member member,
                                                      const char* srcPath)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->UnregisterSignalHandlerC(signal_handler,
                                                                 member,
                                                                 srcPath);
}

QStatus alljoyn_busattachment_unregisterallhandlers(alljoyn_busattachment bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->UnregisterAllHandlersC();
}

QStatus alljoyn_busattachment_setdaemondebug(alljoyn_busattachment bus, const char* module, uint32_t level)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->SetDaemonDebug(module, level);
}

uint32_t alljoyn_busattachment_gettimestamp()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ajn::BusAttachmentC::GetTimestamp();
}

QStatus alljoyn_busattachment_ping(alljoyn_busattachment bus, const char* name, uint32_t timeout)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusAttachmentC*)bus)->Ping(name, timeout);
}
