/*
 * TransportBase.h
 *
 *  Created on: May 29, 2015
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
#ifndef TRANSPORTBASE_H_
#define TRANSPORTBASE_H_

#include "Endpoint.h"

#include <functional>
#include <memory>
#include <string>
#include <map>

#include <alljoyn/Status.h>

namespace nio {

class Proactor;
class Buffer;

class TransportBase {
  public:

    TransportBase(Proactor& proactor, const std::string& name);

    TransportBase(const TransportBase&) = delete;
    TransportBase& operator=(const TransportBase&) = delete;

    virtual ~TransportBase();

    typedef std::shared_ptr<Endpoint> EndpointPtr;


    virtual QStatus Send(Endpoint::Handle handle, MessageType msg, Endpoint::SendCompleteCB cb) = 0;

    // whoever is listening for ReadMessageCB must call NotifyMessageDoneCB when the app is finished with the message
    // must call this again after each CB, when the listener is ready to recv again

    // these should be called from EndpointBase!
    virtual QStatus Recv(Endpoint::Handle handle, MessageType msg, Endpoint::ReadMessageCB cb) = 0;
    //virtual QStatus UnregisterMessageRecvCB(EndpointHandle handle) = 0;


    typedef std::function<void (EndpointPtr, QStatus)> ConnectedCB;
    virtual QStatus Connect(const std::string& spec, ConnectedCB cb) = 0;
    virtual QStatus Disconnect(Endpoint::Handle handle, bool force = false) = 0;


    typedef std::function<bool (EndpointPtr)> AcceptedCB;
    virtual QStatus Listen(const std::string& spec, AcceptedCB cb) = 0;
    virtual QStatus StopListen(const std::string& spec) = 0;

    std::string GetName() const;

  protected:

    Proactor& proactor;
    const std::string name;
};



} /* namespace nio */

#endif /* TRANSPORTBASE_H_ */
