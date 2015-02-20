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

#include <alljoyn/securitymgr/SecurityManagerFactory.h>
#include <alljoyn/securitymgr/SecurityManager.h>

#include <qcc/CryptoECC.h>

#define QCC_MODULE "SEC_MGR"

using namespace ajn;
using namespace qcc;
using namespace securitymgr;

SecurityManagerFactory::SecurityManagerFactory()
{
    ownBa = false;
    status = ER_OK;
    ba = NULL;
}

SecurityManagerFactory::~SecurityManagerFactory()
{
    if (ownBa == true) {
        ba->Disconnect();
        ba->Stop();
        ba->Join();
        delete ba;
    }
}

SecurityManager* SecurityManagerFactory::GetSecurityManager(const Storage* storage,
                                                            BusAttachment* _ba)
{
    SecurityManager* sm = NULL;

    if (!storage) {
        QCC_LogError(ER_FAIL, ("NULL Storage"));
        return NULL;
    }

    if (_ba == NULL) {
        if (ba == NULL) {
            ba = new BusAttachment("SecurityMgr", true);
            ownBa = true;
            do {
                // \todo provide sensible data or remove name argument
                status = ba->Start();
                if (status != ER_OK) {
                    QCC_LogError(status, ("Failed to start bus attachment"));
                    break;
                }

                status = ba->Connect();
                if (status != ER_OK) {
                    QCC_LogError(status, ("Failed to connect bus attachment"));
                    break;
                }
            } while (0);
            _ba = ba;             //TODO: don't override; clean up code
        }
    }

    sm = new SecurityManager(_ba, storage);
    if (NULL == sm || ER_OK != sm->Init()) {
        delete sm;
        sm = NULL;
    }
    return sm;
}

#undef QCC_MODULE
