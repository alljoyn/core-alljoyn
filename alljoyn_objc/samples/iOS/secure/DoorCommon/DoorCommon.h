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

#import <Foundation/Foundation.h>
#import <alljoyn/SessionListener.h>
#import <alljoyn/InterfaceDescription.h>
#import <alljoyn/MessageReceiver.h>
#import <alljoyn/ProxyBusObject.h>
#import <alljoyn/BusAttachment.h>
#import <alljoyn/TransportMask.h>

#import "ViewController.h"
#import "AJNBusAttachment.h"
#import "AJNBusInterface.h"
#import "AJNProxyBusObject.h"
#import "AJNInterfaceMember.h"
#import "AJNInterfaceDescription.h"
#import "AJNPermissionConfigurationListener.h"

/* Door sample common definitions. */
#define DOOR_INTERFACE  @"sample.securitymgr.door.Door"
#define DOOR_OPEN @"Open"
#define DOOR_CLOSE @"Close"
#define DOOR_GET_STATE @"GetState"
#define DOOR_STATE @"State"
#define DOOR_STATE_CHANGED @"StateChanged"
#define DOOR_SIGNAL_MATCH_RULE  @"type='signal',interface='" DOOR_INTERFACE @"',member='" DOOR_STATE_CHANGED @"'"

#define DOOR_OBJECT_PATH @"/sample/security/Door"

#define KEYX_ECDHE_NULL @"ALLJOYN_ECDHE_NULL"
#define KEYX_ECDHE_PSK @"ALLJOYN_ECDHE_PSK"
#define KEYX_ECDHE_DSA @"ALLJOYN_ECDHE_ECDSA"

#define DOOR_APPLICATION_PORT 12345

/* Door session listener */
class DoorSessionListener :
public ajn::SessionListener {
};

/* Door message receiver */
class DoorMessageReceiver : public ajn::MessageReceiver {
public:
    void DoorEventHandler(const ajn::InterfaceDescription::Member* member,
                          const char* srcPath,
                          ajn::Message& msg)
    {
        const ajn::MsgArg* result;
        result = msg->GetArg(0);
        bool value;
        QStatus status = result->Get("b", &value);
        if (ER_OK != status) {
            NSLog(@"Failed to Get boolean - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        } else {
            NSLog(@"Received door %s event ...\n", value ? "opened" : "closed");
        }
    }
};

static DoorSessionListener theListener;

/* Door session manager */
class DoorSessionManager {
public:

    DoorSessionManager(ajn::BusAttachment* _ba, uint32_t _timeout, ViewController* view) :
    ba(_ba), timeout(_timeout), view(view)
    {
    }

    void MethodCall(const std::string& busName, const std::string& methodName)
    {
        std::shared_ptr<ajn::ProxyBusObject> remoteObj;
        QStatus status = GetProxyDoorObject(busName, remoteObj);
        if (ER_OK != status) {
            NSLog(@"Failed to GetProxyDoorObject - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
            return;
        }

        ajn::Message reply(*ba);
        ajn::MsgArg arg;
        NSLog(@"Calling %s on '%s'\n", methodName.c_str(), busName.c_str());
        status = remoteObj->MethodCall([DOOR_INTERFACE UTF8String], methodName.c_str(),
                                       nullptr, 0, reply, timeout);

        // Retry on policy/identity update
        std::string securityViolation = "org.alljoyn.Bus.SecurityViolation";
        if ((ER_BUS_REPLY_IS_ERROR_MESSAGE == status) &&
            (reply->GetErrorName() != NULL) &&
            (std::string(reply->GetErrorName()) == securityViolation)) {
            status = remoteObj->MethodCall([DOOR_INTERFACE UTF8String], methodName.c_str(),
                                           nullptr, 0, reply, timeout);
        }

        if (ER_OK != status) {
            NSLog(@"Failed to call method %s - status (%@)\n", methodName.c_str(), [AJNStatus descriptionForStatusCode:status]);
            return;
        }

        const ajn::MsgArg* result;
        result = reply->GetArg(0);
        bool value;
        status = result->Get("b", &value);
        if (ER_OK != status) {
            NSLog(@"Failed to Get boolean - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
            return;
        }

        [view didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"%s returned %d\n", methodName.c_str(), value]];
    }

    void GetProperty(const std::string& busName, const std::string& propertyName)
    {
        std::shared_ptr<ajn::ProxyBusObject> remoteObj;
        QStatus status = GetProxyDoorObject(busName, remoteObj);
        if (ER_OK != status) {
            fprintf(stderr, "Failed to GetProxyDoorObject - status (%s)\n",
                    QCC_StatusText(status));
            return;
        }

        ajn::MsgArg arg;
        status = remoteObj->GetProperty([DOOR_INTERFACE UTF8String], propertyName.c_str(),
                                        arg, timeout);

        // Retry on policy/identity update
        if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
            // Impossible to check specific error message (see ASACORE-1811)
            status = remoteObj->GetProperty([DOOR_INTERFACE UTF8String], propertyName.c_str(),
                                            arg, timeout);
        }

        if (ER_OK != status) {
            fprintf(stderr, "Failed to GetProperty %s - status (%s)\n", propertyName.c_str(),
                    QCC_StatusText(status));
            return;
        }

        const ajn::MsgArg* result;
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
        ajn::SessionId id;
        std::shared_ptr<ajn::ProxyBusObject> doorProxy;
    };
    typedef std::map<std::string, Session> SessionsMap;

    ajn::BusAttachment* ba;
    uint32_t timeout;
    SessionsMap sessions;
    ViewController* view;

    QStatus GetProxyDoorObject(const std::string& busName,
                               std::shared_ptr<ajn::ProxyBusObject>& remoteObj)
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

    QStatus JoinSession(const std::string& busName, Session& session)
    {
        QStatus status = ER_FAIL;

        ajn::SessionOpts opts(ajn::SessionOpts::TRAFFIC_MESSAGES, false,
                              ajn::SessionOpts::PROXIMITY_ANY, ajn::TRANSPORT_ANY);
        status = ba->JoinSession(busName.c_str(), DOOR_APPLICATION_PORT,
                                 &theListener, session.id, opts);
        if (ER_OK != status) {
            fprintf(stderr, "Failed to JoinSession - status (%s)\n", QCC_StatusText(status));
            return status;
        }

        const ajn::InterfaceDescription* remoteIntf = ba->GetInterface([DOOR_INTERFACE UTF8String]);
        if (!remoteIntf) {
            status = ER_FAIL;
            fprintf(stderr, "Failed to GetInterface\n");
            return status;
        }

        session.doorProxy = std::make_shared<ajn::ProxyBusObject>(*ba, busName.c_str(),
                                                        [DOOR_OBJECT_PATH UTF8String], session.id);
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

@interface DoorAboutListener : NSObject<AJNAboutListener>

@property (nonatomic, strong) NSMutableSet* doors;

- (id)init;
- (void)removeDoorName:(NSString*)doorName;

@end

@interface DoorCommonPCL : NSObject<AJNPermissionConfigurationListener>

- (id)initWithBus:(AJNBusAttachment*)bus;

- (QStatus)waitForClaimedState;

@end

@interface SPListener : NSObject

@end

/* Door bus object that emulates a door on the bus; it opens and closes, etc. */

@protocol AJNDoorDelegate <AJNBusInterface>

@property (nonatomic, readonly) BOOL IsOpen;

- (void)open:(AJNMessage *)methodCallMessage;
- (void)close:(AJNMessage *)methodCallMessage;

@end

@interface Door : AJNBusObject<AJNDoorDelegate>

@property (nonatomic, readonly) BOOL IsOpen;
@property (nonatomic) BOOL autoSignal;

- (QStatus)initialize;
- (id)initWithBus:(AJNBusAttachment*)bus;
- (id)initWithBus:(AJNBusAttachment *)bus andView:(ViewController*)view;
- (void)open:(AJNMessage *)methodCallMessage;
- (void)close:(AJNMessage *)methodCallMessage;

- (QStatus)sendDoorEvent;

@end

@interface DoorCommon : NSObject

@property (nonatomic, strong) NSString* AppName;
@property (nonatomic, strong) AJNBusAttachment* BusAttachment;
@property (nonatomic, readonly) AJNInterfaceMember* DoorSignal;

- (id)initWithAppName:(NSString*)appName;

- (id)initWithAppName:(NSString*)appName andView:(ViewController*)view;

- (QStatus)initialize:(BOOL)provider withListener:(id<AJNPermissionConfigurationListener>)inPcl;

- (AJNInterfaceMember*)getDoorSignal;

- (QStatus)announceAbout;

- (QStatus)setSecurityForClaimedMode;

+ (QStatus)updateDoorProviderManifest:(DoorCommon*)common;

@end
