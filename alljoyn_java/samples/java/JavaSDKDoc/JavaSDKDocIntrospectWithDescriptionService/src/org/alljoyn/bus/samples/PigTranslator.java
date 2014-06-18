/*
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
