//-----------------------------------------------------------------------
// <copyright file="BasicServiceBusObject.cs" company="AllSeen Alliance.">
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

namespace Basic_Service_Bus_Object
{
    using AllJoyn;
    using Basic_CSharp_;
    using Service_Globals;

    /// <summary>
    /// Bus Object which implements the 'cat' interface and handles its method calls
    /// </summary>
    public class BasicServiceBusObject
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="BasicServiceBusObject" /> class
        /// </summary>
        /// <param name="busAtt">object responsible for connecting to and optionally managing a message 
        /// bus</param>
        public BasicServiceBusObject(BusAttachment busAtt)
        {
            this.BusObject = new BusObject(busAtt, BasicServiceGlobals.ServicePath, false);

            // Create 'cat' interface
            InterfaceDescription[] interfaceDescription = new InterfaceDescription[1];
            busAtt.CreateInterface(BasicServiceGlobals.InterfaceName, interfaceDescription, false);
            interfaceDescription[0].AddMethod("cat", "ss", "s", "inStr1,inStr2,outStr", (byte)0, string.Empty);
            interfaceDescription[0].Activate();

            // Create and register the 'cat' method handlers
            this.BusObject.AddInterface(interfaceDescription[0]);
            InterfaceMember catMember = interfaceDescription[0].GetMember("cat");
            MessageReceiver catReceiver = new MessageReceiver(busAtt);
            catReceiver.MethodHandler += new MessageReceiverMethodHandler(this.CatMethodHandler);
            this.BusObject.AddMethodHandler(catMember, catReceiver);

            App.OutputLine("Created 'cat' interface and registered method handlers.");

            busAtt.RegisterBusObject(this.BusObject);
        }

        /// <summary>
        /// Gets or sets alljoyn bus object which implements 'cat' interface
        /// </summary>
        public BusObject BusObject { get; set; }

        /// <summary>
        /// concatenates two strings sent by the client and returns the result
        /// </summary>
        /// <param name="member">Member of the interface which message contains</param>
        /// <param name="message">arguments sent by the client</param>
        public void CatMethodHandler(InterfaceMember member, Message message)
        {
            MsgArg msg = message.GetArg(0);
            MsgArg msg2 = message.GetArg(1);
            string val = msg.Value as string;
            string val2 = msg2.Value as string;
            App.OutputLine("//////////////////////////////////////////////////////////////////");
            App.OutputLine("Cat Method was called by '" + message.Sender + "' given '" + val +
                            "' and '" + val2 + "' as arguments.");
            string ret = val + val2;
            object[] obj = { ret };
            MsgArg msgReply = new MsgArg("s", obj);
            this.BusObject.MethodReply(message, new MsgArg[] { msgReply });
            App.OutputLine("The value '" + ret + "' was returned to '" + message.Sender + "'.");
        }
    }
}