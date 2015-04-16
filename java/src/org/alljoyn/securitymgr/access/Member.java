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
import java.util.Arrays;
import java.util.List;

/**
 * Describes a Member part of an {@link Rule}. A member with name "*" is
 * considered a wildcard. It represents all members of the interface.
 */
public class Member {
    private final String name;
    private final Action[] actions;
    private final MemberType type;
    private final boolean mutualAuth;

    /* Helper constructor for the JNI layer */
    Member(String memberName, int actionMask, int memberType) {
        name = memberName;
        List<Action> list = new ArrayList<Action>();
        Action[] allActions = Action.values();
        for (Action action : allActions) {
            if ((action.getBitmask() & actionMask) > 0) {
                list.add(action);
            }
        }
        actions = list.toArray(new Action[list.size()]);
        type = MemberType.values()[memberType];
        mutualAuth = true;
    }

    /**
     * Constructs a wildcard member granting full access.
     */
    public Member() {
        name = "*";
        actions = Action.fullAccess();
        type = MemberType.ANY;
        mutualAuth = true;
    }

    /**
     * Constructs a member.
     *
     * @param name the name of the member (can be the wildcard).
     * @param actions the actions allowed on this member.
     * @param type the type of the member.
     */
    public Member(String name, Action[] actions, MemberType type) {
        this.name = name;
        this.actions = actions;
        this.type = type;
        mutualAuth = true;
    }

    /**
     * The name of this member
     *
     * @return the name.
     */
    public String getName() {
        return name;
    }

    /**
     * Get the Actions on this member.
     *
     * @return a copy of the actions.
     */
    public Action[] getActions() {
        return actions.clone();
    }

    /**
     * The type of the member.
     *
     * @return the member type.
     */
    public MemberType getType() {
        return type;
    }

    /**
     * Check if mutual authentication is required for this Member.
     *
     * @return true if mutual authentication is required.
     */
    public boolean isMutualAuth() {
        return mutualAuth;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = prime + Arrays.hashCode(actions);
        result = prime * result + (mutualAuth ? 1231 : 1237);
        result = prime * result + ((name == null) ? 0 : name.hashCode());
        result = prime * result + ((type == null) ? 0 : type.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        Member other = (Member) obj;
        if (!Arrays.equals(actions, other.actions))
            return false;
        if (mutualAuth != other.mutualAuth)
            return false;
        if (name == null) {
            if (other.name != null)
                return false;
        } else if (!name.equals(other.name))
            return false;
        if (type != other.type)
            return false;
        return true;
    }

    @Override
    public String toString() {
        return "Member [name=" + name + ", actions=" + Arrays.toString(actions)
                + ", type=" + type + ", mutualAuth=" + mutualAuth + "]";
    }

    /* Helper function to support the JNI implementation */
    int getActionMask() {
        int mask = 0;
        for (Action action : actions) {
            mask |= action.getBitmask();
        }
        return mask;
    }
}
