/**
 * @file
 *
 * Bus is the top-level object responsible for implementing the message bus.
 */

/******************************************************************************
 * Copyright (c) 2010-2011,2014 AllSeen Alliance. All rights reserved.
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
#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/String.h>

#include <assert.h>

#include "Bus.h"
#include "DaemonRouter.h"
#include "TransportList.h"

#define QCC_MODULE "ALLJOYN_DAEMON"

using namespace ajn;
using namespace qcc;
using namespace std;

/*
 * Set the number of concurrent method and signal handlers on our local endpoint
 * to four.
 */
const uint32_t EP_CONCURRENCY = 4;

Bus::Bus(const char* applicationName, TransportFactoryContainer& factories, const char* listenSpecs) :
    BusAttachment(new Internal(applicationName, *this, factories, new DaemonRouter, true, listenSpecs, EP_CONCURRENCY), EP_CONCURRENCY)
{
    GetInternal().GetRouter().SetGlobalGUID(GetInternal().GetGlobalGUID());
}

Bus::~Bus()
{
    /* Make sure all listeners are unregistered */
    listenersLock.Lock(MUTEX_CONTEXT);
    set<ProtectedBusListener>::iterator it = busListeners.begin();
    while (it != busListeners.end()) {
        BusListener* listener = **it;
        listenersLock.Unlock(MUTEX_CONTEXT);
        UnregisterBusListener(*listener);
        listenersLock.Lock(MUTEX_CONTEXT);
        it = busListeners.begin();
    }
    listenersLock.Unlock(MUTEX_CONTEXT);
}

QStatus Bus::StartListen(const qcc::String& listenSpec, bool& listening)
{
    QStatus status;

    /** Get the transport */
    Transport* trans = GetInternal().GetTransportList().GetTransport(listenSpec);
    if (trans) {
        status = trans->StartListen(listenSpec.c_str());
        if (ER_OK == status) {
            if (trans->IsBusToBus()) {
                if (!externalAddrs.empty()) {
                    externalAddrs += ';';
                }
                externalAddrs += listenSpec + ",guid=" + GetInternal().GetGlobalGUID().ToString();
            } else {
                if (!localAddrs.empty()) {
                    localAddrs += ';';
                }
                localAddrs += listenSpec + ",guid=" + GetInternal().GetGlobalGUID().ToString();
            }
            listening = true;
        }
    } else {
        status = ER_BUS_TRANSPORT_NOT_AVAILABLE;
    }
    return status;
}

QStatus Bus::StartListen(const char* listenSpecs)
{
    QStatus status(ER_OK);
    qcc::String specs = listenSpecs;
    bool listening = false;

    if (!IsStarted()) {
        status = ER_BUS_BUS_NOT_STARTED;
    } else {
        size_t pos = 0;
        while (qcc::String::npos != pos) {
            size_t endPos = specs.find_first_of(';', pos);
            qcc::String spec((qcc::String::npos == endPos) ? specs.substr(pos) : specs.substr(pos, endPos - pos));
            QStatus s(StartListen(spec, listening));
            if (status == ER_OK) {
                status = s;
            }

            pos = ((qcc::String::npos == endPos) || (specs.size() <= endPos + 1)) ? qcc::String::npos : endPos + 1;
        }
        /*
         * BusAttachment needs to be listening on at least one transport
         */
        if (listening) {
            status = ER_OK;
        } else {
            status = ER_BUS_NO_TRANSPORTS;
        }
    }
    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment::StartListen failed"));
    }
    return status;
}

QStatus Bus::StopListen(const char* listenSpecs)
{
    QStatus status(ER_OK);
    qcc::String specs = listenSpecs;

    if (!IsStarted()) {
        status = ER_BUS_BUS_NOT_STARTED;
        QCC_LogError(status, ("BusAttachment::StopListen() failed"));
    } else {
        size_t pos = 0;
        while (qcc::String::npos != pos) {
            size_t endPos = specs.find_first_of(';', pos);
            qcc::String spec = (qcc::String::npos == endPos) ? specs.substr(pos) : specs.substr(pos, endPos - pos);

            QStatus s = ER_BUS_TRANSPORT_NOT_AVAILABLE;
            Transport* trans = GetInternal().GetTransportList().GetTransport(spec);
            if (trans) {
                s = trans->StopListen(spec.c_str());
                if (ER_OK != s) {
                    QCC_LogError(s, ("Transport::StopListen failed"));
                }
                // TODO - remove spec from localAddrs and externalAddrs???
            }

            if (status == ER_OK) {
                status = s;
            }

            pos = ((qcc::String::npos == endPos) || (specs.size() <= endPos + 1)) ? qcc::String::npos : endPos + 1;
        }
    }
    return status;
}

void Bus::RegisterBusListener(BusListener& listener)
{
    listenersLock.Lock(MUTEX_CONTEXT);
    BusListener* l = &listener;
    busListeners.insert(ProtectedBusListener(l));

    if (busListeners.size() == 1) {
        /*
         * The bus gets name changed callbacks from the daemon router.
         */
        reinterpret_cast<DaemonRouter&>(GetInternal().GetRouter()).AddBusNameListener(this);
    }
    listenersLock.Unlock(MUTEX_CONTEXT);
}

void Bus::UnregisterBusListener(BusListener& listener)
{
    /* Find listener and remove it*/
    listenersLock.Lock(MUTEX_CONTEXT);
    BusListener* pListener = &listener;
    set<ProtectedBusListener>::iterator it = busListeners.find(ProtectedBusListener(pListener));
    if (it != busListeners.end()) {
        ProtectedBusListener l  = *it;
        busListeners.erase(it);

        /* Unregister name listener for this attachment if no listeners are registered */
        if (busListeners.size() == 0) {
            reinterpret_cast<DaemonRouter&>(GetInternal().GetRouter()).RemoveBusNameListener(this);
        }

        /* Wait for any callbacks on listener to complete */
        while (l.GetRefCount() > 1) {
            listenersLock.Unlock(MUTEX_CONTEXT);
            qcc::Sleep(5);
            listenersLock.Lock(MUTEX_CONTEXT);
        }
    }
    listenersLock.Unlock(MUTEX_CONTEXT);
}

void Bus::NameOwnerChanged(const qcc::String& alias,
                           const qcc::String* oldOwner, SessionOpts::NameTransferType oldOwnerNameTransfer,
                           const qcc::String* newOwner, SessionOpts::NameTransferType newOwnerNameTransfer)
{
    listenersLock.Lock(MUTEX_CONTEXT);
    set<ProtectedBusListener>::iterator it = busListeners.begin();
    while (it != busListeners.end()) {
        ProtectedBusListener l = *it;
        listenersLock.Unlock(MUTEX_CONTEXT);
        (*l)->NameOwnerChanged(alias.c_str(), oldOwner ? oldOwner->c_str() : NULL, newOwner ? newOwner->c_str() : NULL);
        listenersLock.Lock(MUTEX_CONTEXT);
        it = busListeners.upper_bound(l);
    }
    listenersLock.Unlock(MUTEX_CONTEXT);
}

