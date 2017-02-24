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
 * Method definition used to describe an interface method.
 * Annotations commonly used: DocString, NoReply, Deprecated.
 */
public class MethodDef extends BaseDef {

    final private String interfaceName;
    final private String signature;
    final private String replySignature;
    final private int timeout;

    final private List<ArgDef> argList;


    /**
     * Constructor. Timeout defaults to -1, indicating that an implementation dependant
     * default timeout will be used.
     *
     * @param name the name of the bus method.
     * @param signature input parameter signature.
     * @param replySignature output signature.
     * @param interfaceName the name of the bus interface.
     * @throws IllegalArgumentException one or more arguments is invalid.
     */
    public MethodDef(String name, String signature, String replySignature, String interfaceName) {
        this(name, signature, replySignature, interfaceName, -1);
    }

    /**
     * Constructor.
     *
     * @param name the name of the bus method.
     * @param signature input parameter signature.
     * @param replySignature output signature.
     * @param interfaceName the name of the bus interface.
     * @param timeout reply timeout in milliseconds; -1 indicates that an implementation dependant
     *                default timeout will be used.
     * @throws IllegalArgumentException one or more arguments is invalid.
     */
    public MethodDef(String name, String signature, String replySignature, String interfaceName, int timeout) {
        super(name);
        if (signature == null) {
            throw new IllegalArgumentException("Null signature");
        }
        if (replySignature == null) {
            throw new IllegalArgumentException("Null replySignature");
        }
        if (interfaceName == null) {
            throw new IllegalArgumentException("Null interfaceName");
        }
        if (timeout < -1) {
            throw new IllegalArgumentException("Invalid timeout");
        }
        this.interfaceName = interfaceName;
        this.signature = signature;
        this.replySignature = replySignature;
        this.argList = new ArrayList<ArgDef>();
        this.timeout = timeout;
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
     * @return the output signature.
     */
    public String getReplySignature() {
        return replySignature;
    }

    /**
     * Timeout specified in milliseconds to wait for a reply.
     * The default value is -1.
     * The value -1 means use the implementation dependent default timeout.
     *
     * @return reply timeout in milliseconds
     */
    public int getTimeout() {
        return timeout;
    }

    /**
     * @return the method's contained arg definitions.
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
     * @return whether the bus method has no reply (indicated via a NoReply annotation).
     */
    public boolean isNoReply() {
        String value = getAnnotation(ANNOTATION_METHOD_NOREPLY);
        return Boolean.parseBoolean(value);
    }

    /**
     * @return whether the bus method is deprecated (indicated via a Deprecated annotation).
     */
    public boolean isDeprecated() {
        String value = getAnnotation(ANNOTATION_DEPRECATED);
        return Boolean.parseBoolean(value);
    }

    @Override
    public String toString() {
        StringBuilder builder = new StringBuilder();
        builder.append("MethodDef {name=");
        builder.append(getName());
        builder.append(", signature=");
        builder.append(signature);
        builder.append(", replySignature=");
        builder.append(replySignature);
        builder.append(", interfaceName=");
        builder.append(interfaceName);
        builder.append("}");
        return builder.toString();
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        MethodDef methodDef = (MethodDef) o;

        if (!interfaceName.equals(methodDef.interfaceName)) return false;
        if (!getName().equals(methodDef.getName())) return false;
        if (!signature.equals(methodDef.signature)) return false;
        return replySignature.equals(methodDef.replySignature);

    }

    @Override
    public int hashCode() {
        int result = interfaceName.hashCode();
        result = 31 * result + getName().hashCode();
        result = 31 * result + signature.hashCode();
        result = 31 * result + replySignature.hashCode();
        return result;
    }

}