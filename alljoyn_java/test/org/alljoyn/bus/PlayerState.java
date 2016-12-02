/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 */

package org.alljoyn.bus;

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusSignal;

/**
 * PlayerState contains the state information of a single player for a fictitional
 * multi-player game. In this game, each player periodially and continuously
 * broadcasts his state to all the other players of the game using signals.
 */
@BusInterface
public interface PlayerState {

    /**
     * Player position information signal. (frequently sent)
     *
     * @param x         X position of character
     * @param y         Y position of character
     * @param rotation  360 rototation of character
     */
    @BusSignal(name="PlayerPosition", signature="uuu")
    public void playerPosition(int x,
                               int y,
                               int rotation) throws BusException;
}
