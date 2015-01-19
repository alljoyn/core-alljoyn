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

#ifndef SAMPLE_DOOR_COMMON_H_
#define SAMPLE_DOOR_COMMON_H_

#define DOOR_INTF_SECURE 1

#include <qcc/GUID.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/AuthListener.h>
#include <alljoyn/about/AboutPropertyStoreImpl.h>

#define DOOR_INTERFACE "sample.securitymgr.door.Door"
#define DOOR_OPEN "Open"
#define DOOR_CLOSE "Close"
#define DOOR_GET_STATE "GetState"
#define DOOR_STATE "State"
#define DOOR_STATE_CHANGED "StateChanged"
#define DOOR_SIGNAL_MATCH_RULE  "type='signal',interface='" DOOR_INTERFACE "',member='" DOOR_STATE_CHANGED "'"

#define DOOR_OBJECT_PATH "/sample/security/Door"

#define KEYX_ECDHE_NULL "ALLJOYN_ECDHE_NULL"
#define KEYX_ECDHE_DSA "ALLJOYN_ECDHE_ECDSA"

#define DOOR_APPLICATION_PORT 12345
using namespace ajn;
using namespace services;

namespace sample {
namespace securitymgr {
namespace door {
/**
 * Class to allow authentication mechanisms to interact with the user or application.
 */

class DoorAuthListener :
    public AuthListener {
  public:

    DoorAuthListener() { }

    bool RequestCredentials(const char* authMechanism,
                            const char* authPeer,
                            uint16_t authCount,
                            const char* userId,
                            uint16_t credMask,
                            Credentials& creds);

    bool VerifyCredentials(const char* authMechanism,
                           const char* authPeer,
                           const Credentials& creds);

    void AuthenticationComplete(const char* authMechanism,
                                const char* authPeer,
                                bool success);
};

class Door :
    public ajn::BusObject {
  public:
    Door(BusAttachment& ba);

    QStatus RegisterToAbout();

    ~Door() { }

  protected:
    QStatus Get(const char* ifcName,
                const char* propName,
                MsgArg& val);

  private:
    bool open; //open = true, close = false
    const InterfaceDescription::Member* stateSignal;

    void Open(const ajn::InterfaceDescription::Member* member,
              ajn::Message& msg);

    void Close(const ajn::InterfaceDescription::Member* member,
               ajn::Message& msg);

    void GetState(const ajn::InterfaceDescription::Member* member,
                  ajn::Message& msg);

    void SendDoorEvent(bool newState);

    void ReplyWithBoolean(bool answer,
                          Message& msg);
};

class DoorCommon {
  public:
    DoorCommon(const char* appName) :
        ba(appName, true), appName(appName)
    {
    }

    ~DoorCommon();

    QStatus init(const char* keyStoreName,
                 bool provider);

    const InterfaceDescription::Member* GetDoorSignal()
    {
        return ba.GetInterface(DOOR_INTERFACE)->GetMember(DOOR_STATE_CHANGED);
    }

    BusAttachment& GetBusAttachment()
    {
        return ba;
    }

  private:
    QStatus CreateInterface();

    QStatus AdvertiseApplication();

    QStatus FillAboutPropertyStore();

    AboutPropertyStoreImpl aboutPropertyStore;

    BusAttachment ba;

    const char* appName;
};
}
}
}

#endif /* SAMPLE_DOOR_COMMON_H_ */
