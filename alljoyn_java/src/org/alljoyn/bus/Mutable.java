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

/**
 * The underlying C++ implementation of the AllJoyn bus is fond of using in/out
 * parameters.  Rather than fight this, changing the calling functions to
 * something almost unrecognizable to a person who speaks the C++ version, we
 * provide classes to allow this.  This does mean that we need to allocate
 * parameters on the heap, but we expect infrequent use of the bus methods
 * that use these.
 */
public class Mutable {

    /**
     * A class providing [in,out] parameter semantics for Java Strings.
     */
    static public class StringValue {
        /**
         * Construct a StringValue.
         */
        public StringValue() {
            value = "";
        }

        /**
         * Construct a StringValue with the given value.
         *
         * @param string String used to construct a StringValue
         */
        public StringValue(String string) {
            value = string;
        }

        /**
         * The string in question.
         */
        public String value;
    }

    /**
     * A class providing inout parameter semantics for Java ints.
     */
    static public class IntegerValue {
        /**
         * Construct an IntegerValue.
         */
        public IntegerValue() {
            value = 0;
        }

        /**
         * Construct an IntegerValue with the given value.
         *
         * @param v int used to construct an IntegerValue
         */
        public IntegerValue(int v) {
            value = v;
        }

        /**
         * The int in question.
         */
        public int value;
    }

    /**
     * A class providing inout parameter semantics for Java shorts.
     */
    static public class ShortValue {
        /**
         * Construct a ShortValue.
         */
        public ShortValue() {
            value = 0;
        }

        /**
         * Construct a ShortValue with the given value.
         *
         * @param v short used to construct a ShortValue
         */
        public ShortValue(short v) {
            value = v;
        }

        /**
         * The short in question.
         */
        public short value;
    }
}
