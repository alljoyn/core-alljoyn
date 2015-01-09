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

package org.alljoyn.bus.annotation;

import java.lang.annotation.Documented;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Declares the AllJoyn data type of a Java data type.
 *
 * Valid type declarations are:
 * <table border="1" width="100%" cellpadding="3" cellspacing="0" summary="All valid declarations">
 *   <tr bgcolor="#ccccff" class="tableheadingcolor"><th><b>Type Id</b></th>
 *      <th><b>AllJoyn type</b></th><th><b>Compatible Java types</b></th></tr>
 *   <tr><td>y</td><td>BYTE</td><td>byte, Enum<a href="#note-1"><sup>[1]</sup></a></td></tr>
 *   <tr><td>b</td><td>BOOLEAN</td><td>boolean</td></tr>
 *   <tr><td>n</td><td>INT16</td><td>short, Enum<a href="#note-1"><sup>[1]</sup></a></td></tr>
 *   <tr><td>q</td><td>UINT16</td><td>short, Enum<a href="#note-1"><sup>[1]</sup></a></td></tr>
 *   <tr><td>i</td><td>INT32</td><td>int, Enum<a href="#note-1"><sup>[1]</sup></a></td></tr>
 *   <tr><td>u</td><td>UINT32</td><td>int, Enum<a href="#note-1"><sup>[1]</sup></a></td></tr>
 *   <tr><td>x</td><td>INT64</td><td>long, Enum<a href="#note-1"><sup>[1]</sup></a></td></tr>
 *   <tr><td>t</td><td>UINT64</td><td>long, Enum<a href="#note-1"><sup>[1]</sup></a></td></tr>
 *   <tr><td>d</td><td>DOUBLE</td><td>double</td></tr>
 *   <tr><td>s</td><td>STRING</td><td>String<a href="#note-2"><sup>[2]</sup></a></td></tr>
 *   <tr><td>o</td><td>OBJECT_PATH</td><td>String<a href="#note-2"><sup>[2]</sup></a></td></tr>
 *   <tr><td>g</td><td>SIGNATURE</td><td>String<a href="#note-2"><sup>[2]</sup></a></td></tr>
 *   <tr><td>a</td><td>ARRAY</td><td>Array. The array type code must be followed by a <em>single
 *      complete type</em>.</td></tr>
 *   <tr><td>r</td><td>STRUCT</td><td>User-defined type<a href="#note-3"><sup>[3]</sup></a> whose fields are annotated with
 *      {@link Position} and {@link Signature}</td></tr>
 *   <tr><td>v</td><td>VARIANT</td><td>{@link org.alljoyn.bus.Variant}</td></tr>
 *   <tr><td>a{TS}</td><td>DICTIONARY</td><td>Map&lt;JT,JS&gt; where T and S are AllJoyn type ids and JT and
 *      JS are compatible Java types</td></tr>
 * </table>
 * <ol>
 * <li><a name="note-1">The ordinal numbers of the enumeration constant must correspond to the values of the
 *      AllJoyn type.</a></li>
 * <li><a name="note-2">Automatically converted to/from UTF-8(AllJoyn) to UTF-16 (Java).</a></li>
 * <li><a name="note-3">The user-defined type must supply a parameterless constructor.  Any nested classes
 *        of the user-defined type must be static nested classes.</a></li>
 * </ol>
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.FIELD)
public @interface Signature {

    /**
     * AllJoyn type declaration.
     * The default AllJoyn type is the "natural" Java type.  For example:
     * <ul>
     * <li>Java short defaults to "n", not "q",
     * <li>Java int defaults to "i", not "u",
     * <li>and Java String defaults to "s".
     * </ul>
     *
     * @return signature value
     */
    String value() default "";
}
