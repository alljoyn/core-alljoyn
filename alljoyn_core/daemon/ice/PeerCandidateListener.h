#ifndef _PEERCANDIDATELISTENER_H
#define _PEERCANDIDATELISTENER_H

/**
 * @file PeerCandidateListener.h
 *
 */

/******************************************************************************
 * Copyright (c) 2012 AllSeen Alliance. All rights reserved.
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

#include "RendezvousServerInterface.h"

namespace ajn {

/**
 * DiscoveryManager registered listeners must implement this abstract class in order
 * to receive notifications from DiscoveryManager when Peer candidates are available.
 */
class PeerCandidateListener {
  public:

    /**
     * Virtual Destructor
     */
    virtual ~PeerCandidateListener() { };

    /**
     * Notify listener that Peer candidates are available
     */
    virtual void SetPeerCandiates(list<ICECandidates>& candidates, const String& frag, const String& pwd) = 0;
};

} //namespace ajn

#endif
