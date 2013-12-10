//-----------------------------------------------------------------------
// <copyright file="BasicServiceBusListener.cs" company="AllSeen Alliance.">
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

namespace Basic_Service_Bus_Listener
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Text;
    using System.Threading.Tasks;
    using AllJoyn;
    using Basic_CSharp_;

    /// <summary>
    /// This is the bus listener specific to the basic service which handles its events 
    /// </summary>
    public class BasicServiceBusListener
    {
        /// <summary>
        /// Session id for the current service and client session
        /// </summary>
        private uint sessionId;

        /// <summary>
        /// seesion listener for the service
        /// </summary>
        private SessionListener sessionListener;

        /// <summary>
        /// session port listener for the service
        /// </summary>
        private SessionPortListener sessionPortListener;

        /// <summary>
        /// bus listener for the client
        /// </summary>
        private BusListener busListener;

        /// <summary>
        /// The D-Bus attachment which this client is connected to
        /// </summary>
        private BusAttachment busAtt;

        /// <summary>
        /// Initializes a new instance of the <see cref="BasicServiceBusListener" /> class
        /// </summary>
        /// <param name="bus">object responsible for connecting to and optionally managing a message 
        /// bus</param>
        public BasicServiceBusListener(BusAttachment bus)
        {
            SessionidList = new List<uint>();
            this.busAtt = bus;
            this.busListener = new BusListener(bus);
            this.busListener.BusDisconnected += new BusListenerBusDisconnectedHandler(this.BusListenerBusDisconnected);
            this.busListener.BusStopping += new BusListenerBusStoppingHandler(this.BusListenerBusStopping);
            this.busListener.FoundAdvertisedName += new BusListenerFoundAdvertisedNameHandler(this.BusListenerFoundAdvertisedName);
            this.busListener.ListenerRegistered += new BusListenerListenerRegisteredHandler(this.BusListenerListenerRegistered);
            this.busListener.ListenerUnregistered += new BusListenerListenerUnregisteredHandler(this.BusListenerListenerUnregistered);
            this.busListener.LostAdvertisedName += new BusListenerLostAdvertisedNameHandler(this.BusListenerLostAdvertisedName);
            this.busListener.NameOwnerChanged += new BusListenerNameOwnerChangedHandler(this.BusListenerNameOwnerChanged);

            this.sessionPortListener = new SessionPortListener(bus);
            this.sessionPortListener.AcceptSessionJoiner += new SessionPortListenerAcceptSessionJoinerHandler(this.SessionPortListenerAcceptSessionJoiner);
            this.sessionPortListener.SessionJoined += new SessionPortListenerSessionJoinedHandler(this.SessionPortListenerSessionJoined);

            this.sessionListener = new SessionListener(bus);
            this.sessionListener.SessionLost += new SessionListenerSessionLostHandler(this.SessionListenerSessionLost);
            this.sessionListener.SessionMemberAdded += new SessionListenerSessionMemberAddedHandler(this.SessionListenerSessionMemberAdded);
            this.sessionListener.SessionMemberRemoved += new SessionListenerSessionMemberRemovedHandler(this.SessionListenerSessionMemberRemoved);
        }

        /// <summary>
        /// Gets or sets the active sessionID's which this listener belongs to
        /// </summary>
        private static List<uint> SessionidList { get; set; }

        /// <summary>
        /// Gets or sets the active sessionID
        /// </summary>
        private uint Sessionid 
        {
            get { return this.sessionId; } set { this.sessionId = value; } 
        }

        /// <summary>
        /// return the bus listener for 'mbl'
        /// </summary>
        /// <param name="serviceBusListener">the bus listener for the program</param>
        /// <returns>session listener stored in mbl</returns>
        public static implicit operator BusListener(BasicServiceBusListener serviceBusListener)
        {
            return serviceBusListener.busListener;
        }

        /// <summary>
        /// return the sesssion port listener for 'mbl'
        /// </summary>
        /// <param name="serviceBusListener">the bus listener for the program</param>
        /// <returns>session listener stored in mbl</returns>
        public static implicit operator SessionPortListener(BasicServiceBusListener serviceBusListener)
        {
            return serviceBusListener.sessionPortListener;
        }

        /// <summary>
        /// return the sesssion listener for 'mbl'
        /// </summary>
        /// <param name="serviceBusListener">the bus listener for the program</param>
        /// <returns>session listener stored in mbl</returns>
        public static implicit operator SessionListener(BasicServiceBusListener serviceBusListener)
        {
            return serviceBusListener.sessionListener;
        }

        /// <summary>
        /// Called by the bus when a member of a multipoint session is removed.
        /// </summary>
        /// <param name="sessionID">Id of session whose member(s) changed.</param>
        /// <param name="uniqueName">Unique name of member who was removed.</param>
        public void SessionListenerSessionMemberRemoved(uint sessionID, string uniqueName)
        {
            App.OutputLine("Member '" + uniqueName + "' has left the session.");
        }

        /// <summary>
        /// Called by the bus when a member of a multipoint session is added.
        /// </summary>
        /// <param name="sessionID">Id of session whose member(s) changed.</param>
        /// <param name="uniqueName">Unique name of member who was added.</param>
        public void SessionListenerSessionMemberAdded(uint sessionID, string uniqueName)
        {
            App.OutputLine("Member '" + uniqueName + "' was added to the session.");
        }

        /// <summary>
        /// Called by the bus when an existing session becomes disconnected.
        /// </summary>
        /// <param name="sessionID">Id of session that was lost.</param>
        public void SessionListenerSessionLost(uint sessionID)
        {
        }

        /// <summary>
        /// Called by the bus when ownership of any well-known name changes
        /// </summary>
        /// <param name="busName">the well-known name that has changed</param>
        /// <param name="prevOwner">The unique name that previously owned the name or NULL if 
        /// there was no previous owner.</param>
        /// <param name="newOwner">The unique name that now owns the name or NULL if the there 
        /// is no new owner.</param>
        public void BusListenerNameOwnerChanged(string busName, string prevOwner, string newOwner)
        {
            App.OutputLine("Name Owner Changed (WKN=" + busName + " newOwner=" + newOwner + " prevOwner=" + prevOwner + ")");
        }

        /// <summary>
        /// Called by the bus when an advertisement previously reported through FoundName has 
        /// become unavailable.
        /// </summary>
        /// <param name="name">A well known name that the remote bus is advertising that is 
        /// of interest to this attachment.</param>
        /// <param name="transport">Transport that stopped receiving the given advertised 
        /// name.</param>
        /// <param name="namePrefix">The well-known name prefix that was used in a call to 
        /// FindAdvertisedName that triggered this callback.</param>
        public void BusListenerLostAdvertisedName(string name, TransportMaskType transport, string namePrefix)
        {
        }

        /// <summary>
        /// Called by the bus when the listener is unregistered.
        /// </summary>
        public void BusListenerListenerUnregistered()
        {
        }

        /// <summary>
        /// Called by the bus when the listener is registered. This give the listener implementation 
        /// the opportunity to save a reference to the bus.
        /// </summary>
        /// <param name="bus">The bus the listener is registered with.</param>
        public void BusListenerListenerRegistered(BusAttachment bus)
        {
        }

        /// <summary>
        /// Called by the bus when an external bus is discovered that is advertising a well-known 
        /// name that this attachment has registered interest in via a DBus call to 
        /// org.alljoyn.Bus.FindAdvertisedName
        /// </summary>
        /// <param name="name">A well known name that the remote bus is advertising.</param>
        /// <param name="transport">Transport that received the advertisement.</param>
        /// <param name="namePrefix">The well-known name prefix used in call to FindAdvertisedName 
        /// that triggered this callback.</param>
        public void BusListenerFoundAdvertisedName(string name, TransportMaskType transport, string namePrefix)
        {
        }

        /// <summary>
        /// Called when a BusAttachment this listener is registered with is stopping.
        /// </summary>
        public void BusListenerBusStopping()
        {
        }

        /// <summary>
        /// Called when a BusAttachment this listener is registered with is has become disconnected 
        /// from the bus.
        /// </summary>
        public void BusListenerBusDisconnected()
        {
        }

        /// <summary>
        /// Called by the bus when a session has been successfully joined. The session is 
        /// now fully up.
        /// </summary>
        /// <param name="sessionPort">Session port that was joined.</param>
        /// <param name="id">Id of session.</param>
        /// <param name="joiner">Unique name of the joiner.</param>
        private void SessionPortListenerSessionJoined(ushort sessionPort, uint id, string joiner)
        {
            App.OutputLine("Joined Session with '" + joiner + "' (SessionId=" + id + ").");
            this.sessionId = id;
        }

        /// <summary>
        /// Accept or reject an incoming JoinSession request. The session does not exist until 
        /// after this function returns.
        /// </summary>
        /// <param name="sessionPort">Session port that was joined.</param>
        /// <param name="joiner">Unique name of potential joiner.</param>
        /// <param name="opts">Session options requested by the joiner.</param>
        /// <returns>Return true if JoinSession request is accepted. false if rejected</returns>
        private bool SessionPortListenerAcceptSessionJoiner(ushort sessionPort, string joiner, SessionOpts opts)
        {
            App.OutputLine("Accepting JoinSessionRequest from " + joiner);
            return true;
        }
    }
}
