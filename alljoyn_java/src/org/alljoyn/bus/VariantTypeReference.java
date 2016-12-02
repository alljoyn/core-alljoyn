/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 */

package org.alljoyn.bus;

/**
 * Used when calling {@link Variant#getObject(Class)} where the wrapped
 * object is a generic type such as {@code Map<K, V>}.  For example, the
 * proper syntax is {@code getObject(new VariantTypeReference<TreeMap<String,
 * String>>() {})} when the wrapped object is a {@code TreeMap<String,
 * String>}.
 */
public abstract class VariantTypeReference<T> {
}