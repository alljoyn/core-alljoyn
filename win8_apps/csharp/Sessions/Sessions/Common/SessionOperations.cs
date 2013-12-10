//-----------------------------------------------------------------------
// <copyright file="SessionOperations.cs" company="AllSeen Alliance.">
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

namespace Sessions.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices.ComTypes;
    using System.Text;
    using System.Threading.Tasks;
    using System.Windows;
    using AllJoyn;

    /// <summary>
    /// Manages all of the alljoyn operations which can be specified by the user and 
    /// tracks the current sessions, requested names and session ports currently acquired.
    /// </summary>
    public class SessionOperations
    {
        public BusAttachment Bus
        {
            get
            {
                 return busAtt;
            }
        }


        /// <summary>
        /// Connect specifications for the tcp connection
        /// </summary>
        private const string ConnectSpecs = "tcp:addr=127.0.0.1,port=9956";

        /// <summary>
        /// Primary message bus
        /// </summary>
        private BusAttachment busAtt = null;

        /// <summary>
        /// Bus object implementing the 'chat' interface and its handles
        /// </summary>
        private MyBusObject busObject = null;

        /// <summary>
        /// Handles all of the event sent over the alljoyn bus
        /// </summary>
        private MyBusListener busListener = null;

        /// <summary>
        /// Store a reference to the main page for printing purposes
        /// </summary>
        private MainPage mainPage = null;

        /// <summary>
        /// Well known names obtained by the application
        /// </summary>
        private HashSet<string> requestedNames = new HashSet<string>();

        /// <summary>
        /// Names currently being advertised by this application
        /// </summary>
        private HashSet<string> advertisedNames = new HashSet<string>();

        /// <summary>
        /// Holds information for all discovered names and their transports
        /// </summary>
        private HashSet<DiscoveryInfo> discoverySet = new HashSet<DiscoveryInfo>();
        
        /// <summary>
        /// Maps currently bound session ports with specification info for the port
        /// </summary>
        private Dictionary<uint, SessionPortInfo> sessionPortMap = new Dictionary<uint, SessionPortInfo>();

        /// <summary>
        /// Maps all sessions joined by the applications to details for the session
        /// </summary>
        private Dictionary<uint, SessionInfo> sessionMap = new Dictionary<uint, SessionInfo>();

        /// <summary>
        /// Initializes a new instance of the <see cref="SessionOperations"/> class. Creates and registers 
        /// the bus attachment, bus listener, bus object and interface for the application then establishes 
        /// the alljoyn tcp connection.
        /// </summary>
        /// <param name="mp">Main page for the application</param>
        public SessionOperations(MainPage mp)
        {
            Task task = new Task(async () =>
                {
                    try
                    {
                        this.mainPage = mp;

                        this.busAtt = new BusAttachment("Sessions", true, 4);
                        this.busObject = new MyBusObject(this.busAtt, this);
                        this.busListener = new MyBusListener(this.busAtt, this);

                        this.busAtt.Start();
                        await this.busAtt.ConnectAsync(ConnectSpecs);
                    }
                    catch (Exception ex)
                    {
                        var errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                        this.mainPage.Output("Couldn't connect to AllJoyn: " + errMsg);
                    }
                });
                task.Start();
        }

        /// <summary>
        /// Set the specified AllJoyn module to the specified level
        /// </summary>
        /// <param name="moduleName">AllJoyn module name</param>
        /// <param name="level">Debug level</param>
        public void DoDebug(string moduleName, uint level)
        {
            try
            {
                AllJoyn.Debug.SetDebugLevel(moduleName.ToUpper(), level);
            }
            catch (Exception ex)
            {
                var errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                this.mainPage.Output("AllJoyn.Debug.SetDebugLevel(" + moduleName + ", " + level + ") failed: " + errMsg);
            }
        }

        /// <summary>
        /// Request the well known name 'name' from the bus
        /// </summary>
        /// <param name="name">Well known name to request</param>
        public void DoRequestName(string name)
        {
            try
            {
                this.busAtt.RequestName(name, (uint)RequestNameType.DBUS_NAME_DO_NOT_QUEUE);
                lock (this.requestedNames)
                {
                    this.requestedNames.Add(name);
                }
            }
            catch (Exception ex)
            {
                var errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                this.mainPage.Output("BusAttachment.RequestName(" + name + ") failed: " + errMsg);
            }
        }

        /// <summary>
        /// Release well known name previously obtained through request name call
        /// </summary>
        /// <param name="name">Well Known name to release</param>
        public void DoReleaseName(string name)
        {
            try
            {
                this.busAtt.ReleaseName(name);
                lock (this.requestedNames)
                {
                    this.requestedNames.Remove(name);
                }
            }
            catch (Exception ex)
            {
                var errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                this.mainPage.Output("BusAttachment.ReleaseName(" + name + ") failed: " + errMsg);
            }
        }

        /// <summary>
        /// Bind the session port 
        /// </summary>
        /// <param name="port">Port number to bind</param>
        /// <param name="opts">Session options for binding the port</param>
        public void DoBind(uint port, SessionOpts opts)
        {
            try
            {
                ushort[] sessionPortOut = new ushort[1];
                this.busAtt.BindSessionPort((ushort)port, sessionPortOut, opts, (SessionPortListener)this.busListener);
                lock (this.sessionPortMap)
                {
                    this.sessionPortMap.Add(port, new SessionPortInfo(port, this.busAtt.UniqueName, opts));
                }
            }
            catch (Exception ex)
            {
                var errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                this.mainPage.Output("BusAttachment.BindSessionPort for port " + port + " failed: " + errMsg);
            }
        }

        /// <summary>
        /// Unbind the session port previously bound
        /// </summary>
        /// <param name="port">Bound session port number</param>
        public void DoUnbind(uint port)
        {
            try
            {
                this.busAtt.UnbindSessionPort((ushort)port);
                lock (this.sessionPortMap)
                {
                    this.sessionPortMap.Remove(port);
                }
            }
            catch (Exception ex)
            {
                var errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                this.mainPage.Output("BusAttachment.UnbindSessionPort(" + port + ") failed: " + errMsg);
            }
        }

        /// <summary>
        /// Advertise the well known name 'name' for client to discover
        /// </summary>
        /// <param name="name">Well known name to advertise</param>
        /// <param name="transport">Transport type for which to advertise the name over</param>
        public void DoAdvertise(string name, TransportMaskType transport)
        {
            try
            {
                lock (this.requestedNames)
                {
                    if (this.requestedNames.Contains(name))
                    {
                        this.busAtt.AdvertiseName(name, transport);
                        lock (this.advertisedNames)
                        {
                            this.advertisedNames.Add(name);
                        }
                    }
                    else
                    {
                        this.mainPage.Output(name + " must first be requested from the bus before it can be advertised");
                    }
                }
            }
            catch (Exception ex)
            {
                var errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                this.mainPage.Output("BusAttachment.AdvertiseName(" + name + ", " + (uint)transport + ") failed: " + errMsg);
            }
        }

        /// <summary>
        /// Cancel advertising the well known name 'name'
        /// </summary>
        /// <param name="name">Well known name currently being advertised</param>
        /// <param name="transport">Transport type which 'name' is being advertised over</param>
        public void DoCancelAdvertise(string name, TransportMaskType transport)
        {
            try
            {
                lock (this.advertisedNames)
                {
                    if (this.advertisedNames.Contains(name))
                    {
                        this.busAtt.CancelAdvertiseName(name, transport);
                        this.advertisedNames.Remove(name);
                    }
                    else
                    {
                        this.mainPage.Output(name + " is not currently being advertised");
                    }
                }
            }
            catch (Exception ex)
            {
                var errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                this.mainPage.Output("BusAttachment.CancelAdvertiseName(" + name + ", " + (uint)transport + ") failed: " + errMsg);
            }
        }

        /// <summary>
        /// Start a discovery action for namePrefix
        /// </summary>
        /// <param name="namePrefix">Name prefix of device to discover</param>
        public void DoFind(string namePrefix)
        {
            try
            {
                this.busAtt.FindAdvertisedName(namePrefix);
            }
            catch (Exception ex)
            {
                var errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                this.mainPage.Output("BusAttachment.FindAdvertisedName(" + namePrefix + ") failed: " + errMsg);
            }
        }

        /// <summary>
        /// Cancel the previously started find advertised name action of namePrefix 
        /// </summary>
        /// <param name="namePrefix">Name prefix of advertised name</param>
        public void DoCancelFind(string namePrefix)
        {
            try
            {
                this.busAtt.CancelFindAdvertisedName(namePrefix);
            }
            catch (Exception ex)
            {
                var errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                this.mainPage.Output("BusAttachment.CancelFindAdvertisedName(" + namePrefix + ") failed: " + errMsg);
            }
        }

        /// <summary>
        /// Print out a summary of the current state of the application
        /// </summary>
        public void DoList()
        {
            this.mainPage.Output("-------------------Locally Owned Names-------------------");
            this.mainPage.Output("   " + this.busAtt.UniqueName);
            lock (this.requestedNames)
            {
                foreach (string s in this.requestedNames)
                {
                    this.mainPage.Output("   " + s);
                }
            }

            this.mainPage.Output("-------------------Outgoing Advertisments----------------");
            lock (this.advertisedNames)
            {
                foreach (string s in this.advertisedNames)
                {
                    this.mainPage.Output("   " + s);
                }
            }

            this.mainPage.Output("-------------------Discovered Names----------------------");
            lock (this.discoverySet)
            {
                foreach (DiscoveryInfo d in this.discoverySet)
                {
                    this.mainPage.Output("   Peer: " + d.PeerName + " (transport=" + d.Transport.ToString() + ")");
                }
            }

            this.mainPage.Output("-------------------Bound Session Ports-------------------");
            lock (this.sessionPortMap)
            {
                foreach (KeyValuePair<uint, SessionPortInfo> pair in this.sessionPortMap)
                {
                    SessionOpts opts = pair.Value.Opts;
                    string output = string.Format(
                        "   Port: {0}, traffic={1}, isMultipoint={2}, proximity={3}, transports={4}",
                        pair.Key,
                        opts.Traffic,
                        opts.IsMultipoint,
                        opts.Proximity,
                        opts.TransportMask);
                    this.mainPage.Output(output);    
                }
            }

            this.mainPage.Output("-------------------Active sessions-----------------------");
            lock (this.sessionMap)
            {
                uint count = 0;
                foreach (KeyValuePair<uint, SessionInfo> pair in this.sessionMap)
                {
                    SessionInfo info = pair.Value;
                    SessionPortInfo sessInfo = info.PortInfo;
                    SessionOpts opts = sessInfo.Opts;
                    string details = string.Format(
                        "   #{0}: SessionId: {1}, Creator: {2}, Port: {3}, traffic={4}, isMultipoint={5}, proximity={6}, transport={7}",
                        count, 
                        pair.Key, 
                        sessInfo.SessionHost, 
                        sessInfo.SessionPort,
                        opts.Traffic,
                        opts.IsMultipoint,
                        opts.Proximity, 
                        opts.TransportMask);
                    this.mainPage.Output(details);
                    if (info.PeerNames.Count > 0)
                    {
                        string peers = "     Peers: ";
                        foreach (string s in info.PeerNames)
                        {
                            peers += s + ", ";
                        }

                        this.mainPage.Output(peers);
                    }

                    count++;
                }
            }
        }

        /// <summary>
        /// Joins a session with specified nameprefix using specs provided
        /// </summary>
        /// <param name="name">Name prefix or well-known name to join session with</param>
        /// <param name="sessionPort">Port for session</param>
        /// <param name="opts">Session Opts for session</param>
        public async void DoJoinAsync(string name, uint sessionPort, SessionOpts opts)
        {
            SessionOpts[] optsOut = new SessionOpts[1];
            JoinSessionResult joinResult = await this.busAtt.JoinSessionAsync(name, (ushort)sessionPort, (SessionListener)this.busListener, opts, optsOut, null);
            QStatus status = joinResult.Status;
            if (QStatus.ER_OK == status)
            {
                this.mainPage.Output("BusAttachment.JoinSessionAsync(" + name + ", " + sessionPort + ", ...) succeeded with id=" + joinResult.SessionId);
                lock (this.sessionMap)
                {
                    this.sessionMap.Add(joinResult.SessionId, new SessionInfo(joinResult.SessionId, new SessionPortInfo(sessionPort, name, opts)));
                }
            }
            else
            {
                this.mainPage.Output("BusAttachment.JoinSessionAsync(" + name + ", " + sessionPort + ", ...) failed: " + status.ToString());
            }
        }

        /// <summary>
        /// Leave the session with specified session id
        /// </summary>
        /// <param name="sessionId">Session Id of session to leave</param>
        public void DoLeave(uint sessionId)
        {
            lock (this.sessionMap)
            {
                if (this.sessionMap.ContainsKey(sessionId))
                {
                    try
                    {
                        this.busAtt.LeaveSession(sessionId);
                    }
                    catch (Exception ex)
                    {
                        var errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                        this.mainPage.Output("Leave Session was unsuccessful: " + errMsg);
                    }

                    this.sessionMap.Remove(sessionId);
                }
                else
                {
                    this.mainPage.Output("Session Id doesn't exist in currently joined sessions");
                }
            }
        }

        /// <summary>
        /// Set the link timeout for the specified session
        /// </summary>
        /// <param name="sessionId">Session Id for which to set link timeout</param>
        /// <param name="timeOut">Timeout in seconds</param>
        public async void DoSetLinkTimeoutAsync(uint sessionId, uint timeOut)
        {
            bool found = false;
            lock (this.sessionMap)
            {
                found = this.sessionMap.ContainsKey(sessionId);
            }
            
            if (found)
            {
                SetLinkTimeoutResult timeOutResult = await this.busAtt.SetLinkTimeoutAsync(sessionId, timeOut, null);
                if (QStatus.ER_OK == timeOutResult.Status)
                {
                    this.mainPage.Output("Set Timeout Async was successful (timeout=" + timeOutResult.Timeout + " secs)");
                }
                else
                {
                    this.mainPage.Output("Set Timeout Async was unsuccessful: " + timeOutResult.Status.ToString());
                }
            }
            else
            {
                this.mainPage.Output("Session Id doesn't exist in currently joined sessions");
            }
        }

        /// <summary>
        /// If the session id is valid (a.k.a session exists) then send the 'Chat' signal
        /// </summary>
        /// <param name="sessionId">Session id to send the 'Chat' signal over</param>
        /// <param name="msg">Message to send</param>
        /// <param name="flags">Flags to use for the signal</param>
        public void SendChatSignal(uint sessionId, string msg, byte flags)
        {
            lock (this.sessionMap)
            {
                if (this.sessionMap.ContainsKey(sessionId))
                {
                    this.busObject.SendChatSignal(sessionId, msg, flags, 10);
                }
                else
                {
                    this.mainPage.Output("Session Id doesn't exist in currently joined sessions");
                }
            }
        }

        /// <summary>
        /// Send a signal without a session
        /// </summary>
        /// <param name="msg">Message to send</param>
        /// <param name="flags">Flags to use for the signal</param>
        public void SendSessionlessChatSignal(string msg, byte flags)
        {
            lock (this.sessionMap)
            {
                this.busObject.SendChatSignal(0, msg, flags, 0);
            }
        }

        /// <summary>
        /// Set a rule to bus object
        /// </summary>
        /// <param name="rule">rule to use</param>
        public void AddRule(string rule)
        {
            lock (this.sessionMap)
            {
                this.busObject.AddRule(rule);
            }
        }

        /// <summary>
        /// Set the chat echo switch to value specified by user
        /// </summary>
        /// <param name="isChatEchoOn">True if user wishes to turn chat echo on, else false</param>
        public void SetChatEcho(bool isChatEchoOn)
        {
            this.busObject.ChatEcho = isChatEchoOn;
        }

        /// <summary>
        /// Exit the application by disconnecting and stopping the message bus
        /// </summary>
        public void Exit()
        {
            //// TODO: Exit the application
            //// Not sure if this is necessary in win8 because there's no explicit closing of an app
            this.mainPage.Output("Not implemented.");
        }

        /// <summary>
        /// Print out the help menu to the user
        /// </summary>
        public void PrintHelp()
        {
            string helpMsg = string.Empty +
                "debug <module_name> <level>\t\t\t\t\t\t- Set debug level for a module\n" +
                "requestname <name>\t\t\t\t\t\t\t- Request a well-known name\n" +
                "releasename <name>\t\t\t\t\t\t\t\t- Release a well-known name\n" +
                "bind <port> [traffic] [isMultipoint] [proximity] [transports]\t\t\t- Bind a session port\n" +
                "unbind <port>\t\t\t\t\t\t\t\t- Unbind a session port\n" +
                "advertise <name> [transports]\t\t\t\t\t\t- Advertise a name\n" +
                "canceladvertise <name> [transports]\t\t\t\t\t\t- Cancel an advertisement\n" +
                "find <name_prefix>\t\t\t\t\t\t\t\t- Discover names that begin with prefix\n" +
                "cancelfind <name_prefix>\t\t\t\t\t\t\t- Cancel discovering names that begins with prefix\n" +
                "list\t\t\t\t\t\t\t\t\t\t- List port bindings, discovered names and active sessions\n" +
                "join <name> <port> [traffic] [isMultipoint] [proximity] [transports]\t\t- Join a session\n" +
                "leave <sessionId>\t\t\t\t\t\t\t\t- Leave a session\n" +
                "chat <sessionId> <msg>\t\t\t\t\t\t\t- Send a message over a given session\n" +
                "cchat <sessionId> <msg>\t\t\t\t\t\t\t- Send a message over a given session with compression\n" +
                "schat <msg>\t\t\t\t\t\t\t\t\t- Send a sessionless message\n" +
                "addmatch <rule>\t\t\t\t\t\t\t\t- Add a DBUS rule\n" +
                "timeout <sessionId> <linkTimeout>\t\t\t\t\t\t- Set link timeout for a session\n" +
                "chatecho [on|off]\t\t\t\t\t\t\t\t- Turn on/off chat messages\n" +
                "exit\t\t\t\t\t\t\t\t\t\t- Exit this program\n\n" +
                "SessionIds can be specified by value or by #<idx> where <idx> is the session index printed with \"list\" command";
            this.mainPage.Output(helpMsg);
        }

        /// <summary>
        /// Add the discovered name to the set of discovered names
        /// </summary>
        /// <param name="name">Discovered name</param>
        /// <param name="transport">Transport used for advertised name</param>
        public void FoundAdvertisedName(string name, TransportMaskType transport)
        {
            lock (this.discoverySet)
            {
                this.discoverySet.Add(new DiscoveryInfo(name, transport));
            }
        }

        /// <summary>
        /// Remove the lost name from the discovered names set
        /// </summary>
        /// <param name="name">Lost well-known name</param>
        public void LostAdvertisedName(string name)
        {
            lock (this.discoverySet)
            {
                foreach (DiscoveryInfo d in this.discoverySet)
                {
                    if (d.PeerName == name)
                    {
                        this.discoverySet.Remove(d);
                    }
                }
            }
        }

        /// <summary>
        /// Return true if the session is being requested over a bound port
        /// </summary>
        /// <param name="port">Port Number used for the session</param>
        /// <returns>True if session port has been bound, otherwise false</returns>
        public bool AcceptSessionJoiner(ushort port)
        {
            lock (this.sessionPortMap)
            {
                return this.sessionPortMap.ContainsKey(port);
            }
        }

        /// <summary>
        /// Add the session details to the session port and session maps
        /// </summary>
        /// <param name="port">Port used by the session</param>
        /// <param name="sessionId">Session Id for the joined session</param>
        /// <param name="joiner">Device/Endpoint joining the session</param>
        public void SessionJoined(ushort port, uint sessionId, string joiner)
        {
            lock (this.sessionPortMap)
            {
                if (this.sessionPortMap.ContainsKey(port))
                {
                    try
                    {
                        this.busAtt.SetSessionListener(sessionId, (SessionListener)this.busListener);
                    }
                    catch (Exception ex)
                    {
                        var errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                        this.mainPage.Output("BusAttachment.SetSessionListener(" + sessionId + ", ...) failed: " + errMsg);
                    }

                    lock (this.sessionMap)
                    {
                        if (!this.sessionMap.ContainsKey(sessionId))
                        {
                            this.sessionMap.Add(sessionId, new SessionInfo(sessionId, this.sessionPortMap[port]));
                        }

                        this.sessionMap[sessionId].AddPeer(joiner);
                    }

                    this.mainPage.Output("SessionJoined with " + joiner + " (sessionId=" + sessionId + ")");
                }
                else
                {
                    this.mainPage.Output("Leaving unexpected session " + sessionId + " with " + joiner);
                    try
                    {
                        this.busAtt.LeaveSession(sessionId);
                    }
                    catch (Exception ex)
                    {
                        var errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                        this.mainPage.Output("BusAttachment.LeaveSession(" + sessionId + ") failed: " + errMsg);
                    }
                }
            }
        }

        /// <summary>
        /// Remove the lost session id from current session ids
        /// </summary>
        /// <param name="sessionId">Lost session id</param>
        /// <returns>True if apart of session, otherwise false</returns>
        public bool SessionLost(uint sessionId)
        {
            lock (this.sessionMap)
            {
                if (this.sessionMap.ContainsKey(sessionId))
                {
                    this.sessionMap.Remove(sessionId);
                    return true;
                } 
                else 
                {
                    return false;
                }
            }
        }

        /// <summary>
        /// Add the new session member to the list of peer names for that session
        /// </summary>
        /// <param name="sessionId">Session id for session where peer was added</param>
        /// <param name="uniqueName">Name of added member</param>
        public void SessionMemberAdded(uint sessionId, string uniqueName)
        {
            lock (this.sessionMap)
            {
                if (this.sessionMap.ContainsKey(sessionId))
                {
                    this.sessionMap[sessionId].AddPeer(uniqueName);
                }
            }
        }

        /// <summary>
        /// Remove the lost session member from the list of peer names for that session
        /// </summary>
        /// <param name="sessionId">Session id for session where peer was lost</param>
        /// <param name="uniqueName">Name of removed memeber</param>
        public void SessionMemberRemoved(uint sessionId, string uniqueName)
        {
            lock (this.sessionMap)
            {
                if (this.sessionMap.ContainsKey(sessionId))
                {
                    this.sessionMap[sessionId].RemovePeer(uniqueName);
                }
            }
        }

        /// <summary>
        /// Print out msg to the UI
        /// </summary>
        /// <param name="msg">Message to print</param>
        public void Output(string msg)
        {
            this.mainPage.Output(msg);
        }

        /// <summary>
        /// Retreives the session id using the session table index
        /// </summary>
        /// <param name="index">Index in the session table</param>
        /// <returns>Session id from sessionMap[i]</returns>
        public uint GetSessionId(uint index)
        {
            lock (this.sessionMap)
            {
                if (index < this.sessionMap.Count)
                {
                    uint count = index;
                    foreach (KeyValuePair<uint, SessionInfo> pair in this.sessionMap)
                    {
                        if (0 == count)
                        {
                            return pair.Key;
                        }

                        count--;
                    }
                }

                return 0;
            }
        }

        /// <summary>
        /// Encapsulates the details of a discovered well-known name
        /// </summary>
        public class DiscoveryInfo
        {
            /// <summary>
            /// Initializes a new instance of the <see cref="DiscoveryInfo"/> class
            /// </summary>
            /// <param name="peer">Peer's name that was discovered</param>
            /// <param name="transport">Transport which the name was discovered on</param>
            public DiscoveryInfo(string peer, TransportMaskType transport)
            {
                this.PeerName = peer;
                this.Transport = transport;
            }

            /// <summary>
            /// Gets or sets the discovered peer name
            /// </summary>
            public string PeerName { get; set; }

            /// <summary>
            /// Gets or sets the transport mask type for the discovered peer
            /// </summary>
            public TransportMaskType Transport { get; set; }
        }

        /// <summary>
        /// Encapsulates information about a session
        /// </summary>
        public class SessionInfo
        {
            /// <summary>
            /// Initializes a new instance of the <see cref="SessionInfo"/> class
            /// </summary>
            /// <param name="sId">Session id for this session</param>
            /// <param name="prtInfo">Port which this session uses</param>
            public SessionInfo(uint sId, SessionPortInfo prtInfo)
            {
                this.SessionId = sId;
                this.PortInfo = prtInfo;
                this.PeerNames = new SortedSet<string>();
            }

            /// <summary>
            /// Gets or sets the session id for corresponding session
            /// </summary>
            public uint SessionId { get; set; }

            /// <summary>
            /// Gets or sets the port info for this session
            /// </summary>
            public SessionPortInfo PortInfo { get; set; }

            /// <summary>
            /// Gets or sets the set of peers connected to this session
            /// </summary>
            public SortedSet<string> PeerNames { get; set; }

            /// <summary>
            /// Add a new peer to the session
            /// </summary>
            /// <param name="peer">Peer added to the session</param>
            public void AddPeer(string peer)
            {
                lock (this.PeerNames)
                {
                    if (!this.PeerNames.Contains(peer))
                    {
                        this.PeerNames.Add(peer);
                    }
                }
            }

            /// <summary>
            /// Remove peer from the session
            /// </summary>
            /// <param name="peer">Peer removed from the session</param>
            public void RemovePeer(string peer)
            {
                lock (this.PeerNames)
                {
                    if (this.PeerNames.Contains(peer))
                    {
                        this.PeerNames.Remove(peer);
                    }
                }
            }
        }

        /// <summary>
        /// Encapsulates information about a session port
        /// </summary>
        public class SessionPortInfo
        {
            /// <summary>
            /// Initializes a new instance of the <see cref="SessionPortInfo"/> class
            /// </summary>
            /// <param name="sessPort">Port Number</param>
            /// <param name="host">Host for this session port</param>
            /// <param name="sessOpts">Session options this port uses</param>
            public SessionPortInfo(uint sessPort, string host, SessionOpts sessOpts)
            {
                this.SessionPort = sessPort;
                this.SessionHost = host;
                this.Opts = sessOpts;
            }

            /// <summary>
            /// Gets or sets the session port
            /// </summary>
            public uint SessionPort { get; set; }

            /// <summary>
            /// Gets or sets the session host
            /// </summary>
            public string SessionHost { get; set; }

            /// <summary>
            /// Gets or sets the session opts
            /// </summary>
            public SessionOpts Opts { get; set; }
        }
    }
}