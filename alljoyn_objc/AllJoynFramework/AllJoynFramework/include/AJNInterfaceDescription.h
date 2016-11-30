////////////////////////////////////////////////////////////////////////////////
// Copyright AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import "AJNInterfaceMember.h"
#import "AJNInterfaceProperty.h"
#import "AJNObject.h"
#import "AJNTranslator.h"

@class AJNBusAttachment;

/**
 * Class for describing message bus interfaces. AJNInterfaceDescription objects describe the methods,
 * signals and properties of a AJNBusObject or AJNProxyBusObject.
 *
 * Calling AJNProxyBusObject::addInterface adds the AllJoyn interface described by an
 * AJNInterfaceDescription to a ProxyBusObject instance. After an  AJNInterfaceDescription has been
 * added, the methods described in the interface can be called. Similarly calling
 * AJNBusObject::addInterface adds the interface and its methods, properties, and signal to a
 * BusObject. After an interface has been added method handlers for the methods described in the
 * interface can be added by calling BusObject::AddMethodHandler or BusObject::AddMethodHandlers.
 *
 * An InterfaceDescription can be constructed piecemeal by calling InterfaceDescription::addMethod,
 * InterfaceDescription::addSignal, and InterfaceDescription::addProperty. Alternatively,
 * calling ProxyBusObject::ParseXml will create the %InterfaceDescription instances for that proxy
 * object directly from an XML string. Calling ProxyBusObject::IntrospectRemoteObject or
 * ProxyBusObject::IntrospectRemoteObjectAsync also creates the %InterfaceDescription
 * instances from XML but in this case the XML is obtained by making a remote Introspect method
 * call on a bus object.
 */
@interface AJNInterfaceDescription : AJNObject

/** Name of interface */
@property (readonly, nonatomic) NSString *name;

/** The members of the interface */
@property (readonly, nonatomic) NSArray *members;

/** The properties of the interface */
@property (readonly, nonatomic) NSArray *properties;

/** An XML description of the interface */
@property (readonly, nonatomic) NSString *xmlDescription;

/** Get the language tag for the introspection descriptions of this InterfaceDescription */
@property (readonly, nonatomic) NSString *language DEPRECATED_ATTRIBUTE;

/** Get the set of all available description languages.
 *
 * The set contains the sum of the language tags for the interface description, interface property, interface member and member argument descriptions.
*/
@property (readonly, nonatomic) NSSet *languages;

/** Get the Translator that provies this InterfaceDescription's introspection descprition in multiple lanauges */
@property (readonly, nonatomic) id<AJNTranslator> translator;

/**
 * Indicates if this interface is secure. Secure interfaces require end-to-end authentication.
 * The arguments for methods calls made to secure interfaces and signals emitted by secure
 * interfaces are encrypted.
 * @return true if the interface is secure.
 */
@property (readonly, nonatomic) BOOL isSecure;

/**
 * Check for existence of any properties
 *
 * @return  true if interface has any properties.
 */
@property (readonly, nonatomic) BOOL hasProperties;

/**
 * Check for existence of any cacheable properties.
 */
@property (readonly, nonatomic) BOOL hasCacheableProperties;

/** Does this interface have at least one description on an element. */
@property (readonly, nonatomic) BOOL hasDescription;

@property (nonatomic) AJNBusAttachment *bus;

/**
 * The interface security policy can be inherit, required, or off. If security is
 * required on an interface, methods on that interface can only be called by an authenticated peer
 * and signals emitted from that interfaces can only be received by an authenticated peer. If
 * security is not specified for an interface the interface inherits the security of the objects
 * that implement it.  If security is not applicable to an interface authentication is never
 * required even when the implemented by a secure object. For example, security does not apply to
 * the Introspection interface otherwise secure objects would not be introspectable.
 */
typedef enum AJNInterfaceSecurityPolicy{
    AJN_IFC_SECURITY_INHERIT=0,       /**< Inherit the security of the object that implements the interface */
    AJN_IFC_SECURITY_REQUIRED=1,      /**< Security is required for an interface */
    AJN_IFC_SECURITY_OFF=2            /**< Security does not apply to this interface */
}AJNInterfaceSecurityPolicy;

/**
 * Get the security policy that applies to this interface.
 *
 * @return Returns the security policy for this interface.
 */
@property (readonly, nonatomic) AJNInterfaceSecurityPolicy securityPolicy;

- (id)initWithHandle:(AJNHandle)handle;
- (id)initWithHandle:(AJNHandle)handle shouldDeleteHandleOnDealloc:(BOOL)deletionFlag;

/**
 * Add a member to the interface.
 *
 * @param type        Message type.
 * @param name        Name of member.
 * @param inputSig    Signature of input parameters or NULL for none.
 * @param outSig      Signature of output parameters or NULL for none.
 * @param argNames    Comma separated list of input and then output arg names used in annotation XML.
 *
 * @return
 *      - ER_OK if successful
 *      - ER_BUS_MEMBER_ALREADY_EXISTS if member already exists
 */
- (QStatus)addMember:(AJNMessageType)type name:(NSString*)name inputSig:(NSString*)inputSig outSig:(NSString*)outSig argNames:(NSString*)argNames;

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
 *      - ER_OK if successful
 *      - ER_BUS_MEMBER_ALREADY_EXISTS if member already exists
 */
- (QStatus)addMember:(AJNMessageType)type name:(NSString*)name inputSig:(NSString*)inputSig outSig:(NSString*)outSig argNames:(NSString*)argNames annotation:(uint8_t)annotation accessPerms:(NSString*)accessPerms;



/**
 * Add a method call member to the interface.
 *
 * @param methodName        Name of method call member.
 * @param inputSignature    Signature of input parameters or NULL for none.
 * @param outputSignature   Signature of output parameters or NULL for none.
 * @param arguments         Comma separated list of input and then output arg names used in annotation XML.
 * @param annotation        Annotation flags.
 * @param accessPermissions Access permission requirements on this call
 *
 * @return  - ER_OK if successful
 *          - ER_BUS_MEMBER_ALREADY_EXISTS if member already exists
 */
- (QStatus)addMethodWithName:(NSString *)methodName inputSignature:(NSString *)inputSignature outputSignature:(NSString *)outputSignature argumentNames:(NSArray *)arguments annotation:(AJNInterfaceAnnotationFlags) annotation accessPermissions:(NSString *)accessPermissions;

/**
 * Add a method call member to the interface.
 *
 * @param methodName        Name of method call member.
 * @param inputSignature    Signature of input parameters or NULL for none.
 * @param outputSignature   Signature of output parameters or NULL for none.
 * @param arguments         Comma separated list of input and then output arg names used in annotation XML.
 * @param annotation        Annotation flags.
 *
 * @return  - ER_OK if successful
 *          - ER_BUS_MEMBER_ALREADY_EXISTS if member already exists
 */
- (QStatus)addMethodWithName:(NSString *)methodName inputSignature:(NSString *)inputSignature outputSignature:(NSString *)outputSignature argumentNames:(NSArray *)arguments annotation:(AJNInterfaceAnnotationFlags)annotation;

/**
 * Add a method call member to the interface.
 *
 * @param methodName        Name of method call member.
 * @param inputSignature    Signature of input parameters or NULL for none.
 * @param outputSignature   Signature of output parameters or NULL for none.
 * @param arguments         Comma separated list of input and then output arg names used in annotation XML.
 *
 * @return  - ER_OK if successful
 *          - ER_BUS_MEMBER_ALREADY_EXISTS if member already exists
 */
- (QStatus)addMethodWithName:(NSString *)methodName inputSignature:(NSString *)inputSignature outputSignature:(NSString *)outputSignature argumentNames:(NSArray *)arguments;

/**
 * Lookup a member method description by name
 *
 * @param methodName  Name of the method to lookup
 * @return  - Pointer to member.
 *          - NULL if does not exist.
 */
- (AJNInterfaceMember *)methodWithName:(NSString *)methodName;

/**
 * Add a signal member to the interface. (Deprecated)
 *
 * @param name              Name of method call member.
 *
 * @return  - ER_OK if successful
 *          - ER_BUS_MEMBER_ALREADY_EXISTS if member already exists
 */
- (QStatus)addSignalWithName:(NSString *)name;

/**
 * Add a signal member to the interface. (Deprecated)
 *
 * @param name              Name of method call member.
 * @param inputSignature    Signature of parameters or NULL for none.
 * @param arguments         Comma separated list of arg names used in annotation XML.
 *
 * @return  - ER_OK if successful
 *          - ER_BUS_MEMBER_ALREADY_EXISTS if member already exists
 */
- (QStatus)addSignalWithName:(NSString *)name inputSignature:(NSString *)inputSignature argumentNames:(NSArray *)arguments;

/**
 * Add a signal member to the interface.
 *
 * @param name              Name of method call member.
 * @param inputSignature    Signature of parameters or NULL for none.
 * @param arguments         Comma separated list of arg names used in annotation XML.
 * @param annotation        Annotation flags.
 * @param permissions       Access permission requirements on this call
 *
 * @return  - ER_OK if successful
 *          - ER_BUS_MEMBER_ALREADY_EXISTS if member already exists
 */
- (QStatus)addSignalWithName:(NSString *)name inputSignature:(NSString *)inputSignature argumentNames:(NSArray *)arguments annotation:(uint8_t) annotation accessPermissions:(NSString *)permissions;

/**
 * Lookup a member signal description by name
 *
 * @param signalName  Name of the signal to lookup
 * @return  - Pointer to member.
 *          - nil if does not exist.
 */
- (AJNInterfaceMember *)signalWithName:(NSString *)signalName;

/**
 * Add a property to the interface.
 *
 * @param name          Name of property.
 * @param signature     Property type.
 * @param permissions   Access permission - Read Only, Read/Write, or Write Only
 * @return  - ER_OK if successful.
 *          - ER_BUS_PROPERTY_ALREADY_EXISTS if the property can not be added because it already exists.
 */
- (QStatus)addPropertyWithName:(NSString *)name signature:(NSString *)signature accessPermissions:(AJNInterfacePropertyAccessPermissionsFlags)permissions;

/**
 * Lookup a property description by name.
 *
 * @param propertyName       Name of the property to lookup
 * @return An object representing the property if the property exists, otherwise nil.
 */
- (AJNInterfaceProperty *)propertyWithName:(NSString *)propertyName;

/**
 * Check for existence of a property.
 *
 * @param propertyName       Name of the property to lookup
 * @return true if the property exists.
 */
- (BOOL)hasPropertyWithName:(NSString*)propertyName;

/**
 * Lookup a member description by name
 *
 * @param name  Name of the member to lookup
 * @return  - Pointer to member.
 *          - nil if does not exist.
 */
- (AJNInterfaceMember *)memberWithName:(NSString *)name;

/**
 * Get the value of an annotation on the interface
 *
 * @param annotationName       Name of annotation.
 * @return  - string value of the annotation if annotation found.
 *          - nil if annotation not found
 */
- (NSString *)annotationWithName:(NSString *)annotationName;

/**
 * Add an annotation to the interface.
 *
 * @param annotationName       Name of annotation.
 * @param annotationValue      Value of the annotation
 * @return  - ER_OK if successful.
 *          - ER_BUS_PROPERTY_ALREADY_EXISTS if the annotation can not be added
 *                                        because it already exists.
 */
- (QStatus)addAnnotationWithName:(NSString *)annotationName value:(NSString *)annotationValue;

/**
 * Get annotation to an existing member (signal or method).
 *
 * @param annotationName    Name of annotation
 * @param memberName        Name of member
 *
 * @return  - string value of annotation if found
 *          - nil if annotation not found
 */
- (NSString *)memberAnnotationWithName:(NSString *)annotationName forMemberWithName:(NSString *)memberName;

/**
 * Add an annotation to an exisiting member (signal or method).
 *
 * @param annotationName    Name of annotation
 * @param annotationValue   Value of annotation
 * @param memberName        Name of member
 *
 * @return  - ER_OK if successful
 *          - ER_BUS_MEMBER_ALREADY_EXISTS if annotation already exists
 */
- (QStatus)addMemberAnnotationWithName:(NSString *)annotationName value:(NSString *)annotationValue forMemberWithName:(NSString *)memberName;

/**
 * Get the annotation value for a property.
 *
 * @param annotationName    Name of annotation
 * @param propertyName      Name of property
 *
 * @return  - string value of annotation if found
 *          - nil if annotation not found
 */
- (NSString *)annotationWithName:(NSString *)annotationName forPropertyWithName:(NSString *)propertyName;

/**
 * Add annotation to an existing property.
 *
 * @param annotationName    Name of annotation
 * @param annotationValue   Value of annotation
 * @param propertyName      Name of property
 *
 * @return  - ER_OK if successful
 *          - ER_BUS_MEMBER_ALREADY_EXISTS if annotation already exists
 */
- (QStatus)addPropertyAnnotationWithName:(NSString *)annotationName value:(NSString *)annotationValue forPropertyWithName:(NSString *)propertyName;

/**
 * Set the language tag for the introspection descriptions of this InterfaceDescription.
 *
 * @param language the language of this Interface's descriptions
 */
- (void)setDescriptionLanguage:(NSString *)language DEPRECATED_ATTRIBUTE;

/**
 * Set the introspection description for this InterfaceDescription.
 *
 * @param description This Interface's description
 */
- (void)setDescription:(NSString *)description DEPRECATED_ATTRIBUTE;

/**
 * Set the introspection description for this InterfaceDescription in the given language.
 *
 * The description that was set can be retrieved by calling GetDescriptionForLanguage() OR GetAnnotation()
 * for an "org.alljoyn.Bus.DocString" annotation with the desired language tag
 * (e.g., "org.alljoyn.Bus.DocString.en").
 * For example, a description set by calling
 * SetDescriptionForLanguage("This is the interface", "en") can be retrieved by calling:
 *      - GetDescriptionForLanguage(description, "en") OR
 *      - GetAnnotation("org.alljoyn.Bus.DocString.en", description).
 *
 * @param[in] description The introspection description.
 * @param[in] languageTag The language of the description (language tag as defined in RFC 5646, e.g., "en-US").
 * @return
 *      - ER_OK if successful.
 *      - ER_BUS_INTERFACE_ACTIVATED If the interface has already been activated.
 *      - ER_BUS_DESCRIPTION_ALREADY_EXISTS If the interface already has a description.
 */
- (QStatus)setDescriptionForLanguage:(NSString*)description forLanguage:(NSString*)languageTag;

/**
 * Get the introspection description for this InterfaceDescription in the given language.
 *
 * To obtain the description, the method searches for the best match of the given language tag
 * using the lookup algorithm in RFC 4647 section 3.4.
 * For example, if descriptionForLanguage("en-US") is called, the method will:
 *       - Search for a description with the same language tag ("en-US"), return the description
 *       if such a description is found; else:
 *       - Search for a description with a less specific language tag ("en"), return the description
 *       if such a description is found; else:
 *       - Return nil.
 *
 * The method will also provide descriptions which have been set as description annotations
 * (set by calling AddAnnotation() with the annotation name set to "org.alljoyn.Bus.DocString"
 * plus the desired language tag, e.g., "org.alljoyn.Bus.DocString.en").
 *
 * @param languageTag The language of the description (language tag as defined in RFC 5646, e.g., "en-US").
 * @return
 *      - description for the given language
 *      - nil otherwise.
 */
- (NSString*)descriptionForLanguage:(NSString*)languageTag;

/**
 * Set the introspection description for "member" of this InterfaceDescription.
 *
 * @param description The description of the method or signal
 * @param member The name of the method or signal
 *
 * @return  - ER_OK if successful
 *          - ER_BUS_INTERFACE_NO_SUCH_MEMBER if the method or signal does not exist
 *          - ER_BUS_INTERFACE_ACTIVATED if this interface has already activated
 */
- (QStatus)setMemberDescription:(NSString *)description forMemberWithName:(NSString *)member DEPRECATED_ATTRIBUTE;

/**
 * Set the introspection description for "member" of this InterfaceDescription.
 *
 * @param description The description of the method or signal
 * @param member The name of the method or signal
 * @param sessionless Set this to TRUE if this is a signal you intend on sending sessionless
 *
 * @return  - ER_OK if successful
 *          - ER_BUS_INTERFACE_NO_SUCH_MEMBER if the method or signal does not exist
 *          - ER_BUS_INTERFACE_ACTIVATED if this interface has already activated
 */
- (QStatus)setMemberDescription:(NSString *)description forMemberWithName:(NSString *)member sessionlessSignal:(BOOL)sessionless DEPRECATED_ATTRIBUTE;

/**
 * Set the introspection description for member "memberName" of this InterfaceDescription
 * in the given language.
 *
 * The set description can be retrieved by calling GetMemberDescriptionForLanguage() OR GetMemberAnnotation()
 * for an "org.alljoyn.Bus.DocString" annotation with the desired language tag
 * (e.g., "org.alljoyn.Bus.DocString.en").
 * For example, a description set by calling
 * SetMemberDescriptionForLanguage("MethodName", "This is the method", "en") can be retrieved by calling:
 *      - GetMemberDescriptionForLanguage("MethodName", description, "en") OR
 *      - GetMemberAnnotation("MethodName", "org.alljoyn.Bus.DocString.en", description).
 *
 * @param[in] member The name of the member.
 * @param[in] description The introspection description.
 * @param[in] languageTag The language of the description (language tag as defined in RFC 5646, e.g., "en-US").
 * @return
 *      - ER_OK if successful.
 *      - ER_BUS_INTERFACE_ACTIVATED If the interface has already been activated.
 *      - ER_BUS_DESCRIPTION_ALREADY_EXISTS If the interface member already has a description.
 */
 - (QStatus)setMemberDescriptionForLanguage:(NSString *)member withDescription:(NSString *)description forLanguage:(NSString *)languageTag;

 /**
  * Get the introspection description for the member "member" of this InterfaceDescription
  * in the given language.
  *
  * To obtain the description, the method searches for the best match of the given language tag
  * using the lookup algorithm in RFC 4647 section 3.4.
  * For example, if GetMemberDescriptionForLanguage("MethodName", "en-US") is called, the method will:
  *       - Search for a description with the same language tag ("en-US"), return the description
  *       if such a description is found; else:
  *       - Search for a description with a less specific language tag ("en"), return the description
  *       if such a description is found; else:
  *       - Return nil.
  *
  * The method will also provide descriptions which have been set as description annotations
  * (set by calling AddMemberAnnotation() with the annotation name set to "org.alljoyn.Bus.DocString"
  * plus the desired language tag, e.g., "org.alljoyn.Bus.DocString.en").
  *
  * @param[in] memberName The name of the member.
  * @param[in] languageTag The language of the description (language tag as defined in RFC 5646, e.g., "en-US").
  * @return
  *      - description for given language
  *      - nil otherwise.
  */
- (NSString*)memberDescriptionForLanguage:(NSString*)memberName forLanguage:(NSString*)languageTag;

/**
 * Set the introspection description for "property" of this InterfaceDescription.
 *
 * @param description The description of the property
 * @param propName The name of the property
 *
 * @return  - ER_OK if successful
 *          - ER_BUS_NO_SUCH_PROPERTY if the property does not exist
 *          - ER_BUS_INTERFACE_ACTIVATED if this interface has already activated
 */
- (QStatus)setPropertyDescription:(NSString *)description forPropertyWithName:(NSString *)propName DEPRECATED_ATTRIBUTE;

/**
 * Set the introspection description for the interface property "propertyName"
 * of this InterfaceDescription in the given language.
 *
 * The set description can be retrieved by calling GetPropertyDescription() OR GetPropertyAnnotation()
 * for an "org.alljoyn.Bus.DocString" annotation with the desired language tag
 * (e.g., "org.alljoyn.Bus.DocString.en").
 * For example, a description set by calling
 * SetPropertyDescriptionForLanguage("PropertyName", "This is the property", "en") can be retrieved by calling:
 *      - GetPropertyDescriptionForLanguage("PropertyName", description, "en") OR
 *      - GetPropertyAnnotation("PropertyName", "org.alljoyn.Bus.DocString.en", description).
 *
 * @param[in] propertyName The name of the property.
 * @param[in] description The introspection description.
 * @param[in] languageTag The language of the description (language tag as defined in RFC 5646, e.g., "en-US").
 * @return
 *      - ER_OK if successful.
 *      - ER_BUS_INTERFACE_ACTIVATED If the interface has already been activated.
 *      - ER_BUS_DESCRIPTION_ALREADY_EXISTS If the interface property already has a description.
 */
- (QStatus)setPropertyDescriptionForLanguage:(NSString*)propertyName withDescription:(NSString*)description withLanguage:(NSString*)languageTag;

/**
 * Get the introspection description for for the property "propertyName"
 * of this InterfaceDescription in the given language.
 *
 * To obtain the description, the method searches for the best match of the given language tag
 * using the lookup algorithm in RFC 4647 section 3.4.
 * For example, if propertyDescriptionForLanguage("PropertyName", "en-US") is called, the method will:
 *       - Search for a description with the same language tag ("en-US"), return the description
 *       if such a description is found; else:
 *       - Search for a description with a less specific language tag ("en"), return the description
 *       if such a description is found; else:
 *       - Return nil.
 *
 * The method will also provide descriptions which have been set as description annotations
 * (set by calling AddPropertyAnnotation() with the annotation name set to "org.alljoyn.Bus.DocString"
 * plus the desired language tag, e.g., "org.alljoyn.Bus.DocString.en").
 *
 * @param[in] propertyName The name of the property.
 * @param[in] languageTag The language of the description (language tag as defined in RFC 5646, e.g., "en-US").
 * @return
 *      - description for the given language.
 *      - nil otherwise.
 */
- (NSString*)propertyDescriptionForLanguage:(NSString*)propertyName withLanguage:(NSString*)languageTag;

/**
 * Set the introspection description for the argument "arg of member" of this InterfaceDescription.
 *
 * @param description The description of the argument
 * @param argName The name of the argument
 * @param member The name of the method or signal
 *
 * @return  - ER_OK if successful
 *          - ER_BUS_INTERFACE_NO_SUCH_MEMBER if the method or signal does not exist
 *          - ER_BUS_INTERFACE_ACTIVATED if this interface has already activated
 */
- (QStatus)setArgDescription:(NSString *)description forArgument:(NSString *)argName ofMember:(NSString *)member DEPRECATED_ATTRIBUTE;

/**
 * Set the introspection description for the argument "argName" of the member "memberName"
 * of this InterfaceDescription in the given language.
 *
 * The set description can be retrieved by calling GetArgDescriptionForLanguage() OR GetArgAnnotation()
 * for an "org.alljoyn.Bus.DocString" annotation with the desired language tag
 * (e.g., "org.alljoyn.Bus.DocString.en").
 * For example, a description set by calling
 * SetArgDescriptionForLanguage("MethodName", "ArgName", "This is the argument", "en") can be retrieved by calling:
 *      - GetArgDescriptionForLanguage("MethodName", "ArgName", description, "en") OR
 *      - GetArgAnnotation("MethodName", "ArgName", "org.alljoyn.Bus.DocString.en", description).
 *
 * @param[in] memberName The name of the member.
 * @param[in] argName The name of the argument.
 * @param[in] description The introspection description.
 * @param[in] languageTag The language of the description (language tag as defined in RFC 5646, e.g., "en-US").
 * @return
 *      - ER_OK if successful.
 *      - ER_BUS_INTERFACE_ACTIVATED If the interface has already been activated.
 *      - ER_BUS_DESCRIPTION_ALREADY_EXISTS If the interface member argument already has a description.
 */
- (QStatus)setArgDescriptionForLanguage:(NSString*)memberName forArg:(NSString*)argName withDescription:(NSString*)description withLanguage:(NSString*)languageTag;

/**
 * Get the introspection description for the argument "argName" of the member "memberName"
 * of this InterfaceDescription in the given language.
 *
 * To obtain the description, the method searches for the best match of the given language tag
 * using the lookup algorithm in RFC 4647 section 3.4.
 * For example, if argDescriptionForLanguage("MethodName", "ArgName", "en-US") is called, the method will:
 *       - Search for a description with the same language tag ("en-US"), return the description
 *       if such a description is found; else:
 *       - Search for a description with a less specific language tag ("en"), return the description
 *       if such a description is found; else:
 *       - Return nil.
 *
 * The method will also provide descriptions which have been set as description annotations
 * (set by calling AddArgAnnotation() with the annotation name set to "org.alljoyn.Bus.DocString"
 * plus the desired language tag, e.g., "org.alljoyn.Bus.DocString.en").
 *
 * @param[in] memberName The name of the member.
 * @param[in] argName The name of the argument.
 * @param[in] languageTag The language of the description (language tag as defined in RFC 5646, e.g., "en-US").
 * @return
 *      - description for the given language
 *      - nil otherwise.
 */
- (NSString*)argDescriptionForLanguage:(NSString*)memberName forArg:(NSString*)argName withLanguage:(NSString*)languageTag;

/**
 * Set the Translator that provides this InterfaceDescription's introspection description in multiple lauanges.
 *
 * @param translator The AJNTranslator
 */
- (void)setDescriptionTranslator:(id<AJNTranslator>)translator DEPRECATED_ATTRIBUTE;

/**
 * Check for existence of a member. Optionally check the signature also.
 * @remark
 * if the a signature is not provided this method will only check to see if
 * a member with the given @c name exists.  If a signature is provided a
 * member with the given @c name and @c signature must exist for this to return true.
 *
 * @param name          Name of the member to lookup
 * @param inputs        Input parameter signature of the member to lookup
 * @param outputs       Output parameter signature of the member to lookup (leave NULL for signals)
 * @return TRUE if the member name exists, otherwise returns FALSE.
 */
- (BOOL)hasMemberWithName:(NSString *)name inputSignature:(NSString *)inputs outputSignature:(NSString *)outputs;

/**
 * Add an annotation to an existing argument.
 *
 * @param member     Name of member.
 * @param arg        Name of the argument.
 * @param name       Name of annotation.
 * @param value      Value for the annotation.
 *
 * @return
 *      - ER_OK if successful
 *      - ER_BUS_ANNOTATION_ALREADY_EXISTS if annotation already exists
 */
- (QStatus)addArgAnnotationWithName:(NSString*)member arg:(NSString*)arg name:(NSString*)name value:(NSString*)value;

/**
 * Get annotation from an existing argument.
 *
 * @param member     Name of member.
 * @param arg        Name of the argument.
 * @param name       Name of annotation.
 *
 * @return
 *      - annotaiton value if found
 *      - nil if annotation not found
 */
- (NSString *)getArgAnnotationWithName:(NSString *)member arg:(NSString *)arg name:(NSString *)name;

/**
 * Activate this interface. An interface must be activated before it can be used. Activating an
 * interface locks the interface so that is can no longer be modified.
 */
- (void)activate;

@end
