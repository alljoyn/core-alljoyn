/**
 * @file
 * The AllJoyn P2P Name Service Implementation
 */

/******************************************************************************
 * Copyright (c) 2012, 2014, AllSeen Alliance. All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#include <vector>

#include <qcc/Debug.h>
#include <qcc/Event.h>
#include <qcc/time.h>

#include "P2PNameServiceImpl.h"

#define QCC_MODULE "P2PNS"

namespace ajn {

P2PNameServiceImpl::P2PNameServiceImpl()
    : m_state(IMPL_SHUTDOWN), m_enabled(false), m_callback(0), m_myP2pHelperListener(0), m_p2pHelperInterface(0), m_bus(0)
{
    QCC_DbgPrintf(("P2PNameServiceImpl::P2PNameServiceImpl()"));
}

P2PNameServiceImpl::~P2PNameServiceImpl()
{
    QCC_DbgPrintf(("P2PNameServiceImpl::~P2PNameServiceImpl()"));

    //
    // Delete any callbacks that a user of this class may have set.
    //
    delete m_callback;
    m_callback = NULL;

    //
    // All shut down and ready for bed.
    //
    m_state = IMPL_SHUTDOWN;
}

QStatus P2PNameServiceImpl::Init(BusAttachment* bus, const qcc::String& guid)
{
    QCC_DbgPrintf(("P2PNameServiceImpl::Init()"));

    //
    // Can only call Init() if the object is not running or in the process
    // of initializing
    //
    if (m_state != IMPL_SHUTDOWN) {
        return ER_FAIL;
    }

    m_state = IMPL_INITIALIZING;
    m_bus = bus;
    m_guid = guid;

    return ER_OK;
}

QStatus P2PNameServiceImpl::Start()
{
    m_state = IMPL_RUNNING;
    return ER_OK;
}

QStatus P2PNameServiceImpl::Stop()
{
    m_state = IMPL_STOPPING;
    return ER_OK;
}

QStatus P2PNameServiceImpl::Join()
{
    m_state = IMPL_SHUTDOWN;

    //
    // Delete our instace of the P2P Helper we used to communicate with the P2P
    // Helper Service, and the listener object that plumbed events back from the
    // helper to us.  It is too late to delete these at static destructor time
    // since it untimately involves wandering around in AllJoyn tables which are
    // all protected by mutexes.  If we happen to wander in after one of those
    // is destroyed, we will assert.  So we have got to delete the interface
    // before AllJoyn objects are destroyed.  Join() is a convenient time to
    // do this.
    //
    delete m_p2pHelperInterface;
    m_p2pHelperInterface = NULL;
    delete m_myP2pHelperListener;
    m_myP2pHelperListener = NULL;

    return ER_OK;
}

QStatus P2PNameServiceImpl::Enable(TransportMask transportMask)
{
    QCC_DbgPrintf(("P2PNameServiceImpl::Enable()"));

    if (m_state != IMPL_RUNNING) {
        QCC_DbgPrintf(("P2PNameServiceImpl::Enable(): Not IMPL_RUNNING"));
        return ER_FAIL;
    }

    //
    // We can't start the P2PHelperInterface at a time arbitrarily early in
    // the construction of the AllJoyn daemon.  We need a functioning system
    // in order to do the AddMatch RPC call to the DBus object that we will
    // need in order to receive signals.  We need to wait until we know that
    // the system is up.  A convenient place to do this is here in Enable,
    // since the only way we can get an enable is if we actually have an
    // advertisement or discovery operation coming in from a client.  At
    // that point we must have a functioning bus or we'd never have gotten
    // the precipitating operation.
    //
    if (m_p2pHelperInterface == NULL) {
        QCC_DbgPrintf(("P2PNameServiceImpl::Init(): new P2PHelperInterface"));
        m_p2pHelperInterface = new P2PHelperInterface();
        m_p2pHelperInterface->Init(m_bus);

        assert(m_myP2pHelperListener == NULL && "P2PNameServiceImpl::Init(): m_pyP2pHelperListener must be NULL");
        QCC_DbgPrintf(("P2PNameServiceImpl::Init(): new P2PHelperListener"));
        m_myP2pHelperListener = new MyP2PHelperListener(this);
        m_p2pHelperInterface->SetListener(m_myP2pHelperListener);
    }

    m_enabled = true;
    return ER_OK;
}

QStatus P2PNameServiceImpl::Disable(TransportMask transportMask)
{
    QCC_DbgPrintf(("P2PNameServiceImpl::Enable()"));

    if (m_state != IMPL_RUNNING) {
        QCC_DbgPrintf(("P2PNameServiceImpl::Disable(): Not IMPL_RUNNING"));
        return ER_FAIL;
    }

    m_enabled = false;
    return ER_OK;
}

QStatus P2PNameServiceImpl::Enabled(TransportMask transportMask, bool& enabled)
{
    QCC_DbgPrintf(("P2PNameServiceImpl::Enabled()"));

    if (m_state != IMPL_RUNNING) {
        QCC_DbgPrintf(("P2PNameServiceImpl::Enabled(): Not IMPL_RUNNING"));
        return ER_FAIL;
    }

    enabled = m_enabled;
    return ER_OK;
}

QStatus P2PNameServiceImpl::FindAdvertisedName(TransportMask transportMask, const qcc::String& wkn)
{
    QCC_DbgHLPrintf(("P2PNameServiceImpl::FindAdvertisedName(): \"%s\"", wkn.c_str()));

    if (m_state != IMPL_RUNNING) {
        QCC_DbgPrintf(("P2PNameServiceImpl::FindAdvertisedName(): Not IMPL_RUNNING"));
        return ER_FAIL;
    }

    assert(m_p2pHelperInterface && "P2PNameServiceImpl::FindAdvertisedName(): No m_p2pHelperInterface");
    return m_p2pHelperInterface->FindAdvertisedNameAsync(wkn);
}

QStatus P2PNameServiceImpl::CancelFindAdvertisedName(TransportMask transportMask, const qcc::String& wkn)
{
    QCC_DbgHLPrintf(("P2PNameServiceImpl::CancelFindAdvertisedName(): \"%s\"", wkn.c_str()));

    if (m_state != IMPL_RUNNING) {
        QCC_DbgPrintf(("P2PNameServiceImpl::CancelFindAdvertisedName(): Not IMPL_RUNNING"));
        return ER_FAIL;
    }

    assert(m_p2pHelperInterface && "P2PNameServiceImpl::CancelAdvertisedName(): No m_p2pHelperInterface");
    return m_p2pHelperInterface->CancelFindAdvertisedNameAsync(wkn);
}

QStatus P2PNameServiceImpl::AdvertiseName(TransportMask transportMask, const qcc::String& wkn)
{
    QCC_DbgHLPrintf(("P2PNameServiceImpl::AdvertiseName(): %s", wkn.c_str()));

    if (m_state != IMPL_RUNNING) {
        QCC_DbgPrintf(("P2PNameServiceImpl::AdvertiseName(): Not IMPL_RUNNING"));
        return ER_FAIL;
    }

    assert(m_p2pHelperInterface && "P2PNameServiceImpl::AdvertiseName(): No m_p2pHelperInterface");
    return m_p2pHelperInterface->AdvertiseNameAsync(wkn, m_guid);
}

QStatus P2PNameServiceImpl::CancelAdvertiseName(TransportMask transportMask, const qcc::String& wkn)
{
    QCC_DbgPrintf(("P2PNameServiceImpl::CancelAdvertiseName(): %s", wkn.c_str()));

    if (m_state != IMPL_RUNNING) {
        QCC_DbgPrintf(("P2PNameServiceImpl::CancelAdvertiseName(): Not IMPL_RUNNING"));
        return ER_FAIL;
    }

    assert(m_p2pHelperInterface && "P2PNameServiceImpl::CancelAdvertiseName(): No m_p2pHelperInterface");
    return m_p2pHelperInterface->CancelAdvertiseNameAsync(wkn, m_guid);
}

void P2PNameServiceImpl::SetCallback(TransportMask transportMask,
                                     Callback<void, const qcc::String&, qcc::String&, uint8_t>* cb)
{
    QCC_DbgPrintf(("P2PNameServiceImpl::SetCallback()"));

    Callback<void, const qcc::String&, qcc::String&, uint8_t>* goner = m_callback;
    m_callback = NULL;
    delete goner;
    m_callback = cb;
}

QStatus P2PNameServiceImpl::GetDeviceForGuid(const qcc::String& guid, qcc::String& device)
{
    QCC_DbgPrintf(("P2PNameServiceImpl::GetDeviceForGuid(): GUID \"%s\"", guid.c_str()));
    std::map<qcc::StringMapKey, qcc::String>::iterator i = m_devices.find(guid);
    if (i != m_devices.end()) {
        device = i->second;
        QCC_DbgPrintf(("P2PNameServiceImpl::GetDeviceForGuid(): Device is \"%s\"", device.c_str()));
        return ER_OK;
    } else {
        device = "";
        QCC_DbgPrintf(("P2PNameServiceImpl::GetDeviceForGuid(): No such GUID"));
        return ER_NO_SUCH_DEVICE;
    }
}

void P2PNameServiceImpl::OnFoundAdvertisedName(qcc::String& name, qcc::String& namePrefix, qcc::String& guid, qcc::String& device)
{
    QCC_DbgPrintf(("P2PNameServiceImpl::OnFoundAdvertisedName()"));

    QCC_DbgPrintf(("P2PNameServiceImpl::OnFoundAdvertisedName(): Remembering device \"%s\" has daemon of GUID \"%s\"",
                   device.c_str(), guid.c_str()));

    m_devices[guid] = device;

    if (m_callback == 0) {
        QCC_DbgPrintf(("P2PNameServiceImpl::OnFoundAdvertisedName(): No callback"));
    } else {
        const int8_t TIMER_FOREVER = 255;
        (*m_callback)(guid, name, TIMER_FOREVER);
    }
}

void P2PNameServiceImpl::OnLostAdvertisedName(qcc::String& name, qcc::String& namePrefix, qcc::String& guid, qcc::String& device)
{
    QCC_DbgPrintf(("P2PNameServiceImpl::OnLostAdvertisedName()"));

    QCC_DbgPrintf(("P2PNameServiceImpl::OnLostAdvertisedName(): Device \"%s\" lost.  Daemon of GUID \"%s\" is gone",
                   device.c_str(), guid.c_str()));


    std::map<qcc::StringMapKey, qcc::String>::iterator i = m_devices.find(guid);
    if (i != m_devices.end()) {
        m_devices.erase(i);
    }

    if (m_callback == 0) {
        QCC_DbgPrintf(("P2PNameServiceImpl::OnLostAdvertisedName(): No callback"));
    } else {
        const int8_t TIMER_LOST = 0;
        (*m_callback)(guid, name, TIMER_LOST);
    }
}

void P2PNameServiceImpl::HandleFindAdvertisedNameReply(int32_t result)
{
    QCC_DbgPrintf(("P2PNameServiceImpl::HandleFindAdvertisedNameReply(%d.)", result));

    if (result != P2PHelperInterface::P2P_OK) {
        QCC_LogError(static_cast<QStatus>(-result), ("P2PHelperInterface::HandleFindAdvertisedNameReply(): Reply is %d", result));
    }
}

void P2PNameServiceImpl::HandleCancelFindAdvertisedNameReply(int32_t result)
{
    QCC_DbgPrintf(("P2PNameServiceImpl::HandleCancelFindAdvertisedNameReply(%d.)", result));

    if (result != P2PHelperInterface::P2P_OK) {
        QCC_LogError(static_cast<QStatus>(-result), ("P2PHelperInterface::HandleCancelFindAdvertisedNameReply(): Reply is %d", result));
    }
}

void P2PNameServiceImpl::HandleAdvertiseNameReply(int32_t result)
{
    QCC_DbgPrintf(("P2PNameServiceImpl::HandleAdvertiseNameReply(%d.)", result));

    if (result != P2PHelperInterface::P2P_OK) {
        QCC_LogError(static_cast<QStatus>(-result), ("P2PHelperInterface::HandleAdvertiseNameReply(): Reply is %d", result));
    }
}

void P2PNameServiceImpl::HandleCancelAdvertiseNameReply(int32_t result)
{
    QCC_DbgPrintf(("P2PNameServiceImpl::HandleCancelAdvertiseNameReply(%d.)", result));

    if (result != P2PHelperInterface::P2P_OK) {
        QCC_LogError(static_cast<QStatus>(-result), ("P2PHelperInterface::HandleCancelAdvertiseNameReply(): Reply is %d", result));
    }
}

} // namespace ajn
