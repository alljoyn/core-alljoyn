//-----------------------------------------------------------------------
// <copyright file="Listeners.cs" company="AllSeen Alliance.">
//     Copyright (c) 2012, AllSeen Alliance. All rights reserved.
//
//        Permission to use, copy, modify, and/or distribute this software for any
//        purpose with or without fee is hereby granted, provided that the above
//        copyright notice and this permission notice appear in all copies.
//
//        THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//        WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//        MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//        ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//        WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//        ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//        OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
// </copyright>
//-----------------------------------------------------------------------

namespace FileTransferClient.Common
{
    using AllJoyn;
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// Concrete class which wraps AllJoyn SessionListener, SessionPortListener and BusListener events. 
    /// </summary>
    public class Listeners
    {
        /// <summary>
        /// The SessionLisener.
        /// </summary>
        private SessionListener sessionListener;

        /// <summary>
        /// The SessionPortLisener.
        /// </summary>
        private SessionPortListener sessionPortListener;

        /// <summary>
        /// The BusListener.
        /// </summary>
        private BusListener busListener;

        /// <summary>
        /// The BusAttachment.
        /// </summary>
        private BusAttachment bus;

        /// <summary>
        /// Initializes a new instance of the Listeners class.
        /// </summary>
        /// <param name="bus">The BusAttachment to use.</param>
        public Listeners(BusAttachment bus)
        {
            this.bus = bus;
            this.busListener = new BusListener(this.bus);
            this.busListener.BusDisconnected += new BusListenerBusDisconnectedHandler(this.BusListenerBusDisconnected);
            this.busListener.BusStopping += new BusListenerBusStoppingHandler(this.BusListenerBusStopping);
            this.busListener.FoundAdvertisedName += new BusListenerFoundAdvertisedNameHandler(this.BusListenerFoundAdvertisedName);
            this.busListener.ListenerRegistered += new BusListenerListenerRegisteredHandler(this.BusListenerListenerRegistered);
            this.busListener.ListenerUnregistered += new BusListenerListenerUnregisteredHandler(this.BusListenerListenerUnregistered);
            this.busListener.LostAdvertisedName += new BusListenerLostAdvertisedNameHandler(this.BusListenerLostAdvertisedName);
            this.busListener.NameOwnerChanged += new BusListenerNameOwnerChangedHandler(this.BusListenerNameOwnerChanged);

            this.sessionPortListener = new SessionPortListener(this.bus);
            this.sessionPortListener.AcceptSessionJoiner += new SessionPortListenerAcceptSessionJoinerHandler(this.SessionPortListenerAcceptSessionJoiner);
            this.sessionPortListener.SessionJoined += new SessionPortListenerSessionJoinedHandler(this.SessionPortListenerSessionJoined);

            this.sessionListener = new SessionListener(this.bus);
            this.sessionListener.SessionLost += new SessionListenerSessionLostHandler(this.SessionListenerSessionLost);
            this.sessionListener.SessionMemberAdded += new SessionListenerSessionMemberAddedHandler(this.SessionListenerSessionMemberAdded);
            this.sessionListener.SessionMemberRemoved += new SessionListenerSessionMemberRemovedHandler(this.SessionListenerSessionMemberRemoved);
        }

        /// <summary>
        /// Called by the bus when a member of a multipoint session is removed.
        /// </summary>
        public event SessionListenerSessionMemberRemovedHandler SessionMemberRemoved;

        /// <summary>
        /// Called by the bus when a member of a multipoint session is added.
        /// </summary>
        public event SessionListenerSessionMemberAddedHandler SessionMemberAdded;

        /// <summary>
        /// Called by the bus when an existing session becomes disconnected.
        /// </summary>
        public event SessionListenerSessionLostHandler SessionLost;

        /// <summary>
        /// Called by the bus when the ownership of any well-known name changes.
        /// </summary>
        public event BusListenerNameOwnerChangedHandler NameOwnerChange;

        /// <summary>
        /// Called by the bus when an advertisement previously reported through FoundName has become unavailable.
        /// </summary>
        public event BusListenerLostAdvertisedNameHandler LostAdvertisedName;

        /// <summary>
        /// Called by the bus when the listener is unregistered.
        /// </summary>
        public event BusListenerListenerUnregisteredHandler ListenerUnregistered;

        /// <summary>
        /// Called by the bus when the listener is registered. This gives the listener implementation the
        /// opportunity to save a reference to the bus.
        /// </summary>
        public event BusListenerListenerRegisteredHandler ListenerRegistered;

        /// <summary>
        /// Called by the bus when an external bus is discovered that is advertising a well-known name
        /// that this attachment has registered interest in via a DBus call to org.alljoyn.Bus.FindAdvertisedName
        /// </summary>
        public event BusListenerFoundAdvertisedNameHandler FoundAdvertisedName;

        /// <summary>
        /// Called when a BusAttachment this listener is registered with is stopping.
        /// </summary>
        public event BusListenerBusStoppingHandler BusStopping;

        /// <summary>
        /// Called when a BusAttachment this listener is registered with is has become disconnected from
        /// the bus.
        /// </summary>
        public event BusListenerBusDisconnectedHandler BusDisconnected;

        /// <summary>
        /// Accept or reject an incoming JoinSession request. The session does not exist until this
        /// after this function returns.
        /// </summary>
        public event SessionPortListenerAcceptSessionJoinerHandler AcceptSessionJoiner;

        /// <summary>
        /// Called by the bus when a session has been successfully joined. The session is now fully up.
        /// </summary>
        public event SessionPortListenerSessionJoinedHandler SessionJoined;

        /// <summary>
        /// Implicit cast operator for BusListener
        /// </summary>
        /// <param name="mbl">Listeners collection</param>
        /// <returns>BusListener</returns>
        public static implicit operator BusListener(Listeners mbl)
        {
            return mbl.busListener;
        }

        /// <summary>
        /// Implicit cast operator for SessionPortListener
        /// </summary>
        /// <param name="mbl">Listeners collection</param>
        /// <returns>SessionPortListener</returns>
        public static implicit operator SessionPortListener(Listeners mbl)
        {
            return mbl.sessionPortListener;
        }

        /// <summary>
        /// Implicit cast operator for SessionListener
        /// </summary>
        /// <param name="mbl">Listeners collection</param>
        /// <returns>SessionListener</returns>
        public static implicit operator SessionListener(Listeners mbl)
        {
            return mbl.sessionListener;
        }

        /// <summary>
        /// Called by the bus when a member of a multipoint session is removed.
        /// </summary>
        /// <param name="value">Id of session whose member(s) changed.</param>
        /// <param name="value2">Unique name of member who was removed.</param>
        private void SessionListenerSessionMemberRemoved(uint value, string value2)
        {
            if (this.SessionMemberRemoved != null)
            {
                this.SessionMemberRemoved(value, value2);
            }
        }

        /// <summary>
        /// Called by the bus when a member of a multipoint session is added.
        /// </summary>
        /// <param name="sessionId">Id of session whose member(s) changed.</param>
        /// <param name="uniqueName">Unique name of member who was added.</param>
        private void SessionListenerSessionMemberAdded(uint sessionId, string uniqueName)
        {
            if (this.SessionMemberAdded != null)
            {
                this.SessionMemberAdded(sessionId, uniqueName);
            }
        }

        /// <summary>
        /// Called by the bus when an existing session becomes disconnected.
        /// </summary>
        /// <param name="sessionId">Id of session that was lost.</param>
        private void SessionListenerSessionLost(uint sessionId)
        {
            if (this.SessionLost != null)
            {
                this.SessionLost(sessionId);
            }
        }

        /// <summary>
        /// Called by the bus when the ownership of any well-known name changes.
        /// </summary>
        /// <param name="value">The well-known name that has changed.</param>
        /// <param name="value2">The unique name that previously owned the name or NULL if there was no previous owner.</param>
        /// <param name="value3">The unique name that now owns the name or NULL if the there is no new owner.</param>
        private void BusListenerNameOwnerChanged(string value, string value2, string value3)
        {
            if (this.NameOwnerChange != null)
            {
                this.NameOwnerChange(value, value2, value3);
            }
        }

        /// <summary>
        /// Called by the bus when an advertisement previously reported through FoundName has become unavailable.
        /// </summary>
        /// <param name="value">A well known name that the remote bus is advertising that is of interest to this attachment.</param>
        /// <param name="value2">Transport that stopped receiving the given advertised name.</param>
        /// <param name="value3">The well-known name prefix that was used in a call to FindAdvertisedName that triggered this callback.</param>
        private void BusListenerLostAdvertisedName(string value, TransportMaskType value2, string value3)
        {
            if (this.LostAdvertisedName != null)
            {
                this.LostAdvertisedName(value, value2, value3);
            }
        }

        /// <summary>
        /// Called by the bus when the listener is unregistered.
        /// </summary>
        private void BusListenerListenerUnregistered()
        {
            if (this.ListenerUnregistered != null)
            {
                this.ListenerUnregistered();
            }
        }

        /// <summary>
        /// Called by the bus when the listener is registered. This gives the listener implementation the
        /// opportunity to save a reference to the bus.
        /// </summary>
        /// <param name="value">The bus the listener is registered with.</param>
        private void BusListenerListenerRegistered(BusAttachment value)
        {
            if (this.ListenerRegistered != null)
            {
                this.ListenerRegistered(value);
            }
        }

        /// <summary>
        /// Called by the bus when an external bus is discovered that is advertising a well-known name
        /// that this attachment has registered interest in via a DBus call to org.alljoyn.Bus.FindAdvertisedName
        /// </summary>
        /// <param name="value">A well known name that the remote bus is advertising.</param>
        /// <param name="value2">Transport that received the advertisement.</param>
        /// <param name="value3">The well-known name prefix used in call to FindAdvertisedName that triggered this callback.</param>
        private void BusListenerFoundAdvertisedName(string value, TransportMaskType value2, string value3)
        {
            if (this.FoundAdvertisedName != null)
            {
                this.FoundAdvertisedName(value, value2, value3);
            }
        }

        /// <summary>
        /// Called when a BusAttachment this listener is registered with is stopping.
        /// </summary>
        private void BusListenerBusStopping()
        {
            if (this.BusStopping != null)
            {
                this.BusStopping();
            }
        }

        /// <summary>
        /// Called when a BusAttachment this listener is registered with is has become disconnected from
        /// the bus.
        /// </summary>
        private void BusListenerBusDisconnected()
        {
            if (this.BusDisconnected != null)
            {
                this.BusDisconnected();
            }
        }

        /// <summary>
        /// Called by the bus when a session has been successfully joined. The session is now fully up.
        /// </summary>
        /// <param name="value">Session port that was joined.</param>
        /// <param name="value2">Id of session.</param>
        /// <param name="value3">Unique name of the joiner.</param>
        private void SessionPortListenerSessionJoined(ushort value, uint value2, string value3)
        {
            if (this.SessionJoined != null)
            {
                this.SessionJoined(value, value2, value3);
            }
        }

        /// <summary>
        /// This callback is only used by session creators. Therefore it is only called on listeners
        /// passed to BusAttachment::BindSessionPort.
        /// </summary>
        /// <param name="sessionPort">Session port that was joined.</param>
        /// <param name="joiner">Unique name of potential joiner.</param>
        /// <param name="opts">Session options requested by the joiner.</param>
        /// <returns>True, iff the listener is accepting the join</returns>
        private bool SessionPortListenerAcceptSessionJoiner(ushort sessionPort, string joiner, SessionOpts opts)
        {
            if (this.AcceptSessionJoiner != null)
            {
                return this.AcceptSessionJoiner(sessionPort, joiner, opts);
            }

            return true;
        }
    }
}
