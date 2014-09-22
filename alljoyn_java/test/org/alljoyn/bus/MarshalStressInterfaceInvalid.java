/*
 * Copyright (c) 2009-2011,2014 AllSeen Alliance. All rights reserved.
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
public interface MarshalStressInterfaceInvalid {

    public enum Enum {
        VALUE0,
        VALUE1
    }

    @BusProperty(signature="y")
    public Class<?> getPropy() throws BusException;

    @BusMethod(signature="y")
    public void Methody(Class<?> m) throws BusException;

    public class Structy {
        @Position(0)
        @Signature("y")
        public Class<?> m;
    }

    @BusMethod
    public Structy MethodStructy() throws BusException;

    @BusProperty(signature="b")
    public Class<?> getPropb() throws BusException;

    @BusMethod(signature="b")
    public void Methodb(Class<?> m) throws BusException;

    public class Structb {
        @Position(0)
        @Signature("b")
        public Class<?> m;
    }

    @BusMethod
    public Structb MethodStructb() throws BusException;

    @BusProperty(signature="n")
    public Class<?> getPropn() throws BusException;

    @BusMethod(signature="n")
    public void Methodn(Class<?> m) throws BusException;

    public class Structn {
        @Position(0)
        @Signature("n")
        public Class<?> m;
    }

    @BusMethod
    public Structn MethodStructn() throws BusException;

    @BusProperty(signature="q")
    public Class<?> getPropq() throws BusException;

    @BusMethod(signature="q")
    public void Methodq(Class<?> m) throws BusException;

    public class Structq {
        @Position(0)
        @Signature("q")
        public Class<?> m;
    }

    @BusMethod
    public Structq MethodStructq() throws BusException;

    @BusProperty(signature="i")
    public Class<?> getPropi() throws BusException;

    @BusMethod(signature="i")
    public void Methodi(Class<?> m) throws BusException;

    public class Structi {
        @Position(0)
        @Signature("i")
        public Class<?> m;
    }

    @BusMethod
    public Structi MethodStructi() throws BusException;

    @BusProperty(signature="u")
    public Class<?> getPropu() throws BusException;

    @BusMethod(signature="u")
    public void Methodu(Class<?> m) throws BusException;

    public class Structu {
        @Position(0)
        @Signature("u")
        public Class<?> m;
    }

    @BusMethod
    public Structu MethodStructu() throws BusException;

    @BusProperty(signature="x")
    public Class<?> getPropx() throws BusException;

    @BusMethod(signature="x")
    public void Methodx(Class<?> m) throws BusException;

    public class Structx {
        @Position(0)
        @Signature("x")
        public Class<?> m;
    }

    @BusMethod
    public Structx MethodStructx() throws BusException;

    @BusProperty(signature="t")
    public Class<?> getPropt() throws BusException;

    @BusMethod(signature="t")
    public void Methodt(Class<?> m) throws BusException;

    public class Structt {
        @Position(0)
        @Signature("t")
        public Class<?> m;
    }

    @BusMethod
    public Structt MethodStructt() throws BusException;

    @BusProperty(signature="d")
    public Class<?> getPropd() throws BusException;

    @BusMethod(signature="d")
    public void Methodd(Class<?> m) throws BusException;

    public class Structd {
        @Position(0)
        @Signature("d")
        public Class<?> m;
    }

    @BusMethod
    public Structd MethodStructd() throws BusException;

    @BusProperty(signature="s")
    public Class<?> getProps() throws BusException;

    @BusMethod(signature="s")
    public void Methods(Class<?> m) throws BusException;

    public class Structs {
        @Position(0)
        @Signature("s")
        public Class<?> m;
    }

    @BusMethod
    public Structs MethodStructs() throws BusException;

    @BusProperty(signature="o")
    public Class<?> getPropo() throws BusException;

    @BusMethod(signature="o")
    public void Methodo(Class<?> m) throws BusException;

    public class Structo {
        @Position(0)
        @Signature("o")
        public Class<?> m;
    }

    @BusMethod
    public Structo MethodStructo() throws BusException;

    @BusProperty(signature="g")
    public Class<?> getPropg() throws BusException;

    @BusMethod(signature="g")
    public void Methodg(Class<?> m) throws BusException;

    public class Structg {
        @Position(0)
        @Signature("g")
        public Class<?> m;
    }

    @BusMethod
    public Structg MethodStructg() throws BusException;

    @BusProperty(signature="ay")
    public Class<?>[] getPropay() throws BusException;

    @BusMethod(signature="ay")
    public void Methoday(Class<?>[] m) throws BusException;
    public class Structay {
        @Position(0)
        @Signature("ay")
        public Class<?>[] m;
    }


    @BusMethod
    public Structay MethodStructay() throws BusException;

    @BusProperty(signature="ab")
    public Class<?>[] getPropab() throws BusException;

    @BusMethod(signature="ab")
    public void Methodab(Class<?>[] m) throws BusException;
    public class Structab {
        @Position(0)
        @Signature("ab")
        public Class<?>[] m;
    }


    @BusMethod
    public Structab MethodStructab() throws BusException;

    @BusProperty(signature="an")
    public Class<?>[] getPropan() throws BusException;

    @BusMethod(signature="an")
    public void Methodan(Class<?>[] m) throws BusException;
    public class Structan {
        @Position(0)
        @Signature("an")
        public Class<?>[] m;
    }


    @BusMethod
    public Structan MethodStructan() throws BusException;

    @BusProperty(signature="aq")
    public Class<?>[] getPropaq() throws BusException;

    @BusMethod(signature="aq")
    public void Methodaq(Class<?>[] m) throws BusException;
    public class Structaq {
        @Position(0)
        @Signature("aq")
        public Class<?>[] m;
    }


    @BusMethod
    public Structaq MethodStructaq() throws BusException;

    @BusProperty(signature="ai")
    public Class<?>[] getPropai() throws BusException;

    @BusMethod(signature="ai")
    public void Methodai(Class<?>[] m) throws BusException;
    public class Structai {
        @Position(0)
        @Signature("ai")
        public Class<?>[] m;
    }


    @BusMethod
    public Structai MethodStructai() throws BusException;

    @BusProperty(signature="au")
    public Class<?>[] getPropau() throws BusException;

    @BusMethod(signature="au")
    public void Methodau(Class<?>[] m) throws BusException;
    public class Structau {
        @Position(0)
        @Signature("au")
        public Class<?>[] m;
    }


    @BusMethod
    public Structau MethodStructau() throws BusException;

    @BusProperty(signature="ax")
    public Class<?>[] getPropax() throws BusException;

    @BusMethod(signature="ax")
    public void Methodax(Class<?>[] m) throws BusException;
    public class Structax {
        @Position(0)
        @Signature("ax")
        public Class<?>[] m;
    }


    @BusMethod
    public Structax MethodStructax() throws BusException;

    @BusProperty(signature="at")
    public Class<?>[] getPropat() throws BusException;

    @BusMethod(signature="at")
    public void Methodat(Class<?>[] m) throws BusException;
    public class Structat {
        @Position(0)
        @Signature("at")
        public Class<?>[] m;
    }


    @BusMethod
    public Structat MethodStructat() throws BusException;

    @BusProperty(signature="ad")
    public Class<?>[] getPropad() throws BusException;

    @BusMethod(signature="ad")
    public void Methodad(Class<?>[] m) throws BusException;
    public class Structad {
        @Position(0)
        @Signature("ad")
        public Class<?>[] m;
    }


    @BusMethod
    public Structad MethodStructad() throws BusException;

    @BusProperty(signature="as")
    public int[] getPropas() throws BusException;

    @BusMethod(signature="as")
    public void Methodas(int[] m) throws BusException;
    public class Structas {
        @Position(0)
        @Signature("as")
        public int[] m;
    }


    @BusMethod
    public Structas MethodStructas() throws BusException;

    @BusProperty(signature="ao")
    public int[] getPropao() throws BusException;

    @BusMethod(signature="ao")
    public void Methodao(int[] m) throws BusException;
    public class Structao {
        @Position(0)
        @Signature("ao")
        public int[] m;
    }


    @BusMethod
    public Structao MethodStructao() throws BusException;

    @BusProperty(signature="ag")
    public int[] getPropag() throws BusException;

    @BusMethod(signature="ag")
    public void Methodag(int[] m) throws BusException;
    public class Structag {
        @Position(0)
        @Signature("ag")
        public int[] m;
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

    @BusMethod(signature="(i)")
    public void Methodry(byte m) throws BusException;

    @BusMethod(signature="(i)")
    public void Methodray(byte[] m) throws BusException;

    @BusMethod(signature="v")
    public void Methodvy(byte m) throws BusException;

    @BusMethod(signature="v")
    public void Methodvay(byte[] m) throws BusException;

    @BusProperty(signature="a{ss}")
    public byte getPropaessy() throws BusException;

    @BusProperty(signature="a{ss}")
    public String getPropaesss() throws BusException;

    @BusMethod(signature="a{ss}")
    public void Methodaessy(byte m) throws BusException;

    @BusMethod(signature="a{ss}")
    public void Methodaessay(byte[] m) throws BusException;

    @BusProperty(signature="r")
    public byte getPropr() throws BusException;

    @BusProperty(signature="ar")
    public String getPropar() throws BusException;

    @BusMethod(replySignature="v")
    public int Methodv() throws BusException;

    public class Structr {
        @Position(0)
        @Signature("((i))")
        public int m;
    }

    @BusMethod(replySignature="((i))")
    public Structr MethodStructr() throws BusException;

    public class Structv {
        @Position(0)
        @Signature("v")
        public int m;
    }

    @BusMethod(replySignature="(v)")
    public Structv MethodStructv() throws BusException;

    public class Structaess {
        @Position(0)
        @Signature("a{ss}")
        public int m;
    }

    @BusMethod(replySignature="(a{ss})")
    public Structaess MethodStructaess() throws BusException;

}
