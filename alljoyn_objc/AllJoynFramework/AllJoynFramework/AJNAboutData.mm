//
//  AJNAboutData.m
//  AllJoynFramework_iOS
//
//  Created by Mindscape on 7/4/16.
//  Copyright © 2016 AllSeen Alliance. All rights reserved.
//

#import "AJNAboutData.h"
#import <alljoyn/AboutData.h>

using namespace ajn;

#pragma mark-
@interface AJNAboutData()
@property (nonatomic, readonly) AboutData *aboutData;

/*
 *private Set<String> supportedLanguages;
 *private Map<String, AJNMessageArgument> propertyStore;
 *private Map<String, Map<String, AJNMessageArgument>> localizedPropertyStore;
 *private Map<String, FieldDetails> aboutFields;
*/
@property (nonatomic, strong) NSMutableDictionary *propertyStore;
@property (nonatomic, strong) NSMutableDictionary *localizedPropertyStore;
@property (nonatomic, strong) NSMutableDictionary *aboutFields;
@property (nonatomic, strong) NSSet *supportedLanguages;



- (void)initializeFieldDetails;

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
        self.propertyStore = [[NSMutableDictionary alloc] init];
        self.localizedPropertyStore = [[NSMutableDictionary alloc] init];
        self.aboutFields = [[NSMutableDictionary alloc] init];
        [self initializeFieldDetails];
    }
    return self;
}

- (id)initWithMsgArg:(AJNMessageArgument *)msgArg andLanguage:(NSString*)language
{
    self = [super init];
    if (self) {
        self.handle = new AboutData(*msgArg.msgArg,NULL);
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
    QStatus status = ER_OK;
    if (![self isFieldLocalized:name]) {
        msgArg = self.propertyStore[name];
    }else{
        if (language == nil || language.length == 0) {
            msgArg = self.propertyStore[kDEFAULT_LANGUAGE];
            status = [msgArg value:self.aboutFields[kDEFAULT_LANGUAGE]];
            if (status != ER_OK) {
                return status;
            }
        }
    }
    return status;
}

- (QStatus)setField:(NSString*)name msgArg:(AJNMessageArgument*)msgArg andLanguage:(NSString*)language{
    QStatus status = ER_OK;
    // The user is adding an OEM specific field.
    // At this time OEM specific fields are added as
    //    not required
    //    not announced
    //    can be localized
    if(![self.aboutFields.allKeys containsObject:name]){
        if(msgArg.signature == nil){
            [NSException raise:NSGenericException format:@"Unable to find Variant signature."];
        }else if([msgArg.signature isEqualToString:@"s"]){
            [self.aboutFields setObject:[[FieldDetails alloc] initWithFieldMask:LOCALIZED
                                                                   andSignature:msgArg.signature]
                                 forKey:name];
        }else{
            [self.aboutFields setObject:[[FieldDetails alloc] initWithFieldMask:EMPTY_MASK
                                                                   andSignature:msgArg.signature]
                                 forKey:name];
        }
    }
    
    if([self isFieldLocalized:name]){
        if(language == nil || language == 0){
            if([self.propertyStore.allKeys containsObject:kDEFAULT_LANGUAGE]){
                AJNMessageArgument *arg = self.propertyStore[kDEFAULT_LANGUAGE];
                language = arg.signature;
            }else{
                [NSException raise:NSGenericException format:@"Specified language tag not found."];
            }
        }
    }else{
        [self.propertyStore setObject:msgArg forKey:name];
    }
    
    //TODO:: KISHORE :: incomplete implementation11
    return status;
}

- (BOOL)isValid{
    return [self isValid:nil];
}

- (BOOL)isValid:(NSString*)language{
    if (language == nil || language.length == 0) {
        @try {
            AJNMessageArgument *arg = self.propertyStore[kDEFAULT_LANGUAGE];
            language = arg.signature;
        } @catch (NSException *exception) {
            return NO;
        }
    }else{
        if(![self.supportedLanguages containsObject:language]){
            return NO;
        }
        
        for(NSString *lang in self.aboutFields.allKeys){
            if ([self isFieldRequired:lang]) {
                if ([self isFieldLocalized:lang]) {
                    NSDictionary *propertyObj = self.localizedPropertyStore[lang];
                    if (![self.localizedPropertyStore.allKeys containsObject:lang] || ![propertyObj.allKeys containsObject:language]) {
                        return NO;
                    }else{
                        if (![self.propertyStore.allKeys containsObject:lang]) {
                            return NO;
                        }
                    }
                }
            }
        }
    }
    return YES;
}

- (void)initializeFieldDetails{
    [self.aboutFields setObject:[[FieldDetails alloc] initWithFieldMask:REQUIRED|ANNOUNCED andSignature:@"ay"]
                         forKey:kAPP_ID];
    [self.aboutFields setObject:[[FieldDetails alloc] initWithFieldMask:REQUIRED|ANNOUNCED andSignature:@"s"]
                         forKey:kDEFAULT_LANGUAGE];
    [self.aboutFields setObject:[[FieldDetails alloc] initWithFieldMask:REQUIRED|ANNOUNCED andSignature:@"s"]
                         forKey:kDEVICE_NAME];
    [self.aboutFields setObject:[[FieldDetails alloc] initWithFieldMask:REQUIRED|ANNOUNCED andSignature:@"s"]
                         forKey:kDEVICE_ID];
    [self.aboutFields setObject:[[FieldDetails alloc] initWithFieldMask:REQUIRED|ANNOUNCED andSignature:@"s"]
                         forKey:kAPP_NAME];
    [self.aboutFields setObject:[[FieldDetails alloc] initWithFieldMask:REQUIRED|ANNOUNCED andSignature:@"s"]
                         forKey:kMANUFACTURER];
    [self.aboutFields setObject:[[FieldDetails alloc] initWithFieldMask:REQUIRED|ANNOUNCED andSignature:@"s"]
                         forKey:kMODEL_NUMBER];
    [self.aboutFields setObject:[[FieldDetails alloc] initWithFieldMask:REQUIRED|ANNOUNCED andSignature:@"as"]
                         forKey:kSUPPORTED_LANGUAGES];
    [self.aboutFields setObject:[[FieldDetails alloc] initWithFieldMask:REQUIRED|ANNOUNCED andSignature:@"s"]
                         forKey:kDESCRIPTION];
    [self.aboutFields setObject:[[FieldDetails alloc] initWithFieldMask:REQUIRED|ANNOUNCED andSignature:@"s"]
                         forKey:kDATE_OF_MANUFACTURE];
    [self.aboutFields setObject:[[FieldDetails alloc] initWithFieldMask:REQUIRED|ANNOUNCED andSignature:@"s"]
                         forKey:kSOFTWARE_VERSION];
    [self.aboutFields setObject:[[FieldDetails alloc] initWithFieldMask:REQUIRED|ANNOUNCED andSignature:@"s"]
                         forKey:kAJ_SOFTWARE_VERSION];
    [self.aboutFields setObject:[[FieldDetails alloc] initWithFieldMask:REQUIRED|ANNOUNCED andSignature:@"s"]
                         forKey:kHARDWARE_VERSION];
    [self.aboutFields setObject:[[FieldDetails alloc] initWithFieldMask:REQUIRED|ANNOUNCED andSignature:@"s"]
                         forKey:kSUPPORT_URL];
}

- (void)setNewFieldDetails:(NSString*)fieldName mask:(AJNAboutFieldMask)aboutFieldMask andSignature:(NSString*)signature{
    
    NSAssert1(fieldName == nil || [fieldName isEqualToString:@""], @"field name should not be nil oe empty", fieldName);
    
    [self.aboutFields setObject:[[FieldDetails alloc] initWithFieldMask:aboutFieldMask andSignature:signature]
                         forKey:fieldName];
}

- (QStatus)setField:(NSString*)name msgArg:(AJNMessageArgument*)msgArg{
    return self.aboutData->SetField(name.UTF8String, *msgArg.msgArg);
}

- (QStatus)setDefaultLanguage:(NSString *)language{
    return self.aboutData->SetDefaultLanguage(language.UTF8String);
}

- (NSString*)getFieldSignature:(NSString*)fieldName{
    FieldDetails *fieldDetails = self.aboutFields[fieldName];
    if (fieldDetails == nil) {
        return nil;
    }else{
        return fieldDetails.signature;
    }
}

- (BOOL)isFieldLocalized:(NSString*)fieldName{
    FieldDetails *fieldDetails = self.aboutFields[fieldName];
    if (fieldDetails == nil) {
        return NO;
    }else{
        return (fieldDetails.fieldMask  & LOCALIZED) == LOCALIZED;
    }
}

- (BOOL)isFieldAnnounced:(NSString*)fieldName{
    FieldDetails *fieldDetails = self.aboutFields[fieldName];
    if (fieldDetails == nil) {
        return NO;
    }else{
        return (fieldDetails.fieldMask  & ANNOUNCED) == ANNOUNCED;
    }
}

- (BOOL)isFieldRequired:(NSString*)fieldName{
    FieldDetails *fieldDetails = self.aboutFields[fieldName];
    if (fieldDetails == nil) {
        return NO;
    }else{
        return (fieldDetails.fieldMask  & REQUIRED) == REQUIRED;
    }
}

- (NSSet*)getFields{
    return [NSSet setWithArray:self.aboutFields.allKeys];
}

- (QStatus)setAppId:(uint8_t[])appId{
    QStatus status = ER_OK;
    
//    uint8_t appId[] = { 0x01, 0xB3, 0xBA, 0x14,
//        0x1E, 0x82, 0x11, 0xE4,
//        0x86, 0x51, 0xD1, 0x56,
//        0x1D, 0x5D, 0x46, 0xB0 };

    AJNMessageArgument *args = [[AJNMessageArgument alloc] init];
    [args setValue:@"ay",sizeof(appId) / sizeof(appId[0]),appId];
    [args stabilize];
    
    status = [self setField:kAPP_ID msgArg:args];
    
    return status;
}

- (QStatus)getDefaultLanguage:(NSString*)defaultLanguage{
    QStatus status;
    AJNMessageArgument *msgArg = [[AJNMessageArgument alloc] init];
    status = [self getField:kDEFAULT_LANGUAGE messageArg:msgArg language:defaultLanguage];
    if (status != ER_OK) {
        return status;
    }
    status = [msgArg value:self.aboutFields[kDEFAULT_LANGUAGE],&defaultLanguage];
    return status;
}

- (QStatus)setDeviceName:(NSString*)deviceName andLanguage:(NSString*)language{
    QStatus status = ER_OK;
    AJNMessageArgument *msgArg = [[AJNMessageArgument alloc] init];
    status = [msgArg setValue:self.aboutFields[kDEVICE_NAME],&deviceName];
    if (status != ER_OK) {
        return status;
    }
    status = [self setField:kDEVICE_NAME msgArg:msgArg];
    return status;
}

- (QStatus)getDeviceName:(NSString*)deviceName andLanguage:(NSString*)language{
    QStatus status;
    AJNMessageArgument *msgArg = [[AJNMessageArgument alloc] init];
    status = [self getField:kDEVICE_NAME messageArg:msgArg language:language];
    if (status != ER_OK) {
        return status;
    }
    status = [msgArg value:self.aboutFields[kDEVICE_NAME],&deviceName];
    return status;
}

- (QStatus)setDeviceId:(NSString*)deviceId{
    QStatus status = ER_OK;
    AJNMessageArgument *msgArg = [[AJNMessageArgument alloc] init];
    status = [msgArg setValue:self.aboutFields[kDEVICE_ID],&deviceId];
    if (status != ER_OK) {
        return status;
    }
    status = [self setField:kDEVICE_ID msgArg:msgArg];
    return status;
}

- (QStatus)getDeviceId:(NSString*)deviceId{
    QStatus status;
    AJNMessageArgument *msgArg = [[AJNMessageArgument alloc] init];
    status = [self getField:kDEVICE_ID messageArg:msgArg language:nil];
    if (status != ER_OK) {
        return status;
    }
    status = [msgArg value:self.aboutFields[kDEVICE_ID],&deviceId];
    return status;
}

- (QStatus)setAppName:(NSString*)appName andLanguage:(NSString*)language{
    QStatus status = ER_OK;
    AJNMessageArgument *msgArg = [[AJNMessageArgument alloc] init];
    status = [msgArg setValue:self.aboutFields[kAPP_NAME],&appName];
    if (status != ER_OK) {
        return status;
    }
    status = [self setField:kAPP_NAME msgArg:msgArg andLanguage:language];
    return status;
}

- (QStatus)getAppName:(NSString*)appName andLanguage:(NSString*)language{
    QStatus status;
    AJNMessageArgument *msgArg = [[AJNMessageArgument alloc] init];
    status = [self getField:kDEVICE_ID messageArg:msgArg language:nil];
    if (status != ER_OK) {
        return status;
    }
    status = [msgArg value:self.aboutFields[kAPP_NAME],&appName];
    return status;
}

- (QStatus)setManufacturer:(NSString*)manufacturer andLanguage:(NSString*)language{
    QStatus status = ER_OK;
    AJNMessageArgument *msgArg = [[AJNMessageArgument alloc] init];
    status = [msgArg setValue:self.aboutFields[kAPP_NAME],&manufacturer];
    if (status != ER_OK) {
        return status;
    }
    status = [self setField:kAPP_NAME msgArg:msgArg andLanguage:language];
    return status;
}

- (QStatus)getManufacturer:(NSString*)manufacturer andLanguage:(NSString*)language{
    QStatus status = ER_OK;
    AJNMessageArgument *msgArg = [[AJNMessageArgument alloc] init];
    status = [self getField:kMANUFACTURER messageArg:msgArg language:nil];
    if (status != ER_OK) {
        return status;
    }
    status = [msgArg value:self.aboutFields[kMANUFACTURER],&manufacturer];
    return status;
}

- (QStatus)setModelNumber:(NSString*)modelNumber{
    QStatus status = ER_OK;
    AJNMessageArgument *msgArg = [[AJNMessageArgument alloc] init];
    status = [msgArg setValue:self.aboutFields[kMODEL_NUMBER],&modelNumber];
    if (status != ER_OK) {
        return status;
    }
    status = [self setField:kMODEL_NUMBER msgArg:msgArg];
    return status;
}

- (QStatus)getModelNumber:(NSString*)modelNumber{
    QStatus status;
    AJNMessageArgument *msgArg = [[AJNMessageArgument alloc] init];
    status = [self getField:kMODEL_NUMBER messageArg:msgArg language:nil];
    if (status != ER_OK) {
        return status;
    }
    status = [msgArg value:self.aboutFields[kMODEL_NUMBER],&modelNumber];
    return status;
}

- (QStatus)setSupportedLanguage:(NSString*)language{
    //TODO:: KISHORE
    //TODO add in logic to check information about the tag all of the tags must
    //     conform to the RFC5646 there is currently nothing to make sure the
    //     tags conform to this RFC
   // bool added = false;
   // TODO -  QStatus status = [self.translatorDelegate addTargetLanguage:language withFlag:added];
    return ER_OK;
}

- (size_t)getSupportedLanguages:(NSString*)languageTags num:(size_t)num{
    //TODO:: KISHORE
    return 0;
}

- (QStatus)setDescription:(NSString*)description andLanguage:(NSString*)language{
    QStatus status = ER_OK;
    AJNMessageArgument *arg = [[AJNMessageArgument alloc] init];
    status = [arg setValue:self.aboutFields[kDESCRIPTION],&language];
    if (status != ER_OK) {
        return status;
    }
    status = [self setField:kDESCRIPTION msgArg:arg andLanguage:language];
    return status;
}

- (QStatus)getDescription:(NSString*)description language:(NSString*)language{
    QStatus status = ER_OK;
    AJNMessageArgument *msgArg = [[AJNMessageArgument alloc] init];
    status = [self getField:kDESCRIPTION messageArg:msgArg language:nil];
    if (status != ER_OK) {
        return status;
    }
    status = [msgArg value:self.aboutFields[kDESCRIPTION],&description];
    return status;
}


- (QStatus)setDateOfManufacture:(NSString*)dateOfManufacture
{
    //TODO check that the dateOfManufacture string is of the correct format YYYY-MM-DD
    QStatus status;
    AJNMessageArgument *msgArg = [[AJNMessageArgument alloc] init];
    status = [self getField:kDATE_OF_MANUFACTURE messageArg:msgArg language:nil];
    if (status != ER_OK) {
        return status;
    }
    status = [msgArg value:self.aboutFields[kDATE_OF_MANUFACTURE],&dateOfManufacture];
    return status;
}

- (QStatus)getDateOfManufacture:(NSString*)dateOfManufacture
{
    QStatus status = ER_OK;
    AJNMessageArgument *msgArg = [[AJNMessageArgument alloc] init];
    status = [self getField:kDATE_OF_MANUFACTURE messageArg:msgArg language:nil];
    if (status != ER_OK) {
        return status;
    }
    status = [msgArg value:self.aboutFields[kDATE_OF_MANUFACTURE],&dateOfManufacture];
    return status;
}

- (QStatus)setSoftwareVersion:(NSString*)softwareVersion
{
    QStatus status;
    AJNMessageArgument *msgArg = [[AJNMessageArgument alloc] init];
    status = [self getField:kSOFTWARE_VERSION messageArg:msgArg language:nil];
    if (status != ER_OK) {
        return status;
    }
    status = [msgArg value:self.aboutFields[kSOFTWARE_VERSION],&softwareVersion];
    return status;
}

- (QStatus)getSoftwareVersion:(NSString*)softwareVersion
{
    QStatus status = ER_OK;
    AJNMessageArgument *msgArg = [[AJNMessageArgument alloc] init];
    status = [self getField:kSOFTWARE_VERSION messageArg:msgArg language:nil];
    if (status != ER_OK) {
        return status;
    }
    status = [msgArg value:self.aboutFields[kSOFTWARE_VERSION],&softwareVersion];
    return status;
}

- (QStatus)getAJSoftwareVersion:(NSString*)ajSoftwareVersion
{
    QStatus status = ER_OK;
    AJNMessageArgument *msgArg = [[AJNMessageArgument alloc] init];
    status = [self getField:kAJ_SOFTWARE_VERSION messageArg:msgArg language:nil];
    if (status != ER_OK) {
        return status;
    }
    status = [msgArg value:self.aboutFields[kAJ_SOFTWARE_VERSION],&ajSoftwareVersion];
    return status;
}

- (QStatus)setHardwareVersion:(NSString*)hardwareVersion
{
    QStatus status = ER_OK;
    AJNMessageArgument *msgArg = [[AJNMessageArgument alloc] init];
    status = [self getField:kHARDWARE_VERSION messageArg:msgArg language:nil];
    if (status != ER_OK) {
        return status;
    }
    status = [msgArg value:self.aboutFields[kHARDWARE_VERSION],&hardwareVersion];
    return status;
}

- (QStatus)getHardwareVersion:(NSString*)hardwareVersion
{
    QStatus status = ER_OK;
    AJNMessageArgument *msgArg = [[AJNMessageArgument alloc] init];
    status = [self getField:kHARDWARE_VERSION messageArg:msgArg language:nil];
    if (status != ER_OK) {
        return status;
    }
    status = [msgArg value:self.aboutFields[kHARDWARE_VERSION],&hardwareVersion];
    return status;
}

- (QStatus)setSupportUrl:(NSString*)supportUrl
{
    //TODO:: KISHORE
    return ER_OK;
}

- (QStatus)getSupportUrl:(NSString*)supportUrl
{
    //TODO:: KISHORE
    return ER_OK;
}


#pragma mark- AJNAboutDataListener
- (QStatus)getAboutDataForLanguage:(NSString *)language usingDictionary:(NSMutableDictionary *__autoreleasing *)aboutData{
    //TODO:: KISHORE
    return ER_FAIL;
}

- (QStatus)getDefaultAnnounceData:(NSMutableDictionary *__autoreleasing *)aboutData{
    //TODO:: KISHORE
    return ER_FAIL;
}

@end
