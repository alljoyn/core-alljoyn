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
 * A Term is a collection of {@link Rule}s and {@link Peer}s to which the rules
 * apply.
 */
public class Term {
    private final ArrayList<Rule> rules = new ArrayList<Rule>();
    private final ArrayList<Peer> peers = new ArrayList<Peer>();

    /**
     * Adds a Rule to the Term.
     *
     * @param newRule a non-null Rule.
     */
    public void addRule(Rule newRule) {
        rules.add(newRule);
    }

    /**
     * Adds a Peer to the Term.
     *
     * @param newPeer a non-null Peer
     */
    public void addPeer(Peer newPeer) {
        peers.add(newPeer);
    }

    /**
     * The {@link Rule}s held by the Term.
     *
     * @return a non-null List.
     */
    @SuppressWarnings("unchecked")
    public List<Rule> getRules() {
        return (List<Rule>) rules.clone();
    }

    /**
     * The {@link Peer}s to which the Term applies.
     *
     * @return a non-null List.
     */
    @SuppressWarnings("unchecked")
    public List<Peer> getPeers() {
        return (List<Peer>) peers.clone();
    }

    @Override
    public int hashCode() {
        return 37 + rules.hashCode();
    }

    @Override
    public boolean equals(Object obj) {
        if(obj instanceof Term) {
            return rules.equals(((Term)obj).rules);
        }
        return false;
    }

    /* Helper function for the JNI layer. */
    Peer[] getPeerArray() {
        return peers.toArray(new Peer[peers.size()]);
    }

    /* Helper function for the JNI layer. */
    Rule[] getRuleArray() {
        return rules.toArray(new Rule[rules.size()]);
    }
}
