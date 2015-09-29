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

#include "secure_door_common.h"

#include <set>
#include <memory>

#include <qcc/Crypto.h>

#include <alljoyn/Init.h>
#include <alljoyn/AboutListener.h>

using namespace sample::secure::door;
using namespace ajn;
using namespace std;
using namespace qcc;

/* Door session listener */
class DoorSessionListener :
    public SessionListener {
};

/* Door message receiver */
class DoorMessageReceiver :
    public MessageReceiver {
  public:
    void DoorEventHandler(const InterfaceDescription::Member* member,
                          const char* srcPath,
                          Message& msg)
    {
        QCC_UNUSED(srcPath);
        QCC_UNUSED(member);

        const MsgArg* result;
        result = msg->GetArg(0);
        bool value;
        QStatus status = result->Get("b", &value);
        if (ER_OK != status) {
            fprintf(stderr, "Failed to Get boolean - status (%s)\n", QCC_StatusText(status));
        } else {
            printf("Received door %s event ...\n", value ? "opened" : "closed");
        }
    }
};

static DoorSessionListener theListener;

/* Door about listener */
class DoorAboutListener :
    public AboutListener {
    set<string> doors;

    void Announced(const char* busName, uint16_t version,
                   SessionPort port, const MsgArg& objectDescriptionArg,
                   const MsgArg& aboutDataArg)
    {
        QCC_UNUSED(objectDescriptionArg);
        QCC_UNUSED(port);
        QCC_UNUSED(version);

        AboutData about(aboutDataArg);
        char* appName;
        QStatus status = about.GetAppName(&appName);
        if (ER_OK != status) {
            fprintf(stderr, "Failed to GetAppName - status (%s)\n", QCC_StatusText(status));
            return;
        }
        char* deviceName;
        status = about.GetDeviceName(&deviceName);
        if (ER_OK != status) {
            fprintf(stderr, "Failed to GetDeviceName - status (%s)\n", QCC_StatusText(status));
            return;
        }

        printf("Found door %s @ %s (%s)\n", appName, busName, deviceName);
        doors.insert(string(busName));
    }

  public:
    set<string> GetDoorNames()
    {
        return doors;
    }

    void RemoveDoorName(string doorName)
    {
        doors.erase(doorName);
    }
};

/* Door session manager */
class DoorSessionManager {
  public:

    DoorSessionManager(BusAttachment* _ba, uint32_t _timeout) :
        ba(_ba), timeout(_timeout)
    {
    }

    void MethodCall(const string& busName, const string& methodName)
    {
        shared_ptr<ProxyBusObject> remoteObj;
        QStatus status = GetProxyDoorObject(busName, remoteObj);
        if (ER_OK != status) {
            fprintf(stderr, "Failed to GetProxyDoorObject - status (%s)\n",
                    QCC_StatusText(status));
            return;
        }

        Message reply(*ba);
        MsgArg arg;
        printf("Calling %s on '%s'\n", methodName.c_str(), busName.c_str());
        status = remoteObj->MethodCall(DOOR_INTERFACE, methodName.c_str(),
                                       nullptr, 0, reply, timeout);

        // Retry on policy/identity update
        string securityViolation = "org.alljoyn.Bus.SecurityViolation";
        if ((ER_BUS_REPLY_IS_ERROR_MESSAGE == status) &&
            (reply->GetErrorName() != NULL) &&
            (string(reply->GetErrorName()) == securityViolation)) {
            status = remoteObj->MethodCall(DOOR_INTERFACE, methodName.c_str(),
                                           nullptr, 0, reply, timeout);
        }

        if (ER_OK != status) {
            fprintf(stderr, "Failed to call method %s - status (%s)\n", methodName.c_str(),
                    QCC_StatusText(status));
            return;
        }

        const MsgArg* result;
        result = reply->GetArg(0);
        bool value;
        status = result->Get("b", &value);
        if (ER_OK != status) {
            fprintf(stderr, "Failed to Get boolean - status (%s)\n", QCC_StatusText(status));
            return;
        }

        printf("%s returned %d\n", methodName.c_str(), value);
    }

    void GetProperty(const string& busName, const string& propertyName)
    {
        shared_ptr<ProxyBusObject> remoteObj;
        QStatus status = GetProxyDoorObject(busName, remoteObj);
        if (ER_OK != status) {
            fprintf(stderr, "Failed to GetProxyDoorObject - status (%s)\n",
                    QCC_StatusText(status));
            return;
        }

        MsgArg arg;
        status = remoteObj->GetProperty(DOOR_INTERFACE, propertyName.c_str(),
                                        arg, timeout);

        // Retry on policy/identity update
        if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
            // Impossible to check specific error message (see ASACORE-1811)
            status = remoteObj->GetProperty(DOOR_INTERFACE, propertyName.c_str(),
                                            arg, timeout);
        }

        if (ER_OK != status) {
            fprintf(stderr, "Failed to GetProperty %s - status (%s)\n", propertyName.c_str(),
                    QCC_StatusText(status));
            return;
        }

        const MsgArg* result;
        result = &arg;
        bool value;
        status = result->Get("b", &value);
        if (ER_OK != status) {
            fprintf(stderr, "Failed to Get boolean - status (%s)\n", QCC_StatusText(status));
            return;
        }

        printf("%s returned %d\n", propertyName.c_str(), value);
    }

    void Stop()
    {
        for (SessionsMap::iterator it = sessions.begin(); it != sessions.end(); it++) {
            it->second.doorProxy = nullptr;
            ba->LeaveSession(it->second.id);
        }
    }

  private:

    struct Session {
        SessionId id;
        shared_ptr<ProxyBusObject> doorProxy;
    };
    typedef map<string, Session> SessionsMap;

    BusAttachment* ba;
    uint32_t timeout;
    SessionsMap sessions;

    QStatus GetProxyDoorObject(const string& busName,
                               shared_ptr<ProxyBusObject>& remoteObj)
    {
        QStatus status = ER_FAIL;

        SessionsMap::iterator it;
        it = sessions.find(busName);
        if (it != sessions.end()) {
            remoteObj = it->second.doorProxy;
            status = ER_OK;
        } else {
            Session session;
            status = JoinSession(busName, session);
            if (ER_OK == status) {
                sessions[busName] = session;
                remoteObj = session.doorProxy;
            }
        }

        return status;
    }

    QStatus JoinSession(const string& busName, Session& session)
    {
        QStatus status = ER_FAIL;

        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false,
                         SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        status = ba->JoinSession(busName.c_str(), DOOR_APPLICATION_PORT,
                                 &theListener, session.id, opts);
        if (ER_OK != status) {
            fprintf(stderr, "Failed to JoinSession - status (%s)\n", QCC_StatusText(status));
            return status;
        }

        const InterfaceDescription* remoteIntf = ba->GetInterface(DOOR_INTERFACE);
        if (!remoteIntf) {
            status = ER_FAIL;
            fprintf(stderr, "Failed to GetInterface\n");
            return status;
        }

        session.doorProxy = make_shared<ProxyBusObject>(*ba, busName.c_str(),
                                                        DOOR_OBJECT_PATH, session.id);
        if (nullptr == session.doorProxy) {
            status = ER_FAIL;
            fprintf(stderr, "Failed to create ProxyBusObject - status (%s)\n",
                    QCC_StatusText(status));
            goto exit;
        }

        status = session.doorProxy->AddInterface(*remoteIntf);
        if (ER_OK != status) {
            fprintf(stderr, "Failed to AddInterface - status (%s)\n",
                    QCC_StatusText(status));
            goto exit;
        }

    exit:
        if (ER_OK != status) {
            session.doorProxy = nullptr;
            ba->LeaveSession(session.id);
        }

        return status;
    }
};

void PerformDoorAction(DoorSessionManager& sm, char cmd, const string& busName)
{
    string methodName;
    string propertyName = string(DOOR_STATE);

    switch (cmd) {
    case 'o':
        methodName = string(DOOR_OPEN);
        break;

    case 'c':
        methodName = string(DOOR_CLOSE);
        break;

    case 's':
        methodName = string(DOOR_GET_STATE);
        break;

    case 'g':
        break;
    }

    if (methodName != "") {
        sm.MethodCall(busName, methodName);
    } else {
        sm.GetProperty(busName, propertyName);
    }
}

void printHelp()
{
    printf("Welcome to the door consumer - enter 'h' for this menu\n"
           "Menu\n"
           ">o : Open doors\n"
           ">c : Close doors\n"
           ">s : Doors state - using ProxyBusObject->MethodCall\n"
           ">g : Get doors state - using ProxyBusObject->GetProperty\n"
           ">q : Quit\n");
}

int CDECL_CALL main(int argc, char** argv)
{
    string appName = "DoorConsumer";
    if (argc > 1) {
        appName = string(argv[1]);
    }
    printf("Starting door consumer %s\n", appName.c_str());

    if (AllJoynInit() != ER_OK) {
        return EXIT_FAILURE;
    }

#ifdef ROUTER
    if (AllJoynRouterInit() != ER_OK) {
        AllJoynShutdown();
        return EXIT_FAILURE;
    }
#endif

    //Do the common setup
    DoorCommon common(appName);
    BusAttachment* ba = common.GetBusAttachment();
    DoorCommonPCL pcl(*ba);
    DoorAboutListener dal;
    char cmd;
    set<string> doors;

    //Create a session manager
    DoorSessionManager sessionManager(ba, 10000);

    //Register signal hander
    DoorMessageReceiver dmr;

    QStatus status = common.Init(false, &pcl);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to initialize DoorCommon - status (%s)\n",
                QCC_StatusText(status));
        goto Exit;
    }

    status = common.AnnounceAbout();
    if (ER_OK != status) {
        fprintf(stderr, "Failed to AnnounceAbout - status (%s)\n",
                QCC_StatusText(status));
        goto Exit;
    }

    //Wait until we are claimed
    pcl.WaitForClaimedState();
    if (ER_OK != status) {
        fprintf(stderr, "Failed to WaitForClaimedState - status (%s)\n",
                QCC_StatusText(status));
        goto Exit;
    }

    status = ba->RegisterSignalHandlerWithRule(&dmr,
                                               static_cast<MessageReceiver::SignalHandler>(&DoorMessageReceiver::DoorEventHandler),
                                               common.GetDoorSignal(),
                                               DOOR_SIGNAL_MATCH_RULE);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to register signal handler - status (%s)\n",
                QCC_StatusText(status));
        goto Exit;
    }

    //Register About listener and look for doors
    status = ba->WhoImplements(DOOR_INTERFACE);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to call WhoImplements - status (%s)\n",
                QCC_StatusText(status));
        goto Exit;
    }
    ba->RegisterAboutListener(dal);

    //Execute commands
    printHelp();

    while ((cmd = cin.get()) != 'q') {
        doors.clear();
        printf(">");

        switch (cmd) {
        case 'o':
        case 's':
        case 'c':
        case 'g': {
                doors = dal.GetDoorNames();
                if (doors.size() == 0) {
                    printf("No doors found.\n");
                }
                for (set<string>::iterator it = doors.begin();
                     it != doors.end(); it++) {
                    PerformDoorAction(sessionManager, cmd, *it);
                }
            }
            break;

        case 'h':
            printHelp();
            break;

        case '\n':
            break;

        case '\r':
            break;

        default: {
                fprintf(stderr, "Unknown command!\n");
                printHelp();
            }
        }
    }

Exit:
    sessionManager.Stop();
    ba->UnregisterAllAboutListeners();
    common.Fini();

#ifdef ROUTER
    AllJoynRouterShutdown();
#endif

    AllJoynShutdown();
    return (int)status;
}
