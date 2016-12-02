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