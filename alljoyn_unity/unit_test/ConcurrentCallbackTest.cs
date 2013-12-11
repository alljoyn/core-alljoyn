//-----------------------------------------------------------------------
// <copyright file="ConcurrentCallbackTest.cs" company="AllSeen Alliance.">
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
	public class ConcurrentCallbackTest : IDisposable
	{
		static AllJoyn.BusAttachment mbus;
		AllJoyn.BusListener busListener = null;

		AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;

		public const string ObjectName = "org.alljoyn.test.ConcurrentCallbackTest";
		public TimeSpan MaxWaitTime = TimeSpan.FromSeconds(5);
		public bool listenerRegisteredFlag;
		public bool nameOwnerChangedFlag;
		public AllJoyn.QStatus callbackStatus;

		public AutoResetEvent notifyEvent = new AutoResetEvent(false);

		private void Wait(TimeSpan timeout)
		{
			notifyEvent.WaitOne(timeout);
			notifyEvent.Reset();
		}

		// we need this so that we know when the advertised name has been found
		class BusListenerWithBlockingCall : AllJoyn.BusListener
		{
			ConcurrentCallbackTest _concurrentCallbackTest;

			private void Notify()
			{
				_concurrentCallbackTest.notifyEvent.Set();
			}

			public BusListenerWithBlockingCall(ConcurrentCallbackTest concurrentCallbackTest)
			{
				this._concurrentCallbackTest = concurrentCallbackTest;
			}

			protected override void ListenerRegistered(AllJoyn.BusAttachment busAttachment)
			{
				_concurrentCallbackTest.listenerRegisteredFlag = true;
				Notify();
			}

			protected override void NameOwnerChanged(string busName, string previousOwner, string newOwner)
			{
				AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;
				AllJoyn.ProxyBusObject proxy = new AllJoyn.ProxyBusObject(mbus, "org.alljoyn.Bus", "/org/alljoyn/Bus", 0);
				Assert.NotNull(proxy);
				status = proxy.IntrospectRemoteObject();
				proxy.Dispose();
				_concurrentCallbackTest.callbackStatus = status;
				_concurrentCallbackTest.nameOwnerChangedFlag = true;
				Notify();

			}
		}

		private bool disposed = false;

		public ConcurrentCallbackTest()
		{
			mbus = new AllJoyn.BusAttachment("BusListenerTest", true);

			// start the bus attachment
			status = mbus.Start();
			Assert.Equal(AllJoyn.QStatus.OK, status);

			// connect to the bus
			status = mbus.Connect(AllJoynTestCommon.GetConnectSpec());
			Assert.Equal(AllJoyn.QStatus.OK, status);
		}

		~ConcurrentCallbackTest()
		{
			Dispose(false);
		}

		protected virtual void Dispose(bool disposing)
		{
			if (!this.disposed)
			{
				mbus.ReleaseName(ObjectName);

				mbus.UnregisterBusListener(busListener);
				mbus.Stop();
				mbus.Join();

				if (disposing)
				{
					mbus.Dispose();
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
		public void EnableConcurrentCallbacks_Not_Used()
		{
			callbackStatus = AllJoyn.QStatus.FAIL;
			listenerRegisteredFlag = false;
			nameOwnerChangedFlag = false;

			busListener = new BusListenerWithBlockingCall(this);

			mbus.RegisterBusListener(busListener);
			Wait(MaxWaitTime);
			Assert.True(listenerRegisteredFlag);

			mbus.RequestName(ObjectName, 0);
			Wait(MaxWaitTime);
			Assert.True(nameOwnerChangedFlag);
			/*
			 * Because of the way that callback functions are defered we can still make
			 * what would be a blocking call in alljoyn_core and it is not a blocking
			 * call in Unity.  This is a by product of the alljoyn_c deffered callback class
			 * and its usage.  I am still investigating ways to work around issues caused
			 * by the deffered callback class at some point in the future may start to work
			 * as alljoyn_core.
			 * Assert.Equal(AllJoyn.QStatus.BUS_BLOCKING_CALL_NOT_ALLOWED, callbackStatus);
			 */
			Assert.Equal(AllJoyn.QStatus.OK, callbackStatus);
		}

		// we need this so that we know when the advertised name has been found
		class BusListenerEnableConcurrentCallbacks : AllJoyn.BusListener
		{
			ConcurrentCallbackTest _concurrentCallbackTest;

			private void Notify()
			{
				_concurrentCallbackTest.notifyEvent.Set();
			}

			public BusListenerEnableConcurrentCallbacks(ConcurrentCallbackTest concurrentCallbackTest)
			{
				this._concurrentCallbackTest = concurrentCallbackTest;
			}

			protected override void ListenerRegistered(AllJoyn.BusAttachment busAttachment)
			{
				_concurrentCallbackTest.listenerRegisteredFlag = true;
				Notify();
			}

			protected override void NameOwnerChanged(string busName, string previousOwner, string newOwner)
			{
				AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;
				AllJoyn.ProxyBusObject proxy = new AllJoyn.ProxyBusObject(mbus, "org.alljoyn.Bus", "/org/alljoyn/Bus", 0);
				Assert.NotNull(proxy);
				mbus.EnableConcurrentCallbacks();
				status = proxy.IntrospectRemoteObject();
				proxy.Dispose();
				_concurrentCallbackTest.callbackStatus = status;
				_concurrentCallbackTest.nameOwnerChangedFlag = true;
				Notify();

			}
		}

		[Fact]
		public void EnableConcurrentCallbacks_Used()
		{
			callbackStatus = AllJoyn.QStatus.FAIL;
			listenerRegisteredFlag = false;
			nameOwnerChangedFlag = false;

			busListener = new BusListenerEnableConcurrentCallbacks(this);

			mbus.RegisterBusListener(busListener);
			Wait(MaxWaitTime);
			Assert.True(listenerRegisteredFlag);

			mbus.RequestName(ObjectName, 0);
			Wait(MaxWaitTime);
			Assert.True(nameOwnerChangedFlag);
			Assert.Equal(AllJoyn.QStatus.OK, callbackStatus);
		}
	}
}
