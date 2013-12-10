/**
 * @file
 * AllJoyn-Daemon module property database classes
 */

/******************************************************************************
 * Copyright (c) 2010-2011, AllSeen Alliance. All rights reserved.
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
 ******************************************************************************/

#ifndef _PROPERTYDB_H
#define _PROPERTYDB_H

#include <qcc/platform.h>
#include <qcc/ManagedObj.h>
#include <qcc/String.h>
#include <qcc/StringMapKey.h>

namespace ajn {

class _PropertyMap;

class _PropertyDB {
  public:
    _PropertyDB() { }
    virtual ~_PropertyDB();

    void Set(qcc::String module, qcc::String name, qcc::String value);
    qcc::String Get(qcc::String module, qcc::String name);

  private:
    std::unordered_map<qcc::StringMapKey, _PropertyMap*> m_modules;
};

/**
 * Managed object wrapper for property database class.
 */
typedef qcc::ManagedObj<_PropertyDB> PropertyDB;


} // namespace ajn

#endif // _PROPERTYDB_H
