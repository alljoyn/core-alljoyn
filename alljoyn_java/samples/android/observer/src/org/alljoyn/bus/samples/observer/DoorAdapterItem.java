/*
 * Copyright AllSeen Alliance. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

package org.alljoyn.bus.samples.observer;

/**
 * A helper class for exchanging data between UI and Bus logic.
 */
public class DoorAdapterItem {

    private boolean isOpen;
    private final String name;

    public DoorAdapterItem(String name, boolean isOpen) {
        this.isOpen = isOpen;
        this.name = name;
    }

    public Boolean isOpen() {
        return isOpen;
    }

    public String getName() {
        return name;
    }

    public void setIsOpen(boolean value) {
        isOpen = value;
    }
}
