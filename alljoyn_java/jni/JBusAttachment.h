/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *    
 *    SPDX-License-Identifier: Apache-2.0
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *    
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *    
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *    
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
*
 ******************************************************************************/
#ifndef _ALLJOYN_JBUSATTACHMENT_H
#define _ALLJOYN_JBUSATTACHMENT_H

#include <jni.h>
#include <map>
#include <algorithm>
#include <list>
#include <utility>
#include <vector>

#include <qcc/Mutex.h>
#include <qcc/atomic.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/AutoPinger.h>
#include <alljoyn/PingListener.h>
#include <alljoyn/DBusStd.h>

#include "JAuthListener.h"
#include "JPermissionConfigurationListener.h"
#include "JApplicationStateListener.h"
#include "JKeyStoreListener.h"
#include "JSignalHandler.h"
#include "JAboutObject.h"
#include "PendingAsyncJoin.h"
#include "PendingAsyncPing.h"

class JAuthListener;
class JApplicationStateListener;
class JKeyStoreListener;
class JSignalHandler;
class JAboutObject;
class PendingAsyncJoin;
class PendingAsyncPing;

/**
 * The C++ class that backs the Java BusAttachment class and provides the
 * plumbing connection from AllJoyn out to Java-land.
 */
class JBusAttachment : public ajn::BusAttachment {
  public:
    JBusAttachment(const char* applicationName, bool allowRemoteMessages, int concurrency);
    QStatus Connect(const char* connectArgs, jobject jkeyStoreListener, const char* authMechanisms,
                    jobject jauthListener, const char* keyStoreFileName, jboolean isShared);
    void Disconnect();
    QStatus EnablePeerSecurity(const char* authMechanisms, jobject jauthListener, const char* keyStoreFileName, jboolean isShared, JPermissionConfigurationListener* jpcl = NULL);
    QStatus RegisterBusObject(const char* objPath, jobject jbusObject, jobjectArray jbusInterfaces,
                              jboolean jsecure, jstring jlangTag, jstring jdesc, jobject jtranslator);
    void UnregisterBusObject(jobject jbusObject);

    template <typename T>
    QStatus RegisterSignalHandler(const char* ifaceName, const char* signalName,
                                  jobject jsignalHandler, jobject jmethod, const char* ancillary);
    void UnregisterSignalHandler(jobject jsignalHandler, jobject jmethod);

    bool IsLocalBusObject(jobject jbusObject);
    void ForgetLocalBusObject(jobject jbusObject);

    /**
     * A mutex to serialize access to bus attachment critical sections.  It
     * doesn't seem worthwhile to have any finer granularity than this.  Note
     * that this member is public since we trust that the native binding we
     * wrote will use it correctly.
     */
    qcc::Mutex baCommonLock;

    /**
     * A mutex to serialize method call, property, etc., access in any attached
     * ProxyBusObject.  This is a blunt instrument, but support for
     * multi-threading on client and service sides has not been completely
     * implemented, so we simply disallow it for now.
     */
    qcc::Mutex baProxyLock;

    /**
     * A vector of all of the C++ "halves" of the signal handler objects
     * associated with this bus attachment.  Note that this member is public
     * since we trust that the native binding we wrote will use it correctly.
     */
    std::vector<std::pair<jobject, JSignalHandler*> > signalHandlers;

    /*
     * The single (optional) JPermissionConfigurationListener associated with this bus
     * attachment. Note that this member is public since we trust that the native binding
     * we wrote will use it correctly.
     */
    JPermissionConfigurationListener* jPermissionConfigurationListener;

    /**
     * A mutex to serialize access to bus attachment jApplicationStateListeners
     */
    qcc::Mutex baAppStateListenLock;

    /**
     * A vector of all of the JApplicationStateListeners
     */
    std::vector<JApplicationStateListener*> jApplicationStateListeners;

    /*
     * The single (optionsl) KeyStoreListener associated with this bus
     * attachment.  The KeyStoreListener and AuthListener work together to deal
     * with security exchanges over secure interfaces.  Note that this member is
     * public since we trust that the native binding we wrote will usse it
     * correctly.  When keyStoreListener is set, there must be a corresponding
     * strong reference to the associated Java Object set in
     * jkeyStoreListenerRef.
     */
    JKeyStoreListener* keyStoreListener;

    /**
     * A JNI strong global reference to The single (optional) Java KeyStoreListener
     * that has been associated with this bus attachment.  When jkeystoreListenerRef is
     * set, there must be a corresponding object pointer to an associated
     * C++ backing object set in keyStoreListener.
     */
    jobject jkeyStoreListenerRef;

    /**
     * The single (optional) C++ backing class for a provided AuthListener that
     * has been associated with this bus attachment.  The KeyStoreListener and
     * AuthListener work together to deal with security exchanges over secure
     * interfaces.  Note that this member is public since we trust that the
     * native binding we wrote will use it correctly.  When authListener is
     * set, there must be a corresponding strong reference to the associated
     * Java Object set in jauthListenerRef.
     */
    JAuthListener* authListener;

    /**
     * The single (optional) C++ backing class for JAboutObject. The aboutObj
     * contain a global ref jaboutObjGlobalRef that must be cleared when the
     * BusAttachment is disconnected.
     */
    JAboutObject* aboutObj;

    /**
     * A JNI strong global reference to The single (optional) Java AuthListener
     * that has been associated with this bus attachment.  When jauthListenerRef is
     * set, there must be a corresponding object pointer to an associated
     * C++ backing object set in authListener.
     */
    jobject jauthListenerRef;

    /**
     * A dedicated mutex to serialize access to the authListener,
     * authListenerRef, keyStoreListener and keyStoreListenerRef.  This is
     * required since we can't hold the common lock during callouts to Alljoyn
     * that may result in callins.  This describes the authentication process.
     * In order to prevent users from calling in right in the middle of an
     * authentication session and changing the authentication listeners out
     * from under us, we dedicate a lock that must be taken in order to make
     * a change.  This lock is held during the authentication process and during
     * the change process.
     */
    qcc::Mutex baAuthenticationChangeLock;

    /**
     * A list of strong references to Java bus listener objects.
     *
     * If clients use the unnamed parameter / unnamed class idiom to provide bus
     * listeners to registerBusListener, they can forget that the listeners
     * exist after the register call and never explicitly call unregister.
     *
     * Since we need these Java objects around, we need to hold a strong
     * reference to them to keep them from being garbage collected.
     *
     * Note that this member is public since we trust that the native binding we
     * wrote will use it correctly.
     */
    std::list<jobject> busListeners;

    /**
     * A list of strong references to Java translator objects.
     *
     * If clients use the unnamed parameter / unnamed class idiom to provide bus
     * listeners to setDescriptionTranslator, they can forget that the listeners
     * exist after the register call and never explicitly call unregister.
     *
     * Since we need these Java objects around, we need to hold a strong
     * reference to them to keep them from being garbage collected.
     *
     * Note that this member is public since we trust that the native binding we
     * wrote will usse it correctly.
     */
    std::list<jobject> translators;

    /**
     * A list of strong references to Java Bus Objects we use to indicate that
     * we have a part ownership in a given object.  Used during destruction.
     *
     */
    std::list<jobject> busObjects;

    /**
     * A map from session ports to their associated Java session port listeners.
     *
     * This mapping must be on a per-bus attachment basis since the scope of the
     * uniqueness of a session port is per-bus attachment
     *
     * Note that this member is public since we trust that the native binding we
     * wrote will usse it correctly.
     */
    std::map<ajn::SessionPort, jobject> sessionPortListenerMap;

    typedef struct {
        jobject jhostedListener;
        jobject jjoinedListener;
        jobject jListener;
    }BusAttachmentSessionListeners;

    /**
     * A map from sessions to their associated Java session listeners.
     *
     * This mapping must be on a per-bus attachment basis since the uniqueness of a
     * session is per-bus attachment.
     *
     * Note that this member is public since we trust that the native binding we
     * wrote will usse it correctly.
     */

    std::map<ajn::SessionId, BusAttachmentSessionListeners> sessionListenerMap;

    /**
     * A List of pending asynchronous join operation informations.  We store
     * Java object references here while AllJoyn mulls over what it can do about
     * the operation. Note that this member is public since we trust that the
     * native binding we wrote will use it correctly.
     */
    std::list<PendingAsyncJoin*> pendingAsyncJoins;

    /**
     * A List of pending asynchronous ping operation informations.  We store
     * Java object references here while AllJoyn mulls over what it can do about
     * the operation. Note that this member is public since we trust that the
     * native binding we wrote will use it correctly.
     */
    std::list<PendingAsyncPing*> pendingAsyncPings;

    int32_t IncRef(void)
    {
        return qcc::IncrementAndFetch(&refCount);
    }

    int32_t DecRef(void)
    {
        int32_t refs = qcc::DecrementAndFetch(&refCount);
        if (refs == 0) {
            delete this;
        }
        return refs;
    }

    int32_t GetRef(void)
    {
        return refCount;
    }

  private:
    JBusAttachment(const JBusAttachment& other);
    JBusAttachment& operator =(const JBusAttachment& other);

    /*
     * An intrusive reference count
     */
    volatile int32_t refCount;

    /*
     * Destructor is marked private since it should only be called from DecRef.
     */
    virtual ~JBusAttachment();

};

#endif