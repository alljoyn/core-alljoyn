/**
 * @file
 *
 * Debug control
 */

/******************************************************************************
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
 ******************************************************************************/
#ifndef _QCC_DEBUGCONTROL_H
#define _QCC_DEBUGCONTROL_H

#include <qcc/Mutex.h>
#include <map>

namespace qcc {

class DebugControl {
  public:

    static void Init();
    static void Shutdown();

    DebugControl();

    void AddTagLevelPair(const char* tag, uint32_t level);

    void SetAllLevel(uint32_t level);

    void WriteDebugMessage(DbgMsgType type, const char* module, const qcc::String msg);

    void Register(QCC_DbgMsgCallback cb, void* context);

    bool Check(DbgMsgType type, const char* module);

    bool PrintThread() const;

  private:
    Mutex mutex;
    QCC_DbgMsgCallback cb;
    void* context;
    uint32_t allLevel;
    std::map<const qcc::String, uint32_t> modLevels;
    bool printThread;
};

}

#endif

