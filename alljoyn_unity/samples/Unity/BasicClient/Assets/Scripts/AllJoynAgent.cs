//-----------------------------------------------------------------------
// <copyright file="AllJoynAgent.cs" company="AllSeen Alliance.">
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

// The AllJoynAgent prefab must exist once, and only once, in your scene.
// This prefab/behavior will take care of the background loading and
// message processing required to use AllJoyn with Unity.
//
// In addition, the AllJoynAgent.cs script must execute before any other
// script which uses AllJoyn. Most Unity scripts use Start() for initialization,
// and the AllJoynAgent uses Awake(). In most cases this will ensure that
// the required code is initialized prior to any other AllJoyn code.
//
// If this is not the case in your project, go to the Unity editor menu at:
//   Edit->Project Settings->Script Execution Order
// and drag the AllJoynAgent.cs script into the execution order above 'Default Time',
// or enter the value '-100' for the execution time.
public class AllJoynAgent : MonoBehaviour
{
	// Awake() is called before any calls to Start() on any game object are made.
	void Awake()
	{
		// Output AllJoyn version information to log
		Debug.Log("AllJoyn Library version: " + AllJoyn.GetVersion());
		Debug.Log("AllJoyn Library buildInfo: " + AllJoyn.GetBuildInfo());
	}
	
	void OnApplicationQuit()
	{
		AllJoyn.StopAllJoynProcessing();
	}
}
