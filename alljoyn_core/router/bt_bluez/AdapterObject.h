/**
 * @file
 * AdapterObject managed object class definition.  BT HCI device access class.
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
#ifndef _ALLJOYN_ADAPTEROBJECT_H
#define _ALLJOYN_ADAPTEROBJECT_H

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

class _AdapterObject : public ProxyBusObject,
    private ProxyBusObject::PropertiesChangedListener {
  public:

    _AdapterObject() { }
    _AdapterObject(BusAttachment& bus, const qcc::String& path);
    bool operator==(const _AdapterObject& other) const { return address == other.address; }

    QStatus SetAddress(const qcc::String addrStr) { return address.FromString(addrStr); }
    const qcc::BDAddress& GetAddress() const { return address; }
    bool IsDiscovering() const { return discovering; }
    void SetDiscovering(bool disc) { discovering = disc; }
    bool IsPowered() const { return powered; }
    void SetPowered(bool p) { powered = p; }

  private:

    void PropertiesChanged(ProxyBusObject& obj,
                           const char* ifaceName,
                           const MsgArg& changed,
                           const MsgArg& invalidated,
                           void* context);


    uint16_t id;
    qcc::BDAddress address;
    bool discovering;
    bool powered;
};

typedef qcc::ManagedObj<_AdapterObject> AdapterObject;


} // namespace bluez
} // namespace ajn

#endif
