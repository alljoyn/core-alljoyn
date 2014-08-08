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

import org.alljoyn.bus.Variant;

import junit.framework.TestCase;

public class VariantTest extends TestCase {
    public VariantTest(String name) {
        super(name);
    }

    static {
        System.loadLibrary("alljoyn_java");
    }

    public void testClassCastException() throws BusException {
        boolean thrown = false;
        try {
            Variant v = new Variant(1);
            String s = v.getObject(String.class);
            fail("call to Variant.getObject returned " + s + " expected ClassCastException.");
        } catch (ClassCastException ex) {
            thrown = true;
        } finally {
            assertTrue(thrown);
        }
    }

    public void testGetSignature() throws Exception {
         Variant v = new Variant((byte)1);
         assertEquals("y", v.getSignature());
         v = new Variant(true);
         assertEquals("b", v.getSignature());
         v = new Variant((short)2);
         assertEquals("n", v.getSignature());
         v = new Variant(3);
         assertEquals("i", v.getSignature());
         v = new Variant((long)4);
         assertEquals("x", v.getSignature());
         v = new Variant(4.1);
         assertEquals("d", v.getSignature());
         v = new Variant("five");
         assertEquals("s", v.getSignature());
         v = new Variant(new byte[] { 6 });
         assertEquals("ay", v.getSignature());
         v = new Variant(new boolean[] { true });
         assertEquals("ab", v.getSignature());
         v = new Variant(new short[] { 7 });
         assertEquals("an", v.getSignature());
         v = new Variant(new int[] { 8 });
         assertEquals("ai", v.getSignature());
         v = new Variant(new long[] { 10 });
         assertEquals("ax", v.getSignature());
         v = new Variant(new double[] { 10.1 });
         assertEquals("ad", v.getSignature());
         v = new Variant(new String[] { "eleven" });
         assertEquals("as", v.getSignature());
         v = new Variant(new InferredTypesInterface.InnerStruct(12));
         assertEquals("(i)", v.getSignature());
         v = new Variant(new Variant(new String("thirteen")));
         assertEquals("v", v.getSignature());

         v = new Variant();
         assertNull(v.getSignature());
    }
}
