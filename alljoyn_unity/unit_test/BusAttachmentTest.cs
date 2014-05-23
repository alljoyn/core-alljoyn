//-----------------------------------------------------------------------
// <copyright file="BusAttachmentTest.cs" company="AllSeen Alliance.">
// Copyright (c) 2012-2014, AllSeen Alliance. All rights reserved.
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
using System.Threading;
using AllJoynUnity;
using Xunit;

namespace AllJoynUnityTest
{

	public class BusAttachmentTest : IDisposable
	{

		private AutoResetEvent notifyEventOne = null;
		private AutoResetEvent notifyEventTwo = null;

		private bool handledSignalsOne;
		private bool handledSignalsTwo;
		private string signalOneMsg;
		private string signalTwoMsg;

		AllJoyn.BusAttachment busAttachment = null;
		private bool disposed = false;

		// Setup
		public BusAttachmentTest()
		{
			notifyEventOne = new AutoResetEvent(false);
			notifyEventTwo = new AutoResetEvent(false);

			handledSignalsOne = false;
			handledSignalsTwo = false;
			signalOneMsg = "";
			signalTwoMsg = "";

			busAttachment = new AllJoyn.BusAttachment("BusAttachmentTest", true);
			Assert.NotNull(busAttachment);
		}

		~BusAttachmentTest()
		{
			Dispose(false);
		}

		protected virtual void Dispose(bool disposing)
		{
			if (!this.disposed)
			{
				if (disposing)
				{
					busAttachment.Dispose();
				}
				disposed = true;
			}
		}

		public void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		[Fact]
		public void Concurrency()
		{
			// The default value for concurrency is 4
			Assert.Equal<uint>(4, busAttachment.Concurrency);
			busAttachment.Dispose();

			busAttachment = new AllJoyn.BusAttachment("BusAttachmentTest", true, 8);
			Assert.Equal<uint>(8, busAttachment.Concurrency);
		}

		[Fact]
		public void CreateInterface()
		{
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;

			AllJoyn.InterfaceDescription testIntf = null;
			status = busAttachment.CreateInterface("org.alljoyn.test.BusAttachment", out testIntf);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.NotNull(testIntf);
		}

		[Fact]
		public void Connect_no_params()
		{
			Assert.Equal(AllJoyn.QStatus.OK, busAttachment.Start());
			Assert.Equal(AllJoyn.QStatus.OK, busAttachment.Connect());

			Assert.True(busAttachment.IsConnected);

			Assert.True(busAttachment.ConnectSpec.Equals(AllJoynTestCommon.GetConnectSpec()) || busAttachment.ConnectSpec.Equals("null:"));

			busAttachment.Stop();
			busAttachment.Join();
		}

		[Fact]
		public void Disconnect()
		{
			Assert.Equal(AllJoyn.QStatus.BUS_BUS_NOT_STARTED, busAttachment.Disconnect(AllJoynTestCommon.GetConnectSpec()));

			Assert.Equal(AllJoyn.QStatus.OK, busAttachment.Start());

			Assert.Equal(AllJoyn.QStatus.BUS_NOT_CONNECTED, busAttachment.Disconnect(AllJoynTestCommon.GetConnectSpec()));

			Assert.Equal(AllJoyn.QStatus.OK, busAttachment.Connect(AllJoynTestCommon.GetConnectSpec()));

			Assert.True(busAttachment.IsConnected);

			Assert.Equal(AllJoyn.QStatus.OK, busAttachment.Disconnect(AllJoynTestCommon.GetConnectSpec()));

			Assert.False(busAttachment.IsConnected);

			busAttachment.Stop();
			busAttachment.Join();
		}

		/* test Connect/Disconnect that use no parameters */
		[Fact]
		public void Disconnect_no_params()
		{
			Assert.Equal(AllJoyn.QStatus.BUS_BUS_NOT_STARTED, busAttachment.Disconnect());

			Assert.Equal(AllJoyn.QStatus.OK, busAttachment.Start());

			Assert.Equal(AllJoyn.QStatus.BUS_NOT_CONNECTED, busAttachment.Disconnect());

			Assert.Equal(AllJoyn.QStatus.OK, busAttachment.Connect());

			Assert.True(busAttachment.IsConnected);

			Assert.Equal(AllJoyn.QStatus.OK, busAttachment.Disconnect());

			Assert.False(busAttachment.IsConnected);

			busAttachment.Stop();
			busAttachment.Join();
		}

		[Fact]
		public void DeleteInterface()
		{
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;

			AllJoyn.InterfaceDescription testIntf = null;
			status = busAttachment.CreateInterface("org.alljoyn.test.BusAttachment", out testIntf);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.NotNull(testIntf);
			status = busAttachment.DeleteInterface(testIntf);
			Assert.Equal(AllJoyn.QStatus.OK, status);
		}

		[Fact]
		public void StartAndStop()
		{
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;

			status = busAttachment.Start();
			Assert.Equal(AllJoyn.QStatus.OK, status);
			status = busAttachment.Stop();
			Assert.Equal(AllJoyn.QStatus.OK, status);
			status = busAttachment.Join();
			Assert.Equal(AllJoyn.QStatus.OK, status);

		}

		[Fact, Trait("busattachment", "registersignalhandler")]
		public void RegisterSignalHandler()
		{
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;

			// create the interface description
			AllJoyn.InterfaceDescription testIntf = null;
			status = busAttachment.CreateInterface("org.alljoyn.test.BusAttachment", out testIntf);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.NotNull(testIntf);

			// add the signal member to the interface
			status = testIntf.AddSignal("testSignal", "s", "msg", 0);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// activate the interface
			testIntf.Activate();

			// start the bus attachment
			status = busAttachment.Start();
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// connect to the bus
			status = busAttachment.Connect(AllJoynTestCommon.GetConnectSpec());

			Assert.Equal(AllJoyn.QStatus.OK, status);

			// create the bus object &
			// add the interface to the bus object
			TestBusObject testBusObject = new TestBusObject(busAttachment, "/test");
			busAttachment.RegisterBusObject(testBusObject);

			// get the signal member from the interface description
			AllJoyn.InterfaceDescription.Member testSignalMember = testIntf.GetMember("testSignal");

			// register the signal handler
			status = busAttachment.RegisterSignalHandler(this.TestSignalHandlerOne, testSignalMember, null);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// add match for the signal
			status = busAttachment.AddMatch("type='signal',member='testSignal'");
			Assert.Equal(AllJoyn.QStatus.OK, status);

			handledSignalsOne = false;
			signalOneMsg = null;

			// send a signal
			testBusObject.SendTestSignal("test msg");

			//wait to see if we receive the signal
			WaitEventOne(TimeSpan.FromSeconds(10));
			//System.Threading.Thread.Sleep(10000);

			Assert.Equal(true, handledSignalsOne);
			Assert.Equal("test msg", signalOneMsg);

			handledSignalsOne = false;
			signalOneMsg = "";

			testBusObject.SendTestSignal2("test msg");

			WaitEventOne(TimeSpan.FromSeconds(2));

			Assert.Equal(true, handledSignalsOne);
			Assert.Equal("test msg", signalOneMsg);
		}

		[Fact]
		public void DBusProxyObj()
		{
			Assert.Equal(AllJoyn.QStatus.OK, busAttachment.Start());
			Assert.Equal(AllJoyn.QStatus.OK, busAttachment.Connect());

			AllJoyn.ProxyBusObject dbusBusObj = busAttachment.DBusProxyObj;

			String wellKnownName = "org.alljoyn.test.BusAttachment";
			AllJoyn.MsgArg msgArg = new AllJoyn.MsgArg(2);
			msgArg[0] = wellKnownName;
			msgArg[1] = (uint)(AllJoyn.DBus.NameFlags.AllowReplacement | AllJoyn.DBus.NameFlags.DoNotQueue | AllJoyn.DBus.NameFlags.ReplaceExisting);

			AllJoyn.Message replyMsg = new AllJoyn.Message(busAttachment);

			Assert.Equal(AllJoyn.QStatus.OK, dbusBusObj.MethodCall("org.freedesktop.DBus", "RequestName", msgArg, replyMsg, 5000, 0));
			// TODO this keeps returning 4 which is DBUS_REQUEST_NAME_REPLY_EXISTS when it should return 1 DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER
			// unknown if this is an error only in Unity or in core code.  I suspect this issue is also in alljoyn_core code.

			AllJoyn.MsgArg nho_msgArg = new AllJoyn.MsgArg();
			nho_msgArg = wellKnownName;
			Assert.Equal(AllJoyn.QStatus.OK, dbusBusObj.MethodCall("org.freedesktop.DBus", "NameHasOwner", nho_msgArg, replyMsg, 5000, 0));
			Assert.True((bool)replyMsg[0]);

			Assert.Equal(AllJoyn.QStatus.OK, dbusBusObj.MethodCall("org.freedesktop.DBus", "ListNames", AllJoyn.MsgArg.Zero, replyMsg, 5000, 0));
			string[] sa = (string[])replyMsg[0];
			//the wellknown Name should be found in the list of strings returned.
			Assert.True(Array.IndexOf(sa, wellKnownName) > -1);
		}

		[Fact, Trait("busattachment", "unregistersignalhandler")]
		public void UnregisterSignalHandler()
		{
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;

			// create the interface description
			AllJoyn.InterfaceDescription testIntf = null;
			status = busAttachment.CreateInterface("org.alljoyn.test.BusAttachment", out testIntf);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.NotNull(testIntf);

			// add the signal member to the interface
			status = testIntf.AddSignal("testSignal", "s", "msg", 0);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// activate the interface
			testIntf.Activate();

			// start the bus attachment
			status = busAttachment.Start();
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// connect to the bus
			status = busAttachment.Connect(AllJoynTestCommon.GetConnectSpec());
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// create the bus object &
			// add the interface to the bus object
			TestBusObject testBusObject = new TestBusObject(busAttachment, "/test");
			busAttachment.RegisterBusObject(testBusObject);

			// get the signal member from the interface description
			AllJoyn.InterfaceDescription.Member testSignalMember = testIntf.GetMember("testSignal");

			// register both signal handlers
			status = busAttachment.RegisterSignalHandler(this.TestSignalHandlerOne, testSignalMember, null);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			status = busAttachment.RegisterSignalHandler(this.TestSignalHandlerTwo, testSignalMember, null);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// add match for the signal
			status = busAttachment.AddMatch("type='signal',member='testSignal'");
			Assert.Equal(AllJoyn.QStatus.OK, status);

			handledSignalsOne = false;
			handledSignalsTwo = false;
			signalOneMsg = null;
			signalTwoMsg = null;

			// send a signal
			testBusObject.SendTestSignal("test msg");

			WaitEventOne(TimeSpan.FromSeconds(2));
			WaitEventTwo(TimeSpan.FromSeconds(2));

			// make sure that both handlers got the signal
			Assert.Equal(true, handledSignalsOne);
			Assert.Equal("test msg", signalOneMsg);
			Assert.Equal(true, handledSignalsTwo);
			Assert.Equal("test msg", signalTwoMsg);

			// now unregister one handler & make sure it doesn't receive the signal
			handledSignalsOne = false;
			handledSignalsTwo = false;
			signalOneMsg = null;
			signalTwoMsg = null;

			status = busAttachment.UnregisterSignalHandler(this.TestSignalHandlerOne, testSignalMember, null);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// send another signal
			testBusObject.SendTestSignal("test msg");

			// wait to see if we receive the signal
			WaitEventTwo(TimeSpan.FromSeconds(2));

			// make sure that only the second handler got the signal
			Assert.Equal(false, handledSignalsOne);
			Assert.Null(signalOneMsg);
			Assert.Equal(true, handledSignalsTwo);
			Assert.Equal("test msg", signalTwoMsg);
		}

		[Fact]
		public void PingSelf()
		{
			Assert.Equal(AllJoyn.QStatus.OK, busAttachment.Start());
			Assert.Equal(AllJoyn.QStatus.OK, busAttachment.Connect());
			Assert.Equal(AllJoyn.QStatus.OK, busAttachment.Ping(busAttachment.UniqueName, 1000));

			busAttachment.Stop();
			busAttachment.Join();
		}

		[Fact]
		public void PingOtherOnSameBus()
		{
			Assert.Equal(AllJoyn.QStatus.OK, busAttachment.Start());
			Assert.Equal(AllJoyn.QStatus.OK, busAttachment.Connect());

			AllJoyn.BusAttachment otherBusAttachment = null;
			otherBusAttachment = new AllJoyn.BusAttachment("OtherBusAttachmentTest", false);

			Assert.Equal(AllJoyn.QStatus.OK, otherBusAttachment.Start());
			Assert.Equal(AllJoyn.QStatus.OK, otherBusAttachment.Connect());

			Assert.Equal(AllJoyn.QStatus.OK, busAttachment.Ping(otherBusAttachment.UniqueName, 1000));

			otherBusAttachment.Stop();
			otherBusAttachment.Join();

			busAttachment.Stop();
			busAttachment.Join();
		}

		private void WaitEventOne(TimeSpan timeout)
		{
			notifyEventOne.WaitOne(timeout);
			notifyEventOne.Reset();
		}

		private void WaitEventTwo(TimeSpan timeout)
		{
			notifyEventTwo.WaitOne(timeout);
			notifyEventTwo.Reset();
		}

		private void NotifyEventOne()
		{
			notifyEventOne.Set();
		}

		private void NotifyEventTwo()
		{
			notifyEventTwo.Set();
		}

		public void TestSignalHandlerOne(AllJoyn.InterfaceDescription.Member member, string srcPath, AllJoyn.Message message)
		{
			signalOneMsg = message[0];
			// mark that the signal was received
			handledSignalsOne = true;
			NotifyEventOne();
		}

		public void TestSignalHandlerTwo(AllJoyn.InterfaceDescription.Member member, string srcPath, AllJoyn.Message message)
		{
			signalTwoMsg = message[0];
			// mark that the signal was received
			handledSignalsTwo = true;
			NotifyEventTwo();
		}

		class TestBusObject : AllJoyn.BusObject
		{
			private AllJoyn.InterfaceDescription.Member testSignalMember;

			public TestBusObject(AllJoyn.BusAttachment bus, string path)
				: base(path, false)
			{
				AllJoyn.InterfaceDescription testIntf = bus.GetInterface("org.alljoyn.test.BusAttachment");
				AllJoyn.QStatus status = AddInterface(testIntf);
				Assert.Equal(AllJoyn.QStatus.OK, status);

				testSignalMember = testIntf.GetMember("testSignal");
			}

			public void SendTestSignal(String msg)
			{
				AllJoyn.MsgArg payload = new AllJoyn.MsgArg();
				payload.Set(msg);
				AllJoyn.QStatus status = Signal(null, 0, testSignalMember, payload, 0, 64);
				Assert.Equal(AllJoyn.QStatus.OK, status);
			}
//Still test sending signals using obsolete Signal calls till the obsolete method is removed.
#pragma warning disable 618
			public void SendTestSignal2(String msg)
			{
				AllJoyn.MsgArgs payload = new AllJoyn.MsgArgs(1);
				payload[0].Set(msg);
				AllJoyn.QStatus status = Signal(null, 0, testSignalMember, payload, 0, 64);
				Assert.Equal(AllJoyn.QStatus.OK, status);
			}
#pragma warning restore 618
		}
	}
}
