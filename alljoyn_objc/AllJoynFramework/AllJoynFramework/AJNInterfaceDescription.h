////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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
 * Add a signal member to the interface.
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
 *
 * @return  - ER_OK if successful
 *          - ER_BUS_MEMBER_ALREADY_EXISTS if member already exists
 */
- (QStatus)addSignalWithName:(NSString *)name inputSignature:(NSString *)inputSignature argumentNames:(NSArray *)arguments annotation:(AJNInterfaceAnnotationFlags)annotation;

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
 * @return  - ER_OK if successful.
 *          - ER_BUS_PROPERTY_ALREADY_EXISTS if the property can not be added because it already exists.
 */
- (QStatus)addPropertyWithName:(NSString*)name signature:(NSString*)signature;

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
 * Check for existence of a property.
 *
 * @param propertyName       Name of the property to lookup
 * @return An object representing the property if the property exists, otherwise nil.
 */
- (AJNInterfaceProperty *)propertyWithName:(NSString *)propertyName;

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
 * Get the annotation value for a member (signal or method).
 *
 * @param annotationName    Name of annotation
 * @param memberName        Name of member
 *
 * @return  - string value of annotation if found
 *          - nil if annotation not found
 */
- (NSString *)annotationWithName:(NSString *)annotationName forMemberWithName:(NSString *)memberName;

/**
 * Add an annotation to a member (signal or method).
 *
 * @param annotationName    Name of annotation
 * @param annotationValue   Value of annotation
 * @param memberName        Name of member
 *
 * @return  - ER_OK if successful
 *          - ER_BUS_MEMBER_ALREADY_EXISTS if annotation already exists
 */
- (QStatus)addAnnotationWithName:(NSString *)annotationName value:(NSString *)annotationValue forMemberWithName:(NSString *)memberName;

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
 * Add an annotation to a property.
 *
 * @param annotationName    Name of annotation
 * @param annotationValue   Value of annotation
 * @param propertyName      Name of property
 *
 * @return  - ER_OK if successful
 *          - ER_BUS_MEMBER_ALREADY_EXISTS if annotation already exists
 */
- (QStatus)addAnnotationWithName:(NSString *)annotationName value:(NSString *)annotationValue forPropertyWithName:(NSString *)propertyName;

/**
 * Set the description language for this Interface
 *
 * @param language the language of this Interface's descriptions
 */
- (void)setDescriptionLanguage:(NSString *)language;

/**
 * Set this Interface's description
 * 
 * @param description This Interface's description
 */
- (void)setDescription:(NSString *)description;

/**
 * Set a description for a method or signal of this Interface
 *
 * @param description The description of the method or signal
 * @param member The name of the method or signal
 * @param sessionless Set this to TRUE if this is a signal you intend on sending sessionless
 *
 * @return  - ER_OK if successful
 *          - ER_BUS_INTERFACE_NO_SUCH_MEMBER if the method or signal does not exist
 *          - ER_BUS_INTERFACE_ACTIVATED if this interface has already activated
 */
- (QStatus)setMemberDescription:(NSString *)description forMemberWithName:(NSString *)member sessionlessSignal:(BOOL)sessionless;

/**
 * Set a description for a property of this Interface
 *
 * @param description The description of the property
 * @param propName The name of the property
 *
 * @return  - ER_OK if successful
 *          - ER_BUS_NO_SUCH_PROPERTY if the property does not exist
 *          - ER_BUS_INTERFACE_ACTIVATED if this interface has already activated
 */
- (QStatus)setPropertyDescription:(NSString *)description forPropertyWithName:(NSString *)propName;

/**
 * Set a description for an argument of a method or signal of this Interface
 *
 * @param description The description of the argument
 * @param argName The name of the argument
 * @param member The name of the method or signal
 *
 * @return  - ER_OK if successful
 *          - ER_BUS_INTERFACE_NO_SUCH_MEMBER if the method or signal does not exist
 *          - ER_BUS_INTERFACE_ACTIVATED if this interface has already activated
 */
- (QStatus)setArgDescription:(NSString *)description forArgument:(NSString *)argName ofMember:(NSString *)member;

/**
 * Set this Interface's AJNTransalator
 * 
 * @param translator The AJNTranslator
 */
- (void)setDescriptionTranslator:(id<AJNTranslator>)translator;

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
 * Activate this interface. An interface must be activated before it can be used. Activating an
 * interface locks the interface so that is can no longer be modified.
 */
- (void)activate;

@end
