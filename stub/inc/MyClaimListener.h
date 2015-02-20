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

#ifndef MYCLAIMLISTENER_H_
#define MYCLAIMLISTENER_H_

#include <alljoyn/BusAttachment.h>
#include <qcc/String.h>
#include "PermissionMgmt.h"

/**
 * This class is used to provide the application with feedback/interaction for the claiming process
 */
class MyClaimListener :
    public ClaimListener {
  public:

    MyClaimListener() { }

    ~MyClaimListener();

  private:

    /* This function allows the application to present the user
     * with a notification that the application is being claimed
     * or even give the user the ability to deny the claiming
     */
    bool OnClaimRequest(const qcc::ECCPublicKey* pubKeyRot,
                        void* ctx);

    /* This function is called when the claiming process has completed successfully */
    void OnClaimed(void* ctx);
};

#endif /* MYCLAIMLISTENER_H_ */
