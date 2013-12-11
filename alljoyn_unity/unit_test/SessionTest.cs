//-----------------------------------------------------------------------
// <copyright file="SessionTest.cs" company="AllSeen Alliance.">
// Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
	public class SessionTest : IDisposable
	{
		const string INTERFACE_NAME = "org.alljoyn.test.SessionTest";
		const string OBJECT_NAME = "org.alljoyn.test.SessionTest";
		const string OBJECT_PATH = "/org/alljoyn/test/SessionTest";
		const ushort SERVICE_PORT = 25;

		public TimeSpan MaxWaitTime = TimeSpan.FromSeconds(10);

		AllJoyn.BusAttachment hostBus;
		AllJoyn.BusAttachment memberOneBus;
		AllJoyn.BusAttachment memberTwoBus;

		bool acceptSessionJoinerFlag;
		bool sessionJoinedFlag;
		bool foundAdvertisedNameFlag;
		bool sessionMemberAddedFlag;
		bool sessionMemberRemovedFlag;
		bool sessionLostFlag;
		bool sessionLostReasonFlag;
		AllJoyn.SessionListener.SessionLostReason reasonMarker;

		private bool disposed = false;

		public SessionTest()
		{
			SetupHost();
			SetupMemberOne();
			SetupMemberTwo();
		}

		~SessionTest()
		{
			Dispose(false);
		}

		protected virtual void Dispose(bool disposing)
		{
			if (!this.disposed)
			{
				hostBus.CancelAdvertisedName(OBJECT_NAME, AllJoyn.TransportMask.Any);
				hostBus.ReleaseName(OBJECT_NAME);

				memberOneBus.Stop();
				memberOneBus.Join();

				memberTwoBus.Stop();
				memberTwoBus.Join();

				hostBus.Stop();
				hostBus.Join();

				if (disposing)
				{
					memberOneBus.Dispose();
					memberTwoBus.Dispose();
					hostBus.Dispose();
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
		public void TestSessionJoined()
		{
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;

			// Create session
			AllJoyn.SessionOpts opts = new AllJoyn.SessionOpts(
				AllJoyn.SessionOpts.TrafficType.Messages, false,
				AllJoyn.SessionOpts.ProximityType.Any, AllJoyn.TransportMask.Any);
			ushort sessionPort = SERVICE_PORT;

			// create the session port listener
			AllJoyn.SessionPortListener sessionPortListener = new TestSessionPortListener(this);

			// bind to the session port
			status = hostBus.BindSessionPort(ref sessionPort, opts, sessionPortListener);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// request name
			status = hostBus.RequestName(OBJECT_NAME, AllJoyn.DBus.NameFlags.ReplaceExisting | AllJoyn.DBus.NameFlags.DoNotQueue);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// Advertise name
			status = hostBus.AdvertiseName(OBJECT_NAME, opts.Transports);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			///////////////////////////////////////////////////////////
			foundAdvertisedNameFlag = false;
			acceptSessionJoinerFlag = false;
			sessionJoinedFlag = false;

			// try to join the session & verify callbacks are called

			// register the bus listener
			AllJoyn.BusListener busListener = new TestBusListener(this);
			memberOneBus.RegisterBusListener(busListener);

			// find the advertised name from the "servicebus"
			status = memberOneBus.FindAdvertisedName(OBJECT_NAME);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			EventWaitHandle ewh = new EventWaitHandle(false, EventResetMode.AutoReset, "FoundAdvertisedName");
			ewh.WaitOne(MaxWaitTime);
			Assert.Equal(true, foundAdvertisedNameFlag);

			// try to join & verify that the sessionedJoined callback was called
			uint sSessionId;
			status = memberOneBus.JoinSession(OBJECT_NAME, SERVICE_PORT, null, out sSessionId, opts);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			ewh = new EventWaitHandle(false, EventResetMode.AutoReset, "SessionJoined");
			ewh.WaitOne(MaxWaitTime);
			Assert.Equal(true, acceptSessionJoinerFlag);
			Assert.Equal(true, sessionJoinedFlag);
			hostBus.ReleaseName(OBJECT_NAME);
		}

		[Fact]
		public void TestSessionMemberAddRemove()
		{
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;
			sessionMemberAddedFlag = false;

			// Create session
			AllJoyn.SessionOpts opts = new AllJoyn.SessionOpts(
				AllJoyn.SessionOpts.TrafficType.Messages, true,
				AllJoyn.SessionOpts.ProximityType.Any, AllJoyn.TransportMask.Any);
			ushort sessionPort = SERVICE_PORT;

			// create the session port listener
			AllJoyn.SessionPortListener sessionPortListener = new TestSessionPortListener(this);

			// bind to the session port
			status = hostBus.BindSessionPort(ref sessionPort, opts, sessionPortListener);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// request name
			status = hostBus.RequestName(OBJECT_NAME, AllJoyn.DBus.NameFlags.ReplaceExisting | AllJoyn.DBus.NameFlags.DoNotQueue);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// Advertise name
			status = hostBus.AdvertiseName(OBJECT_NAME, opts.Transports);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// register sessionMemberOne's bus listener
			AllJoyn.BusListener busListenerMemberOne = new TestBusListener(this);
			memberOneBus.RegisterBusListener(busListenerMemberOne);
			// create the session listener
			AllJoyn.SessionListener sessionListener = new TestSessionListener2(this);

			AllJoyn.BusListener busListenerMemberTwo = new TestBusListener(this);
			memberTwoBus.RegisterBusListener(busListenerMemberTwo);

			///////////////////////////////////////////////////////////
			// have sessionMemberOne find and join the session
			foundAdvertisedNameFlag = false;
			status = memberOneBus.FindAdvertisedName(OBJECT_NAME);  // find the advertised name from the "hostbus"
			Assert.Equal(AllJoyn.QStatus.OK, status);
			EventWaitHandle ewh = new EventWaitHandle(false, EventResetMode.AutoReset, "FoundAdvertisedName");
			ewh.WaitOne(MaxWaitTime);
			Assert.Equal(true, foundAdvertisedNameFlag);

			uint sSessionId;
			acceptSessionJoinerFlag = false;
			sessionJoinedFlag = false;
			status = memberOneBus.JoinSession(OBJECT_NAME, SERVICE_PORT, sessionListener, out sSessionId, opts);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			ewh = new EventWaitHandle(false, EventResetMode.AutoReset, "SessionJoined");
			ewh.WaitOne(MaxWaitTime);

			// verify that sessionMemberOne joined by checking that the sessionedJoined callback was called
			Assert.Equal(true, acceptSessionJoinerFlag);
			Assert.Equal(true, sessionJoinedFlag);

			///////////////////////////////////////////////////////////
			// have session member two find and join the session
			sessionMemberAddedFlag = false;
			foundAdvertisedNameFlag = false;
			status = memberTwoBus.FindAdvertisedName(OBJECT_NAME);  // find the advertised name from the "hostbus"
			Assert.Equal(AllJoyn.QStatus.OK, status);
			ewh = new EventWaitHandle(false, EventResetMode.AutoReset, "FoundAdvertisedName");
			ewh.WaitOne(MaxWaitTime);
			Assert.Equal(true, foundAdvertisedNameFlag);

			acceptSessionJoinerFlag = false;
			sessionJoinedFlag = false;
			status = memberTwoBus.JoinSession(OBJECT_NAME, SERVICE_PORT, null, out sSessionId, opts);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			ewh = new EventWaitHandle(false, EventResetMode.AutoReset, "SessionJoined");
			ewh.WaitOne(MaxWaitTime);

			// verify that sessionMemberTwo joined by checking that the sessionedJoined callback was called
			Assert.Equal(true, acceptSessionJoinerFlag);
			Assert.Equal(true, sessionJoinedFlag);

			///////////////////////////////////////////////////////////
			// Now that sessionMemberTwo has joined, the SessionMemberAdded callback should have been triggered
			ewh = new EventWaitHandle(false, EventResetMode.AutoReset, "SessionMemberAdded");
			ewh.WaitOne(MaxWaitTime);
			Assert.Equal(true, sessionMemberAddedFlag);

			///////////////////////////////////////////////////////////
			// Now have sessionMemberTwo leave & verify SessionMemberRemoved callback is triggered
			sessionMemberRemovedFlag = false;
			memberTwoBus.LeaveSession(sSessionId);
			ewh = new EventWaitHandle(false, EventResetMode.AutoReset, "SessionMemberRemoved");
			ewh.WaitOne(MaxWaitTime);
			Assert.Equal(true, sessionMemberRemovedFlag);

			hostBus.ReleaseName(OBJECT_NAME);
		}

		[Fact]
		public void TestSessionLost()
		{
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;

			// Create session
			AllJoyn.SessionOpts opts = new AllJoyn.SessionOpts(
				AllJoyn.SessionOpts.TrafficType.Messages, true,
				AllJoyn.SessionOpts.ProximityType.Any, AllJoyn.TransportMask.Any);
			ushort sessionPort = SERVICE_PORT;

			// create the session port listener
			AllJoyn.SessionPortListener sessionPortListener = new TestSessionPortListener(this);

			// bind to the session port
			status = hostBus.BindSessionPort(ref sessionPort, opts, sessionPortListener);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// request name
			status = hostBus.RequestName(OBJECT_NAME, AllJoyn.DBus.NameFlags.ReplaceExisting | AllJoyn.DBus.NameFlags.DoNotQueue);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// Advertise name
			status = hostBus.AdvertiseName(OBJECT_NAME, opts.Transports);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// register sessionMemberOne's bus listener
			AllJoyn.BusListener busListenerMemberOne = new TestBusListener(this);
			memberOneBus.RegisterBusListener(busListenerMemberOne);
			// create the session listener
			AllJoyn.SessionListener sessionListener = new TestSessionListener(this);

			///////////////////////////////////////////////////////////
			// have sessionMemberOne find and join the session
			foundAdvertisedNameFlag = false;
			status = memberOneBus.FindAdvertisedName(OBJECT_NAME);  // find the advertised name from the "hostbus"
			Assert.Equal(AllJoyn.QStatus.OK, status);
			EventWaitHandle ewh = new EventWaitHandle(false, EventResetMode.AutoReset, "FoundAdvertisedName");
			ewh.WaitOne(MaxWaitTime);
			Assert.Equal(true, foundAdvertisedNameFlag);

			uint sSessionId;
			acceptSessionJoinerFlag = false;
			sessionJoinedFlag = false;
			status = memberOneBus.JoinSession(OBJECT_NAME, SERVICE_PORT, sessionListener, out sSessionId, opts);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			ewh = new EventWaitHandle(false, EventResetMode.AutoReset, "SessionJoined");
			ewh.WaitOne(MaxWaitTime);

			// verify that sessionMemberOne joined by checking that the sessionedJoined callback was called
			Assert.Equal(true, acceptSessionJoinerFlag);
			Assert.Equal(true, sessionJoinedFlag);

			///////////////////////////////////////////////////////////
			// Now have the host leave & verify SessionLost callback is triggered
			sessionLostFlag = false;
			sessionMemberRemovedFlag = false;
			hostBus.Stop();
			hostBus.Join();
			ewh = new EventWaitHandle(false, EventResetMode.AutoReset, "SessionLost");
			ewh.WaitOne(MaxWaitTime);
			Assert.Equal(true, sessionLostFlag);

			// SessionMemberRemoved should also be triggered
			ewh = new EventWaitHandle(false, EventResetMode.AutoReset, "SessionMemberRemoved");
			ewh.WaitOne(MaxWaitTime);
			Assert.Equal(true, sessionMemberRemovedFlag);

			hostBus.ReleaseName(OBJECT_NAME);
		}

		[Fact]
		public void TestSessionLostWithReason()
		{
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;

			// Create session
			AllJoyn.SessionOpts opts = new AllJoyn.SessionOpts(
				AllJoyn.SessionOpts.TrafficType.Messages, true,
				AllJoyn.SessionOpts.ProximityType.Any, AllJoyn.TransportMask.Any);
			ushort sessionPort = SERVICE_PORT;

			// create the session port listener
			AllJoyn.SessionPortListener sessionPortListener = new TestSessionPortListener(this);

			// bind to the session port
			status = hostBus.BindSessionPort(ref sessionPort, opts, sessionPortListener);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// request name
			status = hostBus.RequestName(OBJECT_NAME, AllJoyn.DBus.NameFlags.ReplaceExisting | AllJoyn.DBus.NameFlags.DoNotQueue);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// Advertise name
			status = hostBus.AdvertiseName(OBJECT_NAME, opts.Transports);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// register sessionMemberOne's bus listener
			AllJoyn.BusListener busListenerMemberOne = new TestBusListener(this);
			memberOneBus.RegisterBusListener(busListenerMemberOne);
			// create the session listener
			AllJoyn.SessionListener sessionListener = new TestSessionListener2(this);

			///////////////////////////////////////////////////////////
			// have sessionMemberOne find and join the session
			foundAdvertisedNameFlag = false;
			status = memberOneBus.FindAdvertisedName(OBJECT_NAME);  // find the advertised name from the "hostbus"
			Assert.Equal(AllJoyn.QStatus.OK, status);
			EventWaitHandle ewh = new EventWaitHandle(false, EventResetMode.AutoReset, "FoundAdvertisedName");
			ewh.WaitOne(MaxWaitTime);
			Assert.Equal(true, foundAdvertisedNameFlag);

			uint sSessionId;
			acceptSessionJoinerFlag = false;
			sessionJoinedFlag = false;
			status = memberOneBus.JoinSession(OBJECT_NAME, SERVICE_PORT, sessionListener, out sSessionId, opts);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			ewh = new EventWaitHandle(false, EventResetMode.AutoReset, "SessionJoined");
			ewh.WaitOne(MaxWaitTime);

			// verify that sessionMemberOne joined by checking that the sessionedJoined callback was called
			Assert.Equal(true, acceptSessionJoinerFlag);
			Assert.Equal(true, sessionJoinedFlag);

			///////////////////////////////////////////////////////////
			// Now have the host leave & verify SessionLost callback is triggered
			sessionLostReasonFlag = false;
			reasonMarker = AllJoyn.SessionListener.SessionLostReason.ALLJOYN_SESSIONLOST_INVALID;
			sessionMemberRemovedFlag = false;
			hostBus.Stop();
			hostBus.Join();
			ewh = new EventWaitHandle(false, EventResetMode.AutoReset, "SessionLostReason");
			ewh.WaitOne(MaxWaitTime);
			Assert.Equal(true, sessionLostReasonFlag);
			Assert.Equal(AllJoyn.SessionListener.SessionLostReason.ALLJOYN_SESSIONLOST_REMOTE_END_CLOSED_ABRUPTLY, reasonMarker);

			// SessionMemberRemoved should also be triggered
			ewh = new EventWaitHandle(false, EventResetMode.AutoReset, "SessionMemberRemoved");
			ewh.WaitOne(MaxWaitTime);
			Assert.Equal(true, sessionMemberRemovedFlag);

			hostBus.ReleaseName(OBJECT_NAME);
		}

		[Fact]
		public void RemoveSessionMember()
		{
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;

			// Create session
			AllJoyn.SessionOpts opts = new AllJoyn.SessionOpts(
				AllJoyn.SessionOpts.TrafficType.Messages, true,
				AllJoyn.SessionOpts.ProximityType.Any, AllJoyn.TransportMask.Any);
			ushort sessionPort = SERVICE_PORT;

			// create the session port listener
			AllJoyn.SessionPortListener sessionPortListener = new TestSessionPortListener(this);

			// bind to the session port
			status = hostBus.BindSessionPort(ref sessionPort, opts, sessionPortListener);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// request name
			status = hostBus.RequestName(OBJECT_NAME, AllJoyn.DBus.NameFlags.ReplaceExisting | AllJoyn.DBus.NameFlags.DoNotQueue);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// Advertise name
			status = hostBus.AdvertiseName(OBJECT_NAME, opts.Transports);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// register sessionMemberOne's bus listener
			AllJoyn.BusListener busListenerMemberOne = new TestBusListener(this);
			memberOneBus.RegisterBusListener(busListenerMemberOne);
			// create the session listener
			AllJoyn.SessionListener sessionListener = new TestSessionListener2(this);

			///////////////////////////////////////////////////////////
			// have sessionMemberOne find and join the session
			foundAdvertisedNameFlag = false;
			status = memberOneBus.FindAdvertisedName(OBJECT_NAME);  // find the advertised name from the "hostbus"
			Assert.Equal(AllJoyn.QStatus.OK, status);
			EventWaitHandle ewh = new EventWaitHandle(false, EventResetMode.AutoReset, "FoundAdvertisedName");
			ewh.WaitOne(MaxWaitTime);
			Assert.Equal(true, foundAdvertisedNameFlag);

			uint sSessionId;
			acceptSessionJoinerFlag = false;
			sessionJoinedFlag = false;
			status = memberOneBus.JoinSession(OBJECT_NAME, SERVICE_PORT, sessionListener, out sSessionId, opts);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			ewh = new EventWaitHandle(false, EventResetMode.AutoReset, "SessionJoined");
			ewh.WaitOne(MaxWaitTime);

			// verify that sessionMemberOne joined by checking that the sessionedJoined callback was called
			Assert.Equal(true, acceptSessionJoinerFlag);
			Assert.Equal(true, sessionJoinedFlag);

			///////////////////////////////////////////////////////////
			// Now have the host leave & verify SessionLost callback is triggered
			sessionLostReasonFlag = false;
			reasonMarker = AllJoyn.SessionListener.SessionLostReason.ALLJOYN_SESSIONLOST_INVALID;
			sessionMemberRemovedFlag = false;
			hostBus.RemoveSessionMember(sSessionId, memberOneBus.UniqueName);
			ewh = new EventWaitHandle(false, EventResetMode.AutoReset, "SessionLostReason");
			ewh.WaitOne(MaxWaitTime);
			Assert.Equal(true, sessionLostReasonFlag);
			Assert.Equal(AllJoyn.SessionListener.SessionLostReason.ALLJOYN_SESSIONLOST_REMOVED_BY_BINDER, reasonMarker);

			// SessionMemberRemoved should also be triggered
			ewh = new EventWaitHandle(false, EventResetMode.AutoReset, "SessionMemberRemoved");
			ewh.WaitOne(MaxWaitTime);
			Assert.Equal(true, sessionMemberRemovedFlag);

			hostBus.ReleaseName(OBJECT_NAME);
		}

		private void SetupHost()
		{
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;

			// create+start+connect bus attachment
			hostBus = null;
			hostBus = new AllJoyn.BusAttachment("SessionTestHost", true);
			Assert.NotNull(hostBus);

			status = hostBus.Start();
			Assert.Equal(AllJoyn.QStatus.OK, status);

			status = hostBus.Connect(AllJoynTestCommon.GetConnectSpec());
			Assert.Equal(AllJoyn.QStatus.OK, status);


		}

		private void SetupMemberOne()
		{
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;

			memberOneBus = null;
			memberOneBus = new AllJoyn.BusAttachment("SessionTestMemberOne", true);
			Assert.NotNull(memberOneBus);

			status = memberOneBus.Start();
			Assert.Equal(AllJoyn.QStatus.OK, status);

			status = memberOneBus.Connect(AllJoynTestCommon.GetConnectSpec());
			Assert.Equal(AllJoyn.QStatus.OK, status);
		}

		private void SetupMemberTwo()
		{
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;

			memberTwoBus = null;
			memberTwoBus = new AllJoyn.BusAttachment("SessionTestMemberTwo", true);
			Assert.NotNull(memberTwoBus);

			status = memberTwoBus.Start();
			Assert.Equal(AllJoyn.QStatus.OK, status);

			status = memberTwoBus.Connect(AllJoynTestCommon.GetConnectSpec());
			Assert.Equal(AllJoyn.QStatus.OK, status);
		}

		class TestSessionPortListener : AllJoyn.SessionPortListener
		{
			SessionTest _sessionTest;

			public TestSessionPortListener(SessionTest sessionTest)
			{
				this._sessionTest = sessionTest;
			}

			protected override bool AcceptSessionJoiner(ushort sessionPort, string joiner, AllJoyn.SessionOpts opts)
			{
				_sessionTest.acceptSessionJoinerFlag = true;
				return true;
			}

			protected override void SessionJoined(ushort sessionPort, uint sessionId, string joiner)
			{
				_sessionTest.sessionJoinedFlag = true;
				EventWaitHandle ewh = new EventWaitHandle(true, EventResetMode.AutoReset, "SessionJoined");
				ewh.Set();
			}

		}

		/*
		 * This SessionListener uses the obsolite SessionLost callback
		 */
		class TestSessionListener : AllJoyn.SessionListener
		{
			SessionTest _sessionTest;

			public TestSessionListener(SessionTest sessionTest)
			{
				this._sessionTest = sessionTest;
			}

			protected override void SessionMemberAdded(uint sessionId, string uniqueName)
			{
				_sessionTest.sessionMemberAddedFlag = true;
				EventWaitHandle ewh = new EventWaitHandle(true, EventResetMode.AutoReset, "SessionMemberAdded");
				ewh.Set();
			}

			protected override void SessionMemberRemoved(uint sessionId, string uniqueName)
			{
				_sessionTest.sessionMemberRemovedFlag = true;
				EventWaitHandle ewh = new EventWaitHandle(true, EventResetMode.AutoReset, "SessionMemberRemoved");
				ewh.Set();
			}
			/*
			 * Disable warning stating that this overrides an obsolete member we
			 * are testing that the obsolete member still functions till it is removed
			 */
#pragma warning disable 672
			protected override void SessionLost(uint sessionId)
			{
				_sessionTest.sessionLostFlag = true;
				EventWaitHandle ewh = new EventWaitHandle(true, EventResetMode.AutoReset, "SessionLost");
				ewh.Set();
			}
#pragma warning restore 672
		}

		class TestSessionListener2 : AllJoyn.SessionListener
		{
			SessionTest _sessionTest;

			public TestSessionListener2(SessionTest sessionTest)
			{
				this._sessionTest = sessionTest;
			}

			protected override void SessionMemberAdded(uint sessionId, string uniqueName)
			{
				_sessionTest.sessionMemberAddedFlag = true;
				EventWaitHandle ewh = new EventWaitHandle(true, EventResetMode.AutoReset, "SessionMemberAdded");
				ewh.Set();
			}

			protected override void SessionMemberRemoved(uint sessionId, string uniqueName)
			{
				_sessionTest.sessionMemberRemovedFlag = true;
				EventWaitHandle ewh = new EventWaitHandle(true, EventResetMode.AutoReset, "SessionMemberRemoved");
				ewh.Set();
			}

			protected override void SessionLost(uint sessionId, SessionLostReason reason)
			{
				_sessionTest.sessionLostReasonFlag = true;
				_sessionTest.reasonMarker = reason;
				EventWaitHandle ewh = new EventWaitHandle(true, EventResetMode.AutoReset, "SessionLostReason");
				ewh.Set();
			}
		}

		// we need this so that we know when the advertised name has been found
		class TestBusListener : AllJoyn.BusListener
		{
			SessionTest _sessionTest;

			public TestBusListener(SessionTest sessionTest)
			{
				this._sessionTest = sessionTest;
			}

			protected override void FoundAdvertisedName(string name, AllJoyn.TransportMask transport, string namePrefix)
			{
				if (string.Compare(OBJECT_NAME, name) == 0)
				{
					_sessionTest.foundAdvertisedNameFlag = true;
					EventWaitHandle ewh = new EventWaitHandle(true, EventResetMode.AutoReset, "FoundAdvertisedName");
					ewh.Set();
				}
			}
		}
	}
}
