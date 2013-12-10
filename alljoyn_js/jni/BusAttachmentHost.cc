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
#include "BusAttachmentHost.h"

#include "AcceptSessionJoinerListenerNative.h"
#include "AuthListenerNative.h"
#include "BusListenerNative.h"
#include "BusObject.h"
#include "BusObjectNative.h"
#include "CallbackNative.h"
#include "InterfaceDescriptionNative.h"
#include "MessageHost.h"
#include "MessageListenerNative.h"
#include "SessionJoinedListenerNative.h"
#include "SessionLostListenerNative.h"
#include "SessionMemberAddedListenerNative.h"
#include "SessionMemberRemovedListenerNative.h"
#include "SignalEmitterHost.h"
#include "SocketFdHost.h"
#include "Transport.h"
#include "TypeMapping.h"
#include <qcc/Debug.h>
#include <assert.h>

#define QCC_MODULE "ALLJOYN_JS"

class SignalReceiver : public ajn::MessageReceiver {
  public:
    class _Env {
      public:
        Plugin plugin;
        BusAttachment busAttachment;
        MessageListenerNative* signalListener;
        const ajn::InterfaceDescription::Member* signal;
        qcc::String sourcePath;

        _Env(Plugin& plugin,
             BusAttachment& busAttachment,
             MessageListenerNative* signalListener,
             const ajn::InterfaceDescription::Member* signal,
             qcc::String& sourcePath) :
            plugin(plugin),
            busAttachment(busAttachment),
            signalListener(signalListener),
            signal(signal),
            sourcePath(sourcePath)
        {
            QCC_DbgTrace(("%s this=%p", __FUNCTION__, this));
        }

        ~_Env() {
            delete signalListener;
        }
    };

    typedef qcc::ManagedObj<_Env> Env;
    Env env;

    SignalReceiver(Plugin& plugin,
                   BusAttachment& busAttachment,
                   MessageListenerNative* signalListener,
                   const ajn::InterfaceDescription::Member* signal,
                   qcc::String& sourcePath) :
        env(plugin, busAttachment, signalListener, signal, sourcePath)
    {
        QCC_DbgTrace(("%s this=%p", __FUNCTION__, this));
    }

    virtual ~SignalReceiver()
    {
        QCC_DbgTrace(("%s this=%p", __FUNCTION__, this));
    }

    class SignalHandlerContext : public PluginData::CallbackContext {
      public:
        Env env;
        const ajn::InterfaceDescription::Member* member;
        qcc::String sourcePath;
        ajn::Message message;

        SignalHandlerContext(Env& env,
                             const ajn::InterfaceDescription::Member* member,
                             const char* sourcePath,
                             ajn::Message& message) :
            env(env),
            member(member),
            sourcePath(sourcePath),
            message(message) { }
    };

    virtual void SignalHandler(const ajn::InterfaceDescription::Member* member,
                               const char* sourcePath,
                               ajn::Message& message)
    {
        PluginData::Callback callback(env->plugin, _SignalHandler);
        callback->context = new SignalHandlerContext(env, member, sourcePath, message);
        PluginData::DispatchCallback(callback);
    }

    static void _SignalHandler(PluginData::CallbackContext* ctx)
    {
        SignalHandlerContext* context = static_cast<SignalHandlerContext*>(ctx);
        MessageHost messageHost(context->env->plugin, context->env->busAttachment, context->message);
        size_t numArgs;
        const ajn::MsgArg* args;

        context->message->GetArgs(numArgs, args);
        context->env->signalListener->onMessage(messageHost, args, numArgs);
    }
};


class BusListener : public ajn::BusListener {
  public:
    class _Env {
      public:
        Plugin plugin;
        /*
         * Use a naked pointer here instead of a ManagedObj since the lifetime of BusListener is tied
         * to the lifetime of the BusAttachmentHost.  If we use a ManagedObj, then there is a circular
         * reference and the BusAttachmentHost may never be deleted.
         */
        _BusAttachmentHost* busAttachmentHost;
        BusAttachment busAttachment;
        BusListenerNative* busListenerNative;

        _Env(Plugin& plugin,
             _BusAttachmentHost* busAttachmentHost,
             BusAttachment& busAttachment,
             BusListenerNative* busListenerNative) :
            plugin(plugin),
            busAttachmentHost(busAttachmentHost),
            busAttachment(busAttachment),
            busListenerNative(busListenerNative) { }

        ~_Env() {
            delete busListenerNative;
        }
    };

    typedef qcc::ManagedObj<_Env> Env;
    Env env;

    BusListener(Plugin& plugin,
                _BusAttachmentHost* busAttachmentHost,
                BusAttachment& busAttachment,
                BusListenerNative* busListenerNative) :
        env(plugin, busAttachmentHost, busAttachment, busListenerNative) { }

    virtual ~BusListener() { }

    class ListenerRegisteredContext : public PluginData::CallbackContext {
      public:
        Env env;
        BusAttachmentHost busAttachmentHost;
        ListenerRegisteredContext(Env& env, BusAttachmentHost& busAttachmentHost) :
            env(env),
            busAttachmentHost(busAttachmentHost) { }
    };

    virtual void ListenerRegistered(ajn::BusAttachment* bus) {
        /*
         * Capture the naked pointer into a ManagedObj.  This is safe to do here (and is necessary) since
         * this call will not occur without a valid BusAttachmentHost.  The same cannot be said of the
         * dispatched callback below (_ListenerRegistered).
         */
        BusAttachmentHost busAttachmentHost = BusAttachmentHost::wrap(env->busAttachmentHost);
        PluginData::Callback callback(env->plugin, _ListenerRegistered);
        callback->context = new ListenerRegisteredContext(env, busAttachmentHost);
        PluginData::DispatchCallback(callback);
    }

    static void _ListenerRegistered(PluginData::CallbackContext* ctx) {
        ListenerRegisteredContext* context = static_cast<ListenerRegisteredContext*>(ctx);
        context->env->busListenerNative->onRegistered(context->busAttachmentHost);
    }

    class ListenerUnregisteredContext : public PluginData::CallbackContext {
      public:
        Env env;
        ListenerUnregisteredContext(Env& env) :
            env(env) { }
    };

    virtual void ListenerUnregistered()
    {
        PluginData::Callback callback(env->plugin, _ListenerUnregistered);
        callback->context = new ListenerUnregisteredContext(env);
        PluginData::DispatchCallback(callback);
    }

    static void _ListenerUnregistered(PluginData::CallbackContext* ctx)
    {
        ListenerUnregisteredContext* context = static_cast<ListenerUnregisteredContext*>(ctx);
        context->env->busListenerNative->onUnregistered();
    }

    class FoundAdvertisedNameContext : public PluginData::CallbackContext {
      public:
        Env env;
        qcc::String name;
        ajn::TransportMask transport;
        qcc::String namePrefix;

        FoundAdvertisedNameContext(Env& env, const char* name, ajn::TransportMask transport, const char* namePrefix) :
            env(env),
            name(name),
            transport(transport),
            namePrefix(namePrefix) { }
    };

    virtual void FoundAdvertisedName(const char* name, ajn::TransportMask transport, const char* namePrefix)
    {
        PluginData::Callback callback(env->plugin, _FoundAdvertisedName);
        callback->context = new FoundAdvertisedNameContext(env, name, transport, namePrefix);
        PluginData::DispatchCallback(callback);
    }

    static void _FoundAdvertisedName(PluginData::CallbackContext* ctx)
    {
        FoundAdvertisedNameContext* context = static_cast<FoundAdvertisedNameContext*>(ctx);
        context->env->busListenerNative->onFoundAdvertisedName(context->name, context->transport, context->namePrefix);
    }

    class LostAdvertisedNameContext : public PluginData::CallbackContext {
      public:
        Env env;
        qcc::String name;
        ajn::TransportMask transport;
        qcc::String namePrefix;

        LostAdvertisedNameContext(Env& env, const char* name, ajn::TransportMask transport, const char* namePrefix) :
            env(env),
            name(name),
            transport(transport),
            namePrefix(namePrefix) { }
    };

    virtual void LostAdvertisedName(const char* name, ajn::TransportMask transport, const char* namePrefix)
    {
        PluginData::Callback callback(env->plugin, _LostAdvertisedName);
        callback->context = new LostAdvertisedNameContext(env, name, transport, namePrefix);
        PluginData::DispatchCallback(callback);
    }

    static void _LostAdvertisedName(PluginData::CallbackContext* ctx)
    {
        LostAdvertisedNameContext* context = static_cast<LostAdvertisedNameContext*>(ctx);
        context->env->busListenerNative->onLostAdvertisedName(context->name, context->transport, context->namePrefix);
    }

    class NameOwnerChangedContext : public PluginData::CallbackContext {
      public:
        Env env;
        qcc::String busName;
        qcc::String previousOwner;
        qcc::String newOwner;

        NameOwnerChangedContext(Env& env, const char* busName, const char* previousOwner, const char* newOwner) :
            env(env),
            busName(busName),
            previousOwner(previousOwner),
            newOwner(newOwner) { }
    };

    virtual void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
    {
        PluginData::Callback callback(env->plugin, _NameOwnerChanged);
        callback->context = new NameOwnerChangedContext(env, busName, previousOwner, newOwner);
        PluginData::DispatchCallback(callback);
    }

    static void _NameOwnerChanged(PluginData::CallbackContext* ctx)
    {
        NameOwnerChangedContext* context = static_cast<NameOwnerChangedContext*>(ctx);
        context->env->busListenerNative->onNameOwnerChanged(context->busName,
                                                            context->previousOwner,
                                                            context->newOwner);
    }

    class PropertyChangedContext : public PluginData::CallbackContext {
      public:
        Env env;
        const qcc::String propName;
        const ajn::MsgArg* propValue;

        PropertyChangedContext(Env& env, const char* propName, const ajn::MsgArg* propValue) :
            env(env),
            propName(propName),
            propValue(propValue) { }
    };

    virtual void PropertyChanged(const char* propName, const ajn::MsgArg* propValue)
    {
        PluginData::Callback callback(env->plugin, _PropertyChanged);
        callback->context = new PropertyChangedContext(env, propName, propValue);
        PluginData::DispatchCallback(callback);
    }

    static void _PropertyChanged(PluginData::CallbackContext* ctx)
    {
        PropertyChangedContext* context = static_cast<PropertyChangedContext*>(ctx);
        context->env->busListenerNative->onPropertyChanged(context->propName, context->propValue);
    }


    class BusStoppingContext : public PluginData::CallbackContext {
      public:
        Env env;

        BusStoppingContext(Env& env) :
            env(env) { }
    };

    virtual void BusStopping()
    {
        PluginData::Callback callback(env->plugin, _BusStopping);
        callback->context = new BusStoppingContext(env);
        PluginData::DispatchCallback(callback);
    }

    static void _BusStopping(PluginData::CallbackContext* ctx)
    {
        BusStoppingContext* context = static_cast<BusStoppingContext*>(ctx);
        context->env->busListenerNative->onStopping();
    }

    class BusDisconnectedContext : public PluginData::CallbackContext {
      public:
        Env env;

        BusDisconnectedContext(Env& env) :
            env(env) { }
    };

    virtual void BusDisconnected()
    {
        PluginData::Callback callback(env->plugin, _BusDisconnected);
        callback->context = new BusDisconnectedContext(env);
        PluginData::DispatchCallback(callback);
    }

    static void _BusDisconnected(PluginData::CallbackContext* ctx)
    {
        BusDisconnectedContext* context = static_cast<BusDisconnectedContext*>(ctx);
        context->env->busListenerNative->onDisconnected();
    }
};


class SessionListener : public ajn::SessionListener {
  public:
    class _Env {
      public:
        Plugin plugin;
        BusAttachment busAttachment;
        SessionLostListenerNative* sessionLostListenerNative;
        SessionMemberAddedListenerNative* sessionMemberAddedListenerNative;
        SessionMemberRemovedListenerNative* sessionMemberRemovedListenerNative;

        _Env(Plugin& plugin,
             BusAttachment& busAttachment,
             SessionLostListenerNative* sessionLostListenerNative,
             SessionMemberAddedListenerNative* sessionMemberAddedListenerNative,
             SessionMemberRemovedListenerNative* sessionMemberRemovedListenerNative) :
            plugin(plugin),
            busAttachment(busAttachment),
            sessionLostListenerNative(sessionLostListenerNative),
            sessionMemberAddedListenerNative(sessionMemberAddedListenerNative),
            sessionMemberRemovedListenerNative(sessionMemberRemovedListenerNative)
        { }

        _Env(Plugin& plugin,
             BusAttachment& busAttachment) :
            plugin(plugin),
            busAttachment(busAttachment),
            sessionLostListenerNative(NULL),
            sessionMemberAddedListenerNative(NULL),
            sessionMemberRemovedListenerNative(NULL)
        { }

        ~_Env()
        {
            delete sessionLostListenerNative;
            delete sessionMemberAddedListenerNative;
            delete sessionMemberRemovedListenerNative;
        }
    };

    typedef qcc::ManagedObj<_Env> Env;
    Env env;

    SessionListener(Plugin& plugin,
                    BusAttachment& busAttachment,
                    SessionLostListenerNative* sessionLostListenerNative,
                    SessionMemberAddedListenerNative* sessionMemberAddedListenerNative,
                    SessionMemberRemovedListenerNative* sessionMemberRemovedListenerNative) :
        env(plugin, busAttachment, sessionLostListenerNative, sessionMemberAddedListenerNative, sessionMemberRemovedListenerNative)
    { }

    SessionListener(Plugin& plugin, BusAttachment& busAttachment, Env& env) : env(env) { }

    SessionListener(Plugin& plugin, BusAttachment& busAttachment) : env(plugin, busAttachment) { }

    virtual ~SessionListener() { }

    class SessionLostContext : public PluginData::CallbackContext {
      public:
        Env env;
        ajn::SessionId id;
        ajn::SessionListener::SessionLostReason reason;

        SessionLostContext(Env& env, ajn::SessionId id, ajn::SessionListener::SessionLostReason reason) :
            env(env), id(id), reason(reason)
        { }
    };

    virtual void SessionLost(ajn::SessionId id, ajn::SessionListener::SessionLostReason reason)
    {
        PluginData::Callback callback(env->plugin, _SessionLost);
        callback->context = new SessionLostContext(env, id, reason);
        PluginData::DispatchCallback(callback);
    }

    static void _SessionLost(PluginData::CallbackContext* ctx)
    {
        SessionLostContext* context = static_cast<SessionLostContext*>(ctx);
        if (context->env->sessionLostListenerNative) {
            context->env->sessionLostListenerNative->onLost(context->id, context->reason);
        }
    }

    class SessionMemberAddedContext : public PluginData::CallbackContext {
      public:
        Env env;
        ajn::SessionId id;
        qcc::String uniqueName;

        SessionMemberAddedContext(Env& env, ajn::SessionId id, const char* uniqueName) :
            env(env),
            id(id),
            uniqueName(uniqueName)
        { }
    };

    virtual void SessionMemberAdded(ajn::SessionId id, const char* uniqueName)
    {
        PluginData::Callback callback(env->plugin, _SessionMemberAdded);
        callback->context = new SessionMemberAddedContext(env, id, uniqueName);
        PluginData::DispatchCallback(callback);
    }

    static void _SessionMemberAdded(PluginData::CallbackContext* ctx)
    {
        SessionMemberAddedContext* context = static_cast<SessionMemberAddedContext*>(ctx);
        if (context->env->sessionMemberAddedListenerNative) {
            context->env->sessionMemberAddedListenerNative->onMemberAdded(context->id, context->uniqueName);
        }
    }

    class SessionMemberRemovedContext : public PluginData::CallbackContext {
      public:
        Env env;
        ajn::SessionId id;
        qcc::String uniqueName;

        SessionMemberRemovedContext(Env& env, ajn::SessionId id, const char* uniqueName) :
            env(env),
            id(id),
            uniqueName(uniqueName) { }
    };

    virtual void SessionMemberRemoved(ajn::SessionId id, const char* uniqueName)
    {
        PluginData::Callback callback(env->plugin, _SessionMemberRemoved);
        callback->context = new SessionMemberRemovedContext(env, id, uniqueName);
        PluginData::DispatchCallback(callback);
    }

    static void _SessionMemberRemoved(PluginData::CallbackContext* ctx)
    {
        SessionMemberRemovedContext* context = static_cast<SessionMemberRemovedContext*>(ctx);
        if (context->env->sessionMemberRemovedListenerNative) {
            context->env->sessionMemberRemovedListenerNative->onMemberRemoved(context->id, context->uniqueName);
        }
    }
};


class SessionPortListener : public ajn::SessionPortListener {
  public:

    class _Env {
      public:
        Plugin plugin;
        /*
         * Use a naked pointer here instead of a ManagedObj since the lifetime of SessionPortListener is tied
         * to the lifetime of the BusAttachmentHost.  If we use a ManagedObj, then there is a circular
         * reference and the BusAttachmentHost may never be deleted.
         */
        _BusAttachmentHost* busAttachmentHost;
        BusAttachment busAttachment;
        AcceptSessionJoinerListenerNative* acceptSessionListenerNative;
        SessionJoinedListenerNative* sessionJoinedListenerNative;
        SessionListener::Env sessionListenerEnv;

        _Env(Plugin& plugin,
             _BusAttachmentHost* busAttachmentHost,
             BusAttachment& busAttachment,
             AcceptSessionJoinerListenerNative* acceptSessionListenerNative,
             SessionJoinedListenerNative* sessionJoinedListenerNative,
             SessionListener::Env sessionListenerEnv) :
            plugin(plugin),
            busAttachmentHost(busAttachmentHost),
            busAttachment(busAttachment),
            acceptSessionListenerNative(acceptSessionListenerNative),
            sessionJoinedListenerNative(sessionJoinedListenerNative),
            sessionListenerEnv(sessionListenerEnv)
        { }

        ~_Env()
        {
            delete sessionJoinedListenerNative;
            delete acceptSessionListenerNative;
        }
    };

    typedef qcc::ManagedObj<_Env> Env;
    Env env;
    qcc::Event cancelEvent;

    SessionPortListener(Plugin& plugin,
                        _BusAttachmentHost* busAttachmentHost,
                        BusAttachment& busAttachment,
                        AcceptSessionJoinerListenerNative* acceptSessionListenerNative,
                        SessionJoinedListenerNative* sessionJoinedListenerNative,
                        SessionListener::Env sessionListenerEnv) :
        env(plugin, busAttachmentHost, busAttachment, acceptSessionListenerNative, sessionJoinedListenerNative, sessionListenerEnv)
    { }

    virtual ~SessionPortListener() { }

    class AcceptSessionJoinerContext : public PluginData::CallbackContext {
      public:
        Env env;
        ajn::SessionPort sessionPort;
        qcc::String joiner;
        const ajn::SessionOpts opts;
        AcceptSessionJoinerContext(Env& env, ajn::SessionPort sessionPort, const char* joiner, const ajn::SessionOpts& opts) :
            env(env),
            sessionPort(sessionPort),
            joiner(joiner),
            opts(opts)
        { }
    };

    virtual bool AcceptSessionJoiner(ajn::SessionPort sessionPort, const char* joiner, const ajn::SessionOpts& opts)
    {
        PluginData::Callback callback(env->plugin, _AcceptSessionJoiner);
        callback->context = new AcceptSessionJoinerContext(env, sessionPort, joiner, opts);
        PluginData::DispatchCallback(callback);

        /*
         * Complex processing here to prevent UI thread from deadlocking if it ends up calling
         * unbindSessionPort.
         *
         * UnbindSessionPort() will block until all AcceptSessionJoiner callbacks have returned.
         * Setting the cancelEvent will unblock any synchronous callback.  Then a little extra
         * coordination is needed to remove the dispatch context so that when the dispatched callback
         * is run it does nothing.
         */
        std::vector<qcc::Event*> check;
        check.push_back(&callback->context->event);
        check.push_back(&cancelEvent);

        std::vector<qcc::Event*> signaled;
        signaled.clear();

        env->busAttachment->EnableConcurrentCallbacks();
        QStatus status = qcc::Event::Wait(check, signaled);
        assert(ER_OK == status);
        if (ER_OK != status) {
            QCC_LogError(status, ("Wait failed"));
        }

        for (std::vector<qcc::Event*>::iterator i = signaled.begin(); i != signaled.end(); ++i) {
            if (*i == &cancelEvent) {
                PluginData::CancelCallback(callback);
                callback->context->status = ER_ALERTED_THREAD;
                break;
            }
        }

        return (ER_OK == callback->context->status);
    }

    static void _AcceptSessionJoiner(PluginData::CallbackContext* ctx)
    {
        AcceptSessionJoinerContext* context = static_cast<AcceptSessionJoinerContext*>(ctx);

        if (context->env->acceptSessionListenerNative) {
            SessionOptsHost optsHost(context->env->plugin, context->opts);
            bool accepted = context->env->acceptSessionListenerNative->onAccept(context->sessionPort, context->joiner, optsHost);
            context->status = accepted ? ER_OK : ER_FAIL;
        } else {
            context->status = ER_FAIL;
        }
    }

    class SessionJoinedContext : public PluginData::CallbackContext {
      public:
        Env env;
        BusAttachmentHost busAttachmentHost;
        ajn::SessionPort sessionPort;
        ajn::SessionId id;
        qcc::String joiner;
        SessionListener* sessionListener;

        SessionJoinedContext(Env& env,
                             BusAttachmentHost& busAttachmentHost,
                             ajn::SessionPort sessionPort,
                             ajn::SessionId id,
                             const char* joiner,
                             SessionListener* sessionListener) :
            env(env),
            busAttachmentHost(busAttachmentHost),
            sessionPort(sessionPort),
            id(id),
            joiner(joiner),
            sessionListener(sessionListener)
        { }

        virtual ~SessionJoinedContext()
        { }
    };

    virtual void SessionJoined(ajn::SessionPort sessionPort, ajn::SessionId id, const char* joiner)
    {
        SessionListener* sessionListener = NULL;

        /*
         * We have to do this here, otherwise we can miss the session member added callback (the app won't have called
         * setSessionListener soon enough).
         */

        if (env->sessionListenerEnv->sessionLostListenerNative ||
            env->sessionListenerEnv->sessionMemberAddedListenerNative ||
            env->sessionListenerEnv->sessionMemberRemovedListenerNative) {
            sessionListener = new SessionListener(env->plugin, env->busAttachment, env->sessionListenerEnv);
            QStatus status = env->busAttachment->SetSessionListener(id, sessionListener);
            if (status != ER_OK) {
                QCC_LogError(status, ("SetSessionListener failed"));
                delete sessionListener;
            }
        }

        /*
         * Capture the naked pointer into a ManagedObj.  This is safe to do here (and is necessary) since
         * this call will not occur without a valid BusAttachmentHost.  The same cannot be said of the
         * dispatched callback below (_SessionJoined).
         */
        BusAttachmentHost busAttachmentHost = BusAttachmentHost::wrap(env->busAttachmentHost);
        PluginData::Callback callback(env->plugin, _SessionJoined);
        callback->context = new SessionJoinedContext(env, busAttachmentHost, sessionPort, id, joiner, sessionListener);
        PluginData::DispatchCallback(callback);
    }

    static void _SessionJoined(PluginData::CallbackContext* ctx)
    {
        SessionJoinedContext* context = static_cast<SessionJoinedContext*>(ctx);
        if (context->sessionListener) {
            std::pair<ajn::SessionId, SessionListener*> element(context->id, context->sessionListener);
            context->busAttachmentHost->sessionListeners.insert(element);
        }

        if (context->env->sessionJoinedListenerNative) {
            context->env->sessionJoinedListenerNative->onJoined(context->sessionPort, context->id, context->joiner);
        }
    }
};


class JoinSessionAsyncCB : public ajn::BusAttachment::JoinSessionAsyncCB {
  public:
    class _Env {
      public:
        Plugin plugin;
        BusAttachmentHost busAttachmentHost;
        BusAttachment busAttachment;
        CallbackNative* callbackNative;
        SessionListener* sessionListener;
        QStatus status;

        _Env(Plugin& plugin,
             BusAttachmentHost& busAttachmentHost,
             BusAttachment& busAttachment,
             CallbackNative* callbackNative,
             SessionListener* sessionListener) :
            plugin(plugin),
            busAttachmentHost(busAttachmentHost),
            busAttachment(busAttachment),
            callbackNative(callbackNative),
            sessionListener(sessionListener) { }

        ~_Env() {
            delete sessionListener;
            if (callbackNative) {
                CallbackNative::DispatchCallback(plugin, callbackNative, status);
                callbackNative = NULL;
            }
        }
    };

    typedef qcc::ManagedObj<_Env> Env;
    Env env;

    JoinSessionAsyncCB(Plugin& plugin,
                       BusAttachmentHost& busAttachmentHost,
                       BusAttachment& busAttachment,
                       CallbackNative* callbackNative,
                       SessionListener* sessionListener) :
        env(plugin, busAttachmentHost, busAttachment, callbackNative, sessionListener)
    { }

    virtual ~JoinSessionAsyncCB() { }

    class JoinSessionCBContext : public PluginData::CallbackContext {
      public:
        Env env;
        QStatus status;
        ajn::SessionId sessionId;
        ajn::SessionOpts sessionOpts;
        JoinSessionCBContext(Env& env, QStatus status, ajn::SessionId sessionId, ajn::SessionOpts sessionOpts) :
            env(env),
            status(status),
            sessionId(sessionId),
            sessionOpts(sessionOpts)
        { }
    };

    virtual void JoinSessionCB(QStatus status, ajn::SessionId sessionId, const ajn::SessionOpts& opts, void*)
    {
        Plugin plugin = env->plugin;
        PluginData::Callback callback(env->plugin, _JoinSessionCB);
        callback->context = new JoinSessionCBContext(env, status, sessionId, opts);
        delete this;
        PluginData::DispatchCallback(callback);
    }

    static void _JoinSessionCB(PluginData::CallbackContext* ctx)
    {
        JoinSessionCBContext* context = static_cast<JoinSessionCBContext*>(ctx);
        if (ER_OK == context->status) {
            context->env->busAttachment->SetSessionListener(context->sessionId, context->env->sessionListener);
            std::pair<ajn::SessionId, SessionListener*> element(context->sessionId, context->env->sessionListener);
            context->env->busAttachmentHost->sessionListeners.insert(element);
            context->env->sessionListener = NULL; /* sessionListeners now owns sessionListener */
            SessionOptsHost sessionOpts(context->env->plugin, context->sessionOpts);
            context->env->callbackNative->onCallback(context->status, context->sessionId, sessionOpts);
        } else {
            BusErrorHost busError(context->env->plugin, context->status);
            context->env->callbackNative->onCallback(busError);
        }

        delete context->env->callbackNative;
        context->env->callbackNative = NULL;
    }
};


class BusObjectListener : public _BusObjectListener {
  public:
    class _Env {
      public:
        Plugin plugin;
        BusAttachment busAttachment;
        BusObject busObject;
        BusObjectNative* busObjectNative;

        _Env(Plugin& plugin,
             BusAttachment& busAttachment,
             const char* path,
             BusObjectNative* busObjectNative) :
            plugin(plugin),
            busAttachment(busAttachment),
            busObject(busAttachment, path),
            busObjectNative(busObjectNative) { }

        ~_Env() {
            delete busObjectNative;
        }
    };

    typedef qcc::ManagedObj<_Env> Env;
    mutable Env env; /* mutable so that GenerateIntrospection can be declared const to match ajn::BusObject */

    BusObjectListener(Plugin& plugin,
                      BusAttachment& busAttachment,
                      const char* path,
                      BusObjectNative* busObjectNative) :
        env(plugin, busAttachment, path, busObjectNative)
    {
        env->busObject->SetBusObjectListener(this);
    }

    virtual ~BusObjectListener()
    {
        env->busObject->SetBusObjectListener(0);
    }

    QStatus AddInterfaceAndMethodHandlers()
    {
        QStatus status = ER_OK;
        bool hasSignal = false;
        NPIdentifier* properties = NULL;
        uint32_t propertiesCount = 0;
        if (NPN_Enumerate(env->plugin->npp, env->busObjectNative->objectValue, &properties, &propertiesCount)) {
            for (uint32_t i = 0; (ER_OK == status) && (i < propertiesCount); ++i) {
                if (!NPN_IdentifierIsString(properties[i])) {
                    continue;
                }

                NPUTF8* property = NPN_UTF8FromIdentifier(properties[i]);
                if (!property) {
                    status = ER_OUT_OF_MEMORY;
                    break;
                }

                const ajn::InterfaceDescription* interface = env->busAttachment->GetInterface(property);
                if (!interface) {
                    QCC_DbgHLPrintf(("No such interface '%s', ignoring", property));
                }

                NPN_MemFree(property);
                if (!interface) {
                    continue;
                }

                QCC_DbgTrace(("Adding '%s'", interface->GetName()));
                status = env->busObject->AddInterface(*interface);
                if (ER_OK != status) {
                    QCC_LogError(status, ("AddInterface failed"));
                    break;
                }

                size_t numMembers = interface->GetMembers();
                if (!numMembers) {
                    continue;
                }

                const ajn::InterfaceDescription::Member** members = new const ajn::InterfaceDescription::Member *[numMembers];
                interface->GetMembers(members, numMembers);
                for (size_t j = 0; (ER_OK == status) && (j < numMembers); ++j) {
                    if (ajn::MESSAGE_METHOD_CALL == members[j]->memberType) {
                        status = env->busObject->AddMethodHandler(members[j]);
                    } else if (ajn::MESSAGE_SIGNAL == members[j]->memberType) {
                        hasSignal = true;
                    }
                }

                delete[] members;
            }

            NPN_MemFree(properties);
        }

        if (hasSignal) {
            SignalEmitterHost emitter(env->plugin, env->busObject);
            NPVariant npemitter;
            ToHostObject<SignalEmitterHost>(env->plugin, emitter, npemitter);
            if (!NPN_SetProperty(env->plugin->npp, env->busObjectNative->objectValue, NPN_GetStringIdentifier("signal"), &npemitter)) {
                status = ER_FAIL;
                QCC_LogError(status, ("NPN_SetProperty failed"));
            }

            NPN_ReleaseVariantValue(&npemitter);
        }

        return status;
    }

    class MethodHandlerContext : public PluginData::CallbackContext {
      public:
        Env env;
        const ajn::InterfaceDescription::Member* member;
        ajn::Message message;

        MethodHandlerContext(Env& env, const ajn::InterfaceDescription::Member* member, ajn::Message& message) :
            env(env),
            member(member),
            message(message)
        { }
    };

    void MethodHandler(const ajn::InterfaceDescription::Member* member, ajn::Message& message)
    {
        PluginData::Callback callback(env->plugin, _MethodHandler);
        callback->context = new MethodHandlerContext(env, member, message);
        PluginData::DispatchCallback(callback);
    }

    static void _MethodHandler(PluginData::CallbackContext* ctx)
    {
        MethodHandlerContext* context = static_cast<MethodHandlerContext*>(ctx);
        MessageReplyHost messageReplyHost(context->env->plugin, context->env->busAttachment, context->env->busObject, context->message, context->member->returnSignature);
        size_t numArgs;
        const ajn::MsgArg* args;

        context->message->GetArgs(numArgs, args);
        context->env->busObjectNative->onMessage(context->member->iface->GetName(), context->member->name.c_str(), messageReplyHost, args, numArgs);
    }

    class ObjectRegisteredContext : public PluginData::CallbackContext {
      public:
        Env env;
        ObjectRegisteredContext(Env& env) :
            env(env) { }
    };

    virtual void ObjectRegistered()
    {
        PluginData::Callback callback(env->plugin, _ObjectRegistered);
        callback->context = new ObjectRegisteredContext(env);
        PluginData::DispatchCallback(callback);
    }

    static void _ObjectRegistered(PluginData::CallbackContext* ctx)
    {
        ObjectRegisteredContext* context = static_cast<ObjectRegisteredContext*>(ctx);
        context->env->busObjectNative->onRegistered();
    }

    class ObjectUnregisteredContext : public PluginData::CallbackContext {
      public:
        Env env;
        ObjectUnregisteredContext(Env& env) :
            env(env) { }
    };

    virtual void ObjectUnregistered()
    {
        PluginData::Callback callback(env->plugin, _ObjectUnregistered);
        callback->context = new ObjectUnregisteredContext(env);
        PluginData::DispatchCallback(callback);
    }

    static void _ObjectUnregistered(PluginData::CallbackContext* ctx)
    {
        ObjectUnregisteredContext* context = static_cast<ObjectUnregisteredContext*>(ctx);
        context->env->busObjectNative->onUnregistered();
    }

    class GetContext : public PluginData::CallbackContext {
      public:
        Env env;
        qcc::String ifcName;
        qcc::String propName;
        ajn::MsgArg val;
        GetContext(Env& env, const char* ifcName, const char* propName, ajn::MsgArg& val) :
            env(env),
            ifcName(ifcName),
            propName(propName),
            val(val) { }
    };

    virtual QStatus Get(const char* ifcName, const char* propName, ajn::MsgArg& val)
    {
        PluginData::Callback callback(env->plugin, _Get);
        callback->context = new GetContext(env, ifcName, propName, val);
        PluginData::DispatchCallback(callback);
        env->busAttachment->EnableConcurrentCallbacks();
        qcc::Event::Wait(callback->context->event);
        val = static_cast<GetContext*>(callback->context)->val;
        return callback->context->status;
    }

    static void _Get(PluginData::CallbackContext* ctx)
    {
        GetContext* context = static_cast<GetContext*>(ctx);
        const ajn::InterfaceDescription* interface = context->env->busAttachment->GetInterface(context->ifcName.c_str());
        if (!interface) {
            context->status = ER_BUS_NO_SUCH_INTERFACE;
            return;
        }

        const ajn::InterfaceDescription::Property* property = interface->GetProperty(context->propName.c_str());
        if (!property) {
            context->status = ER_BUS_NO_SUCH_PROPERTY;
            return;
        }

        context->status = context->env->busObjectNative->get(interface, property, context->val);
    }

    class SetContext : public PluginData::CallbackContext {
      public:
        Env env;
        qcc::String ifcName;
        qcc::String propName;
        ajn::MsgArg val;

        SetContext(Env& env, const char* ifcName, const char* propName, ajn::MsgArg& val) :
            env(env),
            ifcName(ifcName),
            propName(propName),
            val(val) { }
    };

    virtual QStatus Set(const char* ifcName, const char* propName, ajn::MsgArg& val)
    {
        PluginData::Callback callback(env->plugin, _Set);
        callback->context = new SetContext(env, ifcName, propName, val);
        PluginData::DispatchCallback(callback);
        env->busAttachment->EnableConcurrentCallbacks();
        qcc::Event::Wait(callback->context->event);
        return callback->context->status;
    }

    static void _Set(PluginData::CallbackContext* ctx)
    {
        SetContext* context = static_cast<SetContext*>(ctx);
        const ajn::InterfaceDescription* interface = context->env->busAttachment->GetInterface(context->ifcName.c_str());
        if (!interface) {
            context->status = ER_BUS_NO_SUCH_INTERFACE;
            return;
        }

        const ajn::InterfaceDescription::Property* property = interface->GetProperty(context->propName.c_str());
        if (!property) {
            context->status = ER_BUS_NO_SUCH_PROPERTY;
            return;
        }

        context->status = context->env->busObjectNative->set(interface, property, context->val);
    }

    class GenerateIntrospectionContext : public PluginData::CallbackContext {
      public:
        Env env;
        bool deep;
        size_t indent;
        qcc::String introspection;

        GenerateIntrospectionContext(Env& env, bool deep, size_t indent, qcc::String& introspection) :
            env(env),
            deep(deep),
            indent(indent),
            introspection(introspection) { }
    };

    virtual QStatus GenerateIntrospection(bool deep, size_t indent, qcc::String& introspection) const
    {
        PluginData::Callback callback(env->plugin, _GenerateIntrospection);
        callback->context = new GenerateIntrospectionContext(env, deep, indent, introspection);
        PluginData::DispatchCallback(callback);
        env->busAttachment->EnableConcurrentCallbacks();
        qcc::Event::Wait(callback->context->event);
        introspection = static_cast<GenerateIntrospectionContext*>(callback->context)->introspection;
        return callback->context->status;
    }

    static void _GenerateIntrospection(PluginData::CallbackContext* ctx)
    {
        GenerateIntrospectionContext* context = static_cast<GenerateIntrospectionContext*>(ctx);
        context->status = context->env->busObjectNative->toXML(context->deep, context->indent, context->introspection);
    }
};

class AuthListener : public ajn::AuthListener {
  public:
    class _Env {
      public:
        Plugin plugin;
        BusAttachment busAttachment;
        qcc::String authMechanisms;
        AuthListenerNative* authListenerNative;

        _Env(Plugin& plugin,
             BusAttachment& busAttachment,
             qcc::String& authMechanisms,
             AuthListenerNative* authListenerNative) :
            plugin(plugin),
            busAttachment(busAttachment),
            authMechanisms(authMechanisms),
            authListenerNative(authListenerNative) { }

        ~_Env() {
            delete authListenerNative;
        }
    };

    typedef qcc::ManagedObj<_Env> Env;
    Env env;
    qcc::Event cancelEvent;

    AuthListener(Plugin& plugin,
                 BusAttachment& busAttachment,
                 qcc::String& authMechanisms,
                 AuthListenerNative* authListenerNative) :
        env(plugin, busAttachment, authMechanisms, authListenerNative)
    {
        QCC_DbgTrace(("AuthListener %p", this));
    }

    virtual ~AuthListener() {
        QCC_DbgTrace(("~AuthListener %p", this));
    }

    class RequestCredentialsContext : public PluginData::CallbackContext {
      public:
        Env env;
        qcc::String authMechanism;
        qcc::String peerName;
        uint16_t authCount;
        qcc::String userName;
        uint16_t credMask;
        Credentials credentials;

        RequestCredentialsContext(Env& env,
                                  const char* authMechanism,
                                  const char* peerName,
                                  uint16_t authCount,
                                  const char* userName,
                                  uint16_t credMask,
                                  Credentials& credentials) :
            env(env),
            authMechanism(authMechanism),
            peerName(peerName),
            authCount(authCount),
            userName(userName),
            credMask(credMask),
            credentials(credentials)
        { }
    };

    virtual bool RequestCredentials(const char* authMechanism,
                                    const char* peerName,
                                    uint16_t authCount,
                                    const char* userName,
                                    uint16_t credMask,
                                    Credentials& credentials)
    {
        QCC_DbgTrace(("%s(authMechanism=%s,peerName=%s,authCount=%u,userName=%s,credMask=0x%04x)",
                      __FUNCTION__, authMechanism, peerName, authCount, userName, credMask));
        PluginData::Callback callback(env->plugin, _RequestCredentials);
        callback->context = new RequestCredentialsContext(env, authMechanism, peerName, authCount, userName, credMask, credentials);
        PluginData::DispatchCallback(callback);
        /*
         * Complex processing here to prevent UI thread from deadlocking if _BusAttachmentHost
         * destructor is called.
         *
         * EnablePeerSecurity(0, ...), called from the _BusAttachmentHost destructor, will block
         * until all AuthListener callbacks have returned.  Setting the cancelEvent will unblock any
         * synchronous callback.  Then a little extra coordination is needed to remove the dispatch
         * context so that when the dispatched callback is run it does nothing.
         */
        std::vector<qcc::Event*> check;
        check.push_back(&callback->context->event);
        check.push_back(&cancelEvent);
        std::vector<qcc::Event*> signaled;
        signaled.clear();
        env->busAttachment->EnableConcurrentCallbacks();
        QStatus status = qcc::Event::Wait(check, signaled);
        assert(ER_OK == status);
        if (ER_OK != status) {
            QCC_LogError(status, ("Wait failed"));
        }

        for (std::vector<qcc::Event*>::iterator i = signaled.begin(); i != signaled.end(); ++i) {
            if (*i == &cancelEvent) {
                PluginData::CancelCallback(callback);
                callback->context->status = ER_ALERTED_THREAD;
                break;
            }
        }

        credentials = static_cast<RequestCredentialsContext*>(callback->context)->credentials;
        return (ER_OK == callback->context->status);
    }

    static void _RequestCredentials(PluginData::CallbackContext* ctx)
    {
        RequestCredentialsContext* context = static_cast<RequestCredentialsContext*>(ctx);
        if (context->env->authListenerNative) {
            CredentialsHost credentialsHost(context->env->plugin, context->credentials);
            bool requested = context->env->authListenerNative->onRequest(context->authMechanism, context->peerName, context->authCount, context->userName, context->credMask, credentialsHost);
            context->status = requested ? ER_OK : ER_FAIL;
        } else {
            context->status = ER_FAIL;
        }
    }

    class VerifyCredentialsContext : public PluginData::CallbackContext {
      public:
        Env env;
        qcc::String authMechanism;
        qcc::String peerName;
        Credentials credentials;

        VerifyCredentialsContext(Env& env,
                                 const char* authMechanism,
                                 const char* peerName,
                                 const Credentials& credentials) :
            env(env),
            authMechanism(authMechanism),
            peerName(peerName),
            credentials(credentials)
        { }
    };

    virtual bool VerifyCredentials(const char* authMechanism, const char* peerName, const Credentials& credentials)
    {
        QCC_DbgTrace(("%s(authMechanism=%s,peerName=%s)", __FUNCTION__, authMechanism, peerName));
        PluginData::Callback callback(env->plugin, _VerifyCredentials);
        callback->context = new VerifyCredentialsContext(env, authMechanism, peerName, credentials);
        PluginData::DispatchCallback(callback);
        std::vector<qcc::Event*> check;
        check.push_back(&callback->context->event);
        check.push_back(&cancelEvent);
        std::vector<qcc::Event*> signaled;
        signaled.clear();
        env->busAttachment->EnableConcurrentCallbacks();
        QStatus status = qcc::Event::Wait(check, signaled);
        assert(ER_OK == status);
        if (ER_OK != status) {
            QCC_LogError(status, ("Wait failed"));
        }

        for (std::vector<qcc::Event*>::iterator i = signaled.begin(); i != signaled.end(); ++i) {
            if (*i == &cancelEvent) {
                PluginData::CancelCallback(callback);
                callback->context->status = ER_ALERTED_THREAD;
                break;
            }
        }

        return (ER_OK == callback->context->status);
    }

    static void _VerifyCredentials(PluginData::CallbackContext* ctx)
    {
        VerifyCredentialsContext* context = static_cast<VerifyCredentialsContext*>(ctx);
        if (context->env->authListenerNative) {
            CredentialsHost credentialsHost(context->env->plugin, context->credentials);
            bool verified = context->env->authListenerNative->onVerify(context->authMechanism, context->peerName, credentialsHost);
            context->status = verified ? ER_OK : ER_FAIL;
        } else {
            context->status = ER_FAIL;
        }
    }

    class SecurityViolationContext : public PluginData::CallbackContext {
      public:
        Env env;
        QStatus violation;
        ajn::Message message;

        SecurityViolationContext(Env& env, QStatus violation, const ajn::Message& message) :
            env(env),
            violation(violation),
            message(message) { }
    };

    virtual void SecurityViolation(QStatus status, const ajn::Message& message)
    {
        QCC_DbgTrace(("%s(status=%s,msg=%s)", __FUNCTION__, QCC_StatusText(status), message->ToString().c_str()));
        PluginData::Callback callback(env->plugin, _SecurityViolation);
        callback->context = new SecurityViolationContext(env, status, message);
        PluginData::DispatchCallback(callback);
    }

    static void _SecurityViolation(PluginData::CallbackContext* ctx)
    {
        SecurityViolationContext* context = static_cast<SecurityViolationContext*>(ctx);
        if (context->env->authListenerNative) {
            MessageHost messageHost(context->env->plugin, context->env->busAttachment, context->message);
            context->env->authListenerNative->onSecurityViolation(context->violation, messageHost);
        }
    }

    class AuthenticationCompleteContext : public PluginData::CallbackContext {
      public:
        Env env;
        qcc::String authMechanism;
        qcc::String peerName;
        bool success;

        AuthenticationCompleteContext(Env& env, const char* authMechanism, const char* peerName, bool success) :
            env(env),
            authMechanism(authMechanism),
            peerName(peerName),
            success(success)
        { }
    };

    virtual void AuthenticationComplete(const char* authMechanism, const char* peerName, bool success)
    {
        QCC_DbgTrace(("%s(authMechanism=%s,peerName=%s,success=%d)", __FUNCTION__, authMechanism, peerName, success));
        PluginData::Callback callback(env->plugin, _AuthenticationComplete);
        callback->context = new AuthenticationCompleteContext(env, authMechanism, peerName, success);
        PluginData::DispatchCallback(callback);
    }

    static void _AuthenticationComplete(PluginData::CallbackContext* ctx)
    {
        AuthenticationCompleteContext* context = static_cast<AuthenticationCompleteContext*>(ctx);
        if (context->env->authListenerNative) {
            context->env->authListenerNative->onComplete(context->authMechanism, context->peerName, context->success);
        }
    }
};

_BusAttachmentHost::_BusAttachmentHost(Plugin& plugin) :
    ScriptableObject(plugin, _BusAttachmentInterface::Constants()),
    busAttachment(0),
    authListener(0)
{
    QCC_DbgTrace(("%s %p", __FUNCTION__, this));

    OPERATION("create", &_BusAttachmentHost::create);
    OPERATION("destroy", &_BusAttachmentHost::destroy);
}

_BusAttachmentHost::~_BusAttachmentHost()
{
    QCC_DbgTrace(("%s %p", __FUNCTION__, this));

    stopAndJoin();
}

bool _BusAttachmentHost::getUniqueName(NPVariant* result)
{
    ToDOMString(plugin, (*busAttachment)->GetUniqueName(), *result, TreatEmptyStringAsNull);
    return true;
}

bool _BusAttachmentHost::getGlobalGUIDString(NPVariant* result)
{
    ToDOMString(plugin, (*busAttachment)->GetGlobalGUIDString(), *result);
    return true;
}

bool _BusAttachmentHost::getTimestamp(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    CallbackNative* callbackNative = NULL;
    uint32_t timestamp;

    if (argCount < 1) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[0], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 0 is not an object");
        goto exit;
    }

    timestamp = (*busAttachment)->GetTimestamp();

    CallbackNative::DispatchCallback(plugin, callbackNative, ER_OK, timestamp);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::getPeerSecurityEnabled(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    CallbackNative* callbackNative = NULL;
    bool enabled;

    if (argCount < 1) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[0], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 0 is not an object");
        goto exit;
    }

    enabled = (*busAttachment)->IsPeerSecurityEnabled();

    CallbackNative::DispatchCallback(plugin, callbackNative, ER_OK, enabled);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::create(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status = ER_OK;
    bool typeError = false;
    qcc::String applicationName;
    bool allowRemoteMessages = false;
    CallbackNative* callbackNative = NULL;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    allowRemoteMessages = ToBoolean(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a boolean");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    status = plugin->Origin(applicationName);
    if (ER_OK != status) {
        goto exit;
    }

    QCC_DbgTrace(("applicationName=%s,allowRemoteMessages=%d", applicationName.c_str(), allowRemoteMessages));

    {
        qcc::String name = plugin->ToFilename(applicationName);
        const char* cname = name.c_str();
        busAttachment = new BusAttachment(cname, allowRemoteMessages);
    }

    ATTRIBUTE("globalGUIDString", &_BusAttachmentHost::getGlobalGUIDString, 0);
    ATTRIBUTE("uniqueName", &_BusAttachmentHost::getUniqueName, 0);

    OPERATION("addLogonEntry", &_BusAttachmentHost::addLogonEntry);
    OPERATION("addMatch", &_BusAttachmentHost::addMatch);
    OPERATION("advertiseName", &_BusAttachmentHost::advertiseName);
    OPERATION("bindSessionPort", &_BusAttachmentHost::bindSessionPort);
    OPERATION("cancelAdvertiseName", &_BusAttachmentHost::cancelAdvertiseName);
    OPERATION("cancelFindAdvertisedName", &_BusAttachmentHost::cancelFindAdvertisedName);
    OPERATION("cancelFindAdvertisedNameByTransport", &_BusAttachmentHost::cancelFindAdvertisedNameByTransport);
    OPERATION("clearKeyStore", &_BusAttachmentHost::clearKeyStore);
    OPERATION("clearKeys", &_BusAttachmentHost::clearKeys);
    OPERATION("connect", &_BusAttachmentHost::connect);
    OPERATION("createInterface", &_BusAttachmentHost::createInterface);
    OPERATION("createInterfacesFromXML", &_BusAttachmentHost::createInterfacesFromXML);
    OPERATION("disconnect", &_BusAttachmentHost::disconnect);
    OPERATION("enablePeerSecurity", &_BusAttachmentHost::enablePeerSecurity);
    OPERATION("findAdvertisedName", &_BusAttachmentHost::findAdvertisedName);
    OPERATION("findAdvertisedNameByTransport", &_BusAttachmentHost::findAdvertisedNameByTransport);
    OPERATION("getInterface", &_BusAttachmentHost::getInterface);
    OPERATION("getInterfaces", &_BusAttachmentHost::getInterfaces);
    OPERATION("getKeyExpiration", &_BusAttachmentHost::getKeyExpiration);
    OPERATION("getPeerGUID", &_BusAttachmentHost::getPeerGUID);
    OPERATION("getPeerSecurityEnabled", &_BusAttachmentHost::getPeerSecurityEnabled);
    OPERATION("getProxyBusObject", &_BusAttachmentHost::getProxyBusObject);
    OPERATION("getTimestamp", &_BusAttachmentHost::getTimestamp);
    OPERATION("joinSession", &_BusAttachmentHost::joinSession);
    OPERATION("leaveSession", &_BusAttachmentHost::leaveSession);
    OPERATION("removeSessionMember", &_BusAttachmentHost::removeSessionMember);
    OPERATION("getSessionFd", &_BusAttachmentHost::getSessionFd);
    OPERATION("nameHasOwner", &_BusAttachmentHost::nameHasOwner);
    OPERATION("registerBusListener", &_BusAttachmentHost::registerBusListener);
    OPERATION("registerBusObject", &_BusAttachmentHost::registerBusObject);
    OPERATION("registerSignalHandler", &_BusAttachmentHost::registerSignalHandler);
    OPERATION("releaseName", &_BusAttachmentHost::releaseName);
    OPERATION("reloadKeyStore", &_BusAttachmentHost::reloadKeyStore);
    OPERATION("removeMatch", &_BusAttachmentHost::removeMatch);
    OPERATION("requestName", &_BusAttachmentHost::requestName);
    OPERATION("setDaemonDebug", &_BusAttachmentHost::setDaemonDebug);
    OPERATION("setKeyExpiration", &_BusAttachmentHost::setKeyExpiration);
    OPERATION("setLinkTimeout", &_BusAttachmentHost::setLinkTimeout);
    OPERATION("setSessionListener", &_BusAttachmentHost::setSessionListener);
    OPERATION("unbindSessionPort", &_BusAttachmentHost::unbindSessionPort);
    OPERATION("unregisterBusListener", &_BusAttachmentHost::unregisterBusListener);
    OPERATION("unregisterBusObject", &_BusAttachmentHost::unregisterBusObject);
    OPERATION("unregisterSignalHandler", &_BusAttachmentHost::unregisterSignalHandler);

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::destroy(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("_BusAttachmentHost.%s(%d) %p", __FUNCTION__, argCount, this));

    bool typeError = false;
    CallbackNative* callbackNative = NULL;

    REMOVE_ATTRIBUTE("globalGUIDString");
    REMOVE_ATTRIBUTE("uniqueName");

    REMOVE_OPERATION("addLogonEntry");
    REMOVE_OPERATION("addMatch");
    REMOVE_OPERATION("advertiseName");
    REMOVE_OPERATION("bindSessionPort");
    REMOVE_OPERATION("cancelAdvertiseName");
    REMOVE_OPERATION("cancelFindAdvertisedName");
    REMOVE_OPERATION("cancelFindAdvertisedNameByTransport");
    REMOVE_OPERATION("clearKeyStore");
    REMOVE_OPERATION("clearKeys");
    REMOVE_OPERATION("connect");
    REMOVE_OPERATION("createInterface");
    REMOVE_OPERATION("createInterfacesFromXML");
    REMOVE_OPERATION("disconnect");
    REMOVE_OPERATION("enablePeerSecurity");
    REMOVE_OPERATION("findAdvertisedName");
    REMOVE_OPERATION("findAdvertisedNameByTransport");
    REMOVE_OPERATION("getInterface");
    REMOVE_OPERATION("getInterfaces");
    REMOVE_OPERATION("getKeyExpiration");
    REMOVE_OPERATION("getPeerGUID");
    REMOVE_OPERATION("getPeerSecurityEnabled");
    REMOVE_OPERATION("getProxyBusObject");
    REMOVE_OPERATION("getTimestamp");
    REMOVE_OPERATION("joinSession");
    REMOVE_OPERATION("leaveSession");
    REMOVE_OPERATION("removeSessionMember");
    REMOVE_OPERATION("getSessionFd");
    REMOVE_OPERATION("nameHasOwner");
    REMOVE_OPERATION("registerBusListener");
    REMOVE_OPERATION("registerBusObject");
    REMOVE_OPERATION("registerSignalHandler");
    REMOVE_OPERATION("releaseName");
    REMOVE_OPERATION("reloadKeyStore");
    REMOVE_OPERATION("removeMatch");
    REMOVE_OPERATION("requestName");
    REMOVE_OPERATION("setDaemonDebug");
    REMOVE_OPERATION("setKeyExpiration");
    REMOVE_OPERATION("setLinkTimeout");
    REMOVE_OPERATION("setSessionListener");
    REMOVE_OPERATION("unbindSessionPort");
    REMOVE_OPERATION("unregisterBusListener");
    REMOVE_OPERATION("unregisterBusObject");
    REMOVE_OPERATION("unregisterSignalHandler");

    if (argCount > 0) {
        callbackNative = ToNativeObject<CallbackNative>(plugin, args[0], typeError);
        if (typeError) {
            plugin->RaiseTypeError("argument 0 is not an object");
            goto exit;
        }

        /*
         * destroy() is a no-op.  Under NPAPI, the runtime takes care of
         * garbage collecting this object and under Cordova, the JavaScript
         * side of destroy() explicitly releases the reference (effectively
         * garbage-collecting this object).
         */

        CallbackNative::DispatchCallback(plugin, callbackNative, ER_OK);
        callbackNative = NULL;
    }

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    stopAndJoin();
    return !typeError;
}

bool _BusAttachmentHost::connect(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    QStatus status = ER_OK;
    qcc::String connectSpec;
    CallbackNative* callbackNative = NULL;

    if (argCount < 1) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    if (argCount > 1) {
        connectSpec = ToDOMString(plugin, args[0], typeError);
        if (typeError) {
            plugin->RaiseTypeError("argument 0 is not a string");
            goto exit;
        }
    } else {
#if defined(QCC_OS_GROUP_WINDOWS)
        connectSpec = "tcp:addr=127.0.0.1,port=9956";
#else
        connectSpec = "unix:abstract=alljoyn";
#endif
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[argCount - 1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("connectSpec=%s", connectSpec.c_str()));

    status = ER_OK;
    if (!(*busAttachment)->IsStarted()) {
        status = (*busAttachment)->Start();
    }

    if ((ER_OK == status) && !(*busAttachment)->IsConnected()) {
        status = Connect(plugin, connectSpec.c_str());
        this->connectSpec = connectSpec;
    }

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::createInterface(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    qcc::String name;
    InterfaceDescriptionNative* interfaceDescriptionNative = NULL;
    CallbackNative* callbackNative = NULL;
    bool typeError = false;
    QStatus status = ER_OK;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    interfaceDescriptionNative = ToNativeObject<InterfaceDescriptionNative>(plugin, args[0], typeError);
    if (typeError || !interfaceDescriptionNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 0 is not an object");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    status = InterfaceDescriptionNative::CreateInterface(plugin, *busAttachment, interfaceDescriptionNative);

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    delete interfaceDescriptionNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::createInterfacesFromXML(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    qcc::String xml;
    CallbackNative* callbackNative = NULL;
    bool typeError = false;
    QStatus status = ER_OK;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    xml = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    status = (*busAttachment)->CreateInterfacesFromXml(xml.c_str());

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::registerSignalHandler(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    MessageListenerNative* signalListener = NULL;
    qcc::String signalName;
    qcc::String sourcePath;
    CallbackNative* callbackNative = NULL;
    const ajn::InterfaceDescription::Member* signal;
    QStatus status = ER_OK;
    SignalReceiver* signalReceiver = NULL;

    bool typeError = false;
    if (argCount < 3) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    signalListener = ToNativeObject<MessageListenerNative>(plugin, args[0], typeError);
    if (typeError || !signalListener) {
        typeError = true;
        plugin->RaiseTypeError("argument 0 is not an object");
        goto exit;
    }

    signalName = ToDOMString(plugin, args[1], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 1 is not a string");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[argCount - 1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    if (argCount > 3) {
        sourcePath = ToDOMString(plugin, args[2], typeError);
        if (typeError) {
            plugin->RaiseTypeError("argument 2 is not a string");
            goto exit;
        }
    }

    status = GetSignal(signalName, signal);
    if (ER_OK == status) {
        for (std::list<SignalReceiver*>::iterator it = signalReceivers.begin(); it != signalReceivers.end(); ++it) {
            if ((*((*it)->env->signalListener) == *signalListener) &&
                (*((*it)->env->signal) == *signal) &&
                ((*it)->env->sourcePath == sourcePath)) {
                /* Identical receiver registered, nothing to do. */
                goto exit;
            }
        }

        signalReceiver = new SignalReceiver(plugin, *busAttachment, signalListener, signal, sourcePath);
        signalListener = NULL; /* signalReceiver now owns signalListener */
        status = (*busAttachment)->RegisterSignalHandler(
            signalReceiver, static_cast<ajn::MessageReceiver::SignalHandler>(&SignalReceiver::SignalHandler),
            signal, sourcePath.empty() ? 0 : sourcePath.c_str());
        if (ER_OK != status) {
            goto exit;
        }

        qcc::String rule = MatchRule(signal, sourcePath);
        status = (*busAttachment)->AddMatch(rule.c_str());
        if (ER_OK == status) {
            signalReceivers.push_back(signalReceiver);
            signalReceiver = NULL; /* signalReceivers now owns signalReceiver */
        } else {
            (*busAttachment)->UnregisterSignalHandler(
                signalReceiver, static_cast<ajn::MessageReceiver::SignalHandler>(&SignalReceiver::SignalHandler),
                signal, sourcePath.empty() ? 0 : sourcePath.c_str());
        }
    }

exit:
    if (!typeError && callbackNative) {
        CallbackNative::DispatchCallback(plugin, callbackNative, status);
        callbackNative = NULL;
    }

    delete callbackNative;
    delete signalReceiver;
    delete signalListener;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::unregisterBusObject(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    qcc::String name;
    CallbackNative* callbackNative = NULL;
    std::map<qcc::String, BusObjectListener*>::iterator it;
    QStatus status = ER_OK;

    bool typeError = false;
    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    name = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    it = busObjectListeners.find(name);
    if (it != busObjectListeners.end()) {
        BusObjectListener* busObjectListener = it->second;
        (*busAttachment)->UnregisterBusObject(*busObjectListener->env->busObject);
        busObjectListeners.erase(it);
        delete busObjectListener;
    }

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::disconnect(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CallbackNative* callbackNative = NULL;
    QStatus status = ER_OK;

    bool typeError = false;
    if (argCount < 1) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[0], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 0 is not an object");
        goto exit;
    }

    if ((*busAttachment)->IsStarted() && !(*busAttachment)->IsStopping() && (*busAttachment)->IsConnected()) {
        status = (*busAttachment)->Disconnect(connectSpec.c_str());
    }

    if ((ER_OK == status) && (*busAttachment)->IsStarted()) {
        status = (*busAttachment)->Stop();
    }

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::registerBusObject(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    /*
     * This function actually works with 2 forms of the registerBusObject JS method:
     *     registerBusObject(objPath, busObject, callback)
     *     registerBusObject(objPath, busObject, secure, callback)
     *
     * Note that the 'secure' parameter is optional.  It defaults to false.
     */

    qcc::String name;
    BusObjectNative* busObjectNative = NULL;
    CallbackNative* callbackNative = NULL;
    BusObjectListener* busObjectListener = NULL;
    const NPVariant* arg = args;
    bool secure = false;
    QStatus status = ER_OK;

    bool typeError = false;
    if (argCount < 3) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    name = ToDOMString(plugin, *arg, typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }
    ++arg;

    busObjectNative = ToNativeObject<BusObjectNative>(plugin, *arg, typeError);
    if (typeError || !busObjectNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }
    ++arg;

    /*
     * This method used to only take the object path, object reference, and
     * callback as its only parameters.  It now takes a boolean to indicate if
     * the object should be secure.  This new secure parameter should belong
     * between the object reference and callback parameters.  We'll use the
     * argCount to determine if the secure parameter is specified or not.
     */
    if (argCount > 3) {
        secure = ToBoolean(plugin, *arg, typeError);
        if (typeError) {
            plugin->RaiseTypeError("argument 2 is not a boolean");
            goto exit;
        }
        ++arg;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, *arg, typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError((argCount > 3) ? "argument 3 is not an object" : "argument 2 is not an object");
        goto exit;
    }

    busObjectListener = new BusObjectListener(plugin, *busAttachment, name.c_str(), busObjectNative);
    busObjectNative = NULL; /* busObject now owns busObjectNative */
    status = busObjectListener->AddInterfaceAndMethodHandlers();
    if (ER_OK != status) {
        goto exit;
    }

    status = (*busAttachment)->RegisterBusObject(*busObjectListener->env->busObject, secure);
    if (ER_OK == status) {
        std::pair<qcc::String, BusObjectListener*> element(name, busObjectListener);
        busObjectListeners.insert(element);
        busObjectListener = NULL; /* busObjectListeners now owns busObjectListener */
    }

exit:
    if (!typeError && callbackNative) {
        CallbackNative::DispatchCallback(plugin, callbackNative, status);
        callbackNative = NULL;
    }

    delete callbackNative;
    delete busObjectListener;
    delete busObjectNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::unregisterSignalHandler(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    MessageListenerNative* signalListener = NULL;
    qcc::String signalName;
    qcc::String sourcePath;
    CallbackNative* callbackNative = NULL;
    const ajn::InterfaceDescription::Member* signal;
    QStatus status = ER_OK;

    bool typeError = false;
    if (argCount < 3) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    signalListener = ToNativeObject<MessageListenerNative>(plugin, args[0], typeError);
    if (typeError || !signalListener) {
        typeError = true;
        plugin->RaiseTypeError("argument 0 is not an object");
        goto exit;
    }

    signalName = ToDOMString(plugin, args[1], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 1 is not a string");
        goto exit;
    }

    if (argCount > 2) {
        sourcePath = ToDOMString(plugin, args[2], typeError);
        if (typeError) {
            plugin->RaiseTypeError("argument 2 is not a string");
            goto exit;
        }
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[argCount - 1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 3 is not an object");
        goto exit;
    }

    status = GetSignal(signalName, signal);
    if (ER_OK == status) {
        std::list<SignalReceiver*>::iterator it;
        for (it = signalReceivers.begin(); it != signalReceivers.end(); ++it) {
            if ((*((*it)->env->signalListener) == *signalListener) &&
                (*((*it)->env->signal) == *signal) &&
                ((*it)->env->sourcePath == sourcePath)) {
                break;
            }
        }

        if (it != signalReceivers.end()) {
            status = (*busAttachment)->UnregisterSignalHandler(
                (*it), static_cast<ajn::MessageReceiver::SignalHandler>(&SignalReceiver::SignalHandler),
                signal, sourcePath.empty() ? 0 : sourcePath.c_str());
            if (ER_OK != status) {
                goto exit;
            }

            qcc::String rule = MatchRule(signal, sourcePath);
            status = (*busAttachment)->RemoveMatch(rule.c_str());
            if (ER_OK == status) {
                SignalReceiver* signalReceiver = (*it);
                signalReceivers.erase(it);
                delete signalReceiver;
            } else {
                (*busAttachment)->RegisterSignalHandler(
                    (*it), static_cast<ajn::MessageReceiver::SignalHandler>(&SignalReceiver::SignalHandler),
                    signal, sourcePath.empty() ? 0 : sourcePath.c_str());
            }
        }
    }

exit:
    if (!typeError && callbackNative) {
        CallbackNative::DispatchCallback(plugin, callbackNative, status);
        callbackNative = NULL;
    }

    delete callbackNative;
    delete signalListener;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::registerBusListener(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    BusListenerNative* busListenerNative = NULL;
    CallbackNative* callbackNative = NULL;
    BusListener* busListener = NULL;
    std::list<BusListener*>::iterator it;

    bool typeError = false;
    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    busListenerNative = ToNativeObject<BusListenerNative>(plugin, args[0], typeError);
    if (typeError || !busListenerNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 0 is not an object");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    for (it = busListeners.begin(); it != busListeners.end(); ++it) {
        if (*((*it)->env->busListenerNative) == *busListenerNative) {
            /* Identical listener registered, nothing to do. */
            goto exit;
        }
    }

    busListener = new BusListener(plugin, this, *busAttachment, busListenerNative);
    busListenerNative = NULL; /* busListener now owns busListenerNative */
    (*busAttachment)->RegisterBusListener(*busListener);
    busListeners.push_back(busListener);
    busListener = NULL; /* busListeners now owns busListener */

exit:
    if (!typeError && callbackNative) {
        CallbackNative::DispatchCallback(plugin, callbackNative, ER_OK);
        callbackNative = NULL;
    }

    delete busListener;
    delete callbackNative;
    delete busListenerNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::unregisterBusListener(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    BusListenerNative* busListenerNative = NULL;
    CallbackNative* callbackNative = NULL;
    std::list<BusListener*>::iterator it;

    bool typeError = false;
    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    busListenerNative = ToNativeObject<BusListenerNative>(plugin, args[0], typeError);
    if (typeError || !busListenerNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 0 is not an object");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    for (it = busListeners.begin(); it != busListeners.end(); ++it) {
        if (*((*it)->env->busListenerNative) == *busListenerNative) {
            break;
        }
    }

    if (it != busListeners.end()) {
        BusListener* busListener = (*it);
        (*busAttachment)->UnregisterBusListener(*busListener);
        busListeners.erase(it);
        delete busListener;
    }

    CallbackNative::DispatchCallback(plugin, callbackNative, ER_OK);
    callbackNative = NULL;

exit:
    delete callbackNative;
    delete busListenerNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::requestName(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    QStatus status = ER_OK;
    qcc::String requestedName;
    uint32_t flags = 0;
    CallbackNative* callbackNative = NULL;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    requestedName = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    if (argCount > 2) {
        flags = ToUnsignedLong(plugin, args[1], typeError);
        if (typeError) {
            plugin->RaiseTypeError("argument 1 is not a number");
            goto exit;
        }
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[argCount - 1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 2 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("requestedName=%s,flags=0x%x", requestedName.c_str(), flags));

    status = (*busAttachment)->RequestName(requestedName.c_str(), flags);

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::releaseName(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    QStatus status = ER_OK;
    qcc::String name;
    CallbackNative* callbackNative = NULL;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    name = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("name=%s", name.c_str()));

    status = (*busAttachment)->ReleaseName(name.c_str());

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::addMatch(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    QStatus status = ER_OK;
    qcc::String rule;
    CallbackNative* callbackNative = NULL;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    rule = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("rule=%s", rule.c_str()));

    status = (*busAttachment)->AddMatch(rule.c_str());

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::removeMatch(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    QStatus status = ER_OK;
    qcc::String rule;
    CallbackNative* callbackNative = NULL;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    rule = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("rule=%s", rule.c_str()));

    status = (*busAttachment)->RemoveMatch(rule.c_str());

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::advertiseName(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    QStatus status = ER_OK;
    qcc::String name;
    uint16_t transports;
    CallbackNative* callbackNative = NULL;

    if (argCount < 3) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    name = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    transports = ToUnsignedShort(plugin, args[1], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 1 is not a number");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[2], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 2 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("name=%s,transports=0x%x", name.c_str(), transports));

    status = (*busAttachment)->AdvertiseName(name.c_str(), transports);

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::cancelAdvertiseName(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    QStatus status = ER_OK;
    qcc::String name;
    uint16_t transports;
    CallbackNative* callbackNative = NULL;

    if (argCount < 3) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    name = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    transports = ToUnsignedShort(plugin, args[1], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 1 is not a number");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[2], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 2 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("name=%s,transports=0x%x", name.c_str(), transports));

    status = (*busAttachment)->CancelAdvertiseName(name.c_str(), transports);

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::findAdvertisedName(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    QStatus status = ER_OK;
    qcc::String namePrefix;
    CallbackNative* callbackNative = NULL;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    namePrefix = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("namePrefix=%s", namePrefix.c_str()));

    status = (*busAttachment)->FindAdvertisedName(namePrefix.c_str());

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::findAdvertisedNameByTransport(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    QStatus status = ER_OK;
    qcc::String namePrefix;
    uint16_t transports;
    CallbackNative* callbackNative = NULL;

    if (argCount < 3) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    namePrefix = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    transports = ToUnsignedShort(plugin, args[1], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 1 is not a number");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[2], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 2 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("namePrefix=%s,transports=0x%x", namePrefix.c_str(), transports));

    status = (*busAttachment)->FindAdvertisedNameByTransport(namePrefix.c_str(), transports);

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::cancelFindAdvertisedName(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    QStatus status = ER_OK;
    qcc::String namePrefix;
    CallbackNative* callbackNative = NULL;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    namePrefix = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("namePrefix=%s", namePrefix.c_str()));

    status = (*busAttachment)->CancelFindAdvertisedName(namePrefix.c_str());

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::cancelFindAdvertisedNameByTransport(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    QStatus status = ER_OK;
    qcc::String namePrefix;
    uint16_t transports;
    CallbackNative* callbackNative = NULL;

    if (argCount < 3) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    namePrefix = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    transports = ToUnsignedShort(plugin, args[1], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 1 is not a number");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[2], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 2 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("namePrefix=%s,transports=0x%x", namePrefix.c_str(), transports));

    status = (*busAttachment)->CancelFindAdvertisedNameByTransport(namePrefix.c_str(), transports);

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::bindSessionPort(const NPVariant* args, uint32_t argCount, NPVariant* npresult)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    ajn::SessionPort sessionPort = ajn::SESSION_PORT_ANY;
    ajn::SessionOpts sessionOpts;
    AcceptSessionJoinerListenerNative* acceptSessionListenerNative = NULL;
    SessionJoinedListenerNative* sessionJoinedListenerNative = NULL;
    SessionLostListenerNative* sessionLostListenerNative = NULL;
    SessionMemberAddedListenerNative* sessionMemberAddedListenerNative = NULL;
    SessionMemberRemovedListenerNative* sessionMemberRemovedListenerNative = NULL;
    SessionListener::Env sessionListenerEnv(plugin, *busAttachment);

    CallbackNative* callbackNative = NULL;
    SessionPortListener* sessionPortListener = NULL;
    QStatus status = ER_OK;
    NPVariant result;

    /*
     * Pull out the parameters from the native object.
     */
    bool typeError = false;
    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    if (!NPVARIANT_IS_OBJECT(args[0])) {
        typeError = true;
        plugin->RaiseTypeError("argument 0 is not an object");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    VOID_TO_NPVARIANT(result);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[0]), NPN_GetStringIdentifier("port"), &result);
    if (!NPVARIANT_IS_VOID(result)) {
        sessionPort = ToUnsignedShort(plugin, result, typeError);
    }

    NPN_ReleaseVariantValue(&result);
    if (typeError) {
        plugin->RaiseTypeError("'port' is not a number");
        goto exit;
    }

    VOID_TO_NPVARIANT(result);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[0]), NPN_GetStringIdentifier("traffic"), &result);
    if (!NPVARIANT_IS_VOID(result)) {
        sessionOpts.traffic = (ajn::SessionOpts::TrafficType) ToOctet(plugin, result, typeError);
    }

    NPN_ReleaseVariantValue(&result);
    if (typeError) {
        plugin->RaiseTypeError("'traffic' is not a number");
        goto exit;
    }

    VOID_TO_NPVARIANT(result);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[0]), NPN_GetStringIdentifier("isMultipoint"), &result);
    if (!NPVARIANT_IS_VOID(result)) {
        sessionOpts.isMultipoint = ToBoolean(plugin, result, typeError);
    }

    NPN_ReleaseVariantValue(&result);
    if (typeError) {
        plugin->RaiseTypeError("'isMultipoint' is not a boolean");
        goto exit;
    }

    VOID_TO_NPVARIANT(result);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[0]), NPN_GetStringIdentifier("proximity"), &result);
    if (!NPVARIANT_IS_VOID(result)) {
        sessionOpts.proximity = ToOctet(plugin, result, typeError);
    }

    NPN_ReleaseVariantValue(&result);
    if (typeError) {
        plugin->RaiseTypeError("'proximity' is not a number");
        goto exit;
    }

    VOID_TO_NPVARIANT(result);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[0]), NPN_GetStringIdentifier("transports"), &result);
    if (!NPVARIANT_IS_VOID(result)) {
        sessionOpts.transports = ToUnsignedShort(plugin, result, typeError);
    }

    NPN_ReleaseVariantValue(&result);
    if (typeError) {
        plugin->RaiseTypeError("'transports' is not a number");
        goto exit;
    }

    VOID_TO_NPVARIANT(result);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[0]), NPN_GetStringIdentifier("onAccept"), &result);
    if (!NPVARIANT_IS_VOID(result)) {
        acceptSessionListenerNative = ToNativeObject<AcceptSessionJoinerListenerNative>(plugin, result, typeError);
    }

    NPN_ReleaseVariantValue(&result);
    if (typeError) {
        plugin->RaiseTypeError("'onAccept' is not an object");
        goto exit;
    }

    VOID_TO_NPVARIANT(result);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[0]), NPN_GetStringIdentifier("onJoined"), &result);
    if (!NPVARIANT_IS_VOID(result)) {
        sessionJoinedListenerNative = ToNativeObject<SessionJoinedListenerNative>(plugin, result, typeError);
    }

    NPN_ReleaseVariantValue(&result);
    if (typeError) {
        plugin->RaiseTypeError("'onJoined' is not an object");
        goto exit;
    }

    VOID_TO_NPVARIANT(result);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[0]), NPN_GetStringIdentifier("onLost"), &result);
    if (!NPVARIANT_IS_VOID(result)) {
        sessionLostListenerNative = ToNativeObject<SessionLostListenerNative>(plugin, result, typeError);
    }

    NPN_ReleaseVariantValue(&result);
    if (typeError) {
        plugin->RaiseTypeError("'onLost' is not an object");
        goto exit;
    }

    VOID_TO_NPVARIANT(result);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[0]), NPN_GetStringIdentifier("onMemberAdded"), &result);
    if (!NPVARIANT_IS_VOID(result)) {
        sessionMemberAddedListenerNative = ToNativeObject<SessionMemberAddedListenerNative>(plugin, result, typeError);
    }

    NPN_ReleaseVariantValue(&result);
    if (typeError) {
        plugin->RaiseTypeError("'onMemberAdded' is not an object");
        goto exit;
    }

    VOID_TO_NPVARIANT(result);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[0]), NPN_GetStringIdentifier("onMemberRemoved"), &result);
    if (!NPVARIANT_IS_VOID(result)) {
        sessionMemberRemovedListenerNative = ToNativeObject<SessionMemberRemovedListenerNative>(plugin, result, typeError);
    }

    NPN_ReleaseVariantValue(&result);
    if (typeError) {
        plugin->RaiseTypeError("'onMemberRemoved' is not an object");
        goto exit;
    }

    QCC_DbgTrace(("sessionPort=%u", sessionPort));


    if (sessionLostListenerNative || sessionMemberAddedListenerNative || sessionMemberRemovedListenerNative) {
        sessionListenerEnv = SessionListener::Env(plugin, *busAttachment, sessionLostListenerNative, sessionMemberAddedListenerNative, sessionMemberRemovedListenerNative);
        /* sessionListener now owns session*ListenerNative */
        sessionLostListenerNative = NULL;
        sessionMemberAddedListenerNative = NULL;
        sessionMemberRemovedListenerNative = NULL;
    }

    sessionPortListener = new SessionPortListener(plugin, this, *busAttachment, acceptSessionListenerNative, sessionJoinedListenerNative, sessionListenerEnv);
    acceptSessionListenerNative = NULL; /* sessionPortListener now owns acceptSessionListenerNative */
    sessionJoinedListenerNative = NULL; /* sessionPortListener now owns sessionJoinedListenerNative */

    status = (*busAttachment)->BindSessionPort(sessionPort, sessionOpts, *sessionPortListener);
    if (ER_OK == status) {
        std::pair<ajn::SessionPort, SessionPortListener*> element(sessionPort, sessionPortListener);
        sessionPortListeners.insert(element);
        sessionPortListener = NULL; /* sessionPortListeners now owns sessionPort */
    }

exit:
    if (!typeError && callbackNative) {
        CallbackNative::DispatchCallback(plugin, callbackNative, status, sessionPort);
        callbackNative = NULL;
    }

    delete callbackNative;
    delete acceptSessionListenerNative;
    delete sessionJoinedListenerNative;
    delete sessionLostListenerNative;
    delete sessionMemberAddedListenerNative;
    delete sessionMemberRemovedListenerNative;
    delete sessionPortListener;
    VOID_TO_NPVARIANT(*npresult);
    return !typeError;
}


bool _BusAttachmentHost::unbindSessionPort(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    QStatus status = ER_OK;
    ajn::SessionPort sessionPort;
    CallbackNative* callbackNative = NULL;
    std::map<ajn::SessionPort, SessionPortListener*>::iterator it;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    sessionPort = ToUnsignedShort(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a number");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("sessionPort=%u", sessionPort));

    it = sessionPortListeners.find(sessionPort);
    if (it != sessionPortListeners.end()) {
        SessionPortListener* listener = it->second;
        QStatus status = listener->cancelEvent.SetEvent();
        assert(ER_OK == status);
        if (ER_OK != status) {
            QCC_LogError(status, ("SetEvent failed")); /* Small chance of deadlock if this occurs. */
        }
    }

    status = (*busAttachment)->UnbindSessionPort(sessionPort);
    if (ER_OK == status) {
        if (it != sessionPortListeners.end()) {
            SessionPortListener* listener = it->second;
            sessionPortListeners.erase(it);
            delete listener;
        }
    }

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::setSessionListener(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    ajn::SessionId id;
    SessionLostListenerNative* sessionLostListenerNative = NULL;
    SessionMemberAddedListenerNative* sessionMemberAddedListenerNative = NULL;
    SessionMemberRemovedListenerNative* sessionMemberRemovedListenerNative = NULL;
    CallbackNative* callbackNative = NULL;
    SessionListener* sessionListener = NULL;
    QStatus status = ER_OK;

    NPVariant variant;
    bool typeError = false;
    if (argCount < 3) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    id = ToUnsignedLong(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a number");
        goto exit;
    }

    if (NPVARIANT_IS_OBJECT(args[1])) {
        VOID_TO_NPVARIANT(variant);
        NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[1]), NPN_GetStringIdentifier("onLost"), &variant);
        sessionLostListenerNative = ToNativeObject<SessionLostListenerNative>(plugin, variant, typeError);
        NPN_ReleaseVariantValue(&variant);
        if (typeError) {
            plugin->RaiseTypeError("'onLost' is not an object");
            goto exit;
        }

        VOID_TO_NPVARIANT(variant);
        NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[1]), NPN_GetStringIdentifier("onMemberAdded"), &variant);
        sessionMemberAddedListenerNative = ToNativeObject<SessionMemberAddedListenerNative>(plugin, variant, typeError);
        NPN_ReleaseVariantValue(&variant);
        if (typeError) {
            plugin->RaiseTypeError("'onMemberAdded' is not an object");
            goto exit;
        }

        VOID_TO_NPVARIANT(variant);
        NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[1]), NPN_GetStringIdentifier("onMemberRemoved"), &variant);
        sessionMemberRemovedListenerNative = ToNativeObject<SessionMemberRemovedListenerNative>(plugin, variant, typeError);
        NPN_ReleaseVariantValue(&variant);
        if (typeError) {
            plugin->RaiseTypeError("'onMemberRemoved' is not an object");
            goto exit;
        }
    } else if (!NPVARIANT_IS_NULL(args[1])) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object or null");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[2], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 2 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("id=%u", id));

    if (sessionLostListenerNative || sessionMemberAddedListenerNative || sessionMemberRemovedListenerNative) {
        sessionListener = new SessionListener(plugin, *busAttachment, sessionLostListenerNative, sessionMemberAddedListenerNative, sessionMemberRemovedListenerNative);
        /* sessionListener now owns session*ListenerNative */
        sessionLostListenerNative = NULL;
        sessionMemberAddedListenerNative = NULL;
        sessionMemberRemovedListenerNative = NULL;
    }

    status = (*busAttachment)->SetSessionListener(id, sessionListener);
    if (ER_OK == status) {
        /* Overwrite existing listener. */
        std::map<ajn::SessionId, SessionListener*>::iterator it = sessionListeners.find(id);
        if (it != sessionListeners.end()) {
            SessionListener* listener = it->second;
            sessionListeners.erase(it);
            delete listener;
        }

        if (sessionListener) {
            std::pair<ajn::SessionId, SessionListener*> element(id, sessionListener);
            sessionListeners.insert(element);
            sessionListener = NULL; /* sessionListeners now owns sessionListener */
        }
    }

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    delete sessionListener;
    delete sessionLostListenerNative;
    delete sessionMemberAddedListenerNative;
    delete sessionMemberRemovedListenerNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}


bool _BusAttachmentHost::joinSession(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    BusAttachmentHost busAttachmentHost = BusAttachmentHost::wrap(this);
    qcc::String sessionHost;
    ajn::SessionPort sessionPort = ajn::SESSION_PORT_ANY;
    ajn::SessionOpts sessionOpts;
    SessionLostListenerNative* sessionLostListenerNative = NULL;
    SessionMemberAddedListenerNative* sessionMemberAddedListenerNative = NULL;
    SessionMemberRemovedListenerNative* sessionMemberRemovedListenerNative = NULL;
    CallbackNative* callbackNative = NULL;
    SessionListener* sessionListener = NULL;
    QStatus status = ER_OK;
    JoinSessionAsyncCB* callback = NULL;

    /*
     * Pull out the parameters from the native object.
     */
    NPVariant variant;
    bool typeError = false;
    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }
    /*
     * Mandatory parameters
     */
    if (!NPVARIANT_IS_OBJECT(args[0])) {
        typeError = true;
        plugin->RaiseTypeError("argument 0 is not an object");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    VOID_TO_NPVARIANT(variant);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[0]), NPN_GetStringIdentifier("host"), &variant);
    if (!NPVARIANT_IS_VOID(variant)) {
        sessionHost = ToDOMString(plugin, variant, typeError);
    }

    NPN_ReleaseVariantValue(&variant);
    if (typeError || sessionHost.empty()) {
        typeError = true;
        plugin->RaiseTypeError("property 'host' of argument 2 is undefined");
        goto exit;
    }

    VOID_TO_NPVARIANT(variant);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[0]), NPN_GetStringIdentifier("port"), &variant);
    if (!NPVARIANT_IS_VOID(variant)) {
        sessionPort = ToUnsignedShort(plugin, variant, typeError);
    }

    NPN_ReleaseVariantValue(&variant);
    if (typeError || (ajn::SESSION_PORT_ANY == sessionPort)) {
        typeError = true;
        plugin->RaiseTypeError("property 'port' of argument 2 is undefined or invalid");
        goto exit;
    }
    /*
     * Optional parameters
     */
    VOID_TO_NPVARIANT(variant);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[0]), NPN_GetStringIdentifier("traffic"), &variant);
    if (!NPVARIANT_IS_VOID(variant)) {
        sessionOpts.traffic = (ajn::SessionOpts::TrafficType) ToOctet(plugin, variant, typeError);
    }

    NPN_ReleaseVariantValue(&variant);
    if (typeError) {
        plugin->RaiseTypeError("'traffic' is not a number");
        goto exit;
    }

    VOID_TO_NPVARIANT(variant);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[0]), NPN_GetStringIdentifier("isMultipoint"), &variant);
    if (!NPVARIANT_IS_VOID(variant)) {
        sessionOpts.isMultipoint = ToBoolean(plugin, variant, typeError);
    }

    NPN_ReleaseVariantValue(&variant);
    if (typeError) {
        plugin->RaiseTypeError("'isMultipoint' is not a boolean");
        goto exit;
    }

    VOID_TO_NPVARIANT(variant);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[0]), NPN_GetStringIdentifier("proximity"), &variant);
    if (!NPVARIANT_IS_VOID(variant)) {
        sessionOpts.proximity = ToOctet(plugin, variant, typeError);
    }

    NPN_ReleaseVariantValue(&variant);
    if (typeError) {
        plugin->RaiseTypeError("'proximity' is not a number");
        goto exit;
    }

    VOID_TO_NPVARIANT(variant);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[0]), NPN_GetStringIdentifier("transports"), &variant);
    if (!NPVARIANT_IS_VOID(variant)) {
        sessionOpts.transports = ToUnsignedShort(plugin, variant, typeError);
    }

    NPN_ReleaseVariantValue(&variant);
    if (typeError) {
        plugin->RaiseTypeError("'transports' is not a number");
        goto exit;
    }

    VOID_TO_NPVARIANT(variant);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[0]), NPN_GetStringIdentifier("onLost"), &variant);
    if (!NPVARIANT_IS_VOID(variant)) {
        sessionLostListenerNative = ToNativeObject<SessionLostListenerNative>(plugin, variant, typeError);
    }

    NPN_ReleaseVariantValue(&variant);
    if (typeError) {
        plugin->RaiseTypeError("'onLost' is not an object");
        goto exit;
    }

    VOID_TO_NPVARIANT(variant);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[0]), NPN_GetStringIdentifier("onMemberAdded"), &variant);
    if (!NPVARIANT_IS_VOID(variant)) {
        sessionMemberAddedListenerNative = ToNativeObject<SessionMemberAddedListenerNative>(plugin, variant, typeError);
    }

    NPN_ReleaseVariantValue(&variant);
    if (typeError) {
        plugin->RaiseTypeError("'onMemberAdded' is not an object");
        goto exit;
    }

    VOID_TO_NPVARIANT(variant);
    NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(args[0]), NPN_GetStringIdentifier("onMemberRemoved"), &variant);
    if (!NPVARIANT_IS_VOID(variant)) {
        sessionMemberRemovedListenerNative = ToNativeObject<SessionMemberRemovedListenerNative>(plugin, variant, typeError);
    }

    NPN_ReleaseVariantValue(&variant);
    if (typeError) {
        plugin->RaiseTypeError("'onMemberRemoved' is not an object");
        goto exit;
    }

    QCC_DbgTrace(("sessionHost=%s,sessionPort=%u,sessionOpts={traffic=%x,isMultipoint=%d,proximity=%x,transports=%x}", sessionHost.c_str(), sessionPort,
                  sessionOpts.traffic, sessionOpts.isMultipoint, sessionOpts.proximity, sessionOpts.transports));

    sessionListener = new SessionListener(plugin, *busAttachment, sessionLostListenerNative, sessionMemberAddedListenerNative, sessionMemberRemovedListenerNative);
    /* sessionListener now owns session*ListenerNative */
    sessionLostListenerNative = NULL;
    sessionMemberAddedListenerNative = NULL;
    sessionMemberRemovedListenerNative = NULL;

    callback = new JoinSessionAsyncCB(plugin, busAttachmentHost, *busAttachment, callbackNative, sessionListener);
    callbackNative = NULL; /* callback now owns callbackNative */
    sessionListener = NULL; /* callback now owns sessionListener */

    status = (*busAttachment)->JoinSessionAsync(sessionHost.c_str(), sessionPort, callback->env->sessionListener, sessionOpts, callback);
    if (ER_OK == status) {
        callback = NULL; /* alljoyn owns callback */
    } else {
        callback->env->status = status;
    }

exit:
    delete callbackNative;
    delete callback;
    delete sessionListener;
    delete sessionLostListenerNative;
    delete sessionMemberAddedListenerNative;
    delete sessionMemberRemovedListenerNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::leaveSession(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    QStatus status = ER_OK;
    ajn::SessionId id;
    CallbackNative* callbackNative = NULL;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    id = ToUnsignedLong(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 1 is not a number");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("id=%u", id));

    status = (*busAttachment)->LeaveSession(id);

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::removeSessionMember(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    QStatus status = ER_OK;
    qcc::String name;
    ajn::SessionId id;
    CallbackNative* callbackNative = NULL;

    if (argCount < 3) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    id = ToUnsignedLong(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a number");
        goto exit;
    }

    name = ToDOMString(plugin, args[1], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 1 is not a string");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[2], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 2 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("id = %u  name=%s", id, name.c_str()));

    status = (*busAttachment)->RemoveSessionMember(id, name.c_str());

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::getSessionFd(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    QStatus status = ER_OK;
    ajn::SessionId id;
    CallbackNative* callbackNative = NULL;
    qcc::SocketFd sockFd;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    id = ToUnsignedLong(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a number");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("id=%u", id));

    status = (*busAttachment)->GetSessionFd(id, sockFd);
    if (ER_OK == status) {
        SocketFdHost socketFdHost(plugin, sockFd);
        CallbackNative::DispatchCallback(plugin, callbackNative, status, socketFdHost);
    } else {
        CallbackNative::DispatchCallback(plugin, callbackNative, status);
    }
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::setLinkTimeout(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    QStatus status = ER_OK;
    ajn::SessionId id;
    uint32_t linkTimeout;
    CallbackNative* callbackNative = NULL;

    if (argCount < 3) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    id = ToUnsignedLong(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 1 is not a number");
        goto exit;
    }

    linkTimeout = ToUnsignedLong(plugin, args[1], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 2 is not a number");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[2], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 2 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("id=%u,linkTimeout=%u", id, linkTimeout));

    status = (*busAttachment)->SetLinkTimeout(id, linkTimeout);

    CallbackNative::DispatchCallback(plugin, callbackNative, status, linkTimeout);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::nameHasOwner(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    QStatus status = ER_OK;
    qcc::String name;
    CallbackNative* callbackNative = NULL;
    bool has;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    name = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("name=%s", name.c_str()));

    status = (*busAttachment)->NameHasOwner(name.c_str(), has);

    CallbackNative::DispatchCallback(plugin, callbackNative, status, has);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::setDaemonDebug(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    QStatus status = ER_OK;
    qcc::String module;
    uint32_t level;
    CallbackNative* callbackNative = NULL;

    if (argCount < 3) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    module = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    level = ToUnsignedLong(plugin, args[1], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 1 is not a number");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[2], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 2 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("module=%s,level=%u", module.c_str(), level));

    status = (*busAttachment)->SetDaemonDebug(module.c_str(), level);

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::enablePeerSecurity(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    qcc::String authMechanisms;
    AuthListenerNative* authListenerNative = NULL;
    CallbackNative* callbackNative = NULL;

    QStatus status = ER_OK;
    bool typeError = false;
    const char* keyStoreFileName = NULL;
    qcc::String fileName;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    authMechanisms = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    if (argCount > 2) {
        authListenerNative = ToNativeObject<AuthListenerNative>(plugin, args[1], typeError);
        if (typeError) {
            typeError = true;
            plugin->RaiseTypeError("argument 1 is not an object");
            goto exit;
        }
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[argCount - 1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 2 is not an object");
        goto exit;
    }

    if (authListener) {
        status = ER_BUS_ALREADY_LISTENING;
        goto exit;
    }

    status = (*busAttachment)->Start();
    if ((ER_OK != status) && (ER_BUS_BUS_ALREADY_STARTED != status)) {
        goto exit;
    }

    authListener = new AuthListener(plugin, *busAttachment, authMechanisms, authListenerNative);
    authListenerNative = NULL; /* authListener now owns authListenerNative */
    fileName = plugin->KeyStoreFileName();
    if (!fileName.empty()) {
        keyStoreFileName = fileName.c_str();
    }

    status = (*busAttachment)->EnablePeerSecurity(authListener->env->authMechanisms.c_str(), authListener, keyStoreFileName, true);
    if (ER_OK != status) {
        delete authListener;
        authListener = NULL;
    }

exit:
    if (!typeError && callbackNative) {
        CallbackNative::DispatchCallback(plugin, callbackNative, status);
        callbackNative = NULL;
    }

    delete callbackNative;
    delete authListenerNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::reloadKeyStore(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CallbackNative* callbackNative = NULL;
    QStatus status = ER_OK;
    bool typeError = false;

    if (argCount < 1) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[0], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 0 is not an object");
        goto exit;
    }

    status = (*busAttachment)->ReloadKeyStore();

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::clearKeyStore(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CallbackNative* callbackNative = NULL;
    bool typeError = false;

    if (argCount < 1) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[0], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 0 is not an object");
        goto exit;
    }

    (*busAttachment)->ClearKeyStore();

    CallbackNative::DispatchCallback(plugin, callbackNative, ER_OK);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::clearKeys(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QStatus status = ER_OK;
    bool typeError = false;
    qcc::String guid;
    CallbackNative* callbackNative = NULL;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    guid = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    status = (*busAttachment)->ClearKeys(guid);

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::getInterface(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    qcc::String name;
    CallbackNative* callbackNative = NULL;
    QStatus status = ER_OK;
    bool typeError = false;
    InterfaceDescriptionNative* interfaceDescriptionNative = NULL;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    name = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    interfaceDescriptionNative = InterfaceDescriptionNative::GetInterface(plugin, *busAttachment, name);

    CallbackNative::DispatchCallback(plugin, callbackNative, status, interfaceDescriptionNative);
    interfaceDescriptionNative = NULL;
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::getInterfaces(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CallbackNative* callbackNative = NULL;
    size_t numIfaces;
    const ajn::InterfaceDescription** ifaces = NULL;
    InterfaceDescriptionNative** descs = NULL;
    QStatus status = ER_OK;
    bool typeError = false;

    if (argCount < 1) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[0], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 0 is not an object");
        goto exit;
    }

    numIfaces = (*busAttachment)->GetInterfaces();
    ifaces = new const ajn::InterfaceDescription *[numIfaces];
    (*busAttachment)->GetInterfaces(ifaces, numIfaces);
    descs = new InterfaceDescriptionNative *[numIfaces];
    for (uint32_t i = 0; i < numIfaces; ++i) {
        descs[i] = InterfaceDescriptionNative::GetInterface(plugin, *busAttachment, ifaces[i]->GetName());
    }

    CallbackNative::DispatchCallback(plugin, callbackNative, status, descs, numIfaces);
    descs = NULL;
    callbackNative = NULL;

exit:
    delete callbackNative;
    delete[] descs;
    delete[] ifaces;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::getKeyExpiration(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status = ER_OK;
    bool typeError = false;
    qcc::String guid;
    CallbackNative* callbackNative = NULL;
    uint32_t timeout;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    guid = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("guid=%s", guid.c_str()));

    status = (*busAttachment)->GetKeyExpiration(guid, timeout);

    CallbackNative::DispatchCallback(plugin, callbackNative, status, timeout);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::setKeyExpiration(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status = ER_OK;
    bool typeError = false;
    qcc::String guid;
    uint32_t timeout;
    CallbackNative* callbackNative = NULL;

    if (argCount < 3) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    guid = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    timeout = ToUnsignedLong(plugin, args[1], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[2], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 2 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("guid=%s,timeout=%u", guid.c_str(), timeout));

    status = (*busAttachment)->SetKeyExpiration(guid, timeout);

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::addLogonEntry(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    bool typeError = false;
    QStatus status = ER_OK;
    qcc::String authMechanism;
    qcc::String userName;
    qcc::String password;
    CallbackNative* callbackNative = NULL;

    if (argCount < 4) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    authMechanism = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    userName = ToDOMString(plugin, args[1], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 1 is not a string");
        goto exit;
    }

    password = ToDOMString(plugin, args[2], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 2 is not a string");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[3], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 3 is not an object");
        goto exit;
    }

    status = (*busAttachment)->AddLogonEntry(authMechanism.c_str(), userName.c_str(), NPVARIANT_IS_NULL(args[2]) ? 0 : password.c_str());

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _BusAttachmentHost::getPeerGUID(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    QStatus status = ER_OK;
    qcc::String name;
    CallbackNative* callbackNative = NULL;
    qcc::String guid;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    name = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    status = (*busAttachment)->GetPeerGUID(name.c_str(), guid);

    CallbackNative::DispatchCallback(plugin, callbackNative, status, guid);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

QStatus _BusAttachmentHost::GetSignal(const qcc::String& name, const ajn::InterfaceDescription::Member*& signal)
{
    size_t dot = name.find_last_of('.');
    if (qcc::String::npos == dot) {
        QCC_LogError(ER_BUS_BAD_MEMBER_NAME, ("Can't find '.' in '%s'", name.c_str()));
        return ER_BUS_BAD_MEMBER_NAME;
    }

    qcc::String interfaceName = name.substr(0, dot);
    qcc::String signalName = name.substr(dot + 1);
    QCC_DbgTrace(("interfaceName=%s,signalName=%s", interfaceName.c_str(), signalName.c_str()));

    const ajn::InterfaceDescription* interface = (*busAttachment)->GetInterface(interfaceName.c_str());
    if (!interface) {
        QCC_LogError(ER_BUS_UNKNOWN_INTERFACE, ("Don't know about interface '%s'", interfaceName.c_str()));
        return ER_BUS_UNKNOWN_INTERFACE;
    }

    signal = interface->GetMember(signalName.c_str());
    if (!signal) {
        QCC_LogError(ER_BUS_INTERFACE_NO_SUCH_MEMBER, ("Don't know about signal '%s' in interface '%s'", signalName.c_str(), interfaceName.c_str()));
        return ER_BUS_INTERFACE_NO_SUCH_MEMBER;
    }

    return ER_OK;
}

qcc::String _BusAttachmentHost::MatchRule(const ajn::InterfaceDescription::Member* signal, const qcc::String& sourcePath)
{
    qcc::String rule = "type='signal',member='" + signal->name + "',interface='" + signal->iface->GetName() + "'";
    if (!sourcePath.empty()) {
        rule += ",path='" + sourcePath + "'";
    }

    return rule;
}

bool _BusAttachmentHost::getProxyBusObject(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    /*
     * This function actually works with 2 forms of the getProxyBusObject JS method:
     *     getProxyBusObject(objPath, callback)
     *     getProxyBusObject(objPath, secure, callback)
     *
     * Note that the 'secure' parameter is optional.  It defaults to false.
     */

    qcc::String name;
    qcc::String keyname;
    CallbackNative* callbackNative = NULL;
    std::map<qcc::String, ProxyBusObjectHost>::iterator it;
    const NPVariant* arg = args;
    bool secure = false;
    QStatus status = ER_OK;
    bool typeError = false;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    name = ToDOMString(plugin, *arg, typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }
    ++arg;

    /*
     * This method used to only take the object path, and callback as its only
     * parameters.  It now takes a boolean to indicate if the proxy object
     * should be secure.  This new secure parameter should belong between the
     * object path and callback parameters.  We'll use the argCount to
     * determine if the secure parameter is specified or not.
     */
    if (argCount > 2) {
        secure = ToBoolean(plugin, *arg, typeError);
        if (typeError) {
            plugin->RaiseTypeError("argument 1 is not a boolean");
            goto exit;
        }
        ++arg;
    }

    /*
     * Tweak the object path to include a distinction between a secure object
     * and an insecure object and save that as a key name used for looking up
     * existing ProxyBusObjectHost instances.  This will handle the case of a
     * poorly written JS app that calls getProxyBusObject with secure=false
     * then later calls getProxyBusObject with secure=true.  The JS app will
     * work the exact same way as a C++ app that instantiates ProxyBusObject
     * in the same manner.
     */
    keyname = name + (secure ? "s" : "n");

    callbackNative = ToNativeObject<CallbackNative>(plugin, *arg, typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError((argCount > 2) ? "argument 2 is not an object" : "argument 1 is not an object");
        goto exit;
    }

    QCC_DbgTrace(("name=%s", name.c_str()));

    if (proxyBusObjects.find(keyname) == proxyBusObjects.end()) {
        qcc::String serviceName, path;
        std::map<qcc::String, qcc::String> argMap;
        ParseName(name, serviceName, path, argMap);
        const char* cserviceName = serviceName.c_str();
        const char* cpath = path.c_str();
        ajn::SessionId sessionId = strtoul(argMap["sessionId"].c_str(), 0, 0);
        std::pair<qcc::String, ProxyBusObjectHost> element(keyname, ProxyBusObjectHost(plugin, *busAttachment, cserviceName, cpath, sessionId));
        proxyBusObjects.insert(element);
    }

    it = proxyBusObjects.find(keyname);
    CallbackNative::DispatchCallback(plugin, callbackNative, status, it->second);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

void _BusAttachmentHost::ParseName(const qcc::String& name, qcc::String& serviceName, qcc::String& path, std::map<qcc::String, qcc::String>& argMap)
{
    size_t slash = name.find_first_of('/');
    size_t colon = name.find_last_of(':');
    serviceName = name.substr(0, slash);
    path = name.substr(slash, colon - slash);
    qcc::String args = name.substr(colon);
    ajn::Transport::ParseArguments("", args.c_str(), argMap); /* Ignore any errors since args are optional */
}

void _BusAttachmentHost::stopAndJoin() {
    QCC_DbgTrace(("%s %p", __FUNCTION__, this));

    if (!busAttachment) {
        return;
    }

    /*
     * Ensure that all callbacks are complete before we start deleting things.
     */
    (*busAttachment)->Stop();
    for (std::map<ajn::SessionPort, SessionPortListener*>::iterator it = sessionPortListeners.begin(); it != sessionPortListeners.end(); ++it) {
        SessionPortListener* sessionPortListener = it->second;
        QStatus status = sessionPortListener->cancelEvent.SetEvent();
        assert(ER_OK == status);
        if (ER_OK != status) {
            QCC_LogError(status, ("SetEvent failed")); /* Small chance of deadlock if this occurs. */
        }
    }

    if (authListener) {
        QStatus status = authListener->cancelEvent.SetEvent();
        assert(ER_OK == status);
        if (ER_OK != status) {
            QCC_LogError(status, ("SetEvent failed")); /* Small chance of deadlock if this occurs. */
        }
    }
    (*busAttachment)->Join();

    for (std::map<qcc::String, BusObjectListener*>::iterator it = busObjectListeners.begin(); it != busObjectListeners.end(); ++it) {
        BusObjectListener* busObjectListener = it->second;
        (*busAttachment)->UnregisterBusObject(*busObjectListener->env->busObject);
        delete busObjectListener;
    }

    for (std::map<ajn::SessionId, SessionListener*>::iterator it = sessionListeners.begin(); it != sessionListeners.end(); ++it) {
        SessionListener* sessionListener = it->second;
        (*busAttachment)->SetSessionListener(it->first, 0);
        delete sessionListener;
    }

    for (std::map<ajn::SessionPort, SessionPortListener*>::iterator it = sessionPortListeners.begin(); it != sessionPortListeners.end(); ++it) {
        SessionPortListener* sessionPortListener = it->second;
        (*busAttachment)->UnbindSessionPort(it->first);
        delete sessionPortListener;
    }

    for (std::list<BusListener*>::iterator it = busListeners.begin(); it != busListeners.end(); ++it) {
        BusListener* busListener = (*it);
        (*busAttachment)->UnregisterBusListener(*busListener);
        delete busListener;
    }

    for (std::list<SignalReceiver*>::iterator it = signalReceivers.begin(); it != signalReceivers.end(); ++it) {
        SignalReceiver* signalReceiver = (*it);
        qcc::String rule = MatchRule(signalReceiver->env->signal, signalReceiver->env->sourcePath);
        (*busAttachment)->RemoveMatch(rule.c_str());
        (*busAttachment)->UnregisterSignalHandler(
            signalReceiver, static_cast<ajn::MessageReceiver::SignalHandler>(&SignalReceiver::SignalHandler),
            signalReceiver->env->signal, signalReceiver->env->sourcePath.empty() ? 0 : signalReceiver->env->sourcePath.c_str());
        delete signalReceiver;
    }

    if (authListener) {
        (*busAttachment)->EnablePeerSecurity(0, 0, 0, true);
        delete authListener;
    }

    proxyBusObjects.clear();
    delete busAttachment;
    busAttachment = NULL;
}

