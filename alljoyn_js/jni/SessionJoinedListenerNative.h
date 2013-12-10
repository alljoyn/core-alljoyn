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
#ifndef _SESSIONJOINEDLISTENERNATIVE_H
#define _SESSIONJOINEDLISTENERNATIVE_H

#include "NativeObject.h"
#include <alljoyn/SessionPortListener.h>
#include <qcc/String.h>

class SessionJoinedListenerNative : public NativeObject {
  public:
    SessionJoinedListenerNative(Plugin& plugin, NPObject* objectValue);
    virtual ~SessionJoinedListenerNative();

    void onJoined(ajn::SessionPort port, ajn::SessionId id, const qcc::String& joiner);
};

#endif // _SESSIONJOINEDLISTENERNATIVE_H
