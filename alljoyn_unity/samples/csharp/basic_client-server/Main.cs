//-----------------------------------------------------------------------
// <copyright file="Main.cs" company="AllSeen Alliance.">
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
using AllJoynUnity;

namespace basic_clientserver
{
	class MainClass
	{
		public static void Main(string[] args)
		{
			Console.WriteLine("AllJoyn Library version: " + AllJoyn.GetVersion());
			Console.WriteLine("AllJoyn Library buildInfo: " + AllJoyn.GetBuildInfo());

			BasicServer basicServer = new BasicServer();
			BasicClient basicClient = new BasicClient();

			basicClient.Connect();

			while(!basicClient.Connected)
			{
				System.Threading.Thread.Sleep(1);
			}

			Console.WriteLine("BasicClient.CallRemoteMethod returned '{0}'", basicClient.CallRemoteMethod());

			while(basicServer.KeepRunning)
			{
				System.Threading.Thread.Sleep(1);
				//System.GC.Collect();
				//System.GC.WaitForPendingFinalizers();
				//System.GC.WaitForFullGCComplete();
				//System.GC.Collect();
				Console.WriteLine("BasicClient.CallRemoteMethod returned '{0}'", basicClient.CallRemoteMethod());
			}
		}
	}
}
