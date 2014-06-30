//-----------------------------------------------------------------------
// <copyright file="FileTransferClient.cc" company="AllSeen Alliance.">
//     Copyright (c) 2012,2014 AllSeen Alliance. All rights reserved.
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
#include "alljoyn/version.h"
#include "alljoyn/AllJoynStd.h"
#include <alljoyn/Status.h>

using namespace std;
using namespace qcc;
using namespace ajn;

/** Static top level message bus object */
static BusAttachment* s_busAtt = NULL;

/* constants for file transfer app*/
const char* INTERFACE_NAME = "org.alljoyn.bus.samples.fileTransfer";
const char* SERVICE_NAME = "org.alljoyn.bus.samples.fileTransfer";
const char* SERVICE_PATH = "/fileTransfer";
const SessionPort SERVICE_PORT = 88;

static bool s_fileTransferComplete = false;
static bool s_joinComplete = false;
SessionId s_sessionId = 0;

static volatile sig_atomic_t s_interrupt = false;

static void SigIntHandler(int sig)
{
    s_interrupt = true;
}

/** AllJoynListener receives discovery events from AllJoyn */
class MyBusListener : public BusListener, public SessionListener {
  public:
    // Called by the bus when an external bus is discovered that is advertising a well-known
    // name that this attachment has registered interest in via a DBus call to
    // org.alljoyn.Bus.FindAdvertisedName.
    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        printf("FoundAdvertisedName(name=%s, prefix=%s)\n", name, namePrefix);

        if (0 == strcmp(name, SERVICE_NAME)) {
            // We found a remote bus that is advertising basic service's  well-known name so connect to it
            /* Since we are in a callback we must enable concurrent callbacks before calling a synchronous method. */
            s_busAtt->EnableConcurrentCallbacks();
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
            QStatus status = s_busAtt->JoinSession(name, SERVICE_PORT, this, s_sessionId, opts);
            if (ER_OK != status) {
                printf("JoinSession failed (status=%s)\n", QCC_StatusText(status));
            } else {
                printf("JoinSession SUCCESS (Session id=%d)\n", s_sessionId);
            }
        }
        s_joinComplete = true;
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
};

class FileTransferObject : public BusObject {
  public:
    FileTransferObject(const char* path) : BusObject(path)
    {
        this->outputStream = NULL;

        InterfaceDescription* fileTransferInterface = NULL;
        QStatus status = s_busAtt->CreateInterface(INTERFACE_NAME, fileTransferInterface);

        if (fileTransferInterface && status == ER_OK) {
            printf("Interface Created.\n");
            fileTransferInterface->AddSignal("FileTransfer", "suay", "name,curr,data", 0, 0);
            fileTransferInterface->Activate();

            status = AddInterface(*fileTransferInterface);
        } else {
            printf("Failed to create interface '%s'\n", INTERFACE_NAME);
        }

        const InterfaceDescription::Member* fileTransferMember = NULL;

        if (fileTransferInterface && status == ER_OK) {
            printf("Interface successfully added to the bus.\n");
            /* Register the signal handler 'FileTransfer' with the bus*/
            fileTransferMember = fileTransferInterface->GetMember("FileTransfer");
            assert(fileTransferMember);
        } else {
            printf("Failed to Add interface: %s\n", INTERFACE_NAME);
        }

        /* register the signal handler for the the 'FileTransfer' signal */
        if (fileTransferMember) {
            status =  s_busAtt->RegisterSignalHandler(this,
                                                      static_cast<MessageReceiver::SignalHandler>(&FileTransferObject::FileTransferSignalHandler),
                                                      fileTransferMember,
                                                      NULL);
            if (ER_OK == status) {
                printf("Registered signal handler for %s.FileTransfer\n", SERVICE_NAME);
            } else {
                printf("Failed to register signal handler for %s.FileTransfer\n", SERVICE_NAME);
            }
        }
    }

    ~FileTransferObject()
    {
        delete outputStream;
        outputStream = NULL;
    }

    // Write the data supplied by the service to an output file with filename provided
    void FileTransferSignalHandler(const InterfaceDescription::Member* member,
                                   const char* sourcePath,
                                   Message& msg)
    {
        uint8_t* data;
        size_t size;
        msg->GetArg(2)->Get("ay", &size, &data);

        if (size != 0) {
            uint32_t currArray;
            char* filePathAndName;

            msg->GetArg(1)->Get("u", &currArray);
            msg->GetArg(0)->Get("s", &filePathAndName);

            if (NULL == this->outputStream) {
                char* fileName = filePathAndName;
                char* fileNameBack = strrchr(filePathAndName, '\\');
                char* fileNameForward = strrchr(filePathAndName, '/');

                if (fileNameBack && fileNameForward) {
                    fileName = (fileNameBack > fileNameForward ? fileNameBack : fileNameForward) + 1;
                } else {
                    if (fileNameForward) {
                        fileName = fileNameForward + 1;
                    }
                    if (fileNameBack) {
                        fileName = fileNameBack + 1;
                    }
                }

                printf("Opening the output stream to transfer the file '%s'.\n", fileName);
                this->outputStream = new ofstream(fileName, ios::out | ios::binary);
            }

            printf("Array Num : %i\tSize : %u\n", currArray, (unsigned int)size);

            if (this->outputStream->is_open()) {
                this->outputStream->write((char*)data, size);
            }
        } else {
            if (this->outputStream->is_open()) {
                printf("The file was transfered sucessfully.\n");
                this->outputStream->close();
            }

            delete this->outputStream;
            this->outputStream = NULL;
            s_fileTransferComplete = true;
        }
    }

  private:
    // Don't allow default constructors.
    FileTransferObject() : BusObject(NULL)
    {
        assert(0);
    }

    // Don't allow default copy constructors.
    FileTransferObject& operator=(const FileTransferObject& obj)
    {
        return *this;
    }

    ofstream* outputStream;
};

/* Register the bus object, report the result status to stdout, and return the status. */
QStatus RegisterBusObject(FileTransferObject* busObject)
{
    QStatus status = s_busAtt->RegisterBusObject(*busObject);

    if (ER_OK == status) {
        printf("Registration of busObject succeeded.\n");
    } else {
        printf("Registration of busObject failed (%s).\n", QCC_StatusText(status));
    }

    return status;
}

/* Register the bus object, report the event to stdout. */
void RegisterBusListener(MyBusListener* busListener)
{
    s_busAtt->RegisterBusListener(*busListener);

    printf("Registration of Buslistener completed.\n");
}

/** Start the message bus, report the result to stdout, and return the result status. */
QStatus StartMessageBus(void)
{
    QStatus status = s_busAtt->Start();

    if (ER_OK == status) {
        printf("BusAttachment started.\n");
    } else {
        printf("BusAttachment::Start failed.\n");
    }

    return status;
}

/** Handle the connection to the bus, report the result to stdout, and return the result status. */
QStatus ConnectToBus(void)
{
    QStatus status = s_busAtt->Connect();

    if (ER_OK == status) {
        printf("BusAttachment connected to '%s'.\n", s_busAtt->GetConnectSpec().c_str());
    } else {
        printf("BusAttachment::Connect('%s') failed.\n", s_busAtt->GetConnectSpec().c_str());
    }

    return status;
}

/* Begin discovery on the well-known name of the service to be called, report the result to
   stdout, and return the result status. */
QStatus FindAdvertisedName(void)
{
    /* Begin discovery on the well-known name of the service to be called */
    QStatus status = s_busAtt->FindAdvertisedName(SERVICE_NAME);

    if (status == ER_OK) {
        printf("org.alljoyn.Bus.FindAdvertisedName ('%s') succeeded.\n", SERVICE_NAME);
    } else {
        printf("org.alljoyn.Bus.FindAdvertisedName ('%s') failed (%s).\n", SERVICE_NAME, QCC_StatusText(status));
    }

    return status;
}

/* Wait for join session to complete, report the event to stdout, and return the result status. */
QStatus WaitForJoinSessionCompletion(void)
{
    unsigned int count = 0;

    while (!s_joinComplete && !s_interrupt) {
        if (0 == (count++ % 10)) {
            printf("Waited %u seconds for JoinSession completion.\n", count / 10);
        }

#ifdef _WIN32
        Sleep(100);
#else
        usleep(100 * 1000);
#endif
    }

    return s_joinComplete && !s_interrupt ? ER_OK : ER_ALLJOYN_JOINSESSION_REPLY_CONNECT_FAILED;
}

/* Wait for the file transfer to complete, report the event to stdout, and return the result status. */
QStatus WaitForFileTransferComplete(void)
{
    unsigned int count = 0;

    while (!s_fileTransferComplete && !s_interrupt) {
        if (0 == (count++ % 10)) {
            printf("Waited %u seconds for file transfer completion.\n", count / 10);
        }

#ifdef _WIN32
        Sleep(100);
#else
        usleep(100 * 1000);
#endif
    }

    return s_fileTransferComplete && !s_interrupt ? ER_OK : ER_FAIL;
}

int main(int argc, char** argv, char** envArg)
{
    printf("AllJoyn Library version: %s.\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s.\n", ajn::GetBuildInfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    QStatus status = ER_OK;

    /* Create message bus */
    s_busAtt = new BusAttachment("FileTransferClient", true);

    /* This test for NULL is only required if new() behavior is to return NULL
     * instead of throwing an exception upon an out of memory failure.
     */
    if (!s_busAtt) {
        status = ER_OUT_OF_MEMORY;
    }

    FileTransferObject busObject(SERVICE_PATH);

    if (ER_OK == status) {
        status = RegisterBusObject(&busObject);
    }

    if (ER_OK == status) {
        status = StartMessageBus();
    }

    if (ER_OK == status) {
        status = ConnectToBus();
    }

    MyBusListener busListener;

    if (ER_OK == status) {
        RegisterBusListener(&busListener);
        status = FindAdvertisedName();
    }

    if (ER_OK == status) {
        status = WaitForJoinSessionCompletion();
    }

    if (ER_OK == status) {
        status = WaitForFileTransferComplete();
    }


    /* Deallocate bus */
    delete s_busAtt;

    s_busAtt = NULL;

    printf("File Transfer Client exiting with status %d (%s)\n", status, QCC_StatusText(status));

    return (int)status;
}
