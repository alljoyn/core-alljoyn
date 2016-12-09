/******************************************************************************
 * @file AJNAboutData.m
 * This contains the AboutData class responsible for holding the org.alljoyn.About
 * interface data fields.
 *
 * Copyright (c) 2016 Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright 2016 Open Connectivity Foundation and Contributors to
 *    AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#import "AJNAboutData.h"
#import <alljoyn/AboutData.h>

using namespace ajn;

#pragma mark-

@interface AJNAboutData()

@property (nonatomic, readonly) AboutData *aboutData;

@end


#pragma mark-
@interface AJNMessageArgument(Private)

@property (nonatomic, readonly) MsgArg *msgArg;

@end

@interface AJNObject(Private)

@property (nonatomic) BOOL shouldDeleteHandleOnDealloc;

@end

#pragma mark-

typedef NS_ENUM(NSInteger, AJNAboutFieldMask) {
    /**
     * The AboutData field is not required, announced, or localized.
     */
    EMPTY_MASK  = 0,
    /**
     * The AboutData field is required.
     */
    REQUIRED    = 1,
    /**
     * The AboutData field is announced.
     */
    ANNOUNCED   = 2,
    /**
     * The AboutData field is localized.
     */
    LOCALIZED   = 4
};

@interface FieldDetails : NSObject
@property (nonatomic, assign) AJNAboutFieldMask fieldMask;
@property (nonatomic, strong) NSString *signature;

- (instancetype)initWithFieldMask:(AJNAboutFieldMask)aboutFieldMask andSignature:(NSString*)signature;
@end

@implementation FieldDetails
- (instancetype)initWithFieldMask:(AJNAboutFieldMask)aboutFieldMask andSignature:(NSString *)signature{
    self = [super init];
    if (self) {
        self.fieldMask = aboutFieldMask;
        self.signature = signature;
    }
    return self;
}
@end





#pragma mark-
@implementation AJNAboutData
/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
- (AboutData*)aboutData
{
    return static_cast<AboutData*>(self.handle);
}

- (id)init
{
    self = [super init];
    if (self) {
        self.handle = new AboutData();
        self.shouldDeleteHandleOnDealloc = YES;
    }
    return self;
}

- (id)initWithLanguage: (NSString*)language
{
    self = [super init];
    if (self) {
        self.handle = new AboutData([ language UTF8String]);
        self.shouldDeleteHandleOnDealloc = YES;
    }
    return self;
}

- (id)initWithMsgArg:(AJNMessageArgument *)msgArg andLanguage:(NSString*)language
{
    self = [super init];
    if (self) {
        self.handle = new AboutData(*msgArg.msgArg,[ language UTF8String]);
        self.shouldDeleteHandleOnDealloc = YES;
    }
    return self;
}

- (void)dealloc
{
    if (self.shouldDeleteHandleOnDealloc) {
        AboutData *ptr = (AboutData*)self.handle;
        delete ptr;
        self.handle = nil;
    }
}

- (BOOL)isHexChar:(char)c
{
    return ((c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'F') ||
            (c >= 'a' && c <= 'f'));
}

- (QStatus)createFromXml:(NSString*)aboutXmlData
{
    return self.aboutData->CreateFromXml(aboutXmlData.UTF8String);
}

- (QStatus)createFromMsgArg:(AJNMessageArgument *)msgArg andLanguage:(NSString*)language
{
    return self.aboutData->CreatefromMsgArg(*msgArg.msgArg,language.UTF8String);
}

- (QStatus)getField:(NSString*)name messageArg:(AJNMessageArgument*)msgArg language:(NSString*)language{
    QStatus status;
    MsgArg* arg = msgArg.msgArg;
    status = self.aboutData->GetField(name.UTF8String, arg, language.UTF8String);
    return status;
}

- (QStatus)setField:(NSString*)name msgArg:(AJNMessageArgument*)msgArg andLanguage:(NSString*)language{
     return self.aboutData->SetField(name.UTF8String, *msgArg.msgArg, language.UTF8String );
}

- (BOOL)isValid{

    return [self isValid:nil];
}

- (BOOL)isValid:(NSString*)language{
    BOOL test;
    if (language == nil || language.length == 0) {
        test = self.aboutData->IsValid();
        return test;
    }
    return self.aboutData->IsValid( [language UTF8String]);
}

- (QStatus)setField:(NSString*)name msgArg:(AJNMessageArgument*)msgArg{
    return self.aboutData->SetField(name.UTF8String, *msgArg.msgArg);
}

- (QStatus)setDefaultLanguage:(NSString *)language{
    return self.aboutData->SetDefaultLanguage(language.UTF8String);
}

- (NSString*)getFieldSignature:(NSString*)fieldName{
    const char *fieldSignature = self.aboutData->GetFieldSignature([fieldName UTF8String]);
    return [NSString stringWithCString:fieldSignature encoding:NSUTF8StringEncoding ];

}

- (BOOL)isFieldLocalized:(NSString*)fieldName{
    return self.aboutData->IsFieldLocalized([fieldName UTF8String]);

}

- (BOOL)isFieldAnnounced:(NSString*)fieldName{
    return self.aboutData->IsFieldAnnounced([fieldName UTF8String]);

}

- (BOOL)isFieldRequired:(NSString*)fieldName{
    return self.aboutData->IsFieldRequired([fieldName UTF8String]);
}

- (QStatus)setAppId:(uint8_t[])appId{
    return self.aboutData->SetAppId(appId);

}

- (QStatus)getDefaultLanguage:(NSString**)defaultLanguage
{
    QStatus status;
    char *defaultLanguageOut;
    status = self.aboutData->GetDeviceId(&defaultLanguageOut);

    if (status == ER_OK && defaultLanguageOut != NULL) {
        *defaultLanguage = [NSString stringWithCString:defaultLanguageOut encoding:NSUTF8StringEncoding ];
    } else {
        *defaultLanguage = nil;
    }

    return status;
}

- (QStatus)setDeviceName:(NSString*)deviceName andLanguage:(NSString*)language
{
    return self.aboutData->SetDeviceName([deviceName UTF8String], language.UTF8String);
}

- (QStatus)getDeviceName:(NSString**)deviceName andLanguage:(NSString*)language
{
    char* deviceNameOut;
    const char *languageIn = [language UTF8String];

    QStatus status;

    status = self.aboutData->GetDeviceName(&deviceNameOut, languageIn);
    if (status == ER_OK && deviceNameOut != NULL) {
        *deviceName = [NSString stringWithCString:deviceNameOut encoding:NSUTF8StringEncoding ];
    } else {
        *deviceName = nil;
    }

    return status;
}

- (QStatus)setDeviceId:(NSString*)deviceId
{
    return self.aboutData->SetDeviceId([deviceId UTF8String]);

}

- (QStatus)getDeviceId:(NSString**)deviceId
{
    QStatus status;
    char* deviceIdOut;
    status = self.aboutData->GetDeviceId(&deviceIdOut);

    if (status == ER_OK && deviceIdOut != NULL) {
        *deviceId = [NSString stringWithCString:deviceIdOut encoding:NSUTF8StringEncoding];
    } else {
        *deviceId = nil;
    }

    return status;
}

- (QStatus)setAppName:(NSString*)appName andLanguage:(NSString*)language
{
    return self.aboutData->SetAppName([appName UTF8String], [language UTF8String]);

}

- (QStatus)getAppName:(NSString**)appName andLanguage:(NSString*)language
{
    char* appNameOut;
    const char *languageIn = [language UTF8String];

    QStatus status;

    status = self.aboutData->GetAppName(&appNameOut, languageIn);

    if (status == ER_OK && appNameOut != NULL) {
        *appName = [NSString stringWithCString:appNameOut encoding:NSUTF8StringEncoding];
    } else {
        *appName = nil;
    }
    return status;
}

- (QStatus)setManufacturer:(NSString*)manufacturer andLanguage:(NSString*)language
{
    return self.aboutData->SetManufacturer([manufacturer UTF8String], [language UTF8String]);

}

- (QStatus)getManufacturer:(NSString**)manufacturer andLanguage:(NSString*)language
{
    char* manufacturerOut;
    QStatus status;

    status = self.aboutData->GetManufacturer(&manufacturerOut, [language UTF8String]);

    if (status == ER_OK && manufacturerOut != NULL) {
        *manufacturer = [NSString stringWithCString:manufacturerOut encoding:NSUTF8StringEncoding];
    } else {
        *manufacturer = nil;
    }
    return status;
}

- (QStatus)setModelNumber:(NSString*)modelNumber
{
    return self.aboutData->SetModelNumber([modelNumber UTF8String]);
}

- (QStatus)getModelNumber:(NSString**)modelNumber
{
    QStatus status;
    char* modelNumberOut;
    status = self.aboutData->GetModelNumber(&modelNumberOut);

    if (status == ER_OK && modelNumberOut != NULL) {
        *modelNumber = [NSString stringWithCString:modelNumberOut encoding:NSUTF8StringEncoding];
    } else {
        *modelNumber = nil;
    }

    return status;
}

- (QStatus)setSupportedLanguage:(NSString*)language
{
    return self.aboutData->SetSupportedLanguage([language UTF8String]);
}

- (size_t)getSupportedLanguages:(NSString*)languageTags num:(size_t)num
{
    const char* languageTagsOut;
    size_t numOut = 0;

    num = self.aboutData->GetSupportedLanguages(&languageTagsOut, numOut);
    return num;
}

- (QStatus)setDescription:(NSString*)description andLanguage:(NSString*)language
{
    return self.aboutData->SetDescription([description UTF8String], [language UTF8String]);
}

- (QStatus)getDescription:(NSString**)description language:(NSString*)language
{
    QStatus status;
    char* descriptionOut;
    const char *languageIn = [language UTF8String];

    status = self.aboutData->GetDescription(&descriptionOut, languageIn);

    if (status == ER_OK && descriptionOut != NULL) {
        *description = [NSString stringWithCString:descriptionOut encoding:NSUTF8StringEncoding];
    } else {
        *description = nil;
    }

    return status;
}


- (QStatus)setDateOfManufacture:(NSString*)dateOfManufacture
{
    return self.aboutData->SetDateOfManufacture([dateOfManufacture UTF8String]);

}

- (QStatus)getDateOfManufacture:(NSString**)dateOfManufacture
{
    char* dateOfManufactureOut;
    QStatus status;

    status = self.aboutData->GetDateOfManufacture(&dateOfManufactureOut);

    if (status == ER_OK && dateOfManufactureOut != NULL) {
        *dateOfManufacture = [NSString stringWithCString:dateOfManufactureOut encoding:NSUTF8StringEncoding];
    } else {
        *dateOfManufacture = nil;
    }

    return status;
}

- (QStatus)setSoftwareVersion:(NSString*)softwareVersion
{
    return self.aboutData->SetSoftwareVersion([softwareVersion UTF8String]);

}

- (QStatus)getSoftwareVersion:(NSString**)softwareVersion
{
    char* softwareVersionOut;
    QStatus status;

    status = self.aboutData->GetSoftwareVersion(&softwareVersionOut);

    if (status == ER_OK && softwareVersionOut != NULL) {
        *softwareVersion = [NSString stringWithCString:softwareVersionOut encoding:NSUTF8StringEncoding];
    } else {
        *softwareVersion = nil;
    }

    return status;
}

- (QStatus)getAJSoftwareVersion:(NSString**)ajSoftwareVersion
{
    char* ajSoftwareVersionOut;
    QStatus status;

    status = self.aboutData->GetAJSoftwareVersion(&ajSoftwareVersionOut);

    if (status == ER_OK && ajSoftwareVersionOut != NULL) {
        *ajSoftwareVersion = [NSString stringWithCString:ajSoftwareVersionOut encoding:NSUTF8StringEncoding];
    } else {
        *ajSoftwareVersion = nil;
    }

    return status;
}

- (QStatus)setHardwareVersion:(NSString*)hardwareVersion
{
    return self.aboutData->SetHardwareVersion([hardwareVersion UTF8String]);
}

- (QStatus)getHardwareVersion:(NSString**)hardwareVersion
{
    char* hardwareVersionOut;
    QStatus status;

    status = self.aboutData->GetHardwareVersion(&hardwareVersionOut);

    if (status == ER_OK && hardwareVersionOut != NULL) {
        *hardwareVersion = [NSString stringWithCString:hardwareVersionOut encoding:NSUTF8StringEncoding];
    } else {
        *hardwareVersion = nil;
    }

    return status;
}

- (QStatus)setSupportUrl:(NSString*)supportUrl
{
    return self.aboutData->SetSupportUrl([supportUrl UTF8String]);
}

- (QStatus)getSupportUrl:(NSString**)supportUrl
{
    char* supportUrlOut;
    QStatus status;

    status = self.aboutData->GetSupportUrl(&supportUrlOut);
    if (status == ER_OK && supportUrlOut != NULL) {
        *supportUrl = [NSString stringWithCString:supportUrlOut encoding:NSUTF8StringEncoding];
    } else {
        *supportUrl = nil;
    }

    return status;
}

#pragma mark- AJNAboutDataListener
- (QStatus)getAboutData:(AJNMessageArgument**)msgArg withLanguage:(NSString*)language
{
    QStatus status = ER_OK;
    MsgArg *messageArg = new MsgArg;

    status = self.aboutData->GetAboutData(messageArg, [language UTF8String]);

    *msgArg = [[AJNMessageArgument alloc]initWithHandle:messageArg ];

    return status;

}

- (QStatus)getAnnouncedAboutData:(AJNMessageArgument**)msgArg
{
    QStatus status = ER_OK;
    MsgArg *messageArg = new MsgArg;

    status = self.aboutData->GetAnnouncedAboutData(messageArg);

    *msgArg = [[AJNMessageArgument alloc]initWithHandle:messageArg ];

    return status;
}

@end