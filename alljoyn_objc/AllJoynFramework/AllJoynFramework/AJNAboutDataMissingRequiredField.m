//
//  AJNAboutDataMissingRequiredField.m
//  AllJoynFramework
//
//  Created by Jayachandra Agraharam on 12/07/2016.
//  Copyright © 2016 AllSeen Alliance. All rights reserved.
//

#import "AJNAboutDataMissingRequiredField.h"

@implementation AJNAboutDataMissingRequiredField
- (void)setAboutData:(NSString*)language{
    //nonlocalized values
    AJNMessageArgument *args = [[AJNMessageArgument alloc] init];
    uint8_t originalAppId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    
    [args setValue:@"ay",sizeof(originalAppId) / sizeof(originalAppId[0]),originalAppId];
    [args stabilize];
    [_aboutData setValue:args forKey:@"AppId"];
    
    [_aboutData setValue:[[AJNMessageArgument alloc] initWithHandle:@"en"]
                 forKey:@"DefaultLanguage"];
    [_aboutData setValue:[[AJNMessageArgument alloc] initWithHandle:@"A1B2C3"]
                 forKey:@"ModelNumber"];
    [_aboutData setValue:[[AJNMessageArgument alloc] initWithHandle:(__bridge AJNHandle)(@[@"en", @"es"])]
                 forKey:@"SupportedLanguages"];
    [_aboutData setValue:[[AJNMessageArgument alloc] initWithHandle:@"2016-07-12"]
                 forKey:@"DateOfManufacture"];
    [_aboutData setValue:[[AJNMessageArgument alloc] initWithHandle:@"1.0"]
                 forKey:@"SoftwareVersion"];
    [_aboutData setValue:[[AJNMessageArgument alloc] initWithHandle:@"0.0.1"]
                 forKey:@"AJSoftwareVersion"];
    [_aboutData setValue:[[AJNMessageArgument alloc] initWithHandle:@"0.1alpha"]
                 forKey:@"HardwareVersion"];
    
    //localized values
    // If the language String is null or an empty string we return the
    // default language
    if (language == nil || language == NULL || language.length == 0 || [language caseInsensitiveCompare:@"en"] == NSOrderedSame ){
        [_aboutData setValue:[[AJNMessageArgument alloc] initWithHandle:@"A device name"]
                     forKey:@"DeviceName"];
        [_aboutData setValue:[[AJNMessageArgument alloc] initWithHandle:@"An application name"]
                     forKey:@"AppName"];
        [_aboutData setValue:[[AJNMessageArgument alloc] initWithHandle:@"A mighty manufacturing company"]
                     forKey:@"Manufacturer"];
        [_aboutData setValue:[[AJNMessageArgument alloc] initWithHandle:@"Sample showing the about feature in a service application"]
                     forKey:@"Description"];
    }else if ([language caseInsensitiveCompare:@"es"] == NSOrderedSame){//Spanish
        [_aboutData setValue:[[AJNMessageArgument alloc] initWithHandle:@"Un nombre de dispositivo"]
                     forKey:@"DeviceName"];
        [_aboutData setValue:[[AJNMessageArgument alloc] initWithHandle:@"Un nombre de aplicación"]
                     forKey:@"AppName"];
        [_aboutData setValue:[[AJNMessageArgument alloc] initWithHandle:@"Una empresa de fabricación de poderosos"]
                     forKey:@"Manufacturer"];
        [_aboutData setValue:[[AJNMessageArgument alloc] initWithHandle:@"Muestra que muestra la característica de sobre en una aplicación de servicio"]
                     forKey:@"Description"];
    }
}

- (void)setAnnouncedData{
    AJNMessageArgument *args = [[AJNMessageArgument alloc] init];
    
    uint8_t originalAppId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    
    [args setValue:@"ay",sizeof(originalAppId) / sizeof(originalAppId[0]),originalAppId];
    [args stabilize];
    
    [_aboutData setValue:args
                 forKey:@"AppId"];
    [_aboutData setValue:[[AJNMessageArgument alloc] initWithHandle:@"en"]
                 forKey:@"DefaultLanguage"];
    [_aboutData setValue:[[AJNMessageArgument alloc] initWithHandle:@"A device name"]
                 forKey:@"DeviceName"];
    [_aboutData setValue:[[AJNMessageArgument alloc] initWithHandle:@"sampleDeviceId"]
                 forKey:@"DeviceId"];
    [_aboutData setValue:[[AJNMessageArgument alloc] initWithHandle:@"An application name"]
                 forKey:@"AppName"];
    [_aboutData setValue:[[AJNMessageArgument alloc] initWithHandle:@"A mighty manufacturing company"]
                 forKey:@"Manufacturer"];
    [_aboutData setValue:[[AJNMessageArgument alloc] initWithHandle:@"A1B2C3"]
                 forKey:@"ModelNumber"];
}

- (void)aboutDataMissingRequiredField:(NSString*)language{
    [self setAboutData:language];
    [self setAnnouncedData];
}

- (QStatus)getAboutDataForLanguage:(NSString *)language usingDictionary:(NSMutableDictionary **)aboutData{
    //aboutData = self.aboutData;
    return ER_OK;
}
- (QStatus)getDefaultAnnounceData:(NSMutableDictionary **)announceData{
    //announceData = self.announceData;
    return ER_OK;
}

@end
