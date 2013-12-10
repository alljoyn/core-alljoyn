//-----------------------------------------------------------------------
// <copyright file="AllJoynClientServer.cs" company="AllSeen Alliance.">
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
using basic_clientserver;

public class AllJoynClientServer : MonoBehaviour
{
	
	private string msgText = "";
	
	private static int BUTTON_SIZE = 75;
	private long spamCount = 0;
	
	private bool spamMessages = false;
	
	void OnGUI ()
	{
		if(BasicChat.chatText != null){
			GUI.TextArea(new Rect (0, 0, Screen.width, (Screen.height / 2)), BasicChat.chatText);
		}
		int i = 0;
		int xStart = (Screen.height / 2)+10+((i++)*BUTTON_SIZE);
		
		if(BasicChat.AllJoynStarted) {
			if(GUI.Button(new Rect(0,xStart,(Screen.width)/3, BUTTON_SIZE),"STOP ALLJOYN"))
			{	
				basicChat.CloseDown();
			}
		}
		
		if(BasicChat.currentJoinedSession != null) {
			if(GUI.Button(new Rect(((Screen.width)/3),xStart,(Screen.width)/3, BUTTON_SIZE),
				"Leave \n"+BasicChat.currentJoinedSession.Substring(BasicChat.currentJoinedSession.LastIndexOf("."))))
			{	
				basicChat.LeaveSession();
			}
		}
		
		if(!BasicChat.AllJoynStarted) {
			if(GUI.Button(new Rect(((Screen.width)/3)*2,xStart,(Screen.width)/3, BUTTON_SIZE), "START ALLJOYN"))
			{	
				basicChat.StartUp();
			}
		}
		
		foreach(string name in BasicChat.sFoundName){
			xStart = (Screen.height / 2)+10+((i++)*BUTTON_SIZE);
			if(GUI.Button(new Rect(10,xStart,(Screen.width-20), BUTTON_SIZE),name.Substring(name.LastIndexOf("."))))
			{	
				basicChat.JoinSession(name);
			}
		}
		
		msgText = GUI.TextField(new Rect (0, Screen.height-BUTTON_SIZE, (Screen.width/4) * 3, BUTTON_SIZE), msgText);
		if(GUI.Button(new Rect(Screen.width - (Screen.width/4),Screen.height-BUTTON_SIZE, (Screen.width/4), BUTTON_SIZE),"Send"))
		{	
			basicChat.SendTheMsg(msgText);
			//Debug easter egg
			if(string.Compare("spam",msgText) == 0)
				spamMessages = true;
			else if(string.Compare("stop",msgText) == 0)
			{
				spamMessages = false;
				spamCount = 0;
			}
		}
	}
	
	// Use this for initialization
	void Start()
	{
		Debug.Log("Starting up AllJoyn service and client");
		basicChat = new BasicChat();
	}

	// Update is called once per frame
    void Update()
	{
        if (Input.GetKeyDown(KeyCode.Escape)) {
			basicChat.CloseDown();
			Application.Quit();
		}
		if(spamMessages) {
			basicChat.SendTheMsg("("+(spamCount++)+") Spam: "+System.DateTime.Today.Millisecond);
		}
	}

	BasicChat basicChat;
}
