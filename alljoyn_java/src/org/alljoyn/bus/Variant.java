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

import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;

/**
 * A DBus/AllJoyn datatype whose type information is encoded into the
 * value field.
 */
public class Variant {

    /** Wrapped object */
    private Object value;

    /** Wrapped object signature */
    private String signature;

    /** Wrapped native object */
    private long handle;

    /**
     * Creates a variant that can represent any AllJoyn data type.
     * This constructor is typically used by message receivers (method and
     * signal handlers) when the actual data type represented by the variant is
     * not known.
     */
    Variant() {}

    /**
     * Creates a Variant that represents a specific AllJoyn data type and value.
     * This constructor is typically used by message senders (method calls and
     * signal emitters) when a specific Variant type and value are known at the
     * time the Variant is created.
     *
     * @param <T> the Java type the Variant contains
     * @param value object wrapped inside this Variant
     */
    public <T> Variant(T value) {
        this.value = value;
    }

    /**
     * Creates a Variant with an explicit signature that represents a specific
     * AllJoyn data type and instance value.
     * This constructor is typically used by message senders (method calls and
     * signal emitters) when a specific Variant type and value are known at the
     * time the Variant is created.
     *
     * @param <T> the Java type the Variant contains
     * @param value object wrapped inside the Variant
     * @param signature the DBus signature of value
     */
    public <T> Variant(T value, String signature) {
        this.value = value;
        this.signature = signature;
    }

    /** Releases native resources */
    private synchronized native void destroy();

    /** Releases resources. */
    protected void finalize() throws Throwable {
        try {
            destroy();
        } finally {
            super.finalize();
        }
    }

    /** Gets the wrapped native object. */
    long getMsgArg() { return handle; }

    /** Sets the wrapped native object to the MsgArg. */
    native void setMsgArg(long msgArg);

    /** Gets the wrapped object. */
    Object getValue() { return value; }

    /**
     * Gets the DBus type signature of the wrapped object.
     *
     * @throws AnnotationBusException This exception will occur if
     * <ul>
     * <li>a field of a user-defined type is not annotated with its position,
     * <li>a Java data type that is not supported is used,
     * <li>an Enum data type is not annotated with a valid AllJoyn type
     * </ul>
     * @return the signature of the wrapped object.
     */
    public String getSignature() throws AnnotationBusException {
        if (value != null) {
            return Signature.typeSig(value.getClass(), signature);
        } else if( handle != 0) {
            return MsgArg.getSignature(new long[] { handle });
        } else {
            return null;
        }
    }

    /**
     * Gets the object wrapped by this Variant when object is specified by Type.
     *
     * @param <T> type returned from the getObject call.
     * @param type the type of the return value.
     * @return the value of the Variant.
     * @throws BusException if Variant data cannot be unmarshalled
     * @throws ClassCastException if Variant data cannot be converted to requested type
     */
    public <T> T getObject(Type type) throws BusException {
        if (value == null) {
            value = MsgArg.unmarshal(handle, type);
        }
        @SuppressWarnings(value = "unchecked")
        T o = (T) value;
        return o;
    }

    /**
     * Gets the object wrapped by this Variant when object is specified by Class.
     *
     * @param <T> type returned from the getObject call.
     * @param type the type of the return value.
     * @return the value of the Variant.
     * @throws BusException if Variant data cannot be unmarshalled
     * @throws ClassCastException if Variant data cannot be converted to requested type
     */
    public <T> T getObject(Class<T> type) throws BusException {
        if (value == null) {
            value = MsgArg.unmarshal(handle, type);
        }
        @SuppressWarnings(value = "unchecked")
        T o = (T) value;
        return o;
    }

    /**
     * Gets the object wrapped by this Variant when the wrapped object is a
     * generic type.
     *
     * @param <T> type returned from the getObject call.
     * @param type the VariantTypeReference of the return value
     * @return the value of the Variant
     * @throws BusException if Variant data cannot be unmarshalled
     * @throws ClassCastException if Variant data cannot be converted to requested type
     * @see VariantTypeReference
     */
    public <T> T getObject(VariantTypeReference<T> type) throws BusException {
        if (value == null) {
            Type sc = type.getClass().getGenericSuperclass();
            value = MsgArg.unmarshal(handle, ((ParameterizedType) sc).getActualTypeArguments()[0]);
        }
        @SuppressWarnings(value = "unchecked")
        T o = (T) value;
        return o;
    }

    /**
     * Indicates whether some other Variant is "equal to" this one.
     *
     * @param obj the other Variant
     * @return {@code true} when variants values are equal to one another
     */
    public boolean equals(Object obj) {
        try {
            Object obj1 = getObject(value.getClass());
            Object obj2 = ((Variant) obj).getObject(value.getClass());
            return obj1.equals(obj2);
        } catch (BusException ex) {
            return false;
        } catch (ClassCastException ex) {
            return false;
        }
    }
}
