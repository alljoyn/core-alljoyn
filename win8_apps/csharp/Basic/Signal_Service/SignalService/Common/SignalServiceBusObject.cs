//-----------------------------------------------------------------------
// <copyright file="SignalServiceBusObject.cs" company="AllSeen Alliance.">
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
    using System;
    using AllJoyn;
    using Signal_Service;
    using Signal_Service_Globals;

    /// <summary>
    /// Bus Object which implements the 'cat' interface and handles its method calls
    /// </summary>
    public class SignalServiceBusObject
    {
        /// <summary>
        /// Value of the 'name' property
        /// </summary>
        private string name;

        /// <summary>
        /// Interface member corresponding to the 
        /// </summary>
        private InterfaceMember signalMember;

        /// <summary>
        /// Initializes a new instance of the <see cref="SignalServiceBusObject" /> class
        /// </summary>
        /// <param name="busAtt">object responsible for connecting to and optionally managing a message 
        /// bus</param>
        public SignalServiceBusObject(BusAttachment busAtt)
        {
            this.BusObject = new BusObject(busAtt, SignalServiceGlobals.ServicePath, false);
            this.name = string.Empty;

            InterfaceDescription[] interfaceDescription = new InterfaceDescription[1];
            busAtt.CreateInterface(SignalServiceGlobals.InterfaceName, interfaceDescription, false);
            interfaceDescription[0].AddSignal("nameChanged", "s", "newName", (byte)0, string.Empty);
            interfaceDescription[0].AddProperty("name", "s", (byte)PropAccessType.PROP_ACCESS_RW);
            interfaceDescription[0].Activate();
            this.BusObject.AddInterface(interfaceDescription[0]);
            App.OutputLine("Interface created and added to the Bus Object.");

            this.signalMember = interfaceDescription[0].GetSignal("nameChanged");
            this.BusObject.Set += this.SetHandler;
            busAtt.RegisterBusObject(this.BusObject);
            App.OutputLine("Bus Object and property set handlers Registered.");
        }

        /// <summary>
        /// Gets or sets alljoyn bus object which implements 'cat' interface
        /// </summary>
        public BusObject BusObject { get; set; }

        /// <summary>
        /// Event handler which is called when a remote object tries to set the property 'name'
        /// in this service's interface.
        /// </summary>
        /// <param name="interfaceName">name of the interface containing property 'name'.</param>
        /// <param name="propertyName">name of the property whos value to set.</param>
        /// <param name="msg">contains the new value for which to set the property value.</param>
        /// <returns>ER_OK if the property value change and signal were executed sucessfully,
        /// else returns ER_BUS_BAD_SEND_PARAMETER.</returns>
        public QStatus SetHandler(string interfaceName, string propertyName, MsgArg msg)
        {
            QStatus status = QStatus.ER_OK;
            if (propertyName == "name")
            {
                string newName = msg.Value.ToString();
                try
                {
                    App.OutputLine("///////////////////////////////////////////////////////////////////////");
                    App.OutputLine("Name Property Changed (newName=" + newName + "\toldName=" + this.name + ").");
                    this.name = newName;
                    object[] obj = { newName };
                    MsgArg msgReply = new MsgArg("s", obj);
                    this.BusObject.Signal(string.Empty, 0, this.signalMember, new MsgArg[] { msgReply }, 0, (byte)AllJoynFlagType.ALLJOYN_FLAG_GLOBAL_BROADCAST);
                    status = QStatus.ER_OK;
                }
                catch (Exception ex)
                {
                    App.OutputLine("The Name Change signal was not able to send.");
                    System.Diagnostics.Debug.WriteLine("Exception:");
                    System.Diagnostics.Debug.WriteLine(ex.ToString());
                    status = QStatus.ER_BUS_BAD_SEND_PARAMETER;
                }
            }

            return status;
        }
    }
}