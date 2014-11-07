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

@implementation AJNAboutObjectDescription

- (id)initWithMsgArg:(AJNMessageArgument *)msgArg
{
    self = [super init];
    if (self) {
        self.handle = new AboutObjectDescription(*msgArg.msgArg);
    }
    return self;
}

- (QStatus)createFromMsgArg:(AJNMessageArgument *)msgArg
{
    return self.aboutObjectDescription->CreateFromMsgArg(*msgArg.msgArg);
}

- (size_t)getPaths:(const char **)path withSize:(size_t)numOfPaths
{
    return self.aboutObjectDescription->GetPaths(path, numOfPaths);
}

- (size_t)getInterfacesForPath:(const char *)path interfaces:(const char **)interfaces numOfInterfaces:(size_t)numOfInterfaces
{
    return self.aboutObjectDescription->GetInterfaces(path, interfaces, numOfInterfaces);
}

- (size_t)getInterfacePathsForInterface:(const char *)interface paths:(const char**)paths numOfPaths:(size_t)numOfPaths
{
    return self.aboutObjectDescription->GetInterfacePaths(interface, paths, numOfPaths);
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