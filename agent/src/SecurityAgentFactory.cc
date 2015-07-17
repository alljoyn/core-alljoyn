/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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

#include <alljoyn/securitymgr/SecurityAgentFactory.h>

#include "SecurityAgentImpl.h"

#define QCC_MODULE "SECMGR_AGENT"

using namespace ajn;
using namespace qcc;
using namespace ajn::securitymgr;

SecurityAgentFactory::SecurityAgentFactory()
{
    ownBa = false;
    ba = nullptr;
}

SecurityAgentFactory::~SecurityAgentFactory()
{
    if (ownBa == true) {
        ba->Disconnect();
        ba->Stop();
        ba->Join();
        delete ba;
        ba = nullptr;
    }
}

QStatus SecurityAgentFactory::GetSecurityAgent(const shared_ptr<AgentCAStorage>& caStorage,
                                               shared_ptr<SecurityAgent>& agentRef,
                                               BusAttachment* _ba)
{
    shared_ptr<SecurityAgentImpl> sa = nullptr;
    QStatus status = ER_OK;
    if (_ba == nullptr) {
        if (ba == nullptr) {
            ba =
                new BusAttachment("SecurityAgent", true);
            ownBa = true;
            do {
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
        }
    } else {
        ba = _ba;
    }

    sa = make_shared<SecurityAgentImpl>(caStorage, ba);
    if (ER_OK == (status = sa->Init())) {
        agentRef = sa;
    } else {
        agentRef = nullptr;
    }
    return status;
}

#undef QCC_MODULE
