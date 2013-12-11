//-----------------------------------------------------------------------
// <copyright file="AuthListenerTest.cs" company="AllSeen Alliance.">
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
using System.Collections.Generic;
using System.Text;
using AllJoynUnity;
using Xunit;

namespace AllJoynUnityTest
{
	public class AuthListenerTest : IDisposable
	{
		const string INTERFACE_NAME = "org.alljoyn.test.cs.authlistener";
		const string OBJECT_PATH = "/org/alljoyn/test/authlistener";
		const string WELLKNOWN_NAME = "org.alljoyn.test.cs.authlistener";

		private AllJoyn.BusAttachment serviceBus;
		private AllJoyn.BusAttachment clientBus;
		private TestBusObject busObject;

		private bool disposed = false;

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


		public AuthListenerTest()
		{
			serviceBus = new AllJoyn.BusAttachment("AuthListenerTestService", false);

			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.Start());
			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.Connect(AllJoynTestCommon.GetConnectSpec()));

			AllJoyn.InterfaceDescription service_intf = null;
			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.CreateInterface(INTERFACE_NAME, AllJoyn.InterfaceDescription.SecurityPolicy.Required, out service_intf));
			Assert.Equal(AllJoyn.QStatus.OK, service_intf.AddMethod("ping", "s", "s", "in,out"));
			service_intf.Activate();

			clientBus = new AllJoyn.BusAttachment("AuthListenerTestClient", false);

			Assert.Equal(AllJoyn.QStatus.OK, clientBus.Start());
			Assert.Equal(AllJoyn.QStatus.OK, clientBus.Connect(AllJoynTestCommon.GetConnectSpec()));
		}

		~AuthListenerTest()
		{
			Dispose(false);
		}

		protected virtual void Dispose(bool disposing)
		{
			if (!this.disposed)
			{
				//additional clean up before disposing
				Assert.Equal(AllJoyn.QStatus.OK, serviceBus.Stop());
				Assert.Equal(AllJoyn.QStatus.OK, serviceBus.Join());

				Assert.Equal(AllJoyn.QStatus.OK, clientBus.Stop());
				Assert.Equal(AllJoyn.QStatus.OK, clientBus.Join());

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
			AuthListenerTest _authListenerTest;

			public TestBusListener(AuthListenerTest authListenerTest)
			{
				this._authListenerTest = authListenerTest;
			}

			protected override void NameOwnerChanged(string busName, string previousOwner, string newOwner)
			{
				_authListenerTest.nameOwnerChangedFlag = true;
				_authListenerTest.Notify();
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

			status = serviceBus.RegisterBusObject(busObject);
			if (!status)
			{
				return status;
			}

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

		public AllJoyn.QStatus SetUpAuthClient()
		{
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;
			AllJoyn.ProxyBusObject proxyObj = new AllJoyn.ProxyBusObject(clientBus, WELLKNOWN_NAME, OBJECT_PATH, 0);

			status = proxyObj.IntrospectRemoteObject();
			if (!status)
			{
				return status;
			}

			AllJoyn.Message reply = new AllJoyn.Message(clientBus);
			AllJoyn.MsgArg input = new AllJoyn.MsgArg("s", "AllJoyn");
			Console.WriteLine(proxyObj.GetInterface(INTERFACE_NAME).Introspect());

			status = proxyObj.MethodCall(INTERFACE_NAME, "ping", input, reply, 5000, 0);
			if (!status)
			{
				return status;
			}

			if ((string)reply[0] != "AllJoyn")
			{
				Console.WriteLine((string)reply[0] + " != \"AllJoyn\" as expected");
				return AllJoyn.QStatus.FAIL;
			}
			return status;
		}

		class SrpKeyx_service_authlistener : AllJoyn.AuthListener
		{
			private AuthListenerTest _authListenerTest;
			public SrpKeyx_service_authlistener(AuthListenerTest authListenerTest)
			{
				_authListenerTest = authListenerTest;
			}
			protected override bool RequestCredentials(string authMechanism, string peerName,
				ushort authCount, string userName, AllJoyn.Credentials.CredentialFlags credMask,
				AllJoyn.Credentials credentials)
			{
				if ((credMask & AllJoyn.Credentials.CredentialFlags.Password) == AllJoyn.Credentials.CredentialFlags.Password)
				{
					credentials.Password = "ABCDEFGH";
				}
				_authListenerTest.authflags.requestCreds_service = true;
				return true;
			}

			protected override bool VerifyCredentials(string authMechanism, string peerName, AllJoyn.Credentials credentials)
			{
				_authListenerTest.authflags.verifyCreds_service = true;
				return true;
			}

			protected override void SecurityViolation(AllJoyn.QStatus status, AllJoyn.Message msg)
			{
				_authListenerTest.authflags.securityViolation_service = true;
			}

			protected override void AuthenticationComplete(string authMechanism, string peerName, bool success)
			{
				_authListenerTest.authflags.authComplete_serivce = true;
			}
		}

		class SrpKeyx_client_authlistener : AllJoyn.AuthListener
		{
			private AuthListenerTest _authListenerTest;
			public SrpKeyx_client_authlistener(AuthListenerTest authListenerTest)
			{
				_authListenerTest = authListenerTest;
			}

			protected override bool RequestCredentials(string authMechanism, string peerName,
				ushort authCount, string userName, AllJoyn.Credentials.CredentialFlags credMask,
				AllJoyn.Credentials credentials)
			{
				if ((credMask & AllJoyn.Credentials.CredentialFlags.Password) == AllJoyn.Credentials.CredentialFlags.Password)
				{
					credentials.Password = "ABCDEFGH";
				}
				_authListenerTest.authflags.requestCreds_client = true;
				return true;
			}

			protected override bool VerifyCredentials(string authMechanism, string peerName, AllJoyn.Credentials credentials)
			{
				_authListenerTest.authflags.verifyCreds_client = true;
				return true;
			}

			protected override void SecurityViolation(AllJoyn.QStatus status, AllJoyn.Message msg)
			{
				_authListenerTest.authflags.securityViolation_client = true;
			}

			protected override void AuthenticationComplete(string authMechanism, string peerName, bool success)
			{
				_authListenerTest.authflags.authComplete_client = true;
			}
		}

		[Fact, Trait("authlistener", "srp_keyx")]
		public void srp_keyx()
		{
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
			Assert.Equal(AllJoyn.QStatus.OK, clientBus.CreateInterface(INTERFACE_NAME, AllJoyn.InterfaceDescription.SecurityPolicy.Required, out iFace));
			Assert.NotNull(iFace);

			Assert.Equal(AllJoyn.QStatus.OK, iFace.AddMethod("ping", "s", "s", "in,out"));
			iFace.Activate();

			AllJoyn.ProxyBusObject proxyBusObject = new AllJoyn.ProxyBusObject(clientBus, WELLKNOWN_NAME, OBJECT_PATH, 0);
			Assert.NotNull(proxyBusObject);
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



		class SrpLogon_service_authlistener : AllJoyn.AuthListener
		{
			private AuthListenerTest _authListenerTest;
			public SrpLogon_service_authlistener(AuthListenerTest authListenerTest)
			{
				_authListenerTest = authListenerTest;
			}
			protected override bool RequestCredentials(string authMechanism, string peerName,
				ushort authCount, string userName, AllJoyn.Credentials.CredentialFlags credMask,
				AllJoyn.Credentials credentials)
			{
				if (userName != "Mr. Cuddles")
				{
					return false;
				}
				else if ((credMask & AllJoyn.Credentials.CredentialFlags.Password) == AllJoyn.Credentials.CredentialFlags.Password)
				{
					credentials.Password = "123456";
				}
				else
				{
					return false;
				}

				_authListenerTest.authflags.requestCreds_service = true;
				return true;
			}

			protected override bool VerifyCredentials(string authMechanism, string peerName, AllJoyn.Credentials credentials)
			{
				_authListenerTest.authflags.verifyCreds_service = true;
				return true;
			}

			protected override void SecurityViolation(AllJoyn.QStatus status, AllJoyn.Message msg)
			{
				_authListenerTest.authflags.securityViolation_service = true;
			}

			protected override void AuthenticationComplete(string authMechanism, string peerName, bool success)
			{
				_authListenerTest.authflags.authComplete_serivce = success;
			}
		}

		class SrpLogon_client_authlistener : AllJoyn.AuthListener
		{
			private AuthListenerTest _authListenerTest;
			public SrpLogon_client_authlistener(AuthListenerTest authListenerTest)
			{
				_authListenerTest = authListenerTest;
			}

			protected override bool RequestCredentials(string authMechanism, string peerName,
				ushort authCount, string userName, AllJoyn.Credentials.CredentialFlags credMask,
				AllJoyn.Credentials credentials)
			{
				if ((credMask & AllJoyn.Credentials.CredentialFlags.Password) == AllJoyn.Credentials.CredentialFlags.Password)
				{
					credentials.Password = "123456";
				}
				if ((credMask & AllJoyn.Credentials.CredentialFlags.UserName) == AllJoyn.Credentials.CredentialFlags.UserName)
				{
					credentials.UserName = "Mr. Cuddles";
				}
				_authListenerTest.authflags.requestCreds_client = true;
				return true;
			}

			protected override bool VerifyCredentials(string authMechanism, string peerName, AllJoyn.Credentials credentials)
			{
				_authListenerTest.authflags.verifyCreds_client = true;
				return true;
			}

			protected override void SecurityViolation(AllJoyn.QStatus status, AllJoyn.Message msg)
			{
				_authListenerTest.authflags.securityViolation_client = true;
			}

			protected override void AuthenticationComplete(string authMechanism, string peerName, bool success)
			{
				_authListenerTest.authflags.authComplete_client = success;
			}
		}


		[Fact, Trait("authlistener", "srp_logon")]
		public void srp_logon()
		{
			ResetAuthFlags();
			clientBus.ClearKeyStore();

			SrpLogon_service_authlistener serviceAuthlistener = new SrpLogon_service_authlistener(this);
			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.EnablePeerSecurity("ALLJOYN_SRP_LOGON", serviceAuthlistener, null, false));

			serviceBus.ClearKeyStore();

			Assert.Equal(AllJoyn.QStatus.OK, SetUpAuthService());

			SrpLogon_client_authlistener clientAuthlistener = new SrpLogon_client_authlistener(this);
			Assert.Equal(AllJoyn.QStatus.OK, clientBus.EnablePeerSecurity("ALLJOYN_SRP_LOGON", clientAuthlistener, null, false));
			clientBus.ClearKeyStore();

			// create+activate the interface
			AllJoyn.QStatus status;
			AllJoyn.InterfaceDescription iFace = null;
			Assert.Equal(AllJoyn.QStatus.OK, clientBus.CreateInterface(INTERFACE_NAME, AllJoyn.InterfaceDescription.SecurityPolicy.Required, out iFace));
			Assert.NotNull(iFace);

			Assert.Equal(AllJoyn.QStatus.OK, iFace.AddMethod("ping", "s", "s", "in,out"));
			iFace.Activate();

			AllJoyn.ProxyBusObject proxyBusObject = new AllJoyn.ProxyBusObject(clientBus, WELLKNOWN_NAME, OBJECT_PATH, 0);
			Assert.NotNull(proxyBusObject);
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



		class PinKeyX_service_authlistener : AllJoyn.AuthListener
		{
			private AuthListenerTest _authListenerTest;
			public PinKeyX_service_authlistener(AuthListenerTest authListenerTest)
			{
				_authListenerTest = authListenerTest;
			}
			protected override bool RequestCredentials(string authMechanism, string peerName,
				ushort authCount, string userName, AllJoyn.Credentials.CredentialFlags credMask,
				AllJoyn.Credentials credentials)
			{
				if ((credMask & AllJoyn.Credentials.CredentialFlags.Password) == AllJoyn.Credentials.CredentialFlags.Password)
				{
					credentials.Password = "FEE_FI_FO_FUM";
				}
				else
				{
					return false;
				}

				_authListenerTest.authflags.requestCreds_service = true;
				return true;
			}

			protected override bool VerifyCredentials(string authMechanism, string peerName, AllJoyn.Credentials credentials)
			{
				_authListenerTest.authflags.verifyCreds_service = true;
				return true;
			}

			protected override void SecurityViolation(AllJoyn.QStatus status, AllJoyn.Message msg)
			{
				_authListenerTest.authflags.securityViolation_service = true;
			}

			protected override void AuthenticationComplete(string authMechanism, string peerName, bool success)
			{
				_authListenerTest.authflags.authComplete_serivce = success;
				//add test
			}
		}

		class PinKeyX_client_authlistener : AllJoyn.AuthListener
		{
			private AuthListenerTest _authListenerTest;
			public PinKeyX_client_authlistener(AuthListenerTest authListenerTest)
			{
				_authListenerTest = authListenerTest;
			}

			protected override bool RequestCredentials(string authMechanism, string peerName,
				ushort authCount, string userName, AllJoyn.Credentials.CredentialFlags credMask,
				AllJoyn.Credentials credentials)
			{
				if ((credMask & AllJoyn.Credentials.CredentialFlags.Password) == AllJoyn.Credentials.CredentialFlags.Password)
				{
					credentials.Password = "FEE_FI_FO_FUM";
				}
				_authListenerTest.authflags.requestCreds_client = true;
				return true;
			}

			protected override bool VerifyCredentials(string authMechanism, string peerName, AllJoyn.Credentials credentials)
			{
				_authListenerTest.authflags.verifyCreds_client = true;
				return true;
			}

			protected override void SecurityViolation(AllJoyn.QStatus status, AllJoyn.Message msg)
			{
				_authListenerTest.authflags.securityViolation_client = true;
			}

			protected override void AuthenticationComplete(string authMechanism, string peerName, bool success)
			{
				_authListenerTest.authflags.authComplete_client = success;
			}
		}


		[Fact, Trait("authlistener", "pin_keyx")]
		public void pin_keyx()
		{
			ResetAuthFlags();
			clientBus.ClearKeyStore();

			PinKeyX_service_authlistener serviceAuthlistener = new PinKeyX_service_authlistener(this);
			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.EnablePeerSecurity("ALLJOYN_PIN_KEYX", serviceAuthlistener, null, false));

			serviceBus.ClearKeyStore();

			Assert.Equal(AllJoyn.QStatus.OK, SetUpAuthService());

			PinKeyX_client_authlistener clientAuthlistener = new PinKeyX_client_authlistener(this);
			Assert.Equal(AllJoyn.QStatus.OK, clientBus.EnablePeerSecurity("ALLJOYN_PIN_KEYX", clientAuthlistener, null, false));
			clientBus.ClearKeyStore();

			// create+activate the interface
			AllJoyn.QStatus status;
			AllJoyn.InterfaceDescription iFace = null;
			Assert.Equal(AllJoyn.QStatus.OK, clientBus.CreateInterface(INTERFACE_NAME, AllJoyn.InterfaceDescription.SecurityPolicy.Required, out iFace));
			Assert.NotNull(iFace);

			Assert.Equal(AllJoyn.QStatus.OK, iFace.AddMethod("ping", "s", "s", "in,out"));
			iFace.Activate();

			AllJoyn.ProxyBusObject proxyBusObject = new AllJoyn.ProxyBusObject(clientBus, WELLKNOWN_NAME, OBJECT_PATH, 0);
			Assert.NotNull(proxyBusObject);
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

		const string service_x509certChain =
		// User certificate
		"-----BEGIN CERTIFICATE-----\n" +
		"MIICxzCCAjCgAwIBAgIJALZkSW0TWinQMA0GCSqGSIb3DQEBBQUAME8xCzAJBgNV\n" +
		"BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMQ0wCwYDVQQKEwRRdUlDMQ0wCwYD\n" +
		"VQQLEwRNQnVzMQ0wCwYDVQQDEwRHcmVnMB4XDTEwMDgyNTIzMTYwNVoXDTExMDgy\n" +
		"NTIzMTYwNVowfzELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAO\n" +
		"BgNVBAcTB1NlYXR0bGUxIzAhBgNVBAoTGlF1YWxjb21tIElubm92YXRpb24gQ2Vu\n" +
		"dGVyMREwDwYDVQQLEwhNQnVzIGRldjERMA8GA1UEAxMIU2VhIEtpbmcwgZ8wDQYJ\n" +
		"KoZIhvcNAQEBBQADgY0AMIGJAoGBALz+YZcH0DZn91sjOA5vaTwjQVBnbR9ZRpCA\n" +
		"kGD2am0F91juEPFvj/PAlvVLPd5nwGKSPiycN3l3ECxNerTrwIG2XxzBWantFn5n\n" +
		"7dDzlRm3aerFr78EJmcCiImwgqsuhUT4eo5/jn457vANO9B5k/1ddc6zJ67Jvuh6\n" +
		"0p4YAW4NAgMBAAGjezB5MAkGA1UdEwQCMAAwLAYJYIZIAYb4QgENBB8WHU9wZW5T\n" +
		"U0wgR2VuZXJhdGVkIENlcnRpZmljYXRlMB0GA1UdDgQWBBTXau+rH64d658efvkF\n" +
		"jkaEZJ+5BTAfBgNVHSMEGDAWgBTu5FqZL5ShsNq4KJjOo8IPZ70MBTANBgkqhkiG\n" +
		"9w0BAQUFAAOBgQBNBt7+/IaqGUSOpYAgHun87c86J+R38P2dmOm+wk8CNvKExdzx\n" +
		"Hp08aA51d5YtGrkDJdKXfC+Ly0CuE2SCiMU4RbK9Pc2H/MRQdmn7ZOygisrJNgRK\n" +
		"Gerh1OQGuc1/USAFpfD2rd+xqndp1WZz7iJh+ezF44VMUlo2fTKjYr5jMQ==\n" +
		"-----END CERTIFICATE-----\n" +
		// Root certificate
		"-----BEGIN CERTIFICATE-----\n" +
		"MIICzjCCAjegAwIBAgIJALZkSW0TWinPMA0GCSqGSIb3DQEBBQUAME8xCzAJBgNV\n" +
		"BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMQ0wCwYDVQQKEwRRdUlDMQ0wCwYD\n" +
		"VQQLEwRNQnVzMQ0wCwYDVQQDEwRHcmVnMB4XDTEwMDgyNTIzMTQwNloXDTEzMDgy\n" +
		"NDIzMTQwNlowTzELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xDTAL\n" +
		"BgNVBAoTBFF1SUMxDTALBgNVBAsTBE1CdXMxDTALBgNVBAMTBEdyZWcwgZ8wDQYJ\n" +
		"KoZIhvcNAQEBBQADgY0AMIGJAoGBANc1GTPfvD347zk1NlZbDhTf5txn3AcSG//I\n" +
		"gdgdZOY7ubXkNMGEVBMyZDXe7K36MEmj5hfXRiqfZwpZjjzJeJBoPJvXkETzatjX\n" +
		"vs4d5k1m0UjzANXp01T7EK1ZdIP7AjLg4QMk+uj8y7x3nElmSpNvPf3tBe3JUe6t\n" +
		"Io22NI/VAgMBAAGjgbEwga4wHQYDVR0OBBYEFO7kWpkvlKGw2rgomM6jwg9nvQwF\n" +
		"MH8GA1UdIwR4MHaAFO7kWpkvlKGw2rgomM6jwg9nvQwFoVOkUTBPMQswCQYDVQQG\n" +
		"EwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjENMAsGA1UEChMEUXVJQzENMAsGA1UE\n" +
		"CxMETUJ1czENMAsGA1UEAxMER3JlZ4IJALZkSW0TWinPMAwGA1UdEwQFMAMBAf8w\n" +
		"DQYJKoZIhvcNAQEFBQADgYEAg3pDFX0270jUTf8mFJHJ1P+CeultB+w4EMByTBfA\n" +
		"ZPNOKzFeoZiGe2AcMg41VXvaKJA0rNH+5z8zvVAY98x1lLKsJ4fb4aIFGQ46UZ35\n" +
		"DMrqZYmULjjSXWMxiphVRf1svKGU4WHR+VSvtUNLXzQyvg2yUb6PKDPUQwGi9kDx\n" +
		"tCI=\n" +
		"-----END CERTIFICATE-----\n";

		const string service_privKey =
		"-----BEGIN RSA PRIVATE KEY-----\n" +
		"Proc-Type: 4,ENCRYPTED\n" +
		"DEK-Info: DES-EDE3-CBC,86B9DBED35AEBAB3\n" +
		"\n" +
		"f28sibgVCkDz3VNoC/MzazG2tFj+KGf6xm9LQki/GsxpMhJsEEvT9dUluT1T4Ypr\n" +
		"NjG+nBleLcfdHxOl5XHnusn8r/JVaQQGVSnDaeP/27KiirtB472p+8Wc2wfXexRz\n" +
		"uSUv0DJT+Fb52zYGiGzwgaOinQEBskeO9AwRyG34sFKqyyapyJtSZDjh+wUAIMZb\n" +
		"wKifvl1KHSCbXEhjDVlxBw4Rt7I36uKzTY5oax2L6W6gzxfHuOtzfVelAaM46j+n\n" +
		"KANZgx6KGW2DKk27aad2HEZUYeDwznpwU5Duw9b0DeMTkez6CuayiZHb5qEod+0m\n" +
		"pCCMwpqxFCJ/vg1VJjmxM7wpCQTc5z5cjX8saV5jMUJXp09NuoU/v8TvhOcXOE1T\n" +
		"ENukIWYBT1HC9MJArroLwl+fMezKCu+F/JC3M0RfI0dlQqS4UWH+Uv+Ujqa2yr9y\n" +
		"20zYS52Z4kyq2WnqwBk1//PLBl/bH/awWXPUI2yMnIILbuCisRYLyK52Ge/rS51P\n" +
		"vUgUCZ7uoEJGTX6EGh0yQhp+5jGYVdHHZB840AyxzBQx7pW4MtTwqkw1NZuQcdSN\n" +
		"IU9y/PferHhMKZeGfVRVEkAOcjeXOqvSi6NKDvYn7osCkvj9h7K388o37VMPSacR\n" +
		"jDwDTT0HH/UcM+5v/74NgE/OebaK3YfxBVyMmBzi0WVFXgxHJir4xpj9c20YQVw9\n" +
		"hE3kYepW8gGz/JPQmRszwLQpwQNEP60CgQveqtH7tZVXzDkElvSyveOdjJf1lw4B\n" +
		"uCz54678UNNeIe7YB4yV1dMVhhcoitn7G/+jC9Qk3FTnuP+Ws5c/0g==\n" +
		"-----END RSA PRIVATE KEY-----";

		const string client_x509cert =
		"-----BEGIN CERTIFICATE-----\n" +
		"MIIBszCCARwCCQDuCh+BWVBk2DANBgkqhkiG9w0BAQUFADAeMQ0wCwYDVQQKDARN\n" +
		"QnVzMQ0wCwYDVQQDDARHcmVnMB4XDTEwMDUxNzE1MTg1N1oXDTExMDUxNzE1MTg1\n" +
		"N1owHjENMAsGA1UECgwETUJ1czENMAsGA1UEAwwER3JlZzCBnzANBgkqhkiG9w0B\n" +
		"AQEFAAOBjQAwgYkCgYEArSd4r62mdaIRG9xZPDAXfImt8e7GTIyXeM8z49Ie1mrQ\n" +
		"h7roHbn931Znzn20QQwFD6pPC7WxStXJVH0iAoYgzzPsXV8kZdbkLGUMPl2GoZY3\n" +
		"xDSD+DA3m6krcXcN7dpHv9OlN0D9Trc288GYuFEENpikZvQhMKPDUAEkucQ95Z8C\n" +
		"AwEAATANBgkqhkiG9w0BAQUFAAOBgQBkYY6zzf92LRfMtjkKs2am9qvjbqXyDJLS\n" +
		"viKmYe1tGmNBUzucDC5w6qpPCTSe23H2qup27///fhUUuJ/ssUnJ+Y77jM/u1O9q\n" +
		"PIn+u89hRmqY5GKHnUSZZkbLB/yrcFEchHli3vLo4FOhVVHwpnwLtWSpfBF9fWcA\n" +
		"7THIAV79Lg==\n" +
		"-----END CERTIFICATE-----";

		const string client_privKey =
		"-----BEGIN RSA PRIVATE KEY-----\n" +
		"Proc-Type: 4,ENCRYPTED\n" +
		"DEK-Info: AES-128-CBC,0AE4BAB94CEAA7829273DD861B067DBA\n" +
		"\n" +
		"LSJOp+hEzNDDpIrh2UJ+3CauxWRKvmAoGB3r2hZfGJDrCeawJFqH0iSYEX0n0QEX\n" +
		"jfQlV4LHSCoGMiw6uItTof5kHKlbp5aXv4XgQb74nw+2LkftLaTchNs0bW0TiGfQ\n" +
		"XIuDNsmnZ5+CiAVYIKzsPeXPT4ZZSAwHsjM7LFmosStnyg4Ep8vko+Qh9TpCdFX8\n" +
		"w3tH7qRhfHtpo9yOmp4hV9Mlvx8bf99lXSsFJeD99C5GQV2lAMvpfmM8Vqiq9CQN\n" +
		"9OY6VNevKbAgLG4Z43l0SnbXhS+mSzOYLxl8G728C6HYpnn+qICLe9xOIfn2zLjm\n" +
		"YaPlQR4MSjHEouObXj1F4MQUS5irZCKgp4oM3G5Ovzt82pqzIW0ZHKvi1sqz/KjB\n" +
		"wYAjnEGaJnD9B8lRsgM2iLXkqDmndYuQkQB8fhr+zzcFmqKZ1gLRnGQVXNcSPgjU\n" +
		"Y0fmpokQPHH/52u+IgdiKiNYuSYkCfHX1Y3nftHGvWR3OWmw0k7c6+DfDU2fDthv\n" +
		"3MUSm4f2quuiWpf+XJuMB11px1TDkTfY85m1aEb5j4clPGELeV+196OECcMm4qOw\n" +
		"AYxO0J/1siXcA5o6yAqPwPFYcs/14O16FeXu+yG0RPeeZizrdlv49j6yQR3JLa2E\n" +
		"pWiGR6hmnkixzOj43IPJOYXySuFSi7lTMYud4ZH2+KYeK23C2sfQSsKcLZAFATbq\n" +
		"DY0TZHA5lbUiOSUF5kgd12maHAMidq9nIrUpJDzafgK9JrnvZr+dVYM6CiPhiuqJ\n" +
		"bXvt08wtKt68Ymfcx+l64mwzNLS+OFznEeIjLoaHU4c=\n" +
		"-----END RSA PRIVATE KEY-----";

		class RsaKeyX_service_authlistener : AllJoyn.AuthListener
		{
			private AuthListenerTest _authListenerTest;
			public RsaKeyX_service_authlistener(AuthListenerTest authListenerTest)
			{
				_authListenerTest = authListenerTest;
			}
			protected override bool RequestCredentials(string authMechanism, string peerName,
				ushort authCount, string userName, AllJoyn.Credentials.CredentialFlags credMask,
				AllJoyn.Credentials credentials)
			{
				if ((credMask & AllJoyn.Credentials.CredentialFlags.CertChain) == AllJoyn.Credentials.CredentialFlags.CertChain)
				{
					credentials.CertChain = service_x509certChain;
				}
				if ((credMask & AllJoyn.Credentials.CredentialFlags.PrivateKey) == AllJoyn.Credentials.CredentialFlags.PrivateKey)
				{
					credentials.PrivateKey = service_privKey;
				}
				if ((credMask & AllJoyn.Credentials.CredentialFlags.Password) == AllJoyn.Credentials.CredentialFlags.Password)
				{
					credentials.Password = "123456";
				}

				_authListenerTest.authflags.requestCreds_service = true;
				return true;
			}

			protected override bool VerifyCredentials(string authMechanism, string peerName, AllJoyn.Credentials credentials)
			{
				_authListenerTest.authflags.verifyCreds_service = true;
				return true;
			}

			protected override void SecurityViolation(AllJoyn.QStatus status, AllJoyn.Message msg)
			{
				_authListenerTest.authflags.securityViolation_service = true;
			}

			protected override void AuthenticationComplete(string authMechanism, string peerName, bool success)
			{
				_authListenerTest.authflags.authComplete_serivce = success;
			}
		}

		class RsaKeyX_client_authlistener : AllJoyn.AuthListener
		{
			private AuthListenerTest _authListenerTest;
			public RsaKeyX_client_authlistener(AuthListenerTest authListenerTest)
			{
				_authListenerTest = authListenerTest;
			}

			protected override bool RequestCredentials(string authMechanism, string peerName,
				ushort authCount, string userName, AllJoyn.Credentials.CredentialFlags credMask,
				AllJoyn.Credentials credentials)
			{
				if ((credMask & AllJoyn.Credentials.CredentialFlags.CertChain) == AllJoyn.Credentials.CredentialFlags.CertChain)
				{
					credentials.CertChain = client_x509cert;
				}
				if ((credMask & AllJoyn.Credentials.CredentialFlags.PrivateKey) == AllJoyn.Credentials.CredentialFlags.PrivateKey)
				{
					credentials.PrivateKey = client_privKey;
				}
				if ((credMask & AllJoyn.Credentials.CredentialFlags.Password) == AllJoyn.Credentials.CredentialFlags.Password)
				{
					credentials.Password = "123456";
				}
				_authListenerTest.authflags.requestCreds_client = true;
				return true;
			}

			protected override bool VerifyCredentials(string authMechanism, string peerName, AllJoyn.Credentials credentials)
			{
				_authListenerTest.authflags.verifyCreds_client = true;
				return true;
			}

			protected override void SecurityViolation(AllJoyn.QStatus status, AllJoyn.Message msg)
			{
				_authListenerTest.authflags.securityViolation_client = true;
			}

			protected override void AuthenticationComplete(string authMechanism, string peerName, bool success)
			{
				_authListenerTest.authflags.authComplete_client = success;
			}
		}


		[Fact, Trait("authlistener", "rsa_keyx")]
		public void rsa_keyx()
		{
			ResetAuthFlags();
			clientBus.ClearKeyStore();

			PinKeyX_service_authlistener serviceAuthlistener = new PinKeyX_service_authlistener(this);
			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.EnablePeerSecurity("ALLJOYN_RSA_KEYX", serviceAuthlistener, null, false));

			serviceBus.ClearKeyStore();

			Assert.Equal(AllJoyn.QStatus.OK, SetUpAuthService());

			PinKeyX_client_authlistener clientAuthlistener = new PinKeyX_client_authlistener(this);
			Assert.Equal(AllJoyn.QStatus.OK, clientBus.EnablePeerSecurity("ALLJOYN_RSA_KEYX", clientAuthlistener, null, false));
			clientBus.ClearKeyStore();

			// create+activate the interface
			AllJoyn.QStatus status;
			AllJoyn.InterfaceDescription iFace = null;
			Assert.Equal(AllJoyn.QStatus.OK, clientBus.CreateInterface(INTERFACE_NAME, AllJoyn.InterfaceDescription.SecurityPolicy.Required, out iFace));
			Assert.NotNull(iFace);

			Assert.Equal(AllJoyn.QStatus.OK, iFace.AddMethod("ping", "s", "s", "in,out"));
			iFace.Activate();

			AllJoyn.ProxyBusObject proxyBusObject = new AllJoyn.ProxyBusObject(clientBus, WELLKNOWN_NAME, OBJECT_PATH, 0);
			Assert.NotNull(proxyBusObject);
			Assert.Equal(AllJoyn.QStatus.OK, proxyBusObject.AddInterface(iFace));

			AllJoyn.MsgArg input = new AllJoyn.MsgArg("s", "AllJoyn");
			AllJoyn.Message replyMsg = new AllJoyn.Message(clientBus);
			status = proxyBusObject.MethodCall(INTERFACE_NAME, "ping", input, replyMsg, 5000, 0);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.Equal("AllJoyn", (string)replyMsg[0]);

			Assert.True(authflags.requestCreds_service);
			Assert.True(authflags.authComplete_serivce);
			Assert.True(authflags.verifyCreds_service);

			Assert.True(authflags.requestCreds_client);
			Assert.True(authflags.authComplete_client);
			Assert.True(authflags.verifyCreds_client);

			clientAuthlistener.Dispose();
			serviceAuthlistener.Dispose();

			busObject.Dispose();

			proxyBusObject.Dispose();
		}


		class SrpKeyx2_service_authlistener : AllJoyn.AuthListener
		{
			private AuthListenerTest _authListenerTest;
			public SrpKeyx2_service_authlistener(AuthListenerTest authListenerTest)
			{
				_authListenerTest = authListenerTest;
			}
			protected override bool RequestCredentials(string authMechanism, string peerName,
				ushort authCount, string userName, AllJoyn.Credentials.CredentialFlags credMask,
				AllJoyn.Credentials credentials)
			{
				_authListenerTest.authflags.requestCreds_service = true;
				return false;
			}

			protected override bool VerifyCredentials(string authMechanism, string peerName, AllJoyn.Credentials credentials)
			{
				_authListenerTest.authflags.verifyCreds_service = true;
				return true;
			}

			protected override void SecurityViolation(AllJoyn.QStatus status, AllJoyn.Message msg)
			{
				_authListenerTest.authflags.securityViolation_service = true;
			}

			protected override void AuthenticationComplete(string authMechanism, string peerName, bool success)
			{
				_authListenerTest.authflags.authComplete_serivce = true;
			}
		}

		class SrpKeyx2_client_authlistener : AllJoyn.AuthListener
		{
			private AuthListenerTest _authListenerTest;
			public SrpKeyx2_client_authlistener(AuthListenerTest authListenerTest)
			{
				_authListenerTest = authListenerTest;
			}

			protected override bool RequestCredentials(string authMechanism, string peerName,
				ushort authCount, string userName, AllJoyn.Credentials.CredentialFlags credMask,
				AllJoyn.Credentials credentials)
			{
				_authListenerTest.authflags.requestCreds_client = true;
				return false;
			}

			protected override bool VerifyCredentials(string authMechanism, string peerName, AllJoyn.Credentials credentials)
			{
				_authListenerTest.authflags.verifyCreds_client = true;
				return true;
			}

			protected override void SecurityViolation(AllJoyn.QStatus status, AllJoyn.Message msg)
			{
				_authListenerTest.authflags.securityViolation_client = true;
				_authListenerTest.Notify();
			}

			protected override void AuthenticationComplete(string authMechanism, string peerName, bool success)
			{
				_authListenerTest.authflags.authComplete_client = true;
			}
		}

		[Fact, Trait("authlistener", "srp_keyx2")]
		public void srp_keyx2()
		{
			ResetAuthFlags();
			clientBus.ClearKeyStore();

			SrpKeyx2_service_authlistener serviceAuthlistener = new SrpKeyx2_service_authlistener(this);
			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", serviceAuthlistener, null, false));

			serviceBus.ClearKeyStore();

			Assert.Equal(AllJoyn.QStatus.OK, SetUpAuthService());

			SrpKeyx2_client_authlistener clientAuthlistener = new SrpKeyx2_client_authlistener(this);
			Assert.Equal(AllJoyn.QStatus.OK, clientBus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", clientAuthlistener, null, false));
			clientBus.ClearKeyStore();

			// create+activate the interface
			AllJoyn.QStatus status;
			AllJoyn.InterfaceDescription iFace = null;
			Assert.Equal(AllJoyn.QStatus.OK, clientBus.CreateInterface(INTERFACE_NAME, AllJoyn.InterfaceDescription.SecurityPolicy.Required, out iFace));
			Assert.NotNull(iFace);

			Assert.Equal(AllJoyn.QStatus.OK, iFace.AddMethod("ping", "s", "s", "in,out"));
			iFace.Activate();

			AllJoyn.ProxyBusObject proxyBusObject = new AllJoyn.ProxyBusObject(clientBus, WELLKNOWN_NAME, OBJECT_PATH, 0);
			Assert.NotNull(proxyBusObject);
			Assert.Equal(AllJoyn.QStatus.OK, proxyBusObject.AddInterface(iFace));

			AllJoyn.MsgArg input = new AllJoyn.MsgArg("s", "AllJoyn");
			AllJoyn.Message replyMsg = new AllJoyn.Message(clientBus);
			status = proxyBusObject.MethodCall(INTERFACE_NAME, "ping", input, replyMsg, 5000, 0);
			Assert.Equal(AllJoyn.QStatus.BUS_REPLY_IS_ERROR_MESSAGE, status);

			Assert.True(authflags.requestCreds_service);
			Assert.True(authflags.authComplete_serivce);

			Assert.True(authflags.authComplete_client);
			// Authentication complete can occure before the SecurityViolation callback
			// with authentication complete the MethodCall will return the BUS_REPLY_IS_ERROR_MESSAGE
			// and the code could check for the sercurityViolation_client before it is actually set
			// for this reason we need to wait for the flag to be set.
			Wait(TimeSpan.FromSeconds(5));
			Assert.True(authflags.securityViolation_client);
			clientAuthlistener.Dispose();
			serviceAuthlistener.Dispose();

			busObject.Dispose();

			proxyBusObject.Dispose();
		}

		[Fact, Trait("authlistener", "secureconnection")]
		public void secureconnection()
		{
			ResetAuthFlags();
			clientBus.ClearKeyStore();

			SrpKeyx_service_authlistener serviceAuthlistener = new SrpKeyx_service_authlistener(this);
			Assert.Equal(AllJoyn.QStatus.OK, serviceBus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", serviceAuthlistener, null, false));

			serviceBus.ClearKeyStore();

			Assert.Equal(AllJoyn.QStatus.OK, SetUpAuthService());

			SrpKeyx_client_authlistener clientAuthlistener = new SrpKeyx_client_authlistener(this);
			Assert.Equal(AllJoyn.QStatus.OK, clientBus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", clientAuthlistener, null, false));
			clientBus.ClearKeyStore();

			AllJoyn.ProxyBusObject proxyBusObject = new AllJoyn.ProxyBusObject(clientBus, WELLKNOWN_NAME, OBJECT_PATH, 0);
			Assert.NotNull(proxyBusObject);

			proxyBusObject.SecureConnection();

			Assert.True(authflags.requestCreds_service);
			Assert.True(authflags.authComplete_serivce);

			Assert.True(authflags.requestCreds_client);
			Assert.True(authflags.authComplete_client);


			ResetAuthFlags();

			/*
			 * the peer-to-peer connection should have been authenticated with the last
			 * call to alljoyn_proxybusobject_secureconnection this call should return
			 * ER_OK with out calling any of the authlistener functions.
			 */
			proxyBusObject.SecureConnection();
			Assert.False(authflags.requestCreds_service);
			Assert.False(authflags.authComplete_serivce);

			Assert.False(authflags.requestCreds_client);
			Assert.False(authflags.authComplete_client);

			ResetAuthFlags();

			/*
			 * Although the peer-to-peer connection has already been authenticated we
			 * are forcing re-authentication so we expect the authlistener functions to
			 * be called again.
			 */
			proxyBusObject.SecureConnection(true);

			Assert.True(authflags.requestCreds_service);
			Assert.True(authflags.authComplete_serivce);

			Assert.True(authflags.requestCreds_client);
			Assert.True(authflags.authComplete_client);

			clientAuthlistener.Dispose();
			serviceAuthlistener.Dispose();

			busObject.Dispose();

			proxyBusObject.Dispose();
		}
	}
}
