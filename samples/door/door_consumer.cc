/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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

#include "door_common.h"

#include <set>
#include <memory>

#include <qcc/Crypto.h>
#include <qcc/Thread.h>

#include <alljoyn/Init.h>
#include <alljoyn/AboutListener.h>

using namespace sample::securitymgr::door;
using namespace ajn;
using namespace std;
using namespace qcc;

class DoorSessionListener :
    public SessionListener {
};

class DoorMessageReceiver :
    public MessageReceiver {
  public:
    void DoorEventHandler(const InterfaceDescription::Member* member, const char* srcPath, Message& msg)
    {
        QCC_UNUSED(msg);
        QCC_UNUSED(srcPath);
        QCC_UNUSED(member);

        printf("received message ...\n");
        //TODO: parse message
    }
};

static DoorSessionListener theListener;

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
        about.GetAppName(&appName);
        char* deviceName;
        about.GetDeviceName(&deviceName);

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

class DoorSessionManager {
  public:

    DoorSessionManager(BusAttachment* _ba, uint32_t _timeout) :
        ba(_ba), timeout(_timeout)
    {
    }

    QStatus MethodCall(string busName, string methodName)
    {
        shared_ptr<ProxyBusObject> remoteObj;
        QStatus status = GetProxyDoorObject(busName, remoteObj);
        if (ER_OK != status) {
            return status;
        }

        Message reply(*ba);
        MsgArg arg;
        printf("Calling %s on '%s'\n", methodName.c_str(), busName.c_str());
        status = remoteObj->MethodCall(DOOR_INTERFACE, methodName.c_str(),
                                       nullptr, 0, reply, timeout);

        // retry on policy/identity update
        string securityViolation = "org.alljoyn.Bus.SecurityViolation";
        if ((ER_BUS_REPLY_IS_ERROR_MESSAGE == status) &&
            (reply->GetErrorName() != NULL) &&
            (string(reply->GetErrorName()) == securityViolation)) {
            status = remoteObj->MethodCall(DOOR_INTERFACE, methodName.c_str(),
                                           nullptr, 0, reply, timeout);
        }

        if (ER_OK != status) {
            printf("Failed to call method %s (%s)\n", methodName.c_str(),
                   QCC_StatusText(status));
            if (reply->GetErrorName() != NULL) {
                printf("ErrorName %s\n", reply->GetErrorName());
            }
            return status;
        }

        const MsgArg* result;
        result = reply->GetArg(0);
        bool value;
        result->Get("b", &value);
        printf("%s returned %d\n", methodName.c_str(), value);

        return status;
    }

    QStatus GetProperty(string busName, string propertyName)
    {
        shared_ptr<ProxyBusObject> remoteObj;
        QStatus status = GetProxyDoorObject(busName, remoteObj);
        if (ER_OK != status) {
            return status;
        }

        MsgArg arg;
        status = remoteObj->GetProperty(DOOR_INTERFACE, propertyName.c_str(),
                                        arg, timeout);

        // retry on policy/identity update
        if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
            // impossible to check specific error message (see ASACORE-1811)
            status = remoteObj->GetProperty(DOOR_INTERFACE, propertyName.c_str(),
                                            arg, timeout);
        }

        if (ER_OK != status) {
            printf("Failed to GetPropery %s (%s)\n", propertyName.c_str(),
                   QCC_StatusText(status));
            return status;
        }

        const MsgArg* result;
        result = &arg;
        bool value;
        result->Get("b", &value);
        printf("%s returned %d\n", propertyName.c_str(), value);

        return status;
    }

    void Stop()
    {
        for (SessionMap::iterator it = sessions.begin(); it != sessions.end(); it++) {
            it->second.doorProxy = nullptr;
            ba->LeaveSession(it->second.id);
        }
    }

  private:

    struct Session {
        SessionId id;
        shared_ptr<ProxyBusObject> doorProxy;
    };
    typedef std::map<string, Session> SessionMap;

    BusAttachment* ba;
    uint32_t timeout;
    SessionMap sessions;

    QStatus GetProxyDoorObject(string busName,
                               shared_ptr<ProxyBusObject>& remoteObj)
    {
        QStatus status = ER_FAIL;

        SessionMap::iterator it;
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

    QStatus JoinSession(string busName, Session& session)
    {
        QStatus status = ER_FAIL;

        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false,
                         SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        status = ba->JoinSession(busName.c_str(), DOOR_APPLICATION_PORT,
                                 &theListener, session.id, opts);
        if (ER_OK != status) {
            printf("Failed to join session\n");
            return status;
        }

        const InterfaceDescription* remoteIntf = ba->GetInterface(DOOR_INTERFACE);
        if (nullptr == remoteIntf) {
            printf("Could not get door interface\n");
            status = ER_FAIL;
            goto exit;
        }

        session.doorProxy = make_shared<ProxyBusObject>(*ba, busName.c_str(),
                                                        DOOR_OBJECT_PATH, session.id);
        if (nullptr == session.doorProxy) {
            printf("Failed to create proxy bus object\n");
            status = ER_FAIL;
            goto exit;
        }

        status = session.doorProxy->AddInterface(*remoteIntf);
        if (status != ER_OK) {
            printf("Failed to add door interface to proxy bus object\n");
            goto exit;
        }

    exit:
        if (status != ER_OK) {
            session.doorProxy = nullptr;
            ba->LeaveSession(session.id);
        }

        return status;
    }
};

QStatus PerformDoorAction(DoorSessionManager& sm, char cmd, string busName)
{
    QStatus status = ER_FAIL;

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
        status = sm.MethodCall(busName, methodName);
    } else {
        status = sm.GetProperty(busName, propertyName);
    }

    return status;
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

    //Do the common set-up
    DoorCommon common(appName);
    QStatus status = common.Init(false);
    if (status != ER_OK) {
        cerr << "Failed to initialize common layer" << endl;
        exit(1);
    }
    printf("Common layer is initialized\n");

    status = common.AnnounceAbout();
    if (status != ER_OK) {
        cerr << "Failed to announce about" << endl;
        exit(1);
    }

    BusAttachment* ba = common.GetBusAttachment();
    DoorSessionManager sessionManager(ba, 10000);

    //Wait until we are claimed...
    PermissionConfigurator::ApplicationState appState = PermissionConfigurator::NOT_CLAIMABLE;
    ba->GetPermissionConfigurator().GetApplicationState(appState);
    while (PermissionConfigurator::CLAIMED != appState) {
        printf("Consumer is not yet Claimed; Waiting to be claimed\n");
        qcc::Sleep(5000);
        ba->GetPermissionConfigurator().GetApplicationState(appState);
    }

    //Register signal hander
    DoorMessageReceiver dmr;
    ba->RegisterSignalHandlerWithRule(&dmr,
                                      static_cast<MessageReceiver::SignalHandler>(&DoorMessageReceiver::DoorEventHandler),
                                      common.GetDoorSignal(),
                                      DOOR_SIGNAL_MATCH_RULE);

    //Register About listener and look for doors...
    ba->WhoImplements(DOOR_INTERFACE);
    DoorAboutListener dal;
    ba->RegisterAboutListener(dal);

    char cmd;
    set<string> doors;

    if (status != ER_OK) {
        goto exit;
    }
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
exit:

    sessionManager.Stop();
    ba->UnregisterAboutListener(dal);
    common.Fini();

#ifdef ROUTER
    AllJoynRouterShutdown();
#endif

    AllJoynShutdown();
    return (int)status;
}
