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
        QCC_UNUSED(aboutDataArg);
        QCC_UNUSED(objectDescriptionArg);
        QCC_UNUSED(port);
        QCC_UNUSED(version);

        printf("Found door @%s\n", busName); //TODO take more data from about
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

QStatus PerformDoorAction(BusAttachment* ba, char cmd, string busName)
{
    const char* methodName = nullptr;
    const char* displayName = nullptr;
    switch (cmd) {
    case 'o':
        methodName = DOOR_OPEN;
        break;

    case 'c':
        methodName = DOOR_CLOSE;
        break;

    case 's':
        methodName = DOOR_GET_STATE;
        break;

    case 'g':
        //getting property; we don't call the method
        break;
    }
    displayName = methodName == nullptr ? "GetProperty" : methodName;

    printf("\nCalling %s on '%s'\n", displayName, busName.c_str());
    SessionId sessionId;
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false,
                     SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    QStatus status = ba->JoinSession(busName.c_str(), DOOR_APPLICATION_PORT,
                                     &theListener, sessionId, opts);
    if (status != ER_OK) {
        printf("Failed to Join session...\n");
        return status;
    }

    /* B. Setup ProxyBusObject:
     *     - Create a ProxyBusObject from remote application and session
     *     - Get the interface description from the bus based on the remote interface name
     *     - Extend the ProxyBusObject with the interface description
     */

    const InterfaceDescription* remoteIntf = ba->GetInterface(DOOR_INTERFACE);
    if (nullptr == remoteIntf) {
        printf("No remote interface found of app to claim\n");
        return ER_FAIL;
    }
    ProxyBusObject* remoteObj = new ProxyBusObject(*ba, busName.c_str(),
                                                   DOOR_OBJECT_PATH, sessionId);
    if (remoteObj == nullptr) {
        printf("Failed create proxy bus object\n");
        return ER_FAIL;
    }
    Message reply(*ba);
    MsgArg arg;
    uint32_t timeout = 10000;
    const MsgArg* result;

    status = remoteObj->AddInterface(*remoteIntf);
    if (status != ER_OK) {
        printf("Failed to add interface to proxy object.\n");
        goto out;
    }

    if (methodName) {
        status = remoteObj->MethodCall(DOOR_INTERFACE, methodName, nullptr, 0, reply, timeout);
    } else {
        status = remoteObj->GetProperty(DOOR_INTERFACE, DOOR_STATE, arg, timeout);
    }
    if (status != ER_OK) {
        printf("Failed to call method %s interface to proxy object\n", displayName);
        goto out;
    }
    if (methodName) {
        result = reply->GetArg(0);
    } else {
        result = &arg;
    }

    bool value;
    result->Get("b", &value);

    printf("%s called result = %d\n", displayName, value);

out:
    delete remoteObj;
    ba->LeaveSession(sessionId);
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

int main(int arg, char** argv)
{
    QCC_UNUSED(argv);
    QCC_UNUSED(arg);

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
    DoorCommon common("DoorConsumer");  //TODO make the name a command-line argument
    QStatus status = common.Init("/tmp/consdb.ks", false); //TODO allow for a keystore path as a command-line argument
    printf("Common layer is initialized\n");

    if (status != ER_OK) {
        exit(1);
    }

    BusAttachment* ba = common.GetBusAttachment();
#if DOOR_INTF_SECURE == 1
    //Wait until we are claimed...
    PermissionConfigurator::ApplicationState appState = PermissionConfigurator::NOT_CLAIMABLE;
    ba->GetPermissionConfigurator().GetApplicationState(appState);
    while (PermissionConfigurator::CLAIMED != appState) {
        printf("Consumer is not yet Claimed; Waiting to be claimed\n");
        qcc::Sleep(5000);
        ba->GetPermissionConfigurator().GetApplicationState(appState);
    }
#endif
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
                    PerformDoorAction(ba, cmd, *it);
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

    ba->UnregisterAboutListener(dal);
    common.Fini();

#ifdef ROUTER
    AllJoynRouterShutdown();
#endif
    AllJoynShutdown();
    return (int)status;
}
