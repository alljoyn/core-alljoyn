//-----------------------------------------------------------------------
// <copyright file="ObjectSecurityTest.cs" company="AllSeen Alliance.">
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
using System.Collections.Generic;
using System.Text;
using AllJoynUnity;
using Xunit;

namespace AllJoynUnityTest
{
	public class ObjectSecurityTest : IDisposable
	{
		const string INTERFACE_NAME = "org.alljoyn.test.cs.objsecurity";
		const string OBJECT_PATH = "/org/alljoyn/test/objsecurity";
		const string WELLKNOWN_NAME = "org.alljoyn.test.cs.objsecurity";

		private AllJoyn.BusAttachment serviceBus;
		private AllJoyn.BusAttachment clientBus;
		private TestBusObject busObject;


		public struct Authflags
		{
			public bool requestCreds_service;
			public bool authComplete_serivce;
			public bool verifyCreds_service;
			public bool securityViolation_service;

			public bool requestCreds_client;
			public bool authComplete_client;
			public bool verifyCreds_client;
			public bool securityViolation_client;
		}

		public Authflags authflags;
		public bool nameOwnerChangedFlag;

		//thread to notify when callbacks have occured.
		private AutoResetEvent notifyEvent = new AutoResetEvent(false);
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

		public ObjectSecurityTest()
		{
			serviceBus = new AllJoyn.BusAttachment("AuthListenerTestService", false);

			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.Start());
			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.Connect(AllJoynTestCommon.GetConnectSpec()));

			clientBus = new AllJoyn.BusAttachment("AuthListenerTestClient", false);

			Assert.Equal(AllJoyn.QStatus.OK, clientBus.Start());
			Assert.Equal(AllJoyn.QStatus.OK, clientBus.Connect(AllJoynTestCommon.GetConnectSpec()));
		}

		~ObjectSecurityTest()
		{
			Dispose(false);
		}

		protected virtual void Dispose(bool disposing)
		{
			if (!this.disposed)
			{
				// additional clean up before disposing
				serviceBus.Stop();
				serviceBus.Join();

				clientBus.Stop();
				clientBus.Join();

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

		void ResetAuthFlags()
		{
			authflags.requestCreds_service = false;
			authflags.authComplete_serivce = false;
			authflags.verifyCreds_service = false;
			authflags.securityViolation_service = false;

			authflags.requestCreds_client = false;
			authflags.authComplete_client = false;
			authflags.verifyCreds_client = false;
			authflags.securityViolation_client = false;
		}

		class TestBusListener : AllJoyn.BusListener
		{
			ObjectSecurityTest _objectSecurityTest;

			public TestBusListener(ObjectSecurityTest objectSecurityTest)
			{
				this._objectSecurityTest = objectSecurityTest;
			}

			protected override void NameOwnerChanged(string busName, string previousOwner, string newOwner)
			{
				_objectSecurityTest.nameOwnerChangedFlag = true;
				_objectSecurityTest.Notify();
			}
		}

		class TestBusObject : AllJoyn.BusObject
		{
			public TestBusObject(string path) : base(path) { }

			public void Ping(AllJoyn.InterfaceDescription.Member member, AllJoyn.Message message)
			{
				string outStr = (string)message[0];
				AllJoyn.MsgArg outArgs = new AllJoyn.MsgArg();
				outArgs = outStr;

				AllJoyn.QStatus status = MethodReply(message, outArgs);
				Assert.Equal(AllJoyn.QStatus.OK, status);
			}
		}


		public AllJoyn.QStatus SetUpAuthService()
		{
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;

			TestBusListener busListener = new TestBusListener(this);
			serviceBus.RegisterBusListener(busListener);

			busObject = new TestBusObject(OBJECT_PATH);
			AllJoyn.InterfaceDescription ifaceDescritpion = serviceBus.GetInterface(INTERFACE_NAME);
			status = busObject.AddInterface(ifaceDescritpion);
			if (!status)
			{
				return status;
			}

			AllJoyn.InterfaceDescription.Member ping_member = ifaceDescritpion.GetMethod("ping");

			status = busObject.AddMethodHandler(ping_member, busObject.Ping);
			if (!status)
			{
				return status;
			}

			status = serviceBus.RegisterBusObject(busObject, true);
			if (!status)
			{
				return status;
			}

			Assert.True(busObject.IsSecure);

			status = serviceBus.RequestName(WELLKNOWN_NAME,
				AllJoyn.DBus.NameFlags.ReplaceExisting |
				AllJoyn.DBus.NameFlags.DoNotQueue |
				AllJoyn.DBus.NameFlags.AllowReplacement);
			if (!status)
			{
				return status;
			}
			Wait(TimeSpan.FromSeconds(5));
			return status;
		}

		class SrpKeyx_service_authlistener : AllJoyn.AuthListener
		{
			private ObjectSecurityTest _objectSecurityTest;
			public SrpKeyx_service_authlistener(ObjectSecurityTest objectSecurityTest)
			{
				_objectSecurityTest = objectSecurityTest;
			}
			protected override bool RequestCredentials(string authMechanism, string peerName,
				ushort authCount, string userName, AllJoyn.Credentials.CredentialFlags credMask,
				AllJoyn.Credentials credentials)
			{
				if ((credMask & AllJoyn.Credentials.CredentialFlags.Password) == AllJoyn.Credentials.CredentialFlags.Password)
				{
					credentials.Password = "ABCDEFGH";
				}
				_objectSecurityTest.authflags.requestCreds_service = true;
				return true;
			}

			protected override bool VerifyCredentials(string authMechanism, string peerName, AllJoyn.Credentials credentials)
			{
				_objectSecurityTest.authflags.verifyCreds_service = true;
				return true;
			}

			protected override void SecurityViolation(AllJoyn.QStatus status, AllJoyn.Message msg)
			{
				_objectSecurityTest.authflags.securityViolation_service = true;
			}

			protected override void AuthenticationComplete(string authMechanism, string peerName, bool success)
			{
				_objectSecurityTest.authflags.authComplete_serivce = true;
			}
		}

		class SrpKeyx_client_authlistener : AllJoyn.AuthListener
		{
			private ObjectSecurityTest _objectSecurityTest;
			public SrpKeyx_client_authlistener(ObjectSecurityTest objectSecurityTest)
			{
				_objectSecurityTest = objectSecurityTest;
			}

			protected override bool RequestCredentials(string authMechanism, string peerName,
				ushort authCount, string userName, AllJoyn.Credentials.CredentialFlags credMask,
				AllJoyn.Credentials credentials)
			{
				if ((credMask & AllJoyn.Credentials.CredentialFlags.Password) == AllJoyn.Credentials.CredentialFlags.Password)
				{
					credentials.Password = "ABCDEFGH";
				}
				_objectSecurityTest.authflags.requestCreds_client = true;
				return true;
			}

			protected override bool VerifyCredentials(string authMechanism, string peerName, AllJoyn.Credentials credentials)
			{
				_objectSecurityTest.authflags.verifyCreds_client = true;
				return true;
			}

			protected override void SecurityViolation(AllJoyn.QStatus status, AllJoyn.Message msg)
			{
				_objectSecurityTest.authflags.securityViolation_client = true;
			}

			protected override void AuthenticationComplete(string authMechanism, string peerName, bool success)
			{
				_objectSecurityTest.authflags.authComplete_client = true;
			}
		}

		[Fact, Trait("objsecurity", "test")]
		public void insecure_interface_secureobject()
		{
			AllJoyn.InterfaceDescription service_intf = null;
			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.CreateInterface(INTERFACE_NAME, out service_intf));
			Assert.Equal(AllJoyn.QStatus.OK, service_intf.AddMethod("ping", "s", "s", "in,out"));
			service_intf.Activate();

			ResetAuthFlags();
			clientBus.ClearKeyStore();

			SrpKeyx_service_authlistener serviceAuthlistener = new SrpKeyx_service_authlistener(this);
			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", serviceAuthlistener, null, false));

			serviceBus.ClearKeyStore();

			Assert.Equal(AllJoyn.QStatus.OK, SetUpAuthService());

			SrpKeyx_client_authlistener clientAuthlistener = new SrpKeyx_client_authlistener(this);
			Assert.Equal(AllJoyn.QStatus.OK, clientBus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", clientAuthlistener, null, false));
			clientBus.ClearKeyStore();

			// create+activate the interface
			AllJoyn.QStatus status;
			AllJoyn.InterfaceDescription iFace = null;
			Assert.Equal(AllJoyn.QStatus.OK, clientBus.CreateInterface(INTERFACE_NAME, out iFace));
			Assert.NotNull(iFace);

			Assert.Equal(AllJoyn.QStatus.OK, iFace.AddMethod("ping", "s", "s", "in,out"));
			iFace.Activate();

			AllJoyn.ProxyBusObject proxyBusObject = new AllJoyn.ProxyBusObject(clientBus, WELLKNOWN_NAME, OBJECT_PATH, 0, true);
			Assert.NotNull(proxyBusObject);

			Assert.True(proxyBusObject.IsSecure);

			Assert.Equal(AllJoyn.QStatus.OK, proxyBusObject.AddInterface(iFace));

			AllJoyn.MsgArg input = new AllJoyn.MsgArg("s", "AllJoyn");
			AllJoyn.Message replyMsg = new AllJoyn.Message(clientBus);
			status = proxyBusObject.MethodCall(INTERFACE_NAME, "ping", input, replyMsg, 5000, 0);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.Equal("AllJoyn", (string)replyMsg[0]);

			Assert.True(authflags.requestCreds_service);
			Assert.True(authflags.authComplete_serivce);

			Assert.True(authflags.requestCreds_client);
			Assert.True(authflags.authComplete_client);

			clientAuthlistener.Dispose();
			serviceAuthlistener.Dispose();

			busObject.Dispose();

			proxyBusObject.Dispose();
		}

		[Fact, Trait("objsecurity", "test")]
		public void interface_security_policy_off_secureobject()
		{
			AllJoyn.InterfaceDescription service_intf = null;
			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.CreateInterface(INTERFACE_NAME, AllJoyn.InterfaceDescription.SecurityPolicy.Off, out service_intf));
			Assert.Equal(AllJoyn.QStatus.OK, service_intf.AddMethod("ping", "s", "s", "in,out"));
			service_intf.Activate();

			ResetAuthFlags();
			clientBus.ClearKeyStore();

			SrpKeyx_service_authlistener serviceAuthlistener = new SrpKeyx_service_authlistener(this);
			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", serviceAuthlistener, null, false));

			serviceBus.ClearKeyStore();

			Assert.Equal(AllJoyn.QStatus.OK, SetUpAuthService());

			SrpKeyx_client_authlistener clientAuthlistener = new SrpKeyx_client_authlistener(this);
			Assert.Equal(AllJoyn.QStatus.OK, clientBus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", clientAuthlistener, null, false));
			clientBus.ClearKeyStore();

			// create+activate the interface
			AllJoyn.QStatus status;
			AllJoyn.InterfaceDescription iFace = null;
			Assert.Equal(AllJoyn.QStatus.OK, clientBus.CreateInterface(INTERFACE_NAME, AllJoyn.InterfaceDescription.SecurityPolicy.Off, out iFace));
			Assert.NotNull(iFace);

			Assert.Equal(AllJoyn.QStatus.OK, iFace.AddMethod("ping", "s", "s", "in,out"));
			iFace.Activate();

			AllJoyn.ProxyBusObject proxyBusObject = new AllJoyn.ProxyBusObject(clientBus, WELLKNOWN_NAME, OBJECT_PATH, 0, true);
			Assert.NotNull(proxyBusObject);

			Assert.True(proxyBusObject.IsSecure);

			Assert.Equal(AllJoyn.QStatus.OK, proxyBusObject.AddInterface(iFace));

			AllJoyn.MsgArg input = new AllJoyn.MsgArg("s", "AllJoyn");
			AllJoyn.Message replyMsg = new AllJoyn.Message(clientBus);
			status = proxyBusObject.MethodCall(INTERFACE_NAME, "ping", input, replyMsg, 5000, 0);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.Equal("AllJoyn", (string)replyMsg[0]);

			Assert.False(authflags.requestCreds_service);
			Assert.False(authflags.authComplete_serivce);

			Assert.False(authflags.requestCreds_client);
			Assert.False(authflags.authComplete_client);

			clientAuthlistener.Dispose();
			serviceAuthlistener.Dispose();

			busObject.Dispose();

			proxyBusObject.Dispose();
		}
	}
}