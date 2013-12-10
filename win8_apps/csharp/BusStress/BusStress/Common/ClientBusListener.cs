//-----------------------------------------------------------------------
// <copyright file="ClientBusListener.cs" company="AllSeen Alliance.">
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

namespace BusStress.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using AllJoyn;

    /// <summary>
    /// Alljoyn listener for the client stress operation
    /// </summary>
    public class ClientBusListener
    {
        /// <summary>
        /// Well-known service name
        /// </summary>
        private const string ServiceName = "org.alljoyn.Bus.test.bastress";

        /// <summary>
        /// Bus listener for session app
        /// </summary>
        private BusListener busListener;

        /// <summary>
        /// Seesion listener for session app
        /// </summary>
        private SessionListener sessionListener;

        /// <summary>
        /// Reference to the Stress operation using this bus listener
        /// </summary>
        private StressOperation stressOp;

        /// <summary>
        /// Mutex used to lock in the FoundAvertisedName callback
        /// </summary>
        private Mutex mutex;

        /// <summary>
        /// Indicates whether a service has been discovered
        /// </summary>
        private bool foundAvertisedName;

        /// <summary>
        /// Event which is signaled when the first service has been discovered
        /// </summary>
        private AutoResetEvent foundName;

        /// <summary>
        /// Initializes a new instance of the <see cref="ClientBusListener" /> class.
        /// </summary>
        /// <param name="busAtt">object responsible for connecting to and optionally managing a message
        /// bus.</param>
        /// <param name="op">Stress Operation using this class as listener object</param>
        /// <param name="foundNameEvent">Event to set when the first service is discovered</param>
        public ClientBusListener(BusAttachment busAtt, StressOperation op, AutoResetEvent foundNameEvent)
        {
            this.stressOp = op;
            this.mutex = new Mutex();
            this.foundAvertisedName = false;
            this.foundName = foundNameEvent;

            // Create Bus Listener and register signal handlers 
            this.busListener = new BusListener(busAtt);
            this.busListener.BusDisconnected += new BusListenerBusDisconnectedHandler(this.BusListenerBusDisconnected);
            this.busListener.BusStopping += new BusListenerBusStoppingHandler(this.BusListenerBusStopping);
            this.busListener.FoundAdvertisedName += new BusListenerFoundAdvertisedNameHandler(this.BusListenerFoundAdvertisedName);
            this.busListener.ListenerRegistered += new BusListenerListenerRegisteredHandler(this.BusListenerListenerRegistered);
            this.busListener.ListenerUnregistered += new BusListenerListenerUnregisteredHandler(this.BusListenerListenerUnregistered);
            this.busListener.LostAdvertisedName += new BusListenerLostAdvertisedNameHandler(this.BusListenerLostAdvertisedName);
            this.busListener.NameOwnerChanged += new BusListenerNameOwnerChangedHandler(this.BusListenerNameOwnerChanged);

            // Create Session Listener and register signal handlers 
            this.sessionListener = new SessionListener(busAtt);
            this.sessionListener.SessionLost += new SessionListenerSessionLostHandler(this.SessionListenerSessionLost);
            this.sessionListener.SessionMemberAdded += new SessionListenerSessionMemberAddedHandler(this.SessionListenerSessionMemberAdded);
            this.sessionListener.SessionMemberRemoved += new SessionListenerSessionMemberRemovedHandler(this.SessionListenerSessionMemberRemoved);

            busAtt.RegisterBusListener(this.busListener);
        }

        /// <summary>
        /// Return the bus listener for 'mbl'
        /// </summary>
        /// <param name="mbl">the bus listener for the program.</param>
        /// <returns>session listener stored in mbl</returns>
        public static implicit operator BusListener(ClientBusListener mbl)
        {
            return mbl.busListener;
        }

        /// <summary>
        /// Return the sesssion listener for 'mbl'.
        /// </summary>
        /// <param name="mbl">the bus listener for the program.</param>
        /// <returns>session listener stored in mbl</returns>
        public static implicit operator SessionListener(ClientBusListener mbl)
        {
            return mbl.sessionListener;
        }

        /// <summary>
        /// Called when a BusAttachment this listener is registered with is has become disconnected 
        /// from the bus.
        /// </summary>
        public void BusListenerBusDisconnected()
        {
        }

        /// <summary>
        /// Called when a BusAttachment this listener is registered with is stopping.
        /// </summary>
        public void BusListenerBusStopping()
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
            this.mutex.WaitOne();
            bool ret = this.foundAvertisedName;
            this.foundAvertisedName = true;
            this.mutex.ReleaseMutex();
            if (ret)
            {
                this.DebugPrint("Will not form a session with(name=" + name + ", prefix=" + namePrefix +
                    ") because we already joined a session.");
                return;
            }

            this.DebugPrint("FoundAdvertisedName(name=" + name + ", prefix=" + namePrefix + ")");
            this.stressOp.DiscoveredName = name;
            this.foundName.Set();
        }

        /// <summary>
        /// Called by the bus when the listener is registered.
        /// </summary>
        /// <param name="bus">The bus the listener is registered with.</param>
        public void BusListenerListenerRegistered(BusAttachment bus)
        {
        }

        /// <summary>
        /// Called by the bus when the listener is unregistered.
        /// </summary>
        public void BusListenerListenerUnregistered()
        {
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
        /// Called by the bus when ownership of any well-known name changes.
        /// </summary>
        /// <param name="busName">the well-known name that has changed.</param>
        /// <param name="prevOwner">The unique name that previously owned the name or NULL if 
        /// there was no previous owner.</param>
        /// <param name="newOwner">The unique name that now owns the name or NULL if the there 
        /// is no new owner.</param>
        public void BusListenerNameOwnerChanged(string busName, string prevOwner, string newOwner)
        {
            if (string.Empty != newOwner && ServiceName == busName)
            {
                this.DebugPrint("NameOwnerChanged: name=<" + busName + ">, oldOwner=<" + prevOwner + ">, newOwner=<" + newOwner + ">");
            }
        }

        /// <summary>
        /// Called by the bus when an existing session becomes disconnected.
        /// </summary>
        /// <param name="sessionID">Id of session that was lost.</param>
        public void SessionListenerSessionLost(uint sessionID)
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
        /// Called by the bus when a member of a multipoint session is removed.
        /// </summary>
        /// <param name="sessionID">Id of session whose member(s) changed.</param>
        /// <param name="uniqueName">Unique name of member who was removed.</param>
        public void SessionListenerSessionMemberRemoved(uint sessionID, string uniqueName)
        {
        }

        /// <summary>
        /// Print to the output window of the debugger
        /// </summary>
        /// <param name="msg">Message to print</param>
        public void DebugPrint(string msg)
        {
            System.Diagnostics.Debug.WriteLine(this.stressOp.TaskId + " : " + msg);
        }
    }
}
