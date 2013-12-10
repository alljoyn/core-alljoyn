//-----------------------------------------------------------------------
// <copyright file="Service.cs" company="AllSeen Alliance.">
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
    using System.Runtime.InteropServices;
    using System.Threading.Tasks;
    using AllJoyn;

    /// <summary>
    /// This class is used to handle all the tasks assocated with the service part of
    /// the secure sample.
    /// </summary>
    public class Service
    {
        /// <summary>
        /// Initializes a new instance of the Service class.
        /// The constructor sets up the AllJoyn bus and starts the interaction.
        /// </summary>
        public Service()
        {
            // Create the BusAttachment and other prep for accepting input from the client(s).
            this.ConnectBus = new Task(() =>
            {
                try
                {
                    this.InitializeAllJoyn();
                    App app = Windows.UI.Xaml.Application.Current as App;
                    app.Bus = this.Bus;
                }
                catch (Exception ex)
                {
                    const string ErrorFormat = "Bus intialization for server produced error(s). QStatus = 0x{0:X}.";
                    QStatus status = AllJoynException.GetErrorCode(ex.HResult);
                    string error = string.Format(ErrorFormat, status);

                    System.Diagnostics.Debug.WriteLine(error);
                    App.OutputLine(error);
                }
            });

            this.ConnectBus.Start();
        }

        /// <summary>
        /// Finalizes an instance of the Service class.
        /// It unregisters and nulls out references so garbage collection can
        /// occur and tear down the AllJoyn service.
        /// </summary>
        ~Service()
        {
            this.Stop();
        }

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
        /// Gets or sets the task that connects to the bus.
        /// </summary>
        private Task ConnectBus { get; set; }

        /// <summary>
        /// Gets or sets the BusObject.
        /// </summary>
        private SecureBusObject BusObject { get; set; }

        /// <summary>
        /// Stop the service.
        /// </summary>
        internal void Stop()
        {
            if (null != this.Bus)
            {
                this.Bus.UnregisterBusListener(this.Listeners);
                this.Bus.ReleaseName(App.ServiceName);
            }

            this.Listeners = null;
            this.Bus = null;
            App.OutputLine("Service has been stopped.");
        }

        /// <summary>
        /// Initializes the global bus attachment
        /// </summary>
        private void InitializeAllJoyn()
        {
            InterfaceDescription[] iface = new InterfaceDescription[1];

            this.Bus = new BusAttachment("SRPSecurityServiceA", true, 4);

            this.Bus.CreateInterface(App.InterfaceName, iface, true);
            iface[0].AddMethod("Ping", "s", "s", "inStr,outStr", 0, string.Empty);
            iface[0].Activate();

            this.BusObject = new SecureBusObject(this.Bus, iface);

            this.Listeners = new Listeners(this.Bus);
            this.Listeners.NameOwnerChange += this.NameOwnerChanged;
            this.Listeners.AcceptSessionJoiner += this.AcceptSessionJoiner;

            this.Bus.RegisterBusListener(this.Listeners);
            this.Bus.Start();

            // Enable security.
            // Note the location of the keystore file has been specified and the
            // isShared parameter is being set to true. So this keystore file can
            // be used by multiple applications.
            this.AuthenticationListener = new AuthListener(this.Bus);

            this.AuthenticationListener.RequestCredentials += this.AuthRequestCredentals;
            this.Bus.EnablePeerSecurity(
                                        App.SecurityType,
                                        this.AuthenticationListener,
                                        "/.alljoyn_keystore/s_central.ks",
                                        true);
            
            this.Bus.ConnectAsync(App.ConnectSpec).AsTask().Wait();

            SessionOpts sessionOpts = new SessionOpts(
                                                      TrafficType.TRAFFIC_MESSAGES,
                                                      false,
                                                      ProximityType.PROXIMITY_ANY,
                                                      TransportMaskType.TRANSPORT_ANY);
            ushort[] portOut = new ushort[1];

            this.Bus.BindSessionPort(App.ServicePort, portOut, sessionOpts, this.Listeners);

            try
            {
                uint flags = (uint)RequestNameType.DBUS_NAME_REPLACE_EXISTING + (uint)RequestNameType.DBUS_NAME_DO_NOT_QUEUE;
                this.Bus.RequestName(App.ServiceName, flags);
            }
            catch (COMException ce)
            {
                QStatus exceptionStatus = AllJoynException.GetErrorCode(ce.HResult);
                string error = string.Format(
                                             "Well known name '{0}' was not accepted. QStatus = 0x{1:X}",
                                             App.ServiceName,
                                             exceptionStatus);

                System.Diagnostics.Debug.WriteLine(error);
                App.OutputLine(error);
            }

            this.Bus.AdvertiseName(App.ServiceName, sessionOpts.TransportMask);
            App.OutputLine(string.Format("Name is being advertised as: '{0}'.", App.ServiceName));
        }

        /// <summary>
        /// Determines whether to accept a session. Returns true if yes, false otherwise.
        /// </summary>
        /// <param name="sessionPort">The session port being joined.</param>
        /// <param name="joiner">The client name attempting to join.</param>
        /// <param name="opts">The sessions options for the join.</param>
        /// <returns>true if joining is acceptable. false if not acceptable.</returns>
        private bool AcceptSessionJoiner(ushort sessionPort, string joiner, SessionOpts opts)
        {
            if (sessionPort != App.ServicePort)
            {
                string rejectMessage = string.Format("Rejecting join attempt on unexpected session port {0}.", sessionPort);

                App.OutputLine(rejectMessage);

                return false;
            }

            string successMessage = string.Format(
                                "Accepting join session request from {0} (opts.proximity={1}, opts.traffic={2}, opts.transports={3}).",
                                joiner,
                                opts.Proximity.ToString(),
                                opts.Traffic.ToString(), 
                                opts.TransportMask.ToString());

            App.OutputLine(successMessage);

            return true;
        }

        /// <summary>
        /// Handler for a name owner change.
        /// </summary>
        /// <param name="busName">The name of the bus being changed.</param>
        /// <param name="previousOwner">The previous owner of the bus.</param>
        /// <param name="newOwner">The new owner of the bus.</param>
        private void NameOwnerChanged(string busName, string previousOwner, string newOwner)
        {
            if (!string.IsNullOrEmpty(newOwner) &&
                !string.IsNullOrEmpty(busName) &&
                (0 == busName.CompareTo(App.ServiceName)))
            {
                string message = string.Format(
                                               "NameOwnerChanged: name={0}, oldOwner={1}, newOwner={2}",
                                               busName,
                                               !string.IsNullOrEmpty(previousOwner) ? previousOwner : "<none>",
                                               !string.IsNullOrEmpty(newOwner) ? newOwner : "<none>");
                App.OutputLine(message);
            }
        }

        /// <summary>
        /// <para>This is the handler of AuthListener. It is designed to only handle SRP Key Exchange
        /// Authentication requests.
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
        /// <param name="context">The details of the credentials.</param>
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
                        Random rand = new Random();
                        string pin = ((int)(rand.NextDouble() * 1000000)).ToString("D6");
                        string pinReport = string.Format("One Time Password : {0}.", pin);

                        App.OutputLine(pinReport);
                        App.OutputPIN(pin);

                        Credentials creds = new Credentials();

                        creds.Expiration = 120;
                        creds.Password = pin;

                        this.AuthenticationListener.RequestCredentialsResponse(context, true, creds);

                        returnValue = QStatus.ER_OK;
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
    }
}
