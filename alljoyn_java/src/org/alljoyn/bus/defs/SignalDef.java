/**
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *    
 *    SPDX-License-Identifier: Apache-2.0
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *    
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *    
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *    
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
*/

package org.alljoyn.bus.defs;

import java.util.ArrayList;
import java.util.List;

/**
 * Signal definition used to describe an interface signal.
 * Annotations commonly used: DocString, Deprecated, Sessionless, Sessioncast, Unicast, GlobalBroadcast.
 *
 * Note: The rule and source properties are mutually exclusive when registering a signal handler.
 * If the rule is non-empty, then the signal handler would be registered with the rule value
 * used as a matching filter for the signal. If the rule is empty, then the signal handler would
 * be registered with the source value used as a matching filter for the signal.
 */
public class SignalDef extends BaseDef {

    final private String interfaceName;
    final private String signature;

    final private List<ArgDef> argList;

    final private String rule;
    final private String source;

    /**
     * Constructor.
     *
     * @param name the name of the bus signal.
     * @param signature input parameter signature.
     * @param interfaceName the parent interface name.
     * @throws IllegalArgumentException one or more arguments is invalid.
     */
    public SignalDef(String name, String signature, String interfaceName) {
        this(name, signature, interfaceName, "", "");
    }

    /**
     * Constructor.
     *
     * @param name the name of the bus signal.
     * @param signature input parameter signature.
     * @param interfaceName the parent interface name.
     * @param rule a filter used to further match which signal handler is to be invoked.
     *             Example: "sender='org.freedesktop.DBus',path='/bar/foo',destination=':452345.34'"
     *             Rule can contain zero or more name='value' pairs. Rule is ignored if empty.
     * @param source a filter representing the source path of the emitter of the signal,
     *               used to further match which signal handler is to be invoked.
     *               Source is ignored if empty or if rule is specified (non-empty).
     * @throws IllegalArgumentException one or more arguments is invalid.
     */
    public SignalDef(String name, String signature, String interfaceName, String rule, String source) {
        super(name);
        if (signature == null) {
            throw new IllegalArgumentException("Null signature");
        }
        if (interfaceName == null) {
            throw new IllegalArgumentException("Null interfaceName");
        }
        if (rule == null) {
            throw new IllegalArgumentException("Null rule");
        }
        if (source == null) {
            throw new IllegalArgumentException("Null source");
        }
        this.interfaceName = interfaceName;
        this.signature = signature;
        this.argList = new ArrayList<ArgDef>();
        this.rule = rule;
        this.source = source;
    }

    /**
     * @return the name of the bus interface.
     */
    public String getInterfaceName() {
        return interfaceName;
    }

    /**
     * @return the input argument signature.
     */
    public String getSignature() {
        return signature;
    }

    /**
     * A signal has no return value (i.e. void return), which for an AllJoyn signature
     * is represented as an empty string. This method is provided as a convenience,
     * since it may not be obvious to clients that a void return is represented as
     * an empty string reply signature.
     *
     * @return the output signature, which is always an empty string.
     */
    public String getReplySignature() {
        return "";
    }

    /**
     * @return the filter rule used to further match the signal with a
     *         registered signal handler to be invoked. Ignored if not
     *         specified.
     */
    public String getRule() {
        return rule;
    }

    /**
     * @return the object path (of the emitter of the signal) used to further
     *         match the signal with a registered signal handler to be invoked.
     *         Ignored if not specified.
     */
    public String getSource() {
        return source;
    }

    /**
     * @return the signal's contained arg definitions.
     */
    public List<ArgDef> getArgList() {
        return argList;
    }

    public void setArgList(List<ArgDef> args) {
        argList.clear();
        argList.addAll(args);
    }

    /**
     * Returns the arg def that matches the given arg name.
     *
     * @param name the arg name.
     * @return the matching arg def. Null if not found.
     */
    public ArgDef getArg(String name) {
        for (ArgDef arg : argList) {
            if (arg.getName().equals(name)) {
                return arg;
            }
        }
        return null;  // not found
    }

    /**
     * Add the given arg to the end of the arg list.
     *
     * @param arg the argument def to add.
     */
    public void addArg(ArgDef arg) {
        argList.add(arg);
    }

    /**
     * @return whether the bus signal is deprecated (indicated via a Deprecated annotation).
     */
    public boolean isDeprecated() {
        String value = getAnnotation(ANNOTATION_DEPRECATED);
        return Boolean.parseBoolean(value);
    }

    /**
     * @return whether the bus signal's emission behavior is sessionless (indicated via a Sessionless annotation).
     */
    public boolean isSessionless() {
        String value = getAnnotation(ANNOTATION_SIGNAL_SESSIONLESS);
        return Boolean.parseBoolean(value);
    }

    /**
     * @return whether the bus signal's emission behavior is sessioncast (indicated via a Sessioncast annotation).
     */
    public boolean isSessioncast() {
        String value = getAnnotation(ANNOTATION_SIGNAL_SESSIONCAST);
        return Boolean.parseBoolean(value);
    }

    /**
     * @return whether the bus signal's emission behavior is unicast (indicated via a Unicast annotation).
     */
    public boolean isUnicast() {
        String value = getAnnotation(ANNOTATION_SIGNAL_UNICAST);
        return Boolean.parseBoolean(value);
    }

    /**
     * @return whether the bus signal's emission behavior is global broadcast (indicated via a GlobalBroadcast annotation).
     */
    public boolean isGlobalBroadcast() {
        String value = getAnnotation(ANNOTATION_SIGNAL_GLOBAL_BROADCAST);
        return Boolean.parseBoolean(value);
    }

    @Override
    public String toString() {
        StringBuilder builder = new StringBuilder();
        builder.append("SignalDef {name=");
        builder.append(getName());
        builder.append(", signature=");
        builder.append(signature);
        builder.append(", interfaceName=");
        builder.append(interfaceName);
        builder.append("}");
        return builder.toString();
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        SignalDef signalDef = (SignalDef) o;

        if (!interfaceName.equals(signalDef.interfaceName)) return false;
        if (!getName().equals(signalDef.getName())) return false;
        return signature.equals(signalDef.signature);

    }

    @Override
    public int hashCode() {
        int result = interfaceName.hashCode();
        result = 31 * result + getName().hashCode();
        result = 31 * result + signature.hashCode();
        return result;
    }

}