/*
 * EndpointBase.h
 *
 *  Created on: Jun 2, 2015
 *      Author: erongo
 */
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

#ifndef ENDPOINTBASE_H_
#define ENDPOINTBASE_H_

#include "Buffer.h"

#include <functional>
#include <string>
#include <memory>
#include <stdint.h>

#include <alljoyn/Status.h>

namespace nio {

typedef std::shared_ptr<Buffer> MessageType;

class TransportBase;

class Endpoint {

  public:

    typedef uint64_t Handle;
    const static Handle INVALID_HANDLE;

    Endpoint(TransportBase& transport, Handle handle, const std::string& spec);

    Endpoint(const Endpoint&) = delete;
    Endpoint& operator=(const Endpoint&) = delete;

    virtual ~Endpoint();

    std::string ToString() const;

    typedef std::function<void (std::shared_ptr<Endpoint>, MessageType, QStatus)> SendCompleteCB;

    /**
     * Send a message to this endpoint
     *
     * @param msg   The message to send
     * @param cb    The callback to make when the entire message has been sent
     *
     * @return      The error code.  If not ER_OK, the callback WILL NEVER HAPPEN.
     *
     * Do not call Send() again until the callback has happened.
     */
    QStatus Send(MessageType msg, SendCompleteCB cb);

    typedef std::function<void (std::shared_ptr<Endpoint>, MessageType, QStatus)> ReadMessageCB;

    /**
     * Receive a message from this endpoint
     *
     * @param msg   The message to receive
     * @param cb    The callback to make when the entire message has been received
     *
     * @return      The error code.  If not ER_OK, the callback WILL NEVER HAPPEN.
     *
     * Once the callback has been made, you must call Recv again to receive another.
     * Not calling Recv again will apply backpressure.
     */
    QStatus Recv(MessageType msg, ReadMessageCB cb);

    /*
     * Stop listening for incoming data from this endpoint.
     */
    //QStatus UnregisterMessageRecvCB();

    void Disconnect(bool force = false);

  protected:
    TransportBase& transport;
    Handle handle;
    const std::string spec;
};

} /* namespace nio */

#endif /* ENDPOINTBASE_H_ */
