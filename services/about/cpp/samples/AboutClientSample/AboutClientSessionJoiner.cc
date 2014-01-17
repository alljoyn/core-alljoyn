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

#include "AboutClientSessionJoiner.h"
#include "AboutClientSessionListener.h"
#include <iostream>

using namespace ajn;

AboutClientSessionJoiner::AboutClientSessionJoiner(const char* name, SessionJoinedCallback callback) :
    m_Busname(""), m_Callback(callback)
{
    if (name) {
        m_Busname.assign(name);
    }
}

AboutClientSessionJoiner::~AboutClientSessionJoiner()
{

}

void AboutClientSessionJoiner::JoinSessionCB(QStatus status, SessionId id, const SessionOpts& opts, void* context)
{
    if (status == ER_OK) {
        std::cout << "JoinSessionCB(" << m_Busname.c_str() << ") succeeded with id" << id << std::endl;
        if (m_Callback) {
            std::cout << "Calling SessionJoiner Callback" << std::endl;
            m_Callback(m_Busname, id);
        }
    } else {
        std::cout << "JoinSessionCB(" << m_Busname.c_str() << ") failed with status: " << QCC_StatusText(status) << std::endl;
    }

    AboutClientSessionListener* t = (AboutClientSessionListener*) context;
    delete t;
    delete this;
}
