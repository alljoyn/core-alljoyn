/*
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;
import org.alljoyn.bus.annotation.BusProperty;
import org.alljoyn.bus.annotation.Position;
import org.alljoyn.bus.annotation.Signature;

import java.util.Map;

@BusInterface(name="org.alljoyn.bus.MarshalStressInterface")
public interface MarshalStressInterface {

    public enum Enum {
        VALUE0,
        VALUE1
    }

    @BusProperty(signature="y")
    public byte getPropy() throws BusException;

    @BusMethod(signature="y")
    public void Methody(byte m) throws BusException;

    public class Structy {
        @Position(0)
        @Signature("y")
        public byte m;
    }

    @BusMethod
    public Structy MethodStructy() throws BusException;

    @BusProperty(signature="b")
    public boolean getPropb() throws BusException;

    @BusMethod(signature="b")
    public void Methodb(boolean m) throws BusException;

    public class Structb {
        @Position(0)
        @Signature("b")
        public boolean m;
    }

    @BusMethod
    public Structb MethodStructb() throws BusException;

    @BusProperty(signature="n")
    public short getPropn() throws BusException;

    @BusMethod(signature="n")
    public void Methodn(short m) throws BusException;

    public class Structn {
        @Position(0)
        @Signature("n")
        public short m;
    }

    @BusMethod
    public Structn MethodStructn() throws BusException;

    @BusProperty(signature="q")
    public short getPropq() throws BusException;

    @BusMethod(signature="q")
    public void Methodq(short m) throws BusException;

    public class Structq {
        @Position(0)
        @Signature("q")
        public short m;
    }

    @BusMethod
    public Structq MethodStructq() throws BusException;

    @BusProperty(signature="i")
    public int getPropi() throws BusException;

    @BusMethod(signature="i")
    public void Methodi(int m) throws BusException;

    public class Structi {
        @Position(0)
        @Signature("i")
        public int m;
    }

    @BusMethod
    public Structi MethodStructi() throws BusException;

    @BusProperty(signature="u")
    public int getPropu() throws BusException;

    @BusMethod(signature="u")
    public void Methodu(int m) throws BusException;

    public class Structu {
        @Position(0)
        @Signature("u")
        public int m;
    }

    @BusMethod
    public Structu MethodStructu() throws BusException;

    @BusProperty(signature="x")
    public long getPropx() throws BusException;

    @BusMethod(signature="x")
    public void Methodx(long m) throws BusException;

    public class Structx {
        @Position(0)
        @Signature("x")
        public long m;
    }

    @BusMethod
    public Structx MethodStructx() throws BusException;

    @BusProperty(signature="t")
    public long getPropt() throws BusException;

    @BusMethod(signature="t")
    public void Methodt(long m) throws BusException;

    public class Structt {
        @Position(0)
        @Signature("t")
        public long m;
    }

    @BusMethod
    public Structt MethodStructt() throws BusException;

    @BusProperty(signature="d")
    public double getPropd() throws BusException;

    @BusMethod(signature="d")
    public void Methodd(double m) throws BusException;

    public class Structd {
        @Position(0)
        @Signature("d")
        public double m;
    }

    @BusMethod
    public Structd MethodStructd() throws BusException;

    @BusProperty(signature="s")
    public String getProps() throws BusException;

    @BusMethod(signature="s")
    public void Methods(String m) throws BusException;

    public class Structs {
        @Position(0)
        @Signature("s")
        public String m;
    }

    @BusMethod
    public Structs MethodStructs() throws BusException;

    @BusProperty(signature="o")
    public String getPropo() throws BusException;

    @BusMethod(signature="o")
    public void Methodo(String m) throws BusException;

    public class Structo {
        @Position(0)
        @Signature("o")
        public String m;
    }

    @BusMethod
    public Structo MethodStructo() throws BusException;

    @BusProperty(signature="g")
    public String getPropg() throws BusException;

    @BusMethod(signature="g")
    public void Methodg(String m) throws BusException;

    public class Structg {
        @Position(0)
        @Signature("g")
        public String m;
    }

    @BusMethod
    public Structg MethodStructg() throws BusException;

    @BusProperty(signature="ay")
    public byte[] getPropay() throws BusException;

    @BusMethod(signature="ay")
    public void Methoday(byte[] m) throws BusException;

    public class Structay {
        @Position(0)
        @Signature("ay")
        public byte[] m;
    }

    @BusMethod
    public Structay MethodStructay() throws BusException;

    @BusProperty(signature="ab")
    public boolean[] getPropab() throws BusException;

    @BusMethod(signature="ab")
    public void Methodab(boolean[] m) throws BusException;

    public class Structab {
        @Position(0)
        @Signature("ab")
        public boolean[] m;
    }

    @BusMethod
    public Structab MethodStructab() throws BusException;

    @BusProperty(signature="an")
    public short[] getPropan() throws BusException;

    @BusMethod(signature="an")
    public void Methodan(short[] m) throws BusException;

    public class Structan {
        @Position(0)
        @Signature("an")
        public short[] m;
    }

    @BusMethod
    public Structan MethodStructan() throws BusException;

    @BusProperty(signature="aq")
    public short[] getPropaq() throws BusException;

    @BusMethod(signature="aq")
    public void Methodaq(short[] m) throws BusException;

    public class Structaq {
        @Position(0)
        @Signature("aq")
        public short[] m;
    }

    @BusMethod
    public Structaq MethodStructaq() throws BusException;

    @BusProperty(signature="ai")
    public int[] getPropai() throws BusException;

    @BusMethod(signature="ai")
    public void Methodai(int[] m) throws BusException;

    public class Structai {
        @Position(0)
        @Signature("ai")
        public int[] m;
    }

    @BusMethod
    public Structai MethodStructai() throws BusException;

    @BusProperty(signature="au")
    public int[] getPropau() throws BusException;

    @BusMethod(signature="au")
    public void Methodau(int[] m) throws BusException;

    public class Structau {
        @Position(0)
        @Signature("au")
        public int[] m;
    }

    @BusMethod
    public Structau MethodStructau() throws BusException;

    @BusProperty(signature="ax")
    public long[] getPropax() throws BusException;

    @BusMethod(signature="ax")
    public void Methodax(long[] m) throws BusException;

    public class Structax {
        @Position(0)
        @Signature("ax")
        public long[] m;
    }

    @BusMethod
    public Structax MethodStructax() throws BusException;

    @BusProperty(signature="at")
    public long[] getPropat() throws BusException;

    @BusMethod(signature="at")
    public void Methodat(long[] m) throws BusException;

    public class Structat {
        @Position(0)
        @Signature("at")
        public long[] m;
    }

    @BusMethod
    public Structat MethodStructat() throws BusException;

    @BusProperty(signature="ad")
    public double[] getPropad() throws BusException;

    @BusMethod(signature="ad")
    public void Methodad(double[] m) throws BusException;

    public class Structad {
        @Position(0)
        @Signature("ad")
        public double[] m;
    }

    @BusMethod
    public Structad MethodStructad() throws BusException;

    @BusProperty(signature="as")
    public String[] getPropas() throws BusException;

    @BusMethod(signature="as")
    public void Methodas(String[] m) throws BusException;

    public class Structas {
        @Position(0)
        @Signature("as")
        public String[] m;
    }

    @BusMethod
    public Structas MethodStructas() throws BusException;

    @BusProperty(signature="ao")
    public String[] getPropao() throws BusException;

    @BusMethod(signature="ao")
    public void Methodao(String[] m) throws BusException;

    public class Structao {
        @Position(0)
        @Signature("ao")
        public String[] m;
    }

    @BusMethod
    public Structao MethodStructao() throws BusException;

    @BusProperty(signature="ag")
    public String[] getPropag() throws BusException;

    @BusMethod(signature="ag")
    public void Methodag(String[] m) throws BusException;

    public class Structag {
        @Position(0)
        @Signature("ag")
        public String[] m;
    }

    @BusMethod
    public Structag MethodStructag() throws BusException;

    public class Struct {
        @Position(0)
        @Signature("i")
        public int m;
    }

    @BusMethod(signature="a")
    public void Methoda(byte[] m) throws BusException;

    @BusMethod(signature="a{i")
    public void Methodaei(Map<Integer, String> m) throws BusException;

    @BusMethod(signature="a{(i)s}")
    public void Methodaers(Map<Struct, String> m) throws BusException;

    @BusMethod(signature="a{(is}")
    public void IncompleteMethodaers(Map<Struct, String> m) throws BusException;

    @BusMethod(signature="r")
    public void Methodry(Struct m) throws BusException;

    @BusMethod(signature="r")
    public void Methodray(Struct m) throws BusException;

    @BusMethod(signature="v")
    public void Methodvy(Variant m) throws BusException;

    @BusMethod(signature="v")
    public void Methodvay(Variant m) throws BusException;

    @BusProperty(signature="a{ss}")
    public Map<String, String> getPropaessy() throws BusException;

    @BusProperty(signature="a{ss}")
    public Map<String, String> getPropaesss() throws BusException;

    @BusMethod(signature="a{ss}")
    public void Methodaessy(Map<String, String> m) throws BusException;

    @BusMethod(signature="a{ss}")
    public void Methodaessay(Map<String, String> m) throws BusException;

    @BusProperty(signature="r")
    public Struct getPropr() throws BusException;

    @BusProperty(signature="ar")
    public Struct[] getPropar() throws BusException;

    @BusMethod(replySignature="v")
    public Variant Methodv() throws BusException;

    public class Structr {
        @Position(0)
        @Signature("(i)")
        public Struct m;
    }

    @BusMethod(replySignature="((i))")
    public Structr MethodStructr() throws BusException;

    public class Structv {
        @Position(0)
        @Signature("v")
        public Variant m;
    }

    @BusMethod(replySignature="(v)")
    public Structv MethodStructv() throws BusException;

    public class Structaess {
        @Position(0)
        @Signature("a{ss}")
        public Map<String, String> m;
    }

    @BusMethod(replySignature="(a{ss})")
    public Structaess MethodStructaess() throws BusException;

}
