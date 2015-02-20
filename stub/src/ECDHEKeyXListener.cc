/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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

#include "ECDHEKeyXListener.h"

ECDHEKeyXListener::ECDHEKeyXListener()
{
}

bool ECDHEKeyXListener::RequestCredentials(const char* authMechanism,
                                           const char* authPeer,
                                           uint16_t authCount,
                                           const char* userId,
                                           uint16_t credMask,
                                           Credentials& creds)
{
    printf("ECDHEKeyXListener::RequestCredentials %s\n", authMechanism);

    // only allow ECDHE_NULL sessions for now
    if (strcmp(authMechanism, KEYX_ECDHE_NULL) == 0) {
        creds.SetExpiration(100);          /* set the master secret expiry time to 100 seconds */
        return true;
    }
    return false;
}

bool ECDHEKeyXListener::VerifyCredentials(const char* authMechanism, const char* authPeer, const Credentials& creds)
{
    fprintf(stderr, "ECDHEKeyXListener::VerifyCredentials %s\n", authMechanism);
    if (strcmp(authMechanism, "ALLJOYN_ECDHE_ECDSA") == 0) {
        return true;
    }
    return false;
}

void ECDHEKeyXListener::AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success)
{
    fprintf(stderr, "ECDHEKeyXListener::AuthenticationComplete %s success = %i\n", authMechanism, success);
}
