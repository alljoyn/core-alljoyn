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

//
// Even though we're not actually using a map, we have to include it (in
// Windows) so STLPort can figure out where std::nothrow_t comes from.
//
#include <map>

#include "PropertyDB.h"

namespace ajn {

class _PropertyMap {
  public:
    _PropertyMap() { }
    virtual ~_PropertyMap() { }

    void Set(qcc::String name, qcc::String value);
    qcc::String Get(qcc::String name);

  private:
    std::unordered_map<qcc::StringMapKey, qcc::String> m_properties;
};

void _PropertyMap::Set(qcc::String name, qcc::String value)
{
    m_properties[name] = value;
}

qcc::String _PropertyMap::Get(qcc::String name)
{
    std::unordered_map<qcc::StringMapKey, qcc::String>::const_iterator i = m_properties.find(name);
    if (i == m_properties.end()) {
        return "";
    }
    return i->second;
}

_PropertyDB::~_PropertyDB()
{
    for (std::unordered_map<qcc::StringMapKey, _PropertyMap*>::iterator i = m_modules.begin(); i != m_modules.end(); ++i) {
        delete i->second;
    }
}

void _PropertyDB::Set(qcc::String module, qcc::String name, qcc::String value)
{
    std::unordered_map<qcc::StringMapKey, _PropertyMap*>::const_iterator i = m_modules.find(module);
    if (i == m_modules.end()) {
        _PropertyMap* p = new _PropertyMap;
        p->Set(name, value);
        m_modules[module] = p;
        return;
    }
    i->second->Set(name, value);
}

qcc::String _PropertyDB::Get(qcc::String module, qcc::String name)
{
    std::unordered_map<qcc::StringMapKey, _PropertyMap*>::const_iterator i = m_modules.find(module);
    if (i == m_modules.end()) {
        return "";
    }
    return i->second->Get(name);
}

} // namespace ajn {
