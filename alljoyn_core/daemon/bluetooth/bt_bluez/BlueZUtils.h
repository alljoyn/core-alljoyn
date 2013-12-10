/**
 * @file
 * Utility classes for the BlueZ implementation of BT transport.
 */

/******************************************************************************
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_BLUEZUTILS_H
#define _ALLJOYN_BLUEZUTILS_H

#include <qcc/platform.h>

#include <qcc/Event.h>
#include <qcc/ManagedObj.h>
#include <qcc/Socket.h>
#include <qcc/SocketStream.h>
#include <qcc/String.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/ProxyBusObject.h>

#include "BDAddress.h"
#include "BTTransport.h"

#include <alljoyn/Status.h>



namespace ajn {
namespace bluez {

typedef qcc::ManagedObj<std::vector<qcc::String> > AdvertisedNamesList;


class BTSocketStream : public qcc::SocketStream {
  public:
    BTSocketStream(qcc::SocketFd sock);
    ~BTSocketStream() { if (buffer) delete[] buffer; }
    QStatus PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout = qcc::Event::WAIT_FOREVER);
    QStatus PushBytes(const void* buf, size_t numBytes, size_t& numSent);

  private:
    uint8_t* buffer;
    size_t inMtu;
    size_t outMtu;
    size_t offset;
    size_t fill;
};


} // namespace bluez
} // namespace ajn


#endif
