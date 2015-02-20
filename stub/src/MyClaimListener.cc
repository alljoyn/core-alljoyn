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

#include "MyClaimListener.h"
#include "Stub.h"

MyClaimListener::~MyClaimListener()
{
}

/* This can be used, if for example the app has a nice gui
 * and asks the user if he wants to be claimed by this ROT */
bool MyClaimListener::OnClaimRequest(const qcc::ECCPublicKey* pubKeyRot, void* ctx)
{
    printf("Auto-accepting claim request\r\n");
    return true;
}

void MyClaimListener::OnClaimed(void* ctx)
{
    Stub* stub = static_cast<Stub*>(ctx);

    stub->GetBusAttachment().EnableConcurrentCallbacks();     /* nasty: TODO get rid of this */
    stub->CloseClaimWindow();
}
