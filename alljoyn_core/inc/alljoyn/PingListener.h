/**
 * @file
 *
 * PingListener
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

#ifndef _ALLJOYN_PINGLISTENER_H_
#define _ALLJOYN_PINGLISTENER_H_

#ifndef __cplusplus
#error Only include PingListener.h in C++ code.
#endif

#include <qcc/String.h>
#include <qcc/Debug.h>

namespace ajn {

/**
 * PingListener base class
 */
class PingListener {
  public:
    /**
     * Destructor
     */
    virtual ~PingListener() { }

    /**
     * Destination Lost: called when the destination becomes unreachable.
     * Called once.
     *
     * @param  group Pinging group name
     * @param  destination Destination that was pinged
     */
    virtual void DestinationLost(const qcc::String& group, const qcc::String& destination) {
        QCC_UNUSED(group);
        QCC_UNUSED(destination);
    }

    /**
     * Destination Found: called when the destination becomes reachable.
     * Called once.
     *
     * @param  group Pinging group name
     * @param  destination Destination that was pinged
     */
    virtual void DestinationFound(const qcc::String& group, const qcc::String& destination) {
        QCC_UNUSED(group);
        QCC_UNUSED(destination);
    }
};
}
#endif /* PINGLISTENER_H_ */
