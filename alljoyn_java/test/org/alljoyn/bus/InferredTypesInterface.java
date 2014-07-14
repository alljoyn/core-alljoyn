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
/*
 * As a general rule AllJoyn interfaces use methods that have capital letters
 * Since this interface is about inferred types we will let the code infer the
 * method name.  For this interface we will break with AllJoyn convention an
 * use Java convention of lower case letters for method names. That will be
 * inferred from the Java method name not from the annotation name value. Except
 * for instances where the tests previously specified a method name using the
 * name annotation. Or the lower case name conflicts with a Java data type.
 */
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

    @BusMethod(name="Void")
    public void voidMethod() throws BusException;

    @BusMethod(name="Byte")
    public byte byteMethod(byte y) throws BusException;

    @BusMethod(name="Boolean")
    public boolean booleanMethod(boolean b) throws BusException;

    @BusMethod
    public short int16(short n) throws BusException;

    @BusMethod
    public int int32(int i) throws BusException;

    @BusMethod
    public long int64(long x) throws BusException;

    @BusMethod(name="Double")
    public double doubleMethod(double d) throws BusException;

    @BusMethod
    public String string(String s) throws BusException;

    @BusMethod
    public byte[] byteArray(byte[] ay) throws BusException;

    @BusMethod
    public boolean[] booleanArray(boolean[] ab) throws BusException;

    @BusMethod
    public Boolean[] capitalBooleanArray(Boolean[] aB) throws BusException;

    @BusMethod
    public short[] int16Array(short[] an) throws BusException;

    @BusMethod
    public int[] int32Array(int[] ai) throws BusException;

    @BusMethod
    public long[] int64Array(long[] ax) throws BusException;

    @BusMethod
    public double[] doubleArray(double[] ad) throws BusException;

    @BusMethod
    public String[] stringArray(String[] as) throws BusException;

    @BusMethod
    public byte[][] arrayArray(byte[][] aay) throws BusException;

    @BusMethod(name="StructArray")
    public InnerStruct[] inferredStructArray(InnerStruct[] ar) throws BusException;

    @BusMethod
    public Variant[] variantArray(Variant[] av) throws BusException;

    @BusMethod
    public Map<String, String>[] dictionaryArray(Map<String, String>[] aaess) throws BusException;

    @BusMethod(name="Struct")
    public Struct inferredStruct(Struct r) throws BusException;

    @BusMethod
    public Variant variant(Variant v) throws BusException;

    @BusMethod
    public Map<Byte, Byte> dictionaryYY(Map<Byte, Byte> aeyy) throws BusException;

    @BusMethod
    public Map<Byte, Boolean> dictionaryYB(Map<Byte, Boolean> aeyb) throws BusException;

    @BusMethod
    public Map<Byte, Short> dictionaryYN(Map<Byte, Short> aeyn) throws BusException;

    @BusMethod
    public Map<Byte, Integer> dictionaryYI(Map<Byte, Integer> aeyi) throws BusException;

    @BusMethod
    public Map<Byte, Long> dictionaryYX(Map<Byte, Long> aeyx) throws BusException;

    @BusMethod
    public Map<Byte, Double> dictionaryYD(Map<Byte, Double> aeyd) throws BusException;

    @BusMethod
    public Map<Byte, String> dictionaryYS(Map<Byte, String> aeys) throws BusException;

    @BusMethod
    public Map<Byte, byte[]> dictionaryYAY(Map<Byte, byte[]> aeyay) throws BusException;

    @BusMethod(name="DictionaryYR")
    public Map<Byte, InferredTypesInterface.InnerStruct> inferredDictionaryYR(Map<Byte, InferredTypesInterface.InnerStruct> aeyr) throws BusException;

    @BusMethod
    public Map<Byte, Variant> dictionaryYV(Map<Byte, Variant> aeyv) throws BusException;

    @BusMethod
    public Map<Byte, Map<String, String>> dictionaryYAESS(Map<Byte, Map<String, String>> aeyaess) throws BusException;

    @BusMethod
    public Map<Boolean, Byte> dictionaryBY(Map<Boolean, Byte> aeby) throws BusException;

    @BusMethod
    public Map<Boolean, Boolean> dictionaryBB(Map<Boolean, Boolean> aebb) throws BusException;

    @BusMethod
    public Map<Boolean, Short> dictionaryBN(Map<Boolean, Short> aebn) throws BusException;

    @BusMethod
    public Map<Boolean, Integer> dictionaryBI(Map<Boolean, Integer> aebi) throws BusException;

    @BusMethod
    public Map<Boolean, Long> dictionaryBX(Map<Boolean, Long> aebx) throws BusException;

    @BusMethod
    public Map<Boolean, Double> dictionaryBD(Map<Boolean, Double> aebd) throws BusException;

    @BusMethod
    public Map<Boolean, String> dictionaryBS(Map<Boolean, String> aebs) throws BusException;

    @BusMethod
    public Map<Boolean, byte[]> dictionaryBAY(Map<Boolean, byte[]> aebay) throws BusException;

    @BusMethod(name="DictionaryBR")
    public Map<Boolean, InferredTypesInterface.InnerStruct> inferredDictionaryBR(Map<Boolean, InferredTypesInterface.InnerStruct> aebr) throws BusException;

    @BusMethod
    public Map<Boolean, Variant> dictionaryBV(Map<Boolean, Variant> aebv) throws BusException;

    @BusMethod
    public Map<Boolean, Map<String, String>> dictionaryBAESS(Map<Boolean, Map<String, String>> aebaess) throws BusException;

    @BusMethod
    public Map<Short, Byte> dictionaryNY(Map<Short, Byte> aeny) throws BusException;

    @BusMethod
    public Map<Short, Boolean> dictionaryNB(Map<Short, Boolean> aenb) throws BusException;

    @BusMethod
    public Map<Short, Short> dictionaryNN(Map<Short, Short> aenn) throws BusException;

    @BusMethod
    public Map<Short, Integer> dictionaryNI(Map<Short, Integer> aeni) throws BusException;

    @BusMethod
    public Map<Short, Long> dictionaryNX(Map<Short, Long> aenx) throws BusException;

    @BusMethod
    public Map<Short, Double> dictionaryND(Map<Short, Double> aend) throws BusException;

    @BusMethod
    public Map<Short, String> dictionaryNS(Map<Short, String> aens) throws BusException;

    @BusMethod
    public Map<Short, byte[]> dictionaryNAY(Map<Short, byte[]> aenay) throws BusException;

    @BusMethod(name="DictionaryNR")
    public Map<Short, InferredTypesInterface.InnerStruct> inferredDictionaryNR(Map<Short, InferredTypesInterface.InnerStruct> aenr) throws BusException;

    @BusMethod
    public Map<Short, Variant> dictionaryNV(Map<Short, Variant> aenv) throws BusException;

    @BusMethod
    public Map<Short, Map<String, String>> dictionaryNAESS(Map<Short, Map<String, String>> aenaess) throws BusException;

    @BusMethod
    public Map<Integer, Byte> dictionaryIY(Map<Integer, Byte> aeiy) throws BusException;

    @BusMethod
    public Map<Integer, Boolean> dictionaryIB(Map<Integer, Boolean> aeib) throws BusException;

    @BusMethod
    public Map<Integer, Short> dictionaryIN(Map<Integer, Short> aein) throws BusException;

    @BusMethod
    public Map<Integer, Integer> dictionaryII(Map<Integer, Integer> aeii) throws BusException;

    @BusMethod
    public Map<Integer, Long> dictionaryIX(Map<Integer, Long> aeix) throws BusException;

    @BusMethod
    public Map<Integer, Double> dictionaryID(Map<Integer, Double> aeid) throws BusException;

    @BusMethod
    public Map<Integer, String> dictionaryIS(Map<Integer, String> aeis) throws BusException;

    @BusMethod
    public Map<Integer, byte[]> dictionaryIAY(Map<Integer, byte[]> aeiay) throws BusException;

    @BusMethod(name="DictionaryIR")
    public Map<Integer, InferredTypesInterface.InnerStruct> inferredDictionaryIR(Map<Integer, InferredTypesInterface.InnerStruct> aeir) throws BusException;

    @BusMethod
    public Map<Integer, Variant> dictionaryIV(Map<Integer, Variant> aeiv) throws BusException;

    @BusMethod
    public Map<Integer, Map<String, String>> dictionaryIAESS(Map<Integer, Map<String, String>> aeiaess) throws BusException;

    @BusMethod
    public Map<Long, Byte> dictionaryXY(Map<Long, Byte> aexy) throws BusException;

    @BusMethod
    public Map<Long, Boolean> dictionaryXB(Map<Long, Boolean> aexb) throws BusException;

    @BusMethod
    public Map<Long, Short> dictionaryXN(Map<Long, Short> aexn) throws BusException;

    @BusMethod
    public Map<Long, Integer> dictionaryXI(Map<Long, Integer> aexi) throws BusException;

    @BusMethod
    public Map<Long, Long> dictionaryXX(Map<Long, Long> aexx) throws BusException;

    @BusMethod
    public Map<Long, Double> dictionaryXD(Map<Long, Double> aexd) throws BusException;

    @BusMethod
    public Map<Long, String> dictionaryXS(Map<Long, String> aexs) throws BusException;

    @BusMethod
    public Map<Long, byte[]> dictionaryXAY(Map<Long, byte[]> aexay) throws BusException;

    @BusMethod(name="DictionaryXR")
    public Map<Long, InferredTypesInterface.InnerStruct> inferredDictionaryXR(Map<Long, InferredTypesInterface.InnerStruct> aexr) throws BusException;

    @BusMethod
    public Map<Long, Variant> dictionaryXV(Map<Long, Variant> aexv) throws BusException;

    @BusMethod
    public Map<Long, Map<String, String>> dictionaryXAESS(Map<Long, Map<String, String>> aexaess) throws BusException;

    @BusMethod
    public Map<Double, Byte> dictionaryDY(Map<Double, Byte> aedy) throws BusException;

    @BusMethod
    public Map<Double, Boolean> dictionaryDB(Map<Double, Boolean> aedb) throws BusException;

    @BusMethod
    public Map<Double, Short> dictionaryDN(Map<Double, Short> aedn) throws BusException;

    @BusMethod
    public Map<Double, Integer> dictionaryDI(Map<Double, Integer> aedi) throws BusException;

    @BusMethod
    public Map<Double, Long> dictionaryDX(Map<Double, Long> aedx) throws BusException;

    @BusMethod
    public Map<Double, Double> dictionaryDD(Map<Double, Double> aedd) throws BusException;

    @BusMethod
    public Map<Double, String> dictionaryDS(Map<Double, String> aeds) throws BusException;

    @BusMethod
    public Map<Double, byte[]> dictionaryDAY(Map<Double, byte[]> aeday) throws BusException;

    @BusMethod(name="DictionaryDR")
    public Map<Double, InferredTypesInterface.InnerStruct> inferredDictionaryDR(Map<Double, InferredTypesInterface.InnerStruct> aedr) throws BusException;

    @BusMethod
    public Map<Double, Variant> dictionaryDV(Map<Double, Variant> aedv) throws BusException;

    @BusMethod
    public Map<Double, Map<String, String>> dictionaryDAESS(Map<Double, Map<String, String>> aedaess) throws BusException;

    @BusMethod
    public Map<String, Byte> dictionarySY(Map<String, Byte> aesy) throws BusException;

    @BusMethod
    public Map<String, Boolean> dictionarySB(Map<String, Boolean> aesb) throws BusException;

    @BusMethod
    public Map<String, Short> dictionarySN(Map<String, Short> aesn) throws BusException;

    @BusMethod
    public Map<String, Integer> dictionarySI(Map<String, Integer> aesi) throws BusException;

    @BusMethod
    public Map<String, Long> dictionarySX(Map<String, Long> aesx) throws BusException;

    @BusMethod
    public Map<String, Double> dictionarySD(Map<String, Double> aesd) throws BusException;

    @BusMethod
    public Map<String, String> dictionarySS(Map<String, String> aess) throws BusException;

    @BusMethod
    public Map<String, byte[]> dictionarySAY(Map<String, byte[]> aesay) throws BusException;

    @BusMethod(name="DictionarySR")
    public Map<String, InferredTypesInterface.InnerStruct> inferredDictionarySR(Map<String, InferredTypesInterface.InnerStruct> aesr) throws BusException;

    @BusMethod
    public Map<String, Variant> dictionarySV(Map<String, Variant> aesv) throws BusException;

    @BusMethod
    public Map<String, Map<String, String>> dictionarySAESS(Map<String, Map<String, String>> aesaess) throws BusException;

    @BusMethod
    public TreeMap<String, String>[] treeDictionaryArray(TreeMap<String, String>[] aaess) throws BusException;

    @BusProperty
    public Map<String, String> getDictionarySS() throws BusException;

    @BusProperty
    public void setDictionarySS(Map<String, String> aess) throws BusException;

    public class TwoByteArrays {
        @Position(0) public byte[] ay0;
        @Position(1) public byte[] ay1;
    }

    @BusMethod
    public TwoByteArrays twoByteArrays(TwoByteArrays rayay) throws BusException;
}
