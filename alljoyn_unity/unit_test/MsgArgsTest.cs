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
using AllJoynUnity;
using Xunit;

namespace AllJoynUnityTest
{
	public class MsgArgsTest
	{
		//We are specifically testing an obsolete class
		//don't warn about usage of obsolete classes.
#pragma warning disable 618
		[Fact]
		public void BasicAssignment()
		{
			AllJoyn.MsgArgs arg = new AllJoyn.MsgArgs(1);
			arg[0] = (byte)42;
			Assert.Equal((byte)42, (byte)arg[0]);
			arg[0] = true;
			Assert.True((bool)arg[0]);

			arg[0] = false;
			Assert.False((bool)arg[0]);

			arg[0] = (short)42;
			Assert.Equal((short)42, (short)arg[0]);

			arg[0] = (ushort)0xBEBE;
			Assert.Equal((ushort)0xBEBE, (ushort)arg[0]);

			arg[0] = (int)-9999;
			Assert.Equal((int)-9999, (int)arg[0]);

			arg[0] = (uint)0x32323232;
			Assert.Equal((uint)0x32323232, (uint)arg[0]);

			arg[0] = (long)-1;
			Assert.Equal((long)-1, (long)arg[0]);

			arg[0] = (ulong)0x6464646464646464;
			Assert.Equal((ulong)0x6464646464646464, (ulong)arg[0]);

			arg[0] = (float)1.61803f;
			Assert.Equal((float)1.61803f, (float)arg[0]);

			arg[0] = (double)3.14159265D;
			Assert.Equal((double)3.14159265D, (double)arg[0]);

			arg[0] = (string)"this is a string";
			Assert.Equal("this is a string", (string)arg[0]);

			arg[0].ObjectPath = "/org/foo/bar";
			Assert.Equal("/org/foo/bar", arg[0].ObjectPath);

			//assignment of an array
			AllJoyn.MsgArgs arg3 = new AllJoyn.MsgArgs(3);
			arg3[0] = (byte)5;
			arg3[1] = (byte)13;
			arg3[2] = (byte)42;

			Assert.Equal((byte)5, (byte)arg3[0]);
			Assert.Equal((byte)13, (byte)arg3[1]);
			Assert.Equal((byte)42, (byte)arg3[2]);
		}

		[Fact]
		public void BasicSet()
		{
			AllJoyn.MsgArgs arg = new AllJoyn.MsgArgs(1);

			Assert.Equal(AllJoyn.QStatus.OK, arg[0].Set((byte)13));
			Assert.Equal((byte)13, (byte)arg[0]);

			Assert.Equal(AllJoyn.QStatus.OK, arg[0].Set(true));
			Assert.True((bool)arg[0]);

			Assert.Equal(AllJoyn.QStatus.OK, arg[0].Set((short)42));
			Assert.Equal((short)42, (short)arg[0]);

			Assert.Equal(AllJoyn.QStatus.OK, arg[0].Set((ushort)0xBEBE));
			Assert.Equal((ushort)0xBEBE, (ushort)arg[0]);

			Assert.Equal(AllJoyn.QStatus.OK, arg[0].Set((int)-9999));
			Assert.Equal((int)-9999, (int)arg[0]);

			Assert.Equal(AllJoyn.QStatus.OK, arg[0].Set((uint)0x32323232));
			Assert.Equal((uint)0x32323232, (uint)arg[0]);

			Assert.Equal(AllJoyn.QStatus.OK, arg[0].Set((long)-1));
			Assert.Equal((long)-1, (long)arg[0]);

			Assert.Equal(AllJoyn.QStatus.OK, arg[0].Set((ulong)0x6464646464646464));
			Assert.Equal((ulong)0x6464646464646464, (ulong)arg[0]);

			Assert.Equal(AllJoyn.QStatus.OK, arg[0].Set((float)1.61803f));
			Assert.Equal((float)1.61803f, (float)arg[0]);

			Assert.Equal(AllJoyn.QStatus.OK, arg[0].Set((double)3.14159265D));
			Assert.Equal((double)3.14159265D, (double)arg[0]);

			Assert.Equal(AllJoyn.QStatus.OK, arg[0].Set((string)"this is a string"));
			Assert.Equal("this is a string", (string)arg[0]);

			arg[0].ObjectPath = "/org/foo/bar";
			Assert.Equal("/org/foo/bar", arg[0].ObjectPath);

			// todo  no support for signature type
		}

		[Fact]
		public void BasicArrayAssignment()
		{
			AllJoyn.MsgArgs arg = new AllJoyn.MsgArgs(1);

			//byte
			byte[] in_byte_array = { 9, 19, 29, 39, 49 };
			arg[0] = in_byte_array;
			byte[] out_byte_array = (byte[])arg[0];
			Assert.Equal(in_byte_array.Length, out_byte_array.Length);
			for (int i = 0; i < out_byte_array.Length; i++)
			{
				Assert.Equal(in_byte_array[i], out_byte_array[i]);
			}

			//bool
			bool[] in_bool_array = { false, false, true, true, false, true };
			arg[0].Set(in_bool_array);
			bool[] out_bool_array = (bool[])arg[0];
			Assert.Equal(in_bool_array.Length, out_bool_array.Length);
			for (int i = 0; i < out_bool_array.Length; i++)
			{
				Assert.Equal(in_bool_array[i], out_bool_array[i]);
			}

			//short
			short[] in_short_array = { -9, -99, 999, 9999 };
			arg[0] = in_short_array;
			short[] out_short_array = (short[])arg[0];
			Assert.Equal(in_short_array.Length, out_short_array.Length);
			for (int i = 0; i < out_short_array.Length; i++)
			{
				Assert.Equal(in_short_array[i], out_short_array[i]);
			}

			//ushort
			ushort[] in_ushort_array = { 9, 99, 999, 9999 };
			arg[0] = in_ushort_array;
			ushort[] out_ushort_array = (ushort[])arg[0];
			Assert.Equal(in_ushort_array.Length, out_ushort_array.Length);
			for (int i = 0; i < out_short_array.Length; i++)
			{
				Assert.Equal(in_ushort_array[i], out_ushort_array[i]);
			}

			//int
			int[] in_int_array = { -8, -88, 888, 8888 };
			arg[0] = in_int_array;
			int[] out_int_array = (int[])arg[0];
			Assert.Equal(in_int_array.Length, out_int_array.Length);
			for (int i = 0; i < out_int_array.Length; i++)
			{
				Assert.Equal(in_int_array[i], out_int_array[i]);
			}

			//uint
			uint[] in_uint_array = { 8, 88, 888, 8888 };
			arg[0] = in_uint_array;
			uint[] out_uint_array = (uint[])arg[0];
			Assert.Equal(in_uint_array.Length, out_uint_array.Length);
			for (int i = 0; i < out_int_array.Length; i++)
			{
				Assert.Equal(in_uint_array[i], out_uint_array[i]);
			}

			//long
			long[] in_long_array = { -7, -77, 777, 7777 };
			arg[0] = in_long_array;
			long[] out_long_array = (long[])arg[0];
			Assert.Equal(in_long_array.Length, out_long_array.Length);
			for (int i = 0; i < out_long_array.Length; i++)
			{
				Assert.Equal(in_long_array[i], out_long_array[i]);
			}

			//ulong
			ulong[] in_ulong_array = { 7, 77, 777, 7777 };
			arg[0] = in_ulong_array;
			ulong[] out_ulong_array = (ulong[])arg[0];
			Assert.Equal(in_ulong_array.Length, out_ulong_array.Length);
			for (int i = 0; i < out_long_array.Length; i++)
			{
				Assert.Equal(in_ulong_array[i], out_ulong_array[i]);
			}

			//double
			double[] in_double_array = { 0.001, 0.01, 0.1, 1.0, 10.0, 100.0 };
			arg[0] = in_double_array;
			double[] out_double_array = (double[])arg[0];
			Assert.Equal(in_double_array.Length, out_double_array.Length);
			for (int i = 0; i < out_long_array.Length; i++)
			{
				Assert.Equal(in_double_array[i], out_double_array[i]);
			}

			//string
			string[] in_string_array = { "one", "two", "three", "four" };
			arg[0] = in_string_array;
			string[] out_string_array = (string[])arg[0];
			Assert.Equal(in_string_array.Length, out_string_array.Length);
			for (int i = 0; i < out_string_array.Length; i++)
			{
				Assert.Equal(in_string_array[i], out_string_array[i]);
			}
		}

		[Fact]
		public void BasicArraySet()
		{
			AllJoyn.MsgArgs arg = new AllJoyn.MsgArgs(1);

			//byte
			byte[] in_byte_array = { 9, 19, 29, 39, 49 };
			arg[0].Set(in_byte_array);
			byte[] out_byte_array = (byte[])arg[0];
			Assert.Equal(in_byte_array.Length, out_byte_array.Length);
			for (int i = 0; i < out_byte_array.Length; i++)
			{
				Assert.Equal(in_byte_array[i], out_byte_array[i]);
			}

			//bool
			bool[] in_bool_array = { false, false, true, true, false };
			arg[0].Set(in_bool_array);
			bool[] out_bool_array = (bool[])arg[0];
			Assert.Equal(in_bool_array.Length, out_bool_array.Length);
			for (int i = 0; i < out_bool_array.Length; i++)
			{
				Assert.Equal(in_bool_array[i], out_bool_array[i]);
			}

			//short
			short[] in_short_array = { -9, -99, 999, 9999 };
			arg[0].Set(in_short_array);
			short[] out_short_array = (short[])arg[0];
			Assert.Equal(in_short_array.Length, out_short_array.Length);
			for (int i = 0; i < out_short_array.Length; i++)
			{
				Assert.Equal(in_short_array[i], out_short_array[i]);
			}

			//ushort
			ushort[] in_ushort_array = { 9, 99, 999, 9999 };
			arg[0].Set(in_ushort_array);
			ushort[] out_ushort_array = (ushort[])arg[0];
			Assert.Equal(in_ushort_array.Length, out_ushort_array.Length);
			for (int i = 0; i < out_short_array.Length; i++)
			{
				Assert.Equal(in_ushort_array[i], out_ushort_array[i]);
			}

			//int
			int[] in_int_array = { -8, -88, 888, 8888 };
			arg[0].Set(in_int_array);
			int[] out_int_array = (int[])arg[0];
			Assert.Equal(in_int_array.Length, out_int_array.Length);
			for (int i = 0; i < out_int_array.Length; i++)
			{
				Assert.Equal(in_int_array[i], out_int_array[i]);
			}

			//uint
			uint[] in_uint_array = { 8, 88, 888, 8888 };
			arg[0].Set(in_uint_array);
			uint[] out_uint_array = (uint[])arg[0];
			Assert.Equal(in_uint_array.Length, out_uint_array.Length);
			for (int i = 0; i < out_int_array.Length; i++)
			{
				Assert.Equal(in_uint_array[i], out_uint_array[i]);
			}

			//long
			long[] in_long_array = { -7, -77, 777, 7777 };
			arg[0].Set(in_long_array);
			long[] out_long_array = (long[])arg[0];
			Assert.Equal(in_long_array.Length, out_long_array.Length);
			for (int i = 0; i < out_long_array.Length; i++)
			{
				Assert.Equal(in_long_array[i], out_long_array[i]);
			}

			//ulong
			ulong[] in_ulong_array = { 7, 77, 777, 7777 };
			arg[0].Set(in_ulong_array);
			ulong[] out_ulong_array = (ulong[])arg[0];
			Assert.Equal(in_ulong_array.Length, out_ulong_array.Length);
			for (int i = 0; i < out_long_array.Length; i++)
			{
				Assert.Equal(in_ulong_array[i], out_ulong_array[i]);
			}

			//double
			double[] in_double_array = { 0.001, 0.01, 0.1, 1.0, 10.0, 100.0 };
			arg[0].Set(in_double_array);
			double[] out_double_array = (double[])arg[0];
			Assert.Equal(in_double_array.Length, out_double_array.Length);
			for (int i = 0; i < out_long_array.Length; i++)
			{
				Assert.Equal(in_double_array[i], out_double_array[i]);
			}

			//string
			string[] in_string_array = { "one", "two", "three", "four" };
			arg[0].Set(in_string_array);
			string[] out_string_array = (string[])arg[0];
			Assert.Equal(in_string_array.Length, out_string_array.Length);
			for (int i = 0; i < out_string_array.Length; i++)
			{
				Assert.Equal(in_string_array[i], out_string_array[i]);
			}
		}
#pragma warning restore 618
	}
}