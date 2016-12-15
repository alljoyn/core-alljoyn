////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
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

#import "AJNTranslatorImpl.h"


AJNTranslatorImpl::AJNTranslatorImpl(id<AJNTranslator> aDelegate) : m_delegate(aDelegate)
{

}

/**
 * Virtual destructor for derivable class.
 */
AJNTranslatorImpl::~AJNTranslatorImpl()
{
    m_delegate = nil;
}

size_t AJNTranslatorImpl::NumTargetLanguages()
{
    return [m_delegate numTargetLanguages];
}

void AJNTranslatorImpl::GetTargetLanguage(size_t index, qcc::String& ret)
{
    ret.assign([[m_delegate targetLanguage:(index)] UTF8String]);
}

const char* AJNTranslatorImpl::Translate(const char* sourceLanguage, const char* targetLanguage, const char* source, qcc::String& buffer)
{
    NSString* fromLang = [[NSString alloc] initWithUTF8String:(sourceLanguage)];
    NSString* toLang = [[NSString alloc] initWithUTF8String:(targetLanguage)];
    NSString* text = [[NSString alloc] initWithUTF8String:(source)];

    NSString* translation = [m_delegate translateText:(text) from:(fromLang) to:(toLang)];
    if(translation == nil)
    {
        return 0;
    }

    buffer.assign([translation UTF8String]);
    return buffer.c_str();
}

QStatus AJNTranslatorImpl::AddTargetLanguage(const char* language, bool *added)
{
    BOOL ajnAdded = *added ? YES : NO;
    return [m_delegate addTargetLanguage:[NSString stringWithCString:language encoding:NSUTF8StringEncoding] hasBeenAdded:&ajnAdded];
}

QStatus AJNTranslatorImpl::TranslateToMsgArg(const char *sourceLanguage, const char *targetLanguage, const char* sourceText, ajn::MsgArg *&msgarg)
{
    NSString *srcLang = [[NSString alloc] initWithUTF8String:sourceLanguage];
    NSString *trgLang = [[NSString alloc] initWithUTF8String:targetLanguage];
    NSString *srcText = [[NSString alloc] initWithUTF8String:sourceText];
    AJNMessageArgument *ajnMsgArg = [[AJNMessageArgument alloc] initWithHandle:msgarg];
    
    return [m_delegate translateToMsgArg:srcLang forLanguage:trgLang fromText:srcText msgArg:&ajnMsgArg];
}

QStatus AJNTranslatorImpl::AddStringTranslation(const char *id, const char *value, const char *language)
{
    return [m_delegate addStringTranslation:[NSString stringWithCString:id encoding:NSUTF8StringEncoding] textTranslation:[NSString stringWithCString:value encoding:NSUTF8StringEncoding] forLanguage:[NSString stringWithCString:language encoding:NSUTF8StringEncoding]];
}

QStatus AJNTranslatorImpl::AddMsgArgTranslation(const char *id, const ajn::MsgArg *value, const char* language)
{
    return [m_delegate addMsgArgTranslation:[NSString stringWithCString:id encoding:NSUTF8StringEncoding] msgTranslation:[[AJNMessageArgument alloc] initWithHandle:(AJNHandle)value] forLanguage:[NSString stringWithCString:id encoding:NSUTF8StringEncoding]];
}

void AJNTranslatorImpl::GetBestLanguage(const char *requested, const qcc::String &defaultLanguage, qcc::String &ret)
{
    NSString *ajnReq = nil;
    if (requested) {
        ajnReq = [NSString stringWithCString:requested encoding:NSUTF8StringEncoding];
    }
    NSString *ajnDefaultLang = [NSString stringWithCString:defaultLanguage.c_str() encoding:NSUTF8StringEncoding];
    NSString *ajnBestLang = [m_delegate bestLanguage:ajnReq useDefaultLanguage:ajnDefaultLang];
    if (ajnBestLang != nil && [ajnBestLang length] != 0) {
        ret.assign_std([ajnBestLang UTF8String], [ajnBestLang length]);
    }
}

