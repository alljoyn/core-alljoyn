/******************************************************************************
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
 ******************************************************************************/

package org.alljoyn.securitymgr.access;

import java.util.ArrayList;
import java.util.List;

/**
 * A Rule provide info on how an interface can be accessed.
 */
public final class Rule {
    private final String name;
    private final List<Member> members = new ArrayList<Member>();

    /**
     * Constructs a Rule providing full access to all members of the interface.
     *
     * @param intfName a non-null interface name.
     */
    public Rule(String intfName) {
        name = intfName;
        members.add(new Member());
    }

    /**
     * Constructs a Rule providing access to the specified members of the
     * interface.
     *
     * @param inftName a non-null interface name.
     * @param memberList a list of non-null Members.
     */
    public Rule(String inftName, Member ... memberList) {
        name = inftName;
        if (memberList.length == 0) {
            members.add(new Member());
        } else {
            for (int i = 0; i < memberList.length; i++) {
                members.add(memberList[i]);
            }
        }
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = prime + members.hashCode();
        result = prime * result + ((name == null) ? 0 : name.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (!(obj instanceof Rule)) {
            return false;
        }
        Rule other = (Rule) obj;
        return name == null ? other.name == null : name.equals(other.name)
                && members.equals(other.members);
    }

    @Override
    public String toString() {
        return "Rule [name=" + name + ", members=" + members + "]";
    }

    public String getName()
    {
        return name;
    }

    public List<Member> getMembers()
    {
        return members;
    }

    /* A Helper function to support the JNI layer. */
    Object[] getNativeMemberInfo() {
        Member[] currentMembers = members.toArray(new Member[members.size()]);
        int length = currentMembers.length;
        if (length == 0) {
            return null;
        }
        String[] names = new String[length];
        int[] types = new int[length];
        int[] actions = new int[length];
        Object[] result = new Object[] { names, types, actions };
        for (int i = 0; i < length; i++) {
            Member member = currentMembers[i];
            names[i] = member.getName();
            types[i] = member.getType().ordinal();
            actions[i] = member.getActionMask();
        }
        return result;
    }
}
