// ****************************************************************************
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
// ******************************************************************************

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Basic_Client_Bus_Listener
{
    /// <summary>
    /// This is the bus listener specific to the basic client which handles its events 
    /// </summary>
    public class BasicClientBusListener
    {
        /// <summary>
        /// Gets or sets the active sessionID's which this listener belongs to
        /// </summary>
        private static List<uint> SessionidList { get; set; }

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
        /// Initializes a new instance of the <see cref="BasicClientBusListener" /> class
        /// </summary>
        /// <param name="bus">object responsible for connecting to and optionally managing a message 
        /// bus</param>
        /// <param name="foundNameEvent">event to set when the well-known name being queried 
        /// is found</param>
        public BasicClientBusListener(BusAttachment bus, AutoResetEvent foundNameEvent)
        {
            SessionidList = new List<uint>();
            busAtt = bus;
            foundName = foundNameEvent;

            busListener = new BusListener(bus);
            busListener.BusDisconnected += new BusListenerBusDisconnectedHandler(BusListenerBusDisconnected);
            busListener.BusStopping += new BusListenerBusStoppingHandler(BusListenerBusStopping);
            busListener.FoundAdvertisedName += new BusListenerFoundAdvertisedNameHandler(BusListenerFoundAdvertisedName);
            busListener.ListenerRegistered += new BusListenerListenerRegisteredHandler(BusListenerListenerRegistered);
            busListener.ListenerUnregistered += new BusListenerListenerUnregisteredHandler(BusListenerListenerUnregistered);
            busListener.LostAdvertisedName += new BusListenerLostAdvertisedNameHandler(BusListenerLostAdvertisedName);
            busListener.NameOwnerChanged += new BusListenerNameOwnerChangedHandler(BusListenerNameOwnerChanged);

            sessionListener = new SessionListener(bus);
            sessionListener.SessionLost += new SessionListenerSessionLostHandler(SessionListenerSessionLost);
            sessionListener.SessionMemberAdded += new SessionListenerSessionMemberAddedHandler(SessionListenerSessionMemberAdded);
            sessionListener.SessionMemberRemoved += new SessionListenerSessionMemberRemovedHandler(SessionListenerSessionMemberRemoved);
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
            if (name.CompareTo(BasicClientGlobals.WellKnownServiceName) == 0)
            {
                System.Diagnostics.Debug.WriteLine("Name=" + name + "\tPrefix=" + namePrefix);
                foundName.Set();
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
        /// Called by the bus when a member of a multipoint session is removed.
        /// </summary>
        /// <param name="sessionID">Id of session whose member(s) changed.</param>
        /// <param name="uniqueName">Unique name of member who was removed.</param>
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
            lock (SessionidList)
            {
                if (!SessionidList.Contains(sessionID))
                {
                    SessionidList.Add(sessionID);
                }
            }
        }

        /// <summary>
        /// Called by the bus when an existing session becomes disconnected.
        /// </summary>
        /// <param name="sessionID">Id of session that was lost.</param>
        public void SessionListenerSessionLost(uint sessionID)
        {
            lock (SessionidList)
            {
                if (SessionidList.Contains(sessionID))
                {
                    SessionidList.Remove(sessionID);
                }
            }
        }

        /// <summary>
        /// return the sesssion listener for 'msl'
        /// </summary>
        /// <param name="msl">the bus listener for the program</param>
        /// <returns>session listener stored in msl</returns>
        static public implicit operator SessionListener(BasicClientBusListener msl)
        {
            return msl.sessionListener;
        }

        /// <summary>
        /// returns the buslistener for 'msl'
        /// </summary>
        /// <param name="msl">the bus listener for the program</param>
        /// <returns></returns>
        static public implicit operator BusListener(BasicClientBusListener msl)
        {
            return msl.busListener;
        }
    }

    /// <summary>
    /// Container for the argument past to the dispatcher which prints to UI
    /// </summary>
    public class ArgumentObject
    {
        /// <summary>
        /// The text that will be displayed in the text box of the UI
        /// </summary>
        private string text;

        /// <summary>
        /// Reference to the textbox in the UI
        /// </summary>
        private TextBox textBox;

        /// <summary>
        /// Gets or sets the string text
        /// </summary>
        public String Text
        {
            get { return text; }
            set { text = value; }
        }

        /// <summary>
        /// Gets or sets textBox
        /// </summary>
        public TextBox TextBox
        {
            get { return textBox; }
            set { textBox = value; }
        }

        /// <summary>
        /// used by the dispacter when printing 'txt' to the UI's textbox
        /// </summary>
        public void OnDispatched()
        {
            TextBox.Text += Text;
        }
    }
}
