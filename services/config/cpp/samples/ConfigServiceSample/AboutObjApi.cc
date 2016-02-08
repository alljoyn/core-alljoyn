/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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

#include <alljoyn/AboutObj.h>
#include <alljoyn/AboutData.h>
#include "AboutObjApi.h"

using namespace ajn;
using namespace services;

AboutObjApi* AboutObjApi::m_instance = NULL;
BusAttachment* AboutObjApi::m_BusAttachment = NULL;
AboutData* AboutObjApi::m_AboutData = NULL;
AboutObj* AboutObjApi::m_AboutObj = NULL;
SessionPort AboutObjApi::m_sessionPort = 0;

AboutObjApi::AboutObjApi() {
}

AboutObjApi::~AboutObjApi() {
}

AboutObjApi* AboutObjApi::getInstance() {

    if (!m_instance) {
        m_instance = new AboutObjApi();
    }
    return m_instance;
}

void AboutObjApi::Init(ajn::BusAttachment* bus, AboutData* aboutData, AboutObj* aboutObj) {
    m_BusAttachment = bus;
    m_AboutData = aboutData;
    m_AboutObj = aboutObj;
}

void AboutObjApi::DestroyInstance() {
    if (m_instance) {
        delete m_instance;
        m_instance = NULL;
    }
}

void AboutObjApi::SetPort(SessionPort sessionPort) {
    m_sessionPort = sessionPort;
}

QStatus AboutObjApi::Announce() {
    if (m_AboutObj == NULL) {
        return ER_FAIL;
    }
    return m_AboutObj->Announce(m_sessionPort, *m_AboutData);
}
