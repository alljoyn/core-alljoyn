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

    @BusMethod(name="Void", signature="", replySignature="")
    public void voidMethod() throws BusException;

    @BusMethod(name="Byte", signature="y", replySignature="y")
    public byte byteMethod(byte y) throws BusException;

    @BusMethod(name="Boolean", signature="b", replySignature="b")
    public boolean booleanMethod(boolean b) throws BusException;

    @BusMethod(name="Int16", signature="n", replySignature="n")
    public short int16(short n) throws BusException;

    @BusMethod(name="Uint16", signature="q", replySignature="q")
    public short uint16(short q) throws BusException;

    @BusMethod(name="Int32", signature="i", replySignature="i")
    public int int32(int i) throws BusException;

    @BusMethod(name="Uint32", signature="u", replySignature="u")
    public int uint32(int u) throws BusException;

    @BusMethod(name="Int64", signature="x", replySignature="x")
    public long int64(long x) throws BusException;

    @BusMethod(name="Uint64", signature="t", replySignature="t")
    public long uint64(long t) throws BusException;

    @BusMethod(name="Double", signature="d", replySignature="d")
    public double doubleMethod(double d) throws BusException;

    @BusMethod(name="String", signature="s", replySignature="s")
    public String string(String s) throws BusException;

    @BusMethod(name="ObjectPath", signature="o", replySignature="o")
    public String objectPath(String o) throws BusException;

    @BusMethod(name="Signature", signature="g", replySignature="g")
    public String signature(String g) throws BusException;

    @BusMethod(name="DictionaryArray", signature="aa{ss}", replySignature="aa{ss}")
    public Map<String, String>[] dictionaryArray(Map<String, String>[] aaess) throws BusException;

    @BusMethod(name="StructArray", signature="ar", replySignature="ar")
    public InnerStruct[] annotatedStructArray(InnerStruct[] ar) throws BusException;

    @BusMethod(name="DoubleArray", signature="ad", replySignature="ad")
    public double[] doubleArray(double[] ad) throws BusException;

    @BusMethod(name="Int64Array", signature="ax", replySignature="ax")
    public long[] int64Array(long[] ax) throws BusException;

    @BusMethod(name="ByteArray", signature="ay", replySignature="ay")
    public byte[] byteArray(byte[] ay) throws BusException;

    @BusMethod(name="Uint32Array", signature="au", replySignature="au")
    public int[] uint32Array(int[] au) throws BusException;

    @BusMethod(name="SignatureArray", signature="ag", replySignature="ag")
    public String[] signatureArray(String[] ag) throws BusException;

    @BusMethod(name="Int32Array", signature="ai", replySignature="ai")
    public int[] int32Array(int[] ai) throws BusException;

    @BusMethod(name="Uint64Array", signature="at", replySignature="at")
    public long[] uint64Array(long[] at) throws BusException;

    @BusMethod(name="Int16Array", signature="an", replySignature="an")
    public short[] int16Array(short[] an) throws BusException;

    @BusMethod(name="VariantArray", signature="av", replySignature="av")
    public Variant[] variantArray(Variant[] av) throws BusException;

    @BusMethod(name="StringArray", signature="as", replySignature="as")
    public String[] stringArray(String[] as) throws BusException;

    @BusMethod(name="ArrayArray", signature="aay", replySignature="aay")
    public byte[][] arrayArray(byte[][] aay) throws BusException;

    @BusMethod(name="Uint16Array", signature="aq", replySignature="aq")
    public short[] uint16Array(short[] aq) throws BusException;

    @BusMethod(name="BooleanArray", signature="ab", replySignature="ab")
    public boolean[] booleanArray(boolean[] ab) throws BusException;

    @BusMethod(name="CapitalBooleanArray", signature="ab", replySignature="ab")
    public Boolean[] capitalBooleanArray(Boolean[] aB) throws BusException;

    @BusMethod(name="ObjectPathArray", signature="ao", replySignature="ao")
    public String[] objectPathArray(String[] ao) throws BusException;

    @BusMethod(name="Struct", signature="r", replySignature="r")
    public Struct annotatedStruct(Struct r) throws BusException;

    @BusMethod(name="Variant",signature="v", replySignature="v")
    public Variant variant(Variant v) throws BusException;

    @BusMethod(name="DictionaryNAESS", signature="a{na{ss}}", replySignature="a{na{ss}}")
    public Map<Short, Map<String, String>> dictionaryNAESS(Map<Short, Map<String, String>> aenaess) throws BusException;

    @BusMethod(name="DictionaryNR", signature="a{nr}", replySignature="a{nr}")
    public Map<Short, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionaryNR(Map<Short, AnnotatedTypesInterface.InnerStruct> aenr) throws BusException;

    @BusMethod(name="DictionaryND", signature="a{nd}", replySignature="a{nd}")
    public Map<Short, Double> dictionaryND(Map<Short, Double> aend) throws BusException;

    @BusMethod(name="DictionaryNX", signature="a{nx}", replySignature="a{nx}")
    public Map<Short, Long> dictionaryNX(Map<Short, Long> aenx) throws BusException;

    @BusMethod(name="DictionaryNY", signature="a{ny}", replySignature="a{ny}")
    public Map<Short, Byte> dictionaryNY(Map<Short, Byte> aeny) throws BusException;

    @BusMethod(name="DictionaryNU", signature="a{nu}", replySignature="a{nu}")
    public Map<Short, Integer> dictionaryNU(Map<Short, Integer> aenu) throws BusException;

    @BusMethod(name="DictionaryNG", signature="a{ng}", replySignature="a{ng}")
    public Map<Short, String> dictionaryNG(Map<Short, String> aeng) throws BusException;

    @BusMethod(name="DictionaryNI", signature="a{ni}", replySignature="a{ni}")
    public Map<Short, Integer> dictionaryNI(Map<Short, Integer> aeni) throws BusException;

    @BusMethod(name="DictionaryNT", signature="a{nt}", replySignature="a{nt}")
    public Map<Short, Long> dictionaryNT(Map<Short, Long> aent) throws BusException;

    @BusMethod(name="DictionaryNN", signature="a{nn}", replySignature="a{nn}")
    public Map<Short, Short> dictionaryNN(Map<Short, Short> aenn) throws BusException;

    @BusMethod(name="DictionaryNV", signature="a{nv}", replySignature="a{nv}")
    public Map<Short, Variant> dictionaryNV(Map<Short, Variant> aenv) throws BusException;

    @BusMethod(name="DictionaryNS", signature="a{ns}", replySignature="a{ns}")
    public Map<Short, String> dictionaryNS(Map<Short, String> aens) throws BusException;

    @BusMethod(name="DictionaryNAY", signature="a{nay}", replySignature="a{nay}")
    public Map<Short, byte[]> dictionaryNAY(Map<Short, byte[]> aenay) throws BusException;

    @BusMethod(name="DictionaryNQ", signature="a{nq}", replySignature="a{nq}")
    public Map<Short, Short> dictionaryNQ(Map<Short, Short> aenq) throws BusException;

    @BusMethod(name="DictionaryNB", signature="a{nb}", replySignature="a{nb}")
    public Map<Short, Boolean> dictionaryNB(Map<Short, Boolean> aenb) throws BusException;

    @BusMethod(name="DictionaryNO", signature="a{no}", replySignature="a{no}")
    public Map<Short, String> dictionaryNO(Map<Short, String> aeno) throws BusException;

    @BusMethod(name="DictionaryDAESS", signature="a{da{ss}}", replySignature="a{da{ss}}")
    public Map<Double, Map<String, String>> dictionaryDAESS(Map<Double, Map<String, String>> aedaess) throws BusException;

    @BusMethod(name="DictionaryDR", signature="a{dr}", replySignature="a{dr}")
    public Map<Double, AnnotatedTypesInterface.InnerStruct> annotatedDictionaryDR(Map<Double, AnnotatedTypesInterface.InnerStruct> aedr) throws BusException;

    @BusMethod(name="DictionaryDD", signature="a{dd}", replySignature="a{dd}")
    public Map<Double, Double> dictionaryDD(Map<Double, Double> aedd) throws BusException;

    @BusMethod(name="DictionaryDX", signature="a{dx}", replySignature="a{dx}")
    public Map<Double, Long> dictionaryDX(Map<Double, Long> aedx) throws BusException;

    @BusMethod(name="DictionaryDY", signature="a{dy}", replySignature="a{dy}")
    public Map<Double, Byte> dictionaryDY(Map<Double, Byte> aedy) throws BusException;

    @BusMethod(name="DictionaryDU", signature="a{du}", replySignature="a{du}")
    public Map<Double, Integer> dictionaryDU(Map<Double, Integer> aedu) throws BusException;

    @BusMethod(name="DictionaryDG", signature="a{dg}", replySignature="a{dg}")
    public Map<Double, String> dictionaryDG(Map<Double, String> aedg) throws BusException;

    @BusMethod(name="DictionaryDI", signature="a{di}", replySignature="a{di}")
    public Map<Double, Integer> dictionaryDI(Map<Double, Integer> aedi) throws BusException;

    @BusMethod(name="DictionaryDT", signature="a{dt}", replySignature="a{dt}")
    public Map<Double, Long> dictionaryDT(Map<Double, Long> aedt) throws BusException;

    @BusMethod(name="DictionaryDN", signature="a{dn}", replySignature="a{dn}")
    public Map<Double, Short> dictionaryDN(Map<Double, Short> aedn) throws BusException;

    @BusMethod(name="DictionaryDV", signature="a{dv}", replySignature="a{dv}")
    public Map<Double, Variant> dictionaryDV(Map<Double, Variant> aedv) throws BusException;

    @BusMethod(name="DictionaryDS", signature="a{ds}", replySignature="a{ds}")
    public Map<Double, String> dictionaryDS(Map<Double, String> aeds) throws BusException;

    @BusMethod(name="DictionaryDAY", signature="a{day}", replySignature="a{day}")
    public Map<Double, byte[]> dictionaryDAY(Map<Double, byte[]> aeday) throws BusException;

    @BusMethod(name="DictionaryDQ", signature="a{dq}", replySignature="a{dq}")
    public Map<Double, Short> dictionaryDQ(Map<Double, Short> aedq) throws BusException;

    @BusMethod(name="DictionaryDB", signature="a{db}", replySignature="a{db}")
    public Map<Double, Boolean> dictionaryDB(Map<Double, Boolean> aedb) throws BusException;

    @BusMethod(name="DictionaryDO", signature="a{do}", replySignature="a{do}")
    public Map<Double, String> dictionaryDO(Map<Double, String> aedo) throws BusException;

    @BusMethod(name="DictionaryXAESS", signature="a{xa{ss}}", replySignature="a{xa{ss}}")
    public Map<Long, Map<String, String>> dictionaryXAESS(Map<Long, Map<String, String>> aexaess) throws BusException;

    @BusMethod(name="DictionaryXR", signature="a{xr}", replySignature="a{xr}")
    public Map<Long, AnnotatedTypesInterface.InnerStruct> annotatedDictionaryXR(Map<Long, AnnotatedTypesInterface.InnerStruct> aexr) throws BusException;

    @BusMethod(name="DictionaryXD", signature="a{xd}", replySignature="a{xd}")
    public Map<Long, Double> dictionaryXD(Map<Long, Double> aexd) throws BusException;

    @BusMethod(name="DictionaryXX", signature="a{xx}", replySignature="a{xx}")
    public Map<Long, Long> dictionaryXX(Map<Long, Long> aexx) throws BusException;

    @BusMethod(name="DictionaryXY", signature="a{xy}", replySignature="a{xy}")
    public Map<Long, Byte> dictionaryXY(Map<Long, Byte> aexy) throws BusException;

    @BusMethod(name="DictionaryXU", signature="a{xu}", replySignature="a{xu}")
    public Map<Long, Integer> dictionaryXU(Map<Long, Integer> aexu) throws BusException;

    @BusMethod(name="DictionaryXG", signature="a{xg}", replySignature="a{xg}")
    public Map<Long, String> dictionaryXG(Map<Long, String> aexg) throws BusException;

    @BusMethod(name="DictionaryXI", signature="a{xi}", replySignature="a{xi}")
    public Map<Long, Integer> dictionaryXI(Map<Long, Integer> aexi) throws BusException;

    @BusMethod(name="DictionaryXT", signature="a{xt}", replySignature="a{xt}")
    public Map<Long, Long> dictionaryXT(Map<Long, Long> aext) throws BusException;

    @BusMethod(name="DictionaryXN", signature="a{xn}", replySignature="a{xn}")
    public Map<Long, Short> dictionaryXN(Map<Long, Short> aexn) throws BusException;

    @BusMethod(name="DictionaryXV", signature="a{xv}", replySignature="a{xv}")
    public Map<Long, Variant> dictionaryXV(Map<Long, Variant> aexv) throws BusException;

    @BusMethod(name="DictionaryXS", signature="a{xs}", replySignature="a{xs}")
    public Map<Long, String> dictionaryXS(Map<Long, String> aexs) throws BusException;

    @BusMethod(name="DictionaryXAY", signature="a{xay}", replySignature="a{xay}")
    public Map<Long, byte[]> dictionaryXAY(Map<Long, byte[]> aexay) throws BusException;

    @BusMethod(name="DictionaryXQ", signature="a{xq}", replySignature="a{xq}")
    public Map<Long, Short> dictionaryXQ(Map<Long, Short> aexq) throws BusException;

    @BusMethod(signature="a{xb}", replySignature="a{xb}")
    public Map<Long, Boolean> dictionaryXB(Map<Long, Boolean> aexb) throws BusException;

    @BusMethod(name="DictionaryXO", signature="a{xo}", replySignature="a{xo}")
    public Map<Long, String> dictionaryXO(Map<Long, String> aexo) throws BusException;

    @BusMethod(name="DictionarySAESS", signature="a{sa{ss}}", replySignature="a{sa{ss}}")
    public Map<String, Map<String, String>> dictionarySAESS(Map<String, Map<String, String>> aesaess) throws BusException;

    @BusMethod(name="DictionarySR", signature="a{sr}", replySignature="a{sr}")
    public Map<String, AnnotatedTypesInterface.InnerStruct> annotatedDictionarySR(Map<String, AnnotatedTypesInterface.InnerStruct> aesr) throws BusException;

    @BusMethod(name="DictionarySD", signature="a{sd}", replySignature="a{sd}")
    public Map<String, Double> dictionarySD(Map<String, Double> aesd) throws BusException;

    @BusMethod(name="DictionarySX", signature="a{sx}", replySignature="a{sx}")
    public Map<String, Long> dictionarySX(Map<String, Long> aesx) throws BusException;

    @BusMethod(name="DictionarySY", signature="a{sy}", replySignature="a{sy}")
    public Map<String, Byte> dictionarySY(Map<String, Byte> aesy) throws BusException;

    @BusMethod(name="DictionarySU", signature="a{su}", replySignature="a{su}")
    public Map<String, Integer> dictionarySU(Map<String, Integer> aesu) throws BusException;

    @BusMethod(name="DictionarySG", signature="a{sg}", replySignature="a{sg}")
    public Map<String, String> dictionarySG(Map<String, String> aesg) throws BusException;

    @BusMethod(name="DictionarySI", signature="a{si}", replySignature="a{si}")
    public Map<String, Integer> dictionarySI(Map<String, Integer> aesi) throws BusException;

    @BusMethod(name="DictionaryST", signature="a{st}", replySignature="a{st}")
    public Map<String, Long> dictionaryST(Map<String, Long> aest) throws BusException;

    @BusMethod(name="DictionarySN", signature="a{sn}", replySignature="a{sn}")
    public Map<String, Short> dictionarySN(Map<String, Short> aesn) throws BusException;

    @BusMethod(name="DictionarySV", signature="a{sv}", replySignature="a{sv}")
    public Map<String, Variant> dictionarySV(Map<String, Variant> aesv) throws BusException;

    @BusMethod(name="DictionarySS", signature="a{ss}", replySignature="a{ss}")
    public Map<String, String> dictionarySS(Map<String, String> aess) throws BusException;

    @BusMethod(name="DictionarySAY", signature="a{say}", replySignature="a{say}")
    public Map<String, byte[]> dictionarySAY(Map<String, byte[]> aesay) throws BusException;

    @BusMethod(name="DictionarySQ", signature="a{sq}", replySignature="a{sq}")
    public Map<String, Short> dictionarySQ(Map<String, Short> aesq) throws BusException;

    @BusMethod(name="DictionarySB", signature="a{sb}", replySignature="a{sb}")
    public Map<String, Boolean> dictionarySB(Map<String, Boolean> aesb) throws BusException;

    @BusMethod(name="DictionarySO", signature="a{so}", replySignature="a{so}")
    public Map<String, String> DictionarySO(Map<String, String> aeso) throws BusException;

    @BusMethod(signature="a{ya{ss}}", replySignature="a{ya{ss}}")
    public Map<Byte, Map<String, String>> dictionaryYAESS(Map<Byte, Map<String, String>> aeyaess) throws BusException;

    @BusMethod(name="DictionaryYR", signature="a{yr}", replySignature="a{yr}")
    public Map<Byte, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionaryYR(Map<Byte, AnnotatedTypesInterface.InnerStruct> aeyr) throws BusException;

    @BusMethod(name="DictionaryYD", signature="a{yd}", replySignature="a{yd}")
    public Map<Byte, Double> dictionaryYD(Map<Byte, Double> aeyd) throws BusException;

    @BusMethod(name="DictionaryYX", signature="a{yx}", replySignature="a{yx}")
    public Map<Byte, Long> dictionaryYX(Map<Byte, Long> aeyx) throws BusException;

    @BusMethod(name="DictionaryYY", signature="a{yy}", replySignature="a{yy}")
    public Map<Byte, Byte> dictionaryYY(Map<Byte, Byte> aeyy) throws BusException;

    @BusMethod(name="DictionaryYU", signature="a{yu}", replySignature="a{yu}")
    public Map<Byte, Integer> dictionaryYU(Map<Byte, Integer> aeyu) throws BusException;

    @BusMethod(name="DictionaryYG", signature="a{yg}", replySignature="a{yg}")
    public Map<Byte, String> dictionaryYG(Map<Byte, String> aeyg) throws BusException;

    @BusMethod(name="DictionaryYI", signature="a{yi}", replySignature="a{yi}")
    public Map<Byte, Integer> dictionaryYI(Map<Byte, Integer> aeyi) throws BusException;

    @BusMethod(name="DictionaryYT", signature="a{yt}", replySignature="a{yt}")
    public Map<Byte, Long> dictionaryYT(Map<Byte, Long> aeyt) throws BusException;

    @BusMethod(name="DictionaryYN", signature="a{yn}", replySignature="a{yn}")
    public Map<Byte, Short> dictionaryYN(Map<Byte, Short> aeyn) throws BusException;

    @BusMethod(name="DictionaryYV", signature="a{yv}", replySignature="a{yv}")
    public Map<Byte, Variant> dictionaryYV(Map<Byte, Variant> aeyv) throws BusException;

    @BusMethod(name="DictionaryYS", signature="a{ys}", replySignature="a{ys}")
    public Map<Byte, String> dictionaryYS(Map<Byte, String> aeys) throws BusException;

    @BusMethod(name="DictionaryYAY", signature="a{yay}", replySignature="a{yay}")
    public Map<Byte, byte[]> dictionaryYAY(Map<Byte, byte[]> aeyay) throws BusException;

    @BusMethod(name="DictionaryYQ", signature="a{yq}", replySignature="a{yq}")
    public Map<Byte, Short> dictionaryYQ(Map<Byte, Short> aeyq) throws BusException;

    @BusMethod(name="DictionaryYB", signature="a{yb}", replySignature="a{yb}")
    public Map<Byte, Boolean> dictionaryYB(Map<Byte, Boolean> aeyb) throws BusException;

    @BusMethod(name="DictionaryYO", signature="a{yo}", replySignature="a{yo}")
    public Map<Byte, String> dictionaryYO(Map<Byte, String> aeyo) throws BusException;

    @BusMethod(name="DictionaryUAESS", signature="a{ua{ss}}", replySignature="a{ua{ss}}")
    public Map<Integer, Map<String, String>> dictionaryUAESS(Map<Integer, Map<String, String>> aeuaess) throws BusException;

    @BusMethod(name="DictionaryUR", signature="a{ur}", replySignature="a{ur}")
    public Map<Integer, AnnotatedTypesInterface.InnerStruct> dictionaryUR(Map<Integer, AnnotatedTypesInterface.InnerStruct> aeur) throws BusException;

    @BusMethod(name="DictionaryUD", signature="a{ud}", replySignature="a{ud}")
    public Map<Integer, Double> dictionaryUD(Map<Integer, Double> aeud) throws BusException;

    @BusMethod(name="DictionaryUX", signature="a{ux}", replySignature="a{ux}")
    public Map<Integer, Long> dictionaryUX(Map<Integer, Long> aeux) throws BusException;

    @BusMethod(name="DictionaryUY", signature="a{uy}", replySignature="a{uy}")
    public Map<Integer, Byte> dictionaryUY(Map<Integer, Byte> aeuy) throws BusException;

    @BusMethod(name="DictionaryUU", signature="a{uu}", replySignature="a{uu}")
    public Map<Integer, Integer> dictionaryUU(Map<Integer, Integer> aeuu) throws BusException;

    @BusMethod(name="DictionaryUG", signature="a{ug}", replySignature="a{ug}")
    public Map<Integer, String> dictionaryUG(Map<Integer, String> aeug) throws BusException;

    @BusMethod(name="DictionaryUI", signature="a{ui}", replySignature="a{ui}")
    public Map<Integer, Integer> dictionaryUI(Map<Integer, Integer> aeui) throws BusException;

    @BusMethod(name="DictionaryUT", signature="a{ut}", replySignature="a{ut}")
    public Map<Integer, Long> dictionaryUT(Map<Integer, Long> aeut) throws BusException;

    @BusMethod(name="DictionaryUN", signature="a{un}", replySignature="a{un}")
    public Map<Integer, Short> dictionaryUN(Map<Integer, Short> aeun) throws BusException;

    @BusMethod(name="DictionaryUV", signature="a{uv}", replySignature="a{uv}")
    public Map<Integer, Variant> dictionaryUV(Map<Integer, Variant> aeuv) throws BusException;

    @BusMethod(name="DictionaryUS", signature="a{us}", replySignature="a{us}")
    public Map<Integer, String> dictionaryUS(Map<Integer, String> aeus) throws BusException;

    @BusMethod(name="DictionaryUAY", signature="a{uay}", replySignature="a{uay}")
    public Map<Integer, byte[]> dictionaryUAY(Map<Integer, byte[]> aeuay) throws BusException;

    @BusMethod(name="DictionaryUQ", signature="a{uq}", replySignature="a{uq}")
    public Map<Integer, Short> dictionaryUQ(Map<Integer, Short> aeuq) throws BusException;

    @BusMethod(name="DictionaryUB", signature="a{ub}", replySignature="a{ub}")
    public Map<Integer, Boolean> dictionaryUB(Map<Integer, Boolean> aeub) throws BusException;

    @BusMethod(name="DictionaryUO", signature="a{uo}", replySignature="a{uo}")
    public Map<Integer, String> dictionaryUO(Map<Integer, String> aeuo) throws BusException;

    @BusMethod(name="DictionaryGAESS", signature="a{ga{ss}}", replySignature="a{ga{ss}}")
    public Map<String, Map<String, String>> dictionaryGAESS(Map<String, Map<String, String>> aegaess) throws BusException;

    @BusMethod(name="DictionaryGR", signature="a{gr}", replySignature="a{gr}")
    public Map<String, AnnotatedTypesInterface.InnerStruct> dictionaryGR(Map<String, AnnotatedTypesInterface.InnerStruct> aegr) throws BusException;

    @BusMethod(name="DictionaryGD", signature="a{gd}", replySignature="a{gd}")
    public Map<String, Double> dictionaryGD(Map<String, Double> aegd) throws BusException;

    @BusMethod(name="DictionaryGX", signature="a{gx}", replySignature="a{gx}")
    public Map<String, Long> dictionaryGX(Map<String, Long> aegx) throws BusException;

    @BusMethod(name="DictionaryGY", signature="a{gy}", replySignature="a{gy}")
    public Map<String, Byte> DictionaryGY(Map<String, Byte> aegy) throws BusException;

    @BusMethod(name="DictionaryGU", signature="a{gu}", replySignature="a{gu}")
    public Map<String, Integer> dictionaryGU(Map<String, Integer> aegu) throws BusException;

    @BusMethod(name="DictionaryGG", signature="a{gg}", replySignature="a{gg}")
    public Map<String, String> dictionaryGG(Map<String, String> aegg) throws BusException;

    @BusMethod(name="DictionaryGI", signature="a{gi}", replySignature="a{gi}")
    public Map<String, Integer> dictionaryGI(Map<String, Integer> aegi) throws BusException;

    @BusMethod(name="DictionaryGT", signature="a{gt}", replySignature="a{gt}")
    public Map<String, Long> dictionaryGT(Map<String, Long> aegt) throws BusException;

    @BusMethod(name="DictionaryGN", signature="a{gn}", replySignature="a{gn}")
    public Map<String, Short> dictionaryGN(Map<String, Short> aegn) throws BusException;

    @BusMethod(name="DictionaryGV", signature="a{gv}", replySignature="a{gv}")
    public Map<String, Variant> dictionaryGV(Map<String, Variant> aegv) throws BusException;

    @BusMethod(name="DictionaryGS", signature="a{gs}", replySignature="a{gs}")
    public Map<String, String> dictionaryGS(Map<String, String> aegs) throws BusException;

    @BusMethod(name="DictionaryGAY", signature="a{gay}", replySignature="a{gay}")
    public Map<String, byte[]> dictionaryGAY(Map<String, byte[]> aegay) throws BusException;

    @BusMethod(name="DictionaryGQ", signature="a{gq}", replySignature="a{gq}")
    public Map<String, Short> dictionaryGQ(Map<String, Short> aegq) throws BusException;

    @BusMethod(name="DictionaryGB", signature="a{gb}", replySignature="a{gb}")
    public Map<String, Boolean> dictionaryGB(Map<String, Boolean> aegb) throws BusException;

    @BusMethod(name="DictionaryGO", signature="a{go}", replySignature="a{go}")
    public Map<String, String> dictionaryGO(Map<String, String> aego) throws BusException;

    @BusMethod(name="DictionaryBAESS", signature="a{ba{ss}}", replySignature="a{ba{ss}}")
    public Map<Boolean, Map<String, String>> dictionaryBAESS(Map<Boolean, Map<String, String>> aebaess) throws BusException;

    @BusMethod(name="DictionaryBR", signature="a{br}", replySignature="a{br}")
    public Map<Boolean, AnnotatedTypesInterface.InnerStruct> annotatedDictionaryBR(Map<Boolean, AnnotatedTypesInterface.InnerStruct> aebr) throws BusException;

    @BusMethod(name="DictionaryBD", signature="a{bd}", replySignature="a{bd}")
    public Map<Boolean, Double> dictionaryBD(Map<Boolean, Double> aebd) throws BusException;

    @BusMethod(name="DictionaryBX", signature="a{bx}", replySignature="a{bx}")
    public Map<Boolean, Long> dictionaryBX(Map<Boolean, Long> aebx) throws BusException;

    @BusMethod(name="DictionaryBY", signature="a{by}", replySignature="a{by}")
    public Map<Boolean, Byte> dictionaryBY(Map<Boolean, Byte> aeby) throws BusException;

    @BusMethod(name="DictionaryBU", signature="a{bu}", replySignature="a{bu}")
    public Map<Boolean, Integer> dictionaryBU(Map<Boolean, Integer> aebu) throws BusException;

    @BusMethod(name="DictionaryBG", signature="a{bg}", replySignature="a{bg}")
    public Map<Boolean, String> dictionaryBG(Map<Boolean, String> aebg) throws BusException;

    @BusMethod(name="DictionaryBI", signature="a{bi}", replySignature="a{bi}")
    public Map<Boolean, Integer> dictionaryBI(Map<Boolean, Integer> aebi) throws BusException;

    @BusMethod(name="DictionaryBT", signature="a{bt}", replySignature="a{bt}")
    public Map<Boolean, Long> dictionaryBT(Map<Boolean, Long> aebt) throws BusException;

    @BusMethod(name="DictionaryBN", signature="a{bn}", replySignature="a{bn}")
    public Map<Boolean, Short> dictionaryBN(Map<Boolean, Short> aebn) throws BusException;

    @BusMethod(name="DictionaryBV", signature="a{bv}", replySignature="a{bv}")
    public Map<Boolean, Variant> dictionaryBV(Map<Boolean, Variant> aebv) throws BusException;

    @BusMethod(name="DictionaryBS", signature="a{bs}", replySignature="a{bs}")
    public Map<Boolean, String> dictionaryBS(Map<Boolean, String> aebs) throws BusException;

    @BusMethod(name="DictionaryBAY", signature="a{bay}", replySignature="a{bay}")
    public Map<Boolean, byte[]> dictionaryBAY(Map<Boolean, byte[]> aebay) throws BusException;

    @BusMethod(name="DictionaryBQ", signature="a{bq}", replySignature="a{bq}")
    public Map<Boolean, Short> dictionaryBQ(Map<Boolean, Short> aebq) throws BusException;

    @BusMethod(name="DictionaryBB", signature="a{bb}", replySignature="a{bb}")
    public Map<Boolean, Boolean> dictionaryBB(Map<Boolean, Boolean> aebb) throws BusException;

    @BusMethod(name="DictionaryBO", signature="a{bo}", replySignature="a{bo}")
    public Map<Boolean, String> dictionaryBO(Map<Boolean, String> aebo) throws BusException;

    @BusMethod(name="DictionaryQAESS", signature="a{qa{ss}}", replySignature="a{qa{ss}}")
    public Map<Short, Map<String, String>> dictionaryQAESS(Map<Short, Map<String, String>> aeqaess) throws BusException;

    @BusMethod(name="DictionaryQR", signature="a{qr}", replySignature="a{qr}")
    public Map<Short, AnnotatedTypesInterface.InnerStruct> dictionaryQR(Map<Short, AnnotatedTypesInterface.InnerStruct> aeqr) throws BusException;

    @BusMethod(name="DictionaryQD", signature="a{qd}", replySignature="a{qd}")
    public Map<Short, Double> dictionaryQD(Map<Short, Double> aeqd) throws BusException;

    @BusMethod(name="DictionaryQX", signature="a{qx}", replySignature="a{qx}")
    public Map<Short, Long> dictionaryQX(Map<Short, Long> aeqx) throws BusException;

    @BusMethod(name="DictionaryQY", signature="a{qy}", replySignature="a{qy}")
    public Map<Short, Byte> dictionaryQY(Map<Short, Byte> aeqy) throws BusException;

    @BusMethod(name="DictionaryQU", signature="a{qu}", replySignature="a{qu}")
    public Map<Short, Integer> dictionaryQU(Map<Short, Integer> aequ) throws BusException;

    @BusMethod(name="DictionaryQG", signature="a{qg}", replySignature="a{qg}")
    public Map<Short, String> dictionaryQG(Map<Short, String> aeqg) throws BusException;

    @BusMethod(name="DictionaryQI", signature="a{qi}", replySignature="a{qi}")
    public Map<Short, Integer> dictionaryQI(Map<Short, Integer> aeqi) throws BusException;

    @BusMethod(name="DictionaryQT", signature="a{qt}", replySignature="a{qt}")
    public Map<Short, Long> dictionaryQT(Map<Short, Long> aeqt) throws BusException;

    @BusMethod(name="DictionaryQN", signature="a{qn}", replySignature="a{qn}")
    public Map<Short, Short> dictionaryQN(Map<Short, Short> aeqn) throws BusException;

    @BusMethod(name="DictionaryQV", signature="a{qv}", replySignature="a{qv}")
    public Map<Short, Variant> dictionaryQV(Map<Short, Variant> aeqv) throws BusException;

    @BusMethod(name="DictionaryQS", signature="a{qs}", replySignature="a{qs}")
    public Map<Short, String> dictionaryQS(Map<Short, String> aeqs) throws BusException;

    @BusMethod(name="DictionaryQAY", signature="a{qay}", replySignature="a{qay}")
    public Map<Short, byte[]> dictionaryQAY(Map<Short, byte[]> aeqay) throws BusException;

    @BusMethod(name="DictionaryQQ", signature="a{qq}", replySignature="a{qq}")
    public Map<Short, Short> dictionaryQQ(Map<Short, Short> aeqq) throws BusException;

    @BusMethod(name="DictionaryQB", signature="a{qb}", replySignature="a{qb}")
    public Map<Short, Boolean> dictionaryQB(Map<Short, Boolean> aeqb) throws BusException;

    @BusMethod(name="DictionaryQO", signature="a{qo}", replySignature="a{qo}")
    public Map<Short, String> dictionaryQO(Map<Short, String> aeqo) throws BusException;

    @BusMethod(name="DictionaryOAESS", signature="a{oa{ss}}", replySignature="a{oa{ss}}")
    public Map<String, Map<String, String>> dictionaryOAESS(Map<String, Map<String, String>> aeoaess) throws BusException;

    @BusMethod(name="DictionaryOR", signature="a{or}", replySignature="a{or}")
    public Map<String, AnnotatedTypesInterface.InnerStruct> dictionaryOR(Map<String, AnnotatedTypesInterface.InnerStruct> aeor) throws BusException;

    @BusMethod(name="DictionaryOD", signature="a{od}", replySignature="a{od}")
    public Map<String, Double> dictionaryOD(Map<String, Double> aeod) throws BusException;

    @BusMethod(name="DictionaryOX", signature="a{ox}", replySignature="a{ox}")
    public Map<String, Long> dictionaryOX(Map<String, Long> aeox) throws BusException;

    @BusMethod(name="DictionaryOY", signature="a{oy}", replySignature="a{oy}")
    public Map<String, Byte> dictionaryOY(Map<String, Byte> aeoy) throws BusException;

    @BusMethod(name="DictionaryOU", signature="a{ou}", replySignature="a{ou}")
    public Map<String, Integer> dictionaryOU(Map<String, Integer> aeou) throws BusException;

    @BusMethod(name="DictionaryOG", signature="a{og}", replySignature="a{og}")
    public Map<String, String> dictionaryOG(Map<String, String> aeog) throws BusException;

    @BusMethod(name="DictionaryOI", signature="a{oi}", replySignature="a{oi}")
    public Map<String, Integer> dictionaryOI(Map<String, Integer> aeoi) throws BusException;

    @BusMethod(name="DictionaryOT", signature="a{ot}", replySignature="a{ot}")
    public Map<String, Long> dictionaryOT(Map<String, Long> aeot) throws BusException;

    @BusMethod(name="DictionaryON", signature="a{on}", replySignature="a{on}")
    public Map<String, Short> dictionaryON(Map<String, Short> aeon) throws BusException;

    @BusMethod(name="DictionaryOV", signature="a{ov}", replySignature="a{ov}")
    public Map<String, Variant> dictionaryOV(Map<String, Variant> aeov) throws BusException;

    @BusMethod(name="DictionaryOS", signature="a{os}", replySignature="a{os}")
    public Map<String, String> dictionaryOS(Map<String, String> aeos) throws BusException;

    @BusMethod(name="DictionaryOAY", signature="a{oay}", replySignature="a{oay}")
    public Map<String, byte[]> dictionaryOAY(Map<String, byte[]> aeoay) throws BusException;

    @BusMethod(name="DictionaryOQ", signature="a{oq}", replySignature="a{oq}")
    public Map<String, Short> dictionaryOQ(Map<String, Short> aeoq) throws BusException;

    @BusMethod(name="DictionaryOB", signature="a{ob}", replySignature="a{ob}")
    public Map<String, Boolean> dictionaryOB(Map<String, Boolean> aeob) throws BusException;

    @BusMethod(name="DictionaryOO", signature="a{oo}", replySignature="a{oo}")
    public Map<String, String> dictionaryOO(Map<String, String> aeoo) throws BusException;

    @BusMethod(name="dictionaryIAESS", signature="a{ia{ss}}", replySignature="a{ia{ss}}")
    public Map<Integer, Map<String, String>> dictionaryIAESS(Map<Integer, Map<String, String>> aeiaess) throws BusException;

    @BusMethod(name="DictionaryIR", signature="a{ir}", replySignature="a{ir}")
    public Map<Integer, AnnotatedTypesInterface.InnerStruct> annotatedDictionaryIR(Map<Integer, AnnotatedTypesInterface.InnerStruct> aeir) throws BusException;

    @BusMethod(name="DictionaryID", signature="a{id}", replySignature="a{id}")
    public Map<Integer, Double> dictionaryID(Map<Integer, Double> aeid) throws BusException;

    @BusMethod(name="DictionaryIX", signature="a{ix}", replySignature="a{ix}")
    public Map<Integer, Long> dictionaryIX(Map<Integer, Long> aeix) throws BusException;

    @BusMethod(name="DictionaryIY", signature="a{iy}", replySignature="a{iy}")
    public Map<Integer, Byte> dictionaryIY(Map<Integer, Byte> aeiy) throws BusException;

    @BusMethod(name="DictionaryIU", signature="a{iu}", replySignature="a{iu}")
    public Map<Integer, Integer> dictionaryIU(Map<Integer, Integer> aeiu) throws BusException;

    @BusMethod(name="DictionaryIG", signature="a{ig}", replySignature="a{ig}")
    public Map<Integer, String> dictionaryIG(Map<Integer, String> aeig) throws BusException;

    @BusMethod(name="DictionaryII", signature="a{ii}", replySignature="a{ii}")
    public Map<Integer, Integer> dictionaryII(Map<Integer, Integer> aeii) throws BusException;

    @BusMethod(name="DictionaryIT", signature="a{it}", replySignature="a{it}")
    public Map<Integer, Long> dictionaryIT(Map<Integer, Long> aeit) throws BusException;

    @BusMethod(name="DictionaryIN", signature="a{in}", replySignature="a{in}")
    public Map<Integer, Short> dictionaryIN(Map<Integer, Short> aein) throws BusException;

    @BusMethod(name="DictionaryIV", signature="a{iv}", replySignature="a{iv}")
    public Map<Integer, Variant> dictionaryIV(Map<Integer, Variant> aeiv) throws BusException;

    @BusMethod(name="DictionaryIS", signature="a{is}", replySignature="a{is}")
    public Map<Integer, String> dictionaryIS(Map<Integer, String> aeis) throws BusException;

    @BusMethod(name="DictionaryIAY", signature="a{iay}", replySignature="a{iay}")
    public Map<Integer, byte[]> dictionaryIAY(Map<Integer, byte[]> aeiay) throws BusException;

    @BusMethod(name="DictionaryIQ", signature="a{iq}", replySignature="a{iq}")
    public Map<Integer, Short> dictionaryIQ(Map<Integer, Short> aeiq) throws BusException;

    @BusMethod(name="DictionaryIB", signature="a{ib}", replySignature="a{ib}")
    public Map<Integer, Boolean> dictionaryIB(Map<Integer, Boolean> aeib) throws BusException;

    @BusMethod(name="DictionaryIO", signature="a{io}", replySignature="a{io}")
    public Map<Integer, String> dictionaryIO(Map<Integer, String> aeio) throws BusException;

    @BusMethod(name="DictionaryTAESS", signature="a{ta{ss}}", replySignature="a{ta{ss}}")
    public Map<Long, Map<String, String>> dictionaryTAESS(Map<Long, Map<String, String>> aetaess) throws BusException;

    @BusMethod(name="DictionaryTR", signature="a{tr}", replySignature="a{tr}")
    public Map<Long, AnnotatedTypesInterface.InnerStruct> dictionaryTR(Map<Long, AnnotatedTypesInterface.InnerStruct> aetr) throws BusException;

    @BusMethod(name="DictionaryTD", signature="a{td}", replySignature="a{td}")
    public Map<Long, Double> dictionaryTD(Map<Long, Double> aetd) throws BusException;

    @BusMethod(name="DictionaryTX", signature="a{tx}", replySignature="a{tx}")
    public Map<Long, Long> dictionaryTX(Map<Long, Long> aetx) throws BusException;

    @BusMethod(name="DictionaryTY", signature="a{ty}", replySignature="a{ty}")
    public Map<Long, Byte> dictionaryTY(Map<Long, Byte> aety) throws BusException;

    @BusMethod(name="DictionaryTU", signature="a{tu}", replySignature="a{tu}")
    public Map<Long, Integer> dictionaryTU(Map<Long, Integer> aetu) throws BusException;

    @BusMethod(name="DictionaryTG", signature="a{tg}", replySignature="a{tg}")
    public Map<Long, String> dictionaryTG(Map<Long, String> aetg) throws BusException;

    @BusMethod(name="DictionaryTI", signature="a{ti}", replySignature="a{ti}")
    public Map<Long, Integer> dictionaryTI(Map<Long, Integer> aeti) throws BusException;

    @BusMethod(name="DictionaryTT", signature="a{tt}", replySignature="a{tt}")
    public Map<Long, Long> dictionaryTT(Map<Long, Long> aett) throws BusException;

    @BusMethod(name="DictionaryTN", signature="a{tn}", replySignature="a{tn}")
    public Map<Long, Short> dictionaryTN(Map<Long, Short> aetn) throws BusException;

    @BusMethod(name="DictionaryTV", signature="a{tv}", replySignature="a{tv}")
    public Map<Long, Variant> dictionaryTV(Map<Long, Variant> aetv) throws BusException;

    @BusMethod(name="DictionaryTS", signature="a{ts}", replySignature="a{ts}")
    public Map<Long, String> dictionaryTS(Map<Long, String> aets) throws BusException;

    @BusMethod(name="DictionaryTAY", signature="a{tay}", replySignature="a{tay}")
    public Map<Long, byte[]> dictionaryTAY(Map<Long, byte[]> aetay) throws BusException;

    @BusMethod(name="DictionaryTQ", signature="a{tq}", replySignature="a{tq}")
    public Map<Long, Short> dictionaryTQ(Map<Long, Short> aetq) throws BusException;

    @BusMethod(name="DictionaryTB", signature="a{tb}", replySignature="a{tb}")
    public Map<Long, Boolean> dictionaryTB(Map<Long, Boolean> aetb) throws BusException;

    @BusMethod(name="DictionaryTO", signature="a{to}", replySignature="a{to}")
    public Map<Long, String> dictionaryTO(Map<Long, String> aeto) throws BusException;

    @BusProperty(signature="a{ss}")
    public Map<String, String> getDictionarySS() throws BusException;

    @BusProperty(signature="a{ss}")
    public void setDictionarySS(Map<String, String> aess) throws BusException;

    public enum EnumType {
        Enum0;
    };

    @BusMethod(name="EnumY", signature="y", replySignature="y")
    public EnumType enumY(EnumType y) throws BusException;

    @BusMethod(name="EnumN", signature="n", replySignature="n")
    public EnumType enumN(EnumType n) throws BusException;

    @BusMethod(name="EnumQ", signature="q", replySignature="q")
    public EnumType enumQ(EnumType q) throws BusException;

    @BusMethod(name="EnumI", signature="i", replySignature="i")
    public EnumType enumI(EnumType i) throws BusException;

    @BusMethod(name="EnumU", signature="u", replySignature="u")
    public EnumType enumU(EnumType u) throws BusException;

    @BusMethod(name="EnumX", signature="x", replySignature="x")
    public EnumType enumX(EnumType x) throws BusException;

    @BusMethod(name="EnumT", signature="t", replySignature="t")
    public EnumType enumT(EnumType t) throws BusException;
}
