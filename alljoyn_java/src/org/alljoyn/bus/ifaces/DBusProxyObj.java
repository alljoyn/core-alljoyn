/*
 * Copyright (c) 2009-2011,2014, AllSeen Alliance. All rights reserved.
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

package org.alljoyn.bus.ifaces;

import java.util.Map;

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;
import org.alljoyn.bus.annotation.BusSignal;

/**
 * The standard org.freedesktop.DBus interface that is implemented by the local
 * AllJoyn router.  It is used to control bus operations such as obtaining
 * well-known names and establishing routing rules.
 * <p>
 * The functionality of DBusProxyObj is limited to functions that are available
 * in the DBus protocols. To use advanced AllJoyn features such as Peer-to-peer
 * connections, use AllJoynProxyObj.
 */
@BusInterface(name = "org.freedesktop.DBus")
public interface DBusProxyObj {

    @BusMethod(replySignature = "s")
    String Hello() throws BusException;

    /**
     * If there is no owner of this name then take ownership.  Otherwise this
     * request is queued until the current owner releases this name.
     */
    int REQUEST_NAME_NO_FLAGS = 0x00;

    /**
     * Allow other bus connections to "take" this name away from this
     * BusAttachment.
     */
    int REQUEST_NAME_ALLOW_REPLACEMENT = 0x01;

    /**
     * Attempt to take the name away from another bus connection if it is
     * already owned.
     */
    int REQUEST_NAME_REPLACE_EXISTING = 0x02;

    /**
     * If the name is already taken, don't wait in a queue for the name to
     * become available.
     */
    int REQUEST_NAME_DO_NOT_QUEUE = 0x04;

    /** {@link #RequestName(String, int)} return value. */
    enum RequestNameResult {

        /** Invalid. */
        Invalid,

        /** Bus name is now owned by this BusAttachment. */
        PrimaryOwner,

        /**
         * The name is already owned by another bus connection that allows
         * queing.  The request is queued.
         */
        InQueue,

        /**
         * The name is already owned by another bus connection. The request has
         * failed.
         */
        AlreadyExists,

        /** This BusAttachment already owns the name. The request has failed. */
        AlreadyOwner;
    }

    /** {@link #ReleaseName(String)} return value. */
    enum ReleaseNameResult {

        /** Invalid. */
        Invalid,

        /** The name has been released. */
        Released,

        /**
         * The name is unknown to the AllJoyn router and therefore cannot be
         * released.
         */
        NonExistent,

        /**
         * The name is owned by another bus connection and therefore cannot be
         * released.
         */
        NotOwner,
    }

    /**
     * Returns a list of well-known names registered with the bus.
     *
     * @return the well-known names
     * @throws BusException indicating failure when calling ListNames method
     */
    @BusMethod(replySignature = "as")
    String[] ListNames() throws BusException;

    /**
     * Returns a list of well-known names that the local AllJoyn router has been
     * configured to automatically start routers for when the names are
     * accessed.
     *
     * @return the well-known names
     * @throws BusException indicating failure when calling ListActivatableNames method
     */
    @BusMethod(replySignature = "as")
    String[] ListActivatableNames() throws BusException;

    /**
     * Requests a well-known name.
     *
     * @param name name being requested
     * @param flags bitmask of RequestName flags
     * @return disposition of request name operation
     * @throws BusException indicating failure when calling RequestName method
     */
    @BusMethod(signature = "su", replySignature = "u")
    RequestNameResult RequestName(String name, int flags) throws BusException;

    /**
     * Releases a previously registered well-known name.
     *
     * @param name name being released
     * @return disposition of release name operation
     * @throws BusException indicating failure when calling ReleaseName method
     */
    @BusMethod(signature = "s", replySignature = "u")
    ReleaseNameResult ReleaseName(String name) throws BusException;

    /**
     * Returns {@code true} if name has an owner on the bus.
     *
     * @param name name to check for ownership
     * @return {@code true} iff the well-known name is currently registered with the bus
     * @throws BusException indicating failure when calling NameHasOwner method
     */
    @BusMethod(signature = "s", replySignature = "b")
    boolean NameHasOwner(String name) throws BusException;

    /** {@link #StartServiceByName(String, int)} return value. */
    enum StartServiceByNameResult {

        /** Invalid. */
        Invalid,

        /** Service is started. */
        Success,

        /** Service is already running. */
        AlreadyRunning;
    }

    /**
     * Starts the local router process responsible for a given well-known name.
     * Names for services that can be started in this manner are returned by
     * {@link #ListActivatableNames()}.
     *
     * @param name well-known name whose service process will be started
     * @param flags not used
     * @return disposition of start service by name operation
     * @throws BusException indicating failure when when calling StartServiceByName method
     */
    @BusMethod(signature = "su", replySignature = "u")
    StartServiceByNameResult StartServiceByName(String name, int flags) throws BusException;

    /**
     * Gets the unique id of the bus connection that owns a given well-known
     * name.
     *
     * @param name well-known name.
     * @return unique id that owns well-known name
     * @throws BusException if the well-known name is not owned
     */
    @BusMethod(signature = "s", replySignature = "s")
    String GetNameOwner(String name) throws BusException;

    /**
     * Gets the uid of the process connected to the local AllJoyn router identified
     * by the well-known or unique name.
     *
     * @param name well-known or unique name whose connection will be examined
     * @return uid of process connected to local AllJoyn router with given name
     * @throws BusException if the uid cannot be determined
     */
    @BusMethod(signature = "s", replySignature = "u")
    int GetConnectionUnixUser(String name) throws BusException;

    /**
     * Gets the pid of the process connected to the local AllJoyn router identified
     * by the well-known or unique name.
     *
     * @param name well-known or unique name whose connection will be examined
     * @return pid of process connected to local AllJoyn router with the given name
     * @throws BusException if the pid cannot be determined
     */
    @BusMethod(signature = "s", replySignature = "u")
    int GetConnectionUnixProcessID(String name) throws BusException;

    /**
     * Adds a DBus routing rule.
     * See the DBus spec for details and format of DBus routing rules.
     *
     * @param rule match rule to add
     * @throws BusException indicating failure when calling AddMatch method
     */
    @BusMethod(signature = "s")
    void AddMatch(String rule) throws BusException;

    /**
     * Removes a DBus routing rule.
     * See the DBus spec for details on DBus routing.
     *
     * @param rule match rule to remove
     * @throws BusException indicating failure when calling RemoveMatch method
     */
    @BusMethod(signature = "s")
    void RemoveMatch(String rule) throws BusException;

    /**
     * Gets the unique id of the bus.
     *
     * @return the GUID for the local AllJoyn router in string form
     * @throws BusException indicating failure when calling GetId method
     */
    @BusMethod(replySignature = "s")
    String GetId() throws BusException;

    /**
     * Updates the activation environment.
     *
     * @param environment the environment to add or update
     * @throws BusException indicating failure when calling UpdateActivationEnvironment method
     */
    @BusMethod(signature = "a{ss}")
    void UpdateActivationEnvironment(Map<String, String> environment) throws BusException;

    /**
     * Gets the list of queued owners.
     *
     * @param name the well-known bus name to query
     * @return the unique bus names of connections currently queued for the name
     * @throws BusException indicating failure when calling ListQueuedOwners method
     */
    @BusMethod(signature = "s", replySignature = "as")
    String[] ListQueuedOwners(String name) throws BusException;

    /**
     * Gets the ADT audit session data.
     *
     * @param name the name of the connection to query
     * @return an array of bytes
     * @throws BusException indicating failure when calling GetAdtAudidSessionData method
     */
    @BusMethod(signature = "s", replySignature = "ay")
    byte[] GetAdtAuditSessionData(String name) throws BusException;

    /**
     * Gets the SE Linux security context for a connection.
     *
     * @param name the name of the connection to query
     * @return an array of bytes
     * @throws BusException indicating failure when calling GetConnectionSELinuxSecurityContext method
     */
    @BusMethod(signature = "s", replySignature = "ay")
    byte[] GetConnectionSELinuxSecurityContext(String name) throws BusException;

    /**
     * Reloads the config file.
     *
     * @throws BusException indicating failure when calling ReloadConfig method
     */
    @BusMethod
    void ReloadConfig() throws BusException;

    /**
     * Signal broadcast whenever a name (well-known or unique) changes
     * ownership.
     *
     * @param name well-known or unique name that has changed owner
     * @param oldOwner unique name of previous owner or empty string if no
     *                 previous owner
     * @param newOwner unique name of new name owner or empty string if no new
     *                 owner
     * @throws BusException indicating failure when sending the NameOwnerChanged signal
     */
    @BusSignal(signature = "sss")
    void NameOwnerChanged(String name, String oldOwner, String newOwner) throws BusException;

    /**
     * Signal sent (non-broadcast) to the bus connection that loses a name.
     *
     * @param name name that was lost
     * @throws BusException indicating failure when sending the NameLost signal
     */
    @BusSignal(signature = "s")
    void NameLost(String name) throws BusException;

    /**
     * Signal sent (non-broadcast) to the bus connection that acquires a name.
     *
     * @param name name that was acquired from the bus
     * @throws BusException indicating failure when sending the NameAcquired signal
     */
    @BusSignal(signature = "s")
    void NameAcquired(String name) throws BusException;
}
