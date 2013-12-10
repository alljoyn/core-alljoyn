//-----------------------------------------------------------------------
// <copyright file="NameChangeBusListener.cs" company="AllSeen Alliance.">
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

namespace Name_Change_Bus_Listener
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using AllJoyn;
    using Name_Change_Client;
    using Name_Change_Globals;

    /// <summary>
    /// This is the bus listener specific to the basic client which handles its events 
    /// </summary>
    public class NameChangeBusListener
    {
        /// <summary>
        /// seesion listener for the client
        /// </summary>
        private SessionListener sessionListener;

        /// <summary>
        /// bus listener for the client
        /// </summary>
        private BusListener busListener;

        /// <summary>
        /// The D-Bus attachment which this client is connected to
        /// </summary>
        private BusAttachment busAtt;

        /// <summary>
        /// nameFoundEvent which is set when BusListenerFoundAdvertisedName is called by the bus
        /// </summary>
        private AutoResetEvent foundName;

        /// <summary>
        /// Initializes a new instance of the <see cref="NameChangeBusListener" /> class
        /// </summary>
        /// <param name="bus">object responsible for connecting to and optionally managing a message 
        /// bus</param>
        /// <param name="foundNameEvent">event to set when the well-known name being queried 
        /// is found</param>
        public NameChangeBusListener(BusAttachment bus, AutoResetEvent foundNameEvent)
        {
            this.busAtt = bus;
            this.foundName = foundNameEvent;

            this.busListener = new BusListener(this.busAtt);
            this.busListener.BusDisconnected += new BusListenerBusDisconnectedHandler(this.BusListenerBusDisconnected);
            this.busListener.BusStopping += new BusListenerBusStoppingHandler(this.BusListenerBusStopping);
            this.busListener.FoundAdvertisedName += new BusListenerFoundAdvertisedNameHandler(this.BusListenerFoundAdvertisedName);
            this.busListener.ListenerRegistered += new BusListenerListenerRegisteredHandler(this.BusListenerListenerRegistered);
            this.busListener.ListenerUnregistered += new BusListenerListenerUnregisteredHandler(this.BusListenerListenerUnregistered);
            this.busListener.LostAdvertisedName += new BusListenerLostAdvertisedNameHandler(this.BusListenerLostAdvertisedName);
            this.busListener.NameOwnerChanged += new BusListenerNameOwnerChangedHandler(this.BusListenerNameOwnerChanged);

            this.sessionListener = new SessionListener(this.busAtt);
            this.sessionListener.SessionLost += new SessionListenerSessionLostHandler(this.SessionListenerSessionLost);
            this.sessionListener.SessionMemberAdded += new SessionListenerSessionMemberAddedHandler(this.SessionListenerSessionMemberAdded);
            this.sessionListener.SessionMemberRemoved += new SessionListenerSessionMemberRemovedHandler(this.SessionListenerSessionMemberRemoved);
        }

        /// <summary>
        /// Gets or sets the current session id of the client-service application
        /// </summary>
        public uint SessionId
        {
            get { return this.SessionId; }
            set { this.SessionId = value; }
        }

        /// <summary>
        /// return the sesssion listener for 'msl'
        /// </summary>
        /// <param name="nameChangeBusListener">the bus listener for the program</param>
        /// <returns>session listener stored in msl</returns>
        public static implicit operator SessionListener(NameChangeBusListener nameChangeBusListener)
        {
            return nameChangeBusListener.sessionListener;
        }

        /// <summary>
        /// returns the buslistener for 'msl'
        /// </summary>
        /// <param name="nameChangeBusListener">the bus listener for the program</param>
        /// <returns></returns>
        public static implicit operator BusListener(NameChangeBusListener nameChangeBusListener)
        {
            return nameChangeBusListener.busListener;
        }

        /// <summary>
        /// Called by the bus when ownership of any well-known name changes
        /// </summary>
        /// <param name="busName">the well-known name that has changed</param>
        /// <param name="newOwner">The unique name that previously owned the name or NULL if 
        /// there was no previous owner.</param>
        /// <param name="oldOwner">The unique name that now owns the name or NULL if the there 
        /// is no new owner.</param>
        public void BusListenerNameOwnerChanged(string busName, string newOwner, string oldOwner)
        {
            App.OutputLine("Name Owner Changed (WKN=" + busName + " newOwner=" + newOwner + " prevOwner=" + oldOwner + ")");
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
            App.OutputLine("Lost Advertised name '" + name + "'.");
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
            if (name.CompareTo(NameChangeGlobals.WellKnownServiceName) == 0)
            {
                App.OutputLine("Found Advertised Name (Name=" + name + "\tPrefix=" + namePrefix + ")");
                this.foundName.Set();
            }
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
        /// Called when a BusAttachment this listener is registered with is has become disconnected 
        /// from the bus.
        /// </summary>
        /// <param name="sessionID">Id of session whose member(s) changed.</param>
        /// <param name="uniqueName">Unique name of member who was added.</param>
        public void SessionListenerSessionMemberRemoved(uint sessionID, string uniqueName)
        {
        }

        /// <summary>
        /// Called by the bus when a member of a multipoint session is added.
        /// </summary>
        /// <param name="sessionID">Id of session whose member(s) changed.</param>
        /// <param name="uniqueName">Unique name of member who was added.</param>
        public void SessionListenerSessionMemberAdded(uint sessionID, string uniqueName)
        {
        }

        /// <summary>
        /// Called by the bus when an existing session becomes disconnected.
        /// </summary>
        /// <param name="sessionID">Id of session that was lost.</param>
        public void SessionListenerSessionLost(uint sessionID)
        {
        }
    }
}
