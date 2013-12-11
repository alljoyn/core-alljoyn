//-----------------------------------------------------------------------
// <copyright file="SignalsTest.cs" company="AllSeen Alliance.">
// Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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
	public class SignalsTest : IDisposable
	{
		AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;
		AllJoyn.BusAttachment bus = null;

		private bool disposed = false;

		public SignalsTest()
		{
			// create+start+connect bus attachment
			bus = new AllJoyn.BusAttachment("SignalsTest", true);
			Assert.NotNull(bus);

			status = bus.Start();
			Assert.Equal(AllJoyn.QStatus.OK, status);

			status = bus.Connect(AllJoynTestCommon.GetConnectSpec());
			Assert.Equal(AllJoyn.QStatus.OK, status);
		}

		~SignalsTest()
		{
			Dispose(false);
		}

		protected virtual void Dispose(bool disposing)
		{
			if (!this.disposed)
			{
				if (disposing)
				{
					bus.Dispose();
				}
				disposed = true;
			}
		}

		public void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		class TestBusObject : AllJoyn.BusObject
		{
			public TestBusObject(string path): base(path)
			{

			}

			public AllJoyn.QStatus SendTestSignal(string destination, uint sessionId,
												AllJoyn.InterfaceDescription.Member member,
												AllJoyn.MsgArg args, ushort timeToLife, byte flags,
												AllJoyn.Message msg) {
				return Signal(destination, sessionId, member, args, timeToLife, flags, msg);
			}
		}
		[Fact]
		public void RegisterUnregisterSessionlessSignals()
		{
			AllJoyn.InterfaceDescription testIntf;
			Assert.Equal(AllJoyn.QStatus.OK, bus.CreateInterface("org.alljoyn.test.signalstest", out testIntf));
			Assert.Equal(AllJoyn.QStatus.OK, testIntf.AddSignal("testSignal", "s", "newName"));
			testIntf.Activate();

			TestBusObject testObj = new TestBusObject("/org/alljoyn/test/signal");
			Assert.Equal(AllJoyn.QStatus.OK, testObj.AddInterface(testIntf));
			Assert.Equal(AllJoyn.QStatus.OK, bus.RegisterBusObject(testObj));

			AllJoyn.InterfaceDescription.Member mySignalMember = testIntf.GetMember("testSignal");

			Assert.Equal(AllJoyn.QStatus.OK, bus.AddMatch("type='signal',sessionless='t',interface='org.alljoyn.test.signalstest,member='testSignal'"));

			AllJoyn.Message msg = new AllJoyn.Message(bus);
			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("s", "AllJoyn"));
			Assert.Equal(AllJoyn.QStatus.OK, testObj.SendTestSignal("", 0, mySignalMember, arg, 0, AllJoyn.ALLJOYN_FLAG_SESSIONLESS, msg));

			Assert.Equal(AllJoyn.QStatus.OK, testObj.CancelSessionlessMessage(msg.CallSerial));

			Assert.Equal(AllJoyn.QStatus.OK, testObj.SendTestSignal("", 0, mySignalMember, arg, 0, AllJoyn.ALLJOYN_FLAG_SESSIONLESS, msg));
			Assert.Equal(AllJoyn.QStatus.OK, testObj.CancelSessionlessMessage(msg));
		}
	};
}