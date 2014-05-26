/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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

#import "AboutServiceViewController.h"
#import "AJNVersion.h"
#import "AJNAboutIconService.h"
#import "AJNAboutServiceApi.h"
#import "AJNAboutService.h"
#import "CommonBusListener.h"


static NSString * ABOUT_ICON_OBJECT_PATH=@"/About/DeviceIcon"; //About Service - Icon
static NSString * ABOUT_ICON_INTERFACE_NAME=@"org.alljoyn.Icon";   //About Service - Icon
static bool ALLOWREMOTEMESSAGES = true; // About Client -  allow Remote Messages flag
static NSString *APPNAME = @"AboutClientMain"; //About Client - default application name
static AJNSessionPort SERVICE_PORT = 900; // About Service - service port

//  About Service - Icon
uint8_t aboutIconContent[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44,
    0x52, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x0A, 0x08, 0x02, 0x00, 0x00, 0x00, 0x02, 0x50, 0x58, 0xEA, 0x00,
    0x00, 0x00, 0x04, 0x67, 0x41, 0x4D, 0x41, 0x00, 0x00, 0xAF, 0xC8, 0x37, 0x05, 0x8A, 0xE9, 0x00, 0x00, 0x00, 0x19,
    0x74, 0x45, 0x58, 0x74, 0x53, 0x6F, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 0x00, 0x41, 0x64, 0x6F, 0x62, 0x65, 0x20,
    0x49, 0x6D, 0x61, 0x67, 0x65, 0x52, 0x65, 0x61, 0x64, 0x79, 0x71, 0xC9, 0x65, 0x3C, 0x00, 0x00, 0x00, 0x18, 0x49,
    0x44, 0x41, 0x54, 0x78, 0xDA, 0x62, 0xFC, 0x3F, 0x95, 0x9F, 0x01, 0x37, 0x60, 0x62, 0xC0, 0x0B, 0x46, 0xAA, 0x34,
    0x40, 0x80, 0x01, 0x00, 0x06, 0x7C, 0x01, 0xB7, 0xED, 0x4B, 0x53, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E,
    0x44, 0xAE, 0x42, 0x60, 0x82 };

@interface AboutServiceViewController ()
@property NSString *className;
// About Service
@property (strong, nonatomic) AJNBusAttachment *serviceBusAttachment;
@property (strong, nonatomic) AJNAboutPropertyStoreImpl *aboutPropertyStoreImpl;
@property (strong, nonatomic) AJNAboutServiceApi *aboutServiceApi;
@property (strong, nonatomic) CommonBusListener *aboutSessionPortListener;
@property (strong, nonatomic) AJNAboutIconService *aboutIconService;
@property (nonatomic) bool isAboutServiceConnected;
@property (strong, nonatomic) NSString *uniqueID;
@end

@implementation AboutServiceViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
	self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
	if (self) {
		// Custom initialization
	}
	return self;
}

- (void)viewDidLoad
{
	[super viewDidLoad];
	self.isAboutServiceConnected = false;
	[self aboutServiceButtonTitle:self.isAboutServiceConnected];
    
    // Create unique ID
	self.uniqueID = [[NSUUID UUID] UUIDString];
}



- (IBAction)didTouchStartAboutServiceButton:(id)sender
{
	if (self.isAboutServiceConnected == true) {
		[self stopAboutService];
	}
	else {
		[self startAboutService];
	}
	[self aboutServiceButtonTitle:self.isAboutServiceConnected];
}

- (void)aboutServiceButtonTitle:(bool)isConnected
{
	if (isConnected) {
		[self.startAboutServiceButton setTitle:@"Stop About Service" forState:UIControlStateNormal];
	}
	else {
		[self.startAboutServiceButton setTitle:@"Start About Service" forState:UIControlStateNormal];
	}
}

#pragma mark - About Service
- (void)startAboutService
{
    QStatus serviceStatus;
   
    NSLog(@"[%@] [%@] Start About Service", @"DEBUG", [[self class] description]);
    
    NSLog(@"[%@] [%@] AboutService using port %hu", @"DEBUG", [[self class] description], SERVICE_PORT);
  
	// Create message bus
    
    NSLog(@"[%@] [%@] Create a message bus", @"DEBUG", [[self class] description]);
    
	self.serviceBusAttachment = [[AJNBusAttachment alloc] initWithApplicationName:@"AboutServiceName" allowRemoteMessages:ALLOWREMOTEMESSAGES];
	if (!self.serviceBusAttachment) {
        
        NSLog(@"[%@] [%@] Failed to create a message bus - exiting application", @"DEBUG", [[self class] description]);
        
		serviceStatus = ER_OUT_OF_MEMORY;
		exit(serviceStatus);
	}
	// Start the bus
    NSLog(@"[%@] [%@] Start bus", @"DEBUG", [[self class] description]);

	serviceStatus = [self.serviceBusAttachment start];
	if (serviceStatus != ER_OK) {
        
        NSLog(@"[%@] [%@] Failed to start bus - exiting application", @"FATAL", [[self class] description]);

		exit(serviceStatus);
	}
    
     NSLog(@"[%@] [%@] Connect to an AllJoyn daemon", @"DEBUG", [[self class] description]);

	serviceStatus = [self.serviceBusAttachment connectWithArguments:@""];
	if (serviceStatus != ER_OK) {
        NSLog(@"[%@] [%@] Failed to start bus - exiting application", @"FATAL", [[self class] description]);

		exit(serviceStatus);
	}
    
     NSLog(@"[%@] [%@] Create AboutSessionPortListener", @"DEBUG", [[self class] description]);

	self.aboutSessionPortListener = [[CommonBusListener alloc] initWithServicePort:SERVICE_PORT];
    
	if (self.aboutSessionPortListener) {
         NSLog(@"[%@] [%@] Register AboutSessionPortListener", @"DEBUG", [[self class] description]);

		[self.serviceBusAttachment registerBusListener:self.aboutSessionPortListener];
	}
    
	// prepare data for propertyStoreImpl
	self.aboutPropertyStoreImpl = [[AJNAboutPropertyStoreImpl alloc] init];
    
	if (!self.aboutPropertyStoreImpl) {
        NSLog(@"[%@] [%@] Failed to create propertyStoreImpl object - exiting application", @"FATAL", [[self class] description]);

		exit(1);
	}
    
	serviceStatus = [self fillAboutPropertyStoreImplData];
    
	if (serviceStatus != ER_OK) {
        NSLog(@"[%@] [%@] Failed to fill propertyStore - exiting application", @"FATAL", [[self class] description]);

		exit(1);
	}
    
    
     NSLog(@"[%@] [%@] Create aboutServiceApi", @"DEBUG", [[self class] description]);

	self.aboutServiceApi = [AJNAboutServiceApi sharedInstance];
	if (!self.aboutServiceApi) {
        NSLog(@"[%@] [%@] Failed to create aboutServiceApi - exiting application", @"FATAL", [[self class] description]);

		exit(1);
	}
    
     NSLog(@"[%@] [%@] Start aboutServiceApi", @"DEBUG", [[self class] description]);

	[self.aboutServiceApi startWithBus:self.serviceBusAttachment andPropertyStore:self.aboutPropertyStoreImpl];
    
	// Register Port
     NSLog(@"[%@] [%@] Register the AboutService on the AllJoyn bus", @"DEBUG", [[self class] description]);

	if (self.aboutServiceApi.isServiceStarted) {
		serviceStatus = [self.aboutServiceApi registerPort:(SERVICE_PORT)];
	}
    
	if (serviceStatus != ER_OK) {
        NSLog(@"[%@] [%@] Failed register port - exiting application", @"FATAL", [[self class] description]);

		exit(1);
	}
    
	// Bind session port
	AJNSessionOptions *opt = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:false proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
     NSLog(@"[%@] [%@] Bind session", @"DEBUG", [[self class] description]);

  
	serviceStatus = [self.serviceBusAttachment bindSessionOnPort:SERVICE_PORT withOptions:opt withDelegate:self.aboutSessionPortListener];
    
	if (serviceStatus == ER_ALLJOYN_BINDSESSIONPORT_REPLY_ALREADY_EXISTS) {
        NSLog(@"[%@] [%@] SessionPort already exists", @"DEBUG", [[self class] description]);
	}
    
	// Start AboutIcon
	serviceStatus = [self startAboutIcon];
	if (serviceStatus != ER_OK) {
        NSLog(@"[%@] [%@] Failed to startAboutIcon. %@ ", @"DEBUG", [[self class] description], [AJNStatus descriptionForStatusCode:serviceStatus]);
	}
	else {
		// Advertise the service name
		serviceStatus = ER_BUS_ESTABLISH_FAILED;
		if (self.serviceBusAttachment.isConnected && ([[self.serviceBusAttachment uniqueName] length]) > 0) {
             NSLog(@"[%@] [%@] Will advertise %@", @"DEBUG", [[self class] description],[self.serviceBusAttachment uniqueName]);

            serviceStatus = [self.serviceBusAttachment advertiseName:([self.serviceBusAttachment uniqueName]) withTransportMask:(kAJNTransportMaskAny)];
		}
	}
    
	if (serviceStatus == ER_OK) {
		if (self.aboutServiceApi.isServiceStarted) {
             NSLog(@"[%@] [%@] Calling Announce", @"DEBUG", [[self class] description]);
			serviceStatus = [self.aboutServiceApi announce];
		}
		if (serviceStatus == ER_OK) {
             NSLog(@"[%@] [%@] Successfully announced", @"DEBUG", [[self class] description]);
		}
	}
    
	self.isAboutServiceConnected = true;
}

- (QStatus)startAboutIcon
{
    NSLog(@"[%@] [%@] Start About Icon", @"DEBUG", [[self class] description]);

	// Add Icon ObjectDescription
	NSMutableArray *interfaces = [[NSMutableArray alloc] init];
	[interfaces addObject:ABOUT_ICON_INTERFACE_NAME];
	NSString *path = ABOUT_ICON_OBJECT_PATH;
	QStatus aboutIconStatus = [self.aboutServiceApi addObjectDescriptionWithPath:path andInterfaceNames:interfaces];
    
	if (aboutIconStatus != ER_OK) {
        NSLog(@"[%@] [%@] Failed to addObjectDescription.   %@", @"DEBUG", [[self class] description], [AJNStatus descriptionForStatusCode:aboutIconStatus]);
		return aboutIconStatus;
	}
	// Create aboutIconService
	NSString *mimeType = @"image/png";
	NSString *url = @"http://tinyurl.com/llrqvrb";
	size_t csize = sizeof(aboutIconContent) / sizeof(*aboutIconContent);
	self.aboutIconService = [[AJNAboutIconService alloc] initWithBus:self.serviceBusAttachment mimeType:mimeType url:url content:aboutIconContent csize:csize];
    
	aboutIconStatus = [self.aboutIconService registerAboutIconService];
	if (aboutIconStatus != ER_OK) {
        NSLog(@"[%@] [%@] Failed to registerAboutIconService.   %@", @"ERROR", [[self class] description], [AJNStatus descriptionForStatusCode:aboutIconStatus]);
        
		return aboutIconStatus;
	}
	aboutIconStatus = [self.serviceBusAttachment registerBusObject:self.aboutIconService];
    
	if (aboutIconStatus != ER_OK) {
        NSLog(@"[%@] [%@] Failed to registerBusObject.   %@", @"ERROR", [[self class] description], [AJNStatus descriptionForStatusCode:aboutIconStatus]);
	}
	return aboutIconStatus;
}

- (QStatus)fillAboutPropertyStoreImplData
{
	QStatus status;
    
	// AppId
	status = [self.aboutPropertyStoreImpl setAppId:self.uniqueID];
	if (status != ER_OK) {
		return status;
	}
    
	// AppName
	status = [self.aboutPropertyStoreImpl setAppName:@"AboutConfig"];
	if (status != ER_OK) {
		return status;
	}
    
	// DeviceId
	status = [self.aboutPropertyStoreImpl setDeviceId:@"1231232145667745675477"];
	if (status != ER_OK) {
		return status;
	}
    
	// DeviceName
	status = [self.aboutPropertyStoreImpl setDeviceName:@"Screen" language:@"en"];
	if (status != ER_OK) {
		return status;
	}
    
    status = [self.aboutPropertyStoreImpl setDeviceName:@"écran" language:@"fr"];
    if (status != ER_OK) {
		return status;
	}
    
    status = [self.aboutPropertyStoreImpl setDeviceName:@"pantalla" language:@"sp"];
    if (status != ER_OK) {
		return status;
	}
    
	// SupportedLangs
	NSArray *languages = @[@"en", @"sp", @"fr"];
	status = [self.aboutPropertyStoreImpl setSupportedLangs:languages];
	if (status != ER_OK) {
		return status;
	}
    
	//  DefaultLang
	status = [self.aboutPropertyStoreImpl setDefaultLang:@"en"];
	if (status != ER_OK) {
		return status;
	}
    
	//  ModelNumbe
	status = [self.aboutPropertyStoreImpl setModelNumber:@"Wxfy388i"];
	if (status != ER_OK) {
		return status;
	}
    
	//  DateOfManufacture
	status = [self.aboutPropertyStoreImpl setDateOfManufacture:@"10/1/2199"];
	if (status != ER_OK) {
		return status;
	}
	//  SoftwareVersion
	status = [self.aboutPropertyStoreImpl setSoftwareVersion:@"12.20.44 build 44454"];
	if (status != ER_OK) {
		return status;
	}
    
	//  AjSoftwareVersion
	status = [self.aboutPropertyStoreImpl setAjSoftwareVersion:[AJNVersion versionInformation]];
	if (status != ER_OK) {
		return status;
	}
    
	//  HardwareVersion
	status = [self.aboutPropertyStoreImpl setHardwareVersion:@"355.499. b"];
	if (status != ER_OK) {
		return status;
	}
	//  Description
	status = [self.aboutPropertyStoreImpl setDescription:@"This is an Alljoyn Application" language:@"en"];
	if (status != ER_OK) {
		return status;
	}
	status = [self.aboutPropertyStoreImpl setDescription:@"Esta es una Alljoyn aplicación" language:@"sp"];
	if (status != ER_OK) {
		return status;
	}
	status = [self.aboutPropertyStoreImpl setDescription:@"C'est une Alljoyn application"  language:@"fr"];
	if (status != ER_OK) {
		return status;
	}
	//  Manufacturer
	status = [self.aboutPropertyStoreImpl setManufacturer:@"Company" language:@"en"];
	if (status != ER_OK) {
		return status;
	}
	status = [self.aboutPropertyStoreImpl setManufacturer:@"Empresa" language:@"sp"];
	if (status != ER_OK) {
		return status;
	}
	status = [self.aboutPropertyStoreImpl setManufacturer:@"Entreprise" language:@"fr"];
	if (status != ER_OK) {
		return status;
	}
    
	//  SupportedUrl
	status = [self.aboutPropertyStoreImpl setSupportUrl:@"http://www.alljoyn.org"];
	if (status != ER_OK) {
		return status;
	}
	return status;
}

- (void)stopAboutService
{
	QStatus status;
    
     NSLog(@"[%@] [%@] Stop About Service", @"DEBUG", [[self class] description]);

    
	//  Stop AbouIcon
    
	[self.serviceBusAttachment unregisterBusObject:self.aboutIconService];
	self.aboutIconService = nil;
    
	//  Delete AboutServiceApi
	[self.aboutServiceApi destroyInstance];
	self.aboutServiceApi = nil;
    
	//  Delete AboutPropertyStoreImpl
	self.aboutPropertyStoreImpl = nil;
    
	//  Bus attachment cleanup
	status = [self.serviceBusAttachment cancelAdvertisedName:[self.serviceBusAttachment uniqueName] withTransportMask:kAJNTransportMaskAny];
	if (status == ER_OK) {
         NSLog(@"[%@] [%@] Successfully cancel advertised name", @"DEBUG", [[self class] description]);
	}
    
	status = [self.serviceBusAttachment unbindSessionFromPort:SERVICE_PORT];
	if (status == ER_OK) {
         NSLog(@"[%@] [%@] Successfully unbind Session", @"DEBUG", [[self class] description]);
	}
    
	//  Delete AboutSessionPortListener
	[self.serviceBusAttachment unregisterBusListener:self.aboutSessionPortListener];
	self.aboutSessionPortListener = nil;
    
	//  Stop bus attachment
	status = [self.serviceBusAttachment stop];
	if (status == ER_OK) {
         NSLog(@"[%@] [%@] Successfully stopped bus", @"DEBUG", [[self class] description]);
	}
	self.serviceBusAttachment = nil;
    
	//  set flag
	self.isAboutServiceConnected = false;
     NSLog(@"[%@] [%@] About Service is stopped", @"DEBUG", [[self class] description]);
}

@end
