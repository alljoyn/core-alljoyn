////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
//    Project (AJOSP) Contributors and others.
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
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//    PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import "AJNObject.h"
#import "AJNMessageArgument.h"

/**
 * Protocol implemented by AllJoyn apps and called by AllJoyn to provide
 * translation of human readable strings
 */
@protocol AJNTranslator <NSObject>

/**
 * Get the size of the list of target translation languages
 *
 * @return the size of the list of target translation languages
 */
- (size_t)numTargetLanguages;

/**
 * Retrieve one of the list of target translation languages
 *
 * @param index which translation target
 *
 * @return the translation target requested or nil if the index was out of bounds
 */
- (NSString*)targetLanguage:(size_t)index;

/**
 * Translate a string
 *
 * @param text the string to be translated
 * @param fromLang the language "text" is in
 * @param toLang the language to translate "text" to
 *
 * @return the translation of "text" or nil
 */
- (NSString*)translateText:(NSString*)text from:(NSString*)fromLang to:(NSString*)toLang;

/**
 * Get the best matching language according to RFC 4647 section 3.4.
 *
 * @param requested The requested IETF language range.
 * @param defaultLanguage The default language to use.
 *
 * @return the returned value
 */
- (NSString*)bestLanguage:(NSString*)requested useDefaultLanguage:(NSString*)defaultLanguage;

/**
 * Add a language to the set of supported target languages.
 *
 * @param[in] language The IETF language tag specified by RFC 5646
 * @param[out] added Returns true if added, false if already present
 * @return
 *  - #ER_OK on success
 *  - #ER_NOT_IMPLEMENTED if translator does not support adding target languages.
 */
- (QStatus)addTargetLanguage:(NSString*)language hasBeenAdded:(BOOL*)added;

/**
 * Translate an id or source text from sourceLanguage into text in the
 * given targetLanguage. If this Translator does not support MsgArgs it
 * should return NULL.
 * This version of the function is designed for implementations that
 * return a pointer to a MsgArg that will not go away. This is required
 * by the AboutData::GetField() API.
 *
 * @param[in] sourceLanguage The language tag of the text in sourcetext. If
 *  sourceLanguage is NULL or empty, then sourceText is simply an id used
 *  for lookup.
 * @param[in] targetLanguage The language tag to translate into
 * @param[in] sourceText The source text to translate
 * @param[out] msgarg Returns the MsgArg containing the translation
 * @return
 *  - #ER_OK on success
 *  - #ER_NOT_IMPLEMENTED if translator does not support MsgArgs
 */
- (QStatus)translateToMsgArg:(NSString*)sourceLanguage forLanguage:(NSString*)targetLanguage fromText:(NSString*)sourceText msgArg:(AJNMessageArgument**)msgArg;

/**
 * Add new localized text.
 *
 * @param withId The id of the localized text to add.
 * @param value The localized text to be associated with the id.
 * @param language The IETF language tag specified by RFC 5646.
 *
 * @return
 *  - #ER_OK on success
 *  - #ER_NOT_IMPLEMENTED if translator does not support adding target languages.
 */
- (QStatus)addStringTranslation:(NSString*)withId textTranslation:(NSString*)value forLanguage:(NSString*)language;

/**
 * Add new localized text.
 *
 * @param withId The id of the localized text to add.
 * @param value The localized text to be associated with the id.
 * @param language The IETF language tag specified by RFC 5646.
 *
 * @return
 *  - #ER_OK on success
 *  - #ER_NOT_IMPLEMENTED if translator does not support adding target languages or the MsgArg type.
 */
- (QStatus)addMsgArgTranslation:(NSString*)withId msgTranslation:(AJNMessageArgument*)value forLanguage:(NSString*)language;

@end
