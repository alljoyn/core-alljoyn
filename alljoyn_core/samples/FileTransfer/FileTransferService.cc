//-----------------------------------------------------------------------
// <copyright file="FileTransferService.cc" company="AllSeen Alliance.">
//     Copyright (c) 2012, 2014, AllSeen Alliance. All rights reserved.
//
//        Permission to use, copy, modify, and/or distribute this software for any
//        purpose with or without fee is hereby granted, provided that the above
//        copyright notice and this permission notice appear in all copies.
//
//        THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//        WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//        MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//        ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//        WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//        ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//        OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
// </copyright>
//-----------------------------------------------------------------------

#include <qcc/platform.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <vector>
#include <fstream>

#include "qcc/String.h"

#include "alljoyn/BusAttachment.h"
#include "alljoyn/BusObject.h"
#include "alljoyn/MsgArg.h"
#include "alljoyn/Message.h"
#include "alljoyn/DBusStd.h"
#include "alljoyn/AllJoynStd.h"
#include "alljoyn/version.h"
#include <alljoyn/Status.h>

using namespace std;
using namespace qcc;
using namespace ajn;

/** Static top level message bus object */
static BusAttachment* s_msgBus = NULL;

/* constants for file transfer app*/
const char* INTERFACE_NAME = "org.alljoyn.bus.samples.fileTransfer";
const char* SERVICE_NAME = "org.alljoyn.bus.samples.fileTransfer";
const char* SERVICE_PATH = "/fileTransfer";
const SessionPort SERVICE_PORT = 88;

static char* s_fileName = NULL;
static bool s_filePending = true;

static bool sessionJoinComplete = false;
SessionId serviceSessionId = 0;

static volatile sig_atomic_t g_interrupt = false;

static void SigIntHandler(int sig)
{
    g_interrupt = true;
}

class FileTransferObject : public BusObject {
  public:
    FileTransferObject(const char* path) : BusObject(path)
    {
        // Add the File Transfer Interface
        InterfaceDescription* fileTransferInterface = NULL;
        QStatus status = s_msgBus->CreateInterface(INTERFACE_NAME, fileTransferInterface);

        if (status == ER_OK) {
            printf("Interface Created.\n");
            fileTransferInterface->AddSignal("FileTransfer", "suay", "name,curr,data", 0, 0);
            fileTransferInterface->Activate();
        } else {
            printf("Failed to create interface '%s'\n", INTERFACE_NAME);
        }

        if (fileTransferInterface && status == ER_OK) {
            AddInterface(*fileTransferInterface);
            fileTransferMember = fileTransferInterface->GetMember("FileTransfer");
            assert(fileTransferMember);

            printf("Interface successfully added to the bus.\n");
        } else {
            printf("Failed to Add interface: %s\n", INTERFACE_NAME);
        }
    }

    // Write the data supplied by the service to an output file with filename provided
    void FileTransfer()
    {
        // Give the file buffer a scope such that it can be deleted if an exception occurs.
        char* buf = new char[ALLJOYN_MAX_ARRAY_LEN];

        ifstream inputStream(s_fileName, ios::in | ios::binary);

        if (inputStream.is_open()) {
            MsgArg args[3];
            QStatus status = ER_OK;
            const uint8_t flags = ALLJOYN_FLAG_GLOBAL_BROADCAST;
            uint32_t count = 1;

            // Get length of the file.
            filebuf* streamBuf = inputStream.rdbuf();
            filebuf::pos_type length = streamBuf->pubseekoff(0, ios::end, ios::in);

            // Seek back to the beginning for reading the file.
            streamBuf->pubseekpos(0, ios::in);

            while (length > 0) {
                std::streamsize bufferLength = ALLJOYN_MAX_ARRAY_LEN;

                if (length > (filebuf::pos_type)ALLJOYN_MAX_ARRAY_LEN) {
                    length -= (filebuf::pos_type)ALLJOYN_MAX_ARRAY_LEN;
                } else {
                    bufferLength = length;
                    length = 0;
                }

                inputStream.read(buf, bufferLength);

                args[0].Set("s", s_fileName);
                args[1].Set("u", count);
                args[2].Set("ay", bufferLength, buf);

                status = Signal(NULL, serviceSessionId, *fileTransferMember, args, 3, 0, flags);

                printf("Sent signal with Array#: %u and returned status: %s.\n", count, QCC_StatusText(status));

                count++;
            }

            args[0].Set("s", s_fileName);
            args[1].Set("u", count);
            args[2].Set("ay", 0, NULL);

            status = Signal(NULL, serviceSessionId, *fileTransferMember, args, 3, 0, flags);

            s_filePending = false;

            printf("Sent end of file signal and returned status: %s.\n", QCC_StatusText(status));
        } else {
            printf("The file doesn't exist or the permissions is stopping the app from opening the file.\n");
        }

        delete [] buf;
    }

  private:
    const InterfaceDescription::Member* fileTransferMember;
};

/** Static bus listener */
static FileTransferObject* s_busObject = NULL;

/** AllJoynListener receives discovery events from AllJoyn */
class MyBusListener : public BusListener, public SessionListener, public SessionPortListener {
  public:
    // Called by the bus when a session has been successfully joined.
    void SessionJoined(SessionPort sessionPort, SessionId sessionId, const char* joiner)
    {
        sessionJoinComplete = true;
        serviceSessionId = sessionId;
        printf("Session joined successfully with %s (sessionId=%d)\n", joiner, sessionId);
        s_busObject->FileTransfer();
    }

    // Accept or reject an incoming JoinSession request.
    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
    {
        if (sessionPort != SERVICE_PORT) {
            printf("Rejecting join attempt on unexpected session port %d\n", sessionPort);
            return false;
        }
        printf("Accepting join session request from %s (opts.proximity=%x, opts.traffic=%x, opts.transports=%x)\n",
               joiner, opts.proximity, opts.traffic, opts.transports);
        return true;
    }

    // Called by the bus when an external bus is discovered that is advertising a well-known
    // name that this attachment has registered interest in via a DBus call to
    // org.alljoyn.Bus.FindAdvertisedName.
    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
    }

    // Called by the bus when the ownership of any well-known name changes.
    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
    {
        if (newOwner && (0 == strcmp(busName, SERVICE_NAME))) {
            printf("NameOwnerChanged: name=%s, oldOwner=%s, newOwner=%s\n",
                   busName,
                   previousOwner ? previousOwner : "<none>",
                   newOwner ? newOwner : "<none>");
        }
    }

    // Called by the bus when an advertisement previously reported through FoundName has become unavailable.
    void LostAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
    }

    // Called when a BusAttachment this listener is registered with is has become disconnected from the bus.
    void BusDisconnected()
    {
    }

    // Called when a BusAttachment this listener is registered with is stopping.
    void BusStopping()
    {
    }

    // Called by the bus when the listener is registered.
    void ListenerRegistered(BusAttachment*bus)
    {
    }

    // Called by the bus when the listener is unregistered.
    void ListenerUnregistered()
    {
    }

    // Called by the bus when an existing session becomes disconnected.
    void SessionLost(SessionId sessionId, SessionLostReason reason)
    {
    }

    //Called by the bus when a member of a multipoint session is added.
    void SessionMemberAdded(SessionId sessionId, const char* uniqueName)
    {
    }

    // Called by the bus when a member of a multipoint session is removed.
    void SessionMemberRemoved(SessionId sessionId, const char* uniqueName)
    {
    }
};

/* Static data. */
static MyBusListener s_busListener;

/* Wait for SessionJoin to be completed, then wait for the pending file transfer,
 * to complete, then repeat. If at any time SIGINT occurs then return.
 */
void WaitForProgramComplete(void)
{
    while (!g_interrupt) {
        while (!sessionJoinComplete && !g_interrupt) {
#ifdef _WIN32
            Sleep(100);
#else
            usleep(100 * 1000);
#endif
        }

        while (s_filePending && !g_interrupt) {
#ifdef _WIN32
            Sleep(100);
#else
            usleep(100 * 1000);
#endif
        }
        sessionJoinComplete = false;
    }
}

/* Wait for SessionJoin to be completed, then wait for the pending file transfer, */
QStatus CreateAndRegisterBusObject()
{
    QStatus status = ER_OUT_OF_MEMORY;

    s_busObject = new FileTransferObject(SERVICE_PATH);

    if (s_busObject) {
        status = s_msgBus->RegisterBusObject(*s_busObject);
    }

    return status;
}

/* Start the message bus, report the result to stdout, and return the status code. */
QStatus StartMessageBus(void)
{
    QStatus status = s_msgBus->Start();

    if (ER_OK == status) {
        printf("BusAttachment started.\n");
    } else {
        printf("Start of BusAttachment failed (%s).\n", QCC_StatusText(status));
    }

    return status;
}

/* Connect, report the result to stdout, and return the status code. */
QStatus ConnectBusAttachment(void)
{
    QStatus status = s_msgBus->Connect();

    if (ER_OK == status) {
        printf("Connect to '%s' succeeded.\n", s_msgBus->GetConnectSpec().c_str());
    } else {
        printf("Failed to connect to '%s' (%s).\n", s_msgBus->GetConnectSpec().c_str(), QCC_StatusText(status));
    }

    return status;
}

/* Request the service name, report the result to stdout, and return the status code. */
QStatus RequestName(void)
{
    const uint32_t flags = DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE;
    QStatus status = s_msgBus->RequestName(SERVICE_NAME, flags);

    if (ER_OK == status) {
        printf("RequestName('%s') succeeded.\n", SERVICE_NAME);
    } else {
        printf("RequestName('%s') failed (status=%s).\n", SERVICE_NAME, QCC_StatusText(status));
    }

    return status;
}

/** Create the session, report the result to stdout, and return the status code. */
QStatus CreateSession(TransportMask mask)
{
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, mask);
    SessionPort sp = SERVICE_PORT;
    QStatus status = s_msgBus->BindSessionPort(sp, opts, s_busListener);

    if (ER_OK == status) {
        printf("BindSessionPort succeeded.\n");
    } else {
        printf("BindSessionPort failed (%s).\n", QCC_StatusText(status));
    }

    return status;
}

/** Advertise the service name, report the result to stdout, and return the status code. */
QStatus AdvertiseName(TransportMask mask)
{
    QStatus status = s_msgBus->AdvertiseName(SERVICE_NAME, mask);

    if (ER_OK == status) {
        printf("Advertisement of the service name '%s' succeeded.\n", SERVICE_NAME);
    } else {
        printf("Failed to advertise name '%s' (%s).\n", SERVICE_NAME, QCC_StatusText(status));
    }

    return status;
}



void Usage(int status)
{
    printf("Usage: FileTransferService <FileName>\nUse Control Break to exit.");
    exit(status);
}

int main(int argc, char** argv, char** envArg)
{
    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    QStatus status = ER_OK;

    // Make sure a file path was provided
    if (argc <= 1) {
        Usage(ER_BAD_ARG_1);
    } else {
        s_fileName = argv[1];
    }


    /* Create message bus */
    s_msgBus = new BusAttachment("FileTransferClient", true);

    /* This test for NULL is only required if new() behavior is to return NULL
     * instead of throwing an exception upon an out of memory failure.
     */
    if (!s_msgBus) {
        status = ER_OUT_OF_MEMORY;
    }

    if (ER_OK == status) {
        status = CreateAndRegisterBusObject();
    }

    /* Register a bus listener in order to get discovery indications */
    if (ER_OK == status) {
        s_msgBus->RegisterBusListener(s_busListener);
        printf("BusListener Registered.\n");
    }

    if (ER_OK == status) {
        status = StartMessageBus();
    }


    if (ER_OK == status) {
        status = ConnectBusAttachment();
    }

    /*
     * Advertise this service on the bus.
     * There are three steps to advertising this service on the bus.
     * 1) Request a well-known name that will be used by the client to discover
     *    this service.
     * 2) Create a session.
     * 3) Advertise the well-known name.
     */
    if (ER_OK == status) {
        status = RequestName();
    }

    const TransportMask SERVICE_TRANSPORT_TYPE = TRANSPORT_ANY;

    if (ER_OK == status) {
        status = CreateSession(SERVICE_TRANSPORT_TYPE);
    }

    if (ER_OK == status) {
        status = AdvertiseName(SERVICE_TRANSPORT_TYPE);
    }

    if (ER_OK == status) {
        WaitForProgramComplete();
    }

    /* Deallocate bus and other objects. */
    delete s_msgBus;
    delete s_busObject;

    s_msgBus = NULL;
    s_busObject = NULL;

    printf("File Transfer Service exiting with status 0x%04x (%s).\n", status, QCC_StatusText(status));

    return (int)status;
}
