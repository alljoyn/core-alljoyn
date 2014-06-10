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

#ifndef AJN_DESCRIPTION_TRANSLATOR_IMPL_H
#define AJN_DESCRIPTION_TRANSLATOR_IMPL_H

#import <Foundation/Foundation.h>
#import "alljoyn/Translator.h"
#import "AJNTranslator.h"

class AJNTranslatorImpl : public ajn::Translator {
  protected:

    __strong id<AJNTranslator> m_delegate;

  public:

    AJNTranslatorImpl(id<AJNTranslator> aDelegate);
    
    virtual ~AJNTranslatorImpl();
 
    virtual size_t NumTargetLanguages();

    virtual void GetTargetLanguage(size_t index, qcc::String& ret);

    virtual const char* Translate(const char* sourceLanguage, const char* targetLanguage, const char* source, qcc::String& buffer);
    
    id<AJNTranslator> getDelegate();

    void setDelegate(id<AJNTranslator> delegate);
};

// inline methods
//

inline id<AJNTranslator> AJNTranslatorImpl::getDelegate()
{
    return m_delegate;
}

inline void AJNTranslatorImpl::setDelegate(id<AJNTranslator> delegate)
{
    m_delegate = delegate;
}

#endif
