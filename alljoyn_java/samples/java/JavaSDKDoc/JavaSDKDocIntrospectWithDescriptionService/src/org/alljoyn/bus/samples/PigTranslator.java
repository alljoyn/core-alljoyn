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

package org.alljoyn.bus.samples;

import org.alljoyn.bus.Translator;

public class PigTranslator extends Translator {

    public int numTargetLanguages(){
        return 1;
    }

    public String getTargetLanguage(int index){
        if(0 == index) {
            return "pig";
        } else {
            return null;
        }
    }

    public String translate(String fromLanguage, String toLanguage, String fromText){
        if(toLanguage.equals("en") || null == fromText) {
            return null;
        }

        StringBuilder res = new StringBuilder();
        String lower = fromText.toLowerCase();

        for(String word : lower.split("\\s", 0)){
            if(word.charAt(0) < 'a' || word.charAt(0) > 'z' || word.length() == 1) {
                res.append(word);
                res.append(' ');
                continue;
            }

            String firstTwoChars = word.substring(0, 2);
            int cutAt = 1;
            if(firstTwoChars.equals("sh") || firstTwoChars.equals("ch") ||
                    firstTwoChars.equals("th") || firstTwoChars.equals("ph") ||
                    firstTwoChars.equals("wh") || firstTwoChars.equals("kn") ||
                    firstTwoChars.equals("tr")){
                cutAt = 2;
            }

            res.append(word.substring(cutAt));
            res.append(word.substring(0, cutAt));
            res.append("ay ");
        }

        res.setLength(res.length() - 1);

        return res.toString();
    }
}