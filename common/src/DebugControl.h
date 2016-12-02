/**
 * @file
 *
 * Debug control
 */

/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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

    bool DbgModulesSpecified() const;

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
