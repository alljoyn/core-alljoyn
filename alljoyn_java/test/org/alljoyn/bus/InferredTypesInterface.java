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
import org.alljoyn.bus.annotation.Position;

import java.util.Map;
import java.util.TreeMap;
import java.util.Arrays;

@BusInterface
public interface InferredTypesInterface {

    public class InnerStruct {
        @Position(0) public int i;

        public InnerStruct() { i = 0; }
        public InnerStruct(int i) { this.i = i; }
        public boolean equals(Object obj) { return i == ((InnerStruct)obj).i; }
    }

    public class Struct {
        @Position(0) public byte y;
        @Position(1) public boolean b;
        @Position(2) public short n;
        @Position(3) public int i;
        @Position(4) public long x;
        @Position(5) public double d;
        @Position(6) public String s;
        @Position(7) public byte[] ay;
        @Position(8) public boolean[] ab;
        @Position(9) public short[] an;
        @Position(10) public int[] ai;
        @Position(11) public long[] ax;
        @Position(12) public double[] ad;
        @Position(13) public String[] as;
        @Position(14) public InnerStruct r;
        @Position(15) public TreeMap<String, String> ae;
        @Position(16) public Variant v;

        public Struct() {}
        public Struct(byte y, boolean b, short n, int i, long x, double d, String s,
                      byte[] ay, boolean[] ab, short[] an, int[] ai, long[] ax, double[] ad, String[] as,
                      InnerStruct r, Variant v, TreeMap<String, String> ae) {
            this.y = y;
            this.b = b;
            this.n = n;
            this.i = i;
            this.x = x;
            this.d = d;
            this.s = s;
            this.ay = ay;
            this.ab = ab;
            this.an = an;
            this.ai = ai;
            this.ax = ax;
            this.ad = ad;
            this.as = as;
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
                i == struct.i &&
                x == struct.x &&
                d == struct.d &&
                s.equals(struct.s) &&
                Arrays.equals(ay, struct.ay) &&
                Arrays.equals(ab, struct.ab) &&
                Arrays.equals(an, struct.an) &&
                Arrays.equals(ai, struct.ai) &&
                Arrays.equals(ax, struct.ax) &&
                Arrays.equals(ad, struct.ad) &&
                Arrays.equals(as, struct.as) &&
                r.equals(struct.r) &&
                v.equals(struct.v) &&
                ae.equals(struct.ae);
        }
    }

    @BusMethod
    public void Void() throws BusException;

    @BusMethod
    public byte Byte(byte y) throws BusException;

    @BusMethod
    public boolean Boolean(boolean b) throws BusException;

    @BusMethod
    public short Int16(short n) throws BusException;

    @BusMethod
    public int Int32(int i) throws BusException;

    @BusMethod
    public long Int64(long x) throws BusException;

    @BusMethod
    public double Double(double d) throws BusException;

    @BusMethod
    public String String(String s) throws BusException;

    @BusMethod
    public byte[] ByteArray(byte[] ay) throws BusException;

    @BusMethod
    public boolean[] BooleanArray(boolean[] ab) throws BusException;

    @BusMethod
    public Boolean[] CapitalBooleanArray(Boolean[] aB) throws BusException;

    @BusMethod
    public short[] Int16Array(short[] an) throws BusException;

    @BusMethod
    public int[] Int32Array(int[] ai) throws BusException;

    @BusMethod
    public long[] Int64Array(long[] ax) throws BusException;

    @BusMethod
    public double[] DoubleArray(double[] ad) throws BusException;

    @BusMethod
    public String[] StringArray(String[] as) throws BusException;

    @BusMethod
    public byte[][] ArrayArray(byte[][] aay) throws BusException;

    @BusMethod(name="StructArray")
    public InnerStruct[] InferredStructArray(InnerStruct[] ar) throws BusException;

    @BusMethod
    public Variant[] VariantArray(Variant[] av) throws BusException;

    @BusMethod
    public Map<String, String>[] DictionaryArray(Map<String, String>[] aaess) throws BusException;

    @BusMethod(name="Struct")
    public Struct InferredStruct(Struct r) throws BusException;

    @BusMethod
    public Variant Variant(Variant v) throws BusException;

    @BusMethod
    public Map<Byte, Byte> DictionaryYY(Map<Byte, Byte> aeyy) throws BusException;

    @BusMethod
    public Map<Byte, Boolean> DictionaryYB(Map<Byte, Boolean> aeyb) throws BusException;

    @BusMethod
    public Map<Byte, Short> DictionaryYN(Map<Byte, Short> aeyn) throws BusException;

    @BusMethod
    public Map<Byte, Integer> DictionaryYI(Map<Byte, Integer> aeyi) throws BusException;

    @BusMethod
    public Map<Byte, Long> DictionaryYX(Map<Byte, Long> aeyx) throws BusException;

    @BusMethod
    public Map<Byte, Double> DictionaryYD(Map<Byte, Double> aeyd) throws BusException;

    @BusMethod
    public Map<Byte, String> DictionaryYS(Map<Byte, String> aeys) throws BusException;

    @BusMethod
    public Map<Byte, byte[]> DictionaryYAY(Map<Byte, byte[]> aeyay) throws BusException;

    @BusMethod(name="DictionaryYR")
    public Map<Byte, InferredTypesInterface.InnerStruct> InferredDictionaryYR(Map<Byte, InferredTypesInterface.InnerStruct> aeyr) throws BusException;

    @BusMethod
    public Map<Byte, Variant> DictionaryYV(Map<Byte, Variant> aeyv) throws BusException;

    @BusMethod
    public Map<Byte, Map<String, String>> DictionaryYAESS(Map<Byte, Map<String, String>> aeyaess) throws BusException;

    @BusMethod
    public Map<Boolean, Byte> DictionaryBY(Map<Boolean, Byte> aeby) throws BusException;

    @BusMethod
    public Map<Boolean, Boolean> DictionaryBB(Map<Boolean, Boolean> aebb) throws BusException;

    @BusMethod
    public Map<Boolean, Short> DictionaryBN(Map<Boolean, Short> aebn) throws BusException;

    @BusMethod
    public Map<Boolean, Integer> DictionaryBI(Map<Boolean, Integer> aebi) throws BusException;

    @BusMethod
    public Map<Boolean, Long> DictionaryBX(Map<Boolean, Long> aebx) throws BusException;

    @BusMethod
    public Map<Boolean, Double> DictionaryBD(Map<Boolean, Double> aebd) throws BusException;

    @BusMethod
    public Map<Boolean, String> DictionaryBS(Map<Boolean, String> aebs) throws BusException;

    @BusMethod
    public Map<Boolean, byte[]> DictionaryBAY(Map<Boolean, byte[]> aebay) throws BusException;

    @BusMethod(name="DictionaryBR")
    public Map<Boolean, InferredTypesInterface.InnerStruct> InferredDictionaryBR(Map<Boolean, InferredTypesInterface.InnerStruct> aebr) throws BusException;

    @BusMethod
    public Map<Boolean, Variant> DictionaryBV(Map<Boolean, Variant> aebv) throws BusException;

    @BusMethod
    public Map<Boolean, Map<String, String>> DictionaryBAESS(Map<Boolean, Map<String, String>> aebaess) throws BusException;

    @BusMethod
    public Map<Short, Byte> DictionaryNY(Map<Short, Byte> aeny) throws BusException;

    @BusMethod
    public Map<Short, Boolean> DictionaryNB(Map<Short, Boolean> aenb) throws BusException;

    @BusMethod
    public Map<Short, Short> DictionaryNN(Map<Short, Short> aenn) throws BusException;

    @BusMethod
    public Map<Short, Integer> DictionaryNI(Map<Short, Integer> aeni) throws BusException;

    @BusMethod
    public Map<Short, Long> DictionaryNX(Map<Short, Long> aenx) throws BusException;

    @BusMethod
    public Map<Short, Double> DictionaryND(Map<Short, Double> aend) throws BusException;

    @BusMethod
    public Map<Short, String> DictionaryNS(Map<Short, String> aens) throws BusException;

    @BusMethod
    public Map<Short, byte[]> DictionaryNAY(Map<Short, byte[]> aenay) throws BusException;

    @BusMethod(name="DictionaryNR")
    public Map<Short, InferredTypesInterface.InnerStruct> InferredDictionaryNR(Map<Short, InferredTypesInterface.InnerStruct> aenr) throws BusException;

    @BusMethod
    public Map<Short, Variant> DictionaryNV(Map<Short, Variant> aenv) throws BusException;

    @BusMethod
    public Map<Short, Map<String, String>> DictionaryNAESS(Map<Short, Map<String, String>> aenaess) throws BusException;

    @BusMethod
    public Map<Integer, Byte> DictionaryIY(Map<Integer, Byte> aeiy) throws BusException;

    @BusMethod
    public Map<Integer, Boolean> DictionaryIB(Map<Integer, Boolean> aeib) throws BusException;

    @BusMethod
    public Map<Integer, Short> DictionaryIN(Map<Integer, Short> aein) throws BusException;

    @BusMethod
    public Map<Integer, Integer> DictionaryII(Map<Integer, Integer> aeii) throws BusException;

    @BusMethod
    public Map<Integer, Long> DictionaryIX(Map<Integer, Long> aeix) throws BusException;

    @BusMethod
    public Map<Integer, Double> DictionaryID(Map<Integer, Double> aeid) throws BusException;

    @BusMethod
    public Map<Integer, String> DictionaryIS(Map<Integer, String> aeis) throws BusException;

    @BusMethod
    public Map<Integer, byte[]> DictionaryIAY(Map<Integer, byte[]> aeiay) throws BusException;

    @BusMethod(name="DictionaryIR")
    public Map<Integer, InferredTypesInterface.InnerStruct> InferredDictionaryIR(Map<Integer, InferredTypesInterface.InnerStruct> aeir) throws BusException;

    @BusMethod
    public Map<Integer, Variant> DictionaryIV(Map<Integer, Variant> aeiv) throws BusException;

    @BusMethod
    public Map<Integer, Map<String, String>> DictionaryIAESS(Map<Integer, Map<String, String>> aeiaess) throws BusException;

    @BusMethod
    public Map<Long, Byte> DictionaryXY(Map<Long, Byte> aexy) throws BusException;

    @BusMethod
    public Map<Long, Boolean> DictionaryXB(Map<Long, Boolean> aexb) throws BusException;

    @BusMethod
    public Map<Long, Short> DictionaryXN(Map<Long, Short> aexn) throws BusException;

    @BusMethod
    public Map<Long, Integer> DictionaryXI(Map<Long, Integer> aexi) throws BusException;

    @BusMethod
    public Map<Long, Long> DictionaryXX(Map<Long, Long> aexx) throws BusException;

    @BusMethod
    public Map<Long, Double> DictionaryXD(Map<Long, Double> aexd) throws BusException;

    @BusMethod
    public Map<Long, String> DictionaryXS(Map<Long, String> aexs) throws BusException;

    @BusMethod
    public Map<Long, byte[]> DictionaryXAY(Map<Long, byte[]> aexay) throws BusException;

    @BusMethod(name="DictionaryXR")
    public Map<Long, InferredTypesInterface.InnerStruct> InferredDictionaryXR(Map<Long, InferredTypesInterface.InnerStruct> aexr) throws BusException;

    @BusMethod
    public Map<Long, Variant> DictionaryXV(Map<Long, Variant> aexv) throws BusException;

    @BusMethod
    public Map<Long, Map<String, String>> DictionaryXAESS(Map<Long, Map<String, String>> aexaess) throws BusException;

    @BusMethod
    public Map<Double, Byte> DictionaryDY(Map<Double, Byte> aedy) throws BusException;

    @BusMethod
    public Map<Double, Boolean> DictionaryDB(Map<Double, Boolean> aedb) throws BusException;

    @BusMethod
    public Map<Double, Short> DictionaryDN(Map<Double, Short> aedn) throws BusException;

    @BusMethod
    public Map<Double, Integer> DictionaryDI(Map<Double, Integer> aedi) throws BusException;

    @BusMethod
    public Map<Double, Long> DictionaryDX(Map<Double, Long> aedx) throws BusException;

    @BusMethod
    public Map<Double, Double> DictionaryDD(Map<Double, Double> aedd) throws BusException;

    @BusMethod
    public Map<Double, String> DictionaryDS(Map<Double, String> aeds) throws BusException;

    @BusMethod
    public Map<Double, byte[]> DictionaryDAY(Map<Double, byte[]> aeday) throws BusException;

    @BusMethod(name="DictionaryDR")
    public Map<Double, InferredTypesInterface.InnerStruct> InferredDictionaryDR(Map<Double, InferredTypesInterface.InnerStruct> aedr) throws BusException;

    @BusMethod
    public Map<Double, Variant> DictionaryDV(Map<Double, Variant> aedv) throws BusException;

    @BusMethod
    public Map<Double, Map<String, String>> DictionaryDAESS(Map<Double, Map<String, String>> aedaess) throws BusException;

    @BusMethod
    public Map<String, Byte> DictionarySY(Map<String, Byte> aesy) throws BusException;

    @BusMethod
    public Map<String, Boolean> DictionarySB(Map<String, Boolean> aesb) throws BusException;

    @BusMethod
    public Map<String, Short> DictionarySN(Map<String, Short> aesn) throws BusException;

    @BusMethod
    public Map<String, Integer> DictionarySI(Map<String, Integer> aesi) throws BusException;

    @BusMethod
    public Map<String, Long> DictionarySX(Map<String, Long> aesx) throws BusException;

    @BusMethod
    public Map<String, Double> DictionarySD(Map<String, Double> aesd) throws BusException;

    @BusMethod
    public Map<String, String> DictionarySS(Map<String, String> aess) throws BusException;

    @BusMethod
    public Map<String, byte[]> DictionarySAY(Map<String, byte[]> aesay) throws BusException;

    @BusMethod(name="DictionarySR")
    public Map<String, InferredTypesInterface.InnerStruct> InferredDictionarySR(Map<String, InferredTypesInterface.InnerStruct> aesr) throws BusException;

    @BusMethod
    public Map<String, Variant> DictionarySV(Map<String, Variant> aesv) throws BusException;

    @BusMethod
    public Map<String, Map<String, String>> DictionarySAESS(Map<String, Map<String, String>> aesaess) throws BusException;

    @BusMethod
    public TreeMap<String, String>[] TreeDictionaryArray(TreeMap<String, String>[] aaess) throws BusException;

    @BusProperty
    public Map<String, String> getDictionarySS() throws BusException;

    @BusProperty
    public void setDictionarySS(Map<String, String> aess) throws BusException;

    public class TwoByteArrays {
        @Position(0) public byte[] ay0;
        @Position(1) public byte[] ay1;
    }

    @BusMethod
    public TwoByteArrays TwoByteArrays(TwoByteArrays rayay) throws BusException;
}
