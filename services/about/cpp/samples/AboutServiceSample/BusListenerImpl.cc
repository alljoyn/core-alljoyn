/******************************************************************************
 * Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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

#include "BusListenerImpl.h"
#include <iostream>

using namespace ajn;

BusListenerImpl::BusListenerImpl() :
    BusListener(), SessionPortListener(), m_SessionPort(0)
{
}

BusListenerImpl::BusListenerImpl(ajn::SessionPort sessionPort) :
    BusListener(), SessionPortListener(), m_SessionPort(sessionPort)
{

}

BusListenerImpl::~BusListenerImpl()
{
}

void BusListenerImpl::setSessionPort(ajn::SessionPort sessionPort)
{
    m_SessionPort = sessionPort;
}

SessionPort BusListenerImpl::getSessionPort()
{
    return m_SessionPort;
}

bool BusListenerImpl::AcceptSessionJoiner(ajn::SessionPort sessionPort, const char* joiner, const ajn::SessionOpts& opts)
{
    if (sessionPort != m_SessionPort) {
        std::cout << "Rejecting join attempt on unexpected session port " << sessionPort << std::endl;
        return false;
    }

    std::cout << "Accepting JoinSessionRequest from " << joiner << " (opts.proximity= " << opts.proximity
              << ", opts.traffic=" << opts.traffic << ", opts.transports=" << opts.transports << ")." << std::endl;
    return true;
}
