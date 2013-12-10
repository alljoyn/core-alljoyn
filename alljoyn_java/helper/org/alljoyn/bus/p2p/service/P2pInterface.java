/*
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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

package org.alljoyn.bus.p2p.service;

public interface P2pInterface {
    /*
     * The daemon running on an Android system is expected to call this method
     * in order to ask this service to communicate the FindAdvertisedName from
     * an interested client to the Android P2P frameork.  A so-called name
     * prefix is provided which is wildcard matched against advertising
     * services.
     *
     * This is an asynchronous start call only.  If a corresponding name is
     * found, that event will be communicated via the FoundAdvertisedName
     * signal.
     */
    public int FindAdvertisedName(String namePrefix);

    /*
     * The daemon running on an Android system is expected to call this method
     * in order to stop looking for a given name prefix.
     */
    public int CancelFindAdvertisedName(String namePrefix);

    /*
     * The daemon running on an Android system is expected to call this method
     * in order to ask this service to communicate an AdvertiseName request
     * from an AllJoyn service associated to the daemon down to the Android
     * P2P frameork.  The well-known name of the service is provided along with
     * the GUID of the daemon.
     */
    public int AdvertiseName(String name, String guid);

    /*
     * The daemon running on an Android system is expected to call this method
     * in order to ask this service to stop advertising a well-known name
     * using the P2P frameork.
     */
    public int CancelAdvertiseName(String name, String guid);

    /*
     * Once a client associated to a daemon has been notified about the existence
     * of a service in which it is interested, it may decide to joing a session
     * on that service.  The first step in joining the session is to either join
     * an existing P2P group or begin the group formation processs.  This method
     * begins that process.
     *
     * This is an asynchronous start call only.  The result of the link
     * establishment is conveyed by either an OnLinkEstablished or OnLinkError
     * signal.
     */
    public int EstablishLink(String device, int groupOwnerIntent);

    /*
     * When a client or a service no longer has need of a P2P connection, it may
     * call this method to tear down the P2P group and release any resources
     * associated to the group.  The handle provided must be the handle returned
     * by the system in the OnLinkEstablished signal.
     *
     * This is an asynchronous start call only.  The link will be taken down and
     * resources released at a time convenient to the underlying framework.
     */
    public int ReleaseLink(int handle);

    /*
     * After a P2P link is established, only layer two information is available.
     * The AllJoyn daemon must run the multicast name service in order to discover
     * the layer three (IP) address and port of a daemon to which it must connect.
     * In order to specify which network interfaces to send multicast messages
     * over, we must be able to map a given link handle to an underlying network
     * interface name.  This method performs that mapping.
     */
    public String GetInterfaceNameFromHandle(int handle);

    /*
     * This signal communicates the existence of a found service name back from
     * the Android framework to the AllJoyn daemon.  The AllJoyn daemon will then
     * send a FoundAdvertisedName message to clients which have expressed interest
     * in discovering instances of the found name prefix.
     */
    public void OnFoundAdvertisedName(String name, String namePrefix, String guid, String device);

    /*
     * This signal communicates the disappearance of a previously found service
     * name back from the Android framework to the AllJoyn daemon.  The AllJoyn
     * daemon will then send a LostAdvertisedName message to clients which have
     * previously been notified of the existence of the given name prefix.
     */
    public void OnLostAdvertisedName(String name, String namePrefix, String guid, String device);

    /*
     * The AllJoyn dameon begins the process of a session join by calling the
     * EstablishLink method of this interface.  This signal communicates a
     * successful completion of a link establishment procedure from the Android
     * framework back to the AllJoyn daemon.
     */
    public void OnLinkEstablished(int handle, String p2pInterfaceName);

    /*
     * The AllJoyn dameon begins the process of a session join by calling the
     * EstablishLink method of this interface.  This signal communicates a
     * failed completion of a link establishment procedure from the Android
     * framework back to the AllJoyn daemon.
     */
    public void OnLinkError(int handle, int error);

    /*
     * Once a link has been successfully established, there must be a way to
     * communicate back to the AllJoyn dameon that the link has failed (for
     * example due to devices leaving proximity),  This signal communicates a
     * failed completion of a link establishment procedure from the Android
     * framework back to the AllJoyn daemon.
     */
    public void OnLinkLost(int handle);
}
