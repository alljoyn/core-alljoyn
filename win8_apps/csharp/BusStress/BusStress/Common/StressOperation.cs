//-----------------------------------------------------------------------
// <copyright file="StressOperation.cs" company="AllSeen Alliance.">
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
    using System.Threading;
    using System.Threading.Tasks;
    using AllJoyn;

    /// <summary>
    /// Contains the functionality of each stress operation
    /// </summary>
    public class StressOperation
    {
        /// <summary>
        /// Connect specifications for the tcp connection
        /// </summary>
        private const string ConnectSpecs = "tcp:addr=127.0.0.1,port=9956";

        /// <summary>
        /// Well-known service name
        /// </summary>
        private const string ServiceName = "org.alljoyn.Bus.test.bastress";

        /// <summary>
        /// Name of the interface implementing the 'cat' interface
        /// </summary>
        private const string InterfaceName = "org.alljoyn.Bus.test.bastress";

        /// <summary>
        /// Relative path of the service bus object
        /// </summary>
        private const string ServicePath = "/sample";

        /// <summary>
        /// Port bound by the service to allow session with clients
        /// </summary>
        private const ushort ServicePort = 25;

        /// <summary>
        /// Used to generate random numbers in this class
        /// </summary>
        private static Random rand = new Random();

        /// <summary>
        /// Primary message bus for this stress operation
        /// </summary>
        private BusAttachment busAtt;

        /// <summary>
        /// Reference to this application's stress manager
        /// </summary>
        private StressManager stressManager;

        /// <summary>
        /// Event which is signaled when the first service has been discovered
        /// </summary>
        private AutoResetEvent foundName;

        /// <summary>
        /// Initializes a new instance of the <see cref="StressOperation"/> class
        /// </summary>
        /// <param name="count">Unique id number that will be used to identify the operation</param>
        public StressOperation(uint count)
        {
            this.TaskId = count;
        }

        /// <summary>
        /// Gets or sets the discovered service name
        /// </summary>
        public string DiscoveredName { get; set; }

        /// <summary>
        /// Gets or sets a unique id given to each stress operation so that tasks can be 
        /// distinguished easier when debugging
        /// </summary>
        public uint TaskId { get; set; }

        /// <summary>
        /// Output a message to the UI for this app
        /// </summary>
        /// <param name="msg">Message to output to user</param>
        public void Output(string msg)
        {
            this.stressManager.Output(msg);
        }

        /// <summary>
        /// Print to the output window of the debugger
        /// </summary>
        /// <param name="msg">Message to print</param>
        public void DebugPrint(string msg)
        {
            System.Diagnostics.Debug.WriteLine(this.TaskId + " : " + msg);
        }

        /// <summary>
        /// Run the stress operation by setting up alljoyn, running the specified stress 
        /// operation then tearing down alljoyn.
        /// </summary>
        /// <param name="stressType">Type of stress operation to run</param>
        /// <param name="manager">Stress manager which is managing this task</param>
        /// <param name="isMultipoint">True if operation uses multipoint sessions</param>
        public void Start(StressType stressType, StressManager manager, bool isMultipoint)
        {
            this.stressManager = manager;

            try
            {
                // Set up alljoyn
                this.DebugPrint("Creating and Starting the bus attachment");
                this.busAtt = new BusAttachment("BusStress", true, 4);
                this.busAtt.Start();
                this.busAtt.ConnectAsync(ConnectSpecs).AsTask().Wait();
                this.DebugPrint("Successfully connected to the bundled daemon");

                // Run the stress operation
                if (stressType == StressType.Default)
                {
                    this.RunDefault();
                }
                else if (stressType == StressType.Service)
                {
                    this.RunService(isMultipoint);
                }
                else
                {
                    this.foundName = new AutoResetEvent(false);
                    this.RunClient(isMultipoint);
                }

                // Tear down alljoyn
                this.DebugPrint("Disconnecting and stopping the bus attachment");
                this.busAtt.DisconnectAsync(ConnectSpecs).AsTask().Wait();
                this.busAtt.StopAsync().AsTask().Wait();
                this.DebugPrint("Successfully disconnected and stopped the bus attachment");
            }
            catch (Exception ex)
            {
                var errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                this.DebugPrint(">>>> BusAttachment Error >>>> : " + errMsg);
            }

            this.busAtt = null;
        }

        /// <summary>
        /// Run the default stress op which stresses the bus attachment
        /// </summary>
        private void RunDefault()
        {
            try
            {
                BusListener busListener = new BusListener(this.busAtt);
                this.busAtt.RegisterBusListener(busListener);
                BusObject busObject = new BusObject(this.busAtt, "/default", false);
                this.busAtt.RegisterBusObject(busObject);
                this.DebugPrint("Registered BusListener and BusObject");

                uint flags = (uint)(RequestNameType.DBUS_NAME_REPLACE_EXISTING | RequestNameType.DBUS_NAME_DO_NOT_QUEUE);
                string randServiceName = null;
                lock (rand)
                {
                    randServiceName = ServiceName + ".n" + rand.Next(10000000);
                }

                this.busAtt.RequestName(randServiceName, flags);
                this.busAtt.AdvertiseName(randServiceName, TransportMaskType.TRANSPORT_ANY);
                this.DebugPrint("Advertising WKN : " + randServiceName);

                this.busAtt.CancelAdvertiseName(randServiceName, TransportMaskType.TRANSPORT_ANY);
                this.busAtt.ReleaseName(randServiceName);
                this.busAtt.UnregisterBusListener(busListener);
                this.busAtt.UnregisterBusObject(busObject);
                this.DebugPrint("Successfully unraveled the default operation");
            }
            catch (Exception ex)
            {
                var errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                this.DebugPrint(">>>> Default Execution Error >>>> : " + errMsg);
            }
        }

        /// <summary>
        /// Run the service stress op which stresses the bus attachment in a service type
        /// configuration by advertising a well known name which an implementation of the
        /// 'cat' method
        /// </summary>
        /// <param name="isMultipoint">True if operation uses multipoint sessions</param>
        private void RunService(bool isMultipoint)
        {
            try
            {
                ServiceBusListener serviceBusListener = new ServiceBusListener(this.busAtt, this);
                ServiceBusObject serviceBusObject = new ServiceBusObject(this.busAtt, this);
                this.DebugPrint("Registered the Service BusListener and BusObject");

                uint flags = (uint)(RequestNameType.DBUS_NAME_REPLACE_EXISTING | RequestNameType.DBUS_NAME_DO_NOT_QUEUE);
                string randServiceName = null;
                lock (rand)
                {
                    randServiceName = ServiceName + ".n" + rand.Next(10000000);
                }

                this.busAtt.RequestName(randServiceName, flags);

                SessionOpts optsIn = new SessionOpts(
                            TrafficType.TRAFFIC_MESSAGES,
                            isMultipoint,
                            ProximityType.PROXIMITY_ANY,
                            TransportMaskType.TRANSPORT_ANY);
                ushort[] portOut = new ushort[1];
                this.busAtt.BindSessionPort(ServicePort, portOut, optsIn, (SessionPortListener)serviceBusListener);

                this.busAtt.AdvertiseName(randServiceName, TransportMaskType.TRANSPORT_ANY);
                this.DebugPrint("Advertising WKN : " + randServiceName);

                int wait = 0;
                lock (rand)
                {
                    wait = 8000 + rand.Next(6000);
                }

                Task.Delay(wait).AsAsyncAction().AsTask().Wait();

                this.DebugPrint("Unraveling the service opertaion");
                this.busAtt.CancelAdvertiseName(randServiceName, TransportMaskType.TRANSPORT_ANY);
                this.busAtt.UnbindSessionPort(ServicePort);
                this.busAtt.ReleaseName(randServiceName);
                this.busAtt.UnregisterBusObject((BusObject)serviceBusObject);
                this.busAtt.UnregisterBusListener((BusListener)serviceBusListener);
                this.DebugPrint("Successfully unraveled the service opertaion");
            }
            catch (Exception ex)
            {
                var errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                this.DebugPrint(">>>> Service Exectution Execution Error >>>> : " + errMsg);
            }
        }

        /// <summary>
        /// Run the client stress op which stresses the bus attachment in a client type
        /// configuration which find the well-known service name and calls the 'cat'
        /// method
        /// </summary>
        /// <param name="isMultipoint">True if operation uses multipoint sessions</param>
        private void RunClient(bool isMultipoint)
        {
            try
            {
                ClientBusListener clientBusListener = new ClientBusListener(this.busAtt, this, this.foundName);
                this.busAtt.FindAdvertisedName(ServiceName);
                this.DebugPrint("Looking for WKN : " + ServiceName);

                this.foundName.WaitOne(12000);

                SessionOpts optsIn = new SessionOpts(
                            TrafficType.TRAFFIC_MESSAGES,
                            isMultipoint,
                            ProximityType.PROXIMITY_ANY,
                            TransportMaskType.TRANSPORT_ANY);
                SessionOpts[] optsOut = new SessionOpts[1];
                Task<JoinSessionResult> joinTask = this.busAtt.JoinSessionAsync(
                            this.DiscoveredName,
                            ServicePort,
                            (SessionListener)clientBusListener,
                            optsIn,
                            optsOut,
                            null).AsTask<JoinSessionResult>();
                joinTask.Wait();
                JoinSessionResult joinResult = joinTask.Result;
                QStatus status = joinResult.Status;

                ProxyBusObject proxyBusObj = null;
                if (QStatus.ER_OK == status)
                {
                    this.DebugPrint("JoinSession with " + this.DiscoveredName + " was successful (sessionId=" + joinResult.SessionId + ")");
                    proxyBusObj = new ProxyBusObject(this.busAtt, this.DiscoveredName, ServicePath, joinResult.SessionId);
                    Task<IntrospectRemoteObjectResult> introTask = proxyBusObj.IntrospectRemoteObjectAsync(null).AsTask<IntrospectRemoteObjectResult>();
                    introTask.Wait();
                    IntrospectRemoteObjectResult introResult = introTask.Result;
                    status = introResult.Status;

                    if (QStatus.ER_OK == status)
                    {
                        this.DebugPrint("Introspection of the service was successfull");
                        MsgArg hello = new MsgArg("s", new object[] { "Hello " });
                        MsgArg world = new MsgArg("s", new object[] { "World!" });
                        InterfaceMember catMethod = proxyBusObj.GetInterface(InterfaceName).GetMethod("cat");
                        byte flags = (byte)0;
                        Task<MethodCallResult> catTask = proxyBusObj.MethodCallAsync(catMethod, new MsgArg[] { hello, world }, null, 5000, flags).AsTask<MethodCallResult>();
                        catTask.Wait();
                        MethodCallResult catResult = catTask.Result;
                        if (catResult.Message.Type == AllJoynMessageType.MESSAGE_METHOD_RET)
                        {
                            this.DebugPrint(this.DiscoveredName + ".cat ( path=" + ServicePath + ") returned \"" + catResult.Message.GetArg(0).Value.ToString() + "\"");
                        }
                        else
                        {
                            this.DebugPrint("Method call on " + this.DiscoveredName + ".cat failed (ReturnType=" + catResult.Message.Type.ToString() + ")");
                        }
                    }
                    else
                    {
                        this.DebugPrint("Introspection was unsuccessful: " + status.ToString());
                    }
                }
                else
                {
                    this.DebugPrint("Join Session was unsuccessful: " + status.ToString());
                }

                this.busAtt.CancelFindAdvertisedName(ServiceName);
                this.busAtt.UnregisterBusListener(clientBusListener);
                this.DebugPrint("Successfully unraveled the client operation");
            }
            catch (ArgumentNullException ex)
            {
                this.DebugPrint(">>>> TIMEOUT, Client could not find WKN >>>>");
            }
            catch (Exception ex)
            {
                var errMsg = AllJoynException.GetErrorMessage(ex.HResult);
                this.DebugPrint(">>>> Client Execution Error >>>> " + errMsg);
            }
        }
    }
}
