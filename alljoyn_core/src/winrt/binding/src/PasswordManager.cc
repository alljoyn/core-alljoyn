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

#include "PasswordManager.h"

#include <alljoyn/PasswordManager.h>
#include <qcc/String.h>
#include <qcc/winrt/utility.h>

using namespace Windows::Foundation;

namespace AllJoyn {

QStatus PasswordManager::SetCredentials(Platform::String ^ authMechanism, Platform::String ^ password) {
    qcc::String strAuthMechanism = PlatformToMultibyteString(authMechanism);
    qcc::String strPassword = PlatformToMultibyteString(password);
    return (AllJoyn::QStatus)ajn::PasswordManager::SetCredentials(strAuthMechanism.c_str(), strPassword.c_str());

}
}
