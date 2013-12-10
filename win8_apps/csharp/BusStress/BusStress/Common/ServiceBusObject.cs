//-----------------------------------------------------------------------
// <copyright file="ServiceBusObject.cs" company="AllSeen Alliance.">
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
    using System.Threading.Tasks;
    using AllJoyn;

    /// <summary>
    /// Service bus object which implements the 'cat' interface and handles 'cat' method calls
    /// </summary>
    public class ServiceBusObject
    {
        /// <summary>
        /// Relative path for this bus object
        /// </summary>
        private const string ServicePath = "/sample";

        /// <summary>
        /// Interface name of interface containing 'cat' method
        /// </summary>
        private const string InterfaceName = "org.alljoyn.Bus.test.bastress";

        /// <summary>
        /// Bus object which implements the 'cat' interface on the alljoyn message bus
        /// </summary>
        private BusObject busObject;

        /// <summary>
        /// Reference to the stress operation using this bus object
        /// </summary>
        private StressOperation stressOp;

        /// <summary>
        /// Initializes a new instance of the <see cref="ServiceBusObject"/> class.
        /// </summary>
        /// <param name="busAtt">Message bus for the stress operation using this</param>
        /// <param name="op">Stress operation using this bus object</param>
        public ServiceBusObject(BusAttachment busAtt, StressOperation op)
        {
            this.stressOp = op;
            this.busObject = new BusObject(busAtt, ServicePath, false);

            // Implement the 'cat' interface
            InterfaceDescription[] intfDescription = new InterfaceDescription[1];
            busAtt.CreateInterface(InterfaceName, intfDescription, false);
            intfDescription[0].AddMethod("cat", "ss", "s", "inStr1,inStr2,outStr", (byte)0, string.Empty);
            intfDescription[0].Activate();
            this.busObject.AddInterface(intfDescription[0]);

            // Register 'cat' method handler
            InterfaceMember catMethod = intfDescription[0].GetMethod("cat");
            MessageReceiver catReceiver = new MessageReceiver(busAtt);
            catReceiver.MethodHandler += new MessageReceiverMethodHandler(this.Cat);
            this.busObject.AddMethodHandler(catMethod, catReceiver);

            busAtt.RegisterBusObject(this.busObject);
        }

        /// <summary>
        /// Return the alljoyn bus object attached to 'bo' object
        /// </summary>
        /// <param name="bo">Service bus object instance</param>
        /// <returns>Alljoyn bus object corresponding to 'bo'</returns>
        public static implicit operator BusObject(ServiceBusObject bo)
        {
            return bo.busObject;
        }

        /// <summary>
        /// Concatenate the two string arguments and return the result to the caller
        /// </summary>
        /// <param name="member">Method interface member entry.</param>
        /// <param name="message">The received method call message containing the two strings
        /// to concatenate</param>
        public void Cat(InterfaceMember member, Message message)
        {
            try
            {
                string arg1 = message.GetArg(0).Value as string;
                string arg2 = message.GetArg(1).Value as string;
                MsgArg retArg = new MsgArg("s", new object[] { arg1 + arg2 });
                this.busObject.MethodReply(message, new MsgArg[] { retArg });
                this.DebugPrint("Method Reply successful (ret=" + arg1 + arg2 + ")");
            }
            catch (Exception ex)
            {
                var errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                this.DebugPrint("Method Reply unsuccessful: " + errMsg);
            }
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
