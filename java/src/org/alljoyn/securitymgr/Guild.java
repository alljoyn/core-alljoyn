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

package org.alljoyn.securitymgr;

public final class Guild {
    private String name;
    private String description;
    private final GUID guid;

    Guild(String guildName, String guildDescription, byte[] guidData) {
        this.name = guildName;
        this.description = guildDescription;
        guid = new GUID(guidData);
    }

    public Guild(String name, String description, GUID guid) {
        this.name = name;
        this.description = description;
        this.guid = guid;
    }

    public String getName() {
        return name;
    }

    /**
     * @param name the name to set
     */
    public final void setName(String name) {
        this.name = name;
    }

    public String getDescription() {
        return description;
    }

    /**
     * @param description the description to set
     */
    public final void setDescription(String description) {
        this.description = description;
    }

    public GUID getGuid() {
        return guid;
    }

    @Override
    public int hashCode() {
        return 47 + guid.hashCode();
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof Guild) {
            Guild other = (Guild) obj;
            return guid.equals(other.guid);
        }
        return true;
    }

    @Override
    public String toString() {
        return "Guild [name=" + name + ", description=" + description
                + ", guid=" + guid + "]";
    }
}
