//-----------------------------------------------------------------------
// <copyright file="BusObjectTest.cs" company="AllSeen Alliance.">
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
using System.Threading;
using AllJoynUnity;
using Xunit;

namespace AllJoynUnityTest
{
	public class BusObjectTest : IDisposable
	{
		const string INTERFACE_NAME = "org.alljoyn.test.BusObjectTest";
		const string OBJECT_NAME = "org.alljoyn.test.BusObjectTest";
		const string OBJECT_PATH = "/org/alljoyn/test/BusObjectTest";

		public TimeSpan MaxWaitTime = TimeSpan.FromSeconds(15);

		private AutoResetEvent notifyEvent;

		private AllJoyn.BusAttachment bus = null;
		private AllJoyn.BusAttachment servicebus = null;

		private AllJoyn.ProxyBusObject proxyBusObject = null;

		bool objectRegistered;
		bool objectUnregistered;
		bool nameOwnerChangedFlag;

		private AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;

		private bool disposed = false;

		public BusObjectTest()
		{
			notifyEvent = new AutoResetEvent(false);

			// create+start+connect bus attachment
			bus = new AllJoyn.BusAttachment("BusObjectTest", true);
			Assert.NotNull(bus);

			status = bus.Start();
			Assert.Equal(AllJoyn.QStatus.OK, status);

			status = bus.Connect(AllJoynTestCommon.GetConnectSpec());
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// create+start+connect bus attachment
			servicebus = new AllJoyn.BusAttachment("BusObjectTestService", true);
			Assert.NotNull(servicebus);

			status = servicebus.Start();
			Assert.Equal(AllJoyn.QStatus.OK, status);

			status = servicebus.Connect(AllJoynTestCommon.GetConnectSpec());
			Assert.Equal(AllJoyn.QStatus.OK, status);
		}

		protected virtual void Dispose(bool disposing)
		{
			if (!this.disposed)
			{
				// additional clean up before disposing
				// make sure the ObjectName is not advertised or owned by the bus
				// between runs these are run regardless we actually don't expect
				// these names to be owned between runs this is here just in case
				// a test fails we want to make sure the names are cleared from
				// the daemon
				bus.ReleaseName(OBJECT_NAME);
				servicebus.ReleaseName(OBJECT_NAME);

				if (disposing)
				{
					if (proxyBusObject != null)
					{
						proxyBusObject.Dispose();
					}
					bus.Dispose();
					servicebus.Dispose();
				}
				disposed = true;
			}
		}

		~BusObjectTest()
		{
			Dispose(false);
		}

		public void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		[Fact]
		public void ObjectRegisteredUnregistered()
		{
			// create the bus object
			TestBusObject testBusObject = new TestBusObject(bus, OBJECT_PATH, this);
			objectRegistered = false;
			objectUnregistered = false;

			// test registering the bus object
			status = bus.RegisterBusObject(testBusObject);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Wait(MaxWaitTime);
			Assert.Equal(true, objectRegistered);

			// test unregistering the bus object
			bus.UnregisterBusObject(testBusObject);
			Wait(MaxWaitTime);
			Assert.Equal(true, objectUnregistered);
		}

		[Fact]
		public void AddMethodHandler()
		{
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;

			// create+activate the interface
			AllJoyn.InterfaceDescription testIntf = null;
			status = servicebus.CreateInterface(INTERFACE_NAME, out testIntf);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.NotNull(testIntf);

			status = testIntf.AddMember(AllJoyn.Message.Type.MethodCall, "ping", "s", "s", "in,out");
			Assert.Equal(AllJoyn.QStatus.OK, status);

			testIntf.Activate();

			// register bus listener
			AllJoyn.BusListener testBusListener = new TestBusListener(this);
			servicebus.RegisterBusListener(testBusListener);

			// create the bus object
			// the MethodTestBusObject constructor adds the interface & a handler for the ping method
			MethodTestBusObject methodTestBusObject = new MethodTestBusObject(servicebus, OBJECT_PATH);

			// register the bus object
			status = servicebus.RegisterBusObject(methodTestBusObject);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// request name
			nameOwnerChangedFlag = false;
			status = servicebus.RequestName(OBJECT_NAME, AllJoyn.DBus.NameFlags.ReplaceExisting | AllJoyn.DBus.NameFlags.DoNotQueue);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Wait(MaxWaitTime);
			Assert.Equal(true, nameOwnerChangedFlag);

			// create+activate the interface
			AllJoyn.InterfaceDescription iFace = null;
			status = bus.CreateInterface(INTERFACE_NAME, out iFace);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.NotNull(iFace);

			status = iFace.AddMember(AllJoyn.Message.Type.MethodCall, "ping", "s", "s", "in,out");
			Assert.Equal(AllJoyn.QStatus.OK, status);

			iFace.Activate();

			proxyBusObject = new AllJoyn.ProxyBusObject(bus, OBJECT_NAME, OBJECT_PATH, 0);
			status = proxyBusObject.AddInterface(iFace);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			AllJoyn.MsgArg input = new AllJoyn.MsgArg();
			input.Set("AllJoyn");
			AllJoyn.Message replyMsg = new AllJoyn.Message(bus);
			status = proxyBusObject.MethodCall(INTERFACE_NAME, "ping", input, replyMsg, 5000, 0);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.Equal("AllJoyn", (string)replyMsg[0]);

//continue testing obsolete method calls till they are removed.
#pragma warning disable 618
			AllJoyn.MsgArg input1 = new AllJoyn.MsgArg();
			input1.Set("AllJoyn");
			AllJoyn.Message replyMsg1 = new AllJoyn.Message(bus);
			status = proxyBusObject.MethodCallSynch(INTERFACE_NAME, "ping", input1, replyMsg1, 5000, 0);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.Equal("AllJoyn", (string)replyMsg1[0]);

			AllJoyn.MsgArgs input2 = new AllJoyn.MsgArgs(1);
			input2[0].Set("AllJoyn");
			AllJoyn.Message replyMsg2 = new AllJoyn.Message(bus);
			status = proxyBusObject.MethodCallSynch(INTERFACE_NAME, "ping", input2, replyMsg2, 5000, 0);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.Equal("AllJoyn", (string)replyMsg2[0]);
#pragma warning restore 618

			methodTestBusObject.Dispose();
		}

		class DingBusObject : AllJoyn.BusObject
		{
			public DingBusObject(AllJoyn.BusAttachment bus, string path)
				: base(path)
			{
				// add the interface to the bus object
				AllJoyn.InterfaceDescription testIntf = bus.GetInterface(INTERFACE_NAME);
				AllJoyn.QStatus status = AddInterface(testIntf);
				Assert.Equal(AllJoyn.QStatus.OK, status);

				// register a method handler for the ping method
				AllJoyn.InterfaceDescription.Member dingMember = testIntf.GetMember("ding");
				status = AddMethodHandler(dingMember, this.Ding);
				Assert.Equal(AllJoyn.QStatus.OK, status);
			}

			protected void Ding(AllJoyn.InterfaceDescription.Member member, AllJoyn.Message message)
			{
				AllJoyn.MsgArg outArgs = new AllJoyn.MsgArg();
				outArgs = "Hello from Ding.";

				AllJoyn.QStatus status = MethodReply(message, outArgs);
				Assert.Equal(AllJoyn.QStatus.OK, status);
			}
		}

		[Fact]
		public void MethodNoInputParams()
		{
			// create+activate the interface
			AllJoyn.InterfaceDescription testIntf = null;
			status = servicebus.CreateInterface(INTERFACE_NAME, out testIntf);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.NotNull(testIntf);

			status = testIntf.AddMember(AllJoyn.Message.Type.MethodCall, "ding", "", "s", "out");
			Assert.Equal(AllJoyn.QStatus.OK, status);

			testIntf.Activate();

			// register bus listener
			AllJoyn.BusListener testBusListener = new TestBusListener(this);
			servicebus.RegisterBusListener(testBusListener);

			// create the bus object
			// the dingBusObject constructor adds the interface & a handler for the ping method
			DingBusObject dingBusObject = new DingBusObject(servicebus, OBJECT_PATH);

			// register the bus object
			status = servicebus.RegisterBusObject(dingBusObject);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// request name
			nameOwnerChangedFlag = false;
			status = servicebus.RequestName(OBJECT_NAME, AllJoyn.DBus.NameFlags.ReplaceExisting | AllJoyn.DBus.NameFlags.DoNotQueue | AllJoyn.DBus.NameFlags.AllowReplacement);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Wait(MaxWaitTime);
			Assert.Equal(true, nameOwnerChangedFlag);

			///////////////////////////////////////////////////////////

			// create+activate the interface
			AllJoyn.InterfaceDescription iFace = null;
			status = bus.CreateInterface(INTERFACE_NAME, out iFace);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.NotNull(iFace);

			status = iFace.AddMethod("ding", "", "s", "in,out");
			Assert.Equal(AllJoyn.QStatus.OK, status);

			iFace.Activate();

			proxyBusObject = new AllJoyn.ProxyBusObject(bus, OBJECT_NAME, OBJECT_PATH, 0);
			status = proxyBusObject.AddInterface(iFace);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			AllJoyn.Message replyMsg = new AllJoyn.Message(bus);
			status = proxyBusObject.MethodCall(INTERFACE_NAME, "ding", AllJoyn.MsgArg.Zero, replyMsg, 5000, 0);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.Equal("Hello from Ding.", (string)replyMsg[0]);

			//continue testing obsolete method calls till they are removed.
#pragma warning disable 618
			AllJoyn.Message replyMsg1 = new AllJoyn.Message(bus);
			status = proxyBusObject.MethodCallSynch(INTERFACE_NAME, "ding", AllJoyn.MsgArg.Zero, replyMsg1, 5000, 0);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.Equal("Hello from Ding.", (string)replyMsg1[0]);

			AllJoyn.Message replyMsg2 = new AllJoyn.Message(bus);
			status = proxyBusObject.MethodCallSynch(INTERFACE_NAME, "ding", AllJoyn.MsgArg.Zero, replyMsg2, 5000, 0);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.Equal("Hello from Ding.", (string)replyMsg2[0]);
#pragma warning restore 618

			dingBusObject.Dispose();
		}

		private void Wait(TimeSpan timeout)
		{
			notifyEvent.WaitOne(timeout);
			notifyEvent.Reset();
		}

		class TestBusListener : AllJoyn.BusListener
		{
			BusObjectTest _busObjectTest;

			public TestBusListener(BusObjectTest busObjectTest)
			{
				this._busObjectTest = busObjectTest;
			}

			protected override void NameOwnerChanged(string busName, string previousOwner, string newOwner)
			{
				_busObjectTest.nameOwnerChangedFlag = true;
				Notify();
			}

			private void Notify()
			{
				_busObjectTest.notifyEvent.Set();
			}
		}

		class TestBusObject : AllJoyn.BusObject
		{
			BusObjectTest _busObjectTest;

			public TestBusObject(AllJoyn.BusAttachment bus, string path, BusObjectTest busObjectTest)
				: base(path, false)
			{
				_busObjectTest = busObjectTest;
			}

			protected override void OnObjectRegistered()
			{
				_busObjectTest.objectRegistered = true;
				Notify();
			}

			protected override void OnObjectUnregistered()
			{
				_busObjectTest.objectUnregistered = true;
				Notify();
			}

			private void Notify()
			{
				_busObjectTest.notifyEvent.Set();
			}
		}

		class MethodTestBusObject : AllJoyn.BusObject
		{
			public MethodTestBusObject(AllJoyn.BusAttachment bus, string path)
				: base(path, false)
			{
				// add the interface to the bus object
				AllJoyn.InterfaceDescription testIntf = bus.GetInterface(INTERFACE_NAME);
				AllJoyn.QStatus status = AddInterface(testIntf);
				Assert.Equal(AllJoyn.QStatus.OK, status);

				// register a method handler for the ping method
				AllJoyn.InterfaceDescription.Member pingMember = testIntf.GetMember("ping");
				status = AddMethodHandler(pingMember, this.Ping);
				Assert.Equal(AllJoyn.QStatus.OK, status);
			}

			protected void Ping(AllJoyn.InterfaceDescription.Member member, AllJoyn.Message message)
			{
				string outStr = (string)message[0];
				AllJoyn.MsgArg outArgs = new AllJoyn.MsgArg();
				outArgs = outStr;

				AllJoyn.QStatus status = MethodReply(message, outArgs);
				Assert.Equal(AllJoyn.QStatus.OK, status);

//Continue testing obsolete method calls till they are removed.
#pragma warning disable 618
				AllJoyn.MsgArgs outArgs2 = new AllJoyn.MsgArgs(1);
				outArgs2[0] = outStr;

				status = MethodReply(message, outArgs2);
				Assert.Equal(AllJoyn.QStatus.OK, status);
#pragma warning restore 618
			}
		}
	}


}
