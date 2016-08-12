/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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
 ******************************************************************************/

package org.alljoyn.bus;

import org.alljoyn.bus.annotation.Position;
import java.util.Arrays;

/**
 * the SHA-256 digest of the manifest template.
 */

public class ManifestTemplateRule {

    /** 
     * objPath of interface
     */
    @Position(0) public String objPath;

    /**
     * interfaceName of interface
     */
    @Position(1) public String interfaceName;

    /**
     * recommendedSecurityLevel for this particular interface
     */
    @Position(2) public byte recommendedSecurityLevel;

    /** 
     * the rules of this interface
     */
    @Position(3) public ManifestRuleMember[] rules;

    public String toString() {
        return objPath + ", " + interfaceName + ", " + Byte.toString(recommendedSecurityLevel) + ", " + Arrays.toString(rules);
    }
}
