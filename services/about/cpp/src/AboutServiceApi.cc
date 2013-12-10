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

#include <alljoyn/about/AboutServiceApi.h>

using namespace ajn;
using namespace services;

AboutServiceApi* AboutServiceApi::m_instance = NULL;
BusAttachment* AboutServiceApi::m_BusAttachment = NULL;
PropertyStore* AboutServiceApi::m_PropertyStore = NULL;

AboutServiceApi::AboutServiceApi(ajn::BusAttachment& bus, PropertyStore& store) :
    AboutService(bus, store) {

}

AboutServiceApi::~AboutServiceApi() {

}

AboutServiceApi* AboutServiceApi::getInstance() {
    if (!m_BusAttachment || !m_PropertyStore) {
        return NULL;
    }
    if (!m_instance) {
        m_instance = new AboutServiceApi(*m_BusAttachment, *m_PropertyStore);
    }
    return m_instance;
}

void AboutServiceApi::Init(ajn::BusAttachment& bus, PropertyStore& store) {
    m_BusAttachment = &bus;
    m_PropertyStore = &store;
}

void AboutServiceApi::DestroyInstance() {
    if (m_instance) {
        delete m_instance;
        m_instance = NULL;
    }
}
