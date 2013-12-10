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
#ifndef _SOCKETFDINTERFACE_H
#define _SOCKETFDINTERFACE_H

#include "HttpServer.h"
#include "ScriptableObject.h"
#include <qcc/ManagedObj.h>

class _SocketFdInterface : public ScriptableObject {
  public:
    _SocketFdInterface(Plugin& plugin);
    virtual ~_SocketFdInterface();
    virtual bool Construct(const NPVariant* args, uint32_t argCount, NPVariant* result);

  private:
    HttpServer httpServer;

    bool createObjectURL(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool revokeObjectURL(const NPVariant* args, uint32_t argCount, NPVariant* result);
};

typedef qcc::ManagedObj<_SocketFdInterface> SocketFdInterface;

#endif // _SOCKETFDINTERFACE_H
