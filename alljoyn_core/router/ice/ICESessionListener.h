#ifndef _ICESESSIONLISTENER_H
#define _ICESESSIONLISTENER_H

/**
 * @file ICESessionListener.h
 *
 */

/******************************************************************************
 * Copyright (c) 2009,2012 AllSeen Alliance. All rights reserved.
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

#include "ICESession.h"

namespace ajn {

// Forward Declaration
class ICESession;

/**
 * ICESession registered listeners must implement this abstract class in order
 * to receive notifications from ICESession.
 */
class ICESessionListener {
  public:

    /**
     * Virtual Destructor
     */
    virtual ~ICESessionListener() { };

    /**
     * Notify listener that ICESession has changed state.
     * @param session    Session whose state has changed.
     */

    virtual void ICESessionChanged(ICESession* session) = 0;
};

} //namespace ajn

#endif
