/******************************************************************************
 *
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
 *
 *****************************************************************************/

#include "TypeCoercerFactory.h"

#include <StrictTypeCoercer.h>
#include <WeakTypeCoercer.h>
#include <qcc/String.h>
#include <qcc/winrt/utility.h>
#include <alljoyn/Status.h>
#include <AllJoynException.h>
#include <qcc/Mutex.h>

namespace AllJoyn {

StrictTypeCoercer* _strictCoercer = NULL;
WeakTypeCoercer* _weakCoercer = NULL;
qcc::Mutex _typeFactoryMutex;

ITypeCoercer* TypeCoercerFactory::GetTypeCoercer(Platform::String ^ name)
{
    ITypeCoercer* result = NULL;

    // Check if factory requested is "strict" or "weak"
    if (wcscmp(L"strict", name->Data()) == 0) {
        _typeFactoryMutex.Lock();
        // Allocate StrictTypeCoercer type coercer
        if (NULL == _strictCoercer) {
            _strictCoercer = new StrictTypeCoercer();
        }
        _typeFactoryMutex.Unlock();
        // Store the result
        result =  _strictCoercer;
    } else if (wcscmp(L"weak", name->Data()) == 0) {
        _typeFactoryMutex.Lock();
        // Allocate WeakTypeCoercer type coercer
        if (NULL == _weakCoercer) {
            _weakCoercer = new WeakTypeCoercer();
        }
        _typeFactoryMutex.Unlock();
        // Store the result
        result = _weakCoercer;
    }

    return result;
}

}
