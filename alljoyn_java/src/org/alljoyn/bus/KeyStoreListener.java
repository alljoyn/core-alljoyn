/*
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

/**
 * Implemented by a user-defined class that is responsible for loading and
 * storing keys.  Encryption and decryption of key data is handled internally
 * using the password supplied by {@code getPassword}.
 *
 * Listener objects are the Java objects that handle notification events and are
 * called from AllJoyn in the context of one of its threads.  All listener
 * objects are expected to be multithread safe (MT-Safe) between construction
 * and destruction.
 *
 * This rule extends to other objects accessed during processing of
 * notifications.  For example, it is a programming error to allow a notifiation
 * method to access a collection in another object without serializing access
 * to the collection.
 *
 * The important consideration in this case is that as soon as one sets up a
 * listener object to receive notifications from AllJoyn, one is implicitly
 * dealing with multithreaded code.
 *
 * Since listener objects generally run in the context of the AllJoyn thread
 * which manages reception of events, If a blocking AllJoyn call is made in
 * the context of a notification, the necessary and sufficient conditions for
 * deadlock are established.
 *
 * The important consideration in this case is that when one receives a
 * notification from AllJoyn, that notification is executing in the context of
 * an AllJoyn thread.  If one makes a blocking call back into AllJoyn on that
 * thread, a deadlock cycle is likely, and if this happens your bus attachment
 * receive thread will deadlock (with itself).  The deadlock is typically broken
 * after a bus timeout eventually happens.
 *
 * Note well that calls into the KeyStoreListener will be happening in the
 * context of an AllJoyn endpoint receive thread, and therefore long-lived
 * operations will block that endpoint for the duration of the call.  Don't
 * dawdle in your implementations of these functions.
 */
public interface KeyStoreListener {

    /**
     * Asks to read encrypted key data, presumably from a local KeyStore.
     *
     * The return value here is a reference to a byte array Object.  Upon
     * return, this reference will be held by AllJoyn code which is certainly
     * running on a different thread than the Java client that has provided
     * the listener.  Since there are two threads and one Object involved,
     * one must consider multi-thread safety.
     *
     * Note that since there is no enforced rule that there is a one-to-one
     * relationship between endpoints and KeyStoreListeners, it is possible to
     * to have putKeys requests interleave with getKeys and getPasswords on
     * different threads.
     *
     * It would be possible to require the KeyStoreListener to provide a
     * public critical section mechanism to allow the AllJoyn bindings to
     * serialize access to the keys array, but this would complicate simple
     * cases.  We choose to let the client decide how to best handle the
     * situation and do not impose a locking scheme.  This means that the
     * reference returned to AllJoyn must be to a byte[] that will not change
     * during the lifetime of the AllJoyn bus attachment.
     *
     * The basic alternatives are:
     *
     * - If the keys are guaranteed not to change after being initialized i.e.,
     *   they are final, the client can simply return a reference to the
     *   internal byte array.
     *
     * - If the keys only change at times when it is impossible for the AllJoyn
     *   code to be using them, the client can simply return a reference to the
     *   internal byte array.
     *
     * - If the keys are dynamic and could change while AllJoyn is using them,
     *   the client must return a current copy of the keys to AllJoyn which
     *   will not change (and should be consistent with values returned for
     *   getPassword).
     *
     * @throws BusException indicating failure getting the encrypted key data
     *
     * @return a reference to a copy of the key data if it is mutable, or a
     *         reference to the key data if not.
     */
    byte[] getKeys() throws BusException;

    /**
     * Asks to provide the password required to decrypt/encrypt the keys.
     *
     * The return value here is a reference to a byte array Object.  Upon
     * return, this reference will be held by AllJoyn code which is certainly
     * running on a different thread than the Java client that has provided
     * the listener.  Since there are two threads and one Object involved,
     * one must consider multi-thread safety.
     *
     * Note that since there is no enforced rule that there is a one-to-one
     * relationship between endpoints and KeyStoreListeners, it is possible to
     * to have putKeys requests interleave with getKeys and getPasswords on
     * different threads.
     *
     * It would be possible to require the KeyStoreListener to provide a
     * public critical section mechanism to allow the AllJoyn bindings to
     * serialize access to the password array, but this would complicate simple
     * cases.  We choose to let the client decide how to best handle the
     * situation and do not impose a locking scheme.  This means that the
     * reference returned to AllJoyn must be to a char[] that will not change
     * during the lifetime of the AllJoyn bus attachment (unless the AllJoyn
     * code decides to do so).
     *
     * The basic alternatives are:
     *
     * - If the password is guaranteed not to change after being initialized
     *   i.e., it is final, the client can simply return a reference to the
     *   internal char array.  Note that the AllJoyn code will clear the bytes
     *   stored in the password array after it is done with them.
     *
     * - If the password only changes at times when it is impossible for the
     *   AllJoyn code to be using it, the client can simply return a reference
     *   to the internal char array.  AllJoyn will clear the bytes stored in
     *   the password array after it is done with them.
     *
     * - If the password is dynamic and could change while AllJoyn is using it,
     *   the client must return a current copy of the password to AllJoyn
     *   which it prmises not to change.  AllJoyn will clear the bytes stored in
     *   the retruned password array object after it is done with them.
     *
     * @throws BusException indicating failure getting the password
     * @return a reference to a copy of the password if it is mutable, or a
     *         reference to the key data if not.
     */
    char[] getPassword() throws BusException;

    /**
     * A request to write the encrypted keys, presumably to a local key store.
     *
     * Note that since there is no enforced rule that there is a one-to-one
     * relationship between endpoints and KeyStoreListeners, it is possible to
     * to have putKeys requests interleave with getKeys and getPasswords on
     * different threads.
     *
     * The implementation of the KeyStoreListener is expected to provide a
     * multi-thread safe version.  The AllJoyn Java bindings will simply use
     * that implementation, trusting that it produces consistent results.
     *
     * @throws BusException indication failure to write the encrypted keys
     * @param keys the key data
     */
    void putKeys(byte[] keys) throws BusException;
}
