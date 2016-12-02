////////////////////////////////////////////////////////////////////////////////
// // 
//    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
//    Source Project Contributors and others.
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0

////////////////////////////////////////////////////////////////////////////////

#ifndef AJN_DESCRIPTION_TRANSLATOR_IMPL_H
#define AJN_DESCRIPTION_TRANSLATOR_IMPL_H

#import <Foundation/Foundation.h>
#import "alljoyn/Translator.h"
#import <alljoyn/MsgArg.h>
#import "AJNTranslator.h"

class AJNTranslatorImpl : public ajn::Translator {
  protected:

    __strong id<AJNTranslator> m_delegate;

  public:

    AJNTranslatorImpl(id<AJNTranslator> aDelegate);

    virtual ~AJNTranslatorImpl();

    virtual size_t NumTargetLanguages();

    virtual void GetTargetLanguage(size_t index, qcc::String& ret);

    virtual const char* Translate(const char *sourceLanguage, const char* targetLanguage, const char* source, qcc::String& buffer);

    virtual QStatus AddTargetLanguage(const char *language, bool *added=NULL);

    virtual QStatus TranslateToMsgArg(const char *sourceLanguage, const char *targetLanguage, const char* sourceText, ajn::MsgArg *&msgarg);

    virtual QStatus AddStringTranslation(const char *id, const char *value, const char *language);

    virtual QStatus AddMsgArgTranslation(const char *id, const ajn::MsgArg *value, const char* language);

    virtual void GetBestLanguage(const char *requested, const qcc::String &defaultLanguage, qcc::String &ret);

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