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
 */
public abstract class Translator {

    /**
     * Create native resources held by objects of this class.
     */
    public Translator() {
        create();
    }

    /**
     * Destroy native resources held by objects of this class.
     */
    protected void finalize() throws Throwable {
        try {
            destroy();
        } finally {
            super.finalize();
        }
    }

    /**
     * Create any native resources held by objects of this class.  Specifically,
     * we allocate a C++ counterpart of this Translator object.
     */
    private native void create();

    /**
     * Release any native resources held by objects of this class.
     * Specifically, we may delete a C++ counterpart of this Translator object.
     */
    private native void destroy();

    /**
     * Get the number of target languages this Translator supports
     *
     * @return numver of target languages
     */
    public abstract int numTargetLanguages();

    /**
     * Retrieve one of the list of target languages this Translator supports
     *
     * @param index the index of the requested target language within the list
     * @return The requested target language or null if index is out of bounds
     */
    public abstract String getTargetLanguage(int index);

    /**
     * Translate a string into another language
     *
     * @param fromLanguage the language to translate from
     * @param toLanguage the language to translate to
     * @param fromText the text to be translated
     * @return the translated text or null if it can not be translated
     */
    public abstract String translate(String fromLanguage, String toLanguage, String fromText);

    /**
     * The opaque pointer to the underlying C++ object which is actually tied
     * to the AllJoyn code.
     */
    private long handle = 0;
}