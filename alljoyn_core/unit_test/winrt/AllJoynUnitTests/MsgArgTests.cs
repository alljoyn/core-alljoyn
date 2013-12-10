//-----------------------------------------------------------------------
// <copyright file="MsgArgTests.cs" company="AllSeen Alliance.">
//     Copyright (c) 2012, AllSeen Alliance. All rights reserved.
//
//        Permission to use, copy, modify, and/or distribute this software for any
//        purpose with or without fee is hereby granted, provided that the above
//        copyright notice and this permission notice appear in all copies.
//
//        THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//        WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//        MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//        ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//        WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//        ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//        OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
// </copyright>
//-----------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.VisualStudio.TestPlatform.UnitTestFramework;
using AllJoyn;

namespace AllJoynUnitTests
{
    [TestClass]
    public class MsgArgTests
    {
        struct SimpleStruct1
        {
            public Int32 i;
            public string s;
            public bool b;
            public byte y;
            public SimpleStruct1(Int32 i1, string s1, bool b1, byte y1) { i = i1; s = s1; b = b1; y = y1; }
            public bool Equals(SimpleStruct1 other) { return (i == other.i) && (s == other.s) && (b == other.b) && (y == other.y); }
        }
        struct SimpleStruct2
        {
            public double d;
            public Int64 x;
            public UInt64 t;
            public Int16 n;
            public SimpleStruct2(double i1, Int64 s1, UInt64 b1, Int16 y1) { d = i1; x = s1; t = b1; n = y1; }
            public bool Equals(SimpleStruct2 other) { return (d == other.d) && (x == other.x) && (t == other.t) && (n == other.n); }
        }
        struct SimpleStruct3
        {
            public UInt16 q;
            public bool b;
            public UInt32 u;
            public double d;
            public SimpleStruct3(UInt16 i1, bool s1, UInt32 b1, double y1) { q = i1; b = s1; u = b1; d = y1; }
            public bool Equals(SimpleStruct3 other) { return (q == other.q) && (b == other.b) && (u == other.u) && (d == other.d); }
        }
        struct TestStruct
        {
            public Int32 i;
            public string s;
        }

        [TestMethod]
        public void PackAndUnpackingSimpleMsgArgs()
        {
            bool b1 = true;
            bool b2 = false;
            byte y1 = 0;
            byte y2 = 255;
            Int16 n1 = Int16.MaxValue;
            Int16 n2 = Int16.MinValue;
            UInt16 q1 = UInt16.MaxValue;
            UInt16 q2 = UInt16.MinValue;
            Int32 i1 = Int32.MaxValue;
            Int32 i2 = Int32.MinValue;
            UInt32 u1 = UInt32.MaxValue;
            UInt32 u2 = UInt32.MinValue;
            Int64 x1 = Int64.MaxValue;
            Int64 x2 = Int64.MinValue;
            UInt64 t1 = UInt64.MaxValue;
            UInt64 t2 = UInt64.MinValue;
            double d1 = double.MaxValue;
            double d2 = double.MinValue;
            string s1 = "string one";
            string s2 = "string two";
            bool[] bArray = new bool[] { true, true, false, true, false };
            byte[] yArray = new byte[] { 1, 7, 13, 27, 43, 111, 137, 231 };
            Int16[] nArray = new Int16[] { -231, -137, -111, -43, -27, -13, -7, -1, 1, 7, 13, 27, 43, 111, 137, 231 };
            UInt16[] qArray = new UInt16[] { 1, 7, 13, 27, 43, 111, 137, 231 };
            Int32[] iArray = new Int32[] { -231, -137, -111, -43, -27, -13, -7, -1, 1, 7, 13, 27, 43, 111, 137, 231 };
            UInt32[] uArray = new UInt32[] { 1, 7, 13, 27, 43, 111, 137, 231 };
            Int64[] xArray = new Int64[] { -231, -137, -111, -43, -27, -13, -7, -1, 1, 7, 13, 27, 43, 111, 137, 231 };
            UInt64[] tArray = new UInt64[] { 1, 7, 13, 27, 43, 111, 137, 231 };
            double[] dArray = new double[] { -1235.3476, -5.456366, 0.0, 4.56566, 12344.2345 };
            string[] sArray = new string[] { "this", "is", "a", "string", "array" };

            MsgArg bArg1 = new MsgArg("b", new object[] { b1 });
            MsgArg bArg2 = new MsgArg("b", new object[] { b2 });
            Assert.AreEqual(b1, (bool)bArg1.Value);
            Assert.AreEqual(b2, (bool)bArg2.Value);

            MsgArg yArg1 = new MsgArg("y", new object[] { y1 });
            MsgArg yArg2 = new MsgArg("y", new object[] { y2 });
            Assert.AreEqual(y1, (byte)yArg1.Value);
            Assert.AreEqual(y2, (byte)yArg2.Value);

            MsgArg nArg1 = new MsgArg("n", new object[] { n1 });
            MsgArg nArg2 = new MsgArg("n", new object[] { n2 });
            Assert.AreEqual(n1, (Int16)nArg1.Value);
            Assert.AreEqual(n2, (Int16)nArg2.Value);

            MsgArg qArg1 = new MsgArg("q", new object[] { q1 });
            MsgArg qArg2 = new MsgArg("q", new object[] { q2 });
            Assert.AreEqual(q1, (UInt16)qArg1.Value);
            Assert.AreEqual(q2, (UInt16)qArg2.Value);

            MsgArg iArg1 = new MsgArg("i", new object[] { i1 });
            MsgArg iArg2 = new MsgArg("i", new object[] { i2 });
            Assert.AreEqual(i1, (Int32)iArg1.Value);
            Assert.AreEqual(i2, (Int32)iArg2.Value);

            MsgArg uArg1 = new MsgArg("u", new object[] { u1 });
            MsgArg uArg2 = new MsgArg("u", new object[] { u2 });
            Assert.AreEqual(u1, (UInt32)uArg1.Value);
            Assert.AreEqual(u2, (UInt32)uArg2.Value);

            MsgArg xArg1 = new MsgArg("x", new object[] { x1 });
            MsgArg xArg2 = new MsgArg("x", new object[] { x2 });
            Assert.AreEqual(x1, (Int64)xArg1.Value);
            Assert.AreEqual(x2, (Int64)xArg2.Value);

            MsgArg tArg1 = new MsgArg("t", new object[] { t1 });
            MsgArg tArg2 = new MsgArg("t", new object[] { t2 });
            Assert.AreEqual(t1, (UInt64)tArg1.Value);
            Assert.AreEqual(t2, (UInt64)tArg2.Value);

            MsgArg dArg1 = new MsgArg("d", new object[] { d1 });
            MsgArg dArg2 = new MsgArg("d", new object[] { d2 });
            Assert.AreEqual(d1, (double)dArg1.Value);
            Assert.AreEqual(d2, (double)dArg2.Value);

            MsgArg sArg1 = new MsgArg("s", new object[] { s1 });
            MsgArg sArg2 = new MsgArg("s", new object[] { s2 });
            Assert.AreEqual(s1, (string)sArg1.Value);
            Assert.AreEqual(s2, (string)sArg2.Value);

            MsgArg abArg = new MsgArg("ab", new object[] { bArray });
            CompareArrays<bool>(bArray, (bool[])abArg.Value);

            MsgArg ayArg = new MsgArg("ay", new object[] { yArray });
            CompareArrays<byte>(yArray, (byte[])ayArg.Value);

            MsgArg anArg = new MsgArg("an", new object[] { nArray });
            CompareArrays<Int16>(nArray, (Int16[])anArg.Value);

            MsgArg aqArg = new MsgArg("aq", new object[] { qArray });
            CompareArrays<UInt16>(qArray, (UInt16[])aqArg.Value);

            MsgArg aiArg = new MsgArg("ai", new object[] { iArray });
            CompareArrays<Int32>(iArray, (Int32[])aiArg.Value);

            MsgArg auArg = new MsgArg("au", new object[] { uArray });
            CompareArrays<UInt32>(uArray, (UInt32[])auArg.Value);

            MsgArg axArg = new MsgArg("ax", new object[] { xArray });
            CompareArrays<Int64>(xArray, (Int64[])axArg.Value);

            MsgArg atArg = new MsgArg("at", new object[] { tArray });
            CompareArrays<UInt64>(tArray, (UInt64[])atArg.Value);

            MsgArg adArg = new MsgArg("ad", new object[] { dArray });
            CompareArrays<double>(dArray, (double[])adArg.Value);

            MsgArg asArg = new MsgArg("as", new object[] { sArray });
            // BUGBUG: Throws exception when casting asArg.Value to string[]
            //CompareArrays<string>(sArray, (string[])asArg.Value);

            MsgArg dictArg1 = new MsgArg("{yb}", new object[] { y1, b1 });
            Assert.AreEqual(y1, (byte)dictArg1.Key);
            Assert.AreEqual(b1, (bool)dictArg1.Value);
            MsgArg dictArg2 = new MsgArg("{by}", new object[] { b2, y2 });
            Assert.AreEqual(b2, (bool)dictArg2.Key);
            Assert.AreEqual(y2, (byte)dictArg2.Value);

            MsgArg dictArg3 = new MsgArg("{bn}", new object[] { b1, n1 });
            Assert.AreEqual(b1, (bool)dictArg3.Key);
            Assert.AreEqual(n1, (Int16)dictArg3.Value);
            MsgArg dictArg4 = new MsgArg("{nb}", new object[] { n2, b2 });
            Assert.AreEqual(n2, (Int16)dictArg4.Key);
            Assert.AreEqual(b2, (bool)dictArg4.Value);

            MsgArg dictArg5 = new MsgArg("{nq}", new object[] { n1, q1 });
            Assert.AreEqual(n1, (Int16)dictArg5.Key);
            Assert.AreEqual(q1, (UInt16)dictArg5.Value);
            MsgArg dictArg6 = new MsgArg("{qn}", new object[] { q2, n2 });
            Assert.AreEqual(q2, (UInt16)dictArg6.Key);
            Assert.AreEqual(n2, (Int16)dictArg6.Value);

            MsgArg dictArg7 = new MsgArg("{qi}", new object[] { q1, i1 });
            Assert.AreEqual(q1, (UInt16)dictArg7.Key);
            Assert.AreEqual(i1, (Int32)dictArg7.Value);
            MsgArg dictArg8 = new MsgArg("{iq}", new object[] { i2, q2 });
            Assert.AreEqual(i2, (Int32)dictArg8.Key);
            Assert.AreEqual(q2, (UInt16)dictArg8.Value);

            MsgArg dictArg9 = new MsgArg("{iu}", new object[] { i1, u1 });
            Assert.AreEqual(i1, (Int32)dictArg9.Key);
            Assert.AreEqual(u1, (UInt32)dictArg9.Value);
            MsgArg dictArg10 = new MsgArg("{ui}", new object[] { u2, i2 });
            Assert.AreEqual(u2, (UInt32)dictArg10.Key);
            Assert.AreEqual(i2, (Int32)dictArg10.Value);

            MsgArg dictArg11 = new MsgArg("{ux}", new object[] { u1, x1 });
            Assert.AreEqual(u1, (UInt32)dictArg11.Key);
            Assert.AreEqual(x1, (Int64)dictArg11.Value);
            MsgArg dictArg12 = new MsgArg("{xu}", new object[] { x2, u2 });
            Assert.AreEqual(x2, (Int64)dictArg12.Key);
            Assert.AreEqual(u2, (UInt32)dictArg12.Value);

            MsgArg dictArg13 = new MsgArg("{xt}", new object[] { x1, t1 });
            Assert.AreEqual(x1, (Int64)dictArg13.Key);
            Assert.AreEqual(t1, (UInt64)dictArg13.Value);
            MsgArg dictArg14 = new MsgArg("{tx}", new object[] { t2, x2 });
            Assert.AreEqual(t2, (UInt64)dictArg14.Key);
            Assert.AreEqual(x2, (Int64)dictArg14.Value);

            MsgArg dictArg15 = new MsgArg("{td}", new object[] { t1, d1 });
            Assert.AreEqual(t1, (UInt64)dictArg15.Key);
            Assert.AreEqual(d1, (double)dictArg15.Value);
            MsgArg dictArg16 = new MsgArg("{dt}", new object[] { d2, t2 });
            Assert.AreEqual(d2, (double)dictArg16.Key);
            Assert.AreEqual(t2, (UInt64)dictArg16.Value);

            MsgArg dictArg17 = new MsgArg("{ds}", new object[] { d1, s1 });
            Assert.AreEqual(d1, (double)dictArg17.Key);
            Assert.AreEqual(s1, (string)dictArg17.Value);
            MsgArg dictArg18 = new MsgArg("{sd}", new object[] { s2, d2 });
            Assert.AreEqual(s2, (string)dictArg18.Key);
            Assert.AreEqual(d2, (double)dictArg18.Value);

            TestStruct ts = new TestStruct();
            ts.i = i1;
            ts.s = s1;
            MsgArg sArg = new MsgArg("(is)", new object[] { ts.i, ts.s });
            object[] ar = (object[]) sArg.Value;
            Assert.AreEqual(ts.i, ar[0]);
            Assert.AreEqual(ts.s, ar[1]);
            TestStruct ts2 = new TestStruct() { i = (int)ar[0], s = (string)ar[1] };
            Assert.AreEqual(ts, ts2);


            SimpleStruct1 struct1 = new SimpleStruct1(6566566, "simplestruct", true, 167);
            MsgArg structArg1 = new MsgArg("(isby)", new object[] { struct1.i, struct1.s, struct1.b, struct1.y });
            object[] ar1 = (object[])structArg1.Value;
            Assert.AreEqual(struct1.i, ar1[0]);
            Assert.AreEqual(struct1.s, ar1[1]);
            Assert.AreEqual(struct1.b, ar1[2]);
            Assert.AreEqual(struct1.y, ar1[3]);

            SimpleStruct2 struct2 = new SimpleStruct2(2345.3344, -789342, 7832487934, 134);
            MsgArg structArg2 = new MsgArg("(dxtn)", new object[] { struct2.d, struct2.x, struct2.t, struct2.n });
            object[] ar2 = (object[])structArg2.Value;
            Assert.AreEqual(struct2.d, ar2[0]);
            Assert.AreEqual(struct2.x, ar2[1]);
            Assert.AreEqual(struct2.t, ar2[2]);
            Assert.AreEqual(struct2.n, ar2[3]);

            SimpleStruct3 struct3 = new SimpleStruct3(255, false, 838954, 66.36566);
            MsgArg structArg3 = new MsgArg("(qbud)", new object[] { struct3.q, struct3.b, struct3.u, struct3.d });
            object[] ar3 = (object[])structArg3.Value;
            Assert.AreEqual(struct3.q, ar3[0]);
            Assert.AreEqual(struct3.b, ar3[1]);
            Assert.AreEqual(struct3.u, ar3[2]);
            Assert.AreEqual(struct3.d, ar3[3]);
        }

        public void CompareArrays<T>(T[] a1, T[] a2)
        {
            Assert.AreEqual(a1.Length, a2.Length);
            for (int i = 0; i < a1.Length; i++)
            {
                Assert.AreEqual(a1[i], a2[i]);
            }
        }

        public Random rand = new Random();
        struct Struct1
        {
            public UInt32 u;
            public Struct2 st1;
            public string s;
            public double d;
            public Struct2 st2;
            public Int32 i;
            public UInt16 q;
            public Struct2 st3;
            public bool b;
            public Struct1(UInt32 u1, Struct2 stk1, string s1, double d1, Struct2 stk2, Int32 i1, UInt16 q1, Struct2 stk3, bool b1)
            {
                u = u1; st1 = stk1; s = s1; d = d1; st2 = stk2; i = i1; q = q1; st3 = stk3; b = b1;
            }
            public bool Equals(Struct1 other)
            {
                return (u == other.u) && (st1.Equals(other.st1)) && (s == other.s) && (d == other.d) && (st2.Equals(other.st2)) && 
                    (i == other.i) && (q == other.q) && (st3.Equals(other.st3)) && (b == other.b);
            }
        }
        struct Struct2
        {
            public byte y;
            public Int64 x;
            public double d;
            public Struct2(byte y1, Int64 x1, double d1) { y = y1; x = x1; d = d1; }
            public bool Equals(Struct2 other) { return (y == other.y) && (x == other.x) && (d == other.d); }
        }

        [TestMethod]
        public void PackAndUnpackingComplexMsgArgs()
        {
            //////////////////////////////////////////////////////////////////////////
            Int32[][] aai = new Int32[5][];
            lock (aai)
            {
                for (int i = 0; i < 5; i++)
                {
                    aai[i] = new Int32[10];
                    for (int j = 0; j < 10; j++)
                    {
                        aai[i][j] = rand.Next(Int32.MinValue, Int32.MaxValue);
                    }
                }
            }
            MsgArg[] ma = new MsgArg[5];
            lock (aai)
            {
                //populate the row objects from aai.
                for (int i = 0; i < 5; i++)
                {
                    ma[i] = new MsgArg("ai", new object[] { aai[i] });
                }
                // populate the MsgArg from the array of MsgArg, ma.
                MsgArg aaiArg = new MsgArg("aai", new object[] { ma });

                // now retrieve the values and compare them with the original array.
                object[] aaiOut = (object[])aaiArg.Value;
                Assert.AreEqual(aaiOut.Length, aai.Length);
                for (int i = 0; i < aaiOut.Length; i++)
                {
                    MsgArg a = (MsgArg)aaiOut[i];
                    Int32[] iArr = (Int32[])a.Value;
                    Assert.AreEqual(iArr.Length, aai[i].Length);
                    for (int j = 0; j < iArr.Length; j++)
                    {
                        Assert.AreEqual(iArr[j], aai[i][j]);
                    }
                }
            }
            //////////////////////////////////////////////////////////////////////////

            string sig1 = "(u(yxd)sd(yxd)iq(yxd)b)";
            MsgArg s1 = new MsgArg("(yxd)", new object[] { (byte)212, 35683450634546, 39.435723 });
            MsgArg s2 = new MsgArg("(yxd)", new object[] { (byte)111, -2346656767823, -34.83457 });
            MsgArg s3 = new MsgArg("(yxd)", new object[] { (byte)7, (Int64)3245445, 732.234 });
            MsgArg s4 = new MsgArg(sig1, new object[] { (UInt32)8743,
                (byte)212, 35683450634546, 39.435723,
                "testingthestructs",
                2456.666,
                (byte)111, -2346656767823, -34.83457,
                -36566,
                (UInt16)34,
                (byte)7, (Int64)3245445, 732.234,
                false });

            object[] s4Out = (object[])s4.Value;
            {
                UInt32 ui1 = (UInt32)s4Out[0];
                MsgArg m2 = new MsgArg("(yxd)", (object[])s4Out[1]);
                String st3 = (string)s4Out[2];
                double  d3 = (double)s4Out[3];
                MsgArg m4 = new MsgArg("(yxd)", (object[])s4Out[4]);
                Int32 i4 = (Int32)s4Out[5];
                UInt16 ui5 = (UInt16)s4Out[6];
                MsgArg m6 = new MsgArg("(yxd)", (object[])s4Out[7]);
                bool b7 = (bool)s4Out[8];

                for (int i = 0; i < ((object[])m2.Value).Length; i++)
                {
                    Assert.AreEqual(((object[])m2.Value)[i], ((object[])s1.Value)[i]);
                }
                for (int i = 0; i < ((object[])m4.Value).Length; i++)
                {
                    Assert.AreEqual(((object[])m4.Value)[i], ((object[])s2.Value)[i]);
                }
                for (int i = 0; i < ((object[])m6.Value).Length; i++)
                {
                    Assert.AreEqual(((object[])m6.Value)[i], ((object[])s3.Value)[i]);
                }
            }

            // this is not a valid dictionary arrangement, so don't run this.
            // string sig2 = "{aai(yxd)}";
            // MsgArg dictArg = new MsgArg(sig2, new object[] { ma, (byte)212, 35683450634546, 39.435723 });
            // Compare2DimArrays<Int32>(aai, (Int32[,])dictArg.Key);
            // Assert.AreEqual(s1, (Struct2)dictArg.Value);



        }

        public void Compare2DimArrays<T>(T[,] one, T[,] two)
        {
            Assert.AreEqual(one.GetLength(0), two.GetLength(0));
            Assert.AreEqual(one.GetLength(1), two.GetLength(1));
            for (int i = 0; i < one.GetLength(0); i++)
            {
                for (int j = 0; j < one.GetLength(1); j++)
                {
                    Assert.AreEqual(one[i, j], two[i, j]);
                }
            }
        }
    }
}
