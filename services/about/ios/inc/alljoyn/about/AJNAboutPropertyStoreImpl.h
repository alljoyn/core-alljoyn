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

#import <Foundation/Foundation.h>
#import "alljoyn/Status.h"
#import "AJNMessageArgument.h"
#import "alljoyn/about/AboutPropertyStoreImpl.h"
#import "AJNPropertyStore.h"
#import "AJNPropertyStoreProperty.h"
@class PropertyStoreImpl;
@class AJNMessageArgument;

typedef ajn::services::AboutPropertyStoreImpl * (^HandleAllocationBlock)(void);

/**
 AJNAboutPropertyStoreImpl is the default implementation , it is responsible to store the properties of the AJNAboutService and QCSConfigService in memory.
 */
@interface AJNAboutPropertyStoreImpl : NSObject <AJNPropertyStore>

/**
 List of PropertyStore keys.
 */
typedef enum {
    DEVICE_ID = 0,
    DEVICE_NAME = 1,
    APP_ID = 2,
    APP_NAME = 3,
    DEFAULT_LANG = 4,
    SUPPORTED_LANGS = 5,
    DESCRIPTION = 6,
    MANUFACTURER = 7,
    DATE_OF_MANUFACTURE = 8,
    MODEL_NUMBER = 9,
    SOFTWARE_VERSION = 10,
    AJ_SOFTWARE_VERSION = 11,
    HARDWARE_VERSION = 12,
    SUPPORT_URL = 13,
    NUMBER_OF_KEYS = 14
} AJNPropertyStoreKey;

/**
 Initializer
 */
- (id)init;

/**
 Designated initializer
 Create an AJNAboutPropertyStoreImpl Object using the passed parameters.
 @param Block a block that return a pointer to the C++ property store implementation.
 @return AJNAboutPropertyStoreImpl if successful.
 */
- (id)initWithHandleAllocationBlock:(HandleAllocationBlock) Block;

/**
 ReadAll populate a AJNMessageArgument according to the given languageTag and filter.
 @param languageTag The language to use for the action (NULL means default).
 @param filter Describe which properties to read.
 @param all A reference to AJNMessageArgument [out].
 @return ER_OK if successful.
 */
- (QStatus)readAll:(const char*)languageTag withFilter:(PFilter)filter ajnMsgArg:(AJNMessageArgument **)all;

/**
 Update is not implemented in this class.
 @param name not used
 @param languageTag not used
 @param value not used
 @return ER_NOT_IMPLEMENTED
 */
- (QStatus)Update:(const char *)name languageTag:(const char *)languageTag ajnMsgArg:(AJNMessageArgument *)value;

/**
 Delete is not implemented in this class.
 @param name not used
 @param languageTag not used
 @return ER_NOT_IMPLEMENTED
 */
- (QStatus)delete:(const char *)name languageTag:(const char *)languageTag;

/**
 property will return a AJNPropertyStoreProperty according to a property key. Each property key defines one kind of properties
 @param propertyKey one of AJNPropertyStoreKey
 @return AJNPropertyStoreProperty*
 */
- (AJNPropertyStoreProperty*) property:(AJNPropertyStoreKey)propertyKey;

/**
 property same as above with language
 @param propertyKey one of AJNPropertyStoreKey
 @param language The language to use for the action (NULL means default).
 @return AJNPropertyStoreProperty*
 */
- (AJNPropertyStoreProperty*) property:(AJNPropertyStoreKey)propertyKey withLanguage:(NSString *)language;

/**
 setDeviceId sets the device ID property in the property store
 @param deviceId new device ID to set
 @return ER_OK if successful.
 */
- (QStatus)setDeviceId:(NSString *)deviceId;

/**
 setDeviceName sets the device name property in the property store
 @param deviceName new device name to set
 @return ER_OK if successful.
 */
- (QStatus)setDeviceName:(NSString *)deviceName __attribute__ ((deprecated));

/**
 setDeviceName sets the device name property in the property store
 @param deviceName new device name to set
 @param language define this property for a specific language
 @return ER_OK if successful.
 */
- (QStatus)setDeviceName:(NSString *)deviceName language:(NSString *)language;

/**
 setAppId sets the app ID property in the property store
 @param appId new app ID to set
 @return ER_OK if successful.
 */
- (QStatus)setAppId:(NSString *)appId;
/**
 setAppName sets the app name property in the property store
 @param appName new app name to set
 @return ER_OK if successful.
 */
- (QStatus)setAppName:(NSString *)appName;
/**
 setDefaultLang sets the default language property in the property store
 @param defaultLang new default language to set
 @return ER_OK if successful.
 */
- (QStatus)setDefaultLang:(NSString *)defaultLang;
/**
 setSupportedLangs sets the suppoerted languages property in the property store
 @param supportedLangs an array of NSStrings with the languages (i.e. @"en",@"ru")
 @return ER_OK if successful.
 */
- (QStatus)setSupportedLangs:(NSArray *)supportedLangs;

/**
 setDescription sets the description property in the property store per language
 @param description new description string to set
 @param language the language for this description
 @return ER_OK if successful.
 */
- (QStatus)setDescription:(NSString *)description language:(NSString *)language;
/**
 setManufacturer sets the manufacturer property in the property store per language
 @param manufacturer new manufacturer string to set
 @param language the language for this manufacturer
 @return ER_OK if successful.
 */
- (QStatus)setManufacturer:(NSString *)manufacturer  language:(NSString *)language;

/**
 setDateOfManufacture sets the date of manufacture property in the property store
 @param dateOfManufacture date of manufacture to set
 @return ER_OK if successful.
 */
- (QStatus)setDateOfManufacture:(NSString *)dateOfManufacture;
/**
 setSoftwareVersion sets the software version property in the property store
 @param softwareVersion the new software version to set
 @return ER_OK if successful.
 */
- (QStatus)setSoftwareVersion:(NSString *)softwareVersion;
/**
 setAjSoftwareVersion sets the alljoyn software version property in the property store
 @param ajSoftwareVersion alljoyn software version to set
 @return ER_OK if successful.
 */
- (QStatus)setAjSoftwareVersion:(NSString *)ajSoftwareVersion;
/**
 setHardwareVersion sets the alljoyn software version property in the property store
 @param hardwareVersion hardware version to set
 @return ER_OK if successful.
 */
- (QStatus)setHardwareVersion:(NSString *)hardwareVersion;

/**
 setModelNumber sets the model number property in the property store
 @param modelNumber the model number to set
 @return ER_OK if successful.
 */
- (QStatus)setModelNumber:(NSString *)modelNumber;

/**
 setSupportUrl sets the support URL property in the property store
 @param supportUrl support url to set
 @return ER_OK if successful.
 */
- (QStatus)setSupportUrl:(NSString *)supportUrl;

/**
 propertyStoreName returns the property store name for a specific property store key
 @param propertyStoreKey one of AJNPropertyStoreKey
 @return the name corresponding to the key
 */
- (NSString*)propertyStoreName:(AJNPropertyStoreKey) propertyStoreKey;

/**
 getHandle get the C++ handle of the property store in use
 @return ajn::services::AboutPropertyStoreImpl* or inheriting children
 */
- (ajn::services::AboutPropertyStoreImpl *)getHandle;

@end
