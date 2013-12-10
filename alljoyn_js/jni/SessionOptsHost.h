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
#ifndef _SESSIONOPTSHOST_H
#define _SESSIONOPTSHOST_H

#include "ScriptableObject.h"
#include <alljoyn/Session.h>
#include <qcc/ManagedObj.h>

class _SessionOptsHost : public ScriptableObject {
  public:
    _SessionOptsHost(Plugin& plugin, const ajn::SessionOpts& opts);
    virtual ~_SessionOptsHost();

  private:
    const ajn::SessionOpts opts;

    bool getTraffic(NPVariant* result);
    bool getIsMultipoint(NPVariant* result);
    bool getProximity(NPVariant* result);
    bool getTransports(NPVariant* result);
};

typedef qcc::ManagedObj<_SessionOptsHost> SessionOptsHost;

#endif // _SESSIONOPTSHOST_H
