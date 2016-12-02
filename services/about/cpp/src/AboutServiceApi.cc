/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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