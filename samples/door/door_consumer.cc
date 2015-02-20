/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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
#include <qcc/Crypto.h>
#include <qcc/String.h>
#include <qcc/Thread.h>
#include <alljoyn/about/AnnouncementRegistrar.h>

#include <set>

using namespace sample::securitymgr::door;
using namespace ajn;
using namespace services;

class DoorSessionListener :
    public SessionListener {
};

class DoorMessageReceiver :
    public MessageReceiver {
  public:
    void DoorEventHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath, ajn::Message& msg)
    {
        printf("received message ...\n");
        //TODO: parse message
    }
};

static DoorSessionListener theListener;

class DoorAnnounceHandler :
    public AnnounceHandler {
    std::set<qcc::String> doors;

    virtual void Announce(uint16_t version, uint16_t port, const char* busName, const ObjectDescriptions& objectDescs,
                          const AboutData& aboutData)
    {
        printf("Found door @%s\n", busName); //TODO take more data from about
        doors.insert(qcc::String(busName));
    }

  public:
    std::set<qcc::String> GetDoorNames()
    {
        return doors;
    }

    void RemoveDoorName(qcc::String doorName)
    {
        doors.erase(doorName);
    }
};

QStatus PerformDoorAction(BusAttachment& ba, char cmd, qcc::String busName)
{
    const char* methodName = NULL;
    const char* displayName = NULL;
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

    default:
        printf("Internal error - Unknown command\n");
        exit(7);
    }
    displayName = methodName == NULL ? "GetProperty" : methodName;

    printf("\nCalling %s on '%s'\n", displayName, busName.c_str());
    ajn::SessionId sessionId;
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false,
                     SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    QStatus status = ba.JoinSession(busName.c_str(), DOOR_APPLICATION_PORT,
                                    &theListener, sessionId, opts);
    if (status != ER_OK) {
        printf("Failed to Join session...\n");
        return status;
    }

    /* B. Setup ProxyBusObject:
     *     - Create a ProxyBusObject from remote application info and session
     *     - Get the interface description from the bus based on the remote interface name
     *     - Extend the ProxyBusObject with the interface description
     */

    const InterfaceDescription* remoteIntf = ba.GetInterface(DOOR_INTERFACE);
    if (NULL == remoteIntf) {
        printf("No remote interface found of app to claim\n");
        return ER_FAIL;
    }
    ajn::ProxyBusObject* remoteObj = new ajn::ProxyBusObject(ba, busName.c_str(),
                                                             DOOR_OBJECT_PATH, sessionId);
    if (remoteObj == NULL) {
        printf("Failed create proxy bus object\n");
        return ER_FAIL;
    }
    Message reply(ba);
    MsgArg arg;
    uint32_t timeout = 10000;
    const MsgArg* result;

    status = remoteObj->AddInterface(*remoteIntf);
    if (status != ER_OK) {
        printf("Failed to add interface to proxy object.\n");
        goto out;
    }

    if (methodName) {
        status = remoteObj->MethodCall(DOOR_INTERFACE, methodName, NULL, 0, reply, timeout);
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
    ba.LeaveSession(sessionId);
    return status;
}

int main(int arg, char** argv)
{
    //Do the common set-up
    DoorCommon common("DoorConsumer");     //TODO make name commandline param
    QStatus status = common.init("/tmp/consdb.ks", false); //TODO allow keystore to be defined from cmdline

    printf("Common layer is initialized\n");
    if (status != ER_OK) {
        exit(1);
    }

    BusAttachment& ba = common.GetBusAttachment();
#if DOOR_INTF_SECURE == 1
    //Wait until we are claimed...
    while (PermissionConfigurator::STATE_CLAIMED != ba.GetPermissionConfigurator().GetClaimableState()) {
        printf("Consumer is not yet Claimed; Waiting to be claimed\n");
        qcc::Sleep(5000);
    }
#endif
    //Register signal hander
    DoorMessageReceiver dmr;
    ba.RegisterSignalHandlerWithRule(&dmr,
                                     static_cast<MessageReceiver::SignalHandler>(&DoorMessageReceiver::DoorEventHandler),
                                     common.GetDoorSignal(),
                                     DOOR_SIGNAL_MATCH_RULE);

    //Register About listener and look for doors...
    DoorAnnounceHandler dah;
    const char* intfs[] = { DOOR_INTERFACE };
    status = ajn::services::AnnouncementRegistrar::RegisterAnnounceHandler(
        ba, dah, intfs, 1);

    if (status != ER_OK) {
        exit(1);
    }
    //Execute commands
    printf(
        "Consumer is ready to execute commands; type command 'o', 'c' or 's'; 'g' for getting the property or 'q' to quit\n");

    char cmd;
    do {
        cmd = std::cin.get();
        switch (cmd) {
        case 'q':
        case '\r':
        case '\n':
        case ' ':
            //No special action required for this characters.
            break;

        case 'o':
        case 's':
        case 'c':
        case 'g':
            //TODO do more advanced CLI features if needed.
            std::set<qcc::String> doors = dah.GetDoorNames();
            if (doors.size() == 0) {
                printf("No doors found.\n");
            }
            for (std::set<qcc::String>::iterator it = doors.begin(); it != doors.end(); it++) {
                if (ER_OK != PerformDoorAction(ba, cmd, *it)) {
                    //TODO: don't always remove a failing door from the list.
                    dah.RemoveDoorName(*it);
                }
            }
            break;
        }
    } while (cmd != 'q');
}
