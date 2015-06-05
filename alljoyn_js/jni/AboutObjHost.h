/*
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
 */

#ifndef _ABOUTOBJHOST_H
#define _ABOUTOBJHOST_H

#include "BusAttachment.h"
#include "ScriptableObject.h"
#include "AboutObj.h"
#include <qcc/GUID.h>

class _AboutObjHost : public ScriptableObject {
  public:
    _AboutObjHost(Plugin& plugin, BusAttachment& busAttachment);
    virtual ~_AboutObjHost();

  private:
    BusAttachment busAttachment;
    AboutObj aboutObj;
    ajn::AboutData* aboutData;

    bool announce(const NPVariant* args, uint32_t argCount, NPVariant* result);
};

typedef qcc::ManagedObj<_AboutObjHost> AboutObjHost;

#endif
