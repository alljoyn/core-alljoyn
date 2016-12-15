////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
//    Source Project (AJOSP) Contributors and others.
//
//    SPDX-License-Identifier: Apache-2.0
//
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0
//
//    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
//    Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for
//    any purpose with or without fee is hereby granted, provided that the
//    above copyright notice and this permission notice appear in all
//    copies.
//
//     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//     PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import <alljoyn/AboutObjectDescription.h>
#import "AJNAboutObjectDescription.h"

using namespace ajn;

@interface AJNAboutObjectDescription()
@property (nonatomic, readonly) AboutObjectDescription *aboutObjectDescription;
@end


@interface AJNMessageArgument(Private)

@property (nonatomic, readonly) MsgArg *msgArg;

@end

@interface AJNObject(Private)

@property (nonatomic) BOOL shouldDeleteHandleOnDealloc;

@end

@implementation AJNAboutObjectDescription

/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
- (AboutObjectDescription*)aboutObjectDescription
{
    return static_cast<AboutObjectDescription*>(self.handle);
}

- (id)init
{
    self = [super init];
    if (self) {
        self.handle = new AboutObjectDescription();
        self.shouldDeleteHandleOnDealloc = YES;
    }
    return self;
}

- (id)initWithMsgArg:(AJNMessageArgument *)msgArg
{
    self = [super init];
    if (self) {
        self.handle = new AboutObjectDescription(*msgArg.msgArg);
        self.shouldDeleteHandleOnDealloc = YES;
    }
    return self;
}

- (void)dealloc
{
    if (self.shouldDeleteHandleOnDealloc) {
        AboutObjectDescription *ptr = (AboutObjectDescription*)self.handle;
        delete ptr;
        self.handle = nil;
    }
}

- (QStatus)createFromMsgArg:(AJNMessageArgument *)msgArg
{
    return self.aboutObjectDescription->CreateFromMsgArg(*msgArg.msgArg);
}

- (NSArray*)paths
{
    size_t pathCount = self.aboutObjectDescription->GetPaths(NULL, 0);
    const char** ajPaths = new const char*[pathCount];
    NSMutableArray *paths = [[NSMutableArray alloc] initWithCapacity:pathCount];
    self.aboutObjectDescription->GetPaths(ajPaths, pathCount);
    
    for (int i=0 ; i < pathCount ; i++) {
        NSString *objPath = [NSString stringWithCString:ajPaths[i] encoding:NSUTF8StringEncoding];
        [paths addObject:objPath];
    }
    
    delete [] ajPaths;
    
    return paths;
}

- (NSMutableArray*)getInterfacesForPath:(NSString *)path
{
    size_t interfaceCount = self.aboutObjectDescription->GetInterfaces([path UTF8String], NULL, 0);
    const char** ifaces = new const char*[interfaceCount];
    NSMutableArray *interfaces = [[NSMutableArray alloc] initWithCapacity:interfaceCount];
    self.aboutObjectDescription->GetInterfaces([path UTF8String], ifaces, interfaceCount);
    
    for (int i=0 ; i < interfaceCount ; i++) {
        NSString *ifacePath = [NSString stringWithCString:ifaces[i] encoding:NSUTF8StringEncoding];
        [interfaces addObject:ifacePath];
    }
    
    delete [] ifaces;
    
    return interfaces;
}

- (NSMutableArray*)getInterfacePathsForInterface:(NSString *)interface
{
    size_t pathCount = self.aboutObjectDescription->GetInterfacePaths([interface UTF8String], NULL, 0);
    const char** interfacePaths = new const char*[pathCount];
    NSMutableArray *paths = [[NSMutableArray alloc] initWithCapacity:pathCount];
    self.aboutObjectDescription->GetInterfacePaths([interface UTF8String], interfacePaths, pathCount);
    
    for (int i=0 ; i < pathCount ; i++) {
        NSString *objPath = [NSString stringWithCString:interfacePaths[i] encoding:NSUTF8StringEncoding];
        [paths addObject:objPath];
    }
    
    delete [] interfacePaths;
    
    return paths;
}

- (void)clear
{
    return self.aboutObjectDescription->Clear();
}

- (BOOL)hasPath:(NSString*)path
{
    return self.aboutObjectDescription->HasPath([path UTF8String]) ? YES : NO;
}

- (BOOL)hasInterface:(NSString*)interface
{
    return self.aboutObjectDescription->HasInterface([interface UTF8String]) ? YES : NO;
}

- (BOOL)hasInterface:(NSString*)interface withPath:(NSString*)path
{
    return self.aboutObjectDescription->HasInterface([path UTF8String], [interface UTF8String]) ? YES : NO;
}

- (QStatus)getMsgArg:(AJNMessageArgument *)msgArg
{
    return self.aboutObjectDescription->GetMsgArg(msgArg.msgArg);
}

@end