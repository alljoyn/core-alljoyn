#ifndef _ALLJOYN_OBSINTERNAL_H
#define _ALLJOYN_OBSINTERNAL_H
/**
 * @file
 *
 * This file defines internal state for a BusAttachment
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

#ifndef __cplusplus
#error Only include CoreObserver.h in C++ code.
#endif

#include <set>
#include <map>

#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <qcc/ManagedObj.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/Observer.h>
#include <alljoyn/InterfaceDescription.h>

namespace ajn {

class CoreObserver {
  public:
    typedef std::set<qcc::String> InterfaceSet;
    InterfaceSet mandatory;

    CoreObserver(InterfaceSet mandatory) : mandatory(mandatory) { }

    virtual ~CoreObserver() { }

    /* interface towards ObserverManager */
    virtual void ObjectDiscovered(const ObjectId& oid, const InterfaceSet& interfaces, SessionId sessionid) = 0;
    virtual void ObjectLost(const ObjectId& oid) = 0;
    /**
     * Enable all disabled listeners for this observer.
     *
     * Called from the ObserverManager work queue to make sure the
     * initial callbacks of triggerOnExisting listeners are called
     * from the local endpoint dispatcher threads.
     */
    virtual void EnablePendingListeners() = 0;

};

}


#endif
