/**
 * @file
 * User/Kernel communication declarations for Windows
 */

/******************************************************************************
 * Copyright (c) 2011, AllSeen Alliance. All rights reserved.
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

#ifndef _ALLJOYN_USERKERNELCOMM_H
#define _ALLJOYN_USERKERNELCOMM_H

#ifndef NTSTATUS
typedef long NTSTATUS;
#endif

#include "..\..\inc\Status.h"

#include <ws2bth.h>
#include <Guiddef.h>

// {B88B4034-DE8B-45A3-B5B5-1B61CEBCBBC8}
DEFINE_GUID(WINDOWS_BLUETOOTH_DEVICE_INTERFACE, 0xb88b4034, 0xde8b, 0x45a3, 0xb5, 0xb5, 0x1b, 0x61, 0xce, 0xbc, 0xbb, 0xc8);

/**
 * This is the maximum number of simultanous open Bluetooth L2 Cap channels we
 * allocate resources for.
 */
#define MAX_OPEN_L2CAP_CHANNELS 20

#define READ_TIMEOUT_IN_MILLISECONDS 10000

// The control code used to send a message to the kernel.
// Device type -- in the "User Defined" range."
#define ALLJOYN_TYPE 0x9000
// The IOCTL function codes from 0x800 to 0xFFF are for customer use.
#define IOCTL_ALLJOYN_MESSAGE CTL_CODE(ALLJOYN_TYPE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * This is the different states an individual L2CAP channel context could be in.
 */
typedef enum _L2CAP_CHANNEL_STATE_TYPE {
    CHAN_STATE_NONE,             // The channel context is not in use and may be allocated.
    CHAN_STATE_NONE_PENDING,     // The L2CAP channel is in the process of being disconnected.
    CHAN_STATE_READ_READY,       // There was a change in number of bytes of data in the buffer.
    CHAN_STATE_L2CAP_EVENT,      // An incoming L2CAP connection request has been received.
    CHAN_STATE_ACCEPT_COMPLETE,  // The acceptance of an incoming L2CAP connection request is
                                 // complete.
    CHAN_STATE_CONNECT_COMPLETE, // The outgoing connection request has been completed.
    CHAN_STATE_CLOSED,           // The L2CAP channel has being disconnected but user mode has
                                 // not been informed.
} L2CAP_CHANNEL_STATE_TYPE;

/**
 * This is the complete set of all commands sent to/from user mode from/to kernel mode.
 */
typedef enum {
    // The first set of commands are from user mode to kernel mode.
    USRKRNCMD_SETMESSAGEEVENT,  // Tells the kernel the event to signal when a message is waiting.
    USRKRNCMD_GETMESSAGE,       // Get the waiting message from the kernel.
    USRKRNCMD_STARTCONNECTABLE,
    USRKRNCMD_STOPCONNECTABLE,
    USRKRNCMD_ACCEPT,
    USRKRNCMD_CONNECT,
    USRKRNCMD_DISCONNECT,

    // ISMASTER and REQUESTROLECHANGE are unsupported but are left here to preserve compatiability
    // with existing drivers and for possible future use.
    USRKRNCMD_ISMASTER,
    USRKRNCMD_REQUESTROLECHANGE,

    USRKRNCMD_READ,             // Read the waiting data.
    USRKRNCMD_WRITE,            // Write the given data.
    USRKRNCMD_GET_STATE,        // Get the current status of the kernel.

    // These are kernel mode to user mode commands.
    KRNUSRCMD_L2CAP_EVENT = 0x100,  // Tells user mode a connection request has been made.
    KRNUSRCMD_ACCEPT_COMPLETE,      // Tells user mode the accept of a L2Cap connection is complete.
    KRNUSRCMD_CONNECT_COMPLETE,     // Tells user mode a connect request is complete.
    KRNUSRCMD_READ_READY,           // Tells user mode there is data ready to be read.
    KRNUSRCMD_BAD_MESSAGE,          // Tells user mode it received a bad message or something wrong happened.
} USER_KERNEL_COMMAND;

// In the WDK (DDK) the l2cap channel handle is defined as a void *.
// Rather than include a WDK file in the user mode code we redefine it here.
#ifndef L2CAP_CHANNEL_HANDLE
typedef  void*L2CAP_CHANNEL_HANDLE;
#endif

// This will increment with driver changes that are incompatiable with the current user mode
// code.
const int DRIVER_VERSION = 2;
const BOOLEAN IS_64BIT = sizeof(void*) == 8;

/**
 * This structure is used for sending the message event handle to the kernel.
 */
struct _USRKRNCMD_SETMESSAGEEVENT {
    HANDLE eventHandle;
};

/**
 * This structure is used for a connect (L2CAP) message from kernel mode to user.
 */
struct _USRKRNCMD_STARTCONNECTABLE {
    // Return value from the command.
    USHORT psm;
};

/**
 * This structure is used for an accept message from user mode to kernel.
 */
struct _USRKRNCMD_ACCEPT {
    // The channel handle of the currently waiting connection.
    L2CAP_CHANNEL_HANDLE channelHandle;

    // The address of the currently waiting connection.
    BTH_ADDR address;
};

/**
 * This structure is used for an accept message from user mode to kernel
 * and the response back to user mode.
 */
struct _USRKRNCMD_CONNECT {
    // Valid when sent to kernel mode.
    // The address of the currently waiting connection.
    BTH_ADDR address;

    // Valid when sent to kernel mode.
    // The psm to connect to.
    USHORT psm;

    // Valid when sent to user mode.
    // The channel handle of the currently waiting connection.
    L2CAP_CHANNEL_HANDLE channelHandle;
};

struct _USRKRNCMD_DISCONNECT {
    // The channel handle of the connection to disconnect.
    L2CAP_CHANNEL_HANDLE channelHandle;

    // The address of the connection to disconnect.
    BTH_ADDR address;
};

struct _USRKRNCMD_READ {
    // The channel handle of the data to be read.
    L2CAP_CHANNEL_HANDLE channelHandle;

    // The number of bytes of data to read.
    size_t bytesOfData;
};

struct _USRKRNCMD_WRITE {
    // The channel handle the data is to be written to.
    L2CAP_CHANNEL_HANDLE channelHandle;

    // The number of bytes of data to write.
    size_t bytesOfData;

    // Set to the NT error value on output for better diagnostics.
    NTSTATUS ntStatus;

    // The size of the buffer allocated for this structure will be
    // sizeof(_USRKRNCMD_WRITE) + bytesOfData - 1. The first byte will be
    // at data[0] with the remaining bytes in the over allocated struct.
    // Hence this member must always be at the end of the structure.
    BYTE data[1];
};

typedef struct _L2CAP_CHANNEL_STATE {
    // The address of the connecting device.
    BTH_ADDR address;

    // The channelHandle for the open L2Cap channel.
    L2CAP_CHANNEL_HANDLE channelHandle;

    // Flags that indicate the nature of the channel such as master or slave. These flags are
    // defined in the Windows Driver Kit file BthDdi.h.
    ULONG channelFlags;

    // Configuration results.
    USHORT outgoingMtus;
    USHORT incomingMtus;

    // This indicates the state type for this channel.
    L2CAP_CHANNEL_STATE_TYPE stateType;

    // Used for Accept and Connect completion status.
    // Set to ER_OK if the connection was successful.
    // Set to ER_SOCK_OTHER_END_CLOSED if the channel is closed.
    QStatus status;

    // Used for Accept and Connect completion status.
    // Set to the NT error value for better diagnostics.
    NTSTATUS ntStatus;

    // Number of bytes in the buffer.
    ULONG bytesInBuffer;
} L2CAP_CHANNEL_STATE;

/**
 * This structure is used get information from the kernel about it's current status.
 */
struct _USRKRNCMD_GET_STATE {
    // The handle used to signal to user mode that a message is waiting.
    HANDLE eventHandle;

    // The dynamically allocated psm.
    USHORT psm;

    // The handle of our l2Cap server.
    // This is used during register server and deregister server.
    void* l2CapServerHandle;

    L2CAP_CHANNEL_STATE channelState[MAX_OPEN_L2CAP_CHANNELS];
};

/**
 * This structure is used for a connect (L2CAP) message from kernel mode to user.
 */
struct _KRNUSRCMD_L2CAP_EVENT {
    // The channel handle of the currently waiting connection.
    L2CAP_CHANNEL_HANDLE channelHandle;

    // The address of the currently waiting connection.
    BTH_ADDR address;
};

/**
 * This structure is used to indicate the status of the accept of a L2Cap connnection has
 * completed. The message is sent from kernel mode to user.
 */
struct _KRNUSRCMD_ACCEPT_COMPLETE {
    // If successful the channel handle of the completed connection.
    L2CAP_CHANNEL_HANDLE channelHandle;

    // If successful the address of the completed connection.
    BTH_ADDR address;

    // Set to ER_OK if the connection was successful.
    QStatus status;

    // Set to the NT error value for better diagnostics.
    NTSTATUS ntStatus;
};

/**
 * This structure is used to indicate the status of connection request of a L2Cap
 * connnection which has just been completed. The message is sent from kernel mode to user.
 */
struct _KRNUSRCMD_CONNECT_COMPLETE {
    // If successful the channel handle of the completed connection.
    L2CAP_CHANNEL_HANDLE channelHandle;

    // If successful the address of the completed connection.
    BTH_ADDR address;

    // Set to ER_OK if the connection was successful.
    QStatus status;

    // Set to the NT error value for better diagnostics.
    NTSTATUS ntStatus;
};

/**
 * This structure is used to indicate that data is ready for reading.
 */
struct _KRNUSRCMD_READ_READY {
    // The channel handle that has the incoming data.
    L2CAP_CHANNEL_HANDLE channelHandle;

    // The address that has the incoming data.
    BTH_ADDR address;

    // The number of bytes of data.
    size_t bytesOfData;

    // Set to ER_OK if the read was successful.
    QStatus status;
};

/**
 * This structure is used to indicate something unexpected happened in the drive.
 */
struct _KRNUSRCMD_BAD_MESSAGE {
    // The line number in the driver where the event happened.
    unsigned int lineNumber;
};

/**
 * This structure is used for messages sent back and forth between user and kernel.
 * The command, USRKRNCMD_XXXX, determines which member of the union is valid.
 */
typedef struct _USER_KERNEL_MESSAGE {
    // The first three members are at the beginning of the structure to be compatiable
    // for mixed 32/64-bit user/kernel combinations.
    union {
        USER_KERNEL_COMMAND command;    // Valid as an input message.
        QStatus status;                 // Valid as an output message.
    } commandStatus;

    // As an input message this is the version expected by user mode code.
    // As an output message this is the negative of the version expected by kernel mode code.
    int version;

    // On input true if user mode is 64-bit.
    // On output true if kernel mode is 64-bit.
    BOOLEAN is64Bit;

    // Which structure is valid depends on the command and whether it is
    // on the input to a command or the return from a command.
    union {
        // User mode to kernel messages.
        struct _USRKRNCMD_SETMESSAGEEVENT setMessageEventData;
        struct _USRKRNCMD_STARTCONNECTABLE startConnectableData;
        struct _USRKRNCMD_ACCEPT acceptData;
        struct _USRKRNCMD_CONNECT connectData;
        struct _USRKRNCMD_DISCONNECT disconnectData;
        struct _USRKRNCMD_READ read;
        struct _USRKRNCMD_WRITE write;
        struct _USRKRNCMD_GET_STATE state;

        // Kernel to user messages.
        struct _KRNUSRCMD_L2CAP_EVENT l2capeventData;
        struct _KRNUSRCMD_ACCEPT_COMPLETE acceptComplete;
        struct _KRNUSRCMD_CONNECT_COMPLETE connectComplete;
        struct _KRNUSRCMD_READ_READY readReady;
        struct _KRNUSRCMD_BAD_MESSAGE badMessage;
    } messageData;

} USER_KERNEL_MESSAGE;

#endif