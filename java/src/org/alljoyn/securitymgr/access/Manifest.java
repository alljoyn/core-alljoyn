/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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

package org.alljoyn.securitymgr.access;

import java.util.ArrayList;
import java.util.List;

/**
 * A Class describing the Manifest of an application.
 */
public final class Manifest {
    private final ArrayList<Rule> rules = new ArrayList<Rule>();

    /**
     * Constructs an empty Manifest.
     */
    public Manifest() {
    }

    public Manifest(List<Rule> rulesSet) {
        rules.addAll(rulesSet);
    }

    /**
     * The set of permissions required by this manifest.
     *
     * @return a non-null List.
     */
    @SuppressWarnings("unchecked")
    public List<Rule> getRules() {
        return (List<Rule>) rules.clone();
    }

    @Override
    public int hashCode() {
        return 31 + rules.hashCode();
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof Manifest) {
            return rules.equals(((Manifest) obj).rules);
        }
        return false;
    }

    @Override
    public String toString() {
        return "Manifest [rules=" + rules + "]";
    }
}
