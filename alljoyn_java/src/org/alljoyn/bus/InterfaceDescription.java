/*
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

package org.alljoyn.bus;

import java.lang.reflect.Method;
import java.lang.reflect.Type;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.TreeMap;

import org.alljoyn.bus.annotation.AccessPermission;
import org.alljoyn.bus.annotation.BusAnnotation;
import org.alljoyn.bus.annotation.BusAnnotations;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;
import org.alljoyn.bus.annotation.BusProperty;
import org.alljoyn.bus.annotation.BusSignal;
import org.alljoyn.bus.annotation.Secure;

import org.alljoyn.bus.defs.BaseDef;
import org.alljoyn.bus.defs.InterfaceDef;
import org.alljoyn.bus.defs.MethodDef;
import org.alljoyn.bus.defs.PropertyDef;
import org.alljoyn.bus.defs.PropertyDef.ChangedSignalPolicy;
import org.alljoyn.bus.defs.SignalDef;

/**
 * InterfaceDescription represents a message bus interface.
 * This class is used internally by registered bus objects.
 */
public class InterfaceDescription {

    private static final int INVALID     = 0; /**< An invalid member type. */
    private static final int METHOD_CALL = 1; /**< A method call member. */
    private static final int SIGNAL      = 4; /**< A signal member. */

    private static final int READ        = 1;              /**< Read access type. */
    private static final int WRITE       = 2;              /**< Write access type. */
    private static final int RW          = READ | WRITE;   /**< Read-write access type. */

    private static final int AJ_IFC_SECURITY_INHERIT   = 0; /**< Inherit the security of the object that implements the interface */
    private static final int AJ_IFC_SECURITY_REQUIRED  = 1; /**< Security is required for an interface */
    private static final int AJ_IFC_SECURITY_OFF       = 2; /**< Security does not apply to this interface */

    private static final int MEMBER_ANNOTATE_NO_REPLY         = 1; /**< If used on method type member, don't expect a reply to the method call */
    private static final int MEMBER_ANNOTATE_DEPRECATED       = 2; /**< If used, indicates that the member has been deprecated */
    private static final int MEMBER_ANNOTATE_SESSIONCAST      = 4; /**< Signal sessioncast annotate flag */
    private static final int MEMBER_ANNOTATE_SESSIONLESS      = 8; /**< Signal sessionless annotate flag */
    private static final int MEMBER_ANNOTATE_UNICAST          = 16; /**< Signal unicast annotate flag */
    private static final int MEMBER_ANNOTATE_GLOBAL_BROADCAST = 32; /**< Signal global-broadcast annotate flag */

    private static final int PROP_ANNOTATE_EMIT_CHANGED_SIGNAL             = 1; /**< EmitsChangedSignal annotate flag. */
    private static final int PROP_ANNOTATE_EMIT_CHANGED_SIGNAL_INVALIDATES = 2; /**< EmitsChangedSignal annotate flag for notifying invalidation of property instead of value. */
    private static final int PROP_ANNOTATE_EMIT_CHANGED_SIGNAL_CONST       = 4; /**< EmitsChangedSignal annotate flag for const property. */


    private static Map<String, Translator> translatorCache = new HashMap<String, Translator>();
    private class Property {

        public String name;

        public TreeMap<String, String> annotations;

        public String signature;

        public Method get;

        public Method set;

        public Property(String name, String signature, TreeMap<String, String> annotations) {
            this.name = name;
            this.signature = signature;
            this.annotations = annotations;
        }
    }

    /**
     * The native interface description handle.
     *
     * This differs from most of the native bindings code in that the underlying
     * object points directly to the corresponding AllJoyn object without a JNI
     * intermediary.  The underlying AlljJoyn object is actually managed there,
     * so we have a pointer but have no responsibility for deleting it.  You
     * will see a create() method, but that method gets the pointer, it doesn't
     * allocate an object.
     */
    private long handle;

    /**
     * The dynamic bus object implementing this interface. This is needed when the interface implements
     * bus operations, and the dynamic bus object is required in order to initially retrieve the
     * interface's associated implementation Methods (which will then be cached in dynamicMembers).
     */
    private DynamicBusObject dynamicBusObject = null;

    /**
     * The dynamic members of this interface. A map of implementation Methods associated with
     * dynamic members of this interface (Key: method name, Value: implementation Method).
     */
    private Map<String, Method> dynamicMembers = null;

    /** The members of this interface (for use with static interface definitions). */
    private List<Method> members;

    /** The properties of this interface. */
    private Map<String, Property> properties;

    protected InterfaceDescription() {
        members = new ArrayList<Method>();
        properties = new HashMap<String, Property>();
    }

    /**
     * Constructor. Create this interface using a dynamic bus object, which has 
     * dynamic interface definitions instead of using static class annotations.
     *
     * @param dynamicBusObject the dynamic bus object. May be null when the interface does
     *                  not need to actually implement any bus operations. For example,
     *                  when an interface description is created by a ProxyBusObject
     *                  in order to send messages from a client, or a client creates an
     *                  interface description in order to register a sessionless signal
     *                  handler on a bus attachment. However, it is a coding error to
     *                  specify a null dynamicBusObject when the interface does need to
     *                  implement and locally execute its bus operations.
     */
    public InterfaceDescription(DynamicBusObject dynamicBusObject) {
        this.dynamicBusObject = dynamicBusObject;
        this.dynamicMembers = new HashMap<String,Method>();

        members = new ArrayList<Method>();
        properties = new HashMap<String, Property>();
    }

    private boolean announced;

    /** Allocate native resources. */
    private native Status create(BusAttachment busAttachment, String name,
            int securePolicy, int numProps, int numMembers);

    /** Add a member to the native interface description. */
    private native Status addMember(int type, String name, String inputSig, String outSig,
            int annotation,  String accessPerm);

    /** Add an annotation to the specified interface member */
    private native Status addMemberAnnotation(String member, String annotation, String value);

    /** Add a property to the native interface description. */
    private native Status addProperty(String name, String signature, int access, int annotation);

    /** Add an annotation to the specified interface property */
    private native Status addPropertyAnnotation(String property, String annotation, String value);

    /** Add an annotation to the interface */
    private native Status addAnnotation(String annotation, String value);

    @Deprecated private native void setDescriptionLanguage(String language);
    @Deprecated private native void setDescription(String Description);
    @Deprecated private native void setDescriptionTranslator(BusAttachment busAttachment, Translator dt);
    @Deprecated private native Status setMemberDescription(String member, String description, boolean isSessionlessSignal);
    @Deprecated private native Status setPropertyDescription(String propName, String description);

    public native String[] getDescriptionLanguages();
    public native String getDescriptionForLanguage(String languageTag);
    public native Status setDescriptionForLanguage(String description, String languageTag);
    public native String getMemberDescriptionForLanguage(String member, String languageTag);
    public native Status setMemberDescriptionForLanguage(String member, String description, String languageTag);
    public native String getPropertyDescriptionForLanguage(String property, String languageTag);
    public native Status setPropertyDescriptionForLanguage(String property, String description, String languageTag);
    public native String getArgDescriptionForLanguage(String member, String arg, String languageTag);
    public native Status setArgDescriptionForLanguage(String member, String arg, String description, String languageTag);

    /** Activate the interface on the bus. */
    private native void activate();

    /**
     * Called by the native code when registering bus objects to obtain the member
     * implementations.
     */
    private Method getMember(String name) {
        // Check if using dynamic member definitions
        if (dynamicMembers != null) {
            Method member = dynamicMembers.get(name);
            if (member != null ) {
                return member;
            }
        } else {
            // Otherwise, interface is using definitions based on static class Annotations
            for (Method m : members) {
                if (InterfaceDescription.getName(m).equals(name)) {
                    return m;
                }
            }
        }
        return null;
    }

    /**
     * Called by the native code when registering bus objects to obtain the property
     * implementations.
     */
    private Method[] getProperty(String name) {
        for (Property p : properties.values()) {
            if (p.name.equals(name)) {
                return new Method[] { p.get, p.set };
            }
        }
        return null;
    }

    /**
     * Create the native interface description for the busInterface.
     *
     * @param busAttachment the connection the interface is on
     * @param busInterface the interface
     */
    public Status create(BusAttachment busAttachment, Class<?> busInterface)
            throws AnnotationBusException {
        Status status = getProperties(busInterface);
        if (status != Status.OK) {
            return status;
        }
        status = getMembers(busInterface);
        if (status != Status.OK) {
            return status;
        }

        int securePolicy = AJ_IFC_SECURITY_INHERIT;
        Secure secureAnnotation = busInterface.getAnnotation(Secure.class);
        if (secureAnnotation != null) {
            if (secureAnnotation.value().equals("required")) {
                securePolicy = AJ_IFC_SECURITY_REQUIRED;
            } else if (secureAnnotation.value().equals("off")) {
                securePolicy = AJ_IFC_SECURITY_OFF;
            } else {
                /*
                 * In C++ if an interface provides an unknown security annotation
                 * it automatically defaults to the inherit for security. For
                 * that reason the Java code will do the same.
                 */
                securePolicy = AJ_IFC_SECURITY_INHERIT;
            }
        } else {
            securePolicy = AJ_IFC_SECURITY_INHERIT;
        }
        status = create(busAttachment, getName(busInterface), securePolicy, properties.size(),
                        members.size());
        if (status != Status.OK) {
            return status;
        }
        status = addProperties(busInterface);
        if (status != Status.OK) {
            return status;
        }
        status = addMembers(busInterface);
        if (status != Status.OK) {
            return status;
        }

        // now we need to add the DBus annotations for the interface;
        // this must be done *before* calling activate
        BusAnnotations busAnnotations = busInterface.getAnnotation(BusAnnotations.class);
        if (busAnnotations != null)
        {
            for (BusAnnotation annotation : busAnnotations.value()) {
                addAnnotation(annotation.name(), annotation.value());
            }
        }

        configureDescriptions(busAttachment, busInterface);

        BusInterface intf = busInterface.getAnnotation(BusInterface.class);
        if (intf != null) {
            if(intf.announced().equals("true")) {
                announced = true;
            } else {
                announced = false;
            }
        } else {
            announced = false;
        }

        activate();
        return Status.OK;
    }

    private void configureDescriptions(BusAttachment busAttachment, Class<?> busInterface) throws AnnotationBusException {
        BusInterface ifcNote = busInterface.getAnnotation(BusInterface.class);
        if(null == ifcNote) return;

        boolean hasDescriptions = false;

        if(!ifcNote.description().equals("")){
            setDescription(ifcNote.description());
            hasDescriptions = true;
        }

        for(Method method : busInterface.getMethods()) {
            String name = getName(method);

            BusMethod methodNote = method.getAnnotation(BusMethod.class);
            if(null != methodNote && (methodNote.description().length() > 0)){
                setMemberDescription(name, methodNote.description(), false);
                hasDescriptions = true;
            }

            BusSignal signalNote = method.getAnnotation(BusSignal.class);
            if(null != signalNote && (signalNote.description().length() > 0)){
                setMemberDescription(name, signalNote.description(), signalNote.sessionless());
                hasDescriptions = true;
            }

            BusProperty propNote = method.getAnnotation(BusProperty.class);
            if(null != propNote && (propNote.description().length() > 0)){
                setPropertyDescription(name, propNote.description());
                hasDescriptions = true;
            }
        }

        if(hasDescriptions) {
            setDescriptionLanguage(ifcNote.descriptionLanguage());
        }

        try{
            if(ifcNote.descriptionTranslator().length() > 0){
                //We store these so as not to create a separate instance each time it is used.
                //Although this means we'll be holding on to each instance forever this is probably
                //not a problem since most Translators will need to live forever anyway
                Translator dt = translatorCache.get(ifcNote.descriptionTranslator());
                if(null == dt) {
                    Class<?> c = Class.forName(ifcNote.descriptionTranslator());
                    dt = (Translator)c.newInstance();
                    translatorCache.put(ifcNote.descriptionTranslator(), dt);
                }
                setDescriptionTranslator(busAttachment, dt);
            }
        }catch(Exception e) {
            e.printStackTrace();
        }
    }

    private Status getProperties(Class<?> busInterface) throws AnnotationBusException {
        for (Method method : busInterface.getMethods()) {

            if (method.getAnnotation(BusProperty.class) != null) {
                String name = getName(method);
                Property property = properties.get(name);
                BusAnnotations propertyAnnotations = method.getAnnotation(BusAnnotations.class);
                TreeMap<String, String> annotations = new TreeMap<String, String>();
                if (propertyAnnotations != null)
                {
                    for (BusAnnotation propertyAnnotation : propertyAnnotations.value()) {
                        annotations.put(propertyAnnotation.name(), propertyAnnotation.value());
                    }
                }

                if (property == null) {
                    property = new Property(name, getPropertySig(method), annotations);
                } else if (!property.signature.equals(getPropertySig(method))) {
                    return Status.BAD_ANNOTATION;
                }

                if (method.getName().startsWith("get")) {
                    property.get = method;
                } else if (method.getName().startsWith("set")
                        && (method.getGenericReturnType().equals(void.class))) {
                    property.set = method;
                } else {
                    return Status.BAD_ANNOTATION;
                }
                properties.put(name, property);
            }
        }
        return Status.OK;
    }

    private Status addProperties(Class<?> busInterface) throws AnnotationBusException {
        for (Property property : properties.values()) {

            int access = ((property.get != null) ? READ : 0) | ((property.set != null) ? WRITE : 0);
            int annotation = 0;

            for (Method method : busInterface.getMethods()) {
                BusProperty p = method.getAnnotation(BusProperty.class);
                if (p != null) {
                    if (getName(method).equals(property.name)) {
                        annotation = p.annotation();
                    }
                }
            }
            Status status = addProperty(property.name, property.signature, access, annotation);
            if (status != Status.OK) {
                return status;
            }

            if (annotation == BusProperty.ANNOTATE_EMIT_CHANGED_SIGNAL) {
                property.annotations.put("org.freedesktop.DBus.Property.EmitsChangedSignal", "true");
            } else if (annotation == BusProperty.ANNOTATE_EMIT_CHANGED_SIGNAL_INVALIDATES) {
                property.annotations.put("org.freedesktop.DBus.Property.EmitsChangedSignal", "invalidates");
            }

            // loop through the map of properties and add them via native code
            for(Entry<String, String> entry : property.annotations.entrySet()) {
                addPropertyAnnotation(property.name, entry.getKey(), entry.getValue());
            }
        }
        return Status.OK;
    }

    private Status getMembers(Class<?> busInterface) throws AnnotationBusException {
        for (Method method : busInterface.getMethods()) {
            if (method.getAnnotation(BusMethod.class) != null) {
                members.add(method);
            } else if (method.getAnnotation(BusSignal.class) != null) {
                members.add(method);
            }
        }
        return Status.OK;
    }

    private Status addMembers(Class<?> busInterface) throws AnnotationBusException {
        for (Method member : members) {
            int type = INVALID;
            int annotation = 0;
            String accessPerm = null;
            BusMethod m = member.getAnnotation(BusMethod.class);
            BusSignal s = member.getAnnotation(BusSignal.class);
            AccessPermission ap = member.getAnnotation(AccessPermission.class);

            if (m != null) {
                type = METHOD_CALL;
                annotation = m.annotation();
            } else if (s != null) {
                type = SIGNAL;
                annotation = s.annotation();
            }
            if (type != INVALID) {
                if(ap != null) {
                    accessPerm = ap.value();
                }

                String memberName = getName(member);
                Status status = addMember(type, memberName, getInputSig(member),
                                          getOutSig(member), annotation, accessPerm);
                if (status != Status.OK) {
                    return status;
                }

                // pull out the DBus annotations
                BusAnnotations dbusAnnotations = member.getAnnotation(BusAnnotations.class);
                if (dbusAnnotations != null)
                {
                    for (BusAnnotation busAnnotation : dbusAnnotations.value()) {
                        addMemberAnnotation(memberName, busAnnotation.name(), busAnnotation.value());
                    }
                }
            }
        }
        return Status.OK;
    }

    /**
     * Create the native interface descriptions needed by
     * busInterfaces.  The Java interface descriptions are returned
     * in the descs list.
     * @param busAttachment The connection the interfaces are on.
     * @param busInterfaces The interfaces.
     * @param descs The returned interface descriptions.
     */
    public static Status create(BusAttachment busAttachment, Class<?>[] busInterfaces,
            List<InterfaceDescription> descs) throws AnnotationBusException {
        for (Class<?> intf : busInterfaces) {
            if ("org.freedesktop.DBus.Properties".equals(getName(intf))) {
                /* The Properties interface is handled automatically by the underlying library. */
                continue;
            }
            if (intf.getAnnotation(BusInterface.class) != null) {
                InterfaceDescription desc = new InterfaceDescription();

                Status status = desc.create(busAttachment, intf);
                if (status != Status.OK) {
                    return status;
                }
                descs.add(desc);
            }
        }
        return Status.OK;
    }

    /**
     * Get the DBus interface name.
     *
     * @param intf The interface.
     */
    public static String getName(Class<?> intf) {
        BusInterface busIntf = intf.getAnnotation(BusInterface.class);
        if (busIntf != null && busIntf.name().length() > 0) {
            return busIntf.name();
        } else {
            return intf.getName();
        }
    }

    /**
     * Is the interface announced
     */
    public boolean isAnnounced() {
        return announced;
    }

    /**
     * Get the DBus member or property name.
     *
     * @param method The method.
     */
    public static String getName(Method method) {
        BusMethod busMethod = method.getAnnotation(BusMethod.class);
        if (busMethod != null && busMethod.name().length() > 0) {
            return busMethod.name();
        }
        BusSignal busSignal = method.getAnnotation(BusSignal.class);
        if (busSignal != null && busSignal.name().length() > 0) {
            return busSignal.name();
        }
        BusProperty busProperty = method.getAnnotation(BusProperty.class);
        if (busProperty != null) {
            if (busProperty.name().length() > 0) {
                return busProperty.name();
            } else {
                /* The rest of the method name following the "get" or "set" prefix. */
                return method.getName().substring(3);
            }
        }
        return method.getName();
    }

    /**
     * Get the DBus member input signature.
     *
     * @param method The method.
     */
    public static String getInputSig(Method method) throws AnnotationBusException {
        BusMethod busMethod = method.getAnnotation(BusMethod.class);
        if (busMethod != null && busMethod.signature().length() > 0) {
            return Signature.typeSig(method.getGenericParameterTypes(), busMethod.signature());
        }
        BusSignal busSignal = method.getAnnotation(BusSignal.class);
        if (busSignal != null && busSignal.signature().length() > 0) {
            return Signature.typeSig(method.getGenericParameterTypes(), busSignal.signature());
        }
        return Signature.typeSig(method.getGenericParameterTypes(), null);
    }

    /**
     * Get the DBus member output signature.
     *
     * @param method the method
     */
    public static String getOutSig(Method method) throws AnnotationBusException {
        BusMethod busMethod = method.getAnnotation(BusMethod.class);
        if (busMethod != null && busMethod.replySignature().length() > 0) {
            return Signature.typeSig(method.getGenericReturnType(), busMethod.replySignature());
        }
        BusSignal busSignal = method.getAnnotation(BusSignal.class);
        if (busSignal != null && busSignal.replySignature().length() > 0) {
            return Signature.typeSig(method.getGenericReturnType(), busSignal.replySignature());
        }
        return Signature.typeSig(method.getGenericReturnType(), null);
    }

    /**
     * Get the DBus property signature.
     *
     * @param method the method
     */
    public static String getPropertySig(Method method) throws AnnotationBusException {
        Type type = null;
        if (method.getName().startsWith("get")) {
            type = method.getGenericReturnType();
        } else if (method.getName().startsWith("set")) {
            type = method.getGenericParameterTypes()[0];
        }
        BusProperty busProperty = method.getAnnotation(BusProperty.class);
        if (busProperty != null && busProperty.signature().length() > 0) {
            return Signature.typeSig(type, busProperty.signature());
        }
        return Signature.typeSig(type, null);
    }

    /**
     * Get the member timeout value.
     * The default value is -1.
     * The value -1 means use the implementation dependent default timeout.
     *
     * @param method The method.
     */
    public static int getTimeout(Method method) {
        BusMethod busMethod = method.getAnnotation(BusMethod.class);
        if (busMethod != null) {
            return busMethod.timeout();
        }
        BusProperty busProperty = method.getAnnotation(BusProperty.class);
        if (busProperty != null) {
            return busProperty.timeout();
        }
        return -1;
    }

    // *********************************************************************************

    /**
     * @return whether this object represents a dynamic interface.
     */
    public boolean isDynamicInterface() {
        return dynamicMembers != null;
    }

    /**
     * Create the native interface description for the given interface definition.
     *
     * @param busAttachment the connection the interface is on
     * @param interfaceDef a dynamic interface definition that the BusObject is expected to implement
     * @throws IllegalStateException Not a dynamic interface description
     */
    public Status create(BusAttachment busAttachment, InterfaceDef interfaceDef) {
        if (dynamicMembers == null) {
            throw new IllegalStateException("Not a dynamic interface description");
        }

        Status status = getProperties(interfaceDef);
        if (status != Status.OK) {
            return status;
        }

        status = getMembers(interfaceDef);
        if (status != Status.OK) {
            return status;
        }

        int securePolicy = interfaceDef.isSecureInherit() ? AJ_IFC_SECURITY_INHERIT :
                (interfaceDef.isSecureRequired() ? AJ_IFC_SECURITY_REQUIRED : AJ_IFC_SECURITY_OFF);

        status = create(busAttachment, interfaceDef.getName(), securePolicy, properties.size(),
                        dynamicMembers.size());
        if (status != Status.OK) {
            return status;
        }

        status = addProperties(interfaceDef);
        if (status != Status.OK) {
            return status;
        }
        status = addMembers(interfaceDef);
        if (status != Status.OK) {
            return status;
        }

        // now we need to add the DBus annotations for the interface;
        // this must be done *before* calling activate
        Map<String,String> busAnnotations = interfaceDef.getAnnotationList();
        for (Map.Entry<String,String> busAnnotation : busAnnotations.entrySet()) {
            addAnnotation(busAnnotation.getKey(), busAnnotation.getValue());
        }

        announced = interfaceDef.isAnnounced();
        activate();
        return Status.OK;
    }

    /** Caching property implementation handler Methods for the bus interface's property definitions. */
    private Status getProperties(InterfaceDef interfaceDef) {
        if (dynamicBusObject == null) {
            // No dynamic bus object. Skip retrieval of property implementation handler Methods.
            return Status.OK;
        }

        for (PropertyDef propertyDef : interfaceDef.getProperties()) {
            Property property = properties.get(propertyDef.getName());
            if (property == null) {
                TreeMap<String, String> annotations = new TreeMap<String, String>(propertyDef.getAnnotationList());
                property = new Property(propertyDef.getName(), propertyDef.getType(), annotations);
            } else if (!property.signature.equals(propertyDef.getType())) {
                return Status.BAD_ANNOTATION;
            }

            property.get = null;
            property.set = null;

            // Retrieve and cache the get/set property handlers
            Method[] propertyHandler = dynamicBusObject.getPropertyHandler(propertyDef.getInterfaceName(), propertyDef.getName());
            if (propertyHandler != null && propertyHandler.length == 2) {
                if (propertyDef.isReadAccess() && propertyHandler[0] != null) {
                    property.get = propertyHandler[0];
                } else if (propertyDef.isWriteAccess() && propertyHandler[1] != null) {
                    property.set = propertyHandler[1];
                } else if (propertyDef.isReadWriteAccess() &&
                           propertyHandler[0] != null && propertyHandler[1] != null) {
                    property.get = propertyHandler[0];
                    property.set = propertyHandler[1];
                } else {
                    return Status.BUS_CANNOT_ADD_HANDLER;
                }

                // Check that the setProperty Method is implemented as a void return
                if (property.set != null && !property.set.getGenericReturnType().equals(void.class)) {
                    return Status.BUS_CANNOT_ADD_HANDLER;
                }
            } else {
                // Could not retrieve property handler Method (no implementation Method matching this propertyDef)
                return Status.BUS_CANNOT_ADD_HANDLER;
            }
            properties.put(propertyDef.getName(), property);
        }
        return Status.OK;
    }

    private Status addProperties(InterfaceDef interfaceDef) {
        for (PropertyDef property : interfaceDef.getProperties()) {
            int access = ((property.isReadAccess() || property.isReadWriteAccess()) ? READ : 0)
                    | ((property.isWriteAccess() || property.isReadWriteAccess()) ? WRITE : 0);
            int annotation = 0;

            if (property.isEmitsChangedSignal()) {
                annotation |= PROP_ANNOTATE_EMIT_CHANGED_SIGNAL;
            } else if (property.isEmitsChangedSignalInvalidates()) {
                annotation |= PROP_ANNOTATE_EMIT_CHANGED_SIGNAL_INVALIDATES;
            }

            Status status = addProperty(property.getName(), property.getType(), access, annotation);
            if (status != Status.OK) {
                return status;
            }

            // loop through the map of property annotations and add them via native code
            for (Entry<String, String> entry : property.getAnnotationList().entrySet()) {
                addPropertyAnnotation(property.getName(), entry.getKey(), entry.getValue());
            }
        }
        return Status.OK;
    }

    /** Caching method/signal implementation handler Methods for the bus interface's method and signal definitions. */
    private Status getMembers(InterfaceDef interfaceDef) {
        if (dynamicBusObject == null) {
            // No dynamic bus object. Skip retrieval of method/signal implementation handler Methods.
            return Status.OK;
        }

        for (MethodDef methodDef : interfaceDef.getMethods()) {
            Method m = dynamicBusObject.getMethodHandler(methodDef.getInterfaceName(), methodDef.getName(), methodDef.getSignature());
            if (m != null) {
                dynamicMembers.put(methodDef.getName(), m);
            } else {
                // Could not register method handler Method (no implementation Method matching this methodDef)
                return Status.BUS_CANNOT_ADD_HANDLER;
            }
        }

        for (SignalDef signalDef : interfaceDef.getSignals()) {
            Method m = dynamicBusObject.getSignalHandler(signalDef.getInterfaceName(), signalDef.getName(), signalDef.getSignature());
            if (m != null) {
                dynamicMembers.put(signalDef.getName(), m);
            } else {
                // Could not register signal handler Method (no implementation Method matching this signalDef)
                return Status.BUS_CANNOT_ADD_HANDLER;
            }
        }

        return Status.OK;
    }

    private Status addMembers(InterfaceDef interfaceDef) {
        ArrayList<BaseDef> memberDefs = new ArrayList<BaseDef>();
        memberDefs.addAll(interfaceDef.getMethods());
        memberDefs.addAll(interfaceDef.getSignals());

        for (BaseDef def : memberDefs) {
            int type = INVALID;
            int annotation = 0;
            String accessPerm = null; //Android access permission string currently not supported (retrieve from dbusAnnotations?)
            String inSignature = "";
            String outSignature = "";
            Map<String,String> dbusAnnotations = null;

            if (def instanceof MethodDef) {
                MethodDef m = (MethodDef)def;
                type = METHOD_CALL;
                inSignature = m.getSignature();
                outSignature = m.getReplySignature();
                if (m.isNoReply()) annotation |= MEMBER_ANNOTATE_NO_REPLY;
                if (m.isDeprecated()) annotation |= MEMBER_ANNOTATE_DEPRECATED;
                dbusAnnotations = m.getAnnotationList();
            } else if (def instanceof SignalDef) {
                SignalDef s = (SignalDef)def;
                type = SIGNAL;
                inSignature = s.getSignature();
                if (s.isDeprecated()) annotation |= MEMBER_ANNOTATE_DEPRECATED;
                if (s.isSessionless()) annotation |= MEMBER_ANNOTATE_SESSIONLESS;
                if (s.isSessioncast()) annotation |= MEMBER_ANNOTATE_SESSIONCAST;
                if (s.isUnicast()) annotation |= MEMBER_ANNOTATE_UNICAST;
                if (s.isGlobalBroadcast()) annotation |= MEMBER_ANNOTATE_GLOBAL_BROADCAST;
                dbusAnnotations = s.getAnnotationList();
            }
            if (type != INVALID) {
                Status status = addMember(type, def.getName(), inSignature, outSignature, annotation, accessPerm);
                if (status != Status.OK) {
                    return status;
                }

                // pull out the DBus annotations
                if (dbusAnnotations != null) {
                    for (Map.Entry<String,String> busAnnotation : dbusAnnotations.entrySet()) {
                        addMemberAnnotation(def.getName(), busAnnotation.getKey(), busAnnotation.getValue());
                    }
                }
            }
        }
        return Status.OK;
    }

    /**
     * Create the native interface descriptions needed for the given DynamicBusObject's interface definitions.
     * The interface descriptions will also be initialized to reference the appropriate member implementation
     * Methods for given bus object. The created interface descriptions are returned in the descs list parameter.
     *
     * @param busAttachment The connection the interfaces are on.
     * @param dynamicBusObject The DynamicBusObject which implements a list of interfaces to be created.
     * @param descs The returned interface descriptions (populated by the create method itself).
     */
    public static Status create(BusAttachment busAttachment, DynamicBusObject dynamicBusObject, List<InterfaceDescription> descs) {
        if (busAttachment == null || dynamicBusObject == null || descs == null) {
            return Status.FAIL;
        }

        List<InterfaceDef> interfaceDefs = dynamicBusObject.getInterfaces();
        for (InterfaceDef intf : interfaceDefs) {
            if ("org.freedesktop.DBus.Properties".equals(intf.getName())) {
                // The Properties interface is handled automatically by the underlying library
                continue;
            }

            InterfaceDescription desc = new InterfaceDescription(dynamicBusObject);

            Status status = desc.create(busAttachment, intf);
            if (status == Status.BUS_IFACE_ALREADY_EXISTS) {
                continue;
            } else if (status != Status.OK) {
                return status;
            }
            descs.add(desc);
        }

        return Status.OK;
    }

}