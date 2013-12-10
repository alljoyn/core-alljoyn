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
#ifndef _VERSIONINTERFACE_H
#define _VERSIONINTERFACE_H

#include "ScriptableObject.h"
#include <qcc/ManagedObj.h>

class _VersionInterface : public ScriptableObject {
  public:
    _VersionInterface(Plugin& plugin);
    virtual ~_VersionInterface();

  private:
    bool getBuildInfo(NPVariant* result);
    bool getNumericVersion(NPVariant* result);
    bool getArch(NPVariant* result);
    bool getApiLevel(NPVariant* result);
    bool getRelease(NPVariant* result);
    bool getVersion(NPVariant* result);
    //bool toString(const NPVariant* args, uint32_t npargCount, NPVariant* result);
};

typedef qcc::ManagedObj<_VersionInterface> VersionInterface;

#endif // _VERSIONINTERFACE_H
