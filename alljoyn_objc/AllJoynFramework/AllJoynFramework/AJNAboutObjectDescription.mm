////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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

- (size_t)getPaths:(NSMutableArray **)path withSize:(size_t)numOfPaths
{
    size_t pathCount = self.aboutObjectDescription->GetPaths(NULL, 0);
    if (path == nil && numOfPaths == 0) {
        return pathCount;
    }
    numOfPaths = (pathCount < numOfPaths) ? pathCount : numOfPaths;
    const char** paths = new const char*[numOfPaths];
    pathCount = self.aboutObjectDescription->GetPaths(paths, numOfPaths);
    for (int i=0 ; i < numOfPaths ; i++) {
        NSString *objPath = [[NSString alloc] initWithUTF8String:paths[i]];
        [*path addObject:objPath];
    }
    delete []paths;
    return pathCount;
}

- (size_t)getInterfacesForPath:(NSString *)path interfaces:(NSMutableArray **)interfaces numOfInterfaces:(size_t)numOfInterfaces
{
    size_t interfaceCount = self.aboutObjectDescription->GetInterfaces([path UTF8String], NULL, 0);
    if (interfaces == nil && numOfInterfaces == 0) {
        return interfaceCount;
    }
    numOfInterfaces = (interfaceCount < numOfInterfaces) ? interfaceCount : numOfInterfaces;
    const char** ifaces = new const char*[numOfInterfaces];
    interfaceCount = self.aboutObjectDescription->GetInterfaces([path UTF8String], ifaces, numOfInterfaces);
    for (int i=0 ; i < numOfInterfaces ; i++) {
        NSString *ifacePath = [[NSString alloc] initWithUTF8String:ifaces[i]];
        [*interfaces addObject:ifacePath];
    }
    delete []ifaces;
    return interfaceCount;
}

- (size_t)getInterfacePathsForInterface:(NSString *)interface paths:(NSMutableArray **)paths numOfPaths:(size_t)numOfPaths
{
    size_t pathCount = self.aboutObjectDescription->GetInterfacePaths([interface UTF8String], NULL, 0);
    if (paths == nil && numOfPaths == 0) {
        return pathCount;
    }
    numOfPaths = (pathCount < numOfPaths) ? pathCount : numOfPaths;
    const char** interfacePaths = new const char*[numOfPaths];
    pathCount = self.aboutObjectDescription->GetInterfacePaths([interface UTF8String], interfacePaths, numOfPaths);
    for (int i=0 ; i < numOfPaths ; i++) {
        NSString *objPath = [[NSString alloc] initWithUTF8String:interfacePaths[i]];
        [*paths addObject:objPath];
    }
    delete []interfacePaths;
    return pathCount;
}

- (void)clear
{
    return self.aboutObjectDescription->Clear();
}

- (BOOL)hasPath:(const char *)path
{
    return self.aboutObjectDescription->HasPath(path) ? YES : NO;
}

- (BOOL)hasInterface:(const char *)interface
{
    return self.aboutObjectDescription->HasInterface(interface) ? YES : NO;
}

- (BOOL)hasInterface:(const char *)interface withPath:(const char *)path
{
    return self.aboutObjectDescription->HasInterface(path, interface) ? YES : NO;
}

- (QStatus)getMsgArg:(AJNMessageArgument *)msgArg
{
    return self.aboutObjectDescription->GetMsgArg(msgArg.msgArg);
}

@end