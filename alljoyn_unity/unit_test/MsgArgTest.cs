//-----------------------------------------------------------------------
// <copyright file="MsgArgTest.cs" company="AllSeen Alliance.">
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
using AllJoynUnity;
using Xunit;

namespace AllJoynUnityTest
{
	public class MsgArgTest
	{
		[Fact]
		public void BasicAssignment()
		{

			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();
			arg = (byte)42;
			Assert.Equal((byte)42, (byte)arg);
			arg = true;
			Assert.True((bool)arg);

			arg = false;
			Assert.False((bool)arg);

			arg = (short)42;
			Assert.Equal((short)42, (short)arg);

			arg = (ushort)0xBEBE;
			Assert.Equal((ushort)0xBEBE, (ushort)arg);

			arg = (int)-9999;
			Assert.Equal((int)-9999, (int)arg);

			arg = (uint)0x32323232;
			Assert.Equal((uint)0x32323232, (uint)arg);

			arg = (long)-1;
			Assert.Equal((long)-1, (long)arg);

			arg = (ulong)0x6464646464646464;
			Assert.Equal((ulong)0x6464646464646464, (ulong)arg);

			arg = (float)1.61803f;
			Assert.Equal((float)1.61803f, (float)arg);

			arg = (double)3.14159265D;
			Assert.Equal((double)3.14159265D, (double)arg);

			arg = (string)"this is a string";
			Assert.Equal("this is a string", (string)arg);

			AllJoyn.MsgArg arg10 = new AllJoyn.MsgArg(10);
			arg10[0] = (byte)5;
			arg10[1] = true;
			arg10[2] = (short)42;
			arg10[3] = (ushort)45;
			arg10[4] = (int)99;
			arg10[5] = (uint)499;
			arg10[6] = (long)566;
			arg10[7] = (ulong)789301;
			arg10[8] = (double)2.7275;
			arg10[9] = (string)"I say Hello";

			Assert.Equal((byte)5, (byte)arg10[0]);
			Assert.Equal(true, (bool)arg10[1]);
			Assert.Equal((short)42, (short)arg10[2]);
			Assert.Equal((ushort)45, (ushort)arg10[3]);
			Assert.Equal((int)99, (int)arg10[4]);
			Assert.Equal((uint)499, (uint)arg10[5]);
			Assert.Equal((long)566, (long)arg10[6]);
			Assert.Equal((ulong)789301, (ulong)arg10[7]);
			Assert.Equal((double)2.7275, (double)arg10[8]);
			Assert.Equal((string)"I say Hello", (string)arg10[9]);

			//assignment of an array
			AllJoyn.MsgArg arg3 = new AllJoyn.MsgArg(3);
			arg3[0] = (byte)5;
			arg3[1] = (byte)13;
			arg3[2] = (byte)42;

			Assert.Equal((byte)5, (byte)arg3[0]);
			Assert.Equal((byte)13, (byte)arg3[1]);
			Assert.Equal((byte)42, (byte)arg3[2]);

			//older test code to be thrown removed
			AllJoyn.MsgArg argfoo = new AllJoyn.MsgArg();
			argfoo = -9999;
			Assert.Equal((int)-9999, (int)argfoo);

			AllJoyn.MsgArg args = new AllJoyn.MsgArg(10);
			for (int i = 0; i < 10; ++i)
			{
				args[i] = i;
			}

			for (int i = 0; i < 10; ++i)
			{
				Assert.Equal((int)i, (int)args[i]);
			}
		}

		[Fact]
		public void BasicSet()
		{
			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((byte)13));
			Assert.Equal((byte)13, (byte)arg);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set(true));
			Assert.True((bool)arg);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((short)42));
			Assert.Equal((short)42, (short)arg);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((ushort)0xBEBE));
			Assert.Equal((ushort)0xBEBE, (ushort)arg);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((int)-9999));
			Assert.Equal((int)-9999, (int)arg);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((uint)0x32323232));
			Assert.Equal((uint)0x32323232, (uint)arg);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((long)-1));
			Assert.Equal((long)-1, (long)arg);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((ulong)0x6464646464646464));
			Assert.Equal((ulong)0x6464646464646464, (ulong)arg);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((float)1.61803f));
			Assert.Equal((float)1.61803f, (float)arg);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((double)3.14159265D));
			Assert.Equal((double)3.14159265D, (double)arg);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((string)"this is a string"));
			Assert.Equal("this is a string", (string)arg);

			AllJoyn.MsgArg arg10 = new AllJoyn.MsgArg(10);
			arg10[0].Set((byte)5);
			arg10[1].Set(true);
			arg10[2].Set((short)42);
			arg10[3].Set((ushort)45);
			arg10[4].Set((int)99);
			arg10[5].Set((uint)499);
			arg10[6].Set((long)566);
			arg10[7].Set((ulong)789301);
			arg10[8].Set((double)2.7275);
			arg10[9].Set((string)"I say Hello");

			Assert.Equal((byte)5, (byte)arg10[0]);
			Assert.Equal(true, (bool)arg10[1]);
			Assert.Equal((short)42, (short)arg10[2]);
			Assert.Equal((ushort)45, (ushort)arg10[3]);
			Assert.Equal((int)99, (int)arg10[4]);
			Assert.Equal((uint)499, (uint)arg10[5]);
			Assert.Equal((long)566, (long)arg10[6]);
			Assert.Equal((ulong)789301, (ulong)arg10[7]);
			Assert.Equal((double)2.7275, (double)arg10[8]);
			Assert.Equal((string)"I say Hello", (string)arg10[9]);

			//older test code to be thrown removed
			AllJoyn.MsgArg argfoo = new AllJoyn.MsgArg();
			argfoo.Set(-9999);
			Assert.Equal((int)-9999, (int)argfoo);

			AllJoyn.MsgArg args = new AllJoyn.MsgArg(10);
			for (int i = 0; i < 10; ++i)
			{
				args[i].Set(i * 3);
			}

			for (int i = 0; i < 10; ++i)
			{
				Assert.Equal((int)i * 3, (int)args[i]);
			}
		}

		[Fact]
		public void BasicGetBasicTypes()
		{
			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((byte)13));
			byte y;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get(out y));
			Assert.Equal((byte)13, y);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set(true));
			bool b;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get(out b));
			Assert.True(b);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((short)42));
			short n;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get(out n));
			Assert.Equal((short)42, n);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((ushort)0xBEBE));
			ushort q;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get(out q));
			Assert.Equal((ushort)0xBEBE, q);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((int)-9999));
			int i;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get(out i));
			Assert.Equal((int)-9999, i);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((uint)0x32323232));
			uint u;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get(out u));
			Assert.Equal((uint)0x32323232, u);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((long)-1));
			long x;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get(out x));
			Assert.Equal((long)-1, x);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((ulong)0x6464646464646464));
			ulong t;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get(out t));
			Assert.Equal((ulong)0x6464646464646464, t);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((double)3.14159265D));
			double d;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get(out d));
			Assert.Equal((double)3.14159265D, d);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((string)"this is a string"));
			string s;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get(out s));
			Assert.Equal("this is a string", s);
		}

		[Fact]
		public void BasicGet()
		{
			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((byte)13));
			object y;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("y", out y));
			Assert.Equal((byte)13, (byte)y);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set(true));
			object b;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("b", out b));
			Assert.True((bool)b);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((short)42));
			object n;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("n", out n));
			Assert.Equal((short)42, (short)n);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((ushort)0xBEBE));
			object q;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("q", out q));
			Assert.Equal((ushort)0xBEBE, (ushort)q);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((int)-9999));
			object i;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("i", out i));
			Assert.Equal((int)-9999, (object)i);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((uint)0x32323232));
			object u;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("u", out u));
			Assert.Equal((uint)0x32323232, (uint)u);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((long)-1));
			object x;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("x", out x));
			Assert.Equal((long)-1, (long)x);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((ulong)0x6464646464646464));
			object t;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("t", out t));
			Assert.Equal((ulong)0x6464646464646464, (ulong)t);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((double)3.14159265D));
			object d;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("d", out d));
			Assert.Equal((double)3.14159265D, (double)d);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set((string)"this is a string"));
			object s;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("s", out s));
			Assert.Equal("this is a string", (string)s);

			// The only way to set and get object paths and signatures is to use
			// two param Get and Set methods
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("o", "/org/foo/bar"));
			object o;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("o", out o));
			Assert.Equal("/org/foo/bar", (string)o);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("g", "a{is}d(siiux)"));
			object g;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("g", out g));
			Assert.Equal("a{is}d(siiux)", (string)g);
		}

		[Fact]
		public void BasicArrayAssignment()
		{
			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();

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


			AllJoyn.MsgArg arg2 = new AllJoyn.MsgArg(2);
			arg2[0].Set("ab", in_bool_array);
			arg2[1].Set("ab", in_bool_array);
			out_bool_array = (bool[])arg2[0];
			Assert.Equal(in_bool_array.Length, out_bool_array.Length);
			for (int i = 0; i < out_bool_array.Length; i++)
			{
				Assert.Equal(in_bool_array[i], out_bool_array[i]);
			}
			out_bool_array = (bool[])arg2[1];
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
		public void BasicArraySetGet()
		{
			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();

			//byte
			byte[] in_byte_array = { 9, 19, 29, 39, 49 };
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("ay", in_byte_array));
			object ay;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("ay", out ay));
			byte[] out_byte_array = (byte[])ay;
			Assert.Equal(in_byte_array.Length, out_byte_array.Length);
			for (int i = 0; i < out_byte_array.Length; i++)
			{
				Assert.Equal(in_byte_array[i], out_byte_array[i]);
			}

			//bool
			bool[] in_bool_array = { false, false, true, true, false, true };
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("ab", in_bool_array));
			object ab;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("ab", out ab));
			bool[] out_bool_array = (bool[])ab;
			Assert.Equal(in_bool_array.Length, out_bool_array.Length);
			for (int i = 0; i < out_bool_array.Length; i++)
			{
				Assert.Equal(in_bool_array[i], out_bool_array[i]);
			}

			//short
			short[] in_short_array = { -9, -99, 999, 9999 };
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("an", in_short_array));
			object an;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("an", out an));
			short[] out_short_array = (short[])an;
			Assert.Equal(in_short_array.Length, out_short_array.Length);
			for (int i = 0; i < out_short_array.Length; i++)
			{
				Assert.Equal(in_short_array[i], out_short_array[i]);
			}

			//ushort
			ushort[] in_ushort_array = { 9, 99, 999, 9999 };
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("aq", in_ushort_array));
			object aq;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("aq", out aq));
			ushort[] out_ushort_array = (ushort[])aq;
			Assert.Equal(in_ushort_array.Length, out_ushort_array.Length);
			for (int i = 0; i < out_short_array.Length; i++)
			{
				Assert.Equal(in_ushort_array[i], out_ushort_array[i]);
			}

			//int
			int[] in_int_array = { -8, -88, 888, 8888 };
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("ai", in_int_array));
			object ai;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("ai", out ai));
			int[] out_int_array = (int[])ai;
			Assert.Equal(in_int_array.Length, out_int_array.Length);
			for (int i = 0; i < out_int_array.Length; i++)
			{
				Assert.Equal(in_int_array[i], out_int_array[i]);
			}

			//uint
			uint[] in_uint_array = { 8, 88, 888, 8888 };
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("au", in_uint_array));
			object au;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("au", out au));
			uint[] out_uint_array = (uint[])au;
			Assert.Equal(in_uint_array.Length, out_uint_array.Length);
			for (int i = 0; i < out_int_array.Length; i++)
			{
				Assert.Equal(in_uint_array[i], out_uint_array[i]);
			}

			//long
			long[] in_long_array = { -7, -77, 777, 7777 };
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("ax", in_long_array));
			object ax;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("ax", out ax));
			long[] out_long_array = (long[])ax;
			Assert.Equal(in_long_array.Length, out_long_array.Length);
			for (int i = 0; i < out_long_array.Length; i++)
			{
				Assert.Equal(in_long_array[i], out_long_array[i]);
			}

			//ulong
			ulong[] in_ulong_array = { 7, 77, 777, 7777 };
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("at", in_ulong_array));
			object at;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("at", out at));
			ulong[] out_ulong_array = (ulong[])at;
			Assert.Equal(in_ulong_array.Length, out_ulong_array.Length);
			for (int i = 0; i < out_long_array.Length; i++)
			{
				Assert.Equal(in_ulong_array[i], out_ulong_array[i]);
			}

			//double
			double[] in_double_array = { 0.001, 0.01, 0.1, 1.0, 10.0, 100.0 };
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("ad", in_double_array));
			object ad;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("ad", out ad));
			double[] out_double_array = (double[])ad;
			Assert.Equal(in_double_array.Length, out_double_array.Length);
			for (int i = 0; i < out_long_array.Length; i++)
			{
				Assert.Equal(in_double_array[i], out_double_array[i]);
			}

			//string
			string[] in_string_array = { "one", "two", "three", "four" };
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("as", in_string_array));
			object sa;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("as", out sa));
			string[] out_string_array = (string[])sa;
			Assert.Equal(in_string_array.Length, out_string_array.Length);
			for (int i = 0; i < out_string_array.Length; i++)
			{
				Assert.Equal(in_string_array[i], out_string_array[i]);
			}

			//object path
			string[] in_path_array = { "/org/one", "/org/two", "/org/three", "/org/four" };
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("ao", in_path_array));
			object ao;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("ao", out ao));
			string[] out_path_array = (string[])ao;
			Assert.Equal(in_path_array.Length, out_path_array.Length);
			for (int i = 0; i < out_path_array.Length; i++)
			{
				Assert.Equal(in_path_array[i], out_path_array[i]);
			}

			//signature
			string[] in_signature_array = { "s", "sss", "as", "a(iiiiuu)" };
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("ag", in_signature_array));
			object sg;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("ag", out sg));
			string[] out_signature_array = (string[])sg;
			Assert.Equal(in_signature_array.Length, out_signature_array.Length);
			for (int i = 0; i < out_signature_array.Length; i++)
			{
				Assert.Equal(in_signature_array[i], out_signature_array[i]);
			}
		}

		[Fact]
		public void Variants()
		{
			double d = 3.14159265;
			string s = "this is a string";
			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();
			AllJoyn.MsgArg variantArg = new AllJoyn.MsgArg();

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("i", 420));
			Assert.Equal(AllJoyn.QStatus.OK, variantArg.Set("v", arg));
			int i;
			long x;
			Assert.Equal(AllJoyn.QStatus.BUS_SIGNATURE_MISMATCH, variantArg.Get(out x));
			Assert.Equal(AllJoyn.QStatus.OK, variantArg.Get(out i));
			Assert.Equal(420, i);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("d", d));
			Assert.Equal(AllJoyn.QStatus.OK, variantArg.Set("v", arg));
			double d_out;
			Assert.Equal(AllJoyn.QStatus.BUS_SIGNATURE_MISMATCH, variantArg.Get(out i));
			Assert.Equal(AllJoyn.QStatus.BUS_SIGNATURE_MISMATCH, variantArg.Get(out s));
			Assert.Equal(AllJoyn.QStatus.OK, variantArg.Get(out d_out));
			Assert.Equal(d, d_out);

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("s", s));
			Assert.Equal(AllJoyn.QStatus.OK, variantArg.Set("v", arg));
			Assert.Equal(AllJoyn.QStatus.BUS_SIGNATURE_MISMATCH, variantArg.Get(out i));
			string s_out;
			Assert.Equal(AllJoyn.QStatus.OK, variantArg.Get(out s_out));
			Assert.Equal(s, s_out);
		}

		[Fact]
		public void VariantsOfArrays()
		{
			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();
			AllJoyn.MsgArg variantArg = new AllJoyn.MsgArg();

			//int
			int[] in_int_array = { -8, -88, 888, 8888 };
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("ai", in_int_array));

			Assert.Equal(AllJoyn.QStatus.OK, variantArg.Set("v", arg));

			object ai;
			Assert.Equal(AllJoyn.QStatus.OK, variantArg.Get("ai", out ai));
			int[] out_int_array = (int[])ai;
			Assert.Equal(in_int_array.Length, out_int_array.Length);
			for (int i = 0; i < out_int_array.Length; i++)
			{
				Assert.Equal(in_int_array[i], out_int_array[i]);
			}

			//signature
			string[] in_signature_array = { "s", "sss", "as", "a(iiiiuu)" };
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("ag", in_signature_array));

			Assert.Equal(AllJoyn.QStatus.OK, variantArg.Set("v", arg));

			object sg;
			Assert.Equal(AllJoyn.QStatus.OK, variantArg.Get("ag", out sg));
			string[] out_signature_array = (string[])sg;
			Assert.Equal(in_signature_array.Length, out_signature_array.Length);
			for (int i = 0; i < out_signature_array.Length; i++)
			{
				Assert.Equal(in_signature_array[i], out_signature_array[i]);
			}
		}

		[Fact]
		public void VariantsOfStructs()
		{
			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();
			AllJoyn.MsgArg variantArg = new AllJoyn.MsgArg();
			object[] in_v_struct = new object[] { (byte)24, true, (short)42, 3.14159, "Gonzo the magnificant" };
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("(ybnds)", in_v_struct));

			Assert.Equal(AllJoyn.QStatus.OK, variantArg.Set("v", arg));

			//unlike basic data types with container types we must first pull out the variant.
			object tmp;
			Assert.Equal(AllJoyn.QStatus.OK, variantArg.Get("v", out tmp));

			object out_v_struct;
			Assert.Equal(AllJoyn.QStatus.OK, ((AllJoyn.MsgArg)tmp).Get("(ybnds)", out out_v_struct));
			object[] ovs = (object[])out_v_struct;
			Assert.Equal(5, ovs.Length);
			Assert.Equal((byte)24, (byte)ovs[0]);
			Assert.True((bool)ovs[1]);
			Assert.Equal((short)42, (short)ovs[2]);
			Assert.Equal(3.14159, (double)ovs[3]);
			Assert.Equal("Gonzo the magnificant" , (string)ovs[4]);
		}

		[Fact]
		public void VariantOfDictionaries()
		{
			System.Collections.Generic.Dictionary<object, object> dict = new System.Collections.Generic.Dictionary<object, object>();
			dict.Add("January", "Frezzing");
			dict.Add("Febuary", "Cold");
			dict.Add("March", "Still Cold");
			dict.Add("April", "Why is it still Cold?");

			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();
			AllJoyn.MsgArg variantArg = new AllJoyn.MsgArg();

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("a{ss}", dict));
			Assert.Equal(AllJoyn.QStatus.OK, variantArg.Set("v", arg));

			//unlike basic data types with container types we must first pull out the variant.
			object tmp;
			Assert.Equal(AllJoyn.QStatus.OK, variantArg.Get("v", out tmp));

			object out_dict;
			Assert.Equal(AllJoyn.QStatus.OK, ((AllJoyn.MsgArg)tmp).Get("a{ss}", out out_dict));
			Assert.Equal(dict, (System.Collections.Generic.Dictionary<object, object>)out_dict);
			System.Collections.Generic.Dictionary<object, object> d = (System.Collections.Generic.Dictionary<object, object>)out_dict;
			Assert.Equal("Frezzing", (string)d["January"]);
			Assert.Equal("Cold", (string)d["Febuary"]);
			Assert.Equal("Still Cold", (string)d["March"]);
			Assert.Equal("Why is it still Cold?", (string)d["April"]);
		}

		[Fact]
		public void VariantOfVariants()
		{
			string s = "this is a string";
			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();
			AllJoyn.MsgArg variantArg1 = new AllJoyn.MsgArg();
			AllJoyn.MsgArg variantArg2 = new AllJoyn.MsgArg();

			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("s", s));
			Assert.Equal(AllJoyn.QStatus.OK, variantArg1.Set("v", arg));
			Assert.Equal(AllJoyn.QStatus.OK, variantArg2.Set("v", variantArg1));

			string s_out;
			Assert.Equal(AllJoyn.QStatus.OK, variantArg2.Get(out s_out));
			Assert.Equal(s, s_out);
			arg.Dispose();
			variantArg1.Dispose();
			variantArg2.Dispose();

			arg = new AllJoyn.MsgArg();
			variantArg1 = new AllJoyn.MsgArg();
			variantArg2 = new AllJoyn.MsgArg();
			//with struct container (See VariantOfStructs test)
			object[] in_v_struct = new object[] { (byte)24, true, (short)42, 3.14159, "Gonzo the magnificant" };
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("(ybnds)", in_v_struct));

			Assert.Equal(AllJoyn.QStatus.OK, variantArg1.Set("v", arg));
			Assert.Equal(AllJoyn.QStatus.OK, variantArg2.Set("v", variantArg1));

			//nest variants do not have to be unpacked multiple times even when they have a container type
			object tmp;
			Assert.Equal(AllJoyn.QStatus.OK, variantArg2.Get("v", out tmp));
			object tmp2;
			Assert.Equal(AllJoyn.QStatus.BUS_SIGNATURE_MISMATCH, ((AllJoyn.MsgArg)tmp).Get("v", out tmp2));
			object out_v_struct;
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, variantArg2.Get("(ybnds)", out out_v_struct));
			Assert.Equal(AllJoyn.QStatus.OK, ((AllJoyn.MsgArg)tmp).Get("(ybnds)", out out_v_struct));
			object[] ovs = (object[])out_v_struct;
			Assert.Equal(5, ovs.Length);
			Assert.Equal((byte)24, (byte)ovs[0]);
			Assert.True((bool)ovs[1]);
			Assert.Equal((short)42, (short)ovs[2]);
			Assert.Equal(3.14159, (double)ovs[3]);
			Assert.Equal("Gonzo the magnificant", (string)ovs[4]);
		}


		[Fact]
		public void ArraysOfArrays()
		{
			int[][] int_array2D2 = new int[][]
			{
				new int[] { 1, 2, 3 },
				new int[] { 4, 5, 6 }
			};

			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("aai", int_array2D2));

			object out_array;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("aai", out out_array));
			
			object[] i_a1 =(object[])out_array; 
			int i_size1 = i_a1.Length;
			int[][] result = new int[i_size1][];
			for(int j = 0; j < i_size1; j++) {
				result[j] = (int[])i_a1[j];
			}

			Assert.Equal(1, result[0][0]);
			Assert.Equal(2, result[0][1]);
			Assert.Equal(3, result[0][2]);
			Assert.Equal(4, result[1][0]);
			Assert.Equal(5, result[1][1]);
			Assert.Equal(6, result[1][2]);

			string[][][] str_array = new string[][][]
			{
				new string[][]
				{
					new string[] {"one", "two", "three"},
					new string[] {"red", "yellow", "blue"}
				},
				new string[][]
				{
					new string[] {"dog", "cat", "fish", "bird"},
					new string[] {"high", "low"},
					new string[] {"strong", "weak", "indifferent", "lazy"}
				}
			};
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("aaas", str_array));

			object out_str_array;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("aai", out out_str_array));

			object[] s_a1 = (object[])out_str_array;
			string[][][] str_result = new string[s_a1.Length][][];
			for (int j = 0; j < s_a1.Length; ++j)
			{
				object[] s_a2 = (object[])s_a1[j];
				str_result[j] = new string[s_a2.Length][];
				for (int k = 0; k < s_a2.Length; ++k)
				{
					str_result[j][k] = (string[])s_a2[k];
				}
			}
			
			for (int x = 0; x < str_result.Length; ++x)
			{
				for (int y = 0; y < str_result[x].Length; ++y)
				{
					for (int z = 0; z < str_result[x][y].Length; ++z)
					{
						Assert.Equal(str_array[x][y][z], str_result[x][y][z]);
					}
				}
			}
		}

		[Fact]
		public void Dictionaries()
		{
			System.Collections.Generic.Dictionary<object, object> dict = new System.Collections.Generic.Dictionary<object, object>();
			dict.Add("apple", 2);
			dict.Add("pear", 1);
			dict.Add("bannana", 0);
			dict.Add("kiwi", -1);

			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();
			AllJoyn.QStatus status = arg.Set("a{si}", dict);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			object out_dict;
			status = arg.Get("a{si}", out out_dict);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.Equal(dict, (System.Collections.Generic.Dictionary<object, object>)out_dict);
		}

		[Fact]
		public void DictionaryOfArrays()
		{
			int[] in_int_array1 = { -2, -22, 222, 2222 };
			int[] in_int_array2 = { -8, -88, 888, 8888 };
			int[] in_int_array3 = { -4, -42, 424, 4242 };
			int[] in_int_array4 = { -5, -55, 555, 5555 };

			System.Collections.Generic.Dictionary<object, object> dict = new System.Collections.Generic.Dictionary<object, object>();
			dict.Add("one", in_int_array1);
			dict.Add("two", in_int_array2);
			dict.Add("three", in_int_array3);
			dict.Add("four", in_int_array4);

			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();
			Assert.Equal(AllJoyn.QStatus.OK,arg.Set("a{sai}", dict));

			object out_dict;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("a{sai}", out out_dict));

			System.Collections.Generic.Dictionary<object, object> d = (System.Collections.Generic.Dictionary<object, object>)out_dict;
			Assert.Equal(in_int_array1, (int[])d["one"]);
			Assert.Equal(in_int_array2, (int[])d["two"]);
			Assert.Equal(in_int_array3, (int[])d["three"]);
			Assert.Equal(in_int_array4, (int[])d["four"]);
		}

		[Fact]
		public void DictionaryOfStructs()
		{
			object[] in_struct1 = { -2, "say", 1.618, 888u }; //(isdu)
			object[] in_struct2 = { -42, "what", 2.718, 42u }; //(isdu)
			object[] in_struct3 = { 88, "you", 3.14, 10u }; //(isdu)
			object[] in_struct4 = { 1999, "want", 1.202, 33u }; //(isdu)

			System.Collections.Generic.Dictionary<object, object> dict = new System.Collections.Generic.Dictionary<object, object>();
			dict.Add("one", in_struct1);
			dict.Add("two", in_struct2);
			dict.Add("three", in_struct3);
			dict.Add("four", in_struct4);

			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("a{s(isdu)}", dict));

			object out_dict;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("a{s(isdu)}", out out_dict));

			System.Collections.Generic.Dictionary<object, object> d = (System.Collections.Generic.Dictionary<object, object>)out_dict;
			Assert.Equal(in_struct1, (object[])d["one"]);
			Assert.Equal(in_struct2, (object[])d["two"]);
			Assert.Equal(in_struct3, (object[])d["three"]);
			Assert.Equal(in_struct4, (object[])d["four"]);
		}

		[Fact]
		public void DitionaryOfVariants()
		{
			AllJoyn.MsgArg a_arg = new AllJoyn.MsgArg(4);
			int[] in_int_array = { -2, -22, 222, 2222 };
			Assert.Equal(AllJoyn.QStatus.OK, a_arg[0].Set("ai", in_int_array));
			Assert.Equal(AllJoyn.QStatus.OK, a_arg[1].Set("i", 42));
			Assert.Equal(AllJoyn.QStatus.OK, a_arg[2].Set("d", 3.14));
			Assert.Equal(AllJoyn.QStatus.OK, a_arg[3].Set("s", "How are you?"));

			System.Collections.Generic.Dictionary<object, object> dict = new System.Collections.Generic.Dictionary<object, object>();
			dict.Add(10u, a_arg[0]);
			dict.Add(20u, a_arg[1]);
			dict.Add(30u, a_arg[2]);
			dict.Add(42u, a_arg[3]);

			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("a{uv}", dict));

			object out_dict;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("a{uv}", out out_dict));

			System.Collections.Generic.Dictionary<object, object> d = (System.Collections.Generic.Dictionary<object, object>)out_dict;
			object int_array;
			object i;
			object dub;
			object s;
			Assert.Equal(AllJoyn.QStatus.OK, ((AllJoyn.MsgArg)(d[10u])).Get("ai", out int_array));
			Assert.Equal(AllJoyn.QStatus.OK, ((AllJoyn.MsgArg)(d[20u])).Get("i", out i));
			Assert.Equal(AllJoyn.QStatus.OK, ((AllJoyn.MsgArg)(d[30u])).Get("d", out dub));
			Assert.Equal(AllJoyn.QStatus.OK, ((AllJoyn.MsgArg)(d[42u])).Get("s", out s));

			Assert.Equal(in_int_array, (int[])int_array);
			Assert.Equal(42, (int)i);
			Assert.Equal(3.14, (double)dub);
			Assert.Equal("How are you?", (string)s);
		}

		[Fact]
		public void DictionaryOfDictionaries()
		{
			System.Collections.Generic.Dictionary<object, object> inner_dict1 = new System.Collections.Generic.Dictionary<object, object>();
			inner_dict1.Add("apple", 2);
			inner_dict1.Add("pear", 1);
			System.Collections.Generic.Dictionary<object, object> inner_dict2 = new System.Collections.Generic.Dictionary<object, object>();
			inner_dict2.Add("bannana", 0);
			inner_dict2.Add("kiwi", -1);

			System.Collections.Generic.Dictionary<object, object> dict = new System.Collections.Generic.Dictionary<object, object>();
			dict.Add("one", inner_dict1);
			dict.Add("two", inner_dict2);
			dict.Add("three", inner_dict2);
			dict.Add("four", inner_dict1);

			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("a{sa{si}}", dict));

			object out_dict;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("a{sa{si}}", out out_dict));

			System.Collections.Generic.Dictionary<object, object> d = (System.Collections.Generic.Dictionary<object, object>)out_dict;
			Assert.Equal(2, (int)((System.Collections.Generic.Dictionary<object, object>)d["one"])["apple"]);
			Assert.Equal(1, (int)((System.Collections.Generic.Dictionary<object, object>)d["one"])["pear"]);

			Assert.Equal(0, (int)((System.Collections.Generic.Dictionary<object, object>)d["two"])["bannana"]);
			Assert.Equal(-1, (int)((System.Collections.Generic.Dictionary<object, object>)d["two"])["kiwi"]);

			Assert.Equal(0, (int)((System.Collections.Generic.Dictionary<object, object>)d["three"])["bannana"]);
			Assert.Equal(-1, (int)((System.Collections.Generic.Dictionary<object, object>)d["three"])["kiwi"]);

			Assert.Equal(2, (int)((System.Collections.Generic.Dictionary<object, object>)d["four"])["apple"]);
			Assert.Equal(1, (int)((System.Collections.Generic.Dictionary<object, object>)d["four"])["pear"]);
		}

		public struct TestStruct //(issi)
		{
			public int a;
			public string b;
			public string c;
			public int d;
		}

		[Fact]
		public void AllJoynStructs()
		{
			TestStruct testStruct = new TestStruct();
			testStruct.a = 42;
			testStruct.b = "Hello";
			testStruct.c = "World";
			testStruct.d = 88;

			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();
			object[] mystruct = new object[4];
			mystruct[0] = testStruct.a;
			mystruct[1] = testStruct.b;
			mystruct[2] = testStruct.c;
			mystruct[3] = testStruct.d;
			AllJoyn.QStatus status = arg.Set("(issi)", mystruct);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			object outstruct;
			status = arg.Get("(issi)", out outstruct);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			object[] outstructa = (object[])outstruct;
			Assert.Equal(4, outstructa.Length);
			Assert.Equal(testStruct.a, (int)outstructa[0]);
			Assert.Equal(testStruct.b, (string)outstructa[1]);
			Assert.Equal(testStruct.c, (string)outstructa[2]);
			Assert.Equal(testStruct.d, (int)outstructa[3]);
			arg.Dispose();
			arg = new AllJoyn.MsgArg();

			//struct within a struct
			object[] mystruct2 = new object[2];
			mystruct2[0] = "bob";
			mystruct2[1] = mystruct;
			status = arg.Set("(s(issi))", mystruct2);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			status = arg.Get("(s(issi))", out outstruct);
			object[] outstruct1 = (object[])outstruct;
			Assert.Equal(2, outstruct1.Length);
			object[] outstruct2 = (object[])outstruct1[1];
			Assert.Equal(4, outstruct2.Length);

			Assert.Equal("bob", (string)outstruct1[0]);
			Assert.Equal(testStruct.a, (int)outstruct2[0]);
			Assert.Equal(testStruct.b, (string)outstruct2[1]);
			Assert.Equal(testStruct.c, (string)outstruct2[2]);
			Assert.Equal(testStruct.d, (int)outstruct2[3]);
		}

		[Fact]
		public void StructOfArrays()
		{
			string[] in_s_array = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" }; //as
			int[] in_i_array = { -41, 0, 5463, 1244 }; //ai
			object[] in_struct_array = { in_s_array, in_i_array }; //(asai)

			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("(asai)", in_struct_array));

			object out_struct_tmp;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("(asai)", out out_struct_tmp));
			object[] out_struct_array = (object[])out_struct_tmp;
			Assert.Equal(in_struct_array, out_struct_array);
		}

		[Fact]
		public void StructOfVariants()
		{
			AllJoyn.MsgArg arg = new AllJoyn.MsgArg(2);
			AllJoyn.MsgArg variant = new AllJoyn.MsgArg(2);

			Assert.Equal(AllJoyn.QStatus.OK, arg[0].Set("b", true));
			Assert.Equal(AllJoyn.QStatus.OK, arg[1].Set("s", "Hello World"));

			Assert.Equal(AllJoyn.QStatus.OK, variant[0].Set("v", arg[0]));
			Assert.Equal(AllJoyn.QStatus.OK, variant[1].Set("v", arg[1]));
		}

		[Fact]
		public void StructOfDictionaries()
		{
			System.Collections.Generic.Dictionary<object, object> dict = new System.Collections.Generic.Dictionary<object, object>();
			dict.Add("apple", 2);
			dict.Add("pear", 1);
			dict.Add("bannana", 0);
			dict.Add("kiwi", -1);

			object[] in_struct_dict = { dict, dict };

			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("(a{si}a{si})", in_struct_dict));

			object out_struct_dict_tmp;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("(a{si}a{si})", out out_struct_dict_tmp));
			object[] out_struct_dict = (object[]) out_struct_dict_tmp;
			Assert.Equal(dict, (System.Collections.Generic.Dictionary<object, object>)out_struct_dict[0]);
			Assert.Equal(dict, (System.Collections.Generic.Dictionary<object, object>)out_struct_dict[1]);
		}

		[Fact]
		public void ArrayOfStruct()
		{
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;
			object[][] sii = new object[][]
							{
								new object[] {1, 2}, 
								new object[] {11, 22}, 
								new object[] {111, 222}
							};

			//signature a(ii);
			AllJoyn.MsgArg structs = new AllJoyn.MsgArg(3);
			status = structs[0].Set("(ii)", sii[0]);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			status = structs[1].Set("(ii)", sii[1]);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			status = structs[2].Set("(ii)", sii[2]);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			Assert.Equal("(ii)", structs[0].Signature);

			AllJoyn.MsgArg struct_array = new AllJoyn.MsgArg();
			status = struct_array.Set("a(ii)", structs);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.Equal("a(ii)", struct_array.Signature);

			object out_struct_array;
			status = struct_array.Get("a(ii)", out out_struct_array);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			AllJoyn.MsgArg ret_value = (AllJoyn.MsgArg)out_struct_array;
			Assert.Equal(3, ret_value.Length);
			object rii;
			status = ret_value[0].Get("(ii)", out rii);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.Equal(2, ((object[])rii).Length);
			Assert.Equal(1, (int)((object[])rii)[0]);
			Assert.Equal(2, (int)((object[])rii)[1]);
			status = ret_value[1].Get("(ii)", out rii);
			Assert.Equal(2, ((object[])rii).Length);
			Assert.Equal(11, (int)((object[])rii)[0]);
			Assert.Equal(22, (int)((object[])rii)[1]);
			status = ret_value[2].Get("(ii)", out rii);
			Assert.Equal(2, ((object[])rii).Length);
			Assert.Equal(111, (int)((object[])rii)[0]);
			Assert.Equal(222, (int)((object[])rii)[1]);

		}

		[Fact]
		public void ArraysOfVariants()
		{
			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;
			AllJoyn.MsgArg tmp = new AllJoyn.MsgArg(5);
			AllJoyn.MsgArg args = new AllJoyn.MsgArg(5);
			tmp[0] = "blue";
			Assert.Equal(AllJoyn.QStatus.OK, args[0].Set("v", tmp[0]));
			tmp[1] = 24u;
			Assert.Equal(AllJoyn.QStatus.OK, args[1].Set("v", tmp[1]));
			tmp[2] = 88;
			Assert.Equal(AllJoyn.QStatus.OK, args[2].Set("v", tmp[2]));
			tmp[3] = true;
			Assert.Equal(AllJoyn.QStatus.OK, args[3].Set("v", tmp[3]));

			System.Collections.Generic.Dictionary<object, object> dict = new System.Collections.Generic.Dictionary<object, object>();
			dict.Add("apple", 2);
			dict.Add("pear", 1);
			dict.Add("bannana", 0);
			dict.Add("kiwi", -1);

			status = tmp[4].Set("a{si}", dict);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.Equal(AllJoyn.QStatus.OK, args[4].Set("v", tmp[4]));

			AllJoyn.MsgArg v_arg = new AllJoyn.MsgArg();
			status = v_arg.Set("av", args);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			object tmp_args_out;
			status = v_arg.Get("av", out tmp_args_out);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			AllJoyn.MsgArg args_out = (AllJoyn.MsgArg)tmp_args_out;
			Assert.Equal(5, args_out.Length);
			string s_tmp;
			Assert.Equal(AllJoyn.QStatus.OK, args_out[0].Get(out s_tmp));
			Assert.Equal("blue", s_tmp);
			uint u_tmp;
			Assert.Equal(AllJoyn.QStatus.OK, args_out[1].Get(out u_tmp));
			Assert.Equal(24u, u_tmp);
			int i_tmp;
			Assert.Equal(AllJoyn.QStatus.OK, args_out[2].Get(out i_tmp));
			Assert.Equal(88, i_tmp);
			bool b_tmp;
			Assert.Equal(AllJoyn.QStatus.OK, args_out[3].Get(out b_tmp));
			Assert.True(b_tmp);

			object out_dict_v;
			Assert.Equal("v", args_out[4].Signature);
			status = args_out[4].Get("v", out out_dict_v);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.Equal("a{si}", ((AllJoyn.MsgArg)out_dict_v).Signature);
			object out_dict;
			status = ((AllJoyn.MsgArg)out_dict_v).Get("a{si}", out out_dict);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			Assert.Equal(dict, (System.Collections.Generic.Dictionary<object, object>)out_dict);
		}

		[Fact]
		public void SetMulipleValues()
		{
			AllJoyn.MsgArg arg = new AllJoyn.MsgArg(11);
			object[] input = {
								(ulong)42,  //t
								1,          //i
								3.14,       //d
								false,      //b
								"whisp",    //s
								(byte)0xAB, //b
								new object[] //(n(no)ai)
								{
									(short)2, //n
									new object[] //(no)
									{
										(short)3, //n
										"/org/alljoyn/test/MsgArg" //o
									},
									new int[] {-8, -88, 888, 8888} //ai
								},
								"signatu", //g
								new AllJoyn.MsgArg("i", 42), //v
								new string[] {"I", "met", "an", "interesting", "lady"}, //as
								1.618 //d
							 };
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("tidbsy(n(no)ai)gvasd", input));
			object output;
			Assert.Equal(AllJoyn.QStatus.OK, arg.Get("tidbsy(n(no)ai)gvasd", out output));
			Assert.Equal(input[0], ((object[])output)[0]);
			Assert.Equal(input[1], ((object[])output)[1]);
			Assert.Equal(input[2], ((object[])output)[2]);
			Assert.Equal(input[3], ((object[])output)[3]);
			Assert.Equal(input[4], ((object[])output)[4]);
			Assert.Equal(input[5], ((object[])output)[5]);
			Assert.Equal(((object[])input[6])[0], ((object[])((object[])output)[6])[0]);
			Assert.Equal(((object[])((object[])input[6])[1])[0], ((object[])((object[])((object[])output)[6])[1])[0]);
			Assert.Equal(((object[])((object[])input[6])[1])[1], ((object[])((object[])((object[])output)[6])[1])[1]);
			Assert.Equal(((object[])input[6])[2], ((object[])((object[])output)[6])[2]);
			Assert.Equal(input[7], ((object[])output)[7]);
			Assert.Equal(input[8], ((object[])output)[8]);
			Assert.Equal(input[9], ((object[])output)[9]);
			Assert.Equal(input[10], ((object[])output)[10]);

			AllJoyn.MsgArg arg2 = new AllJoyn.MsgArg(3);
			//more arguments passed in than we have room for
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg2.Set("sssss", new object[] { "I", "have", "lost", "my", "mind" }));
			//number of objects and signature don't match
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg2.Set("ss", new object[] { "I", "have", "lost", "my", "mind" }));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg2.Set("sss", new object[] { "hello", "world" }));
			//data type does not match
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg2.Set("ss", new object[] { 1, 2 }));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg2.Set("sss", new object[] { 1, 2, 3 }));

			//should set the first two elements of the MsgArg
			Assert.Equal(AllJoyn.QStatus.OK, arg2.Set("ss", new object[] { "hello", "world"} ));

			Assert.Equal(AllJoyn.QStatus.OK, arg2.Get("ss", out output));
			Assert.Equal("hello", (string)((object[])output)[0]);
			Assert.Equal("world", (string)((object[])output)[1]);
			//the element in arg2 should still be INVALID
			Assert.Equal(AllJoyn.MsgArg.AllJoynTypeId.ALLJOYN_INVALID, arg2[2].TypeId);


		}
		[Fact]
		public void InvalidAssignment()
		{
			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();
			//byte or uint8_t
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("y", true));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("y", (short)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("y", (ushort)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("y", (int)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("y", (uint)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("y", (long)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("y", (ulong)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("y", 3.14));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("y", "Error Gold"));

			//bool
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("b", (byte)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("b", (short)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("b", (ushort)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("b", (int)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("b", (uint)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("b", (long)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("b", (ulong)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("b", 3.14));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("b", "Error Gold"));

			//short or int16_t
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("n", (byte)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("n", true));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("n", (ushort)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("n", (int)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("n", (uint)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("n", (long)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("n", (ulong)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("n", 3.14));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("n", "Error Gold"));

			//ushort or uint16_t
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("q", (byte)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("q", true));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("q", (short)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("q", (int)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("q", (uint)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("q", (long)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("q", (ulong)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("q", 3.14));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("q", "Error Gold"));

			//int or int32_t
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("i", (byte)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("i", true));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("i", (short)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("i", (ushort)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("i", (uint)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("i", (long)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("i", (ulong)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("i", 3.14));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("i", "Error Gold"));

			//uint or uint32_t
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("u", (byte)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("u", true));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("u", (short)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("u", (ushort)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("u", (int)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("u", (long)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("u", (ulong)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("u", 3.14));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("u", "Error Gold"));

			//long or int64_t
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("x", (byte)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("x", true));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("x", (short)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("x", (ushort)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("x", (int)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("x", (uint)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("x", (ulong)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("x", 3.14));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("x", "Error Gold"));

			//ulong or uint64_t
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("t", (byte)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("t", true));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("t", (short)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("t", (ushort)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("t", (int)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("t", (uint)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("t", (long)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("t", 3.14));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("t", "Error Gold"));

			//double
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("d", (byte)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("d", true));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("d", (short)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("d", (ushort)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("d", (int)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("d", (uint)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("d", (long)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("d", (ulong)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("d", "Error Gold"));

			//string
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("s", (byte)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("s", true));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("s", (short)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("s", (ushort)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("s", (int)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("s", (uint)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("s", (long)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("s", (ulong)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("s", 3.14));

			//object path
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("o", (byte)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("o", true));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("o", (short)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("o", (ushort)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("o", (int)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("o", (uint)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("o", (long)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("o", (ulong)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("o", 3.14));
			//TODO figure out why object path is not doing error checking.
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("o", "Error Gold"));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("o", "ybnqiuxtdsog"));
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("o", "/org/alljoyn/test"));
			Assert.Equal(AllJoyn.QStatus.OK, arg.Set("o", "/"));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("o", "/org/alljoyn/test/"));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("o", "/org/alljoyn//test"));

			//signature
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("g", (byte)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("g", true));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("g", (short)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("g", (ushort)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("g", (int)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("g", (uint)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("g", (long)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("g", (ulong)42));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("g", 3.14));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("g", "Error Gold"));
			Assert.Equal(AllJoyn.QStatus.BUS_BAD_SIGNATURE, arg.Set("g", "/org/alljoyn/test"));
		}

		[Fact]
		public void InitilizedConstructor()
		{
			AllJoyn.MsgArg arg = new AllJoyn.MsgArg("y", (byte)13);
			Assert.Equal((byte)13, (byte)arg);

			arg = new AllJoyn.MsgArg("b", true);
			Assert.True((bool)arg);

			arg = new AllJoyn.MsgArg("n", (short)42);
			Assert.Equal((short)42, (short)arg);

			arg = new AllJoyn.MsgArg("q", (ushort)0xBEBE);
			Assert.Equal((ushort)0xBEBE, (ushort)arg);

			arg = new AllJoyn.MsgArg("i", (int)-9999);
			Assert.Equal((int)-9999, (int)arg);

			arg = new AllJoyn.MsgArg("u", (uint)0x32323232);
			Assert.Equal((uint)0x32323232, (uint)arg);

			arg = new AllJoyn.MsgArg("x", (long)-1);
			Assert.Equal((long)-1, (long)arg);

			arg = new AllJoyn.MsgArg("t", (ulong)0x6464646464646464);
			Assert.Equal((ulong)0x6464646464646464, (ulong)arg);

			arg = new AllJoyn.MsgArg("d", (double)3.14159265D);
			Assert.Equal((double)3.14159265D, (double)arg);

			arg = new AllJoyn.MsgArg("s", (string)"this is a string");
			Assert.Equal("this is a string", (string)arg);

			//error case
			arg = new AllJoyn.MsgArg("s", 42);
			Assert.Equal(AllJoyn.MsgArg.AllJoynTypeId.ALLJOYN_INVALID, arg.TypeId);
		}

		[Fact]
		public void SplitSignature()
		{
			string[] a = AllJoyn.MsgArg.splitSignature("issi");
			Assert.Equal(4, a.Length);
			Assert.Equal(a[0], "i");
			Assert.Equal(a[1], "s");
			Assert.Equal(a[2], "s");
			Assert.Equal(a[3], "i");

			a = AllJoyn.MsgArg.splitSignature("aas(issi)a{is}(i)(i(suasi(issi)(a{sv})))");
			Assert.Equal(5, a.Length);
			Assert.Equal(a[0], "aas");
			Assert.Equal(a[1], "(issi)");
			Assert.Equal(a[2], "a{is}");
			Assert.Equal(a[3], "(i)");
			Assert.Equal(a[4], "(i(suasi(issi)(a{sv})))");

			//unbalanced containers
			a = AllJoyn.MsgArg.splitSignature("(ai(ss)");
			Assert.Null(a);
			a = AllJoyn.MsgArg.splitSignature("a{si");
			Assert.Null(a);
		}

		[Fact]
		public void MsgArgToString()
		{
			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();
			arg = "Asign a string";
#if DEBUG
			Assert.Equal("<string>Asign a string</string>", arg.ToString());
			Assert.Equal("  <string>Asign a string</string>", arg.ToString(2));
#else
			Assert.Equal("", arg.ToString());
			Assert.Equal("", arg.ToString(2));
#endif

			AllJoyn.QStatus status = AllJoyn.QStatus.FAIL;
			object[][] sii = new object[][]
							{
								new object[] {1, 2},
								new object[] {11, 22},
								new object[] {111, 222}
							};

			//signature a(ii);
			AllJoyn.MsgArg structs = new AllJoyn.MsgArg(3);
			status = structs[0].Set("(ii)", sii[0]);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			status = structs[1].Set("(ii)", sii[1]);
			Assert.Equal(AllJoyn.QStatus.OK, status);
			status = structs[2].Set("(ii)", sii[2]);
			Assert.Equal(AllJoyn.QStatus.OK, status);

			Assert.Equal("(ii)", structs[0].Signature);

			AllJoyn.MsgArg struct_array = new AllJoyn.MsgArg();
			status = struct_array.Set("a(ii)", structs);
			Assert.Equal(AllJoyn.QStatus.OK, status);
#if DEBUG
			string expected = "<array type_sig=\"(ii)\">\n" +
							"  <struct>\n" +
							"    <int32>1</int32>\n" +
							"    <int32>2</int32>\n" +
							"  </struct>\n" +
							"  <struct>\n" +
							"    <int32>11</int32>\n" +
							"    <int32>22</int32>\n" +
							"  </struct>\n" +
							"  <struct>\n" +
							"    <int32>111</int32>\n" +
							"    <int32>222</int32>\n" +
							"  </struct>\n" +
							"</array>";
			Assert.Equal(expected, struct_array.ToString());
#else
			Assert.Equal("", struct_array.ToString());
#endif

			AllJoyn.MsgArg arg2 = new AllJoyn.MsgArg(2);
			arg2.Set("ii", new object[] { 42, 24 });
#if DEBUG
			Assert.Equal("<int32>42</int32>\n<int32>24</int32>", arg2.ToString());
#else
			Assert.Equal("", arg2.ToString());
#endif
		}

		[Fact]
		public void HasString()
		{
			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();
			arg = "Yo";
			Assert.True(arg.HasSignature("s"));
			Assert.False(arg.HasSignature("i"));

			arg = (byte)42;
			Assert.True(arg.HasSignature("y"));
			Assert.False(arg.HasSignature("o"));
		}

		[Fact]
		public void MsgArgEquals()
		{
			AllJoyn.MsgArg arg1 = new AllJoyn.MsgArg();
			AllJoyn.MsgArg arg2 = new AllJoyn.MsgArg();
			AllJoyn.MsgArg arg3 = new AllJoyn.MsgArg();
			arg1 = "Yo";
			arg2 = "Yo";
			arg3 = 42;
			object arg4;

			Assert.True(arg1 == arg2);
			Assert.False(arg1 != arg2);
			Assert.True(arg1 != arg3);
			Assert.False(arg1 == arg3);

			Assert.True(arg1.Equals(arg2));
			Assert.False(arg1.Equals(arg3));

			Assert.False(arg1.Equals((object)null));
			arg4 = 42; //not a MsgArg;
			Assert.False(arg1.Equals(arg4));
			arg4 = arg2;
			Assert.True(arg1.Equals(arg4));

			Assert.Equal(arg1.GetHashCode(), arg2.GetHashCode());
			Assert.NotEqual(arg1.GetHashCode(), arg3.GetHashCode());
		}

		[Fact]
		public void TypeId()
		{
			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();
			arg = "Bat Man";
			Assert.Equal(AllJoyn.MsgArg.AllJoynTypeId.ALLJOYN_STRING, arg.TypeId);

			arg = 42;
			Assert.Equal(AllJoyn.MsgArg.AllJoynTypeId.ALLJOYN_INT32, arg.TypeId);
		}

		[Fact]
		public void Clear()
		{
			AllJoyn.MsgArg arg = new AllJoyn.MsgArg();
			arg = "Bat Man";
			Assert.Equal(AllJoyn.MsgArg.AllJoynTypeId.ALLJOYN_STRING, arg.TypeId);
			arg.Clear();
			Assert.Equal(AllJoyn.MsgArg.AllJoynTypeId.ALLJOYN_INVALID, arg.TypeId);
		}

	}
}