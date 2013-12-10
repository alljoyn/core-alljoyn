/*
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
 */
#ifndef _MESSAGEHOST_H
#define _MESSAGEHOST_H

#include "BusAttachment.h"
#include "ScriptableObject.h"
#include <alljoyn/Message.h>
#include <alljoyn/MsgArg.h>
#include <qcc/String.h>

class _MessageHost : public ScriptableObject {
  public:
    _MessageHost(Plugin& plugin, BusAttachment& busAttachment, ajn::Message& message);
    virtual ~_MessageHost();

  protected:
    BusAttachment busAttachment;
    ajn::Message message;

    bool getSender(NPVariant* npresult);
    bool getDestination(NPVariant* npresult);
    bool getFlags(NPVariant* npresult);
    bool getInterfaceName(NPVariant* npresult);
    bool getObjectPath(NPVariant* npresult);
    bool getAuthMechanism(NPVariant* npresult);
    bool getIsUnreliable(NPVariant* npresult);
    bool getMemberName(NPVariant* npresult);
    bool getSignature(NPVariant* npresult);
    bool getSessionId(NPVariant* npresult);
    bool getTimestamp(NPVariant* npresult);
};

typedef qcc::ManagedObj<_MessageHost> MessageHost;

#endif // _MESSAGEHOST_H
