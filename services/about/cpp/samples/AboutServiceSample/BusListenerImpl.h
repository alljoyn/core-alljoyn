/******************************************************************************
 * Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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

#ifndef BUSLISTENERIMPL_H_
#define BUSLISTENERIMPL_H_

#include <alljoyn/BusListener.h>
#include <alljoyn/SessionPortListener.h>

class BusListenerImpl : public ajn::BusListener, public ajn::SessionPortListener {

  public:

    /**
     * Constructor of CommonBusListener
     */
    BusListenerImpl();

    /**
     * Constructor of CommonBusListener
     * @param sessionPort - port of listener
     */
    BusListenerImpl(ajn::SessionPort sessionPort);

    /**
     * Destructor of CommonBusListener
     */
    ~BusListenerImpl();

    /**
     * AcceptSessionJoiner - Receive request to join session and decide whether to accept it or not
     * @param sessionPort - the port of the request
     * @param joiner - the name of the joiner
     * @param opts - the session options
     * @return true/false
     */
    bool AcceptSessionJoiner(ajn::SessionPort sessionPort, const char* joiner, const ajn::SessionOpts& opts);

    /**
     * Set the Value of the SessionPort associated with this SessionPortListener
     * @param sessionPort
     */
    void setSessionPort(ajn::SessionPort sessionPort);

    /**
     * Get the SessionPort of the listener
     * @return
     */
    ajn::SessionPort getSessionPort();

  private:

    /**
     * The port used as part of the join session request
     */
    ajn::SessionPort m_SessionPort;
};

#endif /* BUSLISTENERIMPL_H_ */
