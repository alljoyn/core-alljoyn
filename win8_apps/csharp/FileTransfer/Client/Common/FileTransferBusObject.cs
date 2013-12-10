//-----------------------------------------------------------------------
// <copyright file="FileTransferBusObject.cs" company="AllSeen Alliance.">
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

namespace FileTransferClient.Common
{
    using AllJoyn;
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Windows.Foundation;
    using Windows.Storage;
    using Windows.Storage.Pickers;
    using Windows.Storage.Streams;
    using Windows.UI.Xaml;

    class FileTransferBusObject
    {
        /// <summary>
        /// Gets or sets the bus attachment.
        /// </summary>
        private BusAttachment Bus { get; set; }

        /// <summary>
        /// Gets or sets the message receiver.
        /// </summary>
        private MessageReceiver Receiver { get; set; }

        /// <summary>
        /// Gets or sets the interface description.
        /// </summary>
        private InterfaceDescription[] Interface { get; set; }

        /// <summary>
        /// Initializes a new instance of the <see cref="FileTransferBusObject" /> class.
        /// </summary>
        /// <param name="busAtt">Object responsible for connecting to and optionally managing a
        /// message bus.</param>
        public FileTransferBusObject(BusAttachment busAtt)
        {
            this.Bus = busAtt;
            this.Folder = null;
            this.lastIndex = 0;
            this.SessionId = 0;

            if (null == this.Bus)
            {
                App.OutputLine("Unexpected null object in parameters to FileTransferBusObject constructor.");
            }
        }

        /// <summary>
        /// Finalizes an instance of the FileTransferBusObject class. Unregisters and nulls out
        /// references so garbage collection can occur.
        /// </summary>
        ~FileTransferBusObject()
        {
            if (null != this.Bus && null != this.Interface)
            {
                if (0 != this.SessionId)
                {
                    this.Bus.LeaveSession(this.SessionId);
                    this.SessionId = 0;
                }

                InterfaceMember member = this.Interface[0].GetSignal(ClientGlobals.SignalName);

                this.Bus.UnregisterSignalHandler(this.Receiver, member, string.Empty);
                this.Bus.UnregisterBusObject(this.BusObject);

                this.BusObject = null;
                this.Receiver = null;
                this.Folder = null;
                this.Interface = null;
                this.Writer = null;
            }
        }

        /// <summary>
        /// Do the work of joining a session.
        /// </summary>
        /// <param name="folder">The folder to put the received file in.</param>
        /// <returns>true if all the input variables look good.</returns>
        public bool StartJoinSessionTask(StorageFolder folder)
        {
            bool returnValue = false;
            this.Folder = folder;

            if (null != this.Bus && null != this.Folder)
            {
                Task task = new Task(async () =>
                {
                    try
                    {
                        if (0 != this.SessionId)
                        {
                            this.Bus.LeaveSession(this.SessionId);
                        }

                        this.lastIndex = 0;
                        this.CreateInterfaceAndBusObject();
                        this.AddSignalHandler();

                        // We found a remote bus that is advertising the well-known name so connect to it.
                        bool result = await this.JoinSessionAsync();

                        if (result)
                        {
                            App.OutputLine("Successfully joined session.");
                        }
                    }
                    catch (Exception ex)
                    {
                        QStatus status = AllJoynException.GetErrorCode(ex.HResult);
                        string message = string.Format(
                                            "Errors were produced while establishing the application '{0}' (0x{0:X}).",
                                            status.ToString(),
                                            status);
                        App.OutputLine(message);
                    }
                });

                task.Start();

                returnValue = true;
            }

            return returnValue;
        }

        /// <summary>
        /// Do the stuff required to join the session.
        /// </summary>
        /// <returns>True if successfully joined the session.</returns>
        private async Task<bool> JoinSessionAsync()
        {
            bool returnValue = true;
            SessionOpts opts = new SessionOpts(
                        ClientGlobals.SessionProps.TrType,
                        ClientGlobals.SessionProps.IsMultiPoint,
                        ClientGlobals.SessionProps.PrType,
                        ClientGlobals.SessionProps.TmType);
            SessionOpts[] opts_out = new SessionOpts[1];
            App app = Application.Current as App;

            JoinSessionResult result = await this.Bus.JoinSessionAsync(
                                                                       ClientGlobals.ServiceName,
                                                                       ClientGlobals.SessionProps.SessionPort,
                                                                       app.Listeners,
                                                                       opts,
                                                                       opts_out,
                                                                       null);
            string message =
                string.Format("Join Session status = {0}.", result.Status.ToString());
            App.OutputLine(message);

            if (QStatus.ER_OK == result.Status)
            {
                this.SessionId = result.SessionId;
                message =
                    string.Format("SessionId='{0}'.", result.SessionId);
                App.OutputLine(message);
            }
            else
            {
                string errorMessage = string.Format("Join session failed with error {0:X}.", result.Status);
                App.OutputLine(errorMessage);
                returnValue = false;
            }

            return returnValue;
        }

        /// <summary>
        /// Create the file transfer client interface and BusObject. Add the interface to the bus object.
        /// </summary>
        private void CreateInterfaceAndBusObject()
        {
            // Create the file transfer client interface.
            this.Interface = new InterfaceDescription[1];

            try
            {
                this.Interface[0] = this.Bus.GetInterface(ClientGlobals.InterfaceName);
            }
            catch
            {
                this.Interface[0] = null;
            }

            // Don't create an interface if it already has been done.
            if (null == this.Interface[0])
            {
                this.Bus.CreateInterface(ClientGlobals.InterfaceName, this.Interface, false);
                this.Interface[0].AddSignal(ClientGlobals.SignalName, "suay", "name,curr,data", 0, string.Empty);
                this.Interface[0].Activate();
            }

            this.BusObject = new BusObject(this.Bus, ClientGlobals.ServicePath, false);
            this.BusObject.AddInterface(this.Interface[0]);
        }

        /// <summary>
        /// Create a message receiver and add the signal handler to the receiver.
        /// Then register the signal handler.
        /// </summary>
        private void AddSignalHandler()
        {
            InterfaceMember member = this.Interface[0].GetSignal(ClientGlobals.SignalName);

            // Add the File Transfer Signal handler
            this.Receiver = new MessageReceiver(this.Bus);
            this.Receiver.SignalHandler += new MessageReceiverSignalHandler(this.FileTransfer);

            this.Bus.RegisterSignalHandler(this.Receiver, member, string.Empty);
            this.Bus.RegisterBusObject(this.BusObject);
        }

        /// <summary>
        /// Gets or sets alljoyn bus object which implements 'FileTransfer' interface.
        /// </summary>
        public BusObject BusObject { get; set; }

        /// <summary>
        /// Writer used to write data to an external file
        /// </summary>
        private BinaryWriter Writer { get; set; }

        /// <summary>
        /// The folder where the user specifies the file to be written
        /// </summary>
        private StorageFolder Folder { get; set; }

        /// <summary>
        /// The current session ID.
        /// </summary>
        public uint SessionId { get; set; }

        /// <summary>
        /// This is the index of the last data array that was completely processed. It is used to
        /// maintain order of the signals if the signals stack up while waiting for the output file 
        /// to be opened.
        /// </summary>
        private volatile UInt32 lastIndex;

        /// <summary>
        /// Gets or sets a value indication whether there was an error in opening the file or file I/O.
        /// </summary>
        private bool FileWriteError { get; set; }

        /// <summary>
        /// Consumes 'FileTransfer' signals form the service and extracts the data to write to an 
        /// output file.
        /// </summary>
        /// <param name="member">Method or signal interface member entry.</param>
        /// <param name="srcPath">Object path of signal emitter.</param>
        /// <param name="message">The received message.</param>
        private async void FileTransfer(InterfaceMember member, string srcPath, Message message)
        {
            UInt32 currentIndex = (UInt32)message.GetArg(1).Value;

            while (currentIndex != this.lastIndex + 1)
            {
                await Task.Yield();
            }

            byte[] data = message.GetArg(2).Value as byte[];

            if (null != data)
            {
                if (0 != data.Count())
                {
                    string mess = string.Format("Array Num : {0} Size : {1}", currentIndex, data.Count());

                    App.OutputLine(mess);

                    bool openSuccess = null != this.Writer;

                    if (!openSuccess && 1 == currentIndex)
                    {
                        this.FileWriteError = false;
                        openSuccess = await this.OpenStreamAsync(message);
                    }

                    if (openSuccess)
                    {
                        mess = string.Format("Writing Array Num : {0}", currentIndex);
                        App.OutputLine(mess);
                        this.WriteToFile(data);
                    }
                    else
                    {
                        this.FileWriteError = true;
                        mess = string.Format("File was not open for writing array {0}.", currentIndex);
                        App.OutputLine(mess);
                    }
                }
                else
                {
                    // Done with this transfer prepare for another.
                    this.lastIndex = 0;

                    if (this.FileWriteError)
                    {
                        App.OutputLine("File transfer was unsuccessful!", true);
                    }
                    else
                    {
                        App.OutputLine("The file was transfered successfully.", true);
                    }

                    if (null != this.Writer)
                    {
                        this.Writer.Flush();
                        this.Writer.Dispose();
                        this.Writer = null;
                    }
                }
            }
            else
            {
                App.OutputLine("Unable to retrieve data array from message arg 2.");
                this.FileWriteError = true;
            }

            this.lastIndex = currentIndex;
        }

        /// <summary>
        /// On the first signal received from the service open a stream for writing the data to a file.
        /// </summary>
        /// <param name="message">The received message.</param>
        private async Task<bool> OpenStreamAsync(Message message)
        {
            try
            {
                string fileName = Path.GetFileName(message.GetArg(0).Value as string);

                if (!string.IsNullOrEmpty(fileName))
                {
                    StorageFile file = await this.Folder.CreateFileAsync(fileName, CreationCollisionOption.ReplaceExisting);
                    Stream stream = await file.OpenStreamForWriteAsync();
                    this.Writer = new BinaryWriter(stream);
                }
            }
            catch (Exception ex)
            {
                App.OutputLine("An error occurred while opening the file.");
                App.OutputLine("Error: " + ex.Message);
                this.FileWriteError = true;
            }

            return this.Writer != null;
        }

        /// <summary>
        /// Write the bytes received in the 'FileTransfer' signal to the file.
        /// </summary>
        /// <param name="data">The data to write to the file.</param>
        private void WriteToFile(byte[] data)
        {
            try
            {
                this.Writer.Write(data);
            }
            catch (Exception ex)
            {
                App.OutputLine("An error occurred while writing to the file.");
                App.OutputLine("Error: " + ex.Message);
                this.FileWriteError = true;
            }
        }
    }
}
