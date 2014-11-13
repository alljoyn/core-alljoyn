/*
 * Copyright (c) 2009-2011, 2014, AllSeen Alliance. All rights reserved.
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

package org.alljoyn.bus;

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;
import org.alljoyn.bus.annotation.BusSignal;
import org.alljoyn.bus.annotation.BusProperty;

@BusInterface
public interface AnnotationInterface {

    @BusMethod(name="DeprecatedMethod", annotation=BusMethod.ANNOTATE_DEPRECATED)
    public String deprecatedMethod(String str) throws BusException;

    @BusMethod(name="NoReplyMethod", annotation=BusMethod.ANNOTATE_NO_REPLY)
    public void noReplyMethod(String str) throws BusException;

    @BusMethod(name="DeprecatedNoReplyMethod", annotation=BusMethod.ANNOTATE_DEPRECATED | BusMethod.ANNOTATE_NO_REPLY)
    public void deprecatedNoReplyMethod(String str) throws BusException;

    @BusSignal(name="DeprecatedSignal", annotation=BusSignal.ANNOTATE_DEPRECATED)
    public void deprecatedSignal(String str) throws BusException;

    @BusProperty(annotation=BusProperty.ANNOTATE_EMIT_CHANGED_SIGNAL)
    public String getChangeNotifyProperty() throws BusException;

    @BusProperty(annotation=BusProperty.ANNOTATE_EMIT_CHANGED_SIGNAL)
    public void setChangeNotifyProperty(String str) throws BusException;
}

