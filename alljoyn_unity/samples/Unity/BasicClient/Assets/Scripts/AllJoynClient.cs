//-----------------------------------------------------------------------
// <copyright file="AllJoynClient.cs" company="AllSeen Alliance.">
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
using basic_client;

public class AllJoynClient : MonoBehaviour
{
	void OnGUI ()
	{
		if(BasicClient.clientText != null){
			GUI.TextArea (new Rect (0, 0, Screen.width, Screen.height), BasicClient.clientText);
		}
	}
	
	// Use this for initialization
	void Start()
	{
		Debug.Log("Starting up AllJoyn client");
		basicClient = new BasicClient();
	}

	// Update is called once per frame
    void Update()
	{
		if(basicClient != null && basicClient.FoundAdvertisedName)
		{
			basicClient.ConnectToFoundName();	
		}
		else if(basicClient != null && basicClient.Connected)
		{
		    Debug.Log("BasicClient.CallRemoteMethod returned '" + basicClient.CallRemoteMethod() + "'");
		}
        if (Input.GetKeyDown(KeyCode.Escape)) { Application.Quit(); }
		if (BasicClient.clientText.Length > 2048){ BasicClient.clientText = "";}
	}

	BasicClient basicClient;
}
