//-----------------------------------------------------------------------
// <copyright file="SecureBusObject.cs" company="AllSeen Alliance.">
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

namespace Secure.Common
{
    using System;
    using AllJoyn;

    /// <summary>
    /// This class is used by the service to handle the interface with clients.
    /// </summary>
    public class SecureBusObject
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="SecureBusObject" /> class.
        /// </summary>
        /// <param name="busAtt">Object responsible for connecting to and optionally managing a
        /// message bus.</param>
        /// <param name="iface">The interface used by the service that is implemented by this
        /// class.</param>
        public SecureBusObject(BusAttachment busAtt, InterfaceDescription[] iface)
        {
            this.BusObject = new BusObject(busAtt, App.ServicePath, false);
            this.BusObject.AddInterface(iface[0]);

            InterfaceMember pingMember = iface[0].GetMember("Ping");
            MessageReceiver pingReceiver = new MessageReceiver(busAtt);

            pingReceiver.MethodHandler += new MessageReceiverMethodHandler(this.PingMethodHandler);
            BusObject.AddMethodHandler(pingMember, pingReceiver);
            busAtt.RegisterBusObject(this.BusObject);
        }

        /// <summary>
        /// Gets or sets alljoyn bus object which implements 'ping' interface.
        /// </summary>
        public BusObject BusObject { get; set; }

        /// <summary>
        /// Responds to a ping request.
        /// </summary>
        /// <param name="member">The parameter is not used.</param>
        /// <param name="message">arguments sent by the client</param>
        private void PingMethodHandler(InterfaceMember member, Message message)
        {
            string output = message.GetArg(0).Value as string;

            App.OutputLine(string.Format("Ping : {0}", output));
            App.OutputLine(string.Format("Reply : {0}", output));

            object[] args = { output };

            MsgArg msgReply = new MsgArg("s", args);
            this.BusObject.MethodReply(message, new MsgArg[] { msgReply });
        }
    }
}
