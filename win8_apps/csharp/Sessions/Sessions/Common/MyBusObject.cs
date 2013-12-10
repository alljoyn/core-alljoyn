//-----------------------------------------------------------------------
// <copyright file="MyBusObject.cs" company="AllSeen Alliance.">
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
    using System.Text;
    using System.Threading.Tasks;
    using AllJoyn;

    /// <summary>
    /// Bus object which implements the 'chat' interface
    /// </summary>
    public class MyBusObject
    {
        /// <summary>
        /// Sessions interface name
        /// </summary>
        private const string SessionInterfaceName = "org.alljoyn.bus.test.sessions";

        /// <summary>
        /// Path to the Session bus object interface
        /// </summary>
        private const string BusObjectPath = "/sessions";

        /// <summary>
        /// AllJoyn bus object implementing and handling 'Chat' interface
        /// </summary>
        private BusObject busObject;

        /// <summary>
        /// Chat signal member of the 'Chat' interface
        /// </summary>
        private InterfaceMember chatSignal;

        /// <summary>
        /// Reference to the applications session operations object
        /// </summary>
        private SessionOperations sessionOps;

        /// <summary>
        /// Initializes a new instance of the <see cref="MyBusObject"/> class
        /// </summary>
        /// <param name="busAtt">Message bus for the sessions app</param>
        /// <param name="ops">Session Operations object for this application</param>
        public MyBusObject(BusAttachment busAtt, SessionOperations ops)
        {
            this.busObject = new BusObject(busAtt, BusObjectPath, false);
            this.sessionOps = ops;
            this.ChatEcho = true;

            // Implement the 'Chat' interface
            InterfaceDescription[] intfDescription = new InterfaceDescription[1];
            busAtt.CreateInterface(SessionInterfaceName, intfDescription, false);
            intfDescription[0].AddSignal("Chat", "s", "str", (byte)0, string.Empty);
            intfDescription[0].Activate();
            this.chatSignal = intfDescription[0].GetSignal("Chat");
            this.busObject.AddInterface(intfDescription[0]);

            // Register chat signal handler 
            MessageReceiver signalReceiver = new MessageReceiver(busAtt);
            signalReceiver.SignalHandler += new MessageReceiverSignalHandler(this.ChatSignalHandler);
            busAtt.RegisterSignalHandler(signalReceiver, this.chatSignal, string.Empty);

            busAtt.RegisterBusObject(this.busObject);
        }

        /// <summary>
        /// Gets or sets a value indicating whether the 'Chat' messages are printed to the UI
        /// </summary>
        public bool ChatEcho { get; set; } 

        /// <summary>
        /// Sends a 'Chat' signal using the specified parameters
        /// </summary>
        /// <param name="sessionId">A unique SessionId used to contain the signal</param>
        /// <param name="msg">Message that will be sent over the 'Chat' signal</param>
        /// <param name="flags">Flags for the signal</param>
        /// <param name="ttl">Time To Live for the signal</param>
        public void SendChatSignal(uint sessionId, string msg, byte flags, ushort ttl)
        {
            try
            {
                MsgArg msgArg = new MsgArg("s", new object[] { msg });
                this.busObject.Signal(string.Empty, sessionId, this.chatSignal, new MsgArg[] { msgArg }, ttl, flags);
            }
            catch (Exception ex)
            {
                var errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                this.sessionOps.Output("Sending Chat Signal failed: " + errMsg);
            }
        }


        /// <summary>
        /// Sends a 'Chat' signal using the specified parameters
        /// </summary>
        /// <param name="filter">filter to use </param>
        public void AddRule(string filter)
        {
            try
            {
                this.busObject.Bus.AddMatch(filter);
            }
            catch (Exception ex)
            {
                QStatus status = AllJoynException.GetErrorCode(ex.HResult);
                string errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                this.sessionOps.Output("adding a rule failed: " + errMsg);
            }
        }


        /// <summary>
        /// Called when another session application sends a chat signal containing a message which
        /// is printed out to the user.
        /// </summary>
        /// <param name="member">Method or signal interface member entry.</param>
        /// <param name="srcPath">Object path of signal emitter.</param>
        /// <param name="message">The received message.</param>
        private void ChatSignalHandler(InterfaceMember member, string srcPath, Message message)
        {
            if (this.ChatEcho)
            {
                string output = string.Format("RX message from {0}[{1}]: {2}", message.Sender, message.SessionId, message.GetArg(0).Value.ToString());

                this.sessionOps.Output(output);
            }
        }
    }
}
