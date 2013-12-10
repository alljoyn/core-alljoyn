//-----------------------------------------------------------------------
// <copyright file="Client.cs" company="AllSeen Alliance.">
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
    using System.Threading;
    using System.Threading.Tasks;
    using AllJoyn;
    using Windows.Foundation;

    /// <summary>
    /// This class implements client to connect to the server.
    /// </summary>
    public class Client
    {
        /// <summary>
        /// Wait time in mS for a pin to be entered before timing out.
        /// </summary>
        public const int PinWaitTime = 30 * 1000;

        /// <summary>
        /// Initializes a new instance of the Client class.
        /// </summary>
        /// <param name="name">The name of the client application.</param>
        /// <param name="pinReady">The event to communicate there is a pin waiting.</param>
        public Client(string name, AutoResetEvent pinReady)
        {
            this.PinReady = pinReady;

            // Create the BusAttachment and other prep for accepting input from the client(s).
            this.ConnectBus = new Task(() =>
            {
                try
                {
                    this.InitializeAllJoyn(name);
                    App app = Windows.UI.Xaml.Application.Current as App;
                    app.Bus = this.Bus;
                }
                catch (Exception ex)
                {
                    const string ErrorFormat = "Bus intialization for client produced error(s). QStatus = 0x{0:X}.";
                    QStatus status = AllJoynException.GetErrorCode(ex.HResult);
                    string error = string.Format(ErrorFormat, status);

                    System.Diagnostics.Debug.WriteLine(error);
                    App.OutputLine(error);
                }
            });

            this.ConnectBus.Start();
        }

        /// <summary>
        /// Finalizes an instance of the Client class. Unregisters and nulls out references so
        /// garbage collection can occur and tear down the AllJoyn service.
        /// </summary>
        ~Client()
        {
            this.Stop();
        }

        /// <summary>
        /// Gets or sets the pin supplied by the user for authentication. This is not valid
        /// until the PinReady event has been set.
        /// </summary>
        public string Pin { get; set; }

        /// <summary>
        /// Gets or sets the BusAttachment for the application.
        /// </summary>
        public BusAttachment Bus { get; set; }

        /// <summary>
        /// Gets or sets the collection of listeners for this application BusAttachment.
        /// </summary>
        public Listeners Listeners { get; set; }

        /// <summary>
        /// Gets or sets the AuthListner used for authentication.
        /// </summary>
        private AuthListener AuthenticationListener { get; set; }

        /// <summary>
        /// Gets or sets the event to be set when there is a pin waiting from the user
        /// </summary>
        private AutoResetEvent PinReady { get; set; }

        /// <summary>
        /// Gets or sets the task that connects to the bus.
        /// </summary>
        private Task ConnectBus { get; set; }

        /// <summary>
        /// Gets or sets the interface returned when connecting to the AllJoyn bus asyncronously.
        /// </summary>
        private IAsyncAction ConnectOp { get; set; }

        /// <summary>
        /// Gets or sets the BusObject.
        /// </summary>
        private ProxyBusObject ProxyObject { get; set; }

        /// <summary>
        /// Stop this client from using the AllJoyn bus.
        /// </summary>
        internal void Stop()
        {
            if (null != this.Bus)
            {
                this.Bus.UnregisterBusListener(this.Listeners);
            }

            this.ProxyObject = null;
            this.Listeners = null;
            this.Bus = null;
            App.OutputLine("Client has been stopped.");
        }

        /// <summary>
        /// Create the BusAttachment, interface, setup the BusListener and other AllJoyn intialization
        /// tasks.
        /// </summary>
        /// <param name="name">The name for the BusAttachment.</param>
        private void InitializeAllJoyn(string name)
        {
            InterfaceDescription[] iface = new InterfaceDescription[1];

            this.Bus = new BusAttachment(name, true, 4);

            // Create a secure interface.
            this.Bus.CreateInterface(App.InterfaceName, iface, true);
            iface[0].AddMethod("Ping", "s", "s", "inStr,outStr", 0, string.Empty);
            iface[0].Activate();

            this.Listeners = new Listeners(this.Bus);
            this.Bus.RegisterBusListener(this.Listeners);

            this.Listeners.FoundAdvertisedName += this.FoundAdvertisedName;

            this.Bus.Start();

            // Enable security.
            // Note the location of the keystore file has been specified and the
            // isShared parameter is being set to true. So this keystore file can
            // be used by multiple applications.
            this.AuthenticationListener = new AuthListener(this.Bus);

            this.AuthenticationListener.RequestCredentials += this.AuthRequestCredentals;
            this.AuthenticationListener.AuthenticationComplete += this.AuthComplete;
            this.Bus.EnablePeerSecurity(
                                        App.SecurityType,
                                        this.AuthenticationListener,
                                        "/.alljoyn_keystore/s_central.ks",
                                        true);

            this.ConnectOp = this.Bus.ConnectAsync(App.ConnectSpec);
            this.ConnectOp.Completed = new AsyncActionCompletedHandler(this.BusConnected);
        }

        /// <summary>
        /// This method is called when the advertised name is found.
        /// </summary>
        /// <param name="wellKnownName">A well known name that the remote bus is advertising.</param>
        /// <param name="transportMask">Transport that received the advertisement.</param>
        /// <param name="namePrefix">The well-known name prefix used in call to FindAdvertisedName that triggered this callback.</param>
        private async void FoundAdvertisedName(string wellKnownName, TransportMaskType transportMask, string namePrefix)
        {
            if (!string.IsNullOrEmpty(wellKnownName) && wellKnownName.CompareTo(App.ServiceName) == 0)
            {
                string foundIt = string.Format(
                                               "Client found advertised name '{0}' on {1}.",
                                               wellKnownName,
                                               transportMask.ToString());

                App.OutputLine(foundIt);

                // We found a remote bus that is advertising the well-known name so connect to it.
                bool result = await this.JoinSessionAsync();

                if (result)
                {
                    InterfaceDescription secureInterface = this.Bus.GetInterface(App.InterfaceName);

                    this.ProxyObject = new ProxyBusObject(this.Bus, App.ServiceName, App.ServicePath, 0);
                    this.ProxyObject.AddInterface(secureInterface);

                    MsgArg[] inputs = new MsgArg[1];

                    inputs[0] = new MsgArg("s", new object[] { "Client says Hello AllJoyn!" });

                    MethodCallResult callResults = null;

                    try
                    {
                        callResults = await this.ProxyObject.MethodCallAsync(App.InterfaceName, "Ping", inputs, null, 5000, 0);
                    }
                    catch (Exception ex)
                    {
                        const string ErrorFormat =
                            "ProxyObject.MethodCallAsync(\"Ping\") call produced error(s). QStatus = 0x{0:X} ('{1}').";
                        QStatus status = AllJoynException.GetErrorCode(ex.HResult);
                        string error = string.Format(ErrorFormat, status, status.ToString());

                        System.Diagnostics.Debug.WriteLine(error);
                        App.OutputLine(error);
                    }

                    if (callResults != null)
                    {
                        Message mess = callResults.Message;
                        AllJoynMessageType messType = mess.Type;

                        if (messType == AllJoynMessageType.MESSAGE_METHOD_RET)
                        {
                            string strRet = mess.GetArg(0).Value as string;

                            if (!string.IsNullOrEmpty(strRet))
                            {
                                string output = string.Format(
                                                              "{0}.Ping (path = {1}) returned \"{2}\"",
                                                              App.InterfaceName,
                                                              App.ServicePath,
                                                              strRet);
                                App.OutputLine(output);
                            }
                            else
                            {
                                const string Error =
                                        "Error: Server returned null or empty string in response to ping.";

                                App.OutputLine(Error);
                                System.Diagnostics.Debug.WriteLine(Error);
                            }
                        }
                        else
                        {
                            string error =
                                    string.Format("Server returned message of type '{0}'.", messType.ToString());

                            App.OutputLine(error);
                            System.Diagnostics.Debug.WriteLine(error);
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Do the stuff required to join the session.
        /// </summary>
        /// <returns>True if successfully joined the session.</returns>
        private async Task<bool> JoinSessionAsync()
        {
            bool returnValue = true;
            SessionOpts opts = new SessionOpts(
                                               TrafficType.TRAFFIC_MESSAGES,
                                               false,
                                               ProximityType.PROXIMITY_ANY,
                                               TransportMaskType.TRANSPORT_ANY);

            SessionOpts[] opts_out = new SessionOpts[1];
            JoinSessionResult result = await this.Bus.JoinSessionAsync(
                                                                       App.ServiceName, 
                                                                       App.ServicePort,
                                                                       this.Listeners,
                                                                       opts,
                                                                       opts_out,
                                                                       null);

            if (result.Status == QStatus.ER_OK)
            {
                App.OutputLine("Successfully joined session.");
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
        /// <para>This is the handler of authentication requests. It is designed to only handle SRP
        /// Key Exchange Authentication requests.
        /// </para>
        /// <para>When a Password request (CRED_PASSWORD) comes in using App.SecurityType the
        /// code will generate a 6 digit random pin code. The client must enter the same
        /// pin code into his AuthListener for the Authentication to be successful.
        /// </para>
        /// <para>If any other authMechanism is used other than SRP Key Exchange authentication it
        /// will fail.</para>
        /// </summary>
        /// <param name="authMechanism">A string describing the authentication method.</param>
        /// <param name="authPeer">The name of the peer to be authenticated.</param>
        /// <param name="authCount">The number of attempts at authentication.</param>
        /// <param name="userId">The parameter is not used.</param>
        /// <param name="credMask">The type of credentials expected.</param>
        /// <param name="context">The context of the authentication request.</param>
        /// <returns>true if successful or false if there was a problem.</returns>
        private QStatus AuthRequestCredentals(
                                              string authMechanism,
                                              string authPeer,
                                              ushort authCount,
                                              string userId,
                                              ushort credMask,
                                              AuthContext context)
        {
            QStatus returnValue = QStatus.ER_AUTH_FAIL;

            if (!string.IsNullOrEmpty(authMechanism) && !string.IsNullOrEmpty(authPeer))
            {
                const string RequestFormat = "RequestCredentials for authenticating {0} using mechanism {1}.";

                App.OutputLine(string.Format(RequestFormat, authPeer, authMechanism));

                if (authMechanism.CompareTo(App.SecurityType) == 0 &&
                    0 != (credMask & (ushort)CredentialType.CRED_PASSWORD))
                {
                    if (authCount <= 3)
                    {
                        if (this.PinReady.WaitOne(Client.PinWaitTime) && !string.IsNullOrWhiteSpace(this.Pin))
                        {
                            Credentials creds = new Credentials();

                            creds.Expiration = 120;
                            creds.Password = this.Pin;
                            this.AuthenticationListener.RequestCredentialsResponse(context, true, creds);

                            returnValue = QStatus.ER_OK;
                        }
                    }
                    else
                    {
                        App.OutputLine(string.Format("Authorization failed after {0} attempts.", authCount));
                    }
                }
                else
                {
                    App.OutputLine("Unexpected authorization mechanism or credentials mask.");
                }
            }
            else
            {
                App.OutputLine("Empty or null string parameters received in AuthRequestCredentials.");
            }

            return returnValue;
        }

        /// <summary>
        /// This is the handler for the authorization completed event.
        /// </summary>
        /// <param name="authMechanism">The type of authentiation method used.</param>
        /// <param name="authPeer">Name of the peer authenticating with.</param>
        /// <param name="success">True if successful. False if not.</param>
        private void AuthComplete(string authMechanism, string authPeer, bool success)
        {
            string report = string.Format("Authentication {0} with peer '{1}' {2}.", authMechanism, authPeer, success ? "successful" : "failed");

            App.OutputLine(report);
        }

        /// <summary>
        /// Handles the initial connection state for connecting up the BusAttachment for service
        /// </summary>
        /// <param name="sender">The IAsyncAction interface which represents an asynchronous action
        /// that does not return a result and does not have progress notifications.</param>
        /// <param name="status">The status of the async connection request.</param>
        private void BusConnected(IAsyncAction sender, AsyncStatus status)
        {
            if (status == AsyncStatus.Error || status == AsyncStatus.Canceled)
            {
                App.OutputLine(string.Format("Bus connection request status = {0}.", status));
            }
            else
            {
                try
                {
                    App.OutputLine("Client reporting the BusConnected.");
                    sender.GetResults();
                    this.Bus.FindAdvertisedName(App.ServiceName);
                }
                catch
                {
                    this.ConnectBus = new Task(() =>
                    {
                        ManualResetEvent evt = new ManualResetEvent(false);

                        evt.WaitOne(App.ConnectionRetryWaitTimeMilliseconds);
                        this.ConnectOp = this.Bus.ConnectAsync(App.ConnectSpec);
                        this.ConnectOp.Completed = new AsyncActionCompletedHandler(this.BusConnected);
                    });

                    this.ConnectBus.Start();
                }
            }
        }
    }
}
