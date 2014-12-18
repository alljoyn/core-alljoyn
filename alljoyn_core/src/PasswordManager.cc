/**
 * @file
 * This file implements the PasswordManager class that provides the interface to
 * set credentials used for the authentication of thin clients.
 */

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

#include <qcc/platform.h>
#include <qcc/String.h>
#include "PasswordManager.h"

namespace ajn {
using namespace qcc;
String* PasswordManager::authMechanism = NULL;
String* PasswordManager::password = NULL;

static int passwordManagerCounter = 0;
bool PasswordManagerInit::cleanedup = false;
PasswordManagerInit::PasswordManagerInit()
{
    if (0 == passwordManagerCounter++) {
        PasswordManager::authMechanism = new String("ANONYMOUS");
        PasswordManager::password = new String();
    }
}

PasswordManagerInit::~PasswordManagerInit()
{
    if (0 == --passwordManagerCounter && !cleanedup) {
        delete PasswordManager::authMechanism;
        PasswordManager::authMechanism = NULL;
        delete PasswordManager::password;
        PasswordManager::password = NULL;
        cleanedup = true;
    }
}

void PasswordManagerInit::Cleanup()
{
    if (!cleanedup) {
        delete PasswordManager::authMechanism;
        PasswordManager::authMechanism = NULL;
        delete PasswordManager::password;
        PasswordManager::password = NULL;
        cleanedup = true;
    }
}

}
