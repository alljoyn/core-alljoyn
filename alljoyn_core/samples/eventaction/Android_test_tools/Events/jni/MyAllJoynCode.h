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

#include <alljoyn/Status.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/SessionPortListener.h>
#include <alljoyn/SessionListener.h>
#include <alljoyn/Session.h>
#include <qcc/String.h>
#include <stdio.h>
#include <qcc/platform.h>
#include "Constants.h"
#include "EventInfo.h"

#include <alljoyn/about/AboutClient.h>
#include <alljoyn/about/AnnouncementRegistrar.h>

#ifndef _MY_ALLJOYN_CODE_
#define _MY_ALLJOYN_CODE_

class MyAllJoynCode;

class MyAllJoynCode :
    public ajn::services::AnnounceHandler,
    public ajn::BusAttachment::JoinSessionAsyncCB,
    public ajn::SessionListener {
  public:
    /**
     * Construct a MyAllJoynCode object
     *
     */
    MyAllJoynCode(JavaVM* vm, jobject jobj)
        : vm(vm), jobj(jobj), mBusAttachment(NULL), AnnounceHandler()
    { };

    /**
     * Destructor
     */
    ~MyAllJoynCode() {
        shutdown();
    };

    /**
     * Setup AllJoyn, creating the objects needed and registering listeners.
     *
     * @param packageName	This value is provided to the BusAttachment constructor to name the application
     *
     */
    void initialize(const char* packageName);

    /**
     * Join an AllJoyn session.
     *
     * @param sessionName	The busName/Wellknown name to join
     * @param port			The port value that the remote side has bound a session
     *
     */
    void joinSession(const char* sessionName, short port);

    /**
     * Leave an AllJoyn session.
     *
     * @param sessionId	The ID of the session to leave
     *
     */
    void leaveSession(int sessionId);

    /**
     * Perform an IntrospectionWithDescription request over AllJoyn
     *
     * @param sessionName	The busName/Wellknown name to introspect
     * @param path			introspect this specific path
     * @param sessionId		The ID of the session that the is established with sessionName
     *
     *
     */
    char* introspectWithDescriptions(const char* sessionName, const char* path, int sessionId);

    void enableEvent(EventInfo* event);

    /**
     * Free up and release the objects used
     */
    void shutdown();

    /* From About */
    void Announce(unsigned short version, unsigned short port, const char* busName,
                  const ajn::services::AboutClient::ObjectDescriptions& objectDescs,
                  const ajn::services::AboutClient::AboutData& aboutData);

    void EventHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& msg);

    /* For MethodCallAsync */
    void AsyncCallReplyHandler(ajn::Message& msg, void* context);

    /* From SessionListener */
    virtual void SessionLost(ajn::SessionId sessionId);

    /** JoinSessionAsync callback */
    virtual void JoinSessionCB(QStatus status, ajn::SessionId sessionId, const ajn::SessionOpts& opts, void* context);

  private:
    JavaVM* vm;
    jobject jobj;

    std::map<qcc::String, qcc::String> mBusFriendlyMap;
    std::map<qcc::String, int> mBusSessionMap;
    std::map<qcc::String, short> mBusPortMap;

    ajn::BusAttachment* mBusAttachment;
};

#endif //_MY_ALLJOYN_CODE_
