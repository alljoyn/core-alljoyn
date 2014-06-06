/*
 * Copyright (c) 2009-2011, 2014 AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

package org.alljoyn.bus;

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;
import org.alljoyn.bus.annotation.BusProperty;
import org.alljoyn.bus.annotation.Signature;
import org.alljoyn.bus.annotation.Position;

import java.util.Map;
import java.util.TreeMap;
import java.util.Arrays;

@BusInterface
public interface AnnotatedTypesInterface {

    public class InnerStruct {
        @Position(0)
        @Signature("i")
        public int i;

        public InnerStruct() { i = 0; }
        public InnerStruct(int i) { this.i = i; }
        public boolean equals(Object obj) { return i == ((InnerStruct)obj).i; }
    }

    public class Struct {

        @Position(0)
        @Signature("y")
        public byte y;

        @Position(1)
        @Signature("b")
        public boolean b;

        @Position(2)
        @Signature("n")
        public short n;

        @Position(3)
        @Signature("q")
        public short q;

        @Position(4)
        @Signature("i")
        public int i;

        @Position(5)
        @Signature("u")
        public int u;

        @Position(6)
        @Signature("x")
        public long x;

        @Position(7)
        @Signature("t")
        public long t;

        @Position(8)
        @Signature("d")
        public double d;

        @Position(9)
        @Signature("s")
        public String s;

        @Position(10)
        @Signature("o")
        public String o;

        @Position(11)
        @Signature("g")
        public String g;

        @Position(12)
        @Signature("ay")
        public byte[] ay;

        @Position(13)
        @Signature("ab")
        public boolean[] ab;

        @Position(14)
        @Signature("an")
        public short[] an;

        @Position(15)
        @Signature("aq")
        public short[] aq;

        @Position(16)
        @Signature("ai")
        public int[] ai;

        @Position(17)
        @Signature("au")
        public int[] au;

        @Position(18)
        @Signature("ax")
        public long[] ax;

        @Position(19)
        @Signature("at")
        public long[] at;

        @Position(20)
        @Signature("ad")
        public double[] ad;

        @Position(21)
        @Signature("as")
        public String[] as;

        @Position(22)
        @Signature("ao")
        public String[] ao;

        @Position(23)
        @Signature("ag")
        public String[] ag;

        @Position(24)
        @Signature("r")
        public InnerStruct r;

        @Position(25)
        @Signature("a{ss}")
        public TreeMap<String, String> ae;

        @Position(26)
        @Signature("v")
        public Variant v;

        public Struct() {}
        public Struct(byte y, boolean b, short n, short q, int i, int u, long x, long t, double d,
                      String s, String o, String g, byte[] ay, boolean[] ab, short[] an, short[] aq,
                      int[] ai, int[] au, long[] ax, long[] at, double[] ad, String[] as, String[] ao,
                      String[] ag, InnerStruct r, Variant v, TreeMap<String, String> ae) {
            this.y = y;
            this.b = b;
            this.n = n;
            this.q = q;
            this.i = i;
            this.u = u;
            this.x = x;
            this.t = t;
            this.d = d;
            this.s = s;
            this.o = o;
            this.g = g;
            this.ay = ay;
            this.ab = ab;
            this.an = an;
            this.aq = aq;
            this.ai = ai;
            this.au = au;
            this.ax = ax;
            this.at = at;
            this.ad = ad;
            this.as = as;
            this.ao = ao;
            this.ag = ag;
            this.r = r;
            this.v = v;
            this.ae = ae;
        }
        public boolean equals(Object obj) {
            Struct struct = (Struct)obj;
            return
                y == struct.y &&
                b == struct.b &&
                n == struct.n &&
                q == struct.q &&
                i == struct.i &&
                u == struct.u &&
                x == struct.x &&
                t == struct.t &&
                d == struct.d &&
                s.equals(struct.s) &&
                o.equals(struct.o) &&
                g.equals(struct.g) &&
                Arrays.equals(ay, struct.ay) &&
                Arrays.equals(ab, struct.ab) &&
                Arrays.equals(an, struct.an) &&
                Arrays.equals(aq, struct.aq) &&
                Arrays.equals(ai, struct.ai) &&
                Arrays.equals(au, struct.au) &&
                Arrays.equals(ax, struct.ax) &&
                Arrays.equals(at, struct.at) &&
                Arrays.equals(ad, struct.ad) &&
                Arrays.equals(as, struct.as) &&
                Arrays.equals(ao, struct.ao) &&
                Arrays.equals(ag, struct.ag) &&
                r.equals(struct.r) &&
                v.equals(struct.v) &&
                ae.equals(struct.ae);
        }
    }

    @BusMethod(signature="", replySignature="")
    public void Void() throws BusException;

    @BusMethod(signature="y", replySignature="y")
    public byte Byte(byte y) throws BusException;

    @BusMethod(signature="b", replySignature="b")
    public boolean Boolean(boolean b) throws BusException;

    @BusMethod(signature="n", replySignature="n")
    public short Int16(short n) throws BusException;

    @BusMethod(signature="q", replySignature="q")
    public short Uint16(short q) throws BusException;

    @BusMethod(signature="i", replySignature="i")
    public int Int32(int i) throws BusException;

    @BusMethod(signature="u", replySignature="u")
    public int Uint32(int u) throws BusException;

    @BusMethod(signature="x", replySignature="x")
    public long Int64(long x) throws BusException;

    @BusMethod(signature="t", replySignature="t")
    public long Uint64(long t) throws BusException;

    @BusMethod(signature="d", replySignature="d")
    public double Double(double d) throws BusException;

    @BusMethod(signature="s", replySignature="s")
    public String String(String s) throws BusException;

    @BusMethod(signature="o", replySignature="o")
    public String ObjectPath(String o) throws BusException;

    @BusMethod(signature="g", replySignature="g")
    public String Signature(String g) throws BusException;

    @BusMethod(signature="aa{ss}", replySignature="aa{ss}")
    public Map<String, String>[] DictionaryArray(Map<String, String>[] aaess) throws BusException;

    @BusMethod(name="StructArray", signature="ar", replySignature="ar")
    public InnerStruct[] AnnotatedStructArray(InnerStruct[] ar) throws BusException;

    @BusMethod(signature="ad", replySignature="ad")
    public double[] DoubleArray(double[] ad) throws BusException;

    @BusMethod(signature="ax", replySignature="ax")
    public long[] Int64Array(long[] ax) throws BusException;

    @BusMethod(signature="ay", replySignature="ay")
    public byte[] ByteArray(byte[] ay) throws BusException;

    @BusMethod(signature="au", replySignature="au")
    public int[] Uint32Array(int[] au) throws BusException;

    @BusMethod(signature="ag", replySignature="ag")
    public String[] SignatureArray(String[] ag) throws BusException;

    @BusMethod(signature="ai", replySignature="ai")
    public int[] Int32Array(int[] ai) throws BusException;

    @BusMethod(signature="at", replySignature="at")
    public long[] Uint64Array(long[] at) throws BusException;

    @BusMethod(signature="an", replySignature="an")
    public short[] Int16Array(short[] an) throws BusException;

    @BusMethod(signature="av", replySignature="av")
    public Variant[] VariantArray(Variant[] av) throws BusException;

    @BusMethod(signature="as", replySignature="as")
    public String[] StringArray(String[] as) throws BusException;

    @BusMethod(signature="aay", replySignature="aay")
    public byte[][] ArrayArray(byte[][] aay) throws BusException;

    @BusMethod(signature="aq", replySignature="aq")
    public short[] Uint16Array(short[] aq) throws BusException;

    @BusMethod(signature="ab", replySignature="ab")
    public boolean[] BooleanArray(boolean[] ab) throws BusException;

    @BusMethod(signature="ab", replySignature="ab")
    public Boolean[] CapitalBooleanArray(Boolean[] aB) throws BusException;

    @BusMethod(signature="ao", replySignature="ao")
    public String[] ObjectPathArray(String[] ao) throws BusException;

    @BusMethod(name="Struct", signature="r", replySignature="r")
    public Struct AnnotatedStruct(Struct r) throws BusException;

    @BusMethod(signature="v", replySignature="v")
    public Variant Variant(Variant v) throws BusException;

    @BusMethod(signature="a{na{ss}}", replySignature="a{na{ss}}")
    public Map<Short, Map<String, String>> DictionaryNAESS(Map<Short, Map<String, String>> aenaess) throws BusException;

    @BusMethod(name="DictionaryNR", signature="a{nr}", replySignature="a{nr}")
    public Map<Short, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionaryNR(Map<Short, AnnotatedTypesInterface.InnerStruct> aenr) throws BusException;

    @BusMethod(signature="a{nd}", replySignature="a{nd}")
    public Map<Short, Double> DictionaryND(Map<Short, Double> aend) throws BusException;

    @BusMethod(signature="a{nx}", replySignature="a{nx}")
    public Map<Short, Long> DictionaryNX(Map<Short, Long> aenx) throws BusException;

    @BusMethod(signature="a{ny}", replySignature="a{ny}")
    public Map<Short, Byte> DictionaryNY(Map<Short, Byte> aeny) throws BusException;

    @BusMethod(signature="a{nu}", replySignature="a{nu}")
    public Map<Short, Integer> DictionaryNU(Map<Short, Integer> aenu) throws BusException;

    @BusMethod(signature="a{ng}", replySignature="a{ng}")
    public Map<Short, String> DictionaryNG(Map<Short, String> aeng) throws BusException;

    @BusMethod(signature="a{ni}", replySignature="a{ni}")
    public Map<Short, Integer> DictionaryNI(Map<Short, Integer> aeni) throws BusException;

    @BusMethod(signature="a{nt}", replySignature="a{nt}")
    public Map<Short, Long> DictionaryNT(Map<Short, Long> aent) throws BusException;

    @BusMethod(signature="a{nn}", replySignature="a{nn}")
    public Map<Short, Short> DictionaryNN(Map<Short, Short> aenn) throws BusException;

    @BusMethod(signature="a{nv}", replySignature="a{nv}")
    public Map<Short, Variant> DictionaryNV(Map<Short, Variant> aenv) throws BusException;

    @BusMethod(signature="a{ns}", replySignature="a{ns}")
    public Map<Short, String> DictionaryNS(Map<Short, String> aens) throws BusException;

    @BusMethod(signature="a{nay}", replySignature="a{nay}")
    public Map<Short, byte[]> DictionaryNAY(Map<Short, byte[]> aenay) throws BusException;

    @BusMethod(signature="a{nq}", replySignature="a{nq}")
    public Map<Short, Short> DictionaryNQ(Map<Short, Short> aenq) throws BusException;

    @BusMethod(signature="a{nb}", replySignature="a{nb}")
    public Map<Short, Boolean> DictionaryNB(Map<Short, Boolean> aenb) throws BusException;

    @BusMethod(signature="a{no}", replySignature="a{no}")
    public Map<Short, String> DictionaryNO(Map<Short, String> aeno) throws BusException;

    @BusMethod(signature="a{da{ss}}", replySignature="a{da{ss}}")
    public Map<Double, Map<String, String>> DictionaryDAESS(Map<Double, Map<String, String>> aedaess) throws BusException;

    @BusMethod(name="DictionaryDR", signature="a{dr}", replySignature="a{dr}")
    public Map<Double, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionaryDR(Map<Double, AnnotatedTypesInterface.InnerStruct> aedr) throws BusException;

    @BusMethod(signature="a{dd}", replySignature="a{dd}")
    public Map<Double, Double> DictionaryDD(Map<Double, Double> aedd) throws BusException;

    @BusMethod(signature="a{dx}", replySignature="a{dx}")
    public Map<Double, Long> DictionaryDX(Map<Double, Long> aedx) throws BusException;

    @BusMethod(signature="a{dy}", replySignature="a{dy}")
    public Map<Double, Byte> DictionaryDY(Map<Double, Byte> aedy) throws BusException;

    @BusMethod(signature="a{du}", replySignature="a{du}")
    public Map<Double, Integer> DictionaryDU(Map<Double, Integer> aedu) throws BusException;

    @BusMethod(signature="a{dg}", replySignature="a{dg}")
    public Map<Double, String> DictionaryDG(Map<Double, String> aedg) throws BusException;

    @BusMethod(signature="a{di}", replySignature="a{di}")
    public Map<Double, Integer> DictionaryDI(Map<Double, Integer> aedi) throws BusException;

    @BusMethod(signature="a{dt}", replySignature="a{dt}")
    public Map<Double, Long> DictionaryDT(Map<Double, Long> aedt) throws BusException;

    @BusMethod(signature="a{dn}", replySignature="a{dn}")
    public Map<Double, Short> DictionaryDN(Map<Double, Short> aedn) throws BusException;

    @BusMethod(signature="a{dv}", replySignature="a{dv}")
    public Map<Double, Variant> DictionaryDV(Map<Double, Variant> aedv) throws BusException;

    @BusMethod(signature="a{ds}", replySignature="a{ds}")
    public Map<Double, String> DictionaryDS(Map<Double, String> aeds) throws BusException;

    @BusMethod(signature="a{day}", replySignature="a{day}")
    public Map<Double, byte[]> DictionaryDAY(Map<Double, byte[]> aeday) throws BusException;

    @BusMethod(signature="a{dq}", replySignature="a{dq}")
    public Map<Double, Short> DictionaryDQ(Map<Double, Short> aedq) throws BusException;

    @BusMethod(signature="a{db}", replySignature="a{db}")
    public Map<Double, Boolean> DictionaryDB(Map<Double, Boolean> aedb) throws BusException;

    @BusMethod(signature="a{do}", replySignature="a{do}")
    public Map<Double, String> DictionaryDO(Map<Double, String> aedo) throws BusException;

    @BusMethod(signature="a{xa{ss}}", replySignature="a{xa{ss}}")
    public Map<Long, Map<String, String>> DictionaryXAESS(Map<Long, Map<String, String>> aexaess) throws BusException;

    @BusMethod(name="DictionaryXR", signature="a{xr}", replySignature="a{xr}")
    public Map<Long, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionaryXR(Map<Long, AnnotatedTypesInterface.InnerStruct> aexr) throws BusException;

    @BusMethod(signature="a{xd}", replySignature="a{xd}")
    public Map<Long, Double> DictionaryXD(Map<Long, Double> aexd) throws BusException;

    @BusMethod(signature="a{xx}", replySignature="a{xx}")
    public Map<Long, Long> DictionaryXX(Map<Long, Long> aexx) throws BusException;

    @BusMethod(signature="a{xy}", replySignature="a{xy}")
    public Map<Long, Byte> DictionaryXY(Map<Long, Byte> aexy) throws BusException;

    @BusMethod(signature="a{xu}", replySignature="a{xu}")
    public Map<Long, Integer> DictionaryXU(Map<Long, Integer> aexu) throws BusException;

    @BusMethod(signature="a{xg}", replySignature="a{xg}")
    public Map<Long, String> DictionaryXG(Map<Long, String> aexg) throws BusException;

    @BusMethod(signature="a{xi}", replySignature="a{xi}")
    public Map<Long, Integer> DictionaryXI(Map<Long, Integer> aexi) throws BusException;

    @BusMethod(signature="a{xt}", replySignature="a{xt}")
    public Map<Long, Long> DictionaryXT(Map<Long, Long> aext) throws BusException;

    @BusMethod(signature="a{xn}", replySignature="a{xn}")
    public Map<Long, Short> DictionaryXN(Map<Long, Short> aexn) throws BusException;

    @BusMethod(signature="a{xv}", replySignature="a{xv}")
    public Map<Long, Variant> DictionaryXV(Map<Long, Variant> aexv) throws BusException;

    @BusMethod(signature="a{xs}", replySignature="a{xs}")
    public Map<Long, String> DictionaryXS(Map<Long, String> aexs) throws BusException;

    @BusMethod(signature="a{xay}", replySignature="a{xay}")
    public Map<Long, byte[]> DictionaryXAY(Map<Long, byte[]> aexay) throws BusException;

    @BusMethod(signature="a{xq}", replySignature="a{xq}")
    public Map<Long, Short> DictionaryXQ(Map<Long, Short> aexq) throws BusException;

    @BusMethod(signature="a{xb}", replySignature="a{xb}")
    public Map<Long, Boolean> DictionaryXB(Map<Long, Boolean> aexb) throws BusException;

    @BusMethod(signature="a{xo}", replySignature="a{xo}")
    public Map<Long, String> DictionaryXO(Map<Long, String> aexo) throws BusException;

    @BusMethod(signature="a{sa{ss}}", replySignature="a{sa{ss}}")
    public Map<String, Map<String, String>> DictionarySAESS(Map<String, Map<String, String>> aesaess) throws BusException;

    @BusMethod(name="DictionarySR", signature="a{sr}", replySignature="a{sr}")
    public Map<String, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionarySR(Map<String, AnnotatedTypesInterface.InnerStruct> aesr) throws BusException;

    @BusMethod(signature="a{sd}", replySignature="a{sd}")
    public Map<String, Double> DictionarySD(Map<String, Double> aesd) throws BusException;

    @BusMethod(signature="a{sx}", replySignature="a{sx}")
    public Map<String, Long> DictionarySX(Map<String, Long> aesx) throws BusException;

    @BusMethod(signature="a{sy}", replySignature="a{sy}")
    public Map<String, Byte> DictionarySY(Map<String, Byte> aesy) throws BusException;

    @BusMethod(signature="a{su}", replySignature="a{su}")
    public Map<String, Integer> DictionarySU(Map<String, Integer> aesu) throws BusException;

    @BusMethod(signature="a{sg}", replySignature="a{sg}")
    public Map<String, String> DictionarySG(Map<String, String> aesg) throws BusException;

    @BusMethod(signature="a{si}", replySignature="a{si}")
    public Map<String, Integer> DictionarySI(Map<String, Integer> aesi) throws BusException;

    @BusMethod(signature="a{st}", replySignature="a{st}")
    public Map<String, Long> DictionaryST(Map<String, Long> aest) throws BusException;

    @BusMethod(signature="a{sn}", replySignature="a{sn}")
    public Map<String, Short> DictionarySN(Map<String, Short> aesn) throws BusException;

    @BusMethod(signature="a{sv}", replySignature="a{sv}")
    public Map<String, Variant> DictionarySV(Map<String, Variant> aesv) throws BusException;

    @BusMethod(signature="a{ss}", replySignature="a{ss}")
    public Map<String, String> DictionarySS(Map<String, String> aess) throws BusException;

    @BusMethod(signature="a{say}", replySignature="a{say}")
    public Map<String, byte[]> DictionarySAY(Map<String, byte[]> aesay) throws BusException;

    @BusMethod(signature="a{sq}", replySignature="a{sq}")
    public Map<String, Short> DictionarySQ(Map<String, Short> aesq) throws BusException;

    @BusMethod(signature="a{sb}", replySignature="a{sb}")
    public Map<String, Boolean> DictionarySB(Map<String, Boolean> aesb) throws BusException;

    @BusMethod(signature="a{so}", replySignature="a{so}")
    public Map<String, String> DictionarySO(Map<String, String> aeso) throws BusException;

    @BusMethod(signature="a{ya{ss}}", replySignature="a{ya{ss}}")
    public Map<Byte, Map<String, String>> DictionaryYAESS(Map<Byte, Map<String, String>> aeyaess) throws BusException;

    @BusMethod(name="DictionaryYR", signature="a{yr}", replySignature="a{yr}")
    public Map<Byte, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionaryYR(Map<Byte, AnnotatedTypesInterface.InnerStruct> aeyr) throws BusException;

    @BusMethod(signature="a{yd}", replySignature="a{yd}")
    public Map<Byte, Double> DictionaryYD(Map<Byte, Double> aeyd) throws BusException;

    @BusMethod(signature="a{yx}", replySignature="a{yx}")
    public Map<Byte, Long> DictionaryYX(Map<Byte, Long> aeyx) throws BusException;

    @BusMethod(signature="a{yy}", replySignature="a{yy}")
    public Map<Byte, Byte> DictionaryYY(Map<Byte, Byte> aeyy) throws BusException;

    @BusMethod(signature="a{yu}", replySignature="a{yu}")
    public Map<Byte, Integer> DictionaryYU(Map<Byte, Integer> aeyu) throws BusException;

    @BusMethod(signature="a{yg}", replySignature="a{yg}")
    public Map<Byte, String> DictionaryYG(Map<Byte, String> aeyg) throws BusException;

    @BusMethod(signature="a{yi}", replySignature="a{yi}")
    public Map<Byte, Integer> DictionaryYI(Map<Byte, Integer> aeyi) throws BusException;

    @BusMethod(signature="a{yt}", replySignature="a{yt}")
    public Map<Byte, Long> DictionaryYT(Map<Byte, Long> aeyt) throws BusException;

    @BusMethod(signature="a{yn}", replySignature="a{yn}")
    public Map<Byte, Short> DictionaryYN(Map<Byte, Short> aeyn) throws BusException;

    @BusMethod(signature="a{yv}", replySignature="a{yv}")
    public Map<Byte, Variant> DictionaryYV(Map<Byte, Variant> aeyv) throws BusException;

    @BusMethod(signature="a{ys}", replySignature="a{ys}")
    public Map<Byte, String> DictionaryYS(Map<Byte, String> aeys) throws BusException;

    @BusMethod(signature="a{yay}", replySignature="a{yay}")
    public Map<Byte, byte[]> DictionaryYAY(Map<Byte, byte[]> aeyay) throws BusException;

    @BusMethod(signature="a{yq}", replySignature="a{yq}")
    public Map<Byte, Short> DictionaryYQ(Map<Byte, Short> aeyq) throws BusException;

    @BusMethod(signature="a{yb}", replySignature="a{yb}")
    public Map<Byte, Boolean> DictionaryYB(Map<Byte, Boolean> aeyb) throws BusException;

    @BusMethod(signature="a{yo}", replySignature="a{yo}")
    public Map<Byte, String> DictionaryYO(Map<Byte, String> aeyo) throws BusException;

    @BusMethod(signature="a{ua{ss}}", replySignature="a{ua{ss}}")
    public Map<Integer, Map<String, String>> DictionaryUAESS(Map<Integer, Map<String, String>> aeuaess) throws BusException;

    @BusMethod(signature="a{ur}", replySignature="a{ur}")
    public Map<Integer, AnnotatedTypesInterface.InnerStruct> DictionaryUR(Map<Integer, AnnotatedTypesInterface.InnerStruct> aeur) throws BusException;

    @BusMethod(signature="a{ud}", replySignature="a{ud}")
    public Map<Integer, Double> DictionaryUD(Map<Integer, Double> aeud) throws BusException;

    @BusMethod(signature="a{ux}", replySignature="a{ux}")
    public Map<Integer, Long> DictionaryUX(Map<Integer, Long> aeux) throws BusException;

    @BusMethod(signature="a{uy}", replySignature="a{uy}")
    public Map<Integer, Byte> DictionaryUY(Map<Integer, Byte> aeuy) throws BusException;

    @BusMethod(signature="a{uu}", replySignature="a{uu}")
    public Map<Integer, Integer> DictionaryUU(Map<Integer, Integer> aeuu) throws BusException;

    @BusMethod(signature="a{ug}", replySignature="a{ug}")
    public Map<Integer, String> DictionaryUG(Map<Integer, String> aeug) throws BusException;

    @BusMethod(signature="a{ui}", replySignature="a{ui}")
    public Map<Integer, Integer> DictionaryUI(Map<Integer, Integer> aeui) throws BusException;

    @BusMethod(signature="a{ut}", replySignature="a{ut}")
    public Map<Integer, Long> DictionaryUT(Map<Integer, Long> aeut) throws BusException;

    @BusMethod(signature="a{un}", replySignature="a{un}")
    public Map<Integer, Short> DictionaryUN(Map<Integer, Short> aeun) throws BusException;

    @BusMethod(signature="a{uv}", replySignature="a{uv}")
    public Map<Integer, Variant> DictionaryUV(Map<Integer, Variant> aeuv) throws BusException;

    @BusMethod(signature="a{us}", replySignature="a{us}")
    public Map<Integer, String> DictionaryUS(Map<Integer, String> aeus) throws BusException;

    @BusMethod(signature="a{uay}", replySignature="a{uay}")
    public Map<Integer, byte[]> DictionaryUAY(Map<Integer, byte[]> aeuay) throws BusException;

    @BusMethod(signature="a{uq}", replySignature="a{uq}")
    public Map<Integer, Short> DictionaryUQ(Map<Integer, Short> aeuq) throws BusException;

    @BusMethod(signature="a{ub}", replySignature="a{ub}")
    public Map<Integer, Boolean> DictionaryUB(Map<Integer, Boolean> aeub) throws BusException;

    @BusMethod(signature="a{uo}", replySignature="a{uo}")
    public Map<Integer, String> DictionaryUO(Map<Integer, String> aeuo) throws BusException;

    @BusMethod(signature="a{ga{ss}}", replySignature="a{ga{ss}}")
    public Map<String, Map<String, String>> DictionaryGAESS(Map<String, Map<String, String>> aegaess) throws BusException;

    @BusMethod(signature="a{gr}", replySignature="a{gr}")
    public Map<String, AnnotatedTypesInterface.InnerStruct> DictionaryGR(Map<String, AnnotatedTypesInterface.InnerStruct> aegr) throws BusException;

    @BusMethod(signature="a{gd}", replySignature="a{gd}")
    public Map<String, Double> DictionaryGD(Map<String, Double> aegd) throws BusException;

    @BusMethod(signature="a{gx}", replySignature="a{gx}")
    public Map<String, Long> DictionaryGX(Map<String, Long> aegx) throws BusException;

    @BusMethod(signature="a{gy}", replySignature="a{gy}")
    public Map<String, Byte> DictionaryGY(Map<String, Byte> aegy) throws BusException;

    @BusMethod(signature="a{gu}", replySignature="a{gu}")
    public Map<String, Integer> DictionaryGU(Map<String, Integer> aegu) throws BusException;

    @BusMethod(signature="a{gg}", replySignature="a{gg}")
    public Map<String, String> DictionaryGG(Map<String, String> aegg) throws BusException;

    @BusMethod(signature="a{gi}", replySignature="a{gi}")
    public Map<String, Integer> DictionaryGI(Map<String, Integer> aegi) throws BusException;

    @BusMethod(signature="a{gt}", replySignature="a{gt}")
    public Map<String, Long> DictionaryGT(Map<String, Long> aegt) throws BusException;

    @BusMethod(signature="a{gn}", replySignature="a{gn}")
    public Map<String, Short> DictionaryGN(Map<String, Short> aegn) throws BusException;

    @BusMethod(signature="a{gv}", replySignature="a{gv}")
    public Map<String, Variant> DictionaryGV(Map<String, Variant> aegv) throws BusException;

    @BusMethod(signature="a{gs}", replySignature="a{gs}")
    public Map<String, String> DictionaryGS(Map<String, String> aegs) throws BusException;

    @BusMethod(signature="a{gay}", replySignature="a{gay}")
    public Map<String, byte[]> DictionaryGAY(Map<String, byte[]> aegay) throws BusException;

    @BusMethod(signature="a{gq}", replySignature="a{gq}")
    public Map<String, Short> DictionaryGQ(Map<String, Short> aegq) throws BusException;

    @BusMethod(signature="a{gb}", replySignature="a{gb}")
    public Map<String, Boolean> DictionaryGB(Map<String, Boolean> aegb) throws BusException;

    @BusMethod(signature="a{go}", replySignature="a{go}")
    public Map<String, String> DictionaryGO(Map<String, String> aego) throws BusException;

    @BusMethod(signature="a{ba{ss}}", replySignature="a{ba{ss}}")
    public Map<Boolean, Map<String, String>> DictionaryBAESS(Map<Boolean, Map<String, String>> aebaess) throws BusException;

    @BusMethod(name="DictionaryBR", signature="a{br}", replySignature="a{br}")
    public Map<Boolean, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionaryBR(Map<Boolean, AnnotatedTypesInterface.InnerStruct> aebr) throws BusException;

    @BusMethod(signature="a{bd}", replySignature="a{bd}")
    public Map<Boolean, Double> DictionaryBD(Map<Boolean, Double> aebd) throws BusException;

    @BusMethod(signature="a{bx}", replySignature="a{bx}")
    public Map<Boolean, Long> DictionaryBX(Map<Boolean, Long> aebx) throws BusException;

    @BusMethod(signature="a{by}", replySignature="a{by}")
    public Map<Boolean, Byte> DictionaryBY(Map<Boolean, Byte> aeby) throws BusException;

    @BusMethod(signature="a{bu}", replySignature="a{bu}")
    public Map<Boolean, Integer> DictionaryBU(Map<Boolean, Integer> aebu) throws BusException;

    @BusMethod(signature="a{bg}", replySignature="a{bg}")
    public Map<Boolean, String> DictionaryBG(Map<Boolean, String> aebg) throws BusException;

    @BusMethod(signature="a{bi}", replySignature="a{bi}")
    public Map<Boolean, Integer> DictionaryBI(Map<Boolean, Integer> aebi) throws BusException;

    @BusMethod(signature="a{bt}", replySignature="a{bt}")
    public Map<Boolean, Long> DictionaryBT(Map<Boolean, Long> aebt) throws BusException;

    @BusMethod(signature="a{bn}", replySignature="a{bn}")
    public Map<Boolean, Short> DictionaryBN(Map<Boolean, Short> aebn) throws BusException;

    @BusMethod(signature="a{bv}", replySignature="a{bv}")
    public Map<Boolean, Variant> DictionaryBV(Map<Boolean, Variant> aebv) throws BusException;

    @BusMethod(signature="a{bs}", replySignature="a{bs}")
    public Map<Boolean, String> DictionaryBS(Map<Boolean, String> aebs) throws BusException;

    @BusMethod(signature="a{bay}", replySignature="a{bay}")
    public Map<Boolean, byte[]> DictionaryBAY(Map<Boolean, byte[]> aebay) throws BusException;

    @BusMethod(signature="a{bq}", replySignature="a{bq}")
    public Map<Boolean, Short> DictionaryBQ(Map<Boolean, Short> aebq) throws BusException;

    @BusMethod(signature="a{bb}", replySignature="a{bb}")
    public Map<Boolean, Boolean> DictionaryBB(Map<Boolean, Boolean> aebb) throws BusException;

    @BusMethod(signature="a{bo}", replySignature="a{bo}")
    public Map<Boolean, String> DictionaryBO(Map<Boolean, String> aebo) throws BusException;

    @BusMethod(signature="a{qa{ss}}", replySignature="a{qa{ss}}")
    public Map<Short, Map<String, String>> DictionaryQAESS(Map<Short, Map<String, String>> aeqaess) throws BusException;

    @BusMethod(signature="a{qr}", replySignature="a{qr}")
    public Map<Short, AnnotatedTypesInterface.InnerStruct> DictionaryQR(Map<Short, AnnotatedTypesInterface.InnerStruct> aeqr) throws BusException;

    @BusMethod(signature="a{qd}", replySignature="a{qd}")
    public Map<Short, Double> DictionaryQD(Map<Short, Double> aeqd) throws BusException;

    @BusMethod(signature="a{qx}", replySignature="a{qx}")
    public Map<Short, Long> DictionaryQX(Map<Short, Long> aeqx) throws BusException;

    @BusMethod(signature="a{qy}", replySignature="a{qy}")
    public Map<Short, Byte> DictionaryQY(Map<Short, Byte> aeqy) throws BusException;

    @BusMethod(signature="a{qu}", replySignature="a{qu}")
    public Map<Short, Integer> DictionaryQU(Map<Short, Integer> aequ) throws BusException;

    @BusMethod(signature="a{qg}", replySignature="a{qg}")
    public Map<Short, String> DictionaryQG(Map<Short, String> aeqg) throws BusException;

    @BusMethod(signature="a{qi}", replySignature="a{qi}")
    public Map<Short, Integer> DictionaryQI(Map<Short, Integer> aeqi) throws BusException;

    @BusMethod(signature="a{qt}", replySignature="a{qt}")
    public Map<Short, Long> DictionaryQT(Map<Short, Long> aeqt) throws BusException;

    @BusMethod(signature="a{qn}", replySignature="a{qn}")
    public Map<Short, Short> DictionaryQN(Map<Short, Short> aeqn) throws BusException;

    @BusMethod(signature="a{qv}", replySignature="a{qv}")
    public Map<Short, Variant> DictionaryQV(Map<Short, Variant> aeqv) throws BusException;

    @BusMethod(signature="a{qs}", replySignature="a{qs}")
    public Map<Short, String> DictionaryQS(Map<Short, String> aeqs) throws BusException;

    @BusMethod(signature="a{qay}", replySignature="a{qay}")
    public Map<Short, byte[]> DictionaryQAY(Map<Short, byte[]> aeqay) throws BusException;

    @BusMethod(signature="a{qq}", replySignature="a{qq}")
    public Map<Short, Short> DictionaryQQ(Map<Short, Short> aeqq) throws BusException;

    @BusMethod(signature="a{qb}", replySignature="a{qb}")
    public Map<Short, Boolean> DictionaryQB(Map<Short, Boolean> aeqb) throws BusException;

    @BusMethod(signature="a{qo}", replySignature="a{qo}")
    public Map<Short, String> DictionaryQO(Map<Short, String> aeqo) throws BusException;

    @BusMethod(signature="a{oa{ss}}", replySignature="a{oa{ss}}")
    public Map<String, Map<String, String>> DictionaryOAESS(Map<String, Map<String, String>> aeoaess) throws BusException;

    @BusMethod(signature="a{or}", replySignature="a{or}")
    public Map<String, AnnotatedTypesInterface.InnerStruct> DictionaryOR(Map<String, AnnotatedTypesInterface.InnerStruct> aeor) throws BusException;

    @BusMethod(signature="a{od}", replySignature="a{od}")
    public Map<String, Double> DictionaryOD(Map<String, Double> aeod) throws BusException;

    @BusMethod(signature="a{ox}", replySignature="a{ox}")
    public Map<String, Long> DictionaryOX(Map<String, Long> aeox) throws BusException;

    @BusMethod(signature="a{oy}", replySignature="a{oy}")
    public Map<String, Byte> DictionaryOY(Map<String, Byte> aeoy) throws BusException;

    @BusMethod(signature="a{ou}", replySignature="a{ou}")
    public Map<String, Integer> DictionaryOU(Map<String, Integer> aeou) throws BusException;

    @BusMethod(signature="a{og}", replySignature="a{og}")
    public Map<String, String> DictionaryOG(Map<String, String> aeog) throws BusException;

    @BusMethod(signature="a{oi}", replySignature="a{oi}")
    public Map<String, Integer> DictionaryOI(Map<String, Integer> aeoi) throws BusException;

    @BusMethod(signature="a{ot}", replySignature="a{ot}")
    public Map<String, Long> DictionaryOT(Map<String, Long> aeot) throws BusException;

    @BusMethod(signature="a{on}", replySignature="a{on}")
    public Map<String, Short> DictionaryON(Map<String, Short> aeon) throws BusException;

    @BusMethod(signature="a{ov}", replySignature="a{ov}")
    public Map<String, Variant> DictionaryOV(Map<String, Variant> aeov) throws BusException;

    @BusMethod(signature="a{os}", replySignature="a{os}")
    public Map<String, String> DictionaryOS(Map<String, String> aeos) throws BusException;

    @BusMethod(signature="a{oay}", replySignature="a{oay}")
    public Map<String, byte[]> DictionaryOAY(Map<String, byte[]> aeoay) throws BusException;

    @BusMethod(signature="a{oq}", replySignature="a{oq}")
    public Map<String, Short> DictionaryOQ(Map<String, Short> aeoq) throws BusException;

    @BusMethod(signature="a{ob}", replySignature="a{ob}")
    public Map<String, Boolean> DictionaryOB(Map<String, Boolean> aeob) throws BusException;

    @BusMethod(signature="a{oo}", replySignature="a{oo}")
    public Map<String, String> DictionaryOO(Map<String, String> aeoo) throws BusException;

    @BusMethod(signature="a{ia{ss}}", replySignature="a{ia{ss}}")
    public Map<Integer, Map<String, String>> DictionaryIAESS(Map<Integer, Map<String, String>> aeiaess) throws BusException;

    @BusMethod(name="DictionaryIR", signature="a{ir}", replySignature="a{ir}")
    public Map<Integer, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionaryIR(Map<Integer, AnnotatedTypesInterface.InnerStruct> aeir) throws BusException;

    @BusMethod(signature="a{id}", replySignature="a{id}")
    public Map<Integer, Double> DictionaryID(Map<Integer, Double> aeid) throws BusException;

    @BusMethod(signature="a{ix}", replySignature="a{ix}")
    public Map<Integer, Long> DictionaryIX(Map<Integer, Long> aeix) throws BusException;

    @BusMethod(signature="a{iy}", replySignature="a{iy}")
    public Map<Integer, Byte> DictionaryIY(Map<Integer, Byte> aeiy) throws BusException;

    @BusMethod(signature="a{iu}", replySignature="a{iu}")
    public Map<Integer, Integer> DictionaryIU(Map<Integer, Integer> aeiu) throws BusException;

    @BusMethod(signature="a{ig}", replySignature="a{ig}")
    public Map<Integer, String> DictionaryIG(Map<Integer, String> aeig) throws BusException;

    @BusMethod(signature="a{ii}", replySignature="a{ii}")
    public Map<Integer, Integer> DictionaryII(Map<Integer, Integer> aeii) throws BusException;

    @BusMethod(signature="a{it}", replySignature="a{it}")
    public Map<Integer, Long> DictionaryIT(Map<Integer, Long> aeit) throws BusException;

    @BusMethod(signature="a{in}", replySignature="a{in}")
    public Map<Integer, Short> DictionaryIN(Map<Integer, Short> aein) throws BusException;

    @BusMethod(signature="a{iv}", replySignature="a{iv}")
    public Map<Integer, Variant> DictionaryIV(Map<Integer, Variant> aeiv) throws BusException;

    @BusMethod(signature="a{is}", replySignature="a{is}")
    public Map<Integer, String> DictionaryIS(Map<Integer, String> aeis) throws BusException;

    @BusMethod(signature="a{iay}", replySignature="a{iay}")
    public Map<Integer, byte[]> DictionaryIAY(Map<Integer, byte[]> aeiay) throws BusException;

    @BusMethod(signature="a{iq}", replySignature="a{iq}")
    public Map<Integer, Short> DictionaryIQ(Map<Integer, Short> aeiq) throws BusException;

    @BusMethod(signature="a{ib}", replySignature="a{ib}")
    public Map<Integer, Boolean> DictionaryIB(Map<Integer, Boolean> aeib) throws BusException;

    @BusMethod(signature="a{io}", replySignature="a{io}")
    public Map<Integer, String> DictionaryIO(Map<Integer, String> aeio) throws BusException;

    @BusMethod(signature="a{ta{ss}}", replySignature="a{ta{ss}}")
    public Map<Long, Map<String, String>> DictionaryTAESS(Map<Long, Map<String, String>> aetaess) throws BusException;

    @BusMethod(signature="a{tr}", replySignature="a{tr}")
    public Map<Long, AnnotatedTypesInterface.InnerStruct> DictionaryTR(Map<Long, AnnotatedTypesInterface.InnerStruct> aetr) throws BusException;

    @BusMethod(signature="a{td}", replySignature="a{td}")
    public Map<Long, Double> DictionaryTD(Map<Long, Double> aetd) throws BusException;

    @BusMethod(signature="a{tx}", replySignature="a{tx}")
    public Map<Long, Long> DictionaryTX(Map<Long, Long> aetx) throws BusException;

    @BusMethod(signature="a{ty}", replySignature="a{ty}")
    public Map<Long, Byte> DictionaryTY(Map<Long, Byte> aety) throws BusException;

    @BusMethod(signature="a{tu}", replySignature="a{tu}")
    public Map<Long, Integer> DictionaryTU(Map<Long, Integer> aetu) throws BusException;

    @BusMethod(signature="a{tg}", replySignature="a{tg}")
    public Map<Long, String> DictionaryTG(Map<Long, String> aetg) throws BusException;

    @BusMethod(signature="a{ti}", replySignature="a{ti}")
    public Map<Long, Integer> DictionaryTI(Map<Long, Integer> aeti) throws BusException;

    @BusMethod(signature="a{tt}", replySignature="a{tt}")
    public Map<Long, Long> DictionaryTT(Map<Long, Long> aett) throws BusException;

    @BusMethod(signature="a{tn}", replySignature="a{tn}")
    public Map<Long, Short> DictionaryTN(Map<Long, Short> aetn) throws BusException;

    @BusMethod(signature="a{tv}", replySignature="a{tv}")
    public Map<Long, Variant> DictionaryTV(Map<Long, Variant> aetv) throws BusException;

    @BusMethod(signature="a{ts}", replySignature="a{ts}")
    public Map<Long, String> DictionaryTS(Map<Long, String> aets) throws BusException;

    @BusMethod(signature="a{tay}", replySignature="a{tay}")
    public Map<Long, byte[]> DictionaryTAY(Map<Long, byte[]> aetay) throws BusException;

    @BusMethod(signature="a{tq}", replySignature="a{tq}")
    public Map<Long, Short> DictionaryTQ(Map<Long, Short> aetq) throws BusException;

    @BusMethod(signature="a{tb}", replySignature="a{tb}")
    public Map<Long, Boolean> DictionaryTB(Map<Long, Boolean> aetb) throws BusException;

    @BusMethod(signature="a{to}", replySignature="a{to}")
    public Map<Long, String> DictionaryTO(Map<Long, String> aeto) throws BusException;

    @BusProperty(signature="a{ss}")
    public Map<String, String> getDictionarySS() throws BusException;

    @BusProperty(signature="a{ss}")
    public void setDictionarySS(Map<String, String> aess) throws BusException;

    public enum EnumType {
        Enum0;
    };

    @BusMethod(signature="y", replySignature="y")
    public EnumType EnumY(EnumType y) throws BusException;

    @BusMethod(signature="n", replySignature="n")
    public EnumType EnumN(EnumType n) throws BusException;

    @BusMethod(signature="q", replySignature="q")
    public EnumType EnumQ(EnumType q) throws BusException;

    @BusMethod(signature="i", replySignature="i")
    public EnumType EnumI(EnumType i) throws BusException;

    @BusMethod(signature="u", replySignature="u")
    public EnumType EnumU(EnumType u) throws BusException;

    @BusMethod(signature="x", replySignature="x")
    public EnumType EnumX(EnumType x) throws BusException;

    @BusMethod(signature="t", replySignature="t")
    public EnumType EnumT(EnumType t) throws BusException;
}
