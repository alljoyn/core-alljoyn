//-----------------------------------------------------------------------
// <copyright file="BasicChat.cs" company="AllSeen Alliance.">
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

using UnityEngine;
using AllJoynUnity;
using System.Runtime.InteropServices;
using System.Collections;

namespace basic_clientserver
{
	class BasicChat
	{
		private const string INTERFACE_NAME = "org.alljoyn.bus.chat";
		private const string SERVICE_NAME = "org.alljoyn.bus.chat";
		private const string SERVICE_PATH = "/chat";
		private const ushort SERVICE_PORT = 25;
		
		private static readonly string[] connectArgs = {"unix:abstract=alljoyn",
														"tcp:addr=127.0.0.1,port=9955",
														"launchd:"};
		private string connectedVal;
        

		private static AllJoyn.BusAttachment msgBus;
		private MyBusListener busListener;
		private MySessionPortListener sessionPortListener;
		private static MySessionListener sessionListener;
		private static TestBusObject testObj;
		private AllJoyn.InterfaceDescription testIntf;
		public static string chatText = "";
        public AllJoyn.SessionOpts opts;
		
		public static ArrayList sFoundName = new ArrayList();
		public static string currentJoinedSession = null;
		private static uint currentSessionId = 0;
		private static string myAdvertisedName = null;
		
		public static bool AllJoynStarted = false;
       
		class TestBusObject : AllJoyn.BusObject
		{
			private AllJoyn.InterfaceDescription.Member chatMember;
			
			public TestBusObject(AllJoyn.BusAttachment bus, string path) : base(bus, path, false)
			{
			
				AllJoyn.InterfaceDescription exampleIntf = bus.GetInterface(INTERFACE_NAME);
				AllJoyn.QStatus status = AddInterface(exampleIntf);
				if(!status)
				{
					chatText = "Chat Failed to add interface " + status.ToString() + "\n" + chatText;
					Debug.Log("Chat Failed to add interface " + status.ToString());
				}
				
				chatMember = exampleIntf.GetMember("chat");
			}

			protected override void OnObjectRegistered ()
			{
			
				chatText = "Chat ObjectRegistered has been called\n" + chatText;
				Debug.Log("Chat ObjectRegistered has been called");
			}
			
			public void SendChatSignal(string msg) {
				AllJoyn.MsgArgs payload = new AllJoyn.MsgArgs(1);
				payload[0].Set(msg);
				AllJoyn.QStatus status = Signal(null, currentSessionId, chatMember, payload, 0, 64);
				if(!status) {
					Debug.Log("Chat failed to send signal: "+status.ToString());	
				}
			}
		}

		class MyBusListener : AllJoyn.BusListener
		{
			protected override void FoundAdvertisedName(string name, AllJoyn.TransportMask transport, string namePrefix)
			{
				chatText = "Chat FoundAdvertisedName(name=" + name + ", prefix=" + namePrefix + ")\n" + chatText;
				Debug.Log("Chat FoundAdvertisedName(name=" + name + ", prefix=" + namePrefix + ")");
				if(string.Compare(myAdvertisedName, name) == 0)
				{
					chatText = "Ignoring my advertisement\n" + chatText;
					Debug.Log("Ignoring my advertisement");
				} else if(string.Compare(SERVICE_NAME, namePrefix) == 0)
				{
					sFoundName.Add(name);
				}
			}

            protected override void ListenerRegistered(AllJoyn.BusAttachment busAttachment)
            {
                chatText = "Chat ListenerRegistered: busAttachment=" + busAttachment + "\n" + chatText;
                Debug.Log("Chat ListenerRegistered: busAttachment=" + busAttachment);
            }

			protected override void NameOwnerChanged(string busName, string previousOwner, string newOwner)
			{
				//if(string.Compare(SERVICE_NAME, busName) == 0)
				{
					chatText = "Chat NameOwnerChanged: name=" + busName + ", oldOwner=" +
						previousOwner + ", newOwner=" + newOwner + "\n" + chatText;
					Debug.Log("Chat NameOwnerChanged: name=" + busName + ", oldOwner=" +
						previousOwner + ", newOwner=" + newOwner);
				}
			}
			
			protected override void LostAdvertisedName(string name, AllJoyn.TransportMask transport, string namePrefix)
			{
				chatText = "Chat LostAdvertisedName(name=" + name + ", prefix=" + namePrefix + ")\n" + chatText;
				Debug.Log("Chat LostAdvertisedName(name=" + name + ", prefix=" + namePrefix + ")");
				sFoundName.Remove(name);
			}
		}

		class MySessionPortListener : AllJoyn.SessionPortListener
		{
			protected override bool AcceptSessionJoiner(ushort sessionPort, string joiner, AllJoyn.SessionOpts opts)
			{
			
				if (sessionPort != SERVICE_PORT)
				{
					chatText = "Chat Rejecting join attempt on unexpected session port " + sessionPort + "\n" + chatText;
					Debug.Log("Chat Rejecting join attempt on unexpected session port " + sessionPort);
					return false;
				}
				chatText = "Chat Accepting join session request from " + joiner + 
					" (opts.proximity=" + opts.Proximity + ", opts.traffic=" + opts.Traffic + 
					", opts.transports=" + opts.Transports + ")\n" + chatText;
				Debug.Log("Chat Accepting join session request from " + joiner + 
					" (opts.proximity=" + opts.Proximity + ", opts.traffic=" + opts.Traffic + 
					", opts.transports=" + opts.Transports + ")");
				return true;
			}
					
			protected override void SessionJoined(ushort sessionPort, uint sessionId, string joiner)
			{
				Debug.Log("Session Joined!!!!!!");
				chatText = "Session Joined!!!!!! \n" + chatText;
				currentSessionId = sessionId;
				currentJoinedSession = myAdvertisedName;
				if(sessionListener == null) {
					sessionListener = new MySessionListener();
					msgBus.SetSessionListener(sessionListener, sessionId);
				}
			}
		}
		
		class MySessionListener : AllJoyn.SessionListener
		{
			protected override void	SessionLost(uint sessionId)
			{
				chatText = "SessionLost ("+sessionId+") \n" + chatText;	
				Debug.Log("SessionLost ("+sessionId+")");
			}
			
			protected override void SessionMemberAdded(uint sessionId, string uniqueName)
			{
				chatText = "SessionMemberAdded ("+sessionId+","+uniqueName+") \n" + chatText;	
				Debug.Log("SessionMemberAdded ("+sessionId+","+uniqueName+")");
			}

			protected override void SessionMemberRemoved(uint sessionId, string uniqueName)
			{	
				chatText = "SessionMemberRemoved ("+sessionId+","+uniqueName+") \n" + chatText;	
				Debug.Log("SessionMemberRemoved ("+sessionId+","+uniqueName+")");
			}
		}
		
		public BasicChat()
		{
			StartUp();
		}
		
		
		public void StartUp()
		{
			chatText = "Starting AllJoyn\n\n\n" + chatText;
			AllJoynStarted = true;
			AllJoyn.QStatus status = AllJoyn.QStatus.OK;
			{
				chatText = "Creating BusAttachment\n" + chatText;
				// Create message bus
				msgBus = new AllJoyn.BusAttachment("myApp", true);
	
				// Add org.alljoyn.Bus.method_sample interface
				status = msgBus.CreateInterface(INTERFACE_NAME, false, out testIntf);
				if(status)
				{
				
					chatText = "Chat Interface Created.\n" + chatText;
					Debug.Log("Chat Interface Created.");
					testIntf.AddSignal("chat", "s", "msg", 0);
					testIntf.Activate();
				}
				else
				{
					chatText = "Failed to create interface 'org.alljoyn.Bus.chat'\n" + chatText;
					Debug.Log("Failed to create interface 'org.alljoyn.Bus.chat'");
				}
	
				// Create a bus listener
				busListener = new MyBusListener();
				if(status)
				{
				
					msgBus.RegisterBusListener(busListener);
					chatText = "Chat BusListener Registered.\n" + chatText;
					Debug.Log("Chat BusListener Registered.");
				}
				
				
				if(testObj == null)
					testObj = new TestBusObject(msgBus, SERVICE_PATH);
				
				// Start the msg bus
				if(status)
				{
				
					status = msgBus.Start();
					if(status)
					{
						chatText = "Chat BusAttachment started.\n" + chatText;
						Debug.Log("Chat BusAttachment started.");
						
						msgBus.RegisterBusObject(testObj);
						for (int i = 0; i < connectArgs.Length; ++i)
						{
							chatText = "Chat Connect trying: "+connectArgs[i]+"\n" + chatText;
							Debug.Log("Chat Connect trying: "+connectArgs[i]);
							status = msgBus.Connect(connectArgs[i]);
							if (status)
							{
								chatText = "BusAttchement.Connect(" + connectArgs[i] + ") SUCCEDED.\n" + chatText;
								Debug.Log("BusAttchement.Connect(" + connectArgs[i] + ") SUCCEDED.");
								connectedVal = connectArgs[i];
								break;
							}
							else
							{
								chatText = "BusAttachment.Connect(" + connectArgs[i] + ") failed.\n" + chatText;
								Debug.Log("BusAttachment.Connect(" + connectArgs[i] + ") failed.");
							}
						}
						if(!status)
						{
							chatText = "BusAttachment.Connect failed.\n" + chatText;
							Debug.Log("BusAttachment.Connect failed.");
						}
					}
					else
					{
						chatText = "Chat BusAttachment.Start failed.\n" + chatText;
						Debug.Log("Chat BusAttachment.Start failed.");
					}
				}
				
				myAdvertisedName = SERVICE_NAME+"._"+msgBus.GlobalGUIDString;
				
				AllJoyn.InterfaceDescription.Member chatMember = testIntf.GetMember("chat");
				status = msgBus.RegisterSignalHandler(this.ChatSignalHandler, chatMember, null);
				if(!status)
				{
					chatText ="Chat Failed to add signal handler " + status + "\n" + chatText;
					Debug.Log("Chat Failed to add signal handler " + status);
				}
				else {			
					chatText ="Chat add signal handler " + status + "\n" + chatText;
					Debug.Log("Chat add signal handler " + status);
				}
				
				status = msgBus.AddMatch("type='signal',member='chat'");
				if(!status)
				{
					chatText ="Chat Failed to add Match " + status.ToString() + "\n" + chatText;
					Debug.Log("Chat Failed to add Match " + status.ToString());
				}
				else {			
					chatText ="Chat add Match " + status.ToString() + "\n" + chatText;
					Debug.Log("Chat add Match " + status.ToString());
				}
			}
			
			// Request name
			if(status)
			{
			
				status = msgBus.RequestName(myAdvertisedName,
					AllJoyn.DBus.NameFlags.ReplaceExisting | AllJoyn.DBus.NameFlags.DoNotQueue);
				if(!status)
				{
					chatText ="Chat RequestName(" + SERVICE_NAME + ") failed (status=" + status + ")\n" + chatText;
					Debug.Log("Chat RequestName(" + SERVICE_NAME + ") failed (status=" + status + ")");
				}
			}

			// Create session
			opts = new AllJoyn.SessionOpts(AllJoyn.SessionOpts.TrafficType.Messages, false,
					AllJoyn.SessionOpts.ProximityType.Any, AllJoyn.TransportMask.Any);
			if(status)
			{
			
				ushort sessionPort = SERVICE_PORT;
				sessionPortListener = new MySessionPortListener();
				status = msgBus.BindSessionPort(ref sessionPort, opts, sessionPortListener);
				if(!status || sessionPort != SERVICE_PORT)
				{
					chatText = "Chat BindSessionPort failed (" + status + ")\n" + chatText;
					Debug.Log("Chat BindSessionPort failed (" + status + ")");
				}
				chatText = "Chat BindSessionPort on port (" + sessionPort + ")\n" + chatText;
				Debug.Log("Chat BBindSessionPort on port (" + sessionPort + ")");;
			}

			// Advertise name
			if(status)
			{
				status = msgBus.AdvertiseName(myAdvertisedName, opts.Transports);
				if(!status)
				{
					chatText = "Chat Failed to advertise name " + myAdvertisedName + " (" + status + ")\n" + chatText;
					Debug.Log("Chat Failed to advertise name " + myAdvertisedName + " (" + status + ")");
				}
			}
			
			status = msgBus.FindAdvertisedName(SERVICE_NAME);
			if(!status)
			{
				chatText = "Chat org.alljoyn.Bus.FindAdvertisedName failed.\n" + chatText;
				Debug.Log("Chat org.alljoyn.Bus.FindAdvertisedName failed.");
			}
			
			Debug.Log("Completed ChatService Constructor");
		}
		
		public void ChatSignalHandler(AllJoyn.InterfaceDescription.Member member, string srcPath, AllJoyn.Message message)
		{
			Debug.Log("Client Chat msg - : "+message[0]);
			chatText = "Client Chat msg: ("+message[0]+ ")\n" + chatText;
		}
		
		public void SendTheMsg(string msg) {
			if(currentSessionId != 0) {
				testObj.SendChatSignal(msg);
			}
		}
		
		public void JoinSession(string session)
		{
			if(currentJoinedSession != null)
				LeaveSession();
			AllJoyn.QStatus status = AllJoyn.QStatus.NONE;
			if(sessionListener != null) {
				status = msgBus.SetSessionListener(null, currentSessionId);
				sessionListener = null;
				if(!status) {
	            	chatText = "SetSessionListener failed status(" + status.ToString() + ")\n" + chatText;
					Debug.Log("SetSessionListener status(" + status.ToString() + ")");
				}
			}
			sessionListener = new MySessionListener();
			chatText = "About to call JoinSession (Session=" + session + ")\n" + chatText;
			Debug.Log("About to call JoinSession (Session=" + session + ")");
			status = msgBus.JoinSession(session, SERVICE_PORT, sessionListener, out currentSessionId, opts);
			if(status)
			{
				chatText = "Client JoinSession SUCCESS (Session id=" + currentSessionId + ")\n" + chatText;
				Debug.Log("Client JoinSession SUCCESS (Session id=" + currentSessionId + ")");
				currentJoinedSession = session;
			}
			else
			{
				chatText = "Chat JoinSession failed (status=" + status.ToString() + ")\n" + chatText;
				Debug.Log("Chat JoinSession failed (status=" + status.ToString() + ")");
			}
		}
		
		public void LeaveSession()
		{
			Debug.Log("in LeaveSession.");
			if(currentSessionId != 0) {
				AllJoyn.QStatus status = AllJoyn.QStatus.NONE;
				if(sessionListener != null) {
					Debug.Log("clear session listener");
					status = msgBus.SetSessionListener(null, currentSessionId);
					sessionListener = null;
					if(!status) {
		            	chatText = "SetSessionListener failed status(" + status.ToString() + ")\n" + chatText;
						Debug.Log("SetSessionListener status(" + status.ToString() + ")");
					}
				}
				Debug.Log("about to leave session");
				status = msgBus.LeaveSession(currentSessionId);
				if(status)
				{
					chatText = "Chat LeaveSession SUCCESS (Session id=" + currentSessionId + ")\n" + chatText;
					Debug.Log("Chat LeaveSession SUCCESS (Session id=" + currentSessionId + ")");
					currentSessionId = 0;
					currentJoinedSession = null;
				}
				else
				{
					chatText = "Chat LeaveSession failed (status=" + status.ToString() + ")\n" + chatText;
					Debug.Log("Chat LeaveSession failed (status=" + status.ToString() + ")");
				}
			} else {
				currentJoinedSession = null;
			}
			Debug.Log("done LeaveSession.");
		}
		
		public void CloseDown()
		{	
			if(msgBus == null)
				return; //no need to clean anything up
			AllJoynStarted = false;
			LeaveSession();
			AllJoyn.QStatus status = msgBus.CancelFindAdvertisedName(SERVICE_NAME);
			if(!status) {
            	chatText = "CancelAdvertisedName failed status(" + status.ToString() + ")\n" + chatText;
				Debug.Log("CancelAdvertisedName failed status(" + status.ToString() + ")");
			}
			status = msgBus.CancelAdvertisedName(myAdvertisedName, opts.Transports);
			if(!status) {
            	chatText = "CancelAdvertisedName failed status(" + status.ToString() + ")\n" + chatText;
				Debug.Log("CancelAdvertisedName failed status(" + status.ToString() + ")");
			}
			status = msgBus.ReleaseName(myAdvertisedName);
			if(!status) {
            	chatText = "ReleaseName failed status(" + status.ToString() + ")\n" + chatText;
				Debug.Log("ReleaseName status(" + status.ToString() + ")");
			}
			status = msgBus.UnbindSessionPort(SERVICE_PORT);
			if(!status) {
            	chatText = "UnbindSessionPort failed status(" + status.ToString() + ")\n" + chatText;
				Debug.Log("UnbindSessionPort status(" + status.ToString() + ")");
			}
			
			status = msgBus.Disconnect(connectedVal);
			if(!status) {
            	chatText = "Disconnect failed status(" + status.ToString() + ")\n" + chatText;
				Debug.Log("Disconnect status(" + status.ToString() + ")");
			}
			
			AllJoyn.InterfaceDescription.Member chatMember = testIntf.GetMember("chat");
			status = msgBus.UnregisterSignalHandler(this.ChatSignalHandler, chatMember, null);
			chatMember = null;
			if(!status) {
            	chatText = "UnregisterSignalHandler failed status(" + status.ToString() + ")\n" + chatText;
				Debug.Log("UnregisterSignalHandler status(" + status.ToString() + ")");
			}
			if(sessionListener != null) {
				status = msgBus.SetSessionListener(null, currentSessionId);
				sessionListener = null;
				if(!status) {
	            	chatText = "SetSessionListener failed status(" + status.ToString() + ")\n" + chatText;
					Debug.Log("SetSessionListener status(" + status.ToString() + ")");
				}
			}
			chatText = "No Exceptions(" + status.ToString() + ")\n" + chatText;
			Debug.Log("No Exceptions(" + status.ToString() + ")");
			currentSessionId = 0;
			currentJoinedSession = null;
			sFoundName.Clear();
			
			connectedVal = null;
        	msgBus = null;
			busListener = null;
			sessionPortListener = null;
			testObj = null;
			testIntf = null;
	        opts = null;
			myAdvertisedName = null;
			
			AllJoynStarted = false;
			
			AllJoyn.StopAllJoynProcessing(); //Stop processing alljoyn callbacks
		}
	}
}
