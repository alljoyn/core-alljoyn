//-----------------------------------------------------------------------
// <copyright file="Main.cs" company="AllSeen Alliance.">
// Copyright (c) 2012-2013, AllSeen Alliance. All rights reserved.
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
//-----------------------------------------------------------------------

using System;
using AllJoynUnity;

namespace basic_client
{
	class MainClass
	{
		private const string INTERFACE_NAME = "org.alljoyn.Bus.sample";
		private const string SERVICE_NAME = "org.alljoyn.Bus.sample";
		private const string SERVICE_PATH = "/sample";
		private const ushort SERVICE_PORT = 25;

		private static readonly string[] connectArgs = {"null:"};

		private static bool sJoinComplete = false;
		private static AllJoyn.BusAttachment sMsgBus;
		private static MyBusListener sBusListener;
		private static uint sSessionId;

		class MyBusListener : AllJoyn.BusListener
		{
			protected override void FoundAdvertisedName(string name, AllJoyn.TransportMask transport, string namePrefix)
			{
				Console.WriteLine("FoundAdvertisedName(name=" + name + ", prefix=" + namePrefix + ")");
				if(string.Compare(SERVICE_NAME, name) == 0)
				{
					// We found a remote bus that is advertising basic service's  well-known name so connect to it
					AllJoyn.SessionOpts opts = new AllJoyn.SessionOpts(AllJoyn.SessionOpts.TrafficType.Messages, false,
						AllJoyn.SessionOpts.ProximityType.Any, AllJoyn.TransportMask.Any);

					AllJoyn.QStatus status = sMsgBus.JoinSession(name, SERVICE_PORT, null, out sSessionId, opts);
					if(status)
					{
						Console.WriteLine("JoinSession SUCCESS (Session id={0})", sSessionId);
					}
					else
					{
						Console.WriteLine("JoinSession failed (status={0})", status.ToString ());
					}
				}
				sJoinComplete = true;
			}

			protected override void NameOwnerChanged(string busName, string previousOwner, string newOwner)
			{
				if(string.Compare(SERVICE_NAME, busName) == 0)
				{
					Console.WriteLine("NameOwnerChanged: name=" + busName + ", oldOwner=" +
						previousOwner + ", newOwner=" + newOwner);
				}
			}
		}

		public static void Main(string[] args)
		{
			Console.WriteLine("AllJoyn Library version: " + AllJoyn.GetVersion());
			Console.WriteLine("AllJoyn Library buildInfo: " + AllJoyn.GetBuildInfo());

			// Create message bus
			sMsgBus = new AllJoyn.BusAttachment("myApp", true);

			// Add org.alljoyn.Bus.method_sample interface
			AllJoyn.InterfaceDescription testIntf;
			AllJoyn.QStatus status = sMsgBus.CreateInterface(INTERFACE_NAME, out testIntf);
			if(status)
			{
				Console.WriteLine("Interface Created.");
				testIntf.AddMember(AllJoyn.Message.Type.MethodCall, "cat", "ss", "s", "inStr1,inStr2,outStr");
				testIntf.Activate();
			}
			else
			{
				Console.WriteLine("Failed to create interface 'org.alljoyn.Bus.method_sample'");
			}

			// Start the msg bus
			if(status)
			{
				status = sMsgBus.Start();
				if(status)
				{
					Console.WriteLine("BusAttachment started.");
				}
				else
				{
					Console.WriteLine("BusAttachment.Start failed.");
				}
			}

			// Connect to the bus
			if(status)
			{
				for (int i = 0; i < connectArgs.Length; ++i)
				{
					status = sMsgBus.Connect(connectArgs[i]);
					if (status)
					{
						Console.WriteLine("BusAttchement.Connect(" + connectArgs[i] + ") SUCCEDED.");
						break;
					}
					else
					{
						Console.WriteLine("BusAttachment.Connect(" + connectArgs[i] + ") failed.");
					}
				}
				if(!status)
				{
					Console.WriteLine("BusAttachment.Connect failed.");
				}
			}

			// Create a bus listener
			sBusListener = new MyBusListener();

			if(status)
			{
				sMsgBus.RegisterBusListener(sBusListener);
				Console.WriteLine("BusListener Registered.");
			}

			// Begin discovery on the well-known name of the service to be called
			if(status)
			{
				status = sMsgBus.FindAdvertisedName(SERVICE_NAME);
				if(!status)
				{
					Console.WriteLine("org.alljoyn.Bus.FindAdvertisedName failed.");
				}
			}

			// Wait for join session to complete
			while(sJoinComplete == false)
			{
				System.Threading.Thread.Sleep(1);
			}

			if(status)
			{
				using(AllJoyn.ProxyBusObject remoteObj = new AllJoyn.ProxyBusObject(sMsgBus, SERVICE_NAME, SERVICE_PATH, sSessionId))
				{
					AllJoyn.InterfaceDescription alljoynTestIntf = sMsgBus.GetInterface(INTERFACE_NAME);
					if(alljoynTestIntf == null)
					{
						throw new Exception("Failed to get test interface.");
					}
					remoteObj.AddInterface(alljoynTestIntf);

					AllJoyn.Message reply = new AllJoyn.Message(sMsgBus);
					AllJoyn.MsgArg inputs = new AllJoyn.MsgArg(2);
					inputs[0] = "Hello ";
					inputs[1] = "World!";

					status = remoteObj.MethodCall(SERVICE_NAME, "cat", inputs, reply, 5000, 0);
					
					if(status)
					{
						Console.WriteLine("{0}.{1} (path={2}) returned \"{3}\"", SERVICE_NAME, "cat", SERVICE_PATH,
							(string)reply[0]);
					}
					else
					{
						Console.WriteLine("MethodCall on {0}.{1} failed", SERVICE_NAME, "cat");
					}
				}
			}

			// Dispose of objects now
			sMsgBus.Dispose();
			sBusListener.Dispose();

			Console.WriteLine("basic client exiting with status {0} ({1})\n", status, status.ToString());
		}
	}
}
