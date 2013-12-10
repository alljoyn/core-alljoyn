// ****************************************************************************
// <copyright file="Listeners.cs" company="AllSeen Alliance.">
// Copyright (c) 2012, AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
// </copyright>
// ******************************************************************************

namespace Chat.Common
{
    using AllJoyn;

    /// <summary>
    /// A class to handle all the listening events.
    /// </summary>
    public class Listeners
    {
        /// <summary>
        /// The SessionListener.
        /// </summary>
        private SessionListener sessionListener;

        /// <summary>
        /// The SessionPortLisener.
        /// </summary>
        private SessionPortListener sessionPortListener;

        /// <summary>
        /// The BusListener.
        /// </summary>
        private BusListener busListeners;

        /// <summary>
        /// The class which handles the user interface.
        /// </summary>
        private MainPage hostPage;

        /// <summary>
        /// Initializes a new instance of the Listeners class.
        /// </summary>
        /// <param name="bus">The bus to listen on.</param>
        /// <param name="host">The main page which handles the user interface.</param>
        public Listeners(BusAttachment bus, MainPage host)
        {
            this.hostPage = host;
            this.busListeners = new BusListener(bus);
            this.busListeners.BusDisconnected += new BusListenerBusDisconnectedHandler(this.BusListenerBusDisconnected);
            this.busListeners.BusStopping += new BusListenerBusStoppingHandler(this.BusListenerBusStopping);
            this.busListeners.FoundAdvertisedName += new BusListenerFoundAdvertisedNameHandler(this.BusListenerFoundAdvertisedName);
            this.busListeners.ListenerRegistered += new BusListenerListenerRegisteredHandler(this.BusListenerListenerRegistered);
            this.busListeners.ListenerUnregistered += new BusListenerListenerUnregisteredHandler(this.BusListenerListenerUnregistered);
            this.busListeners.LostAdvertisedName += new BusListenerLostAdvertisedNameHandler(this.BusListenerLostAdvertisedName);
            this.busListeners.NameOwnerChanged += new BusListenerNameOwnerChangedHandler(this.BusListenerNameOwnerChanged);

            this.sessionPortListener = new SessionPortListener(bus);
            this.sessionPortListener.AcceptSessionJoiner += new SessionPortListenerAcceptSessionJoinerHandler(this.SessionPortListenerAcceptSessionJoiner);
            this.sessionPortListener.SessionJoined += new SessionPortListenerSessionJoinedHandler(this.SessionPortListenerSessionJoined);

            this.sessionListener = new SessionListener(bus);
            this.sessionListener.SessionLost += new SessionListenerSessionLostHandler(this.SessionListenerSessionLost);
            this.sessionListener.SessionMemberAdded += new SessionListenerSessionMemberAddedHandler(this.SessionListenerSessionMemberAdded);
            this.sessionListener.SessionMemberRemoved += new SessionListenerSessionMemberRemovedHandler(this.SessionListenerSessionMemberRemoved);
        }

        /// <summary>
        /// Implicit cast operator for BusListener
        /// </summary>
        /// <param name="listeners">Listeners collection</param>
        /// <returns>BusListener</returns>
        public static implicit operator BusListener(Listeners listeners)
        {
            return listeners.busListeners;
        }

        /// <summary>
        /// Implicit cast operator for SessionPortListener.
        /// </summary>
        /// <param name="listeners">Listeners collection.</param>
        /// <returns>SessionPortListener.</returns>
        public static implicit operator SessionPortListener(Listeners listeners)
        {
            return listeners.sessionPortListener;
        }

        /// <summary>
        /// Implicit cast operator for SessionListener.
        /// </summary>
        /// <param name="listeners">Listeners collection.</param>
        /// <returns>SessionListener.</returns>
        public static implicit operator SessionListener(Listeners listeners)
        {
            return listeners.sessionListener;
        }

        /// <summary>
        /// Called by the bus when a member of a multipoint session is removed.
        /// </summary>
        /// <param name="sessionId">Id of session whose member(s) changed.</param>
        /// <param name="member">Unique name of member who was removed.</param>
        private void SessionListenerSessionMemberRemoved(uint sessionId, string member)
        {
        }

        /// <summary>
        /// Called by the bus when a member of a multipoint session is added.
        /// </summary>
        /// <param name="sessionId">Id of session whose member(s) changed.</param>
        /// <param name="uniqueName">Unique name of member who was added.</param>
        private void SessionListenerSessionMemberAdded(uint sessionId, string uniqueName)
        {
        }

        /// <summary>
        /// Called by the bus when an existing session becomes disconnected.
        /// </summary>
        /// <param name="sessionId">Id of session that was lost.</param>
        private void SessionListenerSessionLost(uint sessionId)
        {
            this.hostPage.SessionLost(sessionId);
        }

        /// <summary>
        /// Called by the bus when the ownership of any well-known name changes.
        /// </summary>
        /// <param name="value">The well-known name that has changed.</param>
        /// <param name="value2">The unique name that previously owned the name or NULL if there was no previous owner.</param>
        /// <param name="value3">The unique name that now owns the name or NULL if the there is no new owner.</param>
        private void BusListenerNameOwnerChanged(string value, string value2, string value3)
        {
        }

        /// <summary>
        /// Called by the bus when an advertisement previously reported through FoundName has
        /// become unavailable.
        /// </summary>
        /// <param name="name">A well known name that the remote bus is advertising that
        /// is of interest to this attachment.</param>
        /// <param name="transport">Transport that stopped receiving the given advertised name.</param>
        /// <param name="namePrefix">The well-known name prefix that was used in a call to
        /// FindAdvertisedName that triggered this callback.</param>
        private void BusListenerLostAdvertisedName(string name, TransportMaskType transport, string namePrefix)
        {
            if (this.hostPage != null)
            {
                this.hostPage.RemoveChannelName(name);
            }
        }

        /// <summary>
        /// Called by the bus when the listener is unregistered.
        /// </summary>
        private void BusListenerListenerUnregistered()
        {
        }

        /// <summary>
        /// Called by the bus when the listener is registered. This gives the listener implementation the
        /// opportunity to save a reference to the bus.
        /// </summary>
        /// <param name="bus">The bus the listener is registered with.</param>
        private void BusListenerListenerRegistered(BusAttachment bus)
        {
        }

        /// <summary>
        /// Called by the bus when an external bus is discovered that is advertising a well-known name
        /// that this attachment has registered interest in via a DBus call to org.alljoyn.Bus.FindAdvertisedName
        /// </summary>
        /// <param name="name">A well known name that the remote bus is advertising.</param>
        /// <param name="transport">Transport that received the advertisement.</param>
        /// <param name="namePrefix">The well-known name prefix used in call to FindAdvertisedName that triggered this callback.</param>
        private void BusListenerFoundAdvertisedName(string name, TransportMaskType transport, string namePrefix)
        {
            if (this.hostPage != null)
            {
                this.hostPage.AddChannelName(name);
            }
        }

        /// <summary>
        /// Called when a BusAttachment this listener is registered with is stopping.
        /// </summary>
        private void BusListenerBusStopping()
        {
        }

        /// <summary>
        /// Called when a BusAttachment this listener is registered with is has become disconnected from
        /// the bus.
        /// </summary>
        private void BusListenerBusDisconnected()
        {
        }

        /// <summary>
        /// Called by the bus when a session has been successfully joined. The session is now fully up.
        /// </summary>
        /// <param name="sessionPort">Session port that was joined.</param>
        /// <param name="id">Id of session.</param>
        /// <param name="joiner">Unique name of the joiner.</param>
        private void SessionPortListenerSessionJoined(ushort sessionPort, uint id, string joiner)
        {
            if (this.hostPage != null)
            {
                this.hostPage.SessionId = id;
                this.hostPage.DisplayStatus(joiner + " has joined session " + id);
            }
        }

        /// <summary>
        /// This callback is only used by session creators. Therefore it is only called on listeners
        /// passed to BusAttachment::BindSessionPort.
        /// </summary>
        /// <param name="sessionPort">Session port that was joined.</param>
        /// <param name="joiner">Unique name of potential joiner.</param>
        /// <param name="opts">Session options requested by the joiner.</param>
        /// <returns>True, iff the listener is accepting the join.</returns>
        private bool SessionPortListenerAcceptSessionJoiner(ushort sessionPort, string joiner, SessionOpts opts)
        {
            return true;
        }
    }
}
