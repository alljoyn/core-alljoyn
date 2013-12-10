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
#ifndef _INTERFACEDESCRIPTIONNATIVE_H
#define _INTERFACEDESCRIPTIONNATIVE_H

#include "BusAttachment.h"
#include "NativeObject.h"

class InterfaceDescriptionNative : public NativeObject {
  public:
    static QStatus CreateInterface(Plugin& plugin, BusAttachment& busAttachment, InterfaceDescriptionNative* interfaceDescriptionNative);
    static InterfaceDescriptionNative* GetInterface(Plugin& plugin, BusAttachment& busAttachment, const qcc::String& name);

    InterfaceDescriptionNative(Plugin& plugin, NPObject* objectValue);
    InterfaceDescriptionNative(InterfaceDescriptionNative* other);
    virtual ~InterfaceDescriptionNative();
};

#endif // _INTERFACEDESCRIPTIONNATIVE_H
