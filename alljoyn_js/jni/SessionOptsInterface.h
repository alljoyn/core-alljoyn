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
#ifndef _SESSIONOPTSINTERFACE_H
#define _SESSIONOPTSINTERFACE_H

#include "ScriptableObject.h"
#include <qcc/ManagedObj.h>

class _SessionOptsInterface : public ScriptableObject {
  public:
    static std::map<qcc::String, int32_t>& Constants();
    _SessionOptsInterface(Plugin& plugin);
    virtual ~_SessionOptsInterface();

  private:
    static std::map<qcc::String, int32_t> constants;
};

typedef qcc::ManagedObj<_SessionOptsInterface> SessionOptsInterface;

#endif // _SESSIONOPTSINTERFACE_H
