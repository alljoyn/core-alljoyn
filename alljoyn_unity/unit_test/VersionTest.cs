//-----------------------------------------------------------------------
// <copyright file="VersionTest.cs" company="AllSeen Alliance.">
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
	public class VersionTest
	{
		[Fact]
		public void GetExtensionVersion()
		{
			// Currently this value is not well defined so it may change
			// in an upcomming release
			// A string of form #.#.# is returned
			string extensionVer = AllJoyn.GetExtensionVersion();
			string[] extVer = extensionVer.Split('.');
			Assert.Equal(3, extVer.Length);
			foreach(string s in extVer)
			{
				int aaa;
				Assert.True(int.TryParse(s, out aaa));
			}
		}

		[Fact]
		public void GetVersion()
		{
			// version is expecte to be a string of type v#.#.# where # represents a
			// number of unknown length. This test code is most likely more complex than
			// the code used to generate the string but it should handle any value 
			// returned
			string version = AllJoyn.GetVersion();
			Assert.Equal('v', version[0]);
			string[] versionLevels = version.Substring(1).Split('.');

			Assert.Equal(3, versionLevels.Length);
			foreach (string level in versionLevels)
			{
				int aaa;
				Assert.True(int.TryParse(level, out aaa));
			}
		}

		[Fact]
		public void GetBuildInfo()
		{
			// GetBuildInfo is expecte to be a string of type 
			// AllJoyn Library v#.#.# (Built <weekday> <month> dd  hh:mm:ss UTC yyyy by <username>) 
			// This test code is most likely more complex than
			// the code used to generate the string but it should handle any value 
			// returned
			string buildInfo = AllJoyn.GetBuildInfo();
			string failMsg = "Expected the BuildInfo string to start with 'AllJoyn Library' actual string was \n>>>\t " + buildInfo;
			Assert.True(buildInfo.StartsWith("AllJoyn Library"), failMsg);
			string[] bInfo = buildInfo.Split(' ');

			//dummy value to pass into int.TryParse
			int aaa;
			//already checked that the string started with 'AllJoyn Library'
			string[] versionLevels = bInfo[2].Substring(1).Split('.');
			Assert.Equal(3, versionLevels.Length);
			foreach (string s in versionLevels)
			{
				Assert.True(int.TryParse(s, out aaa));
			}
			Assert.True(bInfo[3].Equals("(Built"));
			//abbreviated weekday name.  In the default locale, it is equivalent
			//to one of the following: Sun, Mon, Tue, Wed, Thu, Fri or Sat.
			Assert.Equal(3, bInfo[4].Length);
			// locale's abbreviated month name.  In the default
			//locale, it is equivalent to one of the following: Jan, Feb,
			//Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov or Dec.
			Assert.Equal(3, bInfo[5].Length);
			//day of the month
			Assert.True(int.TryParse(bInfo[6], out aaa));
			//time hh:mm:ss
			string[] time = bInfo[7].Split(':');
			Assert.Equal(3, time.Length);
			foreach(string s in time)
			{
				Assert.True(int.TryParse(s, out aaa));
			}
			Assert.True(bInfo[8].Equals("UTC"));
			//year yyyy
			Assert.True(int.TryParse(bInfo[9], out aaa));
			Assert.True(bInfo[10].Equals("by"));
			//bInfo[11] is the user name will not test
			//we know the string should end in ')'
			Assert.Equal(')', bInfo[11][bInfo[11].Length-1]);
		}

		[Fact]
		public void GetNumericVersion()
		{
			// This will only test that a number larger than 0 is returned
			uint v = AllJoyn.GetNumericVersion();
			Assert.True( v > 0);
		}
	}
}