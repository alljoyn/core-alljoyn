/*
 * TransportManager.h
 *
 *  Created on: Jun 3, 2015
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
#ifndef TRANSPORTMANAGER_H_
#define TRANSPORTMANAGER_H_

#include "TransportBase.h"
#include "Proactor.h"

#include <map>
#include <mutex>
#include <string>


namespace nio {

/**
 * A class used to manage the various endpoints in our system.
 */
class TransportManager {
  public:
    TransportManager(Proactor& proactor);

    TransportManager(const TransportManager&) = delete;
    TransportManager& operator=(const TransportManager&) = delete;

    virtual ~TransportManager();

    /**
     * Start listening on a transport spec and get a callback when somebody tries to join.
     *
     * @param spec  A transport-specific representation of how to listen
     * @param cb    A callback that will be invoked when somebody tries to join.
     *              cb may reject the joiner by returning false.
     *
     * @return      ER_OK if successful, error otherwise
     */
    QStatus Listen(const std::string& spec, TransportBase::AcceptedCB cb);

    /**
     * Stop listening on the given spec
     *
     * @param spec  A transport-specific listen spec, previously passed to Listen
     *
     * @return      ER_OK if successful, error otherwise
     */
    QStatus StopListen(const std::string& spec);

    /**
     * Initiate a connection to a remote endpoint.
     *
     * @param spec  A transport-specific representation of the remote endpoint.
     * @param cb    A callback that will be invoked with the new endpoint on success.
     *
     * @return      ER_OK if successful, error otherwise
     */
    QStatus Connect(const std::string& spec, TransportBase::ConnectedCB cb);

  private:

    TransportBase* GetTransport(const std::string& spec);
    TransportBase* CreateTransport(const std::string& name);

    Proactor& proactor;

    std::map<std::string, TransportBase*> transports;
    std::mutex transports_lock;
};

} /* namespace nio */

#endif /* TRANSPORTMANAGER_H_ */
