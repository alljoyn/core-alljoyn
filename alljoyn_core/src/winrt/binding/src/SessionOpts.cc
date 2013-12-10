/******************************************************************************
 *
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
 *
 *****************************************************************************/

#include "SessionOpts.h"

#include <qcc/String.h>
#include <qcc/winrt/utility.h>
#include <alljoyn/Session.h>
#include <alljoyn/Status.h>
#include <ObjectReference.h>
#include <AllJoynException.h>

namespace AllJoyn {

SessionOpts::SessionOpts()
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create managed _SessionOpts
        _mSessionOpts = new qcc::ManagedObj<_SessionOpts>();
        // Check for allocation error
        if (NULL == _mSessionOpts) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _SessionOpts for convenience
        _sessionOpts = &(**_mSessionOpts);
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

SessionOpts::SessionOpts(TrafficType traffic, bool isMultipoint, ProximityType proximity, TransportMaskType transports)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create managed _SessionOpts
        ajn::SessionOpts::TrafficType trafficType = (ajn::SessionOpts::TrafficType)(int)traffic;
        ajn::SessionOpts::Proximity prox = (ajn::SessionOpts::Proximity)(int)proximity;
        ajn::TransportMask trans = (ajn::TransportMask)(int)transports;
        _mSessionOpts = new qcc::ManagedObj<_SessionOpts>(trafficType, isMultipoint, prox, trans);
        // Check for allocation error
        if (NULL == _mSessionOpts) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _SessionOpts for convenience
        _sessionOpts = &(**_mSessionOpts);
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

SessionOpts::SessionOpts(const ajn::SessionOpts* sessionOpts)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check sessionOpts for invalid values
        if (NULL == sessionOpts) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Create managed _SessionOpts
        _mSessionOpts = new qcc::ManagedObj<_SessionOpts>(sessionOpts->traffic,
                                                          sessionOpts->isMultipoint,
                                                          sessionOpts->proximity,
                                                          sessionOpts->transports);
        // Check for allocation error
        if (NULL == _mSessionOpts) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _SessionOpts for convenience
        _sessionOpts = &(**_mSessionOpts);
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

SessionOpts::SessionOpts(const qcc::ManagedObj<_SessionOpts>* sessionOpts)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check sessionOpts for invalid values
        if (NULL == sessionOpts) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Attach sessionOpts to managed _SessionOpts
        _mSessionOpts = new qcc::ManagedObj<_SessionOpts>(*sessionOpts);
        // Check for allocation error
        if (NULL == _mSessionOpts) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _SessionOpts for convenience
        _sessionOpts = &(**_mSessionOpts);
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

SessionOpts::~SessionOpts()
{
    // Delete managed _SessionOpts to adjust ref count
    if (NULL != _mSessionOpts) {
        delete _mSessionOpts;
        _mSessionOpts = NULL;
        _sessionOpts = NULL;
    }
}

TrafficType SessionOpts::Traffic::get()
{
    TrafficType result = (TrafficType)(int)-1;

    // Check if wrapped value already exists
    if ((TrafficType)(int)-1 == _sessionOpts->_eventsAndProperties->Traffic) {
        result = (TrafficType)(int)_sessionOpts->traffic;
        // Store the result
        _sessionOpts->_eventsAndProperties->Traffic = result;
    } else {
        // Return Traffic
        result = _sessionOpts->_eventsAndProperties->Traffic;
    }

    return result;
}

void SessionOpts::Traffic::set(TrafficType value)
{
    // Store value
    _sessionOpts->traffic = (ajn::SessionOpts::TrafficType)(int)value;
    _sessionOpts->_eventsAndProperties->Traffic = value;
}

bool SessionOpts::IsMultipoint::get()
{
    bool result = (bool)-1;

    // Check if wrapped value already exists
    if ((bool)-1 == _sessionOpts->_eventsAndProperties->IsMultipoint) {
        result = _sessionOpts->isMultipoint;
        // Store the result
        _sessionOpts->_eventsAndProperties->IsMultipoint = result;
    } else {
        // Return IsMultipoint
        result = _sessionOpts->_eventsAndProperties->IsMultipoint;
    }

    return result;
}

void SessionOpts::IsMultipoint::set(bool value)
{
    // Store value
    _sessionOpts->isMultipoint = value;
    _sessionOpts->_eventsAndProperties->IsMultipoint = value;
}

ProximityType SessionOpts::Proximity::get()
{
    ProximityType result = (ProximityType)(int)-1;

    // Check if wrapped value already exits
    if ((ProximityType)(int)-1 == _sessionOpts->_eventsAndProperties->Proximity) {
        result = (ProximityType)(int)_sessionOpts->proximity;
        // Store the result
        _sessionOpts->_eventsAndProperties->Proximity = result;
    } else {
        // Return Proximity
        result = _sessionOpts->_eventsAndProperties->Proximity;
    }

    return result;
}

void SessionOpts::Proximity::set(ProximityType value)
{
    // Store value
    _sessionOpts->proximity = (ajn::SessionOpts::Proximity)(int)value;
    _sessionOpts->_eventsAndProperties->Proximity = value;
}

TransportMaskType SessionOpts::TransportMask::get()
{
    TransportMaskType result = (TransportMaskType)(int)-1;

    // Check if wrapped value already exists
    if ((TransportMaskType)(int)-1 == _sessionOpts->_eventsAndProperties->TransportMask) {
        result = (TransportMaskType)(int)_sessionOpts->transports;
        // Store the result
        _sessionOpts->_eventsAndProperties->TransportMask = result;
    } else {
        // Return TransportMask
        result = _sessionOpts->_eventsAndProperties->TransportMask;
    }

    return result;
}

void SessionOpts::TransportMask::set(TransportMaskType value)
{
    // Store value
    _sessionOpts->transports = (ajn::TransportMask)(int)value;
    _sessionOpts->_eventsAndProperties->TransportMask = value;
}

_SessionOpts::_SessionOpts()
    : ajn::SessionOpts()
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create internal ref class
        _eventsAndProperties = ref new __SessionOpts();
        // Check for allocation error
        if (nullptr == _eventsAndProperties) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

_SessionOpts::_SessionOpts(ajn::SessionOpts::TrafficType traffic, bool isMultipoint, ajn::SessionOpts::Proximity proximity, ajn::TransportMask transports)
    : ajn::SessionOpts(traffic, isMultipoint, proximity, transports)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create internal ref class
        _eventsAndProperties = ref new __SessionOpts();
        // Check for allocation error
        if (nullptr == _eventsAndProperties) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

_SessionOpts::~_SessionOpts()
{
    _eventsAndProperties = nullptr;
}

__SessionOpts::__SessionOpts()
{
    Traffic = (TrafficType)(int)-1;
    IsMultipoint = (bool)-1;
    Proximity = (ProximityType)(int)-1;
    TransportMask = (TransportMaskType)(int)-1;
}

__SessionOpts::~__SessionOpts()
{
    Traffic = (TrafficType)(int)-1;
    IsMultipoint = (bool)-1;
    Proximity = (ProximityType)(int)-1;
    TransportMask = (TransportMaskType)(int)-1;
}

}
