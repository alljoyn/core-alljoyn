/*
 * Copyright (c) 2009-2014, AllSeen Alliance. All rights reserved.
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

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.Variant;
import org.alljoyn.bus.VariantTypeReference;
import org.alljoyn.bus.ifaces.DBusProxyObj;
import static org.alljoyn.bus.Assert.*;

import java.util.Map;
import java.util.TreeMap;
import java.util.HashMap;
import java.lang.reflect.GenericArrayType;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.lang.reflect.TypeVariable;
import java.lang.reflect.WildcardType;
import static junit.framework.Assert.*;
import junit.framework.TestCase;

public class MarshalTest extends TestCase {
    private boolean isAndroid = false; // running on android device?

    public MarshalTest(String name) {
        super(name);
        if ("The Android Project".equals(System.getProperty("java.vendor")))
        {
            isAndroid = true;
        }
    }

    static {
        System.loadLibrary("alljoyn_java");
    }

    public class Service implements InferredTypesInterface,
                                    AnnotatedTypesInterface,
                                    BusObject {

        public void Void() throws BusException {}

        public byte Byte(byte y) throws BusException { return y; }

        public boolean Boolean(boolean b) throws BusException { return b; }

        public short Int16(short n) throws BusException { return n; }

        public int Int32(int i) throws BusException { return i; }

        public long Int64(long x) throws BusException { return x; }

        public double Double(double d) throws BusException { return d; }

        public String String(String s) throws BusException { return s; }

        public short Uint16(short q) throws BusException { return q; }

        public int Uint32(int u) throws BusException { return u; }

        public long Uint64(long t) throws BusException { return t; }

        public String ObjectPath(String o) throws BusException { return o; }

        public String Signature(String g) throws BusException { return g; }

        public byte[] ByteArray(byte[] ay) throws BusException { return ay; }

        public boolean[] BooleanArray(boolean[] ab) throws BusException { return ab; }

        public Boolean[] CapitalBooleanArray(Boolean[] aB) throws BusException { return aB; }

        public short[] Int16Array(short[] an) throws BusException { return an; }

        public short[] Uint16Array(short[] aq) throws BusException { return aq; }

        public int[] Int32Array(int[] ai) throws BusException { return ai; }

        public int[] Uint32Array(int[] au) throws BusException { return au; }

        public long[] Int64Array(long[] ax) throws BusException { return ax; }

        public long[] Uint64Array(long[] at) throws BusException { return at; }

        public double[] DoubleArray(double[] ad) throws BusException { return ad; }

        public String[] StringArray(String[] as) throws BusException { return as; }

        public String[] SignatureArray(String[] ag) throws BusException { return ag; }

        public String[] ObjectPathArray(String[] ao) throws BusException { return ao; }

        public byte[][] ArrayArray(byte[][] aay) throws BusException { return aay; }

        public InferredTypesInterface.InnerStruct[] InferredStructArray(InferredTypesInterface.InnerStruct[] ar) throws BusException { return ar; }

        public AnnotatedTypesInterface.InnerStruct[] AnnotatedStructArray(AnnotatedTypesInterface.InnerStruct[] ar) throws BusException { return ar; }

        public Variant[] VariantArray(Variant[] av) throws BusException { return av; }

        public Map<String, String>[] DictionaryArray(Map<String, String>[] aaess) throws BusException {
            return aaess;
        }

        public InferredTypesInterface.Struct InferredStruct(InferredTypesInterface.Struct r) throws BusException { return r; }

        public AnnotatedTypesInterface.Struct AnnotatedStruct(AnnotatedTypesInterface.Struct r) throws BusException { return r; }

        public Variant Variant(Variant v) throws BusException { return v; }

        public Map<Byte, Byte> DictionaryYY(Map<Byte, Byte> aeyy) throws BusException { return aeyy; }

        public Map<Byte, Boolean> DictionaryYB(Map<Byte, Boolean> aeyb) throws BusException { return aeyb; }

        public Map<Byte, Short> DictionaryYN(Map<Byte, Short> aeyn) throws BusException { return aeyn; }

        public Map<Byte, Integer> DictionaryYI(Map<Byte, Integer> aeyi) throws BusException { return aeyi; }

        public Map<Byte, Long> DictionaryYX(Map<Byte, Long> aeyx) throws BusException { return aeyx; }

        public Map<Byte, Double> DictionaryYD(Map<Byte, Double> aeyd) throws BusException { return aeyd; }

        public Map<Byte, String> DictionaryYS(Map<Byte, String> aeys) throws BusException { return aeys; }

        public Map<Byte, byte[]> DictionaryYAY(Map<Byte, byte[]> aeyay) throws BusException { return aeyay; }

        public Map<Byte, InferredTypesInterface.InnerStruct> InferredDictionaryYR(Map<Byte, InferredTypesInterface.InnerStruct> aeyr) throws BusException { return aeyr; }

        public Map<Byte, Variant> DictionaryYV(Map<Byte, Variant> aeyv) throws BusException { return aeyv; }

        public Map<Byte, Map<String, String>> DictionaryYAESS(Map<Byte, Map<String, String>> aeyaess) throws BusException { return aeyaess; }

        public Map<Boolean, Byte> DictionaryBY(Map<Boolean, Byte> aeby) throws BusException { return aeby; }

        public Map<Boolean, Boolean> DictionaryBB(Map<Boolean, Boolean> aebb) throws BusException { return aebb; }

        public Map<Boolean, Short> DictionaryBN(Map<Boolean, Short> aebn) throws BusException { return aebn; }

        public Map<Boolean, Integer> DictionaryBI(Map<Boolean, Integer> aebi) throws BusException { return aebi; }

        public Map<Boolean, Long> DictionaryBX(Map<Boolean, Long> aebx) throws BusException { return aebx; }

        public Map<Boolean, Double> DictionaryBD(Map<Boolean, Double> aebd) throws BusException { return aebd; }

        public Map<Boolean, String> DictionaryBS(Map<Boolean, String> aebs) throws BusException { return aebs; }

        public Map<Boolean, byte[]> DictionaryBAY(Map<Boolean, byte[]> aebay) throws BusException { return aebay; }

        public Map<Boolean, InferredTypesInterface.InnerStruct> InferredDictionaryBR(Map<Boolean, InferredTypesInterface.InnerStruct> aebr) throws BusException { return aebr; }

        public Map<Boolean, Variant> DictionaryBV(Map<Boolean, Variant> aebv) throws BusException { return aebv; }

        public Map<Boolean, Map<String, String>> DictionaryBAESS(Map<Boolean, Map<String, String>> aebaess) throws BusException { return aebaess; }

        public Map<Short, Byte> DictionaryNY(Map<Short, Byte> aeny) throws BusException { return aeny; }

        public Map<Short, Boolean> DictionaryNB(Map<Short, Boolean> aenb) throws BusException { return aenb; }

        public Map<Short, Short> DictionaryNN(Map<Short, Short> aenn) throws BusException { return aenn; }

        public Map<Short, Integer> DictionaryNI(Map<Short, Integer> aeni) throws BusException { return aeni; }

        public Map<Short, Long> DictionaryNX(Map<Short, Long> aenx) throws BusException { return aenx; }

        public Map<Short, Double> DictionaryND(Map<Short, Double> aend) throws BusException { return aend; }

        public Map<Short, String> DictionaryNS(Map<Short, String> aens) throws BusException { return aens; }

        public Map<Short, byte[]> DictionaryNAY(Map<Short, byte[]> aenay) throws BusException { return aenay; }

        public Map<Short, InferredTypesInterface.InnerStruct> InferredDictionaryNR(Map<Short, InferredTypesInterface.InnerStruct> aenr) throws BusException { return aenr; }

        public Map<Short, Variant> DictionaryNV(Map<Short, Variant> aenv) throws BusException { return aenv; }

        public Map<Short, Map<String, String>> DictionaryNAESS(Map<Short, Map<String, String>> aenaess) throws BusException { return aenaess; }

        public Map<Integer, Byte> DictionaryIY(Map<Integer, Byte> aeiy) throws BusException { return aeiy; }

        public Map<Integer, Boolean> DictionaryIB(Map<Integer, Boolean> aeib) throws BusException { return aeib; }

        public Map<Integer, Short> DictionaryIN(Map<Integer, Short> aein) throws BusException { return aein; }

        public Map<Integer, Integer> DictionaryII(Map<Integer, Integer> aeii) throws BusException { return aeii; }

        public Map<Integer, Long> DictionaryIX(Map<Integer, Long> aeix) throws BusException { return aeix; }

        public Map<Integer, Double> DictionaryID(Map<Integer, Double> aeid) throws BusException { return aeid; }

        public Map<Integer, String> DictionaryIS(Map<Integer, String> aeis) throws BusException { return aeis; }

        public Map<Integer, byte[]> DictionaryIAY(Map<Integer, byte[]> aeiay) throws BusException { return aeiay; }

        public Map<Integer, InferredTypesInterface.InnerStruct> InferredDictionaryIR(Map<Integer, InferredTypesInterface.InnerStruct> aeir) throws BusException { return aeir; }

        public Map<Integer, Variant> DictionaryIV(Map<Integer, Variant> aeiv) throws BusException { return aeiv; }

        public Map<Integer, Map<String, String>> DictionaryIAESS(Map<Integer, Map<String, String>> aeiaess) throws BusException { return aeiaess; }

        public Map<Long, Byte> DictionaryXY(Map<Long, Byte> aexy) throws BusException { return aexy; }

        public Map<Long, Boolean> DictionaryXB(Map<Long, Boolean> aexb) throws BusException { return aexb; }

        public Map<Long, Short> DictionaryXN(Map<Long, Short> aexn) throws BusException { return aexn; }

        public Map<Long, Integer> DictionaryXI(Map<Long, Integer> aexi) throws BusException { return aexi; }

        public Map<Long, Long> DictionaryXX(Map<Long, Long> aexx) throws BusException { return aexx; }

        public Map<Long, Double> DictionaryXD(Map<Long, Double> aexd) throws BusException { return aexd; }

        public Map<Long, String> DictionaryXS(Map<Long, String> aexs) throws BusException { return aexs; }

        public Map<Long, byte[]> DictionaryXAY(Map<Long, byte[]> aexay) throws BusException { return aexay; }

        public Map<Long, InferredTypesInterface.InnerStruct> InferredDictionaryXR(Map<Long, InferredTypesInterface.InnerStruct> aexr) throws BusException { return aexr; }

        public Map<Long, Variant> DictionaryXV(Map<Long, Variant> aexv) throws BusException { return aexv; }

        public Map<Long, Map<String, String>> DictionaryXAESS(Map<Long, Map<String, String>> aexaess) throws BusException { return aexaess; }

        public Map<Double, Byte> DictionaryDY(Map<Double, Byte> aedy) throws BusException { return aedy; }

        public Map<Double, Boolean> DictionaryDB(Map<Double, Boolean> aedb) throws BusException { return aedb; }

        public Map<Double, Short> DictionaryDN(Map<Double, Short> aedn) throws BusException { return aedn; }

        public Map<Double, Integer> DictionaryDI(Map<Double, Integer> aedi) throws BusException { return aedi; }

        public Map<Double, Long> DictionaryDX(Map<Double, Long> aedx) throws BusException { return aedx; }

        public Map<Double, Double> DictionaryDD(Map<Double, Double> aedd) throws BusException { return aedd; }

        public Map<Double, String> DictionaryDS(Map<Double, String> aeds) throws BusException { return aeds; }

        public Map<Double, byte[]> DictionaryDAY(Map<Double, byte[]> aeday) throws BusException { return aeday; }

        public Map<Double, InferredTypesInterface.InnerStruct> InferredDictionaryDR(Map<Double, InferredTypesInterface.InnerStruct> aedr) throws BusException { return aedr; }

        public Map<Double, Variant> DictionaryDV(Map<Double, Variant> aedv) throws BusException { return aedv; }

        public Map<Double, Map<String, String>> DictionaryDAESS(Map<Double, Map<String, String>> aedaess) throws BusException { return aedaess; }

        public Map<String, Byte> DictionarySY(Map<String, Byte> aesy) throws BusException { return aesy; }

        public Map<String, Boolean> DictionarySB(Map<String, Boolean> aesb) throws BusException { return aesb; }

        public Map<String, Short> DictionarySN(Map<String, Short> aesn) throws BusException { return aesn; }

        public Map<String, Integer> DictionarySI(Map<String, Integer> aesi) throws BusException { return aesi; }

        public Map<String, Long> DictionarySX(Map<String, Long> aesx) throws BusException { return aesx; }

        public Map<String, Double> DictionarySD(Map<String, Double> aesd) throws BusException { return aesd; }

        public Map<String, String> DictionarySS(Map<String, String> aess) throws BusException { return aess; }

        public Map<String, byte[]> DictionarySAY(Map<String, byte[]> aesay) throws BusException { return aesay; }

        public Map<String, InferredTypesInterface.InnerStruct> InferredDictionarySR(Map<String, InferredTypesInterface.InnerStruct> aesr) throws BusException { return aesr; }

        public Map<String, Variant> DictionarySV(Map<String, Variant> aesv) throws BusException { return aesv; }

        public Map<String, Map<String, String>> DictionarySAESS(Map<String, Map<String, String>> aesaess) throws BusException { return aesaess; }

        public Map<Short, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionaryNR(Map<Short, AnnotatedTypesInterface.InnerStruct> aenr) throws BusException { return aenr; }

        public Map<Short, Integer> DictionaryNU(Map<Short, Integer> aenu) throws BusException { return aenu; }

        public Map<Short, String> DictionaryNG(Map<Short, String> aeng) throws BusException { return aeng; }

        public Map<Short, Long> DictionaryNT(Map<Short, Long> aent) throws BusException { return aent; }

        public Map<Short, Short> DictionaryNQ(Map<Short, Short> aenq) throws BusException { return aenq; }

        public Map<Short, String> DictionaryNO(Map<Short, String> aeno) throws BusException { return aeno; }

        public Map<Double, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionaryDR(Map<Double, AnnotatedTypesInterface.InnerStruct> aedr) throws BusException { return aedr; }

        public Map<Double, Integer> DictionaryDU(Map<Double, Integer> aedu) throws BusException { return aedu; }

        public Map<Double, String> DictionaryDG(Map<Double, String> aedg) throws BusException { return aedg; }

        public Map<Double, Long> DictionaryDT(Map<Double, Long> aedt) throws BusException { return aedt; }

        public Map<Double, Short> DictionaryDQ(Map<Double, Short> aedq) throws BusException { return aedq; }

        public Map<Double, String> DictionaryDO(Map<Double, String> aedo) throws BusException { return aedo; }

        public Map<Long, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionaryXR(Map<Long, AnnotatedTypesInterface.InnerStruct> aexr) throws BusException { return aexr; }

        public Map<Long, Integer> DictionaryXU(Map<Long, Integer> aexu) throws BusException { return aexu; }

        public Map<Long, String> DictionaryXG(Map<Long, String> aexg) throws BusException { return aexg; }

        public Map<Long, Long> DictionaryXT(Map<Long, Long> aext) throws BusException { return aext; }

        public Map<Long, Short> DictionaryXQ(Map<Long, Short> aexq) throws BusException { return aexq; }

        public Map<Long, String> DictionaryXO(Map<Long, String> aexo) throws BusException { return aexo; }

        public Map<String, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionarySR(Map<String, AnnotatedTypesInterface.InnerStruct> aesr) throws BusException { return aesr; }

        public Map<String, Integer> DictionarySU(Map<String, Integer> aesu) throws BusException { return aesu; }

        public Map<String, String> DictionarySG(Map<String, String> aesg) throws BusException { return aesg; }

        public Map<String, Long> DictionaryST(Map<String, Long> aest) throws BusException { return aest; }

        public Map<String, Short> DictionarySQ(Map<String, Short> aesq) throws BusException { return aesq; }

        public Map<String, String> DictionarySO(Map<String, String> aeso) throws BusException { return aeso; }

        public Map<Byte, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionaryYR(Map<Byte, AnnotatedTypesInterface.InnerStruct> aeyr) throws BusException { return aeyr; }

        public Map<Byte, Integer> DictionaryYU(Map<Byte, Integer> aeyu) throws BusException { return aeyu; }

        public Map<Byte, String> DictionaryYG(Map<Byte, String> aeyg) throws BusException { return aeyg; }

        public Map<Byte, Long> DictionaryYT(Map<Byte, Long> aeyt) throws BusException { return aeyt; }

        public Map<Byte, Short> DictionaryYQ(Map<Byte, Short> aeyq) throws BusException { return aeyq; }

        public Map<Byte, String> DictionaryYO(Map<Byte, String> aeyo) throws BusException { return aeyo; }

        public Map<Integer, Map<String, String>> DictionaryUAESS(Map<Integer, Map<String, String>> aeuaess) throws BusException { return aeuaess; }

        public Map<Integer, AnnotatedTypesInterface.InnerStruct> DictionaryUR(Map<Integer, AnnotatedTypesInterface.InnerStruct> aeur) throws BusException { return aeur; }

        public Map<Integer, Double> DictionaryUD(Map<Integer, Double> aeud) throws BusException { return aeud; }

        public Map<Integer, Long> DictionaryUX(Map<Integer, Long> aeux) throws BusException { return aeux; }

        public Map<Integer, Byte> DictionaryUY(Map<Integer, Byte> aeuy) throws BusException { return aeuy; }

        public Map<Integer, Integer> DictionaryUU(Map<Integer, Integer> aeuu) throws BusException { return aeuu; }

        public Map<Integer, String> DictionaryUG(Map<Integer, String> aeug) throws BusException { return aeug; }

        public Map<Integer, Integer> DictionaryUI(Map<Integer, Integer> aeui) throws BusException { return aeui; }

        public Map<Integer, Long> DictionaryUT(Map<Integer, Long> aeut) throws BusException { return aeut; }

        public Map<Integer, Short> DictionaryUN(Map<Integer, Short> aeun) throws BusException { return aeun; }

        public Map<Integer, Variant> DictionaryUV(Map<Integer, Variant> aeuv) throws BusException { return aeuv; }

        public Map<Integer, String> DictionaryUS(Map<Integer, String> aeus) throws BusException { return aeus; }

        public Map<Integer, byte[]> DictionaryUAY(Map<Integer, byte[]> aeuay) throws BusException { return aeuay; }

        public Map<Integer, Short> DictionaryUQ(Map<Integer, Short> aeuq) throws BusException { return aeuq; }

        public Map<Integer, Boolean> DictionaryUB(Map<Integer, Boolean> aeub) throws BusException { return aeub; }

        public Map<Integer, String> DictionaryUO(Map<Integer, String> aeuo) throws BusException { return aeuo; }

        public Map<String, Map<String, String>> DictionaryGAESS(Map<String, Map<String, String>> aegaess) throws BusException { return aegaess; }

        public Map<String, AnnotatedTypesInterface.InnerStruct> DictionaryGR(Map<String, AnnotatedTypesInterface.InnerStruct> aegr) throws BusException { return aegr; }

        public Map<String, Double> DictionaryGD(Map<String, Double> aegd) throws BusException { return aegd; }

        public Map<String, Long> DictionaryGX(Map<String, Long> aegx) throws BusException { return aegx; }

        public Map<String, Byte> DictionaryGY(Map<String, Byte> aegy) throws BusException { return aegy; }

        public Map<String, Integer> DictionaryGU(Map<String, Integer> aegu) throws BusException { return aegu; }

        public Map<String, String> DictionaryGG(Map<String, String> aegg) throws BusException { return aegg; }

        public Map<String, Integer> DictionaryGI(Map<String, Integer> aegi) throws BusException { return aegi; }

        public Map<String, Long> DictionaryGT(Map<String, Long> aegt) throws BusException { return aegt; }

        public Map<String, Short> DictionaryGN(Map<String, Short> aegn) throws BusException { return aegn; }

        public Map<String, Variant> DictionaryGV(Map<String, Variant> aegv) throws BusException { return aegv; }

        public Map<String, String> DictionaryGS(Map<String, String> aegs) throws BusException { return aegs; }

        public Map<String, byte[]> DictionaryGAY(Map<String, byte[]> aegay) throws BusException { return aegay; }

        public Map<String, Short> DictionaryGQ(Map<String, Short> aegq) throws BusException { return aegq; }

        public Map<String, Boolean> DictionaryGB(Map<String, Boolean> aegb) throws BusException { return aegb; }

        public Map<String, String> DictionaryGO(Map<String, String> aego) throws BusException { return aego; }

        public Map<Boolean, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionaryBR(Map<Boolean, AnnotatedTypesInterface.InnerStruct> aebr) throws BusException { return aebr; }

        public Map<Boolean, Integer> DictionaryBU(Map<Boolean, Integer> aebu) throws BusException { return aebu; }

        public Map<Boolean, String> DictionaryBG(Map<Boolean, String> aebg) throws BusException { return aebg; }

        public Map<Boolean, Long> DictionaryBT(Map<Boolean, Long> aebt) throws BusException { return aebt; }

        public Map<Boolean, Short> DictionaryBQ(Map<Boolean, Short> aebq) throws BusException { return aebq; }

        public Map<Boolean, String> DictionaryBO(Map<Boolean, String> aebo) throws BusException { return aebo; }

        public Map<Short, Map<String, String>> DictionaryQAESS(Map<Short, Map<String, String>> aeqaess) throws BusException { return aeqaess; }

        public Map<Short, AnnotatedTypesInterface.InnerStruct> DictionaryQR(Map<Short, AnnotatedTypesInterface.InnerStruct> aeqr) throws BusException { return aeqr; }

        public Map<Short, Double> DictionaryQD(Map<Short, Double> aeqd) throws BusException { return aeqd; }

        public Map<Short, Long> DictionaryQX(Map<Short, Long> aeqx) throws BusException { return aeqx; }

        public Map<Short, Byte> DictionaryQY(Map<Short, Byte> aeqy) throws BusException { return aeqy; }

        public Map<Short, Integer> DictionaryQU(Map<Short, Integer> aequ) throws BusException { return aequ; }

        public Map<Short, String> DictionaryQG(Map<Short, String> aeqg) throws BusException { return aeqg; }

        public Map<Short, Integer> DictionaryQI(Map<Short, Integer> aeqi) throws BusException { return aeqi; }

        public Map<Short, Long> DictionaryQT(Map<Short, Long> aeqt) throws BusException { return aeqt; }

        public Map<Short, Short> DictionaryQN(Map<Short, Short> aeqn) throws BusException { return aeqn; }

        public Map<Short, Variant> DictionaryQV(Map<Short, Variant> aeqv) throws BusException { return aeqv; }

        public Map<Short, String> DictionaryQS(Map<Short, String> aeqs) throws BusException { return aeqs; }

        public Map<Short, byte[]> DictionaryQAY(Map<Short, byte[]> aeqay) throws BusException { return aeqay; }

        public Map<Short, Short> DictionaryQQ(Map<Short, Short> aeqq) throws BusException { return aeqq; }

        public Map<Short, Boolean> DictionaryQB(Map<Short, Boolean> aeqb) throws BusException { return aeqb; }

        public Map<Short, String> DictionaryQO(Map<Short, String> aeqo) throws BusException { return aeqo; }

        public Map<String, Map<String, String>> DictionaryOAESS(Map<String, Map<String, String>> aeoaess) throws BusException { return aeoaess; }

        public Map<String, AnnotatedTypesInterface.InnerStruct> DictionaryOR(Map<String, AnnotatedTypesInterface.InnerStruct> aeor) throws BusException { return aeor; }

        public Map<String, Double> DictionaryOD(Map<String, Double> aeod) throws BusException { return aeod; }

        public Map<String, Long> DictionaryOX(Map<String, Long> aeox) throws BusException { return aeox; }

        public Map<String, Byte> DictionaryOY(Map<String, Byte> aeoy) throws BusException { return aeoy; }

        public Map<String, Integer> DictionaryOU(Map<String, Integer> aeou) throws BusException { return aeou; }

        public Map<String, String> DictionaryOG(Map<String, String> aeog) throws BusException { return aeog; }

        public Map<String, Integer> DictionaryOI(Map<String, Integer> aeoi) throws BusException { return aeoi; }

        public Map<String, Long> DictionaryOT(Map<String, Long> aeot) throws BusException { return aeot; }

        public Map<String, Short> DictionaryON(Map<String, Short> aeon) throws BusException { return aeon; }

        public Map<String, Variant> DictionaryOV(Map<String, Variant> aeov) throws BusException { return aeov; }

        public Map<String, String> DictionaryOS(Map<String, String> aeos) throws BusException { return aeos; }

        public Map<String, byte[]> DictionaryOAY(Map<String, byte[]> aeoay) throws BusException { return aeoay; }

        public Map<String, Short> DictionaryOQ(Map<String, Short> aeoq) throws BusException { return aeoq; }

        public Map<String, Boolean> DictionaryOB(Map<String, Boolean> aeob) throws BusException { return aeob; }

        public Map<String, String> DictionaryOO(Map<String, String> aeoo) throws BusException { return aeoo; }

        public Map<Integer, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionaryIR(Map<Integer, AnnotatedTypesInterface.InnerStruct> aeir) throws BusException { return aeir; }

        public Map<Integer, Integer> DictionaryIU(Map<Integer, Integer> aeiu) throws BusException { return aeiu; }

        public Map<Integer, String> DictionaryIG(Map<Integer, String> aeig) throws BusException { return aeig; }

        public Map<Integer, Long> DictionaryIT(Map<Integer, Long> aeit) throws BusException { return aeit; }

        public Map<Integer, Short> DictionaryIQ(Map<Integer, Short> aeiq) throws BusException { return aeiq; }

        public Map<Integer, String> DictionaryIO(Map<Integer, String> aeio) throws BusException { return aeio; }

        public Map<Long, Map<String, String>> DictionaryTAESS(Map<Long, Map<String, String>> aetaess) throws BusException { return aetaess; }

        public Map<Long, AnnotatedTypesInterface.InnerStruct> DictionaryTR(Map<Long, AnnotatedTypesInterface.InnerStruct> aetr) throws BusException { return aetr; }

        public Map<Long, Double> DictionaryTD(Map<Long, Double> aetd) throws BusException { return aetd; }

        public Map<Long, Long> DictionaryTX(Map<Long, Long> aetx) throws BusException { return aetx; }

        public Map<Long, Byte> DictionaryTY(Map<Long, Byte> aety) throws BusException { return aety; }

        public Map<Long, Integer> DictionaryTU(Map<Long, Integer> aetu) throws BusException { return aetu; }

        public Map<Long, String> DictionaryTG(Map<Long, String> aetg) throws BusException { return aetg; }

        public Map<Long, Integer> DictionaryTI(Map<Long, Integer> aeti) throws BusException { return aeti; }

        public Map<Long, Long> DictionaryTT(Map<Long, Long> aett) throws BusException { return aett; }

        public Map<Long, Short> DictionaryTN(Map<Long, Short> aetn) throws BusException { return aetn; }

        public Map<Long, Variant> DictionaryTV(Map<Long, Variant> aetv) throws BusException { return aetv; }

        public Map<Long, String> DictionaryTS(Map<Long, String> aets) throws BusException { return aets; }

        public Map<Long, byte[]> DictionaryTAY(Map<Long, byte[]> aetay) throws BusException { return aetay; }

        public Map<Long, Short> DictionaryTQ(Map<Long, Short> aetq) throws BusException { return aetq; }

        public Map<Long, Boolean> DictionaryTB(Map<Long, Boolean> aetb) throws BusException { return aetb; }

        public Map<Long, String> DictionaryTO(Map<Long, String> aeto) throws BusException { return aeto; }

        public Map<String, String> getDictionarySS() throws BusException {
            TreeMap<String, String> aess = new TreeMap<String, String>();
            aess.put("six", "six");
            return aess;
        }

        public void setDictionarySS(Map<String, String> aess) throws BusException {}

        public EnumType EnumY(EnumType y) throws BusException { return y; }

        public EnumType EnumN(EnumType n) throws BusException { return n; }

        public EnumType EnumQ(EnumType q) throws BusException { return q; }

        public EnumType EnumI(EnumType i) throws BusException { return i; }

        public EnumType EnumU(EnumType u) throws BusException { return u; }

        public EnumType EnumX(EnumType x) throws BusException { return x; }

        public EnumType EnumT(EnumType t) throws BusException { return t; }

        public TreeMap<String, String>[] TreeDictionaryArray(TreeMap<String, String>[] aaess) throws BusException {
            return aaess;
        }

        public TwoByteArrays TwoByteArrays(TwoByteArrays rayay) throws BusException { return rayay; }
    }

    public class NullService implements InferredTypesInterface,
                                        AnnotatedTypesInterface,
                                        BusObject {

        public void Void() throws BusException {}

        public byte Byte(byte y) throws BusException { return y; }

        public boolean Boolean(boolean b) throws BusException { return b; }

        public short Int16(short n) throws BusException { return n; }

        public int Int32(int i) throws BusException { return i; }

        public long Int64(long x) throws BusException { return x; }

        public double Double(double d) throws BusException { return d; }

        public String String(String s) throws BusException { return null; }

        public short Uint16(short q) throws BusException { return q; }

        public int Uint32(int u) throws BusException { return u; }

        public long Uint64(long t) throws BusException { return t; }

        public String ObjectPath(String o) throws BusException { return null; }

        public String Signature(String g) throws BusException { return null; }

        public byte[] ByteArray(byte[] ay) throws BusException { return null; }

        public boolean[] BooleanArray(boolean[] ab) throws BusException { return null; }

        public Boolean[] CapitalBooleanArray(Boolean[] aB) throws BusException { return null; }

        public short[] Int16Array(short[] an) throws BusException { return null; }

        public short[] Uint16Array(short[] aq) throws BusException { return null; }

        public int[] Int32Array(int[] ai) throws BusException { return null; }

        public int[] Uint32Array(int[] au) throws BusException { return null; }

        public long[] Int64Array(long[] ax) throws BusException { return null; }

        public long[] Uint64Array(long[] at) throws BusException { return null; }

        public double[] DoubleArray(double[] ad) throws BusException { return null; }

        public String[] StringArray(String[] as) throws BusException { return null; }

        public String[] SignatureArray(String[] ag) throws BusException { return null; }

        public String[] ObjectPathArray(String[] ao) throws BusException { return null; }

        public byte[][] ArrayArray(byte[][] aay) throws BusException { return null; }

        public InferredTypesInterface.InnerStruct[] InferredStructArray(InferredTypesInterface.InnerStruct[] ar) throws BusException { return null; }

        public AnnotatedTypesInterface.InnerStruct[] AnnotatedStructArray(AnnotatedTypesInterface.InnerStruct[] ar) throws BusException { return null; }

        public Variant[] VariantArray(Variant[] av) throws BusException { return null; }

        public Map<String, String>[] DictionaryArray(Map<String, String>[] aaess) throws BusException {
            return null;
        }

        public InferredTypesInterface.Struct InferredStruct(InferredTypesInterface.Struct r) throws BusException { return null; }

        public AnnotatedTypesInterface.Struct AnnotatedStruct(AnnotatedTypesInterface.Struct r) throws BusException { return null; }

        public Variant Variant(Variant v) throws BusException { return null; }

        public Map<Byte, Byte> DictionaryYY(Map<Byte, Byte> aeyy) throws BusException { return null; }

        public Map<Byte, Boolean> DictionaryYB(Map<Byte, Boolean> aeyb) throws BusException { return null; }

        public Map<Byte, Short> DictionaryYN(Map<Byte, Short> aeyn) throws BusException { return null; }

        public Map<Byte, Integer> DictionaryYI(Map<Byte, Integer> aeyi) throws BusException { return null; }

        public Map<Byte, Long> DictionaryYX(Map<Byte, Long> aeyx) throws BusException { return null; }

        public Map<Byte, Double> DictionaryYD(Map<Byte, Double> aeyd) throws BusException { return null; }

        public Map<Byte, String> DictionaryYS(Map<Byte, String> aeys) throws BusException { return null; }

        public Map<Byte, byte[]> DictionaryYAY(Map<Byte, byte[]> aeyay) throws BusException { return null; }

        public Map<Byte, InferredTypesInterface.InnerStruct> InferredDictionaryYR(Map<Byte, InferredTypesInterface.InnerStruct> aeyr) throws BusException { return null; }

        public Map<Byte, Variant> DictionaryYV(Map<Byte, Variant> aeyv) throws BusException { return null; }

        public Map<Byte, Map<String, String>> DictionaryYAESS(Map<Byte, Map<String, String>> aeyaess) throws BusException { return null; }

        public Map<Boolean, Byte> DictionaryBY(Map<Boolean, Byte> aeby) throws BusException { return null; }

        public Map<Boolean, Boolean> DictionaryBB(Map<Boolean, Boolean> aebb) throws BusException { return null; }

        public Map<Boolean, Short> DictionaryBN(Map<Boolean, Short> aebn) throws BusException { return null; }

        public Map<Boolean, Integer> DictionaryBI(Map<Boolean, Integer> aebi) throws BusException { return null; }

        public Map<Boolean, Long> DictionaryBX(Map<Boolean, Long> aebx) throws BusException { return null; }

        public Map<Boolean, Double> DictionaryBD(Map<Boolean, Double> aebd) throws BusException { return null; }

        public Map<Boolean, String> DictionaryBS(Map<Boolean, String> aebs) throws BusException { return null; }

        public Map<Boolean, byte[]> DictionaryBAY(Map<Boolean, byte[]> aebay) throws BusException { return null; }

        public Map<Boolean, InferredTypesInterface.InnerStruct> InferredDictionaryBR(Map<Boolean, InferredTypesInterface.InnerStruct> aebr) throws BusException { return null; }

        public Map<Boolean, Variant> DictionaryBV(Map<Boolean, Variant> aebv) throws BusException { return null; }

        public Map<Boolean, Map<String, String>> DictionaryBAESS(Map<Boolean, Map<String, String>> aebaess) throws BusException { return null; }

        public Map<Short, Byte> DictionaryNY(Map<Short, Byte> aeny) throws BusException { return null; }

        public Map<Short, Boolean> DictionaryNB(Map<Short, Boolean> aenb) throws BusException { return null; }

        public Map<Short, Short> DictionaryNN(Map<Short, Short> aenn) throws BusException { return null; }

        public Map<Short, Integer> DictionaryNI(Map<Short, Integer> aeni) throws BusException { return null; }

        public Map<Short, Long> DictionaryNX(Map<Short, Long> aenx) throws BusException { return null; }

        public Map<Short, Double> DictionaryND(Map<Short, Double> aend) throws BusException { return null; }

        public Map<Short, String> DictionaryNS(Map<Short, String> aens) throws BusException { return null; }

        public Map<Short, byte[]> DictionaryNAY(Map<Short, byte[]> aenay) throws BusException { return null; }

        public Map<Short, InferredTypesInterface.InnerStruct> InferredDictionaryNR(Map<Short, InferredTypesInterface.InnerStruct> aenr) throws BusException { return null; }

        public Map<Short, Variant> DictionaryNV(Map<Short, Variant> aenv) throws BusException { return null; }

        public Map<Short, Map<String, String>> DictionaryNAESS(Map<Short, Map<String, String>> aenaess) throws BusException { return null; }

        public Map<Integer, Byte> DictionaryIY(Map<Integer, Byte> aeiy) throws BusException { return null; }

        public Map<Integer, Boolean> DictionaryIB(Map<Integer, Boolean> aeib) throws BusException { return null; }

        public Map<Integer, Short> DictionaryIN(Map<Integer, Short> aein) throws BusException { return null; }

        public Map<Integer, Integer> DictionaryII(Map<Integer, Integer> aeii) throws BusException { return null; }

        public Map<Integer, Long> DictionaryIX(Map<Integer, Long> aeix) throws BusException { return null; }

        public Map<Integer, Double> DictionaryID(Map<Integer, Double> aeid) throws BusException { return null; }

        public Map<Integer, String> DictionaryIS(Map<Integer, String> aeis) throws BusException { return null; }

        public Map<Integer, byte[]> DictionaryIAY(Map<Integer, byte[]> aeiay) throws BusException { return null; }

        public Map<Integer, InferredTypesInterface.InnerStruct> InferredDictionaryIR(Map<Integer, InferredTypesInterface.InnerStruct> aeir) throws BusException { return null; }

        public Map<Integer, Variant> DictionaryIV(Map<Integer, Variant> aeiv) throws BusException { return null; }

        public Map<Integer, Map<String, String>> DictionaryIAESS(Map<Integer, Map<String, String>> aeiaess) throws BusException { return null; }

        public Map<Long, Byte> DictionaryXY(Map<Long, Byte> aexy) throws BusException { return null; }

        public Map<Long, Boolean> DictionaryXB(Map<Long, Boolean> aexb) throws BusException { return null; }

        public Map<Long, Short> DictionaryXN(Map<Long, Short> aexn) throws BusException { return null; }

        public Map<Long, Integer> DictionaryXI(Map<Long, Integer> aexi) throws BusException { return null; }

        public Map<Long, Long> DictionaryXX(Map<Long, Long> aexx) throws BusException { return null; }

        public Map<Long, Double> DictionaryXD(Map<Long, Double> aexd) throws BusException { return null; }

        public Map<Long, String> DictionaryXS(Map<Long, String> aexs) throws BusException { return null; }

        public Map<Long, byte[]> DictionaryXAY(Map<Long, byte[]> aexay) throws BusException { return null; }

        public Map<Long, InferredTypesInterface.InnerStruct> InferredDictionaryXR(Map<Long, InferredTypesInterface.InnerStruct> aexr) throws BusException { return null; }

        public Map<Long, Variant> DictionaryXV(Map<Long, Variant> aexv) throws BusException { return null; }

        public Map<Long, Map<String, String>> DictionaryXAESS(Map<Long, Map<String, String>> aexaess) throws BusException { return null; }

        public Map<Double, Byte> DictionaryDY(Map<Double, Byte> aedy) throws BusException { return null; }

        public Map<Double, Boolean> DictionaryDB(Map<Double, Boolean> aedb) throws BusException { return null; }

        public Map<Double, Short> DictionaryDN(Map<Double, Short> aedn) throws BusException { return null; }

        public Map<Double, Integer> DictionaryDI(Map<Double, Integer> aedi) throws BusException { return null; }

        public Map<Double, Long> DictionaryDX(Map<Double, Long> aedx) throws BusException { return null; }

        public Map<Double, Double> DictionaryDD(Map<Double, Double> aedd) throws BusException { return null; }

        public Map<Double, String> DictionaryDS(Map<Double, String> aeds) throws BusException { return null; }

        public Map<Double, byte[]> DictionaryDAY(Map<Double, byte[]> aeday) throws BusException { return null; }

        public Map<Double, InferredTypesInterface.InnerStruct> InferredDictionaryDR(Map<Double, InferredTypesInterface.InnerStruct> aedr) throws BusException { return null; }

        public Map<Double, Variant> DictionaryDV(Map<Double, Variant> aedv) throws BusException { return null; }

        public Map<Double, Map<String, String>> DictionaryDAESS(Map<Double, Map<String, String>> aedaess) throws BusException { return null; }

        public Map<String, Byte> DictionarySY(Map<String, Byte> aesy) throws BusException { return null; }

        public Map<String, Boolean> DictionarySB(Map<String, Boolean> aesb) throws BusException { return null; }

        public Map<String, Short> DictionarySN(Map<String, Short> aesn) throws BusException { return null; }

        public Map<String, Integer> DictionarySI(Map<String, Integer> aesi) throws BusException { return null; }

        public Map<String, Long> DictionarySX(Map<String, Long> aesx) throws BusException { return null; }

        public Map<String, Double> DictionarySD(Map<String, Double> aesd) throws BusException { return null; }

        public Map<String, String> DictionarySS(Map<String, String> aess) throws BusException { return null; }

        public Map<String, byte[]> DictionarySAY(Map<String, byte[]> aesay) throws BusException { return null; }

        public Map<String, InferredTypesInterface.InnerStruct> InferredDictionarySR(Map<String, InferredTypesInterface.InnerStruct> aesr) throws BusException { return null; }

        public Map<String, Variant> DictionarySV(Map<String, Variant> aesv) throws BusException { return null; }

        public Map<String, Map<String, String>> DictionarySAESS(Map<String, Map<String, String>> aesaess) throws BusException { return null; }

        public Map<Short, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionaryNR(Map<Short, AnnotatedTypesInterface.InnerStruct> aenr) throws BusException { return null; }

        public Map<Short, Integer> DictionaryNU(Map<Short, Integer> aenu) throws BusException { return null; }

        public Map<Short, String> DictionaryNG(Map<Short, String> aeng) throws BusException { return null; }

        public Map<Short, Long> DictionaryNT(Map<Short, Long> aent) throws BusException { return null; }

        public Map<Short, Short> DictionaryNQ(Map<Short, Short> aenq) throws BusException { return null; }

        public Map<Short, String> DictionaryNO(Map<Short, String> aeno) throws BusException { return null; }

        public Map<Double, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionaryDR(Map<Double, AnnotatedTypesInterface.InnerStruct> aedr) throws BusException { return null; }

        public Map<Double, Integer> DictionaryDU(Map<Double, Integer> aedu) throws BusException { return null; }

        public Map<Double, String> DictionaryDG(Map<Double, String> aedg) throws BusException { return null; }

        public Map<Double, Long> DictionaryDT(Map<Double, Long> aedt) throws BusException { return null; }

        public Map<Double, Short> DictionaryDQ(Map<Double, Short> aedq) throws BusException { return null; }

        public Map<Double, String> DictionaryDO(Map<Double, String> aedo) throws BusException { return null; }

        public Map<Long, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionaryXR(Map<Long, AnnotatedTypesInterface.InnerStruct> aexr) throws BusException { return null; }

        public Map<Long, Integer> DictionaryXU(Map<Long, Integer> aexu) throws BusException { return null; }

        public Map<Long, String> DictionaryXG(Map<Long, String> aexg) throws BusException { return null; }

        public Map<Long, Long> DictionaryXT(Map<Long, Long> aext) throws BusException { return null; }

        public Map<Long, Short> DictionaryXQ(Map<Long, Short> aexq) throws BusException { return null; }

        public Map<Long, String> DictionaryXO(Map<Long, String> aexo) throws BusException { return null; }

        public Map<String, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionarySR(Map<String, AnnotatedTypesInterface.InnerStruct> aesr) throws BusException { return null; }

        public Map<String, Integer> DictionarySU(Map<String, Integer> aesu) throws BusException { return null; }

        public Map<String, String> DictionarySG(Map<String, String> aesg) throws BusException { return null; }

        public Map<String, Long> DictionaryST(Map<String, Long> aest) throws BusException { return null; }

        public Map<String, Short> DictionarySQ(Map<String, Short> aesq) throws BusException { return null; }

        public Map<String, String> DictionarySO(Map<String, String> aeso) throws BusException { return null; }

        public Map<Byte, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionaryYR(Map<Byte, AnnotatedTypesInterface.InnerStruct> aeyr) throws BusException { return null; }

        public Map<Byte, Integer> DictionaryYU(Map<Byte, Integer> aeyu) throws BusException { return null; }

        public Map<Byte, String> DictionaryYG(Map<Byte, String> aeyg) throws BusException { return null; }

        public Map<Byte, Long> DictionaryYT(Map<Byte, Long> aeyt) throws BusException { return null; }

        public Map<Byte, Short> DictionaryYQ(Map<Byte, Short> aeyq) throws BusException { return null; }

        public Map<Byte, String> DictionaryYO(Map<Byte, String> aeyo) throws BusException { return null; }

        public Map<Integer, Map<String, String>> DictionaryUAESS(Map<Integer, Map<String, String>> aeuaess) throws BusException { return null; }

        public Map<Integer, AnnotatedTypesInterface.InnerStruct> DictionaryUR(Map<Integer, AnnotatedTypesInterface.InnerStruct> aeur) throws BusException { return null; }

        public Map<Integer, Double> DictionaryUD(Map<Integer, Double> aeud) throws BusException { return null; }

        public Map<Integer, Long> DictionaryUX(Map<Integer, Long> aeux) throws BusException { return null; }

        public Map<Integer, Byte> DictionaryUY(Map<Integer, Byte> aeuy) throws BusException { return null; }

        public Map<Integer, Integer> DictionaryUU(Map<Integer, Integer> aeuu) throws BusException { return null; }

        public Map<Integer, String> DictionaryUG(Map<Integer, String> aeug) throws BusException { return null; }

        public Map<Integer, Integer> DictionaryUI(Map<Integer, Integer> aeui) throws BusException { return null; }

        public Map<Integer, Long> DictionaryUT(Map<Integer, Long> aeut) throws BusException { return null; }

        public Map<Integer, Short> DictionaryUN(Map<Integer, Short> aeun) throws BusException { return null; }

        public Map<Integer, Variant> DictionaryUV(Map<Integer, Variant> aeuv) throws BusException { return null; }

        public Map<Integer, String> DictionaryUS(Map<Integer, String> aeus) throws BusException { return null; }

        public Map<Integer, byte[]> DictionaryUAY(Map<Integer, byte[]> aeuay) throws BusException { return null; }

        public Map<Integer, Short> DictionaryUQ(Map<Integer, Short> aeuq) throws BusException { return null; }

        public Map<Integer, Boolean> DictionaryUB(Map<Integer, Boolean> aeub) throws BusException { return null; }

        public Map<Integer, String> DictionaryUO(Map<Integer, String> aeuo) throws BusException { return null; }

        public Map<String, Map<String, String>> DictionaryGAESS(Map<String, Map<String, String>> aegaess) throws BusException { return null; }

        public Map<String, AnnotatedTypesInterface.InnerStruct> DictionaryGR(Map<String, AnnotatedTypesInterface.InnerStruct> aegr) throws BusException { return null; }

        public Map<String, Double> DictionaryGD(Map<String, Double> aegd) throws BusException { return null; }

        public Map<String, Long> DictionaryGX(Map<String, Long> aegx) throws BusException { return null; }

        public Map<String, Byte> DictionaryGY(Map<String, Byte> aegy) throws BusException { return null; }

        public Map<String, Integer> DictionaryGU(Map<String, Integer> aegu) throws BusException { return null; }

        public Map<String, String> DictionaryGG(Map<String, String> aegg) throws BusException { return null; }

        public Map<String, Integer> DictionaryGI(Map<String, Integer> aegi) throws BusException { return null; }

        public Map<String, Long> DictionaryGT(Map<String, Long> aegt) throws BusException { return null; }

        public Map<String, Short> DictionaryGN(Map<String, Short> aegn) throws BusException { return null; }

        public Map<String, Variant> DictionaryGV(Map<String, Variant> aegv) throws BusException { return null; }

        public Map<String, String> DictionaryGS(Map<String, String> aegs) throws BusException { return null; }

        public Map<String, byte[]> DictionaryGAY(Map<String, byte[]> aegay) throws BusException { return null; }

        public Map<String, Short> DictionaryGQ(Map<String, Short> aegq) throws BusException { return null; }

        public Map<String, Boolean> DictionaryGB(Map<String, Boolean> aegb) throws BusException { return null; }

        public Map<String, String> DictionaryGO(Map<String, String> aego) throws BusException { return null; }

        public Map<Boolean, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionaryBR(Map<Boolean, AnnotatedTypesInterface.InnerStruct> aebr) throws BusException { return null; }

        public Map<Boolean, Integer> DictionaryBU(Map<Boolean, Integer> aebu) throws BusException { return null; }

        public Map<Boolean, String> DictionaryBG(Map<Boolean, String> aebg) throws BusException { return null; }

        public Map<Boolean, Long> DictionaryBT(Map<Boolean, Long> aebt) throws BusException { return null; }

        public Map<Boolean, Short> DictionaryBQ(Map<Boolean, Short> aebq) throws BusException { return null; }

        public Map<Boolean, String> DictionaryBO(Map<Boolean, String> aebo) throws BusException { return null; }

        public Map<Short, Map<String, String>> DictionaryQAESS(Map<Short, Map<String, String>> aeqaess) throws BusException { return null; }

        public Map<Short, AnnotatedTypesInterface.InnerStruct> DictionaryQR(Map<Short, AnnotatedTypesInterface.InnerStruct> aeqr) throws BusException { return null; }

        public Map<Short, Double> DictionaryQD(Map<Short, Double> aeqd) throws BusException { return null; }

        public Map<Short, Long> DictionaryQX(Map<Short, Long> aeqx) throws BusException { return null; }

        public Map<Short, Byte> DictionaryQY(Map<Short, Byte> aeqy) throws BusException { return null; }

        public Map<Short, Integer> DictionaryQU(Map<Short, Integer> aequ) throws BusException { return null; }

        public Map<Short, String> DictionaryQG(Map<Short, String> aeqg) throws BusException { return null; }

        public Map<Short, Integer> DictionaryQI(Map<Short, Integer> aeqi) throws BusException { return null; }

        public Map<Short, Long> DictionaryQT(Map<Short, Long> aeqt) throws BusException { return null; }

        public Map<Short, Short> DictionaryQN(Map<Short, Short> aeqn) throws BusException { return null; }

        public Map<Short, Variant> DictionaryQV(Map<Short, Variant> aeqv) throws BusException { return null; }

        public Map<Short, String> DictionaryQS(Map<Short, String> aeqs) throws BusException { return null; }

        public Map<Short, byte[]> DictionaryQAY(Map<Short, byte[]> aeqay) throws BusException { return null; }

        public Map<Short, Short> DictionaryQQ(Map<Short, Short> aeqq) throws BusException { return null; }

        public Map<Short, Boolean> DictionaryQB(Map<Short, Boolean> aeqb) throws BusException { return null; }

        public Map<Short, String> DictionaryQO(Map<Short, String> aeqo) throws BusException { return null; }

        public Map<String, Map<String, String>> DictionaryOAESS(Map<String, Map<String, String>> aeoaess) throws BusException { return null; }

        public Map<String, AnnotatedTypesInterface.InnerStruct> DictionaryOR(Map<String, AnnotatedTypesInterface.InnerStruct> aeor) throws BusException { return null; }

        public Map<String, Double> DictionaryOD(Map<String, Double> aeod) throws BusException { return null; }

        public Map<String, Long> DictionaryOX(Map<String, Long> aeox) throws BusException { return null; }

        public Map<String, Byte> DictionaryOY(Map<String, Byte> aeoy) throws BusException { return null; }

        public Map<String, Integer> DictionaryOU(Map<String, Integer> aeou) throws BusException { return null; }

        public Map<String, String> DictionaryOG(Map<String, String> aeog) throws BusException { return null; }

        public Map<String, Integer> DictionaryOI(Map<String, Integer> aeoi) throws BusException { return null; }

        public Map<String, Long> DictionaryOT(Map<String, Long> aeot) throws BusException { return null; }

        public Map<String, Short> DictionaryON(Map<String, Short> aeon) throws BusException { return null; }

        public Map<String, Variant> DictionaryOV(Map<String, Variant> aeov) throws BusException { return null; }

        public Map<String, String> DictionaryOS(Map<String, String> aeos) throws BusException { return null; }

        public Map<String, byte[]> DictionaryOAY(Map<String, byte[]> aeoay) throws BusException { return null; }

        public Map<String, Short> DictionaryOQ(Map<String, Short> aeoq) throws BusException { return null; }

        public Map<String, Boolean> DictionaryOB(Map<String, Boolean> aeob) throws BusException { return null; }

        public Map<String, String> DictionaryOO(Map<String, String> aeoo) throws BusException { return null; }

        public Map<Integer, AnnotatedTypesInterface.InnerStruct> AnnotatedDictionaryIR(Map<Integer, AnnotatedTypesInterface.InnerStruct> aeir) throws BusException { return null; }

        public Map<Integer, Integer> DictionaryIU(Map<Integer, Integer> aeiu) throws BusException { return null; }

        public Map<Integer, String> DictionaryIG(Map<Integer, String> aeig) throws BusException { return null; }

        public Map<Integer, Long> DictionaryIT(Map<Integer, Long> aeit) throws BusException { return null; }

        public Map<Integer, Short> DictionaryIQ(Map<Integer, Short> aeiq) throws BusException { return null; }

        public Map<Integer, String> DictionaryIO(Map<Integer, String> aeio) throws BusException { return null; }

        public Map<Long, Map<String, String>> DictionaryTAESS(Map<Long, Map<String, String>> aetaess) throws BusException { return null; }

        public Map<Long, AnnotatedTypesInterface.InnerStruct> DictionaryTR(Map<Long, AnnotatedTypesInterface.InnerStruct> aetr) throws BusException { return null; }

        public Map<Long, Double> DictionaryTD(Map<Long, Double> aetd) throws BusException { return null; }

        public Map<Long, Long> DictionaryTX(Map<Long, Long> aetx) throws BusException { return null; }

        public Map<Long, Byte> DictionaryTY(Map<Long, Byte> aety) throws BusException { return null; }

        public Map<Long, Integer> DictionaryTU(Map<Long, Integer> aetu) throws BusException { return null; }

        public Map<Long, String> DictionaryTG(Map<Long, String> aetg) throws BusException { return null; }

        public Map<Long, Integer> DictionaryTI(Map<Long, Integer> aeti) throws BusException { return null; }

        public Map<Long, Long> DictionaryTT(Map<Long, Long> aett) throws BusException { return null; }

        public Map<Long, Short> DictionaryTN(Map<Long, Short> aetn) throws BusException { return null; }

        public Map<Long, Variant> DictionaryTV(Map<Long, Variant> aetv) throws BusException { return null; }

        public Map<Long, String> DictionaryTS(Map<Long, String> aets) throws BusException { return null; }

        public Map<Long, byte[]> DictionaryTAY(Map<Long, byte[]> aetay) throws BusException { return null; }

        public Map<Long, Short> DictionaryTQ(Map<Long, Short> aetq) throws BusException { return null; }

        public Map<Long, Boolean> DictionaryTB(Map<Long, Boolean> aetb) throws BusException { return null; }

        public Map<Long, String> DictionaryTO(Map<Long, String> aeto) throws BusException { return null; }

        public Map<String, String> getDictionarySS() throws BusException {
            TreeMap<String, String> aess = new TreeMap<String, String>();
            aess.put("six", "six");
            return null;
        }

        public void setDictionarySS(Map<String, String> aess) throws BusException {}

        public EnumType EnumY(EnumType y) throws BusException { return null; }

        public EnumType EnumN(EnumType n) throws BusException { return null; }

        public EnumType EnumQ(EnumType q) throws BusException { return null; }

        public EnumType EnumI(EnumType i) throws BusException { return null; }

        public EnumType EnumU(EnumType u) throws BusException { return null; }

        public EnumType EnumX(EnumType x) throws BusException { return null; }

        public EnumType EnumT(EnumType t) throws BusException { return null; }

        public TreeMap<String, String>[] TreeDictionaryArray(TreeMap<String, String>[] aaess) throws BusException {
            return null;
        }

        public TwoByteArrays TwoByteArrays(TwoByteArrays rayay) throws BusException { return null; }
    }

    private BusAttachment bus;

    private Service service;
    private NullService nullService;

    private ProxyBusObject remoteObj;
    private ProxyBusObject remoteNullReturnsObj;

    public void setUp() throws Exception {
        bus = new BusAttachment(getClass().getName());
        Status status = bus.connect();
        assertEquals(Status.OK, status);

        service = new Service();
        status = bus.registerBusObject(service, "/testobject");
        assertEquals(Status.OK, status);

        nullService = new NullService();
        status = bus.registerBusObject(nullService, "/testnullobject");
        assertEquals(Status.OK, status);

        DBusProxyObj control = bus.getDBusProxyObj();
        DBusProxyObj.RequestNameResult res = control.RequestName("org.alljoyn.bus.MarshalTest",
                                                                DBusProxyObj.REQUEST_NAME_NO_FLAGS);
        assertEquals(DBusProxyObj.RequestNameResult.PrimaryOwner, res);

        Class[] ifaces = { InferredTypesInterface.class, AnnotatedTypesInterface.class };
        remoteObj = bus.getProxyBusObject("org.alljoyn.bus.MarshalTest", "/testobject", BusAttachment.SESSION_ID_ANY, ifaces);
        remoteNullReturnsObj = bus.getProxyBusObject("org.alljoyn.bus.MarshalTest", "/testnullobject",
                                                     BusAttachment.SESSION_ID_ANY, ifaces);
    }

    public void tearDown() throws Exception {
        remoteNullReturnsObj = null;
        remoteObj = null;

        DBusProxyObj control = bus.getDBusProxyObj();
        DBusProxyObj.ReleaseNameResult res = control.ReleaseName("org.alljoyn.bus.MarshalTest");
        assertEquals(DBusProxyObj.ReleaseNameResult.Released, res);

        bus.unregisterBusObject(nullService);
        nullService = null;

        bus.unregisterBusObject(service);
        service = null;

        bus.disconnect();
        bus = null;
    }

    public void testInferredTypes() throws Exception {
        InferredTypesInterface proxy = remoteObj.getInterface(InferredTypesInterface.class);

        /* Void */
        proxy.Void();

        /* Basic types */
        assertEquals(10, proxy.Byte((byte)10));
        assertEquals(true, proxy.Boolean(true));
        assertEquals(1, proxy.Int16((short)1));
        assertEquals(2, proxy.Int32(2));
        assertEquals(3, proxy.Int64(3));
        assertEquals(3.14159, proxy.Double(3.14159), 0.001);
        assertEquals("string", proxy.String("string"));

        /* Array types */
        byte[] ay = new byte[] { 1, 2 };
        assertArrayEquals(ay, proxy.ByteArray(ay));
        boolean[] ab = new boolean[] { true, false };
        boolean[] ab2 = proxy.BooleanArray(ab);
        for (int i = 0;  i < ab2.length; ++i) {
            assertEquals(ab2[i], ab[i]);
        }
        Boolean[] aB = new Boolean[] { true, false };
        Boolean[] aB2 = proxy.CapitalBooleanArray(aB);
        for (int i = 0;  i < aB2.length; ++i) {
            assertEquals(aB2[i], aB[i]);
        }
        short[] an = new short[] { 3, 4, 5 };
        assertArrayEquals(an, proxy.Int16Array(an));
        int[] ai = new int[] { 7 };
        assertArrayEquals(ai, proxy.Int32Array(ai));
        long[] ax = new long[] { 8, 9, 10, 11 };
        assertArrayEquals(ax, proxy.Int64Array(ax));
        double[] ad = new double[] { 0.1, 0.2, 0.3 };
        assertArrayEquals(ad, proxy.DoubleArray(ad), 0.01);
        String[] as = new String[] { "one", "two" };
        assertArrayEquals(as, proxy.StringArray(as));
        byte[][] aay = new byte[][] { new byte[] { 1 }, new byte[] { 2 } };
        assertArrayEquals(aay, proxy.ArrayArray(aay));
        InferredTypesInterface.InnerStruct[] ar = new InferredTypesInterface.InnerStruct[] {
            new InferredTypesInterface.InnerStruct(12), new InferredTypesInterface.InnerStruct(13) };
        assertArrayEquals(ar, proxy.InferredStructArray(ar));
        Variant[] av = new Variant[] { new Variant(new String("three")) };
        assertArrayEquals(av, proxy.VariantArray(av));
        @SuppressWarnings(value="unchecked")
        TreeMap<String, String>[] aaess = (TreeMap<String, String>[]) new TreeMap<?, ?>[2];
        aaess[0] = new TreeMap<String, String>();
        aaess[0].put("a1", "value1");
        aaess[0].put("a2", "value2");
        aaess[1] = new TreeMap<String, String>();
        aaess[1].put("b1", "value3");
        assertArrayEquals(aaess, proxy.DictionaryArray(aaess));

        /* Struct types */
        TreeMap<String, String> ae = new TreeMap<String, String>();
        ae.put("fourteen", "fifteen");
        ae.put("sixteen", "seventeen");
        InferredTypesInterface.Struct r =
            new InferredTypesInterface.Struct((byte)1, false, (short)1, 2, (long)3, 4.1, "five",
                                              new byte[] { 6 }, new boolean[] { true }, new short[] { 7 },
                                              new int[] { 8 }, new long[] { 10 }, new double[] { 10.1 },
                                              new String[] { "eleven" },
                                              new InferredTypesInterface.InnerStruct(12),
                                              new Variant(new String("thirteen")), ae);
        assertEquals(r, proxy.InferredStruct(r));

        /* Variant types */
        Variant v = new Variant((byte)1);
        assertEquals(v.getObject(byte.class), proxy.Variant(v).getObject(byte.class));
        assertEquals("y", proxy.Variant(v).getSignature());
        v = new Variant(true);
        assertEquals(v.getObject(boolean.class), proxy.Variant(v).getObject(boolean.class));
        assertEquals("b", proxy.Variant(v).getSignature());
        v = new Variant((short)2);
        assertEquals(v.getObject(short.class), proxy.Variant(v).getObject(short.class));
        assertEquals("n", proxy.Variant(v).getSignature());
        v = new Variant(3);
        assertEquals(v.getObject(int.class), proxy.Variant(v).getObject(int.class));
        assertEquals("i", proxy.Variant(v).getSignature());
        v = new Variant((long)4);
        assertEquals(v.getObject(long.class), proxy.Variant(v).getObject(long.class));
        assertEquals("x", proxy.Variant(v).getSignature());
        v = new Variant(4.1);
        assertEquals(v.getObject(double.class), proxy.Variant(v).getObject(double.class));
        assertEquals("d", proxy.Variant(v).getSignature());
        v = new Variant("five");
        assertEquals(v.getObject(String.class), proxy.Variant(v).getObject(String.class));
        assertEquals("s", proxy.Variant(v).getSignature());
        v = new Variant(new byte[] { 6 });
        assertArrayEquals(v.getObject(byte[].class), proxy.Variant(v).getObject(byte[].class));
        assertEquals("ay", proxy.Variant(v).getSignature());
        v = new Variant(new boolean[] { true });
        Variant v2 = proxy.Variant(v);
        for (int i = 0; i < v2.getObject(boolean[].class).length; ++i) {
            assertEquals(v2.getObject(boolean[].class)[i], v.getObject(boolean[].class)[i]);
            assertEquals("ab", proxy.Variant(v).getSignature());
        }
        v = new Variant(new short[] { 7 });
        assertArrayEquals(v.getObject(short[].class), proxy.Variant(v).getObject(short[].class));
        assertEquals("an", proxy.Variant(v).getSignature());
        v = new Variant(new int[] { 8 });
        assertArrayEquals(v.getObject(int[].class), proxy.Variant(v).getObject(int[].class));
        assertEquals("ai", proxy.Variant(v).getSignature());
        v = new Variant(new long[] { 10 });
        assertArrayEquals(v.getObject(long[].class), proxy.Variant(v).getObject(long[].class));
        assertEquals("ax", proxy.Variant(v).getSignature());
        v = new Variant(new double[] { 10.1 });
        assertArrayEquals(v.getObject(double[].class), proxy.Variant(v).getObject(double[].class), 0.01);
        assertEquals("ad", proxy.Variant(v).getSignature());
        v = new Variant(new String[] { "eleven" });
        assertArrayEquals(v.getObject(String[].class), proxy.Variant(v).getObject(String[].class));
        assertEquals("as", proxy.Variant(v).getSignature());
        v = new Variant(new InferredTypesInterface.InnerStruct(12));
        assertEquals(v.getObject(InferredTypesInterface.InnerStruct.class),
                     proxy.Variant(v).getObject(InferredTypesInterface.InnerStruct.class));
        assertEquals("(i)", proxy.Variant(v).getSignature());
        v = new Variant(new Variant(new String("thirteen")));
        assertEquals(v.getObject(Variant.class), proxy.Variant(v).getObject(Variant.class));
        assertEquals("v", proxy.Variant(v).getSignature());

        /* Dictionary types */
        TreeMap<Byte, Byte> aeyy = new TreeMap<Byte, Byte>();
        aeyy.put((byte)1, (byte)1);
        assertEquals(aeyy, proxy.DictionaryYY(aeyy));
        TreeMap<Byte, Boolean> aeyb = new TreeMap<Byte, Boolean>();
        aeyb.put((byte)1, true);
        assertEquals(aeyb, proxy.DictionaryYB(aeyb));
        TreeMap<Byte, Short> aeyn = new TreeMap<Byte, Short>();
        aeyn.put((byte)1, (short)2);
        assertEquals(aeyn, proxy.DictionaryYN(aeyn));
        TreeMap<Byte, Integer> aeyi = new TreeMap<Byte, Integer>();
        aeyi.put((byte)1, 3);
        assertEquals(aeyi, proxy.DictionaryYI(aeyi));
        TreeMap<Byte, Long> aeyx = new TreeMap<Byte, Long>();
        aeyx.put((byte)1, (long)4);
        assertEquals(aeyx, proxy.DictionaryYX(aeyx));
        TreeMap<Byte, Double> aeyd = new TreeMap<Byte, Double>();
        aeyd.put((byte)1, 5.1);
        assertEquals(aeyd, proxy.DictionaryYD(aeyd));
        TreeMap<Byte, String> aeys = new TreeMap<Byte, String>();
        aeys.put((byte)1, "six");
        assertEquals(aeys, proxy.DictionaryYS(aeys));
        TreeMap<Byte, byte[]> aeyay = new TreeMap<Byte, byte[]>();
        aeyay.put((byte)1, new byte[] { (byte)7 });
        Map<Byte, byte[]> ret_aeyay = proxy.DictionaryYAY(aeyay);
        assertEquals(ret_aeyay.keySet(), aeyay.keySet());
        for (Map.Entry<Byte, byte[]> e: aeyay.entrySet()) {
            byte[] value = e.getValue();
            byte[] value2 = ret_aeyay.get(e.getKey());
            assertArrayEquals(value2, value);
        }
        TreeMap<Byte, InferredTypesInterface.InnerStruct> aeyr = new TreeMap<Byte, InferredTypesInterface.InnerStruct>();
        aeyr.put((byte)1, new InferredTypesInterface.InnerStruct(8));
        assertEquals(aeyr, proxy.InferredDictionaryYR(aeyr));
        TreeMap<Byte, Variant> aeyv = new TreeMap<Byte, Variant>();
        aeyv.put((byte)1, new Variant(new String("nine")));
        assertEquals(aeyv, proxy.DictionaryYV(aeyv));
        TreeMap<Byte, Map<String, String>> aeyaess = new TreeMap<Byte, Map<String, String>>();
        aeyaess.put((byte)1, ae);
        assertEquals(aeyaess, proxy.DictionaryYAESS(aeyaess));
        TreeMap<Boolean, Byte> aeby = new TreeMap<Boolean, Byte>();
        aeby.put(true, (byte)1);
        assertEquals(aeby, proxy.DictionaryBY(aeby));
        TreeMap<Boolean, Boolean> aebb = new TreeMap<Boolean, Boolean>();
        aebb.put(true, true);
        assertEquals(aebb, proxy.DictionaryBB(aebb));
        TreeMap<Boolean, Short> aebn = new TreeMap<Boolean, Short>();
        aebn.put(true, (short)2);
        assertEquals(aebn, proxy.DictionaryBN(aebn));
        TreeMap<Boolean, Integer> aebi = new TreeMap<Boolean, Integer>();
        aebi.put(true, 3);
        assertEquals(aebi, proxy.DictionaryBI(aebi));
        TreeMap<Boolean, Long> aebx = new TreeMap<Boolean, Long>();
        aebx.put(true, (long)4);
        assertEquals(aebx, proxy.DictionaryBX(aebx));
        TreeMap<Boolean, Double> aebd = new TreeMap<Boolean, Double>();
        aebd.put(true, 5.1);
        assertEquals(aebd, proxy.DictionaryBD(aebd));
        TreeMap<Boolean, String> aebs = new TreeMap<Boolean, String>();
        aebs.put(true, "six");
        assertEquals(aebs, proxy.DictionaryBS(aebs));
        TreeMap<Boolean, byte[]> aebay = new TreeMap<Boolean, byte[]>();
        aebay.put(true, new byte[] { (byte)7 });
        Map<Boolean, byte[]> ret_aebay = proxy.DictionaryBAY(aebay);
        assertEquals(ret_aebay.keySet(), aebay.keySet());
        for (Map.Entry<Boolean, byte[]> e: aebay.entrySet()) {
            byte[] value = e.getValue();
            byte[] value2 = ret_aebay.get(e.getKey());
            assertArrayEquals(value2, value);
        }
        TreeMap<Boolean, InferredTypesInterface.InnerStruct> aebr = new TreeMap<Boolean, InferredTypesInterface.InnerStruct>();
        aebr.put(true, new InferredTypesInterface.InnerStruct(8));
        assertEquals(aebr, proxy.InferredDictionaryBR(aebr));
        TreeMap<Boolean, Variant> aebv = new TreeMap<Boolean, Variant>();
        aebv.put(true, new Variant(new String("nine")));
        assertEquals(aebv, proxy.DictionaryBV(aebv));
        TreeMap<Boolean, Map<String, String>> aebaess = new TreeMap<Boolean, Map<String, String>>();
        aebaess.put(true, ae);
        assertEquals(aebaess, proxy.DictionaryBAESS(aebaess));
        TreeMap<Short, Byte> aeny = new TreeMap<Short, Byte>();
        aeny.put((short)2, (byte)1);
        assertEquals(aeny, proxy.DictionaryNY(aeny));
        TreeMap<Short, Boolean> aenb = new TreeMap<Short, Boolean>();
        aenb.put((short)2, true);
        assertEquals(aenb, proxy.DictionaryNB(aenb));
        TreeMap<Short, Short> aenn = new TreeMap<Short, Short>();
        aenn.put((short)2, (short)2);
        assertEquals(aenn, proxy.DictionaryNN(aenn));
        TreeMap<Short, Integer> aeni = new TreeMap<Short, Integer>();
        aeni.put((short)2, 3);
        assertEquals(aeni, proxy.DictionaryNI(aeni));
        TreeMap<Short, Long> aenx = new TreeMap<Short, Long>();
        aenx.put((short)2, (long)4);
        assertEquals(aenx, proxy.DictionaryNX(aenx));
        TreeMap<Short, Double> aend = new TreeMap<Short, Double>();
        aend.put((short)2, 5.1);
        assertEquals(aend, proxy.DictionaryND(aend));
        TreeMap<Short, String> aens = new TreeMap<Short, String>();
        aens.put((short)2, "six");
        assertEquals(aens, proxy.DictionaryNS(aens));
        TreeMap<Short, byte[]> aenay = new TreeMap<Short, byte[]>();
        aenay.put((short)2, new byte[] { (byte)7 });
        Map<Short, byte[]> ret_aenay = proxy.DictionaryNAY(aenay);
        assertEquals(ret_aenay.keySet(), aenay.keySet());
        for (Map.Entry<Short, byte[]> e: aenay.entrySet()) {
            byte[] value = e.getValue();
            byte[] value2 = ret_aenay.get(e.getKey());
            assertArrayEquals(value2, value);
        }
        TreeMap<Short, InferredTypesInterface.InnerStruct> aenr = new TreeMap<Short, InferredTypesInterface.InnerStruct>();
        aenr.put((short)2, new InferredTypesInterface.InnerStruct(8));
        assertEquals(aenr, proxy.InferredDictionaryNR(aenr));
        TreeMap<Short, Variant> aenv = new TreeMap<Short, Variant>();
        aenv.put((short)2, new Variant(new String("nine")));
        assertEquals(aenv, proxy.DictionaryNV(aenv));
        TreeMap<Short, Map<String, String>> aenaess = new TreeMap<Short, Map<String, String>>();
        aenaess.put((short)2, ae);
        assertEquals(aenaess, proxy.DictionaryNAESS(aenaess));
        TreeMap<Integer, Byte> aeiy = new TreeMap<Integer, Byte>();
        aeiy.put(3, (byte)1);
        assertEquals(aeiy, proxy.DictionaryIY(aeiy));
        TreeMap<Integer, Boolean> aeib = new TreeMap<Integer, Boolean>();
        aeib.put(3, true);
        assertEquals(aeib, proxy.DictionaryIB(aeib));
        TreeMap<Integer, Short> aein = new TreeMap<Integer, Short>();
        aein.put(3, (short)2);
        assertEquals(aein, proxy.DictionaryIN(aein));
        TreeMap<Integer, Integer> aeii = new TreeMap<Integer, Integer>();
        aeii.put(3, 3);
        assertEquals(aeii, proxy.DictionaryII(aeii));
        TreeMap<Integer, Long> aeix = new TreeMap<Integer, Long>();
        aeix.put(3, (long)4);
        assertEquals(aeix, proxy.DictionaryIX(aeix));
        TreeMap<Integer, Double> aeid = new TreeMap<Integer, Double>();
        aeid.put(3, 5.1);
        assertEquals(aeid, proxy.DictionaryID(aeid));
        TreeMap<Integer, String> aeis = new TreeMap<Integer, String>();
        aeis.put(3, "six");
        assertEquals(aeis, proxy.DictionaryIS(aeis));
        TreeMap<Integer, byte[]> aeiay = new TreeMap<Integer, byte[]>();
        aeiay.put(3, new byte[] { (byte)7 });
        Map<Integer, byte[]> ret_aeiay = proxy.DictionaryIAY(aeiay);
        assertEquals(ret_aeiay.keySet(), aeiay.keySet());
        for (Map.Entry<Integer, byte[]> e: aeiay.entrySet()) {
            byte[] value = e.getValue();
            byte[] value2 = ret_aeiay.get(e.getKey());
            assertArrayEquals(value2, value);
        }
        TreeMap<Integer, InferredTypesInterface.InnerStruct> aeir = new TreeMap<Integer, InferredTypesInterface.InnerStruct>();
        aeir.put(3, new InferredTypesInterface.InnerStruct(8));
        assertEquals(aeir, proxy.InferredDictionaryIR(aeir));
        TreeMap<Integer, Variant> aeiv = new TreeMap<Integer, Variant>();
        aeiv.put(3, new Variant(new String("nine")));
        assertEquals(aeiv, proxy.DictionaryIV(aeiv));
        TreeMap<Integer, Map<String, String>> aeiaess = new TreeMap<Integer, Map<String, String>>();
        aeiaess.put(3, ae);
        assertEquals(aeiaess, proxy.DictionaryIAESS(aeiaess));
        TreeMap<Long, Byte> aexy = new TreeMap<Long, Byte>();
        aexy.put((long)4, (byte)1);
        assertEquals(aexy, proxy.DictionaryXY(aexy));
        TreeMap<Long, Boolean> aexb = new TreeMap<Long, Boolean>();
        aexb.put((long)4, true);
        assertEquals(aexb, proxy.DictionaryXB(aexb));
        TreeMap<Long, Short> aexn = new TreeMap<Long, Short>();
        aexn.put((long)4, (short)2);
        assertEquals(aexn, proxy.DictionaryXN(aexn));
        TreeMap<Long, Integer> aexi = new TreeMap<Long, Integer>();
        aexi.put((long)4, 3);
        assertEquals(aexi, proxy.DictionaryXI(aexi));
        TreeMap<Long, Long> aexx = new TreeMap<Long, Long>();
        aexx.put((long)4, (long)4);
        assertEquals(aexx, proxy.DictionaryXX(aexx));
        TreeMap<Long, Double> aexd = new TreeMap<Long, Double>();
        aexd.put((long)4, 5.1);
        assertEquals(aexd, proxy.DictionaryXD(aexd));
        TreeMap<Long, String> aexs = new TreeMap<Long, String>();
        aexs.put((long)4, "six");
        assertEquals(aexs, proxy.DictionaryXS(aexs));
        TreeMap<Long, byte[]> aexay = new TreeMap<Long, byte[]>();
        aexay.put((long)4, new byte[] { (byte)7 });
        Map<Long, byte[]> ret_aexay = proxy.DictionaryXAY(aexay);
        assertEquals(ret_aexay.keySet(), aexay.keySet());
        for (Map.Entry<Long, byte[]> e: aexay.entrySet()) {
            byte[] value = e.getValue();
            byte[] value2 = ret_aexay.get(e.getKey());
            assertArrayEquals(value2, value);
        }
        TreeMap<Long, InferredTypesInterface.InnerStruct> aexr = new TreeMap<Long, InferredTypesInterface.InnerStruct>();
        aexr.put((long)4, new InferredTypesInterface.InnerStruct(8));
        assertEquals(aexr, proxy.InferredDictionaryXR(aexr));
        TreeMap<Long, Variant> aexv = new TreeMap<Long, Variant>();
        aexv.put((long)4, new Variant(new String("nine")));
        assertEquals(aexv, proxy.DictionaryXV(aexv));
        TreeMap<Long, Map<String, String>> aexaess = new TreeMap<Long, Map<String, String>>();
        aexaess.put((long)4, ae);
        assertEquals(aexaess, proxy.DictionaryXAESS(aexaess));
        TreeMap<Double, Byte> aedy = new TreeMap<Double, Byte>();
        aedy.put(5.1, (byte)1);
        assertEquals(aedy, proxy.DictionaryDY(aedy));
        TreeMap<Double, Boolean> aedb = new TreeMap<Double, Boolean>();
        aedb.put(5.1, true);
        assertEquals(aedb, proxy.DictionaryDB(aedb));
        TreeMap<Double, Short> aedn = new TreeMap<Double, Short>();
        aedn.put(5.1, (short)2);
        assertEquals(aedn, proxy.DictionaryDN(aedn));
        TreeMap<Double, Integer> aedi = new TreeMap<Double, Integer>();
        aedi.put(5.1, 3);
        assertEquals(aedi, proxy.DictionaryDI(aedi));
        TreeMap<Double, Long> aedx = new TreeMap<Double, Long>();
        aedx.put(5.1, (long)4);
        assertEquals(aedx, proxy.DictionaryDX(aedx));
        TreeMap<Double, Double> aedd = new TreeMap<Double, Double>();
        aedd.put(5.1, 5.1);
        assertEquals(aedd, proxy.DictionaryDD(aedd));
        TreeMap<Double, String> aeds = new TreeMap<Double, String>();
        aeds.put(5.1, "six");
        assertEquals(aeds, proxy.DictionaryDS(aeds));
        TreeMap<Double, byte[]> aeday = new TreeMap<Double, byte[]>();
        aeday.put(5.1, new byte[] { (byte)7 });
        Map<Double, byte[]> ret_aeday = proxy.DictionaryDAY(aeday);
        assertEquals(ret_aeday.keySet(), aeday.keySet());
        for (Map.Entry<Double, byte[]> e: aeday.entrySet()) {
            byte[] value = e.getValue();
            byte[] value2 = ret_aeday.get(e.getKey());
            assertArrayEquals(value2, value);
        }
        TreeMap<Double, InferredTypesInterface.InnerStruct> aedr = new TreeMap<Double, InferredTypesInterface.InnerStruct>();
        aedr.put(5.1, new InferredTypesInterface.InnerStruct(8));
        assertEquals(aedr, proxy.InferredDictionaryDR(aedr));
        TreeMap<Double, Variant> aedv = new TreeMap<Double, Variant>();
        aedv.put(5.1, new Variant(new String("nine")));
        assertEquals(aedv, proxy.DictionaryDV(aedv));
        TreeMap<Double, Map<String, String>> aedaess = new TreeMap<Double, Map<String, String>>();
        aedaess.put(5.1, ae);
        assertEquals(aedaess, proxy.DictionaryDAESS(aedaess));
        TreeMap<String, Byte> aesy = new TreeMap<String, Byte>();
        aesy.put("six", (byte)1);
        assertEquals(aesy, proxy.DictionarySY(aesy));
        TreeMap<String, Boolean> aesb = new TreeMap<String, Boolean>();
        aesb.put("six", true);
        assertEquals(aesb, proxy.DictionarySB(aesb));
        TreeMap<String, Short> aesn = new TreeMap<String, Short>();
        aesn.put("six", (short)2);
        assertEquals(aesn, proxy.DictionarySN(aesn));
        TreeMap<String, Integer> aesi = new TreeMap<String, Integer>();
        aesi.put("six", 3);
        assertEquals(aesi, proxy.DictionarySI(aesi));
        TreeMap<String, Long> aesx = new TreeMap<String, Long>();
        aesx.put("six", (long)4);
        assertEquals(aesx, proxy.DictionarySX(aesx));
        TreeMap<String, Double> aesd = new TreeMap<String, Double>();
        aesd.put("six", 5.1);
        assertEquals(aesd, proxy.DictionarySD(aesd));
        TreeMap<String, String> aess = new TreeMap<String, String>();
        aess.put("six", "six");
        assertEquals(aess, proxy.DictionarySS(aess));
        TreeMap<String, byte[]> aesay = new TreeMap<String, byte[]>();
        aesay.put("six", new byte[] { (byte)7 });
        Map<String, byte[]> ret_aesay = proxy.DictionarySAY(aesay);
        assertEquals(ret_aesay.keySet(), aesay.keySet());
        for (Map.Entry<String, byte[]> e: aesay.entrySet()) {
            byte[] value = e.getValue();
            byte[] value2 = ret_aesay.get(e.getKey());
            assertArrayEquals(value2, value);
        }
        TreeMap<String, InferredTypesInterface.InnerStruct> aesr = new TreeMap<String, InferredTypesInterface.InnerStruct>();
        aesr.put("six", new InferredTypesInterface.InnerStruct(8));
        assertEquals(aesr, proxy.InferredDictionarySR(aesr));
        TreeMap<String, Variant> aesv = new TreeMap<String, Variant>();
        aesv.put("six", new Variant(new String("nine")));
        assertEquals(aesv, proxy.DictionarySV(aesv));
        TreeMap<String, Map<String, String>> aesaess = new TreeMap<String, Map<String, String>>();
        aesaess.put("six", ae);
        assertEquals(aesaess, proxy.DictionarySAESS(aesaess));

        assertEquals(aess, proxy.getDictionarySS());
        proxy.setDictionarySS(aess);
    }

    public void testEmptyArrays() throws Exception {
        InferredTypesInterface proxy = remoteObj.getInterface(InferredTypesInterface.class);

        byte[] ay = new byte[] {};
        assertArrayEquals(ay, proxy.ByteArray(ay));

        String[] as = new String[] {};
        assertArrayEquals(as, proxy.StringArray(as));

        /* This test succeeds under AllJoyn but fails under DBus. */
        TreeMap<Byte, Byte> aeyy = new TreeMap<Byte, Byte>();
        assertEquals(aeyy, proxy.DictionaryYY(aeyy));
    }

    public void testEmptyStrings() throws Exception {
        AnnotatedTypesInterface proxy = remoteObj.getInterface(AnnotatedTypesInterface.class);
        proxy.String("");
        proxy.Signature("");
        boolean thrown = false;
        try {
            proxy.ObjectPath("");
        } catch (BusException ex) {
            // null is not a valid object path
            thrown = true;
        }
        assertTrue(thrown);
    }

    public void testAnnotatedTypes() throws Exception {
        AnnotatedTypesInterface proxy = remoteObj.getInterface(AnnotatedTypesInterface.class);

        /* Void */
        proxy.Void();

        /* Basic types */
        assertEquals(10, proxy.Byte((byte)10));
        assertEquals(true, proxy.Boolean(true));
        assertEquals(4, proxy.Uint16((short)4));
        assertEquals(1, proxy.Int16((short)1));
        assertEquals(2, proxy.Int32(2));
        assertEquals(5, proxy.Uint32(5));
        assertEquals(3, proxy.Int64(3));
        assertEquals(6, proxy.Uint64(6));
        assertEquals(3.14159, proxy.Double(3.14159), 0.001);
        assertEquals("string", proxy.String("string"));
        assertEquals("/path", proxy.ObjectPath("/path"));
        assertEquals("g", proxy.Signature("g"));

        /* Array types */
        @SuppressWarnings(value="unchecked")
        TreeMap<String, String>[] aaess = (TreeMap<String, String>[]) new TreeMap<?, ?>[2];
        aaess[0] = new TreeMap<String, String>();
        aaess[0].put("a1", "value1");
        aaess[0].put("a2", "value2");
        aaess[1] = new TreeMap<String, String>();
        aaess[1].put("b1", "value3");
        assertArrayEquals(aaess, proxy.DictionaryArray(aaess));
        AnnotatedTypesInterface.InnerStruct[] ar = new AnnotatedTypesInterface.InnerStruct[] { new AnnotatedTypesInterface.InnerStruct(22), new AnnotatedTypesInterface.InnerStruct(23) };
        assertArrayEquals(ar, proxy.AnnotatedStructArray(ar));
        double[] ad = new double[] { 15.1, 15.2 };
        assertArrayEquals(ad, proxy.DoubleArray(ad), 0.01);
        long[] ax = new long[] { (long)11, (long)12 };
        assertArrayEquals(ax, proxy.Int64Array(ax));
        byte[] ay = new byte[] { (byte)1, (byte)2 };
        assertArrayEquals(ay, proxy.ByteArray(ay));
        int[] au = new int[] { 9, 10 };
        assertArrayEquals(au, proxy.Uint32Array(au));
        String[] ag = new String[] { "g", "o" };
        assertArrayEquals(ag, proxy.SignatureArray(ag));
        int[] ai = new int[] { 7, 8 };
        assertArrayEquals(ai, proxy.Int32Array(ai));
        long[] at = new long[] { (long)13, (long)14 };
        assertArrayEquals(at, proxy.Uint64Array(at));
        short[] an = new short[] { (short)3, (short)4 };
        assertArrayEquals(an, proxy.Int16Array(an));
        Variant[] av = new Variant[] { new Variant(new String("twentyfour")), new Variant(new String("twentyfive")) };
        assertArrayEquals(av, proxy.VariantArray(av));
        String[] as = new String[] { "sixteen", "seventeen" };
        assertArrayEquals(as, proxy.StringArray(as));
        byte[][] aay = new byte[][] { new byte[] { 18, 19 }, new byte[] { 20, 21 } };
        assertArrayEquals(aay, proxy.ArrayArray(aay));
        short[] aq = new short[] { (short)5, (short)6 };
        assertArrayEquals(aq, proxy.Uint16Array(aq));
        boolean[] ab = new boolean[] { true, false };
        boolean[] ab2 = proxy.BooleanArray(ab);
        for (int i = 0;  i < ab2.length; ++i) {
            assertEquals(ab2[i], ab[i]);
        }
        Boolean[] aB = new Boolean[] { true, false };
        Boolean[] aB2 = proxy.CapitalBooleanArray(aB);
        for (int i = 0;  i < aB2.length; ++i) {
            assertEquals(aB2[i], aB[i]);
        }
        String[] ao = new String[] { "/path1", "/path2" };
        assertArrayEquals(ao, proxy.ObjectPathArray(ao));

        /* Struct types */
        TreeMap<String, String> ae = new TreeMap<String, String>();
        ae.put("fourteen", "fifteen");
        ae.put("sixteen", "seventeen");
        AnnotatedTypesInterface.Struct r =
            new AnnotatedTypesInterface.Struct((byte)0, false, (short)1, (short)2, 3, 4, (long)5, (long)6,
                                               7.1, "eight", "/nine", "t",
                                               new byte[] { 11 }, new boolean[] { true }, new short[] { 12 },
                                               new short[] { 13 }, new int[] { 14 }, new int[] { 15 },
                                               new long[] { 16 }, new long[] { 17 }, new double[] { 18.1 },
                                               new String[] { "nineteen" }, new String[] { "/twenty" },
                                               new String[] { "t" }, new AnnotatedTypesInterface.InnerStruct(12),
                                               new Variant(new String("thirteen")), ae);
        assertEquals(r, proxy.AnnotatedStruct(r));

        /* Variant types */
        Variant v = new Variant((byte)1, "y");
        assertEquals(v.getObject(byte.class), proxy.Variant(v).getObject(byte.class));
        v = new Variant(true, "b");
        assertEquals(v.getObject(boolean.class), proxy.Variant(v).getObject(boolean.class));
        v = new Variant((short)2, "n");
        assertEquals(v.getObject(short.class), proxy.Variant(v).getObject(short.class));
        v = new Variant((short)2, "q");
        assertEquals(v.getObject(short.class), proxy.Variant(v).getObject(short.class));
        v = new Variant(3, "i");
        assertEquals(v.getObject(int.class), proxy.Variant(v).getObject(int.class));
        v = new Variant(3, "u");
        assertEquals(v.getObject(int.class), proxy.Variant(v).getObject(int.class));
        v = new Variant((long)4, "x");
        assertEquals(v.getObject(long.class), proxy.Variant(v).getObject(long.class));
        v = new Variant((long)4, "t");
        assertEquals(v.getObject(long.class), proxy.Variant(v).getObject(long.class));
        v = new Variant(4.1, "d");
        assertEquals(v.getObject(double.class), proxy.Variant(v).getObject(double.class));
        v = new Variant("five", "s");
        assertEquals(v.getObject(String.class), proxy.Variant(v).getObject(String.class));
        v = new Variant("/six", "o");
        assertEquals(v.getObject(String.class), proxy.Variant(v).getObject(String.class));
        v = new Variant("b", "g");
        assertEquals(v.getObject(String.class), proxy.Variant(v).getObject(String.class));
        v = new Variant(new byte[] { 6 }, "ay");
        assertArrayEquals(v.getObject(byte[].class), proxy.Variant(v).getObject(byte[].class));
        v = new Variant(new boolean[] { true }, "ab");
        Variant v2 = proxy.Variant(v);
        for (int i = 0; i < v2.getObject(boolean[].class).length; ++i) {
            assertEquals(v2.getObject(boolean[].class)[i], v.getObject(boolean[].class)[i]);
        }
        v = new Variant(new short[] { 7 }, "an");
        assertArrayEquals(v.getObject(short[].class), proxy.Variant(v).getObject(short[].class));
        v = new Variant(new short[] { 7 }, "aq");
        assertArrayEquals(v.getObject(short[].class), proxy.Variant(v).getObject(short[].class));
        v = new Variant(new int[] { 8 }, "ai");
        assertArrayEquals(v.getObject(int[].class), proxy.Variant(v).getObject(int[].class));
        v = new Variant(new int[] { 8 }, "au");
        assertArrayEquals(v.getObject(int[].class), proxy.Variant(v).getObject(int[].class));
        v = new Variant(new long[] { 10 }, "ax");
        assertArrayEquals(v.getObject(long[].class), proxy.Variant(v).getObject(long[].class));
        v = new Variant(new long[] { 10 }, "at");
        assertArrayEquals(v.getObject(long[].class), proxy.Variant(v).getObject(long[].class));
        v = new Variant(new double[] { 10.1 }, "ad");
        assertArrayEquals(v.getObject(double[].class), proxy.Variant(v).getObject(double[].class), 0.01);
        v = new Variant(new String[] { "eleven" }, "as");
        assertArrayEquals(v.getObject(String[].class), proxy.Variant(v).getObject(String[].class));
        v = new Variant(new String[] { "/twelve" }, "ao");
        assertArrayEquals(v.getObject(String[].class), proxy.Variant(v).getObject(String[].class));
        v = new Variant(new String[] { "ag" }, "ag");
        assertArrayEquals(v.getObject(String[].class), proxy.Variant(v).getObject(String[].class));
        v = new Variant(new AnnotatedTypesInterface.InnerStruct(12), "r");
        assertEquals(v.getObject(AnnotatedTypesInterface.InnerStruct.class),
                     proxy.Variant(v).getObject(AnnotatedTypesInterface.InnerStruct.class));
        v = new Variant(new Variant(new String("thirteen")), "v");
        assertEquals(v.getObject(Variant.class), proxy.Variant(v).getObject(Variant.class));

    }

    public void testAnnotatedDictionaryTypes() throws Exception {
        AnnotatedTypesInterface proxy = remoteObj.getInterface(AnnotatedTypesInterface.class);

        TreeMap<String, String> ae = new TreeMap<String, String>();
        ae.put("fourteen", "fifteen");
        ae.put("sixteen", "seventeen");

        /* Dictionary types */
        TreeMap<Short, Map<String, String>> aenaess = new TreeMap<Short, Map<String, String>>();
        aenaess.put((short)2, ae);
        assertEquals(aenaess, proxy.DictionaryNAESS(aenaess));
        TreeMap<Short, AnnotatedTypesInterface.InnerStruct> aenr = new TreeMap<Short, AnnotatedTypesInterface.InnerStruct>();
        aenr.put((short)2, new AnnotatedTypesInterface.InnerStruct(8));
        assertEquals(aenr, proxy.AnnotatedDictionaryNR(aenr));
        TreeMap<Short, Double> aend = new TreeMap<Short, Double>();
        aend.put((short)2, 5.1);
        assertEquals(aend, proxy.DictionaryND(aend));
        TreeMap<Short, Long> aenx = new TreeMap<Short, Long>();
        aenx.put((short)2, (long)4);
        assertEquals(aenx, proxy.DictionaryNX(aenx));
        TreeMap<Short, Byte> aeny = new TreeMap<Short, Byte>();
        aeny.put((short)2, (byte)1);
        assertEquals(aeny, proxy.DictionaryNY(aeny));
        TreeMap<Short, Integer> aenu = new TreeMap<Short, Integer>();
        aenu.put((short)2, 4);
        assertEquals(aenu, proxy.DictionaryNU(aenu));
        TreeMap<Short, String> aeng = new TreeMap<Short, String>();
        aeng.put((short)2, "g");
        assertEquals(aeng, proxy.DictionaryNG(aeng));
        TreeMap<Short, Integer> aeni = new TreeMap<Short, Integer>();
        aeni.put((short)2, 3);
        assertEquals(aeni, proxy.DictionaryNI(aeni));
        TreeMap<Short, Long> aent = new TreeMap<Short, Long>();
        aent.put((short)2, (long)5);
        assertEquals(aent, proxy.DictionaryNT(aent));
        TreeMap<Short, Short> aenn = new TreeMap<Short, Short>();
        aenn.put((short)2, (short)2);
        assertEquals(aenn, proxy.DictionaryNN(aenn));
        TreeMap<Short, Variant> aenv = new TreeMap<Short, Variant>();
        aenv.put((short)2, new Variant(new String("nine")));
        assertEquals(aenv, proxy.DictionaryNV(aenv));
        TreeMap<Short, String> aens = new TreeMap<Short, String>();
        aens.put((short)2, "six");
        assertEquals(aens, proxy.DictionaryNS(aens));
        TreeMap<Short, byte[]> aenay = new TreeMap<Short, byte[]>();
        aenay.put((short)2, new byte[] { (byte)7 });
        Map<Short, byte[]> ret_aenay = proxy.DictionaryNAY(aenay);
        assertEquals(ret_aenay.keySet(), aenay.keySet());
        for (Map.Entry<Short, byte[]> e: aenay.entrySet()) {
            byte[] value = e.getValue();
            byte[] value2 = ret_aenay.get(e.getKey());
            assertArrayEquals(value2, value);
        }
        TreeMap<Short, Short> aenq = new TreeMap<Short, Short>();
        aenq.put((short)2, (short)3);
        assertEquals(aenq, proxy.DictionaryNQ(aenq));
        TreeMap<Short, Boolean> aenb = new TreeMap<Short, Boolean>();
        aenb.put((short)2, true);
        assertEquals(aenb, proxy.DictionaryNB(aenb));
        TreeMap<Short, String> aeno = new TreeMap<Short, String>();
        aeno.put((short)2, "/seven");
        assertEquals(aeno, proxy.DictionaryNO(aeno));
        TreeMap<Double, Map<String, String>> aedaess = new TreeMap<Double, Map<String, String>>();
        aedaess.put(5.1, ae);
        assertEquals(aedaess, proxy.DictionaryDAESS(aedaess));
        TreeMap<Double, AnnotatedTypesInterface.InnerStruct> aedr = new TreeMap<Double, AnnotatedTypesInterface.InnerStruct>();
        aedr.put(5.1, new AnnotatedTypesInterface.InnerStruct(8));
        assertEquals(aedr, proxy.AnnotatedDictionaryDR(aedr));
        TreeMap<Double, Double> aedd = new TreeMap<Double, Double>();
        aedd.put(5.1, 5.1);
        assertEquals(aedd, proxy.DictionaryDD(aedd));
        TreeMap<Double, Long> aedx = new TreeMap<Double, Long>();
        aedx.put(5.1, (long)4);
        assertEquals(aedx, proxy.DictionaryDX(aedx));
        TreeMap<Double, Byte> aedy = new TreeMap<Double, Byte>();
        aedy.put(5.1, (byte)1);
        assertEquals(aedy, proxy.DictionaryDY(aedy));
        TreeMap<Double, Integer> aedu = new TreeMap<Double, Integer>();
        aedu.put(5.1, 4);
        assertEquals(aedu, proxy.DictionaryDU(aedu));
        TreeMap<Double, String> aedg = new TreeMap<Double, String>();
        aedg.put(5.1, "g");
        assertEquals(aedg, proxy.DictionaryDG(aedg));
        TreeMap<Double, Integer> aedi = new TreeMap<Double, Integer>();
        aedi.put(5.1, 3);
        assertEquals(aedi, proxy.DictionaryDI(aedi));
        TreeMap<Double, Long> aedt = new TreeMap<Double, Long>();
        aedt.put(5.1, (long)5);
        assertEquals(aedt, proxy.DictionaryDT(aedt));
        TreeMap<Double, Short> aedn = new TreeMap<Double, Short>();
        aedn.put(5.1, (short)2);
        assertEquals(aedn, proxy.DictionaryDN(aedn));
        TreeMap<Double, Variant> aedv = new TreeMap<Double, Variant>();
        aedv.put(5.1, new Variant(new String("nine")));
        assertEquals(aedv, proxy.DictionaryDV(aedv));
        TreeMap<Double, String> aeds = new TreeMap<Double, String>();
        aeds.put(5.1, "six");
        assertEquals(aeds, proxy.DictionaryDS(aeds));
        TreeMap<Double, byte[]> aeday = new TreeMap<Double, byte[]>();
        aeday.put(5.1, new byte[] { (byte)7 });
        Map<Double, byte[]> ret_aeday = proxy.DictionaryDAY(aeday);
        assertEquals(ret_aeday.keySet(), aeday.keySet());
        for (Map.Entry<Double, byte[]> e: aeday.entrySet()) {
            byte[] value = e.getValue();
            byte[] value2 = ret_aeday.get(e.getKey());
            assertArrayEquals(value2, value);
        }
        TreeMap<Double, Short> aedq = new TreeMap<Double, Short>();
        aedq.put(5.1, (short)3);
        assertEquals(aedq, proxy.DictionaryDQ(aedq));
        TreeMap<Double, Boolean> aedb = new TreeMap<Double, Boolean>();
        aedb.put(5.1, true);
        assertEquals(aedb, proxy.DictionaryDB(aedb));
        TreeMap<Double, String> aedo = new TreeMap<Double, String>();
        aedo.put(5.1, "/seven");
        assertEquals(aedo, proxy.DictionaryDO(aedo));
        TreeMap<Long, Map<String, String>> aexaess = new TreeMap<Long, Map<String, String>>();
        aexaess.put((long)4, ae);
        assertEquals(aexaess, proxy.DictionaryXAESS(aexaess));
        TreeMap<Long, AnnotatedTypesInterface.InnerStruct> aexr = new TreeMap<Long, AnnotatedTypesInterface.InnerStruct>();
        aexr.put((long)4, new AnnotatedTypesInterface.InnerStruct(8));
        assertEquals(aexr, proxy.AnnotatedDictionaryXR(aexr));
        TreeMap<Long, Double> aexd = new TreeMap<Long, Double>();
        aexd.put((long)4, 5.1);
        assertEquals(aexd, proxy.DictionaryXD(aexd));
        TreeMap<Long, Long> aexx = new TreeMap<Long, Long>();
        aexx.put((long)4, (long)4);
        assertEquals(aexx, proxy.DictionaryXX(aexx));
        TreeMap<Long, Byte> aexy = new TreeMap<Long, Byte>();
        aexy.put((long)4, (byte)1);
        assertEquals(aexy, proxy.DictionaryXY(aexy));
        TreeMap<Long, Integer> aexu = new TreeMap<Long, Integer>();
        aexu.put((long)4, 4);
        assertEquals(aexu, proxy.DictionaryXU(aexu));
        TreeMap<Long, String> aexg = new TreeMap<Long, String>();
        aexg.put((long)4, "g");
        assertEquals(aexg, proxy.DictionaryXG(aexg));
        TreeMap<Long, Integer> aexi = new TreeMap<Long, Integer>();
        aexi.put((long)4, 3);
        assertEquals(aexi, proxy.DictionaryXI(aexi));
        TreeMap<Long, Long> aext = new TreeMap<Long, Long>();
        aext.put((long)4, (long)5);
        assertEquals(aext, proxy.DictionaryXT(aext));
        TreeMap<Long, Short> aexn = new TreeMap<Long, Short>();
        aexn.put((long)4, (short)2);
        assertEquals(aexn, proxy.DictionaryXN(aexn));
        TreeMap<Long, Variant> aexv = new TreeMap<Long, Variant>();
        aexv.put((long)4, new Variant(new String("nine")));
        assertEquals(aexv, proxy.DictionaryXV(aexv));
        TreeMap<Long, String> aexs = new TreeMap<Long, String>();
        aexs.put((long)4, "six");
        assertEquals(aexs, proxy.DictionaryXS(aexs));
        TreeMap<Long, byte[]> aexay = new TreeMap<Long, byte[]>();
        aexay.put((long)4, new byte[] { (byte)7 });
        Map<Long, byte[]> ret_aexay = proxy.DictionaryXAY(aexay);
        assertEquals(ret_aexay.keySet(), aexay.keySet());
        for (Map.Entry<Long, byte[]> e: aexay.entrySet()) {
            byte[] value = e.getValue();
            byte[] value2 = ret_aexay.get(e.getKey());
            assertArrayEquals(value2, value);
        }
        TreeMap<Long, Short> aexq = new TreeMap<Long, Short>();
        aexq.put((long)4, (short)3);
        assertEquals(aexq, proxy.DictionaryXQ(aexq));
        TreeMap<Long, Boolean> aexb = new TreeMap<Long, Boolean>();
        aexb.put((long)4, true);
        assertEquals(aexb, proxy.DictionaryXB(aexb));
        TreeMap<Long, String> aexo = new TreeMap<Long, String>();
        aexo.put((long)4, "/seven");
        assertEquals(aexo, proxy.DictionaryXO(aexo));
        TreeMap<String, Map<String, String>> aesaess = new TreeMap<String, Map<String, String>>();
        aesaess.put("six", ae);
        assertEquals(aesaess, proxy.DictionarySAESS(aesaess));
        TreeMap<String, AnnotatedTypesInterface.InnerStruct> aesr = new TreeMap<String, AnnotatedTypesInterface.InnerStruct>();
        aesr.put("six", new AnnotatedTypesInterface.InnerStruct(8));
        assertEquals(aesr, proxy.AnnotatedDictionarySR(aesr));
        TreeMap<String, Double> aesd = new TreeMap<String, Double>();
        aesd.put("six", 5.1);
        assertEquals(aesd, proxy.DictionarySD(aesd));
        TreeMap<String, Long> aesx = new TreeMap<String, Long>();
        aesx.put("six", (long)4);
        assertEquals(aesx, proxy.DictionarySX(aesx));
        TreeMap<String, Byte> aesy = new TreeMap<String, Byte>();
        aesy.put("six", (byte)1);
        assertEquals(aesy, proxy.DictionarySY(aesy));
        TreeMap<String, Integer> aesu = new TreeMap<String, Integer>();
        aesu.put("six", 4);
        assertEquals(aesu, proxy.DictionarySU(aesu));
        TreeMap<String, String> aesg = new TreeMap<String, String>();
        aesg.put("six", "g");
        assertEquals(aesg, proxy.DictionarySG(aesg));
        TreeMap<String, Integer> aesi = new TreeMap<String, Integer>();
        aesi.put("six", 3);
        assertEquals(aesi, proxy.DictionarySI(aesi));
        TreeMap<String, Long> aest = new TreeMap<String, Long>();
        aest.put("six", (long)5);
        assertEquals(aest, proxy.DictionaryST(aest));
        TreeMap<String, Short> aesn = new TreeMap<String, Short>();
        aesn.put("six", (short)2);
        assertEquals(aesn, proxy.DictionarySN(aesn));
        TreeMap<String, Variant> aesv = new TreeMap<String, Variant>();
        aesv.put("six", new Variant(new String("nine")));
        assertEquals(aesv, proxy.DictionarySV(aesv));
        TreeMap<String, String> aess = new TreeMap<String, String>();
        aess.put("six", "six");
        assertEquals(aess, proxy.DictionarySS(aess));
        TreeMap<String, byte[]> aesay = new TreeMap<String, byte[]>();
        aesay.put("six", new byte[] { (byte)7 });
        Map<String, byte[]> ret_aesay = proxy.DictionarySAY(aesay);
        assertEquals(ret_aesay.keySet(), aesay.keySet());
        for (Map.Entry<String, byte[]> e: aesay.entrySet()) {
            byte[] value = e.getValue();
            byte[] value2 = ret_aesay.get(e.getKey());
            assertArrayEquals(value2, value);
        }
        TreeMap<String, Short> aesq = new TreeMap<String, Short>();
        aesq.put("six", (short)3);
        assertEquals(aesq, proxy.DictionarySQ(aesq));
        TreeMap<String, Boolean> aesb = new TreeMap<String, Boolean>();
        aesb.put("six", true);
        assertEquals(aesb, proxy.DictionarySB(aesb));
        TreeMap<String, String> aeso = new TreeMap<String, String>();
        aeso.put("six", "/seven");
        assertEquals(aeso, proxy.DictionarySO(aeso));
        TreeMap<Byte, Map<String, String>> aeyaess = new TreeMap<Byte, Map<String, String>>();
        aeyaess.put((byte)1, ae);
        assertEquals(aeyaess, proxy.DictionaryYAESS(aeyaess));
        TreeMap<Byte, AnnotatedTypesInterface.InnerStruct> aeyr = new TreeMap<Byte, AnnotatedTypesInterface.InnerStruct>();
        aeyr.put((byte)1, new AnnotatedTypesInterface.InnerStruct(8));
        assertEquals(aeyr, proxy.AnnotatedDictionaryYR(aeyr));
        TreeMap<Byte, Double> aeyd = new TreeMap<Byte, Double>();
        aeyd.put((byte)1, 5.1);
        assertEquals(aeyd, proxy.DictionaryYD(aeyd));
        TreeMap<Byte, Long> aeyx = new TreeMap<Byte, Long>();
        aeyx.put((byte)1, (long)4);
        assertEquals(aeyx, proxy.DictionaryYX(aeyx));
        TreeMap<Byte, Byte> aeyy = new TreeMap<Byte, Byte>();
        aeyy.put((byte)1, (byte)1);
        assertEquals(aeyy, proxy.DictionaryYY(aeyy));
        TreeMap<Byte, Integer> aeyu = new TreeMap<Byte, Integer>();
        aeyu.put((byte)1, 4);
        assertEquals(aeyu, proxy.DictionaryYU(aeyu));
        TreeMap<Byte, String> aeyg = new TreeMap<Byte, String>();
        aeyg.put((byte)1, "g");
        assertEquals(aeyg, proxy.DictionaryYG(aeyg));
        TreeMap<Byte, Integer> aeyi = new TreeMap<Byte, Integer>();
        aeyi.put((byte)1, 3);
        assertEquals(aeyi, proxy.DictionaryYI(aeyi));
        TreeMap<Byte, Long> aeyt = new TreeMap<Byte, Long>();
        aeyt.put((byte)1, (long)5);
        assertEquals(aeyt, proxy.DictionaryYT(aeyt));
        TreeMap<Byte, Short> aeyn = new TreeMap<Byte, Short>();
        aeyn.put((byte)1, (short)2);
        assertEquals(aeyn, proxy.DictionaryYN(aeyn));
        TreeMap<Byte, Variant> aeyv = new TreeMap<Byte, Variant>();
        aeyv.put((byte)1, new Variant(new String("nine")));
        assertEquals(aeyv, proxy.DictionaryYV(aeyv));
        TreeMap<Byte, String> aeys = new TreeMap<Byte, String>();
        aeys.put((byte)1, "six");
        assertEquals(aeys, proxy.DictionaryYS(aeys));
        TreeMap<Byte, byte[]> aeyay = new TreeMap<Byte, byte[]>();
        aeyay.put((byte)1, new byte[] { (byte)7 });
        Map<Byte, byte[]> ret_aeyay = proxy.DictionaryYAY(aeyay);
        assertEquals(ret_aeyay.keySet(), aeyay.keySet());
        for (Map.Entry<Byte, byte[]> e: aeyay.entrySet()) {
            byte[] value = e.getValue();
            byte[] value2 = ret_aeyay.get(e.getKey());
            assertArrayEquals(value2, value);
        }
        TreeMap<Byte, Short> aeyq = new TreeMap<Byte, Short>();
        aeyq.put((byte)1, (short)3);
        assertEquals(aeyq, proxy.DictionaryYQ(aeyq));
        TreeMap<Byte, Boolean> aeyb = new TreeMap<Byte, Boolean>();
        aeyb.put((byte)1, true);
        assertEquals(aeyb, proxy.DictionaryYB(aeyb));
        TreeMap<Byte, String> aeyo = new TreeMap<Byte, String>();
        aeyo.put((byte)1, "/seven");
        assertEquals(aeyo, proxy.DictionaryYO(aeyo));
        TreeMap<Integer, Map<String, String>> aeuaess = new TreeMap<Integer, Map<String, String>>();
        aeuaess.put(4, ae);
        assertEquals(aeuaess, proxy.DictionaryUAESS(aeuaess));
        TreeMap<Integer, AnnotatedTypesInterface.InnerStruct> aeur = new TreeMap<Integer, AnnotatedTypesInterface.InnerStruct>();
        aeur.put(4, new AnnotatedTypesInterface.InnerStruct(8));
        assertEquals(aeur, proxy.DictionaryUR(aeur));
        TreeMap<Integer, Double> aeud = new TreeMap<Integer, Double>();
        aeud.put(4, 5.1);
        assertEquals(aeud, proxy.DictionaryUD(aeud));
        TreeMap<Integer, Long> aeux = new TreeMap<Integer, Long>();
        aeux.put(4, (long)4);
        assertEquals(aeux, proxy.DictionaryUX(aeux));
        TreeMap<Integer, Byte> aeuy = new TreeMap<Integer, Byte>();
        aeuy.put(4, (byte)1);
        assertEquals(aeuy, proxy.DictionaryUY(aeuy));
        TreeMap<Integer, Integer> aeuu = new TreeMap<Integer, Integer>();
        aeuu.put(4, 4);
        assertEquals(aeuu, proxy.DictionaryUU(aeuu));
        TreeMap<Integer, String> aeug = new TreeMap<Integer, String>();
        aeug.put(4, "g");
        assertEquals(aeug, proxy.DictionaryUG(aeug));
        TreeMap<Integer, Integer> aeui = new TreeMap<Integer, Integer>();
        aeui.put(4, 3);
        assertEquals(aeui, proxy.DictionaryUI(aeui));
        TreeMap<Integer, Long> aeut = new TreeMap<Integer, Long>();
        aeut.put(4, (long)5);
        assertEquals(aeut, proxy.DictionaryUT(aeut));
        TreeMap<Integer, Short> aeun = new TreeMap<Integer, Short>();
        aeun.put(4, (short)2);
        assertEquals(aeun, proxy.DictionaryUN(aeun));
        TreeMap<Integer, Variant> aeuv = new TreeMap<Integer, Variant>();
        aeuv.put(4, new Variant(new String("nine")));
        assertEquals(aeuv, proxy.DictionaryUV(aeuv));
        TreeMap<Integer, String> aeus = new TreeMap<Integer, String>();
        aeus.put(4, "six");
        assertEquals(aeus, proxy.DictionaryUS(aeus));
        TreeMap<Integer, byte[]> aeuay = new TreeMap<Integer, byte[]>();
        aeuay.put(4, new byte[] { (byte)7 });
        Map<Integer, byte[]> ret_aeuay = proxy.DictionaryUAY(aeuay);
        assertEquals(ret_aeuay.keySet(), aeuay.keySet());
        for (Map.Entry<Integer, byte[]> e: aeuay.entrySet()) {
            byte[] value = e.getValue();
            byte[] value2 = ret_aeuay.get(e.getKey());
            assertArrayEquals(value2, value);
        }
        TreeMap<Integer, Short> aeuq = new TreeMap<Integer, Short>();
        aeuq.put(4, (short)3);
        assertEquals(aeuq, proxy.DictionaryUQ(aeuq));
        TreeMap<Integer, Boolean> aeub = new TreeMap<Integer, Boolean>();
        aeub.put(4, true);
        assertEquals(aeub, proxy.DictionaryUB(aeub));
        TreeMap<Integer, String> aeuo = new TreeMap<Integer, String>();
        aeuo.put(4, "/seven");
        assertEquals(aeuo, proxy.DictionaryUO(aeuo));
        TreeMap<String, Map<String, String>> aegaess = new TreeMap<String, Map<String, String>>();
        aegaess.put("g", ae);
        assertEquals(aegaess, proxy.DictionaryGAESS(aegaess));
        TreeMap<String, AnnotatedTypesInterface.InnerStruct> aegr = new TreeMap<String, AnnotatedTypesInterface.InnerStruct>();
        aegr.put("g", new AnnotatedTypesInterface.InnerStruct(8));
        assertEquals(aegr, proxy.DictionaryGR(aegr));
        TreeMap<String, Double> aegd = new TreeMap<String, Double>();
        aegd.put("g", 5.1);
        assertEquals(aegd, proxy.DictionaryGD(aegd));
        TreeMap<String, Long> aegx = new TreeMap<String, Long>();
        aegx.put("g", (long)4);
        assertEquals(aegx, proxy.DictionaryGX(aegx));
        TreeMap<String, Byte> aegy = new TreeMap<String, Byte>();
        aegy.put("g", (byte)1);
        assertEquals(aegy, proxy.DictionaryGY(aegy));
        TreeMap<String, Integer> aegu = new TreeMap<String, Integer>();
        aegu.put("g", 4);
        assertEquals(aegu, proxy.DictionaryGU(aegu));
        TreeMap<String, String> aegg = new TreeMap<String, String>();
        aegg.put("g", "g");
        assertEquals(aegg, proxy.DictionaryGG(aegg));
        TreeMap<String, Integer> aegi = new TreeMap<String, Integer>();
        aegi.put("g", 3);
        assertEquals(aegi, proxy.DictionaryGI(aegi));
        TreeMap<String, Long> aegt = new TreeMap<String, Long>();
        aegt.put("g", (long)5);
        assertEquals(aegt, proxy.DictionaryGT(aegt));
        TreeMap<String, Short> aegn = new TreeMap<String, Short>();
        aegn.put("g", (short)2);
        assertEquals(aegn, proxy.DictionaryGN(aegn));
        TreeMap<String, Variant> aegv = new TreeMap<String, Variant>();
        aegv.put("g", new Variant(new String("nine")));
        assertEquals(aegv, proxy.DictionaryGV(aegv));
        TreeMap<String, String> aegs = new TreeMap<String, String>();
        aegs.put("g", "six");
        assertEquals(aegs, proxy.DictionaryGS(aegs));
        TreeMap<String, byte[]> aegay = new TreeMap<String, byte[]>();
        aegay.put("g", new byte[] { (byte)7 });
        Map<String, byte[]> ret_aegay = proxy.DictionaryGAY(aegay);
        assertEquals(ret_aegay.keySet(), aegay.keySet());
        for (Map.Entry<String, byte[]> e: aegay.entrySet()) {
            byte[] value = e.getValue();
            byte[] value2 = ret_aegay.get(e.getKey());
            assertArrayEquals(value2, value);
        }
        TreeMap<String, Short> aegq = new TreeMap<String, Short>();
        aegq.put("g", (short)3);
        assertEquals(aegq, proxy.DictionaryGQ(aegq));
        TreeMap<String, Boolean> aegb = new TreeMap<String, Boolean>();
        aegb.put("g", true);
        assertEquals(aegb, proxy.DictionaryGB(aegb));
        TreeMap<String, String> aego = new TreeMap<String, String>();
        aego.put("g", "/seven");
        assertEquals(aego, proxy.DictionaryGO(aego));
        TreeMap<Boolean, Map<String, String>> aebaess = new TreeMap<Boolean, Map<String, String>>();
        aebaess.put(true, ae);
        assertEquals(aebaess, proxy.DictionaryBAESS(aebaess));
        TreeMap<Boolean, AnnotatedTypesInterface.InnerStruct> aebr = new TreeMap<Boolean, AnnotatedTypesInterface.InnerStruct>();
        aebr.put(true, new AnnotatedTypesInterface.InnerStruct(8));
        assertEquals(aebr, proxy.AnnotatedDictionaryBR(aebr));
        TreeMap<Boolean, Double> aebd = new TreeMap<Boolean, Double>();
        aebd.put(true, 5.1);
        assertEquals(aebd, proxy.DictionaryBD(aebd));
        TreeMap<Boolean, Long> aebx = new TreeMap<Boolean, Long>();
        aebx.put(true, (long)4);
        assertEquals(aebx, proxy.DictionaryBX(aebx));
        TreeMap<Boolean, Byte> aeby = new TreeMap<Boolean, Byte>();
        aeby.put(true, (byte)1);
        assertEquals(aeby, proxy.DictionaryBY(aeby));
        TreeMap<Boolean, Integer> aebu = new TreeMap<Boolean, Integer>();
        aebu.put(true, 4);
        assertEquals(aebu, proxy.DictionaryBU(aebu));
        TreeMap<Boolean, String> aebg = new TreeMap<Boolean, String>();
        aebg.put(true, "g");
        assertEquals(aebg, proxy.DictionaryBG(aebg));
        TreeMap<Boolean, Integer> aebi = new TreeMap<Boolean, Integer>();
        aebi.put(true, 3);
        assertEquals(aebi, proxy.DictionaryBI(aebi));
        TreeMap<Boolean, Long> aebt = new TreeMap<Boolean, Long>();
        aebt.put(true, (long)5);
        assertEquals(aebt, proxy.DictionaryBT(aebt));
        TreeMap<Boolean, Short> aebn = new TreeMap<Boolean, Short>();
        aebn.put(true, (short)2);
        assertEquals(aebn, proxy.DictionaryBN(aebn));
        TreeMap<Boolean, Variant> aebv = new TreeMap<Boolean, Variant>();
        aebv.put(true, new Variant(new String("nine")));
        assertEquals(aebv, proxy.DictionaryBV(aebv));
        TreeMap<Boolean, String> aebs = new TreeMap<Boolean, String>();
        aebs.put(true, "six");
        assertEquals(aebs, proxy.DictionaryBS(aebs));
        TreeMap<Boolean, byte[]> aebay = new TreeMap<Boolean, byte[]>();
        aebay.put(true, new byte[] { (byte)7 });
        Map<Boolean, byte[]> ret_aebay = proxy.DictionaryBAY(aebay);
        assertEquals(ret_aebay.keySet(), aebay.keySet());
        for (Map.Entry<Boolean, byte[]> e: aebay.entrySet()) {
            byte[] value = e.getValue();
            byte[] value2 = ret_aebay.get(e.getKey());
            assertArrayEquals(value2, value);
        }
        TreeMap<Boolean, Short> aebq = new TreeMap<Boolean, Short>();
        aebq.put(true, (short)3);
        assertEquals(aebq, proxy.DictionaryBQ(aebq));
        TreeMap<Boolean, Boolean> aebb = new TreeMap<Boolean, Boolean>();
        aebb.put(true, true);
        assertEquals(aebb, proxy.DictionaryBB(aebb));
        TreeMap<Boolean, String> aebo = new TreeMap<Boolean, String>();
        aebo.put(true, "/seven");
        assertEquals(aebo, proxy.DictionaryBO(aebo));
        TreeMap<Short, Map<String, String>> aeqaess = new TreeMap<Short, Map<String, String>>();
        aeqaess.put((short)3, ae);
        assertEquals(aeqaess, proxy.DictionaryQAESS(aeqaess));
        TreeMap<Short, AnnotatedTypesInterface.InnerStruct> aeqr = new TreeMap<Short, AnnotatedTypesInterface.InnerStruct>();
        aeqr.put((short)3, new AnnotatedTypesInterface.InnerStruct(8));
        assertEquals(aeqr, proxy.DictionaryQR(aeqr));
        TreeMap<Short, Double> aeqd = new TreeMap<Short, Double>();
        aeqd.put((short)3, 5.1);
        assertEquals(aeqd, proxy.DictionaryQD(aeqd));
        TreeMap<Short, Long> aeqx = new TreeMap<Short, Long>();
        aeqx.put((short)3, (long)4);
        assertEquals(aeqx, proxy.DictionaryQX(aeqx));
        TreeMap<Short, Byte> aeqy = new TreeMap<Short, Byte>();
        aeqy.put((short)3, (byte)1);
        assertEquals(aeqy, proxy.DictionaryQY(aeqy));
        TreeMap<Short, Integer> aequ = new TreeMap<Short, Integer>();
        aequ.put((short)3, 4);
        assertEquals(aequ, proxy.DictionaryQU(aequ));
        TreeMap<Short, String> aeqg = new TreeMap<Short, String>();
        aeqg.put((short)3, "g");
        assertEquals(aeqg, proxy.DictionaryQG(aeqg));
        TreeMap<Short, Integer> aeqi = new TreeMap<Short, Integer>();
        aeqi.put((short)3, 3);
        assertEquals(aeqi, proxy.DictionaryQI(aeqi));
        TreeMap<Short, Long> aeqt = new TreeMap<Short, Long>();
        aeqt.put((short)3, (long)5);
        assertEquals(aeqt, proxy.DictionaryQT(aeqt));
        TreeMap<Short, Short> aeqn = new TreeMap<Short, Short>();
        aeqn.put((short)3, (short)2);
        assertEquals(aeqn, proxy.DictionaryQN(aeqn));
        TreeMap<Short, Variant> aeqv = new TreeMap<Short, Variant>();
        aeqv.put((short)3, new Variant(new String("nine")));
        assertEquals(aeqv, proxy.DictionaryQV(aeqv));
        TreeMap<Short, String> aeqs = new TreeMap<Short, String>();
        aeqs.put((short)3, "six");
        assertEquals(aeqs, proxy.DictionaryQS(aeqs));
        TreeMap<Short, byte[]> aeqay = new TreeMap<Short, byte[]>();
        aeqay.put((short)3, new byte[] { (byte)7 });
        Map<Short, byte[]> ret_aeqay = proxy.DictionaryQAY(aeqay);
        assertEquals(ret_aeqay.keySet(), aeqay.keySet());
        for (Map.Entry<Short, byte[]> e: aeqay.entrySet()) {
            byte[] value = e.getValue();
            byte[] value2 = ret_aeqay.get(e.getKey());
            assertArrayEquals(value2, value);
        }
        TreeMap<Short, Short> aeqq = new TreeMap<Short, Short>();
        aeqq.put((short)3, (short)3);
        assertEquals(aeqq, proxy.DictionaryQQ(aeqq));
        TreeMap<Short, Boolean> aeqb = new TreeMap<Short, Boolean>();
        aeqb.put((short)3, true);
        assertEquals(aeqb, proxy.DictionaryQB(aeqb));
        TreeMap<Short, String> aeqo = new TreeMap<Short, String>();
        aeqo.put((short)3, "/seven");
        assertEquals(aeqo, proxy.DictionaryQO(aeqo));
        TreeMap<String, Map<String, String>> aeoaess = new TreeMap<String, Map<String, String>>();
        aeoaess.put("/seven", ae);
        assertEquals(aeoaess, proxy.DictionaryOAESS(aeoaess));
        TreeMap<String, AnnotatedTypesInterface.InnerStruct> aeor = new TreeMap<String, AnnotatedTypesInterface.InnerStruct>();
        aeor.put("/seven", new AnnotatedTypesInterface.InnerStruct(8));
        assertEquals(aeor, proxy.DictionaryOR(aeor));
        TreeMap<String, Double> aeod = new TreeMap<String, Double>();
        aeod.put("/seven", 5.1);
        assertEquals(aeod, proxy.DictionaryOD(aeod));
        TreeMap<String, Long> aeox = new TreeMap<String, Long>();
        aeox.put("/seven", (long)4);
        assertEquals(aeox, proxy.DictionaryOX(aeox));
        TreeMap<String, Byte> aeoy = new TreeMap<String, Byte>();
        aeoy.put("/seven", (byte)1);
        assertEquals(aeoy, proxy.DictionaryOY(aeoy));
        TreeMap<String, Integer> aeou = new TreeMap<String, Integer>();
        aeou.put("/seven", 4);
        assertEquals(aeou, proxy.DictionaryOU(aeou));
        TreeMap<String, String> aeog = new TreeMap<String, String>();
        aeog.put("/seven", "g");
        assertEquals(aeog, proxy.DictionaryOG(aeog));
        TreeMap<String, Integer> aeoi = new TreeMap<String, Integer>();
        aeoi.put("/seven", 3);
        assertEquals(aeoi, proxy.DictionaryOI(aeoi));
        TreeMap<String, Long> aeot = new TreeMap<String, Long>();
        aeot.put("/seven", (long)5);
        assertEquals(aeot, proxy.DictionaryOT(aeot));
        TreeMap<String, Short> aeon = new TreeMap<String, Short>();
        aeon.put("/seven", (short)2);
        assertEquals(aeon, proxy.DictionaryON(aeon));
        TreeMap<String, Variant> aeov = new TreeMap<String, Variant>();
        aeov.put("/seven", new Variant(new String("nine")));
        assertEquals(aeov, proxy.DictionaryOV(aeov));
        TreeMap<String, String> aeos = new TreeMap<String, String>();
        aeos.put("/seven", "six");
        assertEquals(aeos, proxy.DictionaryOS(aeos));
        TreeMap<String, byte[]> aeoay = new TreeMap<String, byte[]>();
        aeoay.put("/seven", new byte[] { (byte)7 });
        Map<String, byte[]> ret_aeoay = proxy.DictionaryOAY(aeoay);
        assertEquals(ret_aeoay.keySet(), aeoay.keySet());
        for (Map.Entry<String, byte[]> e: aeoay.entrySet()) {
            byte[] value = e.getValue();
            byte[] value2 = ret_aeoay.get(e.getKey());
            assertArrayEquals(value2, value);
        }
        TreeMap<String, Short> aeoq = new TreeMap<String, Short>();
        aeoq.put("/seven", (short)3);
        assertEquals(aeoq, proxy.DictionaryOQ(aeoq));
        TreeMap<String, Boolean> aeob = new TreeMap<String, Boolean>();
        aeob.put("/seven", true);
        assertEquals(aeob, proxy.DictionaryOB(aeob));
        TreeMap<String, String> aeoo = new TreeMap<String, String>();
        aeoo.put("/seven", "/seven");
        assertEquals(aeoo, proxy.DictionaryOO(aeoo));
        TreeMap<Integer, Map<String, String>> aeiaess = new TreeMap<Integer, Map<String, String>>();
        aeiaess.put(3, ae);
        assertEquals(aeiaess, proxy.DictionaryIAESS(aeiaess));
        TreeMap<Integer, AnnotatedTypesInterface.InnerStruct> aeir = new TreeMap<Integer, AnnotatedTypesInterface.InnerStruct>();
        aeir.put(3, new AnnotatedTypesInterface.InnerStruct(8));
        assertEquals(aeir, proxy.AnnotatedDictionaryIR(aeir));
        TreeMap<Integer, Double> aeid = new TreeMap<Integer, Double>();
        aeid.put(3, 5.1);
        assertEquals(aeid, proxy.DictionaryID(aeid));
        TreeMap<Integer, Long> aeix = new TreeMap<Integer, Long>();
        aeix.put(3, (long)4);
        assertEquals(aeix, proxy.DictionaryIX(aeix));
        TreeMap<Integer, Byte> aeiy = new TreeMap<Integer, Byte>();
        aeiy.put(3, (byte)1);
        assertEquals(aeiy, proxy.DictionaryIY(aeiy));
        TreeMap<Integer, Integer> aeiu = new TreeMap<Integer, Integer>();
        aeiu.put(3, 4);
        assertEquals(aeiu, proxy.DictionaryIU(aeiu));
        TreeMap<Integer, String> aeig = new TreeMap<Integer, String>();
        aeig.put(3, "g");
        assertEquals(aeig, proxy.DictionaryIG(aeig));
        TreeMap<Integer, Integer> aeii = new TreeMap<Integer, Integer>();
        aeii.put(3, 3);
        assertEquals(aeii, proxy.DictionaryII(aeii));
        TreeMap<Integer, Long> aeit = new TreeMap<Integer, Long>();
        aeit.put(3, (long)5);
        assertEquals(aeit, proxy.DictionaryIT(aeit));
        TreeMap<Integer, Short> aein = new TreeMap<Integer, Short>();
        aein.put(3, (short)2);
        assertEquals(aein, proxy.DictionaryIN(aein));
        TreeMap<Integer, Variant> aeiv = new TreeMap<Integer, Variant>();
        aeiv.put(3, new Variant(new String("nine")));
        assertEquals(aeiv, proxy.DictionaryIV(aeiv));
        TreeMap<Integer, String> aeis = new TreeMap<Integer, String>();
        aeis.put(3, "six");
        assertEquals(aeis, proxy.DictionaryIS(aeis));
        TreeMap<Integer, byte[]> aeiay = new TreeMap<Integer, byte[]>();
        aeiay.put(3, new byte[] { (byte)7 });
        Map<Integer, byte[]> ret_aeiay = proxy.DictionaryIAY(aeiay);
        assertEquals(ret_aeiay.keySet(), aeiay.keySet());
        for (Map.Entry<Integer, byte[]> e: aeiay.entrySet()) {
            byte[] value = e.getValue();
            byte[] value2 = ret_aeiay.get(e.getKey());
            assertArrayEquals(value2, value);
        }
        TreeMap<Integer, Short> aeiq = new TreeMap<Integer, Short>();
        aeiq.put(3, (short)3);
        assertEquals(aeiq, proxy.DictionaryIQ(aeiq));
        TreeMap<Integer, Boolean> aeib = new TreeMap<Integer, Boolean>();
        aeib.put(3, true);
        assertEquals(aeib, proxy.DictionaryIB(aeib));
        TreeMap<Integer, String> aeio = new TreeMap<Integer, String>();
        aeio.put(3, "/seven");
        assertEquals(aeio, proxy.DictionaryIO(aeio));
        TreeMap<Long, Map<String, String>> aetaess = new TreeMap<Long, Map<String, String>>();
        aetaess.put((long)5, ae);
        assertEquals(aetaess, proxy.DictionaryTAESS(aetaess));
        TreeMap<Long, AnnotatedTypesInterface.InnerStruct> aetr = new TreeMap<Long, AnnotatedTypesInterface.InnerStruct>();
        aetr.put((long)5, new AnnotatedTypesInterface.InnerStruct(8));
        assertEquals(aetr, proxy.DictionaryTR(aetr));
        TreeMap<Long, Double> aetd = new TreeMap<Long, Double>();
        aetd.put((long)5, 5.1);
        assertEquals(aetd, proxy.DictionaryTD(aetd));
        TreeMap<Long, Long> aetx = new TreeMap<Long, Long>();
        aetx.put((long)5, (long)4);
        assertEquals(aetx, proxy.DictionaryTX(aetx));
        TreeMap<Long, Byte> aety = new TreeMap<Long, Byte>();
        aety.put((long)5, (byte)1);
        assertEquals(aety, proxy.DictionaryTY(aety));
        TreeMap<Long, Integer> aetu = new TreeMap<Long, Integer>();
        aetu.put((long)5, 4);
        assertEquals(aetu, proxy.DictionaryTU(aetu));
        TreeMap<Long, String> aetg = new TreeMap<Long, String>();
        aetg.put((long)5, "g");
        assertEquals(aetg, proxy.DictionaryTG(aetg));
        TreeMap<Long, Integer> aeti = new TreeMap<Long, Integer>();
        aeti.put((long)5, 3);
        assertEquals(aeti, proxy.DictionaryTI(aeti));
        TreeMap<Long, Long> aett = new TreeMap<Long, Long>();
        aett.put((long)5, (long)5);
        assertEquals(aett, proxy.DictionaryTT(aett));
        TreeMap<Long, Short> aetn = new TreeMap<Long, Short>();
        aetn.put((long)5, (short)2);
        assertEquals(aetn, proxy.DictionaryTN(aetn));
        TreeMap<Long, Variant> aetv = new TreeMap<Long, Variant>();
        aetv.put((long)5, new Variant(new String("nine")));
        assertEquals(aetv, proxy.DictionaryTV(aetv));
        TreeMap<Long, String> aets = new TreeMap<Long, String>();
        aets.put((long)5, "six");
        assertEquals(aets, proxy.DictionaryTS(aets));
        TreeMap<Long, byte[]> aetay = new TreeMap<Long, byte[]>();
        aetay.put((long)5, new byte[] { (byte)7 });
        Map<Long, byte[]> ret_aetay = proxy.DictionaryTAY(aetay);
        assertEquals(ret_aetay.keySet(), aetay.keySet());
        for (Map.Entry<Long, byte[]> e: aetay.entrySet()) {
            byte[] value = e.getValue();
            byte[] value2 = ret_aetay.get(e.getKey());
            assertArrayEquals(value2, value);
        }
        TreeMap<Long, Short> aetq = new TreeMap<Long, Short>();
        aetq.put((long)5, (short)3);
        assertEquals(aetq, proxy.DictionaryTQ(aetq));
        TreeMap<Long, Boolean> aetb = new TreeMap<Long, Boolean>();
        aetb.put((long)5, true);
        assertEquals(aetb, proxy.DictionaryTB(aetb));
        TreeMap<Long, String> aeto = new TreeMap<Long, String>();
        aeto.put((long)5, "/seven");
        assertEquals(aeto, proxy.DictionaryTO(aeto));

        assertEquals(aess, proxy.getDictionarySS());
        proxy.setDictionarySS(aess);
    }

    private class AessTypeReference extends VariantTypeReference<Map<String, String>> {};

    public void testVariantTypeReference() throws Exception {
        TreeMap<String, String> ae = new TreeMap<String, String>();

        AnnotatedTypesInterface annotated = remoteObj.getInterface(AnnotatedTypesInterface.class);
        Variant v = new Variant(ae, "a{ss}");
        AessTypeReference aessTypeReference = new AessTypeReference();
        assertEquals(v.getObject(aessTypeReference),
                     annotated.Variant(v).getObject(aessTypeReference));

        InferredTypesInterface inferred = remoteObj.getInterface(InferredTypesInterface.class);
        v = new Variant(ae, "a{ss}");
        assertEquals(v.getObject(aessTypeReference),
                     inferred.Variant(v).getObject(aessTypeReference));
    }

    public void testEnums() throws Exception {
        AnnotatedTypesInterface proxy = remoteObj.getInterface(AnnotatedTypesInterface.class);

        assertEquals(AnnotatedTypesInterface.EnumType.Enum0, proxy.EnumY(AnnotatedTypesInterface.EnumType.Enum0));
        assertEquals(AnnotatedTypesInterface.EnumType.Enum0, proxy.EnumN(AnnotatedTypesInterface.EnumType.Enum0));
        assertEquals(AnnotatedTypesInterface.EnumType.Enum0, proxy.EnumQ(AnnotatedTypesInterface.EnumType.Enum0));
        assertEquals(AnnotatedTypesInterface.EnumType.Enum0, proxy.EnumI(AnnotatedTypesInterface.EnumType.Enum0));
        assertEquals(AnnotatedTypesInterface.EnumType.Enum0, proxy.EnumU(AnnotatedTypesInterface.EnumType.Enum0));
        assertEquals(AnnotatedTypesInterface.EnumType.Enum0, proxy.EnumX(AnnotatedTypesInterface.EnumType.Enum0));
        assertEquals(AnnotatedTypesInterface.EnumType.Enum0, proxy.EnumT(AnnotatedTypesInterface.EnumType.Enum0));
    }

    public void testGenericArrayTypes() throws Exception {

        InferredTypesInterface proxy = remoteObj.getInterface(InferredTypesInterface.class);

        @SuppressWarnings(value="unchecked")
        TreeMap<String, String>[] aaess = (TreeMap<String, String>[]) new TreeMap<?, ?>[2];
        aaess[0] = new TreeMap<String, String>();
        aaess[0].put("a1", "value1");
        aaess[0].put("a2", "value2");
        aaess[1] = new TreeMap<String, String>();
        aaess[1].put("b1", "value3");
        assertArrayEquals(aaess, proxy.TreeDictionaryArray(aaess));
    }

    public void testArraySizes() throws Exception {
        if (!isAndroid) // Android device has less than 32M heap per process JVM
        {
            InferredTypesInterface proxy = remoteObj.getInterface(InferredTypesInterface.class);

            // There exists a hard-coded limit of 128k bytes in an array
            int k = 131072;

            byte[] ay = new byte[k];
            assertArrayEquals(ay, proxy.ByteArray(ay));
            ay = new byte[k + 1];
            boolean thrown = false;
            try {
                assertArrayEquals(ay, proxy.ByteArray(ay));
            } catch (MarshalBusException ex) {
                thrown = true;
            }
            assertEquals(true, thrown);
        }
    }

    public void testPacketSizes() throws Exception {
        if (!isAndroid) // Android device has less than 32M heap per process JVM
        {
            InferredTypesInterface proxy = remoteObj.getInterface(InferredTypesInterface.class);

            // There exists a soft limit of 128k-132k bytes in a total message
            int k = 131072;

            InferredTypesInterface.TwoByteArrays rayay = new InferredTypesInterface.TwoByteArrays();
            rayay.ay0 = new byte[k];
            rayay.ay1 = new byte[k];

            boolean thrown = false;
            try {
                proxy.TwoByteArrays(rayay);
            } catch (BusException ex) {
                thrown = true;
            }
            assertEquals(true, thrown);
        }
    }

    public void testNullArgs() throws Exception {
        InferredTypesInterface proxy = remoteObj.getInterface(InferredTypesInterface.class);

        /* Basic types */
        boolean thrown = false;
        try {
            proxy.String(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);

        /* Array types */
        thrown = false;
        try {
            proxy.ByteArray(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            proxy.BooleanArray(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            proxy.CapitalBooleanArray(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            proxy.Int16Array(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            proxy.Int32Array(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            proxy.Int64Array(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            proxy.DoubleArray(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            proxy.StringArray(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            proxy.ArrayArray(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            proxy.InferredStructArray(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            proxy.VariantArray(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            proxy.DictionaryArray(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);

        /* Struct types */
        thrown = false;
        try {
            proxy.InferredStruct(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);

        /* Variant types */
        thrown = false;
        try {
            proxy.Variant(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);

        /* Dictionary types */
        thrown = true;
        try {
            proxy.DictionaryYY(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryYB(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryYN(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryYI(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryYX(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryYD(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryYS(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryYAY(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.InferredDictionaryYR(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryYV(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryYAESS(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryBY(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryBB(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryBN(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryBI(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryBX(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryBD(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryBS(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryBAY(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.InferredDictionaryBR(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryBV(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryBAESS(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryNY(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryNB(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryNN(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryNI(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryNX(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryND(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryNS(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryNAY(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.InferredDictionaryNR(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryNV(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryNAESS(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryIY(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryIB(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryIN(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryII(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryIX(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryID(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryIS(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryIAY(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.InferredDictionaryIR(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryIV(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryIAESS(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryXY(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryXB(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryXN(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryXI(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryXX(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryXD(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryXS(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryXAY(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.InferredDictionaryXR(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryXV(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryXAESS(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryDY(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryDB(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryDN(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryDI(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryDX(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryDD(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryDS(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryDAY(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.InferredDictionaryDR(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryDV(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionaryDAESS(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionarySY(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionarySB(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionarySN(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionarySI(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionarySX(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionarySD(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionarySS(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionarySAY(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.InferredDictionarySR(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionarySV(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = true;
        try {
            proxy.DictionarySAESS(null);
        } catch (MarshalBusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
    }

    public void testNullReturns() throws Exception {
        InferredTypesInterface proxy = remoteNullReturnsObj.getInterface(InferredTypesInterface.class);

        /* Basic types */
        boolean thrown = false;
        try {
            proxy.String("string");
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);

        /* Array types */
        thrown = false;
        try {
            byte[] ay = new byte[] { 1, 2 };
            proxy.ByteArray(ay);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            boolean[] ab = new boolean[] { true, false };
            proxy.BooleanArray(ab);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            Boolean[] aB = new Boolean[] { true, false };
            proxy.CapitalBooleanArray(aB);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            short[] an = new short[] { 3, 4, 5 };
            proxy.Int16Array(an);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            int[] ai = new int[] { 7 };
            proxy.Int32Array(ai);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            long[] ax = new long[] { 8, 9, 10, 11 };
            proxy.Int64Array(ax);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            double[] ad = new double[] { 0.1, 0.2, 0.3 };
            proxy.DoubleArray(ad);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            String[] as = new String[] { "one", "two" };
            proxy.StringArray(as);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            byte[][] aay = new byte[][] { new byte[] { 1 }, new byte[] { 2 } };
            proxy.ArrayArray(aay);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            InferredTypesInterface.InnerStruct[] ar = new InferredTypesInterface.InnerStruct[] {
                new InferredTypesInterface.InnerStruct(12), new InferredTypesInterface.InnerStruct(13) };
            proxy.InferredStructArray(ar);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            Variant[] av = new Variant[] { new Variant(new String("three")) };
            proxy.VariantArray(av);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            @SuppressWarnings(value="unchecked")
            TreeMap<String, String>[] aaess = (TreeMap<String, String>[]) new TreeMap<?, ?>[2];
            aaess[0] = new TreeMap<String, String>();
            aaess[0].put("a1", "value1");
            aaess[0].put("a2", "value2");
            aaess[1] = new TreeMap<String, String>();
            aaess[1].put("b1", "value3");
            proxy.DictionaryArray(aaess);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);

        /* Struct types */
        thrown = false;
        try {
            TreeMap<String, String> ae = new TreeMap<String, String>();
            ae.put("fourteen", "fifteen");
            ae.put("sixteen", "seventeen");
            InferredTypesInterface.Struct r =
                new InferredTypesInterface.Struct((byte)1, false, (short)1, 2, (long)3, 4.1, "five",
                                                  new byte[] { 6 }, new boolean[] { true }, new short[] { 7 },
                                                  new int[] { 8 }, new long[] { 10 }, new double[] { 10.1 },
                                                  new String[] { "eleven" },
                                                  new InferredTypesInterface.InnerStruct(12),
                                                  new Variant(new String("thirteen")), ae);
            proxy.InferredStruct(r);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);

        /* Variant types */
        TreeMap<String, String> ae = new TreeMap<String, String>();
        ae.put("fourteen", "fifteen");
        ae.put("sixteen", "seventeen");
        Variant v;
        thrown = false;
        try {
            v = new Variant((byte)1);
            proxy.Variant(v);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            v = new Variant(true);
            proxy.Variant(v);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            v = new Variant((short)2);
            proxy.Variant(v);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            v = new Variant(3);
            proxy.Variant(v);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            v = new Variant((long)4);
            proxy.Variant(v);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            v = new Variant(4.1);
            proxy.Variant(v);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            v = new Variant("five");
            proxy.Variant(v);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            v = new Variant(new byte[] { 6 });
            proxy.Variant(v);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            v = new Variant(new boolean[] { true });
            Variant v2 = proxy.Variant(v);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            v = new Variant(new short[] { 7 });
            proxy.Variant(v);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            v = new Variant(new int[] { 8 });
            proxy.Variant(v);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            v = new Variant(new long[] { 10 });
            proxy.Variant(v);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            v = new Variant(new double[] { 10.1 });
            proxy.Variant(v);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            v = new Variant(new String[] { "eleven" });
            proxy.Variant(v);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            v = new Variant(new InferredTypesInterface.InnerStruct(12));
            proxy.Variant(v);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            v = new Variant(new Variant(new String("thirteen")));
        proxy.Variant(v);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);

        /* Dictionary types */
        thrown = false;
        try {
            TreeMap<Byte, Byte> aeyy = new TreeMap<Byte, Byte>();
            aeyy.put((byte)1, (byte)1);
            proxy.DictionaryYY(aeyy);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Byte, Boolean> aeyb = new TreeMap<Byte, Boolean>();
            aeyb.put((byte)1, true);
            proxy.DictionaryYB(aeyb);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Byte, Short> aeyn = new TreeMap<Byte, Short>();
            aeyn.put((byte)1, (short)2);
            proxy.DictionaryYN(aeyn);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Byte, Integer> aeyi = new TreeMap<Byte, Integer>();
            aeyi.put((byte)1, 3);
            proxy.DictionaryYI(aeyi);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Byte, Long> aeyx = new TreeMap<Byte, Long>();
            aeyx.put((byte)1, (long)4);
            proxy.DictionaryYX(aeyx);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Byte, Double> aeyd = new TreeMap<Byte, Double>();
            aeyd.put((byte)1, 5.1);
            proxy.DictionaryYD(aeyd);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Byte, String> aeys = new TreeMap<Byte, String>();
            aeys.put((byte)1, "six");
            proxy.DictionaryYS(aeys);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Byte, byte[]> aeyay = new TreeMap<Byte, byte[]>();
            aeyay.put((byte)1, new byte[] { (byte)7 });
            proxy.DictionaryYAY(aeyay);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Byte, InferredTypesInterface.InnerStruct> aeyr = new TreeMap<Byte, InferredTypesInterface.InnerStruct>();
            aeyr.put((byte)1, new InferredTypesInterface.InnerStruct(8));
            proxy.InferredDictionaryYR(aeyr);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Byte, Variant> aeyv = new TreeMap<Byte, Variant>();
            aeyv.put((byte)1, new Variant(new String("nine")));
            proxy.DictionaryYV(aeyv);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Byte, Map<String, String>> aeyaess = new TreeMap<Byte, Map<String, String>>();
            aeyaess.put((byte)1, ae);
            proxy.DictionaryYAESS(aeyaess);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Boolean, Byte> aeby = new TreeMap<Boolean, Byte>();
            aeby.put(true, (byte)1);
            proxy.DictionaryBY(aeby);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Boolean, Boolean> aebb = new TreeMap<Boolean, Boolean>();
            aebb.put(true, true);
            proxy.DictionaryBB(aebb);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Boolean, Short> aebn = new TreeMap<Boolean, Short>();
            aebn.put(true, (short)2);
            proxy.DictionaryBN(aebn);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Boolean, Integer> aebi = new TreeMap<Boolean, Integer>();
            aebi.put(true, 3);
            proxy.DictionaryBI(aebi);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Boolean, Long> aebx = new TreeMap<Boolean, Long>();
            aebx.put(true, (long)4);
            proxy.DictionaryBX(aebx);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Boolean, Double> aebd = new TreeMap<Boolean, Double>();
            aebd.put(true, 5.1);
            proxy.DictionaryBD(aebd);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Boolean, String> aebs = new TreeMap<Boolean, String>();
            aebs.put(true, "six");
            proxy.DictionaryBS(aebs);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Boolean, byte[]> aebay = new TreeMap<Boolean, byte[]>();
            aebay.put(true, new byte[] { (byte)7 });
            proxy.DictionaryBAY(aebay);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Boolean, InferredTypesInterface.InnerStruct> aebr = new TreeMap<Boolean, InferredTypesInterface.InnerStruct>();
            aebr.put(true, new InferredTypesInterface.InnerStruct(8));
            proxy.InferredDictionaryBR(aebr);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Boolean, Variant> aebv = new TreeMap<Boolean, Variant>();
            aebv.put(true, new Variant(new String("nine")));
            proxy.DictionaryBV(aebv);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Boolean, Map<String, String>> aebaess = new TreeMap<Boolean, Map<String, String>>();
            aebaess.put(true, ae);
            proxy.DictionaryBAESS(aebaess);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Short, Byte> aeny = new TreeMap<Short, Byte>();
            aeny.put((short)2, (byte)1);
            proxy.DictionaryNY(aeny);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Short, Boolean> aenb = new TreeMap<Short, Boolean>();
            aenb.put((short)2, true);
            proxy.DictionaryNB(aenb);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Short, Short> aenn = new TreeMap<Short, Short>();
            aenn.put((short)2, (short)2);
            proxy.DictionaryNN(aenn);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Short, Integer> aeni = new TreeMap<Short, Integer>();
            aeni.put((short)2, 3);
            proxy.DictionaryNI(aeni);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Short, Long> aenx = new TreeMap<Short, Long>();
            aenx.put((short)2, (long)4);
            proxy.DictionaryNX(aenx);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Short, Double> aend = new TreeMap<Short, Double>();
            aend.put((short)2, 5.1);
            proxy.DictionaryND(aend);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Short, String> aens = new TreeMap<Short, String>();
            aens.put((short)2, "six");
            proxy.DictionaryNS(aens);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Short, byte[]> aenay = new TreeMap<Short, byte[]>();
            aenay.put((short)2, new byte[] { (byte)7 });
            proxy.DictionaryNAY(aenay);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Short, InferredTypesInterface.InnerStruct> aenr = new TreeMap<Short, InferredTypesInterface.InnerStruct>();
            aenr.put((short)2, new InferredTypesInterface.InnerStruct(8));
            proxy.InferredDictionaryNR(aenr);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Short, Variant> aenv = new TreeMap<Short, Variant>();
            aenv.put((short)2, new Variant(new String("nine")));
            proxy.DictionaryNV(aenv);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Short, Map<String, String>> aenaess = new TreeMap<Short, Map<String, String>>();
            aenaess.put((short)2, ae);
            proxy.DictionaryNAESS(aenaess);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Integer, Byte> aeiy = new TreeMap<Integer, Byte>();
            aeiy.put(3, (byte)1);
            proxy.DictionaryIY(aeiy);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Integer, Boolean> aeib = new TreeMap<Integer, Boolean>();
            aeib.put(3, true);
            proxy.DictionaryIB(aeib);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Integer, Short> aein = new TreeMap<Integer, Short>();
            aein.put(3, (short)2);
            proxy.DictionaryIN(aein);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Integer, Integer> aeii = new TreeMap<Integer, Integer>();
            aeii.put(3, 3);
            proxy.DictionaryII(aeii);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Integer, Long> aeix = new TreeMap<Integer, Long>();
            aeix.put(3, (long)4);
            proxy.DictionaryIX(aeix);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Integer, Double> aeid = new TreeMap<Integer, Double>();
            aeid.put(3, 5.1);
            proxy.DictionaryID(aeid);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Integer, String> aeis = new TreeMap<Integer, String>();
            aeis.put(3, "six");
            proxy.DictionaryIS(aeis);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Integer, byte[]> aeiay = new TreeMap<Integer, byte[]>();
            aeiay.put(3, new byte[] { (byte)7 });
            proxy.DictionaryIAY(aeiay);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Integer, InferredTypesInterface.InnerStruct> aeir = new TreeMap<Integer, InferredTypesInterface.InnerStruct>();
            aeir.put(3, new InferredTypesInterface.InnerStruct(8));
            proxy.InferredDictionaryIR(aeir);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Integer, Variant> aeiv = new TreeMap<Integer, Variant>();
            aeiv.put(3, new Variant(new String("nine")));
            proxy.DictionaryIV(aeiv);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Integer, Map<String, String>> aeiaess = new TreeMap<Integer, Map<String, String>>();
            aeiaess.put(3, ae);
            proxy.DictionaryIAESS(aeiaess);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Long, Byte> aexy = new TreeMap<Long, Byte>();
            aexy.put((long)4, (byte)1);
            proxy.DictionaryXY(aexy);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Long, Boolean> aexb = new TreeMap<Long, Boolean>();
            aexb.put((long)4, true);
            proxy.DictionaryXB(aexb);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Long, Short> aexn = new TreeMap<Long, Short>();
            aexn.put((long)4, (short)2);
            proxy.DictionaryXN(aexn);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Long, Integer> aexi = new TreeMap<Long, Integer>();
            aexi.put((long)4, 3);
            proxy.DictionaryXI(aexi);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Long, Long> aexx = new TreeMap<Long, Long>();
            aexx.put((long)4, (long)4);
            proxy.DictionaryXX(aexx);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Long, Double> aexd = new TreeMap<Long, Double>();
            aexd.put((long)4, 5.1);
            proxy.DictionaryXD(aexd);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Long, String> aexs = new TreeMap<Long, String>();
            aexs.put((long)4, "six");
            proxy.DictionaryXS(aexs);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Long, byte[]> aexay = new TreeMap<Long, byte[]>();
            aexay.put((long)4, new byte[] { (byte)7 });
            proxy.DictionaryXAY(aexay);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Long, InferredTypesInterface.InnerStruct> aexr = new TreeMap<Long, InferredTypesInterface.InnerStruct>();
            aexr.put((long)4, new InferredTypesInterface.InnerStruct(8));
            proxy.InferredDictionaryXR(aexr);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Long, Variant> aexv = new TreeMap<Long, Variant>();
            aexv.put((long)4, new Variant(new String("nine")));
            proxy.DictionaryXV(aexv);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Long, Map<String, String>> aexaess = new TreeMap<Long, Map<String, String>>();
            aexaess.put((long)4, ae);
            proxy.DictionaryXAESS(aexaess);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Double, Byte> aedy = new TreeMap<Double, Byte>();
            aedy.put(5.1, (byte)1);
            proxy.DictionaryDY(aedy);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Double, Boolean> aedb = new TreeMap<Double, Boolean>();
            aedb.put(5.1, true);
            proxy.DictionaryDB(aedb);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Double, Short> aedn = new TreeMap<Double, Short>();
            aedn.put(5.1, (short)2);
            proxy.DictionaryDN(aedn);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Double, Integer> aedi = new TreeMap<Double, Integer>();
            aedi.put(5.1, 3);
            proxy.DictionaryDI(aedi);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Double, Long> aedx = new TreeMap<Double, Long>();
            aedx.put(5.1, (long)4);
            proxy.DictionaryDX(aedx);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Double, Double> aedd = new TreeMap<Double, Double>();
            aedd.put(5.1, 5.1);
            proxy.DictionaryDD(aedd);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Double, String> aeds = new TreeMap<Double, String>();
            aeds.put(5.1, "six");
            proxy.DictionaryDS(aeds);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Double, byte[]> aeday = new TreeMap<Double, byte[]>();
            aeday.put(5.1, new byte[] { (byte)7 });
            proxy.DictionaryDAY(aeday);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Double, InferredTypesInterface.InnerStruct> aedr = new TreeMap<Double, InferredTypesInterface.InnerStruct>();
            aedr.put(5.1, new InferredTypesInterface.InnerStruct(8));
            proxy.InferredDictionaryDR(aedr);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Double, Variant> aedv = new TreeMap<Double, Variant>();
            aedv.put(5.1, new Variant(new String("nine")));
            proxy.DictionaryDV(aedv);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<Double, Map<String, String>> aedaess = new TreeMap<Double, Map<String, String>>();
            aedaess.put(5.1, ae);
            proxy.DictionaryDAESS(aedaess);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<String, Byte> aesy = new TreeMap<String, Byte>();
            aesy.put("six", (byte)1);
            proxy.DictionarySY(aesy);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<String, Boolean> aesb = new TreeMap<String, Boolean>();
            aesb.put("six", true);
            proxy.DictionarySB(aesb);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<String, Short> aesn = new TreeMap<String, Short>();
            aesn.put("six", (short)2);
            proxy.DictionarySN(aesn);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<String, Integer> aesi = new TreeMap<String, Integer>();
            aesi.put("six", 3);
            proxy.DictionarySI(aesi);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<String, Long> aesx = new TreeMap<String, Long>();
            aesx.put("six", (long)4);
            proxy.DictionarySX(aesx);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<String, Double> aesd = new TreeMap<String, Double>();
            aesd.put("six", 5.1);
            proxy.DictionarySD(aesd);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<String, String> aess = new TreeMap<String, String>();
            aess.put("six", "six");
            proxy.DictionarySS(aess);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<String, byte[]> aesay = new TreeMap<String, byte[]>();
            aesay.put("six", new byte[] { (byte)7 });
            proxy.DictionarySAY(aesay);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<String, InferredTypesInterface.InnerStruct> aesr = new TreeMap<String, InferredTypesInterface.InnerStruct>();
            aesr.put("six", new InferredTypesInterface.InnerStruct(8));
            proxy.InferredDictionarySR(aesr);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<String, Variant> aesv = new TreeMap<String, Variant>();
            aesv.put("six", new Variant(new String("nine")));
            proxy.DictionarySV(aesv);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
        thrown = false;
        try {
            TreeMap<String, Map<String, String>> aesaess = new TreeMap<String, Map<String, String>>();
            aesaess.put("six", ae);
            proxy.DictionarySAESS(aesaess);
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);

        thrown = false;
        try {
            proxy.getDictionarySS();
        } catch (BusException ex) {
            thrown = true;
        }
        assertEquals(true, thrown);
    }
}
