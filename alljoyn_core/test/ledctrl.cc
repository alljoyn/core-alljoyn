/* ledctrl - Control the led of the remote device.*/

/******************************************************************************
 * Copyright (c) 2013, 2014, AllSeen Alliance. All rights reserved.
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
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusListener.h>
#include <alljoyn/Session.h>
#include <alljoyn/Status.h>
#include <alljoyn/version.h>
#include <alljoyn/PasswordManager.h>

#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Event.h>
#include <qcc/Debug.h>
#include <qcc/Environ.h>

#include <vector>
#include <signal.h>

#define QCC_MODULE "LED_CTRL"

using namespace ajn;
using namespace qcc;
using namespace std;

namespace org {
namespace alljoyn {
namespace alljoyn_test {
const char* InterfaceName = "org.alljoyn.sample.ledcontroller";
const char* DefaultWellKnownName = "org.alljoyn.sample.ledservice";
const char* ObjectPath = "/org/alljoyn/sample/ledcontroller";
const char* DaemonBusName = "quiet@org.alljoyn.BusNode.Led";
const SessionPort SessionPort = 24;
}
}
}

class MyBusListener;
static BusAttachment* g_msgBus;
static qcc::Event g_discoverEvent;
static MyBusListener* g_busListener;
static qcc::String g_wellKnownName = ::org::alljoyn::alljoyn_test::DefaultWellKnownName;
static volatile sig_atomic_t g_interrupt = false;

static void SigIntHandler(int sig)
{
    g_interrupt = true;
}

char* get_line(char*str, size_t num, FILE*fp)
{
    char*p = fgets(str, num, fp);

    // fgets will capture the '\n' character if the string entered is shorter than
    // num. Remove the '\n' from the end of the line and replace it with nul '\0'.
    if (p != NULL) {
        size_t last = strlen(str) - 1;
        if (str[last] == '\n') {
            str[last] = '\0';
        }
    }
    return p;
}

static String NextTok(String& inStr)
{
    String ret;
    size_t off = inStr.find_first_of(' ');
    if (off == String::npos) {
        ret = inStr;
        inStr.clear();
    } else {
        ret = inStr.substr(0, off);
        inStr = Trim(inStr.substr(off));
    }
    return Trim(ret);
}


/** AllJoynListener receives discovery events from AllJoyn */
class MyBusListener : public BusListener, public SessionListener {
  public:

    MyBusListener(bool stopDiscover) : BusListener(), sessionId(0), stopDiscover(stopDiscover) { }

    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        QCC_SyncPrintf("FoundAdvertisedName(name=%s, transport=0x%x, prefix=%s)\n", name, transport, namePrefix);

        /* We must enable concurrent callbacks since some of the calls below are blocking */
        g_msgBus->EnableConcurrentCallbacks();

        if (0 == ::strcmp(name, g_wellKnownName.c_str())) {
            /* We found a remote bus that is advertising bbservice's well-known name so connect to it */
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
            QStatus status;

            if (stopDiscover) {
                status = g_msgBus->CancelFindAdvertisedName(g_wellKnownName.c_str());
                if (ER_OK != status) {
                    QCC_LogError(status, ("CancelFindAdvertisedName(%s) failed", name));
                }
            }

            status = g_msgBus->JoinSession(name, ::org::alljoyn::alljoyn_test::SessionPort, this, sessionId, opts);
            if (ER_OK != status) {
                QCC_LogError(status, ("Join Session(%s) failed", name));
            } else {
                QCC_SyncPrintf("Joined Session %d\n", sessionId);
            }

            /* Release the main thread */
            if (ER_OK == status) {
                g_discoverEvent.SetEvent();
            }
        }
    }

    void LostAdvertisedName(const char* name, TransportMask transport, const char* prefix)
    {
        QCC_SyncPrintf("LostAdvertisedName(name=%s, transport=0x%x, prefix=%s)\n", name, transport, prefix);
    }

    void NameOwnerChanged(const char* name, const char* previousOwner, const char* newOwner)
    {
        QCC_SyncPrintf("NameOwnerChanged(%s, %s, %s)\n",
                       name,
                       previousOwner ? previousOwner : "null",
                       newOwner ? newOwner : "null");
    }

    void SessionLost(SessionId sessionId, SessionLostReason reason) {
        QCC_SyncPrintf("SessionLost(%08x) was called. Reason = %u.\n", sessionId, reason);
        _exit(1);
    }

    SessionId GetSessionId() const { return sessionId; }

  private:
    SessionId sessionId;
    bool stopDiscover;
};

int main(int argc, char** argv)
{
    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    PasswordManager::SetCredentials("ALLJOYN_PIN_KEYX", "1234");

    /* Get env vars */
    Environ* env = Environ::GetAppEnviron();

    /* Ensure that the BundledRouter is used since the credentials will
     * not take effect if the pre-installed daemon is used.
     */
    qcc::String connectArgs = "null:";

    QStatus status = ER_OK;
    g_msgBus = new BusAttachment("LedControl", true);
    assert(g_msgBus);

    /* Start the msg bus */
    if (ER_OK == status) {
        status = g_msgBus->Start();
    }

    /* Register a bus listener in order to get discovery indications */
    if (ER_OK == status) {
        g_busListener = new MyBusListener(true);
        g_msgBus->RegisterBusListener(*g_busListener);
    }

    if (ER_OK == status) {
        status = g_msgBus->Connect(connectArgs.c_str());
    }

    if (ER_OK == status) {
        status = g_msgBus->AdvertiseName(::org::alljoyn::alljoyn_test::DaemonBusName, TRANSPORT_TCP);
    }

    if (ER_OK == status) {
        status = g_msgBus->FindAdvertisedName(::org::alljoyn::alljoyn_test::DefaultWellKnownName);
    }

    for (bool discovered = false; !discovered;) {
        /*
         * We want to wait for the discover event, but we also want to
         * be able to interrupt discovery with a control-C. The AllJoyn
         * idiom for waiting for more than one thing this is to create a
         * vector of things to wait on. To provide quick response we
         * poll the g_interrupt bit every 100 ms using a 100 ms timer
         * event.
         */
        qcc::Event timerEvent(100, 100);
        vector<qcc::Event*> checkEvents, signaledEvents;
        checkEvents.push_back(&g_discoverEvent);
        checkEvents.push_back(&timerEvent);
        status = qcc::Event::Wait(checkEvents, signaledEvents);
        if (status != ER_OK && status != ER_TIMEOUT) {
            break;
        }

        /*
         * If it was the discover event that popped, we're done.
         */
        for (vector<qcc::Event*>::iterator i = signaledEvents.begin(); i != signaledEvents.end(); ++i) {
            if (*i == &g_discoverEvent) {
                discovered = true;
                break;
            }
        }
        /*
         * If we see the g_interrupt bit, we're also done.  Set an error
         * condition so we don't do anything else.
         */
        if (g_interrupt) {
            status = ER_FAIL;
            break;
        }
    }

    InterfaceDescription* ledIntf = NULL;
    status = g_msgBus->CreateInterface(::org::alljoyn::alljoyn_test::InterfaceName, ledIntf, false);
    if ((ER_OK == status) && ledIntf) {
        ledIntf->AddMethod("Flash", "u", NULL, "msec", 0);
        ledIntf->AddMethod("On", NULL, NULL, NULL, 0);
        ledIntf->AddMethod("Off", NULL, NULL, NULL, 0);
        ledIntf->Activate();
    } else {
        if (ER_OK == status) {
            status = ER_FAIL;
        }
        QCC_LogError(status, ("Failed to create interface \"%s\"", ::org::alljoyn::alljoyn_test::InterfaceName));
    }

    ProxyBusObject* remoteObj = NULL;
    if (ER_OK == status) {
        /* Create the remote object that will be called */
        remoteObj = new ProxyBusObject(*g_msgBus, g_wellKnownName.c_str(), ::org::alljoyn::alljoyn_test::ObjectPath, g_busListener->GetSessionId());
        remoteObj->AddInterface(*ledIntf);
    }

    /* Parse commands from stdin */
    const int bufSize = 1024;
    char buf[bufSize];
    while (!g_interrupt && (ER_OK == status) && (get_line(buf, bufSize, stdin))) {
        QCC_SyncPrintf(">> %s\n", buf);
        String line(buf);
        String cmd = NextTok(line);
        if (cmd == "flash") {
            uint32_t timeout = StringToU32(NextTok(line), 0, 0);
            if (!timeout) {
                QCC_SyncPrintf("Usage: flash <timeout>\n");
                continue;
            }
            MsgArg args[1];
            args[0].Set("u", timeout);
            Message reply(*g_msgBus);
            status = remoteObj->MethodCall(::org::alljoyn::alljoyn_test::InterfaceName, "Flash", args, 1, reply);
            if (ER_OK != status) {
                QCC_LogError(status, ("MethodCall Flash Fail"));
            }
        } else if (cmd == "on") {
            Message reply(*g_msgBus);
            status = remoteObj->MethodCall(::org::alljoyn::alljoyn_test::InterfaceName, "On", NULL, 0, reply);
            if (ER_OK != status) {
                QCC_LogError(status, ("MethodCall on Fail"));
            }
        } else if (cmd == "off") {
            Message reply(*g_msgBus);
            status = remoteObj->MethodCall(::org::alljoyn::alljoyn_test::InterfaceName, "Off", NULL, 0, reply);
            if (ER_OK != status) {
                QCC_LogError(status, ("MethodCall off Fail"));
            }
        } else if (cmd == "help") {
            QCC_SyncPrintf("Usage:\n");
            QCC_SyncPrintf("flash <timeout>                         - Make device's LED flush for a period in miliseconds\n");
            QCC_SyncPrintf("on                                      - Turn device's LED on\n");
            QCC_SyncPrintf("off                                     - Turn device's LED off\n");
            QCC_SyncPrintf("help                                    - Print usage\n");
            QCC_SyncPrintf("exit                                    - Exit the program\n");
        } else if (cmd == "exit") {
            break;
        } else {
            QCC_SyncPrintf("Unknown command...\n");
        }
    }
    /* Cleanup */

    if (remoteObj) {
        delete remoteObj;
        remoteObj = NULL;
    }
    if (g_msgBus) {
        delete g_msgBus;
        g_msgBus = NULL;
    }

    if (NULL != g_busListener) {
        delete g_busListener;
        g_busListener = NULL;
    }

    return (int) status;
}
