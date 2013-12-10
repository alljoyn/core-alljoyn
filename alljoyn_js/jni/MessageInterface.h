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
#ifndef _MESSAGEINTERFACE_H
#define _MESSAGEINTERFACE_H

#include "ScriptableObject.h"
#include <qcc/ManagedObj.h>

class _MessageInterface : public ScriptableObject {
  public:
    static std::map<qcc::String, int32_t>& Constants();
    _MessageInterface(Plugin& plugin);
    virtual ~_MessageInterface();

  private:
    static std::map<qcc::String, int32_t> constants;
};

typedef qcc::ManagedObj<_MessageInterface> MessageInterface;

#endif // _MESSAGEINTERFACE_H
