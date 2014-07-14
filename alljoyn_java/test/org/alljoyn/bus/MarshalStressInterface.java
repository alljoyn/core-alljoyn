/*
 * Copyright (c) 2009-2011, 2014, AllSeen Alliance. All rights reserved.
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

    @BusMethod(name="Methody", signature="y")
    public void methody(byte m) throws BusException;

    public class Structy {
        @Position(0)
        @Signature("y")
        public byte m;
    }

    @BusMethod(name="MethodStructy")
    public Structy methodStructy() throws BusException;

    @BusProperty(signature="b")
    public boolean getPropb() throws BusException;

    @BusMethod(name="Methodb", signature="b")
    public void methodb(boolean m) throws BusException;

    public class Structb {
        @Position(0)
        @Signature("b")
        public boolean m;
    }

    @BusMethod(name="MethodStructb")
    public Structb methodStructb() throws BusException;

    @BusProperty(signature="n")
    public short getPropn() throws BusException;

    @BusMethod(name="Methodn", signature="n")
    public void methodn(short m) throws BusException;

    public class Structn {
        @Position(0)
        @Signature("n")
        public short m;
    }

    @BusMethod(name="MethodStructn")
    public Structn methodStructn() throws BusException;

    @BusProperty(signature="q")
    public short getPropq() throws BusException;

    @BusMethod(name="Methodq", signature="q")
    public void methodq(short m) throws BusException;

    public class Structq {
        @Position(0)
        @Signature("q")
        public short m;
    }

    @BusMethod(name="MethodStructq")
    public Structq methodStructq() throws BusException;

    @BusProperty(signature="i")
    public int getPropi() throws BusException;

    @BusMethod(name="Methodi", signature="i")
    public void methodi(int m) throws BusException;

    public class Structi {
        @Position(0)
        @Signature("i")
        public int m;
    }

    @BusMethod(name="MethodStructi")
    public Structi methodStructi() throws BusException;

    @BusProperty(signature="u")
    public int getPropu() throws BusException;

    @BusMethod(name="Methodu", signature="u")
    public void methodu(int m) throws BusException;

    public class Structu {
        @Position(0)
        @Signature("u")
        public int m;
    }

    @BusMethod(name="MethodStructu")
    public Structu methodStructu() throws BusException;

    @BusProperty(signature="x")
    public long getPropx() throws BusException;

    @BusMethod(name="Methodx", signature="x")
    public void methodx(long m) throws BusException;

    public class Structx {
        @Position(0)
        @Signature("x")
        public long m;
    }

    @BusMethod(name="MethodStructx")
    public Structx methodStructx() throws BusException;

    @BusProperty(signature="t")
    public long getPropt() throws BusException;

    @BusMethod(name="Methodt", signature="t")
    public void methodt(long m) throws BusException;

    public class Structt {
        @Position(0)
        @Signature("t")
        public long m;
    }

    @BusMethod(name="MethodStructt")
    public Structt methodStructt() throws BusException;

    @BusProperty(signature="d")
    public double getPropd() throws BusException;

    @BusMethod(name="Methodd", signature="d")
    public void methodd(double m) throws BusException;

    public class Structd {
        @Position(0)
        @Signature("d")
        public double m;
    }

    @BusMethod(name="MethodStructd")
    public Structd methodStructd() throws BusException;

    @BusProperty(signature="s")
    public String getProps() throws BusException;

    @BusMethod(name="Methods", signature="s")
    public void methods(String m) throws BusException;

    public class Structs {
        @Position(0)
        @Signature("s")
        public String m;
    }

    @BusMethod(name="MethodStructs")
    public Structs methodStructs() throws BusException;

    @BusProperty(signature="o")
    public String getPropo() throws BusException;

    @BusMethod(name="Methodo", signature="o")
    public void methodo(String m) throws BusException;

    public class Structo {
        @Position(0)
        @Signature("o")
        public String m;
    }

    @BusMethod(name="MethodStructo")
    public Structo methodStructo() throws BusException;

    @BusProperty(signature="g")
    public String getPropg() throws BusException;

    @BusMethod(name="Methodg", signature="g")
    public void methodg(String m) throws BusException;

    public class Structg {
        @Position(0)
        @Signature("g")
        public String m;
    }

    @BusMethod(name="MethodStructg")
    public Structg methodStructg() throws BusException;

    @BusProperty(signature="ay")
    public byte[] getPropay() throws BusException;

    @BusMethod(name="Methoday", signature="ay")
    public void methoday(byte[] m) throws BusException;

    public class Structay {
        @Position(0)
        @Signature("ay")
        public byte[] m;
    }

    @BusMethod(name="MethodStructay")
    public Structay methodStructay() throws BusException;

    @BusProperty(signature="ab")
    public boolean[] getPropab() throws BusException;

    @BusMethod(name="Methodab", signature="ab")
    public void methodab(boolean[] m) throws BusException;

    public class Structab {
        @Position(0)
        @Signature("ab")
        public boolean[] m;
    }

    @BusMethod(name="MethodStructab")
    public Structab methodStructab() throws BusException;

    @BusProperty(signature="an")
    public short[] getPropan() throws BusException;

    @BusMethod(name="Methodan", signature="an")
    public void methodan(short[] m) throws BusException;

    public class Structan {
        @Position(0)
        @Signature("an")
        public short[] m;
    }

    @BusMethod(name="MethodStructan")
    public Structan methodStructan() throws BusException;

    @BusProperty(signature="aq")
    public short[] getPropaq() throws BusException;

    @BusMethod(name="Methodaq", signature="aq")
    public void methodaq(short[] m) throws BusException;

    public class Structaq {
        @Position(0)
        @Signature("aq")
        public short[] m;
    }

    @BusMethod(name="MethodStructaq")
    public Structaq methodStructaq() throws BusException;

    @BusProperty(signature="ai")
    public int[] getPropai() throws BusException;

    @BusMethod(name="Methodai", signature="ai")
    public void methodai(int[] m) throws BusException;

    public class Structai {
        @Position(0)
        @Signature("ai")
        public int[] m;
    }

    @BusMethod(name="MethodStructai")
    public Structai methodStructai() throws BusException;

    @BusProperty(signature="au")
    public int[] getPropau() throws BusException;

    @BusMethod(name="Methodau", signature="au")
    public void methodau(int[] m) throws BusException;

    public class Structau {
        @Position(0)
        @Signature("au")
        public int[] m;
    }

    @BusMethod(name="MethodStructau")
    public Structau methodStructau() throws BusException;

    @BusProperty(signature="ax")
    public long[] getPropax() throws BusException;

    @BusMethod(name="Methodax", signature="ax")
    public void methodax(long[] m) throws BusException;

    public class Structax {
        @Position(0)
        @Signature("ax")
        public long[] m;
    }

    @BusMethod(name="MethodStructax")
    public Structax methodStructax() throws BusException;

    @BusProperty(signature="at")
    public long[] getPropat() throws BusException;

    @BusMethod(name="Methodat", signature="at")
    public void methodat(long[] m) throws BusException;

    public class Structat {
        @Position(0)
        @Signature("at")
        public long[] m;
    }

    @BusMethod(name="MethodStructat")
    public Structat methodStructat() throws BusException;

    @BusProperty(signature="ad")
    public double[] getPropad() throws BusException;

    @BusMethod(name="Methodad", signature="ad")
    public void methodad(double[] m) throws BusException;

    public class Structad {
        @Position(0)
        @Signature("ad")
        public double[] m;
    }

    @BusMethod(name="MethodStructad")
    public Structad methodStructad() throws BusException;

    @BusProperty(signature="as")
    public String[] getPropas() throws BusException;

    @BusMethod(name="Methodas", signature="as")
    public void methodas(String[] m) throws BusException;

    public class Structas {
        @Position(0)
        @Signature("as")
        public String[] m;
    }

    @BusMethod(name="MethodStructas")
    public Structas methodStructas() throws BusException;

    @BusProperty(signature="ao")
    public String[] getPropao() throws BusException;

    @BusMethod(name="Methodao", signature="ao")
    public void methodao(String[] m) throws BusException;

    public class Structao {
        @Position(0)
        @Signature("ao")
        public String[] m;
    }

    @BusMethod(name="MethodStructao")
    public Structao methodStructao() throws BusException;

    @BusProperty(signature="ag")
    public String[] getPropag() throws BusException;

    @BusMethod(name="Methodag", signature="ag")
    public void methodag(String[] m) throws BusException;

    public class Structag {
        @Position(0)
        @Signature("ag")
        public String[] m;
    }

    @BusMethod(name="MethodStructag")
    public Structag methodStructag() throws BusException;

    public class Struct {
        @Position(0)
        @Signature("i")
        public int m;
    }

    @BusMethod(name="Methoda", signature="a")
    public void methoda(byte[] m) throws BusException;

    @BusMethod(name="Methodaei", signature="a{i")
    public void methodaei(Map<Integer, String> m) throws BusException;

    @BusMethod(name="Methodaers", signature="a{(i)s}")
    public void methodaers(Map<Struct, String> m) throws BusException;

    @BusMethod(name="IncompleteMethodaers", signature="a{(is}")
    public void incompleteMethodaers(Map<Struct, String> m) throws BusException;

    @BusMethod(name="Methodry", signature="r")
    public void methodry(Struct m) throws BusException;

    @BusMethod(name="Methodray", signature="r")
    public void methodray(Struct m) throws BusException;

    @BusMethod(name="Methodvy", signature="v")
    public void methodvy(Variant m) throws BusException;

    @BusMethod(name="Methodvay", signature="v")
    public void methodvay(Variant m) throws BusException;

    @BusProperty(signature="a{ss}")
    public Map<String, String> getPropaessy() throws BusException;

    @BusProperty(signature="a{ss}")
    public Map<String, String> getPropaesss() throws BusException;

    @BusMethod(name="Methodaessy", signature="a{ss}")
    public void methodaessy(Map<String, String> m) throws BusException;

    @BusMethod(name="Methodaessay", signature="a{ss}")
    public void methodaessay(Map<String, String> m) throws BusException;

    @BusProperty(signature="r")
    public Struct getPropr() throws BusException;

    @BusProperty(signature="ar")
    public Struct[] getPropar() throws BusException;

    @BusMethod(name="Methodv", replySignature="v")
    public Variant methodv() throws BusException;

    public class Structr {
        @Position(0)
        @Signature("(i)")
        public Struct m;
    }

    @BusMethod(name="MethodStructr", replySignature="((i))")
    public Structr methodStructr() throws BusException;

    public class Structv {
        @Position(0)
        @Signature("v")
        public Variant m;
    }

    @BusMethod(name="MethodStructv", replySignature="(v)")
    public Structv methodStructv() throws BusException;

    public class Structaess {
        @Position(0)
        @Signature("a{ss}")
        public Map<String, String> m;
    }

    @BusMethod(name="MethodStructaess", replySignature="(a{ss})")
    public Structaess methodStructaess() throws BusException;

}
