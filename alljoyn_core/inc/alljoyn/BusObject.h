#ifndef _ALLJOYN_LOCALBUSOBJECT_H
#define _ALLJOYN_LOCALBUSOBJECT_H
/**
 * @file
 *
 * This file defines the base class for message bus objects that
 * are implemented and registered locally.
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

#include <assert.h>

#include <qcc/String.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/MessageReceiver.h>
#include <alljoyn/Session.h>
#include <alljoyn/Status.h>
#include <alljoyn/Translator.h>

namespace ajn {

/// @cond ALLJOYN_DEV
/** @internal Forward references */
class BusAttachment;
class MethodTable;
/// @endcond

/**
 * Message Bus Object base class
 */
class BusObject : public MessageReceiver {

    friend class MethodTable;
    friend class _LocalEndpoint;

  public:
    /**
     * flag used to specify if an interface is announced or not here
     * @see AddInterface
     */
    typedef enum {
        UNANNOUNCED, ///< The interface is not announced
        ANNOUNCED ///< The interface is announced
    } AnnounceFlag;
    /**
     * Return the path for the object
     *
     * @return Object path
     */
    const char* GetPath() { return path.c_str(); }

    /**
     * Get the name of this object.
     * The name is the last component of the path.
     *
     * @return Last component of object path.
     */
    qcc::String GetName();

    /**
     * %BusObject constructor.
     *
     * @param path           Object path for object.
     * @param isPlaceholder  Place-holder objects are created by the bus itself and serve only
     *                       as parent objects (in the object path sense) to other objects.
     */
    BusObject(const char* path, bool isPlaceholder = false);

    /**
     * %BusObject constructor (Deprecated).
     *
     * @param bus            Bus that this object exists on.
     * @param path           Object path for object.
     * @param isPlaceholder  Place-holder objects are created by the bus itself and serve only
     *                       as parent objects (in the object path sense) to other objects.
     */
    QCC_DEPRECATED(BusObject(BusAttachment& bus, const char* path, bool isPlaceholder = false));

    /**
     * %BusObject destructor.
     */
    virtual ~BusObject();

    /**
     * Emit PropertiesChanged to signal the bus that this property has been updated
     *
     *  This is protected because JNI needs to be able to call it.
     *  BusObject must be registered before calling this method.
     *
     * @param ifcName   The name of the interface
     * @param propName  The name of the property being changed
     * @param val       The new value of the property
     * @param id        ID of the session we broadcast the signal to (0 for all)
     * @param flags     Flags to be added to the signal.
     */
    void EmitPropChanged(const char* ifcName, const char* propName, MsgArg& val, SessionId id, uint8_t flags = 0);

    /**
     * Emit PropertiesChanged to signal the bus that these properties have been updated
     *
     *  BusObject must be registered before calling this method.
     *
     * @param ifcName   The name of the interface
     * @param propNames An array with the names of the properties being changed
     * @param numProps  The size of the propNames array
     * @param id        ID of the session we broadcast the signal to (0 for all)
     * @param flags     Flags to be added to the signal.
     *
     * @return   ER_OK if successful.
     */
    QStatus EmitPropChanged(const char* ifcName,
                            const char** propNames,
                            size_t numProps,
                            SessionId id,
                            uint8_t flags = 0);

    /**
     * Get a reference to the underlying BusAttachment
     *
     * @return a reference to the BusAttachment
     */
    const BusAttachment& GetBusAttachment() const
    {
        assert(bus);
        return *bus;
    }

    /**
     * Send a signal.
     *
     * @param destination  The unique or well-known bus name or the signal recipient (NULL for broadcast signals)
     * @param sessionId    A unique SessionId for this AllJoyn session instance. The session this message is for.
     *                     Use SESSION_ID_ALL_HOSTED to emit on all sessions hosted by this BusObject's BusAttachment.
     *                     For broadcast or sessionless signals, the sessionId must be 0.
     * @param signal       Interface member of signal being emitted.
     * @param args         The arguments for the signal (can be NULL)
     * @param numArgs      The number of arguments
     * @param timeToLive   If non-zero this specifies the useful lifetime for this signal.
     *                     For sessionless signals the units are seconds.
     *                     For all other signals the units are milliseconds.
     *                     If delivery of the signal is delayed beyond the timeToLive due to
     *                     network congestion or other factors the signal may be discarded. There is
     *                     no guarantee that expired signals will not still be delivered.
     * @param flags        Logical OR of the message flags for this signals. The following flags apply to signals:
     *                     - If ::ALLJOYN_FLAG_GLOBAL_BROADCAST is set broadcast signal (null destination) will be
     *                       forwarded to all Routing Nodes in the system.
     *                     - If ::ALLJOYN_FLAG_ENCRYPTED is set the message is authenticated and the payload if any is
     *                       encrypted.
     *                     - If ::ALLJOYN_FLAG_SESSIONLESS is set the signal will be sent as a Sessionless Signal. NOTE:
     *                       if this flag and the GLOBAL_BROADCAST flags are set it could result in the same signal
     *                       being received twice.
     *
     * @param msg          [OUT] If non-null, the sent signal message is returned to the caller.
     * @return
     *      - #ER_OK if successful
     *      - #ER_BUS_OBJECT_NOT_REGISTERED if bus object has not yet been registered
     *      - An error status otherwise
     */
    QStatus Signal(const char* destination,
                   SessionId sessionId,
                   const InterfaceDescription::Member& signal,
                   const MsgArg* args = NULL,
                   size_t numArgs = 0,
                   uint16_t timeToLive = 0,
                   uint8_t flags = 0,
                   Message* msg = NULL);

    /**
     * Remove sessionless message sent from this object from local router's
     * store/forward cache.
     *
     * @param serialNumber    Serial number of previously sent sessionless signal.
     * @return   ER_OK if successful.
     */
    QStatus CancelSessionlessMessage(uint32_t serialNumber);

    /**
     * Remove sessionless message sent from this object from local router's
     * store/forward cache.
     *
     * @param msg    Message to be removed.
     * @return   ER_OK if successful.
     */
    QStatus CancelSessionlessMessage(const Message& msg) { return CancelSessionlessMessage(msg->GetCallSerial()); }

    /**
     * Indicates if this object is secure.
     *
     * @return Return true if authentication is required to emit signals or call methods on this object.
     */
    bool IsSecure() const { return isSecure; }

    /**
     * Set the introspection description for this BusObject.
     *
     * Note that when SetDescriptoinTranslator is used the text in this method may
     * actually be a "lookup key". When generating the introspection the "text" is first
     * passed to the Translator where the key should be used to lookup the actual
     * description. In such a case, language should be set to "".
     *
     * @param language A language tag describing the language of the description
     * @param text The description
     */
    void SetDescription(const char* language, const char* text);

    /**
     * Set the Translator that provides this BusObject's introspection description
     * in multiple languages.
     *
     * @param translator The Translator instance.
     */
    void SetDescriptionTranslator(Translator* translator);

    /**
     * Get a list of the interfaces that are added to this BusObject that will
     * be announced.
     *
     * usage example
     * @code
     * size_t numInterfaces = busObject.GetAnnouncedInterfaces(NULL, 0);
     * const char** interfaces = new const char*[numInterfaces];
     * busObject.GetAnnouncedInterfaces(interfaces, numInterfaces);
     * @endcode
     *
     * @param[in] interfaces an char* array pointer
     * @param[in] numInterfaces the size of the char* array
     *
     * @return
     *    The total number of interfaces found that are announced.  If this number
     *    is larger than `numInterfaces` then only `numInterfaces` will be returned.
     *
     */
    size_t GetAnnouncedInterfaceNames(const char** interfaces = NULL, size_t numInterfaces = 0);

    /**
     * Change the announce flag for an already added interface. Changes in the
     * announce flag are not visible to other devices till Announce is called.
     *
     * @see AboutObj::Announce()
     *
     * @param[in] iface InterfaceDescription for the interface you wish to set
     *                  the the announce flag.
     * @param[in] isAnnounced This interface should be part of the Announce signal
     *                        UNANNOUNCED - this interface will not be part of the Announce
     *                                      signal
     *                        ANNOUNCED - this interface will be part of the Announce
     *                                    signal.
     * @return
     *  - #ER_OK if successful
     *  - #ER_BUS_OBJECT_NO_SUCH_INTERFACE if the interface is not part of the
     *                                     bus object.
     */
    QStatus SetAnnounceFlag(const InterfaceDescription* iface, AnnounceFlag isAnnounced = ANNOUNCED);
  protected:

    /**
     * Type used to add multiple methods at one time.
     * @see AddMethodHandlers()
     */
//Pack option to resolve "alignment of a member was sensitive to packing" warning
#pragma pack(push, MethodEntry, 4)
    typedef struct {
        const InterfaceDescription::Member* member;          /**< Pointer to method's member */
        MessageReceiver::MethodHandler handler;              /**< Method implementation */
    } MethodEntry;
#pragma pack(pop, MethodEntry)

    /** Bus associated with object */
    BusAttachment* bus;

    /**
     * Reply to a method call.
     *
     * @param msg      The method call message
     * @param args     The reply arguments (can be NULL)
     * @param numArgs  The number of arguments
     * @return
     *      - #ER_OK if successful
     *      - #ER_BUS_OBJECT_NOT_REGISTERED if bus object has not yet been registered
     *      - An error status otherwise
     */
    QStatus MethodReply(const Message& msg, const MsgArg* args = NULL, size_t numArgs = 0);

    /**
     * Reply to a method call with an error message.
     *
     * @param msg              The method call message
     * @param error            The name of the error
     * @param errorMessage     An error message string
     * @return
     *      - #ER_OK if successful
     *      - #ER_BUS_OBJECT_NOT_REGISTERED if bus object has not yet been registered
     *      - An error status otherwise
     */
    QStatus MethodReply(const Message& msg, const char* error, const char* errorMessage = NULL);

    /**
     * Reply to a method call with an error message.
     *
     * @param msg        The method call message
     * @param status     The status code for the error
     * @return
     *      - #ER_OK if successful
     *      - #ER_BUS_OBJECT_NOT_REGISTERED if bus object has not yet been registered
     *      - An error status otherwise
     */
    QStatus MethodReply(const Message& msg, QStatus status);


    /**
     * Add an interface to this object. If the interface has properties this will also add the
     * standard property access interface. An interface must be added before its method handlers can be
     * added. Note that the Peer interface (org.freedesktop.DBus.peer) is implicit on all objects and
     * cannot be explicitly added, and the Properties interface (org.freedesktop,DBus.Properties) is
     * automatically added when needed and cannot be explicitly added.
     *
     * Once an object is registered, it should not add any additional interfaces. Doing so would
     * confuse remote objects that may have already introspected this object.
     *
     * @param iface       The interface to add
     * @param isAnnounced This interface should be part of the Announce signal
     *                    UNANNOUNCED - this interface will not be part of the Announce
     *                                  signal
     *                    ANNOUNCED - this interface will be part of the Announce
     *                                signal.
     *
     * @return
     *      - #ER_OK if the interface was successfully added.
     *      - #ER_BUS_IFACE_ALREADY_EXISTS if the interface already exists.
     *      - An error status otherwise
     */
    QStatus AddInterface(const InterfaceDescription& iface, AnnounceFlag isAnnounced = UNANNOUNCED);

    /**
     * Add a method handler to this object. The interface for the method handler must have already
     * been added by calling AddInterface().
     *
     * @param member   Interface member implemented by handler.
     * @param handler  Method handler.
     * @param context  An optional context. This is mainly intended for implementing language
     *                 bindings and should normally be NULL.
     *
     * @return
     *      - #ER_OK if the method handler was added.
     *      - An error status otherwise
     */
    QStatus AddMethodHandler(const InterfaceDescription::Member* member,
                             MessageReceiver::MethodHandler handler,
                             void* context = NULL);

    /**
     * Convenience method used to add a set of method handers at once.
     *
     * @param entries      Array of MehtodEntry
     * @param numEntries   Number of entries in array.
     *
     * @return
     *      - #ER_OK if all the methods were added
     *      - #ER_BUS_NO_SUCH_INTERFACE is method can not be added because interface does not exist.
     */
    QStatus AddMethodHandlers(const MethodEntry* entries, size_t numEntries);

    /**
     * Handle a bus request to read a property from this object.
     * BusObjects that implement properties should override this method.
     * The default version simply returns ER_BUS_NO_SUCH_PROPERTY.
     *
     * @param ifcName    Identifies the interface that the property is defined on
     * @param propName  Identifies the property to get
     * @param[out] val        Returns the property value. The type of this value is the actual value
     *                   type.
     * @return #ER_BUS_NO_SUCH_PROPERTY (Should be changed by user implementation of BusObject)
     */
    virtual QStatus Get(const char* ifcName, const char* propName, MsgArg& val) {
        QCC_UNUSED(ifcName);
        QCC_UNUSED(propName);
        QCC_UNUSED(val);
        return ER_BUS_NO_SUCH_PROPERTY;
    }

    /**
     * Handle a bus attempt to write a property value to this object.
     * BusObjects that implement properties should override this method.
     * This default version just replies with ER_BUS_NO_SUCH_PROPERTY
     *
     * @param ifcName    Identifies the interface that the property is defined on
     * @param propName  Identifies the property to set
     * @param val        The property value to set. The type of this value is the actual value
     *                   type.
     * @return #ER_BUS_NO_SUCH_PROPERTY (Should be changed by user implementation of BusObject)
     */
    virtual QStatus Set(const char* ifcName, const char* propName, MsgArg& val) {
        QCC_UNUSED(ifcName);
        QCC_UNUSED(propName);
        QCC_UNUSED(val);
        return ER_BUS_NO_SUCH_PROPERTY;
    }

    /**
     * Returns a description of the object in the D-Bus introspection XML format.
     * This method can be overridden by derived classes in order to customize the
     * introspection XML presented to remote nodes. Note that to DTD description and
     * the root element are not generated.
     *
     * @param deep     Include XML for all descendants rather than stopping at direct children.
     * @param indent   Number of characters to indent the XML
     * @return Description of the object in AllJoyn introspection XML format
     */
    virtual qcc::String GenerateIntrospection(bool deep = false, size_t indent = 0) const;

    /**
     * Returns a description of the object in the AllJoyn introspection XML format.
     * This is the same as the D-Bus format but includes <description> elements.
     * This method can be overridden by derived classes in order to customize the
     * introspection XML presented to remote nodes. Note that to DTD description and
     * the root element are not generated.
     *
     * @param languageTag   language reguested for <description>'s. NULL for no descriptions.
     * @param deep     Include XML for all descendants rather than stopping at direct children.
     * @param indent   Number of characters to indent the XML
     * @return Description of the object in AllJoyn introspection XML format
     */
    virtual qcc::String GenerateIntrospection(const char* languageTag, bool deep = false, size_t indent = 0) const;

    /**
     * Called by the message bus when the object has been successfully registered. The object can
     * perform any initialization such as adding match rules at this time.
     */
    virtual void ObjectRegistered(void) { }

    /**
     * Called by the message bus when the object has been successfully unregistered
     * @remark
     * This base class implementation @b must be called explicitly by any overriding derived class.
     */
    virtual void ObjectUnregistered(void) { isRegistered = false; }

    /**
     * Default handler for a bus attempt to read a property value.
     * @remark
     * A derived class can override this function to provide a custom handler for the GetProp method
     * call. If overridden the custom handler must compose an appropriate reply message to return the
     * requested property value.
     *
     * @param member   Identifies the org.freedesktop.DBus.Properties.Get method.
     * @param msg      The Properties.Get request.
     */
    virtual void GetProp(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Default handler for a bus attempt to write a property value.
     * @remark
     * A derived class can override this function to provide a custom handler for the SetProp method
     * call. If overridden the custom handler must compose an appropriate reply message.
     *
     * @param member   Identifies the org.freedesktop.DBus.Properties.Set method.
     * @param msg      The Properties.Set request.
     */
    virtual void SetProp(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Default handler for a bus attempt to read all properties on an interface.
     * @remark
     * A derived class can override this function to provide a custom handler for the GetAllProps
     * method call. If overridden the custom handler must compose an appropriate reply message
     * listing all properties on this object.
     *
     * @param member   Identifies the org.freedesktop.DBus.Properties.GetAll method.
     * @param msg      The Properties.GetAll request.
     */
    virtual void GetAllProps(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Default handler for a bus attempt to read the object's introspection data.
     * @remark
     * A derived class can override this function to provide a custom handler for the Introspect method
     * call. If overridden the custom handler must compose an appropriate reply message.
     *
     * @param member   Identifies the @c org.freedesktop.DBus.Introspectable.Introspect method.
     * @param msg      The Introspectable.Introspect request.
     */
    virtual void Introspect(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Default handler for a bus attempt to read the object's introspection data with descriptions.
     * @remark
     * A derived class can override this function to provide a custom handler for the IntrospectWithDescription method
     * call. If overridden the custom handler must compose an appropriate reply message.
     *
     * @param member   Identifies the @c org.allseen.Introspectable.IntrospectWithDescription method.
     * @param msg      The Introspectable.IntrospectWithDescription request.
     */
    virtual void IntrospectWithDescription(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Default handler for a bus attempt to read the languages available for IntrospectWithDescription
     * @remark
     * A derived class can override this function to provide a custom handler for the GetDescriptionLanguages method
     * call. If overridden the custom handler must compose an appropriate reply message.
     *
     * @param member   Identifies the @c org.allseen.Introspectable.GetDescriptionLanguages method.
     * @param msg      The Introspectable.GetDescriptionLanguages request.
     */
    virtual void GetDescriptionLanguages(const InterfaceDescription::Member* member, Message& msg);

    /**
     * This method can be overridden to provide access to the context registered in the AddMethodHandler() call.
     *
     * @param member  The method being called.
     * @param handler The handler to call.
     * @param message The message containing the method call arguments.
     * @param context NULL or a private context passed in when the method handler was registered.
     */
    virtual void CallMethodHandler(MessageReceiver::MethodHandler handler,
                                   const InterfaceDescription::Member* member,
                                   Message& message,
                                   void* context) {
        QCC_UNUSED(context);
        (this->*handler)(member, message);
    }

  private:

    /**
     * Assignment operator is private.
     */
    BusObject& operator=(const BusObject&) { return *this; }

    /**
     * Copy constructor is private.
     */
    BusObject(const BusObject& other) : bus(other.bus) { }

    /**
     * Add the registered methods for this object to a method table.
     *
     * @param methodTable   The method table to which this objects methods are added.
     */
    void InstallMethods(MethodTable& methodTable);

    /**
     * This utility method is called by the bus during object registration.
     * Do not call this object explicitly.
     *
     * @param bus  BusAttachement to associate with BusObject.
     * @return
     *      - #ER_OK if all the methods were added
     *      - #ER_BUS_NO_SUCH_INTERFACE is method can not be added because interface does not exist.
     */
    QStatus DoRegistration(BusAttachment& bus);

    /**
     * Returns true if this object implements the given interface.
     *
     * @param  iface   The name of the interface to look for.
     *
     * @return true iff object implements interface.
     */
    bool ImplementsInterface(const char* iface);

    /**
     * Replace this object by another one. This may require unlinking the existing object from its
     * parent and children and linking in the new one.
     *
     * @param object  The object that is replacing this object.
     */
    void Replace(BusObject& object);

    /**
     * Add an object as a child of this object.
     *
     * @param child The object to add as a child of this object.
     */
    void AddChild(BusObject& child);

    /**
     * Remove a child from this object returning the removed child.
     *
     * @return  Returns the removed child or NULL if the object has no children.
     */
    BusObject* RemoveChild();

    /**
     * Remove a specific child from this object.
     *
     * @param obj  Pointer to the child to be removed.
     * @return
     *      - #ER_OK if successful.
     *      - #ER_BUS_NO_SUCH_OBJECT otherwise.
     */
    QStatus RemoveChild(BusObject& obj);

    /**
     * Indicate that this BusObject is being used by an alternate thread.
     * This BusObject should not be deleted till the remote thread has completed
     * using this object.
     * This will increment a counter for each thread that calls this method
     */
    void InUseIncrement();

    /**
     * Indicate that this BusObject is no longer being used by an alternate thread.
     * It is safe to delete the object when the inUse counter has reached zero.
     */
    void InUseDecrement();

    /**
     * Get the introspection description for the provided language or NULL if not
     * defined.
     *
     * @param toLanguage The language tag for which the description is being requested.
     * @param buffer used by the Translator so that we can free its memory
     * @return The description or NULL if not found
     */
    const char* GetDescription(const char* toLanguage, qcc::String& buffer) const;

    struct Components;
    Components* components; /**< Internal components of this object */

    /** Object path of this object */
    const qcc::String path;

    /** Parent object of this object (NULL if this is the root object) */
    BusObject* parent;

    /** true if object's ObjectRegistered callback has been called */
    bool isRegistered;

    /** true if object is a placeholder (i.e. only exists to be the parent of a more meaningful object instance) */
    bool isPlaceholder;

    /** true if this object is secure */
    bool isSecure;

    /** Language tag of the default description for this BusObject */
    qcc::String languageTag;

    /** Default Description */
    qcc::String description;

    /** Provides descriptions in other languages */
    Translator* translator;
};

}

#endif
