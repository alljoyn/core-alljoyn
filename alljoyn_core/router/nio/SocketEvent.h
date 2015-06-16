/*
 * SocketEvent.h
 *
 *  Created on: Jun 8, 2015
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
#ifndef SOCKETEVENT_H_
#define SOCKETEVENT_H_

#include "EventBase.h"

#include <functional>

#include <qcc/Socket.h>

namespace nio {

class SocketEvent : public EventBase {
  public:

    typedef std::function<void (qcc::SocketFd, QStatus)> SocketCallback;

    SocketEvent(qcc::SocketFd fd, SocketCallback cb);

    virtual ~SocketEvent();

    SocketEvent(const SocketEvent&) = delete;
    SocketEvent& operator=(const SocketEvent&) = delete;

    qcc::SocketFd GetSocket() const;

    void SetStatus(QStatus status);

    virtual void ExecuteInternal() override;

  protected:

    const qcc::SocketFd fd;

    SocketCallback cb;

    QStatus status;
};

} /* namespace nio */

#endif /* SOCKETEVENT_H_ */
