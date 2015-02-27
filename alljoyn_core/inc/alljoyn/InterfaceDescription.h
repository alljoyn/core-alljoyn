#ifndef _ALLJOYN_INTERFACEDESCRIPTION_H
#define _ALLJOYN_INTERFACEDESCRIPTION_H
/**
 * @file
 * This file defines types for statically describing a message bus interface
 */

/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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

#include <qcc/platform.h>
#include <qcc/String.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/Message.h>
#include <alljoyn/Status.h>
#include <alljoyn/Translator.h>

/// @cond ALLJOYN_DEV
/*!
   \def QCC_MODULE
   Internal usage
 */
#define QCC_MODULE "ALLJOYN"
/// @endcond

namespace ajn {

/** @name Property Access type */
// @{
static const uint8_t PROP_ACCESS_READ  = 1; /**< Read Access type */
static const uint8_t PROP_ACCESS_WRITE = 2; /**< Write Access type */
static const uint8_t PROP_ACCESS_RW    = 3; /**< Read-Write Access type */
// @}
/** @name Property Annotation flags */
// @{
static const uint8_t PROP_ANNOTATE_EMIT_CHANGED_SIGNAL             = 1; /**< EmitChangedSignal annotate flag. */
static const uint8_t PROP_ANNOTATE_EMIT_CHANGED_SIGNAL_INVALIDATES = 2; /**< EmitChangedSignal annotate flag (for notifying invalidation of property instead of value. */
// @}
/** @name Method/Signal Annotation flags */
// @{
static const uint8_t MEMBER_ANNOTATE_NO_REPLY   = 1; /**< No reply annotate flag */
static const uint8_t MEMBER_ANNOTATE_DEPRECATED = 2; /**< Deprecated annotate flag */
// @}

/**
 * The interface security policy can be inherit, required, or off. If security is
 * required on an interface, methods on that interface can only be called by an authenticated peer
 * and signals emitted from that interfaces can only be received by an authenticated peer. If
 * security is not specified for an interface the interface inherits the security of the objects
 * that implement it.  If security is not applicable to an interface authentication is never
 * required even when the implemented by a secure object. For example, security does not apply to
 * the Introspection interface otherwise secure objects would not be introspectable.
 */
typedef enum {
    AJ_IFC_SECURITY_INHERIT  = 0,   /**< Inherit the security of the object that implements the interface */
    AJ_IFC_SECURITY_REQUIRED = 1,   /**< Security is required for an interface */
    AJ_IFC_SECURITY_OFF      = 2    /**< Security does not apply to this interface */
} InterfaceSecurityPolicy;

/**
 * @class InterfaceDescription
 * Class for describing message bus interfaces. %InterfaceDescription objects describe the methods,
 * signals and properties of a BusObject or ProxyBusObject.
 *
 * Calling ProxyBusObject::AddInterface(const char*) adds the AllJoyn interface described by an
 * %InterfaceDescription to a ProxyBusObject instance. After an  %InterfaceDescription has been
 * added, the methods described in the interface can be called. Similarly calling
 * BusObject::AddInterface adds the interface and its methods, properties, and signal to a
 * BusObject. After an interface has been added method handlers for the methods described in the
 * interface can be added by calling BusObject::AddMethodHandler or BusObject::AddMethodHandlers.
 *
 * An %InterfaceDescription can be constructed piecemeal by calling InterfaceDescription::AddMethod,
 * InterfaceDescription::AddMember(), and InterfaceDescription::AddProperty(). Alternatively,
 * calling ProxyBusObject::ParseXml will create the %InterfaceDescription instances for that proxy
 * object directly from an XML string. Calling ProxyBusObject::IntrospectRemoteObject or
 * ProxyBusObject::IntrospectRemoteObjectAsync also creates the %InterfaceDescription
 * instances from XML but in this case the XML is obtained by making a remote Introspect method
 * call on a bus object.
 */

class InterfaceDescription {

    friend class BusAttachment;
    friend class XmlHelper;

    /** map containing description tables per argument name */
    class ArgumentDescriptions;

  public:

    class AnnotationsMap; /**< A map to store string annotations */

    /**
     * Structure representing the member to be added to the Interface
     */
    struct Member {
        const InterfaceDescription* iface;          /**< Interface that this member belongs to */
        AllJoynMessageType memberType;              /**< %Member type */
        qcc::String name;                           /**< %Member name */
        qcc::String signature;                      /**< Method call IN arguments (NULL for signals) */
        qcc::String returnSignature;                /**< Signal or method call OUT arguments */
        qcc::String argNames;                       /**< Comma separated list of argument names - can be NULL */
        AnnotationsMap* annotations;                /**< Map of annotations */
        qcc::String accessPerms;                    /**< Required permissions to invoke this call */
        qcc::String description;                    /**< Introspection description for this member */
        ArgumentDescriptions* argumentDescriptions; /**< Introspection descriptions for arguments to this member */
        bool isSessionlessSignal;                   /**< True if this is described as a sessionless signal */

        /**
         * %Member constructor.
         *
         * @param iface The interface this member is added to.
         * @param type The message type.
         * @param name The name of this member.
         * @param signature The signature of this member. May be NULL.
         * @param returnSignature The return signature of this member. May be NULL.
         * @param argNames The name of the arguments. May be NULL.
         * @param annotation Ignored.
         * @param accessPerms The permissions of this member.
         */
        Member(const InterfaceDescription* iface, AllJoynMessageType type, const char* name,
               const char* signature, const char* returnSignature, const char* argNames,
               uint8_t annotation, const char* accessPerms);

        /**
         * %Member copy constructor
         * @param other  The %Member being copied to this one.
         */
        Member(const Member& other);

        /**
         * %Member assignment operator
         * @param other  The %Member being copied to this one.
         *
         * @return
         * a reference to the %Member that was just copied
         */
        Member& operator=(const Member& other);

        /** %Member destructor */
        ~Member();

        /**
         * Get the names and values of all annotations
         *
         * @param[out] names    Annotation names
         * @param[out] values   Annotation values
         * @param      size     Number of annotations to get
         * @return              The number of annotations returned or the total number of annotations if props is NULL.
         */
        size_t GetAnnotations(qcc::String* names = NULL, qcc::String* values = NULL, size_t size = 0) const;

        /**
         * Get this member's annotation value
         *
         * @param        name   name of the annotation to look for
         * @param[out]   value  The value of the annotation, if found
         * @return    true iff annotations[name] == value
         */
        bool GetAnnotation(const qcc::String& name, qcc::String& value) const;

        /**
         * Equality. Two members are defined to be equal if their members are
         * equal except for iface which is ignored for equality.
         *
         * @param o   Member to compare against this member.
         *
         * @return    true iff o == this member.
         */
        bool operator==(const Member& o) const;
    };

    /**
     * Structure representing properties of the Interface
     */
    struct Property {
        qcc::String name;               /**< %Property name */
        qcc::String signature;          /**< %Property type */
        uint8_t access;                 /**< Access is #PROP_ACCESS_READ, #PROP_ACCESS_WRITE, or #PROP_ACCESS_RW */
        AnnotationsMap* annotations;    /**< Map of annotations */
        qcc::String description;        /**< Introspection description for this property */
        bool cacheable;                 /**< Is this property cacheable? */

        /** %Property constructor.
         * @param name      The name of the property.
         * @param signature The signature of the property. May be NULL.
         * @param access    The access type, may be #PROP_ACCESS_READ, #PROP_ACCESS_WRITE, or #PROP_ACCESS_RW.
         */
        Property(const char* name, const char* signature, uint8_t access);

        /** %Property constructor */
        Property(const char* name, const char* signature, uint8_t access, uint8_t annotation);

        /**
         * %Property copy constructor
         * @param other  The %Property being copied to this one.
         */
        Property(const Property& other);

        /**
         * %Property assignment operator
         * @param other  The %Property being copied to this one.
         * @return       A reference to the %Property that was just copied
         */
        Property& operator=(const Property& other);

        /** %Property destructor */
        ~Property();

        /**
         * Get the names and values of all annotations
         *
         * @param[out] names    Annotation names
         * @param[out] values   Annotation values
         * @param      size     Number of annotations to get
         * @return              The number of annotations returned or the total number of annotations if props is NULL.
         */
        size_t GetAnnotations(qcc::String* names = NULL, qcc::String* values = NULL, size_t size = 0) const;

        /**
         * Get this property's annotation value
         * @param name   name of the annotation to look for
         * @param[out]   value  The value of the annotation, if found
         * @return    true iff annotations[name] == value
         */
        bool GetAnnotation(const qcc::String& name, qcc::String& value) const;

        /** Equality */
        bool operator==(const Property& o) const;
    };

    /**
     * Add a member to the interface.
     *
     * @param type        Message type.
     * @param name        Name of member.
     * @param inputSig    Signature of input parameters or NULL for none.
     * @param outSig      Signature of output parameters or NULL for none.
     * @param argNames    Comma separated list of input and then output arg names used in annotation XML.
     * @param annotation  Annotation flags.
     * @param accessPerms Required permissions to invoke this call
     *
     * @return
     *      - #ER_OK if successful
     *      - #ER_BUS_MEMBER_ALREADY_EXISTS if member already exists
     */
    QStatus AddMember(AllJoynMessageType type, const char* name, const char* inputSig, const char* outSig, const char* argNames, uint8_t annotation = 0, const char* accessPerms = 0);

    /**
     * Lookup a member description by name
     *
     * @param name  Name of the member to lookup
     * @return
     *      - Pointer to member.
     *      - NULL if does not exist.
     */
    const Member* GetMember(const char* name) const;

    /**
     * Get all the members.
     *
     * @param members     A pointer to a Member array to receive the members. Can be NULL in
     *                    which case no members are returned and the return value gives the number
     *                    of members available.
     * @param numMembers  The size of the Member array. If this value is smaller than the total
     *                    number of members only numMembers will be returned.
     *
     * @return  The number of members returned or the total number of members if members is NULL.
     */
    size_t GetMembers(const Member** members = NULL, size_t numMembers = 0) const;

    /**
     * Check for existence of a member. Optionally check the signature also.
     * @remark
     * if the a signature is not provided this method will only check to see if
     * a member with the given @c name exists.  If a signature is provided a
     * member with the given @c name and @c signature must exist for this to return true.
     *
     * @param name       Name of the member to lookup
     * @param inSig      Input parameter signature of the member to lookup
     * @param outSig     Output parameter signature of the member to lookup (leave NULL for signals)
     * @return true if the member name exists.
     */
    bool HasMember(const char* name, const char* inSig = NULL, const char* outSig = NULL);

    /**
     * Add a method call member to the interface.
     *
     * @param name        Name of method call member.
     * @param inputSig    Signature of input parameters or NULL for none.
     * @param outSig      Signature of output parameters or NULL for none.
     * @param argNames    Comma separated list of input and then output arg names used in annotation XML.
     * @param annotation  Annotation flags.
     * @param accessPerms Access permission requirements on this call
     *
     * @return
     *      - #ER_OK if successful
     *      - #ER_BUS_MEMBER_ALREADY_EXISTS if member already exists
     */
    QStatus AddMethod(const char* name, const char* inputSig, const char* outSig, const char* argNames, uint8_t annotation = 0, const char* accessPerms = 0)
    {
        return AddMember(MESSAGE_METHOD_CALL, name, inputSig, outSig, argNames, annotation, accessPerms);
    }

    /**
     * Add an annotation to an existing member (signal or method).
     *
     * @param member     Name of member
     * @param name       Name of annotation
     * @param value      Value for the annotation
     *
     * @return
     *      - #ER_OK if successful
     *      - #ER_BUS_MEMBER_ALREADY_EXISTS if member already exists
     */
    QStatus AddMemberAnnotation(const char* member, const qcc::String& name, const qcc::String& value);

    /**
     * Get annotation to an existing member (signal or method).
     *
     * @param member     Name of member
     * @param name       Name of annotation
     * @param value      Output value for the annotation
     *
     * @return
     *      - true if found
     *      - false if property not found
     */
    bool GetMemberAnnotation(const char* member, const qcc::String& name, qcc::String& value) const;

    /**
     * Lookup a member method description by name
     *
     * @param name  Name of the method to lookup
     * @return
     *      - Pointer to member.
     *      - NULL if does not exist.
     */
    const Member* GetMethod(const char* name) const
    {
        const Member* method = GetMember(name);
        return (method && method->memberType == MESSAGE_METHOD_CALL) ? method : NULL;
    }

    /**
     * Add a signal member to the interface.
     *
     * @param name        Name of method call member.
     * @param sig         Signature of parameters or NULL for none.
     * @param argNames    Comma separated list of arg names used in annotation XML.
     * @param annotation  Annotation flags.
     * @param accessPerms Access permission requirements on this call
     *
     * @return
     *      - #ER_OK if successful
     *      - #ER_BUS_MEMBER_ALREADY_EXISTS if member already exists
     */
    QStatus AddSignal(const char* name, const char* sig, const char* argNames, uint8_t annotation = 0, const char* accessPerms = 0)
    {
        return AddMember(MESSAGE_SIGNAL, name, sig, NULL, argNames, annotation, accessPerms);
    }

    /**
     * Lookup a member signal description by name
     *
     * @param name  Name of the signal to lookup
     * @return
     *      - Pointer to member.
     *      - NULL if does not exist.
     */
    const Member* GetSignal(const char* name) const
    {
        const Member* method = GetMember(name);
        return (method && method->memberType == MESSAGE_SIGNAL) ? method : NULL;
    }

    /**
     * Lookup a property description by name
     *
     * @param name  Name of the property to lookup
     * @return a structure representing the properties of the interface
     */
    const Property* GetProperty(const char* name) const;

    /**
     * Get all the properties.
     *
     * @param props     A pointer to a Property array to receive the properties. Can be NULL in
     *                  which case no properties are returned and the return value gives the number
     *                  of properties available.
     * @param numProps  The size of the Property array. If this value is smaller than the total
     *                  number of properties only numProperties will be returned.
     *
     *
     * @return  The number of properties returned or the total number of properties if props is NULL.
     */
    size_t GetProperties(const Property** props = NULL, size_t numProps = 0) const;

    /**
     * Add a property to the interface.
     *
     * @param name       Name of property.
     * @param signature  Property type.
     * @param access     #PROP_ACCESS_READ, #PROP_ACCESS_WRITE or #PROP_ACCESS_RW
     * @return
     *      - #ER_OK if successful.
     *      - #ER_BUS_PROPERTY_ALREADY_EXISTS if the property can not be added
     *                                        because it already exists.
     */
    QStatus AddProperty(const char* name, const char* signature, uint8_t access);

    /**
     * Add an annotation to an existing property
     * @param p_name     Name of the property
     * @param name       Name of annotation
     * @param value      Value for the annotation
     * @return
     *      - #ER_OK if successful.
     *      - #ER_BUS_PROPERTY_ALREADY_EXISTS if the annotation can not be added to the property because it already exists.
     */
    QStatus AddPropertyAnnotation(const qcc::String& p_name, const qcc::String& name, const qcc::String& value);

    /**
     * Get the annotation value for a property
     * @param p_name     Name of the property
     * @param name       Name of annotation
     * @param value      Value for the annotation
     * @return           true if found, false if not found
     */
    bool GetPropertyAnnotation(const qcc::String& p_name, const qcc::String& name, qcc::String& value) const;

    /**
     * Check for existence of a property.
     *
     * @param name       Name of the property to lookup
     * @return true if the property exists.
     */
    bool HasProperty(const char* name) const { return GetProperty(name) != NULL; }

    /**
     * Check for existence of any properties
     *
     * @return  true if interface has any properties.
     */
    bool HasProperties() const { return GetProperties() != 0; }

    /**
     * Check for the existence of any cacheable properties.
     *
     * @return  true if the interface has any cacheable properties.
     */
    bool HasCacheableProperties() const;

    /**
     * Returns the name of the interface
     *
     * @return the interface name.
     */
    const char* GetName() const { return name.c_str(); }

    /**
     * Add an annotation to the interface.
     *
     * @param name       Name of annotation.
     * @param value      Value of the annotation
     * @return
     *      - #ER_OK if successful.
     *      - #ER_BUS_PROPERTY_ALREADY_EXISTS if the property can not be added
     *                                        because it already exists.
     */
    QStatus AddAnnotation(const qcc::String& name, const qcc::String& value);

    /**
     * Get the value of an annotation
     *
     * @param name       Name of annotation.
     * @param value      Returned value of the annotation
     * @return
     *      - true if annotation found.
     *      - false if annotation not found
     */
    bool GetAnnotation(const qcc::String& name, qcc::String& value) const;

    /**
     * Get the names and values of all annotations
     *
     * @param[out] names    Annotation names
     * @param[out] values    Annotation values
     * @param[out] size     Number of annotations
     * @return              The number of annotations returned or the total number of annotations if props is NULL.
     */
    size_t GetAnnotations(qcc::String* names = NULL, qcc::String* values = NULL, size_t size = 0) const;

    /**
     * Returns a description of the interface in introspection XML format
     * @return The interface description in introspection XML format.
     *
     * @param indent   Number of space chars to use in XML indentation.
     * @param languageTag Specifies the description language or NULL to omit descriptions
     * @param translator Translator instance to use for introspection descriptions
     * @return The XML introspection data.
     */
    qcc::String Introspect(size_t indent = 0, const char* languageTag = NULL, Translator* translator = NULL) const;

    /**
     * Activate this interface. An interface must be activated before it can be used. Activating an
     * interface locks the interface so that is can no longer be modified.
     */
    void Activate() { isActivated = true; }

    /**
     * Indicates if this interface is required to be secure. Secure interfaces require end-to-end
     * authentication.  The arguments for methods calls made to secure interfaces and signals
     * emitted by secure interfaces are encrypted.
     *
     * @return true if the interface is required to be secure.
     */
    bool IsSecure() const { return secPolicy == AJ_IFC_SECURITY_REQUIRED; }

    /**
     * Get the security policy that applies to this interface.
     *
     * @return Returns the security policy for this interface.
     */
    InterfaceSecurityPolicy GetSecurityPolicy() const { return secPolicy; }

    /**
     * Equality operation.
     */
    bool operator==(const InterfaceDescription& other) const;

    /**
     * Destructor
     */
    ~InterfaceDescription();

    /**
     * Copy constructor
     *
     * @param other  The InterfaceDescription being copied to this one.
     */
    InterfaceDescription(const InterfaceDescription& other);

    /**
     * Set the language tag for the introspection descriptions of this InterfaceDescription
     *
     * @param language The language tag
     */
    void SetDescriptionLanguage(const char* language);

    /**
     * Get the language tag for the instrospection descriptions of this InterfaceDescription
     *
     * @return The langauge tag
     */
    const char* GetDescriptionLanguage() const;

    /**
     * Set the introspection description for this InterfaceDescription
     *
     * Note that when SetDescriptionTranslator is used the text in this method may
     * actually be a "lookup key". When generating the introspection the "text" is first
     * passed to the Translator where the key should be used to lookup the actual
     * description. In such a case, the language tag should be set to "".
     *
     * @param description The introspection description
     */
    void SetDescription(const char* description);

    /**
     * Set the introspection description for "member" of this InterfaceDescription
     *
     * @param member The name of the member
     * @param description The introspection description
     * @param isSessionlessSignal True to document this member as a sessionless signal
     * @return
     *      - #ER_OK if successful.
     *      - #ER_BUS_INTERFACE_ACTIVATED If the interface has already been activated
     *      - #ER_BUS_INTERFACE_NO_SUCH_MEMBER If the member was not found
     */
    QStatus SetMemberDescription(const char* member,
                                 const char* description, bool isSessionlessSignal = false);

    /**
     * Set the introspection description for the argument "arg of "member" of this InterfaceDescription
     *
     * @param member The name of the member
     * @param arg The name of the argument
     * @param description The introspection description
     * @return
     *      - #ER_OK if successful.
     *      - #ER_BUS_INTERFACE_ACTIVATED If the interface has already been activated
     *      - #ER_BUS_INTERFACE_NO_SUCH_MEMBER If the member was not found
     */
    QStatus SetArgDescription(const char* member, const char* arg, const char* description);

    /**
     * Set the introspection description for "property" of this InterfaceDescription
     *
     * @param name The name of the property
     * @param description The introspection description
     * @return
     *      - #ER_OK if successful.
     *      - #ER_BUS_INTERFACE_ACTIVATED If the interface has already been activated
     *      - #ER_BUS_NO_SUCH_PROPERTY If the property was not found
     */
    QStatus SetPropertyDescription(const char* name, const char* description);

    /**
     * Set the Translator that provides this InterfaceDescription's
     * introspection description in multiple languages.
     *
     * @param translator The Translator instance.
     */
    void SetDescriptionTranslator(Translator* translator);

    /**
     * Get the Translator that provides this InterfaceDescription's
     * introspection description in multiple languages.
     *
     * @return The Translator instance.
     */
    Translator* GetDescriptionTranslator() const;

    /**
     * Does this interface have at least one description on an element
     *
     * @return True if this InterfaceDescription has at least one description
     */
    bool HasDescription() const;

  private:

    /**
     * Default constructor is private
     */
    InterfaceDescription() { }

    /**
     * Construct an interface with no methods or properties
     * This constructor cannot be used by any class other than the factory class (Bus).
     *
     * @param name      Fully qualified interface name.
     * @param secPolicy The security policy for this interface
     */
    InterfaceDescription(const char* name, InterfaceSecurityPolicy secPolicy);

    /**
     * Assignment operator
     *
     * @param other  The InterfaceDescription being copied to this one.
     *
     * @return  The assigned InterfaceDescription
     */
    InterfaceDescription& operator=(const InterfaceDescription& other);

    void AppendDescriptionXml(qcc::String& xml, const char* language,
                              const char* localDescription, Translator* translator, qcc::String const& indent) const;

    qcc::String NextArg(const char*& signature, qcc::String& argNames, bool inOut,
                        size_t indent, Member const& member, bool withDescriptions,
                        const char* langTag, Translator* translator) const;

    const char* Translate(const char* toLanguage, const char* text, qcc::String& buffer, Translator* translator) const;


    struct Definitions;
    Definitions* defs;   /**< The definitions for this interface */

    qcc::String name;    /**< Name of interface */
    bool isActivated;    /**< Set to true when interface is activated */
    InterfaceSecurityPolicy secPolicy; /**< The security policy for this interface */

};

}

#undef QCC_MODULE
#endif
