//-----------------------------------------------------------------------
// <copyright file="MessageTest.cs" company="AllSeen Alliance.">
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
	public class MessageTest : IDisposable
	{
		private static string WELLKNOWN_NAME = "org.alljoyn.test.MessageTest";
		private static string OBJECT_PATH = "/org/alljoyn/test/MessageTest";
		private static string INTERFACE_NAME = "org.alljoyn.test.MessageTest";

		private AllJoyn.BusAttachment serviceBus = null;
		private AllJoyn.BusAttachment clientBus = null;

		private AutoResetEvent notifyEvent = new AutoResetEvent(false);
		private bool _nameOwnerChangedFlag;

		private void Wait(TimeSpan timeout)
		{
			notifyEvent.WaitOne(timeout);
			notifyEvent.Reset();
		}

		private void Notify()
		{
			notifyEvent.Set();
		}

		private bool disposed = false;

		public MessageTest()
		{
			serviceBus = new AllJoyn.BusAttachment("MessageTest", false);
			Assert.NotNull(serviceBus);

			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.Start());
			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.Connect(AllJoynTestCommon.GetConnectSpec()));

			//SetUp Client
			//start client BusAttachment
			clientBus = new AllJoyn.BusAttachment("MessageTestClient", true);
			Assert.Equal(AllJoyn.QStatus.OK, clientBus.Start());
			Assert.Equal(AllJoyn.QStatus.OK, clientBus.Connect(AllJoynTestCommon.GetConnectSpec()));
		}

		~MessageTest()
		{
			Dispose(false);
		}

		protected virtual void Dispose(bool disposing)
		{
			if (!this.disposed)
			{
				if (disposing)
				{
					serviceBus.Dispose();
					clientBus.Dispose();
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
		public void CreateDispose()
		{
			AllJoyn.Message message = new AllJoyn.Message(serviceBus);

			message.Dispose();
		}


		class TestBusListener : AllJoyn.BusListener
		{
			private MessageTest _messageTest;

			public TestBusListener(MessageTest messageTest)
			{
				_messageTest = messageTest;
			}

			protected override void NameOwnerChanged(string busName, string previousOwner, string newOwner)
			{
				// in terms of the unit tests we are only interested in the NameOwnerChanged signal for
				// well-known names.
				if (busName.Equals(MessageTest.WELLKNOWN_NAME))
				{
					_messageTest._nameOwnerChangedFlag = true;
					_messageTest.Notify();
				}
			}
		}

		class MessageTestBusObject : AllJoyn.BusObject
		{
			public MessageTestBusObject(string path)
				: base(path, false)
			{
			}

			protected override void OnObjectRegistered()
			{
			}

			protected override void OnObjectUnregistered()
			{
			}

			public void Ping(AllJoyn.InterfaceDescription.Member member, AllJoyn.Message message)
			{
				Assert.Equal(OBJECT_PATH, message.ObjectPath);
				Assert.Equal(INTERFACE_NAME, message.Interface);
				Assert.Equal("ping", message.MemberName);
				string outStr = (string)message[0];
				AllJoyn.MsgArg outArgs = new AllJoyn.MsgArg();
				outArgs = outStr;

				AllJoyn.QStatus status = MethodReply(message, outArgs);
				Assert.Equal(AllJoyn.QStatus.OK, status);
			}
		}

		[Fact]
		public void GetArgs()
		{
			TestBusListener testBusListener = new TestBusListener(this);
			serviceBus.RegisterBusListener(testBusListener);

			//Create and activate the service Interface
			AllJoyn.InterfaceDescription testIntf = null;
			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.CreateInterface(INTERFACE_NAME, out testIntf));
			Assert.NotNull(testIntf);
			Assert.Equal(AllJoyn.QStatus.OK, testIntf.AddMember(AllJoyn.Message.Type.MethodCall, "ping", "s", "s", "in,out"));
			testIntf.Activate();

			//create and register BusObject
			MessageTestBusObject busObj = new MessageTestBusObject(OBJECT_PATH);
			busObj.AddInterface(testIntf);
			AllJoyn.InterfaceDescription.Member ping;
			ping = testIntf.GetMember("ping");
			Assert.NotNull(ping);

			Assert.Equal(AllJoyn.QStatus.OK, busObj.AddMethodHandler(ping, busObj.Ping));

			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.RegisterBusObject(busObj));

			_nameOwnerChangedFlag = false;
			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.RequestName(WELLKNOWN_NAME, AllJoyn.DBus.NameFlags.ReplaceExisting |
																					AllJoyn.DBus.NameFlags.DoNotQueue |
																					AllJoyn.DBus.NameFlags.AllowReplacement));
			Wait(TimeSpan.FromSeconds(2));
			Assert.True(_nameOwnerChangedFlag);

			AllJoyn.ProxyBusObject proxyObj = new AllJoyn.ProxyBusObject(clientBus, WELLKNOWN_NAME, OBJECT_PATH, 0);

			Assert.Equal(AllJoyn.QStatus.OK, proxyObj.IntrospectRemoteObject());

			AllJoyn.Message reply = new AllJoyn.Message(clientBus);
			AllJoyn.MsgArg input = new AllJoyn.MsgArg("s", "AllJoyn");

			proxyObj.MethodCall(INTERFACE_NAME, "ping", input, reply, 25000, 0);

			//Actual tests for GetArg/GetArgs
			// call using GetArg specifying the array index
			Assert.Equal("AllJoyn", (string)reply.GetArg(0));
			// use the this[] operator call to get the MsgArg
			Assert.Equal("AllJoyn", (string)(reply[0]));
			// Return the MsgArgs note could be multiple values
			AllJoyn.MsgArg replyArg = reply.GetArgs();
			Assert.Equal(1, replyArg.Length);
			Assert.Equal("AllJoyn", (string)replyArg);
			// Parse the Message Drectly
			object replyString;
			Assert.Equal(AllJoyn.QStatus.OK, reply.GetArgs("s", out replyString));
			Assert.Equal("AllJoyn", (string)replyString);

			serviceBus.UnregisterBusListener(testBusListener);
			reply.Dispose();
			input.Dispose();
			proxyObj.Dispose();

			testBusListener.Dispose();
			busObj.Dispose();
		}

		[Fact]
		public void Properties()
		{
			TestBusListener testBusListener = new TestBusListener(this);
			serviceBus.RegisterBusListener(testBusListener);

			//Create and activate the service Interface
			AllJoyn.InterfaceDescription testIntf = null;
			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.CreateInterface(INTERFACE_NAME, out testIntf));
			Assert.NotNull(testIntf);
			Assert.Equal(AllJoyn.QStatus.OK, testIntf.AddMember(AllJoyn.Message.Type.MethodCall, "ping", "s", "s", "in,out"));
			testIntf.Activate();

			//create and register BusObject
			MessageTestBusObject busObj = new MessageTestBusObject(OBJECT_PATH);
			busObj.AddInterface(testIntf);
			AllJoyn.InterfaceDescription.Member ping;
			ping = testIntf.GetMember("ping");
			Assert.NotNull(ping);

			Assert.Equal(AllJoyn.QStatus.OK, busObj.AddMethodHandler(ping, busObj.Ping));

			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.RegisterBusObject(busObj));

			_nameOwnerChangedFlag = false;
			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.RequestName(WELLKNOWN_NAME, AllJoyn.DBus.NameFlags.ReplaceExisting |
																					AllJoyn.DBus.NameFlags.DoNotQueue |
																					AllJoyn.DBus.NameFlags.AllowReplacement));
			Wait(TimeSpan.FromSeconds(2));

			Assert.True(_nameOwnerChangedFlag);

			AllJoyn.ProxyBusObject proxyObj = new AllJoyn.ProxyBusObject(clientBus, INTERFACE_NAME, OBJECT_PATH, 0);

			Assert.Equal(AllJoyn.QStatus.OK, proxyObj.IntrospectRemoteObject());

			AllJoyn.Message reply = new AllJoyn.Message(clientBus);
			AllJoyn.MsgArg input = new AllJoyn.MsgArg("s", "AllJoyn");

			proxyObj.MethodCall(INTERFACE_NAME, "ping", input, reply, 25000, 0);

			// Actual tests for GetArg/GetArgs
			// check the message properties
			Assert.False(reply.IsBroadcastSignal);
			Assert.False(reply.IsGlobalBroadcast);
			Assert.False(reply.IsSessionless);
			Assert.False(reply.IsExpired());
			uint timeLeft;
			reply.IsExpired(out timeLeft);
			Assert.True(timeLeft > 0);
			Assert.False(reply.IsUnreliable);
			Assert.False(reply.IsEncrypted);
			// we don't expect any flags
			Assert.Equal((byte)0, reply.Flags);
			// no security is being used so there should be no security mechanism
			Assert.Equal("", reply.AuthMechanism);
			Assert.Equal(AllJoyn.Message.Type.MethodReturn, reply.MessageType);
			// The serial is unknown before hand but it should not be zero
			Assert.NotEqual<uint>(0u, reply.CallSerial);
			Assert.NotEqual<uint>(0u, reply.ReplySerial);
			// A method return does not have an Object Path
			Assert.Equal("", reply.ObjectPath);
			// A method return does not have an interface specified
			Assert.Equal("", reply.Interface);
			// The member name is not specified on a message return
			Assert.Equal("", reply.MemberName);
			// TODO possible error the documentation for Sender states it should
			// be returning the well-known name however in this case it is
			// returning the unique name of the sender.
			Assert.Equal(serviceBus.UniqueName, reply.Sender);
			Assert.Equal(clientBus.UniqueName, reply.ReceiveEndPointName);
			Assert.Equal(clientBus.UniqueName, reply.Destination);
			Assert.Equal(0u, reply.CompressionToken);
			// no session set up for this test Session Id should be 0
			Assert.Equal(0u, reply.SessionId);
			String errorMsg;
			// TODO produce test that generates actual error Message
			Assert.Null(reply.GetErrorName(out errorMsg));
			Assert.Equal("", errorMsg);
			// The  ToString method only returns a string when running debug code
#if DEBUG
			Assert.True(reply.ToString().StartsWith("<message endianness="));
			Assert.True(reply.ToString().Contains("<header field=\"REPLY_SERIAL\">"));
			Assert.True(reply.ToString().Contains("<header field=\"DESTINATION\">"));
			Assert.True(reply.ToString().Contains("<header field=\"SENDER\">"));
			Assert.True(reply.ToString().Contains("<header field=\"SIGNATURE\">"));
			Assert.True(reply.ToString().Contains("<signature>s</signature>"));
			Assert.True(reply.ToString().Contains("<string>AllJoyn</string>"));
			Assert.True(reply.ToString().EndsWith("</message>"));

			// this call to description should return 'METHID_RET[<reply serial>](s)'
			Assert.True(reply.Description.StartsWith("METHOD_RET["));
#else
			Assert.Equal("", reply.ToString());
			Assert.Equal("", reply.Description);
#endif
			// TODO figure out a good way to test the TimeStamp property
			//reply.TimeStamp

			// CleanUp
			serviceBus.UnregisterBusListener(testBusListener);
			reply.Dispose();
			input.Dispose();
			proxyObj.Dispose();

			testBusListener.Dispose();
			busObj.Dispose();
		}
	}
}