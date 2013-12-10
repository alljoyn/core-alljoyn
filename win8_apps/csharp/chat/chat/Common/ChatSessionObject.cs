// ****************************************************************************
// <copyright file="ChatSessionObject.cs" company="AllSeen Alliance.">
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
// </copyright>
// ******************************************************************************

namespace Chat.Common
{
    using AllJoyn;

    /// <summary>
    /// This is the class which handles an instance of a chat session.
    /// </summary>
    public class ChatSessionObject
    {
        /// <summary>
        /// The name used for the chat service.
        /// </summary>
        private const string ChatServiceInterfaceName = "org.alljoyn.bus.samples.chat";

        /// <summary>
        /// The BusObject used for this chat session.
        /// </summary>
        private BusObject busObject;

        /// <summary>
        /// The message receiver used for this chat session.
        /// </summary>
        private MessageReceiver chatSignalReceiver;

        /// <summary>
        /// The chat interface.
        /// </summary>
        private InterfaceMember chatSignalMember;

        /// <summary>
        /// The class for the main page which handles the user interface.
        /// </summary>
        private MainPage hostPage;

        /// <summary>
        /// Initializes a new instance of the ChatSessionObject class. 
        /// </summary>
        /// <param name="bus">The BusAttachment to be associated with.</param>
        /// <param name="path">The path for the BusObject.</param>
        /// <param name="host">The instance of the MainPage which handles the UI for this
        /// application.</param>
        public ChatSessionObject(BusAttachment bus, string path, MainPage host)
        {
            try
            {
                this.hostPage = host;
                this.busObject = new BusObject(bus, path, false);

                /* Add the interface to this object */
                InterfaceDescription[] ifaceArr = new InterfaceDescription[1];
                bus.CreateInterface(ChatServiceInterfaceName, ifaceArr, false);
                ifaceArr[0].AddSignal("Chat", "s", "str", 0, string.Empty);
                ifaceArr[0].Activate();

                InterfaceDescription chatIfc = bus.GetInterface(ChatServiceInterfaceName);

                this.busObject.AddInterface(chatIfc);

                this.chatSignalReceiver = new MessageReceiver(bus);
                this.chatSignalReceiver.SignalHandler += new MessageReceiverSignalHandler(this.ChatSignalHandler);
                this.chatSignalMember = chatIfc.GetMember("Chat");
                bus.RegisterSignalHandler(this.chatSignalReceiver, this.chatSignalMember, path);
            }
            catch (System.Exception ex)
            {
                QStatus errCode = AllJoyn.AllJoynException.GetErrorCode(ex.HResult);
                string errMsg = AllJoyn.AllJoynException.GetErrorMessage(ex.HResult);
                this.hostPage.DisplayStatus("Create ChatSessionObject Error : " + errMsg);
            }
        }

        /// <summary>
        /// Obtain the BusObject from a ChatSession object.
        /// </summary>
        /// <param name="chatObj">The ChatSessionObject which contains the BusObject.</param>
        /// <returns>The BusObject associated with the given ChatSessionObject.</returns>
        public static implicit operator BusObject(ChatSessionObject chatObj)
        {
            return chatObj.busObject;
        }

        /// <summary>
        /// The method to handle a signal. If the message is for the active chat channel this device is
        /// on then the message is forwarded to the user interface page.
        /// </summary>
        /// <param name="member">The parameter is not used.</param>
        /// <param name="path">The parameter is not used.</param>
        /// <param name="msg">The msg from the other end.</param>
        public void ChatSignalHandler(InterfaceMember member, string path, Message msg)
        {
            if (this.hostPage != null && msg != null && msg.SessionId == this.hostPage.SessionId &&
                msg.Sender != msg.RcvEndpointName)
            {
                string sender = msg.Sender;
                var content = msg.GetArg(0).Value;
                if (content != null)
                {
                    this.hostPage.OnChat(this.hostPage.SessionId, sender + ": ", content.ToString());
                }
            }
        }

        /// <summary>
        /// This method sends a message to the person on the other end.
        /// </summary>
        /// <param name="id">The destination ID.</param>
        /// <param name="msg">The message to send.</param>
        public void SendChatSignal(uint id, string msg)
        {
            try
            {
                MsgArg msgarg = new MsgArg("s", new object[] { msg });
                this.busObject.Signal(string.Empty, id, this.chatSignalMember, new MsgArg[] { msgarg }, 0, 0);
            }
            catch (System.Exception ex)
            {
                QStatus errCode = AllJoyn.AllJoynException.GetErrorCode(ex.HResult);
                string errMsg = AllJoyn.AllJoynException.GetErrorMessage(ex.HResult);
                this.hostPage.DisplayStatus("SendChatSignal Error: " + errMsg);
            }
        }
    }
}
