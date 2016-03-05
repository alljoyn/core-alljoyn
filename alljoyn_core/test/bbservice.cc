/**
 * @file
 * Sample implementation of an AllJoyn service.
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
#include <qcc/platform.h>

#include <signal.h>
#include <stdio.h>
#include <vector>

#include <qcc/Debug.h>
#include <qcc/Log.h>
#include <qcc/Environ.h>
#include <qcc/Mutex.h>
#include <qcc/String.h>
#include <qcc/Thread.h>
#include <qcc/time.h>
#include <qcc/Util.h>

#include <alljoyn/AboutObj.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/Init.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/version.h>

#include <alljoyn/Status.h>


#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;
using namespace ajn;

namespace org {
namespace alljoyn {
namespace alljoyn_test {
const char* InterfaceName = "org.alljoyn.alljoyn_test";
const char* DefaultWellKnownName = "org.alljoyn.alljoyn_test";
const char* ObjectPath = "/org/alljoyn/alljoyn_test";
const SessionPort SessionPort = 24;    /**< Well-known session port value for bbclient/bbservice */
namespace values {
const char* InterfaceName = "org.alljoyn.alljoyn_test.values";
}
}
}
}

/* Forward declaration */
class MyBusListener;

/* Static top level globals */
static BusAttachment* g_msgBus = NULL;
static MyBusListener* g_myBusListener = NULL;
static String g_wellKnownName = ::org::alljoyn::alljoyn_test::DefaultWellKnownName;
static bool g_echo_signal = false;
static uint32_t g_keyExpiration = 0xFFFFFFFF;
static bool g_cancelAdvertise = false;
static bool g_ping_back = false;
static bool g_disableConcurrency = false;
static bool g_use_delayed_ping_with_sleep = false;
static String g_testAboutApplicationName = "bbservice";
static bool g_useAboutFeatureDiscovery = false;
static AboutData g_aboutData("en");

static const char* g_alternatePSK = NULL;
static const char* g_defaultPSK = "faaa0af3dd3f1e0379da046a3ab6ca44";

static volatile sig_atomic_t g_interrupt = false;

static void CDECL_CALL SigIntHandler(int sig)
{
    QCC_UNUSED(sig);
    g_interrupt = true;
}

/* Same keys and certs as alljoyn_core/unit_test/AuthListenerECDHETest.cc */
static const char ecdsaPrivateKeyPEM[] = {
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MDECAQEEICCRJMbxSiWUqj4Zs7jFQRXDJdBRPWX6fIVqE1BaXd08oAoGCCqGSM49\n"
    "AwEH\n"
    "-----END EC PRIVATE KEY-----\n"
};

static const char ecdsaCertChainX509PEM[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBuDCCAV2gAwIBAgIHMTAxMDEwMTAKBggqhkjOPQQDAjBCMRUwEwYDVQQLDAxv\n"
    "cmdhbml6YXRpb24xKTAnBgNVBAMMIDgxM2FkZDFmMWNiOTljZTk2ZmY5MTVmNTVk\n"
    "MzQ4MjA2MB4XDTE1MDcyMjIxMDYxNFoXDTE2MDcyMTIxMDYxNFowQjEVMBMGA1UE\n"
    "CwwMb3JnYW5pemF0aW9uMSkwJwYDVQQDDCAzOWIxZGNmMjBmZDJlNTNiZGYzMDU3\n"
    "NzMzMjBlY2RjMzBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABGJ/9F4xHn3Klw7z\n"
    "6LREmHJgzu8yJ4i09b4EWX6a5MgUpQoGKJcjWgYGWb86bzbciMCFpmKzfZ42Hg+k\n"
    "BJs2ZWajPjA8MAwGA1UdEwQFMAMBAf8wFQYDVR0lBA4wDAYKKwYBBAGC3nwBATAV\n"
    "BgNVHSMEDjAMoAoECELxjRK/fVhaMAoGCCqGSM49BAMCA0kAMEYCIQDixoulcO7S\n"
    "df6Iz6lvt2CDy0sjt/bfuYVW3GeMLNK1LAIhALNklms9SP8ZmTkhCKdpC+/fuwn0\n"
    "+7RX8CMop11eWCih\n"
    "-----END CERTIFICATE-----\n"
};

class MyAuthListener : public AuthListener {

    QStatus RequestCredentialsAsync(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, void* context)
    {
        Credentials creds;

        printf("RequestCredentials for authenticating %s using mechanism %s\n", authPeer, authMechanism);

        /* Random delay */
        qcc::Sleep(10 * qcc::Rand8());

        /* Enable concurrent callbacks since some of the calls below could block */
        g_msgBus->EnableConcurrentCallbacks();

        if (g_keyExpiration != 0xFFFFFFFF) {
            creds.SetExpiration(g_keyExpiration);
        }

        if (strcmp(authMechanism, "ALLJOYN_SRP_KEYX") == 0) {
            if (credMask & AuthListener::CRED_PASSWORD) {
                if (authCount == 1) {
                    creds.SetPassword("yyyyyy");
                } else {
                    creds.SetPassword("123456");
                }
                printf("AuthListener returning fixed pin \"%s\" for %s\n", creds.GetPassword().c_str(), authMechanism);
            }
            return RequestCredentialsResponse(context, true, creds);
        }

        if (strcmp(authMechanism, "ALLJOYN_SRP_LOGON") == 0) {
            if (!userId) {
                return RequestCredentialsResponse(context, false, creds);
            }
            printf("Attemping to logon user %s\n", userId);
            if (strcmp(userId, "happy") == 0) {
                if (credMask & AuthListener::CRED_PASSWORD) {
                    creds.SetPassword("123456");
                    return RequestCredentialsResponse(context, true, creds);
                }
            }
            if (strcmp(userId, "sneezy") == 0) {
                if (credMask & AuthListener::CRED_PASSWORD) {
                    creds.SetPassword("123456");
                    return RequestCredentialsResponse(context, true, creds);
                }
            }
            /*
             * Allow 3 logon attempts.
             */
            if (authCount <= 3) {
                return RequestCredentialsResponse(context, true, creds);
            }
        }

        if (strcmp(authMechanism, "ALLJOYN_ECDHE_NULL") == 0) {
            printf("AuthListener::RequestCredentials for key exchange %s\n", authMechanism);
            return RequestCredentialsResponse(context, true, creds);
        }
        if (strcmp(authMechanism, "ALLJOYN_ECDHE_PSK") == 0) {
            if ((credMask& AuthListener::CRED_USER_NAME) == AuthListener::CRED_USER_NAME) {
                printf("AuthListener::RequestCredentials for key exchange %s received psk ID %s\n", authMechanism, creds.GetUserName().c_str());
            }
            /*
             * In this example, the pre shared secret is a hard coded string.
             * Pre-shared keys should be 128 bits long, and generated with a
             * cryptographically secure random number generator.
             * However, if a psk was supplied on the commandline,
             * this is used instead of the default PSK.
             */
            const char* csPSK;
            if (NULL != g_alternatePSK) {
                csPSK = g_alternatePSK;
            } else {
                csPSK = g_defaultPSK;
            }
            String psk(csPSK);
            creds.SetPassword(psk);
            return RequestCredentialsResponse(context, true, creds);
        }
        if (strcmp(authMechanism, "ALLJOYN_ECDHE_ECDSA") == 0) {
            if ((credMask& AuthListener::CRED_PRIVATE_KEY) == AuthListener::CRED_PRIVATE_KEY) {
                String pk(ecdsaPrivateKeyPEM, strlen(ecdsaPrivateKeyPEM));
                creds.SetPrivateKey(pk);
                printf("AuthListener::RequestCredentials for key exchange %s sends DSA private key %s\n", authMechanism, pk.c_str());
            }
            if ((credMask& AuthListener::CRED_CERT_CHAIN) == AuthListener::CRED_CERT_CHAIN) {
                String cert(ecdsaCertChainX509PEM, strlen(ecdsaCertChainX509PEM));
                creds.SetCertChain(cert);
                printf("AuthListener::RequestCredentials for key exchange %s sends DSA public cert %s\n", authMechanism, cert.c_str());
            }
            return RequestCredentialsResponse(context, true, creds);
        }
        return RequestCredentialsResponse(context, false, creds);
    }

    QStatus VerifyCredentialsAsync(const char* authMechanism, const char* authPeer, const Credentials& creds, void* context) {
        QCC_UNUSED(authPeer);
        if (strcmp(authMechanism, "ALLJOYN_ECDHE_ECDSA") == 0) {
            if (creds.IsSet(AuthListener::CRED_CERT_CHAIN)) {
                printf("Verify\n%s\n", creds.GetCertChain().c_str());
                return VerifyCredentialsResponse(context, true);
            }
        }
        return VerifyCredentialsResponse(context, false);
    }

    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {
        QCC_UNUSED(authPeer);

        printf("Authentication %s %s\n", authMechanism, success ? "succesful" : "failed");
    }

    void SecurityViolation(QStatus status, const Message& msg) {
        QCC_UNUSED(msg);
        printf("Security violation %s\n", QCC_StatusText(status));
    }

};

class MyBusListener : public SessionPortListener, public SessionListener {

  public:
    MyBusListener(BusAttachment& bus, const SessionOpts& opts) : bus(bus), opts(opts) { }

    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& sessionOpts)
    {
        /* Reject join attetmpt to unknwown session port */
        if (sessionPort != ::org::alljoyn::alljoyn_test::SessionPort) {
            QCC_SyncPrintf("Received JoinSession request for non-bound port\n");
            return false;
        }

        if (this->opts.IsCompatible(sessionOpts)) {
            /* Allow the join attempt */
            QCC_SyncPrintf("Accepting JoinSession request from %s\n", joiner);
            return true;
        } else {
            /* Reject incompatible transports */
            QCC_SyncPrintf("Rejecting joiner %s with incompatible session options\n", joiner);
            return false;
        }
    }

    void SessionJoined(SessionPort sessionPort, SessionId sessionId, const char* joiner)
    {
        QCC_UNUSED(sessionPort);

        QCC_SyncPrintf("Session Established: joiner=%s, sessionId=%08x\n", joiner, sessionId);

        /* Enable concurrent callbacks since some of the calls below could block */
        g_msgBus->EnableConcurrentCallbacks();

        QStatus status = bus.SetSessionListener(sessionId, this);
        if (status != ER_OK) {
            QCC_LogError(status, ("SetSessionListener failed"));
            return;
        }

        /* Set the link timeout */
        uint32_t timeout = 10;
        status = bus.SetLinkTimeout(sessionId, timeout);
        if (status == ER_OK) {
            QCC_SyncPrintf("Link timeout was successfully set to %d\n", timeout);
        } else {
            QCC_LogError(status, ("SetLinkTimeout failed"));
        }

        /* cancel advertisment */
        if (g_cancelAdvertise) {
            status = bus.CancelAdvertiseName(g_wellKnownName.c_str(), opts.transports);
            if (status != ER_OK) {
                QCC_LogError(status, ("CancelAdvertiseName(%s) failed", g_wellKnownName.c_str()));
            }
        }
    }

    void SessionLost(SessionId sessionId, SessionLostReason reason) {
        QCC_SyncPrintf("SessionLost(%08x) was called. Reason = %u.\n", sessionId, reason);

        /* Enable concurrent callbacks since some of the calls below could block */
        g_msgBus->EnableConcurrentCallbacks();

        if (g_cancelAdvertise) {
            QStatus status = bus.AdvertiseName(g_wellKnownName.c_str(), opts.transports);
            if (status != ER_OK) {
                QCC_LogError(status, ("AdvertiseName(%s) failed", g_wellKnownName.c_str()));
            }
        }
    }

  private:
    /* Private assigment operator - does nothing */
    MyBusListener operator=(const MyBusListener&);

    BusAttachment& bus;
    SessionOpts opts;
};

class LocalTestObject : public BusObject {

    class DelayedResponse : public Thread, public ThreadListener {
        struct DelayedResponseInfo {
            Message msg;
            MsgArg* argList;
            size_t argCount;
            DelayedResponseInfo(Message& msg, MsgArg* argList, size_t argCount) :
                msg(msg), argList(argList), argCount(argCount)
            { }
        };

      public:
        DelayedResponse(LocalTestObject& lto) : Thread("DelayedResponse"), lto(lto) { }

        static void AddResponse(LocalTestObject& lto, uint32_t delay, Message& msg, MsgArg* args, size_t argCount)
        {
            Timespec<MonotonicTime> future;
            GetTimeNow(&future);
            future += delay;
            DelayedResponseInfo* respInfo = new DelayedResponseInfo(msg, args, argCount);
            delayedResponseLock.Lock(MUTEX_CONTEXT);
            delayedResponses.insert(pair<uint64_t, DelayedResponseInfo*>(future.GetMillis(), respInfo));
            delayedResponseLock.Unlock(MUTEX_CONTEXT);

            delayedResponseThreadLock.Lock(MUTEX_CONTEXT);
            if (!thread) {
                thread = new DelayedResponse(lto);
                thread->Start(NULL, thread);
            }
            delayedResponseThreadLock.Unlock(MUTEX_CONTEXT);

        }

        void ThreadExit(Thread* exitingThread)
        {
            delayedResponseThreadLock.Lock(MUTEX_CONTEXT);
            this->thread = NULL;
            delete exitingThread;
            delayedResponseThreadLock.Unlock(MUTEX_CONTEXT);
        }

      protected:
        ThreadReturn STDCALL Run(void* arg)
        {
            QCC_UNUSED(arg);

            delayedResponseLock.Lock(MUTEX_CONTEXT);
            bool done = delayedResponses.empty();
            delayedResponseLock.Unlock(MUTEX_CONTEXT);

            while (!done) {
                delayedResponseLock.Lock(MUTEX_CONTEXT);
                Timespec<MonotonicTime> now;
                GetTimeNow(&now);
                uint64_t nowms = now.GetMillis();
                multimap<uint64_t, DelayedResponseInfo*>::iterator it = delayedResponses.begin();
                uint64_t nextms = it->first;
                delayedResponseLock.Unlock(MUTEX_CONTEXT);

                if (nextms > nowms) {
                    uint64_t delay = nextms - nowms;
                    if (delay >= (1ULL << 32)) {
                        delay = (1ULL << 32) - 1;
                    }
                    qcc::Sleep(static_cast<uint32_t>(delay));
                }

                delayedResponseLock.Lock(MUTEX_CONTEXT);
                GetTimeNow(&now);
                nowms = now.GetMillis();
                it = delayedResponses.begin();
                while ((it != delayedResponses.end()) && (nextms <= nowms)) {
                    Message msg = it->second->msg;
                    MsgArg* args = it->second->argList;
                    size_t argCount = it->second->argCount;
                    delete it->second;
                    delayedResponses.erase(it);
                    delayedResponseLock.Unlock(MUTEX_CONTEXT);
                    QStatus status = lto.WrappedReply(msg, args, argCount);
                    if (ER_OK != status) {
                        QCC_LogError(status, ("Error sending delayed response"));
                    }
                    delete [] args;
                    delayedResponseLock.Lock(MUTEX_CONTEXT);
                    it = delayedResponses.begin();
                }
                if (it == delayedResponses.end()) {
                    done = true;
                }
                delayedResponseLock.Unlock(MUTEX_CONTEXT);
            }

            return static_cast<ThreadReturn>(0);
        }

      private:
        LocalTestObject& lto;
        static DelayedResponse* thread;
        static multimap<uint64_t, DelayedResponseInfo*> delayedResponses;
        static Mutex delayedResponseLock;
        static Mutex delayedResponseThreadLock;
    };


  public:

    LocalTestObject(BusAttachment& bus, const char* path, unsigned long reportInterval, const SessionOpts& opts) :
        BusObject(path),
        reportInterval(reportInterval),
        prop_str_val("hello world"),
        prop_ro_str("I cannot be written"),
        prop_int_val(100),
        opts(opts),
        aboutObj(bus)
    {
        QStatus status = ER_OK;

        /* Add the test interface to this object */
        const InterfaceDescription* regTestIntf = bus.GetInterface(::org::alljoyn::alljoyn_test::InterfaceName);
        QCC_ASSERT(regTestIntf);
        if (g_useAboutFeatureDiscovery) {
            AddInterface(*regTestIntf, ANNOUNCED);
        } else {
            AddInterface(*regTestIntf);
        }
        /* Add the values interface to this object */
        const InterfaceDescription* valuesIntf = bus.GetInterface(::org::alljoyn::alljoyn_test::values::InterfaceName);
        QCC_ASSERT(valuesIntf);
        if (g_useAboutFeatureDiscovery) {
            AddInterface(*valuesIntf, ANNOUNCED);
        } else {
            AddInterface(*valuesIntf);
        }

        /* Register the signal handler with the bus */
        const InterfaceDescription::Member* member = regTestIntf->GetMember("my_signal");
        QCC_ASSERT(member);
        status = bus.RegisterSignalHandler(this,
                                           static_cast<MessageReceiver::SignalHandler>(&LocalTestObject::SignalHandler),
                                           member,
                                           NULL);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to register signal handler"));
        }

        /* Register the method handlers with the object */
        const MethodEntry methodEntries[] = {
            { regTestIntf->GetMember("my_ping"), static_cast<MessageReceiver::MethodHandler>(&LocalTestObject::Ping) },
            { regTestIntf->GetMember("time_ping"), static_cast<MessageReceiver::MethodHandler>(&LocalTestObject::TimePing) }
        };
        status = AddMethodHandlers(methodEntries, ArraySize(methodEntries));
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to register method handlers for LocalTestObject"));
        }

        if (!g_use_delayed_ping_with_sleep) {
            status = AddMethodHandler(regTestIntf->GetMember("delayed_ping"), static_cast<MessageReceiver::MethodHandler>(&LocalTestObject::DelayedPing));
        } else {
            status = AddMethodHandler(regTestIntf->GetMember("delayed_ping"), static_cast<MessageReceiver::MethodHandler>(&LocalTestObject::DelayedPingWithSleep));
        }
    }

    void ObjectRegistered(void)
    {
        QStatus status;
        Message reply(*g_msgBus);

        /* Enable concurrent callbacks since some of the calls below could block */
        g_msgBus->EnableConcurrentCallbacks();

        /*
         * Bind a well-known session port for incoming client connections. This must be done before
         * we request or start advertising a well known name because a client can try to connect
         * immediately and with a bundled daemon this can happen very quickly.
         */
        SessionPort sessionPort = ::org::alljoyn::alljoyn_test::SessionPort;
        status = g_msgBus->BindSessionPort(sessionPort, opts, *g_myBusListener);
        if (status != ER_OK) {
            QCC_LogError(status, ("BindSessionPort failed"));
        }
        /* Add rule for receiving test signals */
        status = g_msgBus->AddMatch("type='signal',interface='org.alljoyn.alljoyn_test',member='my_signal'");
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to register Match rule for 'org.alljoyn.alljoyn_test.my_signal'"));
        }
        if (g_useAboutFeatureDiscovery) {
            //AppId is a 128bit uuid
            uint8_t appId[] = { 0x01, 0xB3, 0xBA, 0x14,
                                0x1E, 0x82, 0x11, 0xE4,
                                0x86, 0x51, 0xD1, 0x56,
                                0x1D, 0x5D, 0x46, 0xB0 };
            g_aboutData.SetAppId(appId, 16);
            g_aboutData.SetDeviceName("DeviceName");
            //DeviceId is a string encoded 128bit UUID
            g_aboutData.SetDeviceId("1273b650-49bc-11e4-916c-0800200c9a66");
            g_aboutData.SetAppName(g_testAboutApplicationName.c_str());
            g_aboutData.SetManufacturer("AllSeen Alliance");
            g_aboutData.SetModelNumber("");
            g_aboutData.SetDescription("bbservice is a test application used to verify AllJoyn functionality");
            // software version of bbservice is the same as the AllJoyn version
            g_aboutData.SetSoftwareVersion(ajn::GetVersion());

            aboutObj.Announce(sessionPort, g_aboutData);
        } else {
            /* Request a well-known name */
            status = g_msgBus->RequestName(g_wellKnownName.c_str(), DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE);
            if (status != ER_OK) {
                QCC_LogError(status, ("RequestName(%s) failed.", g_wellKnownName.c_str()));
                return;
            }
            /* Begin Advertising the well-known name */
            status = g_msgBus->AdvertiseName(g_wellKnownName.c_str(), opts.transports);
            if (ER_OK != status) {
                QCC_LogError(status, ("Sending org.alljoyn.Bus.Advertise failed"));
                return;
            }
        }
    }

    void SignalHandler(const InterfaceDescription::Member* member,
                       const char* sourcePath,
                       Message& msg)
    {
        /* Enable concurrent signal handling */
        if (!g_disableConcurrency) {
            g_msgBus->EnableConcurrentCallbacks();
        }

        if ((IncrementAndFetch(&rxCounts[sourcePath]) % reportInterval) == 0) {
            QCC_SyncPrintf("RxSignal: %s - %u\n", sourcePath, rxCounts[sourcePath]);
            if (msg->IsEncrypted()) {
                QCC_SyncPrintf("Authenticated using %s\n", msg->GetAuthMechanism().c_str());
            }
        }
        if (g_echo_signal) {
            MsgArg arg("a{ys}", 0, NULL);
            uint8_t flags = 0;
            QStatus status = Signal(msg->GetSender(), msg->GetSessionId(), *member, &arg, 1, 0, flags);
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed to send Signal"));
            }
        }
        if (g_ping_back) {
            MsgArg pingArg("s", "pingback");
            const InterfaceDescription* ifc = g_msgBus->GetInterface(::org::alljoyn::alljoyn_test::InterfaceName);
            if (ifc) {
                const InterfaceDescription::Member* pingMethod = ifc->GetMember("my_ping");

                ProxyBusObject remoteObj;
                remoteObj = ProxyBusObject(*g_msgBus, msg->GetSender(), ::org::alljoyn::alljoyn_test::ObjectPath, msg->GetSessionId());
                remoteObj.AddInterface(*ifc);
                /*
                 * Make a fire-and-forget method call. If the signal was encrypted encrypt the ping
                 */
                QStatus status = remoteObj.MethodCall(*pingMethod, &pingArg, 1, msg->IsEncrypted() ? ALLJOYN_FLAG_ENCRYPTED : 0);
                if (status != ER_OK) {
                    QCC_LogError(status, ("MethodCall on %s.%s failed", ::org::alljoyn::alljoyn_test::InterfaceName, pingMethod->name.c_str()));
                }
            }
        }
    }

    void Ping(const InterfaceDescription::Member* member, Message& msg)
    {
        QCC_UNUSED(member);

        char* value = NULL;
        /* Reply with same string that was sent to us */
        const MsgArg* arg((msg->GetArg(0)));
        arg->Get("s", &value);
        printf("Pinged with: %s\n", value);
        if (msg->IsEncrypted()) {
            printf("Authenticated using %s\n", msg->GetAuthMechanism().c_str());
        }
        QStatus status = MethodReply(msg, arg, 1);
        if (ER_OK != status) {
            QCC_LogError(status, ("Ping: Error sending reply"));
        }
    }

    void DelayedPingWithSleep(const InterfaceDescription::Member* member, Message& msg)
    {
        QCC_UNUSED(member);

        /* Enable concurrent callbacks since some of the calls take a long time to execute */
        g_msgBus->EnableConcurrentCallbacks();

        uint32_t delay(msg->GetArg(1)->v_uint32);
        const char* value(msg->GetArg(0)->v_string.str);
        MsgArg* args = new MsgArg[1];
        args[0].Set("s", value);
        printf("Pinged (response delayed %ums) with: \"%s\"\n", delay, value);
        if (msg->IsEncrypted()) {
            printf("Authenticated using %s\n", msg->GetAuthMechanism().c_str());
        }
        qcc::Sleep(delay);
        QStatus status = MethodReply(msg, args, 1);
        if (ER_OK != status) {
            QCC_LogError(status, ("DelayedPing: Error sending reply"));
        }
    }

    void DelayedPing(const InterfaceDescription::Member* member, Message& msg)
    {
        QCC_UNUSED(member);

        /* Enable concurrent callbacks since some of the calls take a long time to execute */
        g_msgBus->EnableConcurrentCallbacks();

        /* Reply with same string that was sent to us */
        uint32_t delay(msg->GetArg(1)->v_uint32);
        const char* value(msg->GetArg(0)->v_string.str);
        MsgArg* args = new MsgArg[1];
        args[0].Set("s", value);
        printf("Pinged (response delayed %ums) with: \"%s\"\n", delay, value);
        if (msg->IsEncrypted()) {
            printf("Authenticated using %s\n", msg->GetAuthMechanism().c_str());
        }
        DelayedResponse::AddResponse(*this, delay, msg, args, 1);
    }

    void TimePing(const InterfaceDescription::Member* member, Message& msg)
    {
        QCC_UNUSED(member);
        /* Reply with same data that was sent to us */
        MsgArg args[] = { (*(msg->GetArg(0))), (*(msg->GetArg(1))) };
        QStatus status = MethodReply(msg, args, 2);
        if (ER_OK != status) {
            QCC_LogError(status, ("Ping: Error sending reply"));
        }
    }

    QStatus Get(const char* ifcName, const char* propName, MsgArg& val)
    {
        QCC_UNUSED(ifcName);
        QStatus status = ER_OK;
        if (0 == strcmp("int_val", propName)) {
            // val.Set("i", prop_int_val);
            val.typeId = ALLJOYN_INT32;
            val.v_int32 = prop_int_val;
        } else if (0 == strcmp("str_val", propName)) {
            // val.Set("s", prop_str_val.c_str());
            val.typeId = ALLJOYN_STRING;
            val.v_string.str = prop_str_val.c_str();
            val.v_string.len = prop_str_val.size();
        } else if (0 == strcmp("ro_str", propName)) {
            // val.Set("s", prop_ro_str_val.c_str());
            val.typeId = ALLJOYN_STRING;
            val.v_string.str = prop_ro_str.c_str();
            val.v_string.len = prop_ro_str.size();
        } else {
            status = ER_BUS_NO_SUCH_PROPERTY;
        }
        return status;
    }

    QStatus Set(const char* ifcName, const char* propName, MsgArg& val)
    {
        QCC_UNUSED(ifcName);

        QStatus status = ER_OK;
        if ((0 == strcmp("int_val", propName)) && (val.typeId == ALLJOYN_INT32)) {
            prop_int_val = val.v_int32;
        } else if ((0 == strcmp("str_val", propName)) && (val.typeId == ALLJOYN_STRING)) {
            prop_str_val = val.v_string.str;
        } else if (0 == strcmp("ro_str", propName)) {
            status = ER_BUS_PROPERTY_ACCESS_DENIED;
        } else {
            status = ER_BUS_NO_SUCH_PROPERTY;
        }
        return status;
    }


    QStatus WrappedReply(Message& msg, MsgArg* args, size_t argCount)
    {
        return MethodReply(msg, args, argCount);
    }

  private:

    map<qcc::String, int32_t> rxCounts;

    unsigned long reportInterval;

    qcc::String prop_str_val;
    qcc::String prop_ro_str;
    int32_t prop_int_val;
    SessionOpts opts;
    AboutObj aboutObj;
};


LocalTestObject::DelayedResponse* LocalTestObject::DelayedResponse::thread = NULL;
multimap<uint64_t, LocalTestObject::DelayedResponse::DelayedResponseInfo*> LocalTestObject::DelayedResponse::delayedResponses;
Mutex LocalTestObject::DelayedResponse::delayedResponseLock;
Mutex LocalTestObject::DelayedResponse::delayedResponseThreadLock;



static void usage(void)
{
    printf("Usage: bbservice [-h <name>] [-m] [-e] [-i #] [-n <name>] [-b] [-t] [-l]\n\n");
    printf("Options:\n");
    printf("   -h                    = Print this help message\n");
    printf("   -?                    = Print this help message\n");
    printf("   -k <key store name>   = The key store file name\n");
    printf("   -kx #                 = Authentication key expiration (seconds)\n");
    printf("   -m                    = Session is a multi-point session\n");
    printf("   -e                    = Echo received signals back to sender\n");
    printf("   -i #                  = Signal report interval (number of signals rx per update; default = 1000)\n");
    printf("   -n <well-known name>  = Well-known name to advertise\n");
    printf("   -t                    = Advertise over TCP (enables selective advertising)\n");
    printf("   -l                    = Advertise locally (enables selective advertising)\n");
    printf("   -u                    = Advertise over UDP-based ARDP (enables selective advertising)\n");
    printf("   -a                    = Cancel advertising while servicing a single client (causes rediscovery between iterations)\n");
    printf("   -p                    = Respond to an incoming signal by pinging back to the sender\n");
    printf("   -sn                   = Interface security is not applicable\n");
    printf("   -sr                   = Interface security is required\n");
    printf("   -so                   = Enable object security\n");
    printf("   -con #                = Specify concurrent threads\n");
    printf("   -dcon                 = Disable concurrency\n");
    printf("   -dpws                 = Use DelayedPingWithSleep as methodhandler instead of DelayedPing\n");
    printf("   -about [name]         = use the about feature for discovery. (optional override default application name.)\n");
    printf("   -runtime #            = runtime of the program in ms. After this time has passed, the application will exit automatically. \n");
    printf("   -psk <psk>            = Use the supplied pre-shared key instead of the built in one.\n");
    printf("                           For interop with tests in version <= 14.12 pass '123456'.\n");
    printf("\n");
}

/** Main entry point */
int CDECL_CALL main(int argc, char** argv)
{
    if (AllJoynInit() != ER_OK) {
        return 1;
    }
#ifdef ROUTER
    if (AllJoynRouterInit() != ER_OK) {
        AllJoynShutdown();
        return 1;
    }
#endif
    QStatus status = ER_OK;
    InterfaceSecurityPolicy secPolicy = AJ_IFC_SECURITY_INHERIT;
    bool objSecure = false;
    unsigned long reportInterval = 1000;
    const char* keyStore = NULL;
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_NONE);
    unsigned long concurrencyLevel = 4;
    unsigned long run_time = 0;

    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    /* Parse command line args */
    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp("-h", argv[i]) || 0 == strcmp("-?", argv[i])) {
            usage();
            exit(0);
        } else if (0 == strcmp("-p", argv[i])) {
            if (g_echo_signal) {
                printf("options -e and -p are mutually exclusive\n");
                usage();
                exit(1);
            }
            g_ping_back = true;
        } else if (0 == strcmp("-e", argv[i])) {
            if (g_ping_back) {
                printf("options -p and -e are mutually exclusive\n");
                usage();
                exit(1);
            }
            g_echo_signal = true;
        } else if (0 == strcmp("-i", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                reportInterval = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-n", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                g_wellKnownName = argv[i];
            }
        } else if (0 == strcmp("-k", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                keyStore = argv[i];
            }
        } else if (0 == strcmp("-kx", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                g_keyExpiration = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-m", argv[i])) {
            opts.isMultipoint = true;
        } else if (0 == strcmp("-t", argv[i])) {
            opts.transports |= TRANSPORT_TCP;
        } else if (0 == strcmp("-l", argv[i])) {
            opts.transports |= TRANSPORT_LOCAL;
        } else if (0 == strcmp("-u", argv[i])) {
            opts.transports |= TRANSPORT_UDP;
        } else if (0 == strcmp("-a", argv[i])) {
            g_cancelAdvertise = true;
        } else if (0 == strcmp("-sn", argv[i])) {
            secPolicy = AJ_IFC_SECURITY_OFF;
        } else if (0 == strcmp("-sr", argv[i])) {
            secPolicy = AJ_IFC_SECURITY_REQUIRED;
        } else if (0 == strcmp("-so", argv[i])) {
            objSecure = true;
        } else if (0 == strcmp("-con", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                concurrencyLevel = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-dcon", argv[i])) {
            g_disableConcurrency = true;
        } else if (0 == strcmp("-dpws", argv[i])) {
            g_use_delayed_ping_with_sleep = true;
        } else if (0 == strcmp("-about", argv[i])) {
            g_useAboutFeatureDiscovery = true;
            if ((i + 1) < argc && argv[i + 1][0] != '-') {
                ++i;
                g_testAboutApplicationName = argv[i];
            } else {
                g_testAboutApplicationName = "bbservice";
            }
        } else if (0 == strcmp("-runtime", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                run_time = ::strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-psk", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                g_alternatePSK = argv[i];
            }
        } else {
            status = ER_FAIL;
            printf("Unknown option %s\n", argv[i]);
            usage();
            exit(1);
        }
    }

    /* If no transport option was specifie, then make session options very open */
    if (opts.transports == 0) {
        opts.transports = TRANSPORT_ANY;
    }

    QCC_SyncPrintf("opts.transports = 0x%x\n", opts.transports);

    /* Get env vars */
    Environ* env = Environ::GetAppEnviron();
    qcc::String clientArgs = env->Find("BUS_ADDRESS");

    /* Create message bus */
    g_msgBus = new BusAttachment("bbservice", true, concurrencyLevel);

    /* Add org.alljoyn.alljoyn_test interface */
    InterfaceDescription* testIntf = NULL;
    status = g_msgBus->CreateInterface(::org::alljoyn::alljoyn_test::InterfaceName, testIntf, secPolicy);
    if (ER_OK == status) {
        testIntf->AddSignal("my_signal", "a{ys}", NULL, 0);
        testIntf->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
        testIntf->AddMethod("delayed_ping", "su", "s", "inStr,delay,outStr", 0);
        testIntf->AddMethod("time_ping", "uq", "uq", NULL, 0);
        testIntf->Activate();
    } else {
        QCC_LogError(status, ("Failed to create interface %s", ::org::alljoyn::alljoyn_test::InterfaceName));
    }

    /* Add org.alljoyn.alljoyn_test.values interface */
    if (ER_OK == status) {
        InterfaceDescription* valuesIntf = NULL;
        status = g_msgBus->CreateInterface(::org::alljoyn::alljoyn_test::values::InterfaceName, valuesIntf, secPolicy);
        if (ER_OK == status) {
            valuesIntf->AddProperty("int_val", "i", PROP_ACCESS_RW);
            valuesIntf->AddProperty("str_val", "s", PROP_ACCESS_RW);
            valuesIntf->AddProperty("ro_str", "s", PROP_ACCESS_READ);
            valuesIntf->Activate();
        } else {
            QCC_LogError(status, ("Failed to create interface %s", ::org::alljoyn::alljoyn_test::values::InterfaceName));
        }
    }

    /* Start the msg bus */
    if (ER_OK == status) {
        status = g_msgBus->Start();
    } else {
        QCC_LogError(status, ("BusAttachment::Start failed"));
        exit(1);
    }

    /* Create a bus listener to be used to accept incoming session requests */
    g_myBusListener = new MyBusListener(*g_msgBus, opts);

    /* Register local objects and connect to the daemon */
    LocalTestObject* testObj = new LocalTestObject(*g_msgBus, ::org::alljoyn::alljoyn_test::ObjectPath, reportInterval, opts);
    g_msgBus->RegisterBusObject(*testObj, objSecure);

    g_msgBus->EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA ALLJOYN_ECDHE_PSK ALLJOYN_SRP_KEYX ALLJOYN_SRP_LOGON ALLJOYN_ECDHE_NULL", new MyAuthListener(), keyStore, keyStore != NULL);
    /*
     * Pre-compute logon entry for user sleepy
     */
    g_msgBus->AddLogonEntry("ALLJOYN_SRP_LOGON", "sleepy", "123456");

    /* Connect to the daemon */
    if (clientArgs.empty()) {
        status = g_msgBus->Connect();
    } else {
        status = g_msgBus->Connect(clientArgs.c_str());
    }
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to connect to \"%s\"", clientArgs.c_str()));
    }

    uint32_t startTime = GetTimestamp();
    uint32_t endTime = GetTimestamp();

    if (ER_OK == status) {
        QCC_SyncPrintf("bbservice %s ready to accept connections\n", g_wellKnownName.c_str());
        while (g_interrupt == false) {
            qcc::Sleep(100);
            if ((run_time != 0) && ((endTime - startTime) > run_time)) {
                break;
            }
            endTime = GetTimestamp();
        }
    }

    g_msgBus->UnregisterBusObject(*testObj);
    delete testObj;

    /* Clean up msg bus */
    delete g_msgBus;
    delete g_myBusListener;

    printf("Runtime elapsed: %u ms \n", (endTime - startTime));
    printf("%s exiting with status %d (%s)\n", argv[0], status, QCC_StatusText(status));

#ifdef ROUTER
    AllJoynRouterShutdown();
#endif
    AllJoynShutdown();
    return (int) status;
}
