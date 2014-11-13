/*
 * Copyright (c) 2009-2011,2014, AllSeen Alliance. All rights reserved.
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

/**
 * InterfaceDescription represents a message bus interface.
 * This class is used internally by registered bus objects.
 */
class InterfaceDescription {

    private static final int INVALID     = 0; /**< An invalid member type. */
    private static final int METHOD_CALL = 1; /**< A method call member. */
    private static final int SIGNAL      = 4; /**< A signal member. */

    private static final int READ        = 1;              /**< Read access type. */
    private static final int WRITE       = 2;              /**< Write access type. */
    private static final int RW          = READ | WRITE;   /**< Read-write access type. */

    private static final int AJ_IFC_SECURITY_INHERIT   = 0; /**< Inherit the security of the object that implements the interface */
    private static final int AJ_IFC_SECURITY_REQUIRED  = 1; /**< Security is required for an interface */
    private static final int AJ_IFC_SECURITY_OFF       = 2; /**< Security does not apply to this interface */

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

    /** The members of this interface. */
    private List<Method> members;

    /** The properties of this interface. */
    private Map<String, Property> properties;

    public InterfaceDescription() {
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

    private native void setDescriptionLanguage(String language);
    private native void setDescription(String Description);
    private native void setDescriptionTranslator(BusAttachment busAttachment, Translator dt);
    private native Status setMemberDescription(String member, String description, boolean isSessionlessSignal);
    private native Status setPropertyDescription(String propName, String description);

    /** Activate the interface on the bus. */
    private native void activate();

    /**
     * Called by the native code when registering bus objects to obtain the member
     * implementations.
     */
    private Method getMember(String name) {
        for (Method m : members) {
            if (InterfaceDescription.getName(m).equals(name)) {
                return m;
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
        // this must be done *before* calling create
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
}
