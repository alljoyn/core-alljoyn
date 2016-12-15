////////////////////////////////////////////////////////////////////////////////
// // Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
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
#import "AJNSessionOptions.h"
#import "AJNObject.h"
#import "AJNBusObject.h"
#import "AJNStatus.h"
#import "AJNAboutDataListener.h"
#import "AJNMessageArgument.h"


@interface AJNAboutObjectDescription : AJNObject

/**
 * Get a list of the paths that are added to this AboutObjectDescription.
 * @return paths found in the AboutObjectDescription
 */
@property (nonatomic, readonly) NSArray *paths;

/**
 * Fill in the ObjectDescription fields using a MsgArg
 *
 * The MsgArg must contain an array of type a(oas) The expected use of this
 * class is to fill in the ObjectDescription using a MsgArg obtain from the Announce
 * signal or the GetObjectDescription method from org.alljoyn.About interface.
 *
 * If the arg came from the org.alljoyn.About.Announce signal or the
 * org.alljoyn.AboutGetObjectDescrption method then it can be used to create
 * the AboutObjectDescription. If the arg came from any other source its best
 * to create an empty AboutObjectDescrption class and use the CreateFromMsgArg
 * class to access the MsgArg. Since it can be checked for errors while parsing
 * the MsgArg.
 *
 * @param msgArg MsgArg contain About ObjectDescription
 */
- (id)initWithMsgArg:(AJNMessageArgument *)msgArg;

/**
 * Fill in the ObjectDescription fields using a MsgArg
 *
 * The MsgArg must contain an array of type a(oas) The expected use of this
 * class is to fill in the ObjectDescription using a MsgArg obtain from the Announce
 * signal or the GetObjectDescription method from org.alljoyn.about interface.
 *
 * @param msgArg MsgArg contain AboutData dictionary
 *
 * @return ER_OK on success
 */
- (QStatus)createFromMsgArg:(AJNMessageArgument *)msgArg;

/**
 * Get a list of interfaces advertised at the given path that are part of
 * this AboutObjectDescription.
 *
 * usage example
 * @code
 * size_t numInterfaces = [aboutObjectDescription getInterfacesForPath:@"/basic_object" interfaces:nil numOfInterfaces:0];
 * NSMutableArray *interfacePaths = [[NSMutableArray alloc] initWithCapacity:numPaths];
 * [aboutObjectDescription getInterfacePathsForInterface:@"com.alljoyn.example" paths:&interfacePaths numOfPaths:numInterfaces]
 * @endcode
 *
 * @param[in]  path the path we want to get a list of interfaces for
 *
 * @return
 *    The total number of interfaces found in the AboutObjectDescription for
 *    the specified path.  If this number is larger than `numInterfaces`
 *    then only `numInterfaces` of interfaces will be returned in the
 *    `interfaces` array.
 */
- (NSMutableArray*)getInterfacesForPath:(NSString *)path;

/**
 * Get a list of the paths for a given interface. Its possible to have the
 * same interface listed under multiple paths.
 *
 * Usage example
 * @code
 * size_t numPaths = [aboutObjectDescription getInterfacePathsForInterface:@"com.alljoyn.example" paths:nil numOfPaths:0];
 * NSMutableArray *interfacePaths = [[NSMutableArray alloc] initWithCapacity:numPaths];
 * [aboutObjectDescription getInterfacePathsForInterface:@"org.alljoyn.example" paths:&interfacePaths numOfPaths:numPaths]
 * @endcode
 *
 * @param[in]  interface the interface we want to get a list of paths for
 *
 * @return
 *    The total number of paths found in the AboutObjectDescription for
 *    the specified path.  If this number it larger than the `numPaths`
 *    then only `numPaths` of interfaces will be returned in the `paths`
 *    array
 *
 */
- (NSMutableArray*)getInterfacePathsForInterface:(NSString *)interface;

/**
 * Clear all the contents of this AboutObjectDescription
 *
 */
- (void)clear;

/**
 * Returns true if the given path is found
 *
 * @param[in] path BusObject path
 *
 * @return true if the path is found
 */
- (BOOL)hasPath:(NSString*)path;

/**
 * Returns true if the given interface name is found in any path
 *
 * @param[in] interface the name of the interface you are looking for
 *
 * @return true if the interface is found
 */
- (BOOL)hasInterface:(NSString*)interface;

/**
 * Returns true if the given interface name is found at the given path
 *
 * @param[in] interface the name of the interface you are looking for
 * @param[in] path of the interface
 *
 * @return true if the interface is found at the given path
 */
- (BOOL)hasInterface:(NSString*)interface withPath:(NSString*)path;

/**
 * @param[out] msgArg containing a signature a(oas)
 *                    an array of object paths and an array of interfaces
 *                    found on that object path
 *
 * @return ER_OK if successful
 */
- (QStatus)getMsgArg:(AJNMessageArgument *)msgArg;

@end



