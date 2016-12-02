/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#import "AJNPropertyStoreProperty.h"
#import "AJNConvertUtil.h"
#import "AJNMessageArgument.h"

@interface AJNMessageArgument (Private)

@property (nonatomic, readonly) ajn::MsgArg *msgArg;

@end

@interface AJNPropertyStoreProperty ()
@property (nonatomic) ajn::services::PropertyStoreProperty *handle;

@end

@implementation AJNPropertyStoreProperty

- (id)initWithHandle:(ajn::services::PropertyStoreProperty *)handle
{
	self = [super init];
	if (self) {
		self.handle = handle;
	}
	return self;
}

- (id)initPropertyStorePropertyWithKey:(NSString *)keyName
{
	self = [super init];
	if (self) {
		self.handle = new ajn::services::PropertyStoreProperty([keyName UTF8String]);
	}
	return self;
}

- (id)initPropertyStorePropertyWithKey:(NSString *)keyName value:(AJNMessageArgument *)value
{
	self = [super init];
	if (self) {
		self.handle = new ajn::services::PropertyStoreProperty([keyName UTF8String], *value.msgArg);
	}
	return self;
}

- (id)initPropertyStorePropertyWithKey:(NSString *)keyName value:(AJNMessageArgument *)value isPublic:(bool)isPublic isWritable:(bool)isWritable isAnnouncable:(bool)isAnnouncable
{
	self = [super init];
	if (self) {
		self.handle = new ajn::services::PropertyStoreProperty([keyName UTF8String], *value.msgArg, isPublic, isWritable, isAnnouncable);
	}
	return self;
}

- (id)initPropertyStorePropertyWithKey:(NSString *)keyName value:(AJNMessageArgument *)value language:
(NSString *)language      isPublic:(bool)isPublic isWritable:(bool)isWritable isAnnouncable:(bool)isAnnouncable
{
	self = [super init];
	if (self) {
		self.handle = new ajn::services::PropertyStoreProperty([keyName UTF8String], *value.msgArg, [language UTF8String], isPublic, isWritable, isAnnouncable);
	}
	return self;
}

- (void)dealloc
{
	// The handle comes from the outside, delete shouldn't be called here
}

- (void)setFlagsIsPublic:(bool)isPublic isWritable:(bool)isWritable isAnnouncable:(bool)isAnnouncable
{
	self.handle->setFlags(isPublic, isWritable, isAnnouncable);
}

// Since this is a wrapper over the C++ object simply call the setters and getters of C++ through the property synthesizer

@synthesize isPublic = _isPublic;
- (void)setIsPublic:(bool)isPublic
{
	self.handle->setIsPublic(isPublic);
}

- (bool)isPublic
{
	return self.handle->getIsPublic();
}

@synthesize isWritable = _isWritable;
- (void)setIsWritable:(bool)isWritable
{
	self.handle->setIsWritable(isWritable);
}

- (bool)isWritable
{
	return self.handle->getIsWritable();
}

@synthesize isAnnouncable = _isAnnouncable;
- (void)setIsAnnouncable:(bool)isAnnouncable
{
	self.handle->setIsAnnouncable(isAnnouncable);
}

- (bool)isAnnouncable
{
	return self.handle->getIsAnnouncable();
}

@synthesize language = _language;
- (void)setLanguage:(NSString *)language
{
	self.handle->setLanguage([language UTF8String]);
}

- (NSString *)language
{
	return [AJNConvertUtil convertQCCStringtoNSString:self.handle->getLanguage()];
}

@end