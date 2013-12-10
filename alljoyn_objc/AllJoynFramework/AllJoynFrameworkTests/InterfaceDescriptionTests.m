////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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

#import "InterfaceDescriptionTests.h"
#import "AJNBusAttachment.h"
#import "AJNInterfaceDescription.h"

static NSString * const kInterfaceName = @"org.alljoyn.bus.objc.tests.NNNNNNEEEEEEEERRRRRRRRRRDDDDDDDSSSSSSSS";
static NSString * const kInterfaceMethod = @"LaughObnoxiously";
static NSString * const kInterfaceMethodArgNames = @"volumeOfLaugh,annoyedCoworkersCount";
static NSString * const kInterfaceMethodInputSignature = @"i";
static NSString * const kInterfaceMethodOutputSignature = @"i";
static NSString * const kInterfaceSignal = @"FigdetingNervously";
static NSString * const kInterfaceSignalArgNames = @"levelOfAwkwardness";
static NSString * const kInterfaceSignalInputSignature = @"s";
static NSString * const kInterfaceProperty = @"NerdinessRating";
static NSString * const kInterfacePropertySignature = @"i";
static const AJNInterfacePropertyAccessPermissionsFlags kInterfacePropertyAccessPermissions = kAJNInterfacePropertyAccessReadFlag;
static NSString * const kInterfaceXML = @"<interface name=\"org.alljoyn.bus.objc.tests.NNNNNNEEEEEEEERRRRRRRRRRDDDDDDDSSSSSSSS\">\
    <signal name=\"FigdetingNervously\">\
        <arg name=\"levelOfAwkwardness\" type=\"s\"/>\
    </signal>\
    <method name=\"LaughObnoxiously\">\
        <arg name=\"volumeOfLaugh\" type=\"i\" direction=\"in\"/>\
        <arg name=\"annoyedCoworkersCount\" type=\"i\" direction=\"out\"/>\
    </method>\
    <property name=\"NerdinessRating\" type=\"i\" access=\"read\"/>\
</interface>";

@interface InterfaceDescriptionTests() <AJNBusListener>

@property (nonatomic, strong) AJNBusAttachment *bus;

@end

@implementation InterfaceDescriptionTests

@synthesize bus = _bus;

- (void)setUp
{
    [super setUp];
    
    // Set-up code here. Executed before each test case is run.
    //
    self.bus = [[AJNBusAttachment alloc] initWithApplicationName:@"testApp" allowRemoteMessages:YES];
}

- (void)tearDown
{
    // Tear-down code here. Executed after each test case is run.
    //    
    [self.bus destroy];
    self.bus = nil;
    
    [super tearDown];
}


- (void)testShouldCreateInterface
{
    AJNInterfaceDescription *iface = [self.bus createInterfaceWithName:kInterfaceName enableSecurity:NO];
    STAssertNotNil(iface, @"Bus failed to create interface.");
    
    [iface activate];
    
    iface = [self.bus interfaceWithName:kInterfaceName];
    STAssertNotNil(iface, @"Bus failed to retrieve interface that had already been created.");
}

- (void)testShouldListMembersAndPropertiesAddedToInterfaceViaXML
{
    QStatus status = [self.bus createInterfacesFromXml:kInterfaceXML];    
    STAssertTrue(status == ER_OK, @"Bus failed to create interface from XML.");    
    
    AJNInterfaceDescription *iface = [self.bus interfaceWithName:kInterfaceName];
    STAssertNotNil(iface, @"Bus failed to retrieve interface that had already been created from XML.");
    
    NSArray *members = iface.members;
    STAssertTrue(members.count == 2, @"The number of members does not match the interface created from XML");

    AJNInterfaceMember *member = [members objectAtIndex:0];
    STAssertTrue([member.name compare:kInterfaceSignal] == NSOrderedSame, @"The interface member name does not match what was specified in the XML.");
    STAssertTrue([member.inputSignature compare:kInterfaceSignalInputSignature] == NSOrderedSame, @"The interface member input signature does not match what was specified in the XML.");
    STAssertTrue(member.outputSignature.length == 0, @"The interface member output signature does not match what was specified in the XML.");
    STAssertTrue([[member.argumentNames componentsJoinedByString:@","] compare:kInterfaceSignalArgNames] == NSOrderedSame, @"The interface member argument names do not match what was specified in the XML.");

    member = [members objectAtIndex:1];
    STAssertTrue([member.name compare:kInterfaceMethod] == NSOrderedSame, @"The interface member name does not match what was specified in the XML.");
    STAssertTrue([member.inputSignature compare:kInterfaceMethodInputSignature] == NSOrderedSame, @"The interface member input signature does not match what was specified in the XML.");
    STAssertTrue([member.outputSignature compare:kInterfaceMethodOutputSignature] == NSOrderedSame, @"The interface member output signature does not match what was specified in the XML.");
    STAssertTrue([[member.argumentNames componentsJoinedByString:@","] compare:kInterfaceMethodArgNames] == NSOrderedSame, @"The interface member argument names do not match what was specified in the XML.");
    
    NSArray *properties = iface.properties;
    STAssertTrue(properties.count == 1, @"The number of properties does not match the interface created from XML");    
    
    AJNInterfaceProperty *property = [properties objectAtIndex:0];
    STAssertTrue([property.name compare:kInterfaceProperty] == NSOrderedSame, @"The interface property name does not match what was specified in the XML.");
    STAssertTrue([property.signature compare:kInterfacePropertySignature] == NSOrderedSame, @"The interface property input signature does not match what was specified in the XML.");
    STAssertTrue(property.accessPermissions == kInterfacePropertyAccessPermissions, @"The interface property output signature does not match what was specified in the XML.");
    
}

@end
