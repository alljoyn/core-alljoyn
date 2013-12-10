//-----------------------------------------------------------------------
// <copyright file="BusListenerTest.cs" company="AllSeen Alliance.">
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
	public class BusListenerTest
	{
		public const string ObjectName = "org.alljoyn.test.BusListenerTest";
		public TimeSpan MaxWaitTime = TimeSpan.FromSeconds(5);

		AutoResetEvent notifyEvent = new AutoResetEvent(false);

		bool listenerRegistered;
		bool listenerUnregistered;
		bool foundAdvertisedName;
		bool lostAdvertisedName;
		bool nameOwnerChanged;
		bool busDisconnected;
		bool busStopping;
		AllJoyn.TransportMask transportFound;

		[Fact]
		public void TestListenerRegisteredUnregistered()
		{
			AllJoyn.BusAttachment bus = new AllJoyn.BusAttachment("BusListenerTest", true);
			AllJoyn.BusListener busListener = new TestBusListener(this);
			listenerRegistered = false;
			listenerUnregistered = false;
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;

			// start the bus attachment
			status = bus.Start();
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// connect to the bus
			status = bus.Connect(AllJoynTestCommon.GetConnectSpec());
			Assert.Equal(AllJoyn.QStatus.OK, status);

			bus.RegisterBusListener(busListener);
			Wait(MaxWaitTime);
			Assert.Equal(true, listenerRegistered);

			bus.UnregisterBusListener(busListener);
			Wait(MaxWaitTime);
			Assert.Equal(true, listenerUnregistered);

			// TODO: move these into a teardown method?
			busListener.Dispose();
			bus.Dispose();
		}

		[Fact]
		public void TestFoundLostAdvertisedName()
		{
			// create bus attachment
			AllJoyn.BusAttachment bus = new AllJoyn.BusAttachment("BusListenerTest", true);
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;

			// start the bus attachment
			status = bus.Start();
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// connect to the bus
			status = bus.Connect(AllJoynTestCommon.GetConnectSpec());
			Assert.Equal(AllJoyn.QStatus.OK, status);

			listenerRegistered = false;
			foundAdvertisedName = false;
			lostAdvertisedName = false;

			// register the bus listener
			AllJoyn.BusListener busListener = new TestBusListener(this);
			bus.RegisterBusListener(busListener);
			Wait(MaxWaitTime);
			Assert.Equal(true, listenerRegistered);

			// advertise the name, & see if we find it
			status = bus.FindAdvertisedName(ObjectName);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			AllJoyn.SessionOpts sessionOpts = new AllJoyn.SessionOpts(
				AllJoyn.SessionOpts.TrafficType.Messages, false,
				AllJoyn.SessionOpts.ProximityType.Any, AllJoyn.TransportMask.Any);

			status = bus.AdvertiseName(ObjectName, sessionOpts.Transports);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			Wait(MaxWaitTime);
			Assert.Equal(true, foundAdvertisedName);

			// stop advertising the name, & see if we lose it
			status = bus.CancelAdvertisedName(ObjectName, sessionOpts.Transports);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			Wait(MaxWaitTime);
			Assert.Equal(true, lostAdvertisedName);

			// TODO: move these into a teardown method?
			busListener.Dispose();
			bus.Dispose();
		}

		[Fact]
		public void TestStopDisconnected()
		{
			// create bus attachment
			AllJoyn.BusAttachment bus = new AllJoyn.BusAttachment("BusListenerTest", true);
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;

			// start the bus attachment
			status = bus.Start();
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// connect to the bus
			status = bus.Connect(AllJoynTestCommon.GetConnectSpec());
			Assert.Equal(AllJoyn.QStatus.OK, status);

			listenerRegistered = false;
			busDisconnected = false;
			busStopping = false;

			// register the bus listener
			AllJoyn.BusListener busListener = new TestBusListener(this);
			bus.RegisterBusListener(busListener);
			Wait(MaxWaitTime);
			Assert.Equal(true, listenerRegistered);

			// test disconnecting from the bus
			status = bus.Disconnect(AllJoynTestCommon.GetConnectSpec());
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Wait(MaxWaitTime);
			Assert.Equal(true, busDisconnected);

			// test stopping the bus
			status = bus.Stop();
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Wait(MaxWaitTime);
			Assert.Equal(true, busStopping);

			busListener.Dispose();
			bus.Dispose();
		}

		[Fact]
		public void TestNameOwnerChanged()
		{
			// create bus attachment
			AllJoyn.BusAttachment bus = new AllJoyn.BusAttachment("BusListenerTest", true);
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;

			// start the bus attachment
			status = bus.Start();
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// connect to the bus
			status = bus.Connect(AllJoynTestCommon.GetConnectSpec());
			Assert.Equal(AllJoyn.QStatus.OK, status);

			listenerRegistered = false;
			nameOwnerChanged = false;

			// register the bus listener
			AllJoyn.BusListener busListener = new TestBusListener(this);
			bus.RegisterBusListener(busListener);
			Wait(MaxWaitTime);
			Assert.Equal(true, listenerRegistered);

			// test name owner changed
			status = bus.RequestName(ObjectName, 0);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Wait(MaxWaitTime);
			Assert.Equal(true, nameOwnerChanged);

			busListener.Dispose();
			bus.Dispose();
		}

		[Fact]
		public void TestFoundNameByTransport()
		{
			// create bus attachment
			AllJoyn.BusAttachment bus = new AllJoyn.BusAttachment("BusListenerTest", true);
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;

			// start the bus attachment
			status = bus.Start();
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// connect to the bus
			status = bus.Connect(AllJoynTestCommon.GetConnectSpec());
			Assert.Equal(AllJoyn.QStatus.OK, status);

			listenerRegistered = false;
			foundAdvertisedName = false;
			nameOwnerChanged = false;
			transportFound = AllJoyn.TransportMask.None;

			// register the bus listener
			AllJoyn.BusListener busListener = new TestBusListener(this);
			bus.RegisterBusListener(busListener);
			Wait(MaxWaitTime);
			Assert.Equal(true, listenerRegistered);

			AllJoyn.SessionOpts sessionOpts = new AllJoyn.SessionOpts(
				AllJoyn.SessionOpts.TrafficType.Messages, false,
				AllJoyn.SessionOpts.ProximityType.Any, AllJoyn.TransportMask.Any);

			// advertise the name, & see if we find it
			status = bus.FindAdvertisedNameByTransport(ObjectName, AllJoyn.TransportMask.Local);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			status = bus.AdvertiseName(ObjectName, sessionOpts.Transports);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			Wait(MaxWaitTime);
			Assert.Equal(true, foundAdvertisedName);
			Assert.Equal(AllJoyn.TransportMask.Local, transportFound);

			status = bus.CancelAdvertisedName(ObjectName, sessionOpts.Transports);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			status = bus.CancelFindAdvertisedNameByTransport(ObjectName, AllJoyn.TransportMask.Local);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			foundAdvertisedName = false;
			status = bus.AdvertiseName(ObjectName, sessionOpts.Transports);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			Wait(TimeSpan.FromSeconds(1));
			Assert.Equal(false, foundAdvertisedName);

			busListener.Dispose();
			bus.Dispose();
		}

		private void Wait(TimeSpan timeout)
		{
			notifyEvent.WaitOne(timeout);
			notifyEvent.Reset();
		}

		public class TestBusListener : AllJoyn.BusListener
		{
			BusListenerTest _busListenerTest;

			public TestBusListener(BusListenerTest busListenerTest)
			{
				this._busListenerTest = busListenerTest;
			}

			protected override void ListenerRegistered(AllJoyn.BusAttachment busAttachment)
			{
				_busListenerTest.listenerRegistered = true;
				Notify();
			}

			protected override void ListenerUnregistered()
			{
				_busListenerTest.listenerUnregistered = true;
				Notify();
			}

			protected override void FoundAdvertisedName(string name, AllJoyn.TransportMask transport, string namePrefix)
			{
				_busListenerTest.transportFound |= transport;
				_busListenerTest.foundAdvertisedName = true;
				Notify();
			}

			protected override void LostAdvertisedName(string name, AllJoyn.TransportMask transport, string namePrefix)
			{
				_busListenerTest.lostAdvertisedName = true;
				Notify();
			}

			protected override void NameOwnerChanged(string busName, string previousOwner, string newOwner)
			{
				_busListenerTest.nameOwnerChanged = true;
				Notify();
			}

			protected override void BusDisconnected()
			{
				_busListenerTest.busDisconnected = true;
				Notify();
			}

			protected override void BusStopping()
			{
				_busListenerTest.busStopping = true;
				Notify();
			}

			private void Notify()
			{
				_busListenerTest.notifyEvent.Set();
			}
		}
	}
}
