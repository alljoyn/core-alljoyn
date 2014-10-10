/**
 * Copyright (c) 2009-2011, 2014, AllSeen Alliance. All rights reserved.
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
 */

package org.alljoyn.bus;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.SignalEmitter;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.annotation.BusSignalHandler;
import org.alljoyn.bus.ifaces.DBusProxyObj;

import junit.framework.TestCase;

public class GameTest extends TestCase {
    static {
        System.loadLibrary("alljoyn_java");
    }

    public GameTest(String name) {
        super(name);
    }

    public class Game implements PlayerState, BusObject {

        private PlayerState playerState;

        public void register(BusAttachment bus) throws Exception {

            /* Register the service */
            Status status = bus.registerBusObject(this, "/game/player");
            if (Status.OK != status) {
                throw new GameException("BusAttachment.registerBusObject() failed: " + status.toString());
            }

            /* Request a well-known name */
            DBusProxyObj control = bus.getDBusProxyObj();
            DBusProxyObj.RequestNameResult res = control.RequestName("org.alljoyn.bus.game",
                                                                    DBusProxyObj.REQUEST_NAME_NO_FLAGS);
            if (res != DBusProxyObj.RequestNameResult.PrimaryOwner) {
                throw new GameException("Failed to obtain well-known name");
            }

            /* Create a signal emitter so we can send out our position */
            SignalEmitter emitter = new SignalEmitter(this);
            playerState = emitter.getInterface(PlayerState.class);
        }

        public void playerPosition(int x, int y, int rotation) throws BusException {
            playerState.playerPosition(x, y, rotation);
        }
    }

    private int x, y, rotation;

    private boolean signalled;

    @BusSignalHandler(iface="org.alljoyn.bus.PlayerState", signal="PlayerPosition")
    public void playerPosition(int x, int y, int rotation) {
        this.x = x;
        this.y = y;
        this.rotation = rotation;
        synchronized (this) {
            signalled = true;
            notify();
        }
    }

    public void testGame() throws Exception {

        /* Create a bus connection and connect to the bus */
        BusAttachment bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        if (Status.OK != status) {
            throw new GameException("BusAttachment.connect() failed with " + status.toString());
        }

        /* Register the service */
        Game game = new Game();
        game.register(bus);

        /* Register a signal handler to receive other players' state */
        status = bus.registerSignalHandlers(this);
        if (Status.OK != status) {
            throw new GameException("Cannot register signal handler");
        }

        /* Add rule to receive non-session based signals */
        status = bus.addMatch("type='signal',interface='org.alljoyn.bus.PlayerState',member='PlayerPosition'");
        if (Status.OK != status) {
            throw new GameException("Cannot add rule to receive signals");
        }

        /* Periodically broadcast this player's state information */
        synchronized (this) {
            while (!signalled) {
                game.playerPosition(100, 200, 180);
                wait(50);
            }
        }

        assertEquals(100, x);
        assertEquals(200, y);
        assertEquals(180, rotation);
        /* Remove rule to receive non-session based signals */
        status = bus.removeMatch("type='signal',interface='org.alljoyn.bus.PlayerState',member='PlayerPosition'");
        if (Status.OK != status) {
            throw new GameException("Cannot remove rule");
        }
    }
}
