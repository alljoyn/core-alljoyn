/*
 * Copyright (c) 2011-2013, AllSeen Alliance. All rights reserved.
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
#ifndef _BUSOBJECTNATIVE_H
#define _BUSOBJECTNATIVE_H

#include "MessageReplyHost.h"
#include "NativeObject.h"
#include <alljoyn/Status.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/MsgArg.h>

class BusObjectNative : public NativeObject {
  public:
    BusObjectNative(Plugin& plugin, NPObject* objectValue);
    virtual ~BusObjectNative();

    void onRegistered();
    void onUnregistered();
    QStatus get(const ajn::InterfaceDescription* interface, const ajn::InterfaceDescription::Property* property, ajn::MsgArg& val);
    QStatus set(const ajn::InterfaceDescription* interface, const ajn::InterfaceDescription::Property* property, const ajn::MsgArg& val);
    QStatus toXML(bool deep, size_t indent, qcc::String& xml);
    void onMessage(const char* interfaceName, const char* methodName, MessageReplyHost& message, const ajn::MsgArg* args, size_t numArgs);
};

#endif // _BUSOBJECTNATIVE_H
