////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
    ret.assign([[m_delegate getTargetLanguage:(index)] UTF8String]);
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

