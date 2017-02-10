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
#import "DoorCommonPCL.h"
#import "AJNPermissionConfigurator.h"

@interface DoorCommonPCL() <AJNPermissionConfigurationListener>

@property (nonatomic, strong) AJNBusAttachment *bus;
@property (nonatomic, strong) NSCondition *condition;
@property (nonatomic) BOOL isServiceInClamedMode;
@property (nonatomic, weak) id<DoorServiceToogleToClaimedModeListener> doorService;

@end

@implementation DoorCommonPCL

- (id)initWithBus:(AJNBusAttachment*)bus service:(id<DoorServiceToogleToClaimedModeListener>)doorService
{
    self = [super init];
    if (self != nil) {
        _bus = bus;
        _condition = [[NSCondition alloc] init];
        _doorService = doorService;
        _isServiceInClamedMode = NO;
    }

    return self;
}

- (void)startManagement
{
    NSLog(@"StartManagement called.\n");
}

- (void)endManagement
{
    NSLog(@"EndManagement called.\n");
    [self.condition lock];
    AJNApplicationState appState;
    QStatus status = [self.bus.permissionConfigurator getApplicationState:&appState];
    if (status == ER_OK) {
        if (appState == CLAIMED) {
            if (!_isServiceInClamedMode) {
                [self.condition signal];
            }
        } else {
            NSLog(@"App not claimed after management finished. Continuing to wait.\n");
        }
    } else {
        NSLog(@"Failed to GetApplicationState - status (%s)\n", QCC_StatusText(status));
    }
    [self.condition unlock];
}

- (QStatus)factoryReset
{
    NSLog(@"Factory reset called but not implemented");
    return ER_OK;
}

- (void)didPolicyChange
{
    NSLog(@"Policy change called but not implemented");
}

- (QStatus)waitForClaimedState
{
    [self.condition lock];
    AJNApplicationState appState;
    QStatus status = [self.bus.permissionConfigurator getApplicationState:&appState];
    if (ER_OK != status) {
        NSLog(@"Failed to GetApplicationState - status (%s)\n",
              QCC_StatusText(status));
        [self.condition unlock];
        return status;
    }

    if (appState == CLAIMED) {
        NSLog(@"Already claimed !\n");
        [self.condition unlock];
        return ER_OK;
    }

    NSLog(@"Waiting to be claimed...\n");
    [self.condition wait];

    [self.condition waitUntilDate:[[NSDate alloc] initWithTimeIntervalSinceNow:1000]]; //wait to be sure that all end managament messages are delivered

    if (!_isServiceInClamedMode) {
        status = [_doorService setSecurityForClaimedMode];
        if (status == ER_OK) {
            _isServiceInClamedMode = YES;
        } else {
            NSLog(@"Failed to SetSecurityForClaimedMode - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
            return status;
        }
    }

    if (ER_OK != status) {
        [self.condition unlock];
        return status;
    }

    NSLog(@"Claimed !\n");
    [self.condition unlock];
    return ER_OK;
}

@end
