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
#ifndef _ALLJOYN_ABOUTOBJECTDESCRIPTIONINTERNAL_H
#define _ALLJOYN_ABOUTOBJECTDESCRIPTIONINTERNAL_H

#include <alljoyn/AboutObjectDescription.h>
#include <qcc/Mutex.h>
#include <qcc/LockLevel.h>

#include <set>
#include <map>

namespace ajn {
/**
 * Class used to hold internal values for the AboutObjectDescription
 */
class AboutObjectDescription::Internal {
    friend class AboutObjectDescription;
  public:
    Internal() : announceObjectsMapLock(qcc::LOCK_LEVEL_ABOUTOBJECTDESCRIPTION_INTERNAL_ANNOUNCEOBJECTSMAPLOCK) { }

    AboutObjectDescription::Internal& operator=(const AboutObjectDescription::Internal& other) {
        announceObjectsMap = other.announceObjectsMap;
        return *this;
    }

  private:
    /**
     * Mutex that protects the announceObjectsMap
     *
     * this is marked as mutable so we can grab the lock to prevent the Objects
     * map being modified while its being read.
     */
    mutable qcc::Mutex announceObjectsMapLock;

    /**
     *  map that holds interfaces
     */
    std::map<qcc::String, std::set<qcc::String> > announceObjectsMap;
};
}
#endif //_ALLJOYN_ABOUTOBJECTDESCRIPTIONINTERNAL_H