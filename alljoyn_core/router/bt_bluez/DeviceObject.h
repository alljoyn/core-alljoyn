/**
 * @file
 * DeviceObject managed object class definition.
 */

/******************************************************************************
 * Copyright (c) 2009-2011, 2014, AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_DEVICEOBJECT_H
#define _ALLJOYN_DEVICEOBJECT_H

#include <qcc/platform.h>

#include <vector>

#include <qcc/ManagedObj.h>
#include <qcc/Socket.h>
#include <qcc/BDAddress.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/ProxyBusObject.h>

#include "BTTransportConsts.h"

#include <alljoyn/Status.h>



namespace ajn {
namespace bluez {

class _DeviceObject : public ProxyBusObject,
    private ProxyBusObject::PropertiesChangedListener {
  public:

    _DeviceObject() { }
    _DeviceObject(BusAttachment& bus, const qcc::String& path);
    bool operator==(const _DeviceObject& other) const { return m_address == other.m_address; }

    QStatus SetAddress(const qcc::String addrStr) { return m_address.FromString(addrStr); }
    void SetConnected(bool connected) { m_connected = connected; }
    const qcc::BDAddress& GetAddress() const { return m_address; }
    bool IsConnected() const { return m_connected; }

  private:

    void PropertiesChanged(ProxyBusObject& obj,
                           const char* ifaceName,
                           const MsgArg& changed,
                           const MsgArg& invalidated,
                           void* context);
    qcc::BDAddress m_address;
    bool m_connected;
};

typedef qcc::ManagedObj<_DeviceObject> DeviceObject;


} // namespace bluez
} // namespace ajn

#endif
