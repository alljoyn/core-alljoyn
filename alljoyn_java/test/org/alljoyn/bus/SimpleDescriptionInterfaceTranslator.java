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
import org.alljoyn.bus.Translator;

public class SimpleDescriptionInterfaceTranslator extends Translator {

    @Override
    public String getTargetLanguage(int index) {
        if(index == 0) return "de";
        return null;
    }

    @Override
    public int numTargetLanguages() {

        return 1;
    }

    @Override
    public String translate(String fromLang, String toLang, String text) {      if(!toLang.equals("de")) return null;
        if(text.equals("My service object")) return "DE: My service object";
        if(text.equals("The ping method sends a small piece of data")) return "DE: The ping method sends a small piece of data";
        if(text.equals("This is a simple interface")) return "DE: This is a simple interface";
        return null;
    }
}