/*
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef _MESSAGEREPLYHOST_H
#define _MESSAGEREPLYHOST_H

#include "BusAttachment.h"
#include "BusObject.h"
#include "MessageHost.h"
#include "ScriptableObject.h"
#include <alljoyn/Message.h>
#include <alljoyn/MsgArg.h>
#include <qcc/String.h>

class _MessageReplyHost : public _MessageHost {
  public:
    _MessageReplyHost(Plugin& plugin, BusAttachment& busAttachment, BusObject& busObject, ajn::Message& message, qcc::String replySignature);
    virtual ~_MessageReplyHost();

  private:
    BusObject busObject;
    qcc::String replySignature;

    bool reply(const NPVariant* npargs, uint32_t npargCount, NPVariant* npresult);
    bool replyError(const NPVariant* npargs, uint32_t npargCount, NPVariant* npresult);
};

typedef qcc::ManagedObj<_MessageReplyHost> MessageReplyHost;

#endif // _MESSAGEREPLYHOST_H