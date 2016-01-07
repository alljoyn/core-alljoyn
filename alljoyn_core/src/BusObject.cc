/**
 * @file
 *
 * This file implements the BusObject class
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


#include <algorithm>
#include <map>
#include <vector>
#include <set>

#include <qcc/Debug.h>
#include <qcc/Util.h>
#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <qcc/XmlElement.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusObject.h>

#include <alljoyn/Status.h>
#include "Router.h"
#include "LocalTransport.h"
#include "AllJoynPeerObj.h"
#include "MethodTable.h"
#include "BusInternal.h"


#define QCC_MODULE "ALLJOYN"

using namespace qcc;
using namespace std;

namespace ajn {

/** Type definition for a METHOD message handler */
//Pack option to resolve "alignment of a member was sensitive to packing" warning
#pragma pack(push, MethodContext, 4)
typedef struct {
    const InterfaceDescription::Member* member;   /**< Pointer to method's member */
    MessageReceiver::MethodHandler handler;       /**< Method implementation */
    void* context;
} MethodContext;
#pragma pack(pop, MethodContext)

static bool operator==(const MethodContext& a, const MethodContext& b)
{
    return (a.member == b.member) &&
           (a.handler == b.handler) &&
           (a.context == b.context);
}

struct BusObject::Components {
    /** The interfaces this object implements */
    vector<pair<const InterfaceDescription*, bool> > ifaces;
    /** The method handlers for this object */
    vector<MethodContext> methodContexts;
    /** Child objects of this object */
    vector<BusObject*> children;

    /** lock to prevent inUseCounter from being modified by two threads at the same time.*/
    qcc::Mutex counterLock;

    /** counter to prevent this BusObject being deleted if it is being used by another thread. */
    volatile int32_t inUseCounter;
};

/**
 * Callback to allow the signal caller to authorize the signal.
 */
class SignalAuthorizationCallback {
  public:
    SignalAuthorizationCallback(BusAttachment& bus, qcc::String ifcName, vector<qcc::String> propNames) : bus(bus), ifcName(ifcName), propNames(propNames)
    {
    }

    ~SignalAuthorizationCallback()
    {
        propNames.clear();
    }

    /**
     * Authorize the signal
     * @return ER_OK when signal is authorized; otherwise, a failure code
     */
    QStatus Authorize(Message& msg)
    {
        if (propNames.size() == 0) {
            return ER_PERMISSION_DENIED;
        }

        PeerState peerState = bus.GetInternal().GetPeerStateTable()->GetPeerState(msg->GetDestination());
        for (vector<qcc::String>::iterator it = propNames.begin(); it != propNames.end(); it++) {
            QStatus status = bus.GetInternal().GetPermissionManager().AuthorizeGetProperty(msg->GetObjectPath(), ifcName.c_str(), (*it).c_str(), peerState);
            if (ER_OK != status) {
                return status;
            }
        }
        return ER_OK;
    }

  private:
    /**
     * Assignment not allowed
     */
    SignalAuthorizationCallback& operator=(const SignalAuthorizationCallback& other);

    /**
     * Copy constructor not allowed
     */
    SignalAuthorizationCallback(const SignalAuthorizationCallback& other);

    BusAttachment& bus;
    qcc::String ifcName;
    vector<qcc::String> propNames;
};

static inline bool SecurityApplies(const BusObject* obj, const InterfaceDescription* ifc)
{
    InterfaceSecurityPolicy ifcSec = ifc->GetSecurityPolicy();
    if (ifcSec == AJ_IFC_SECURITY_REQUIRED) {
        return true;
    } else {
        return obj->IsSecure() && (ifcSec != AJ_IFC_SECURITY_OFF);
    }
}

/*
 * Helper function to lookup an interface. Because we don't expect objects to implement more than a
 * small number of interfaces we just use a simple linear search.
 */
static const InterfaceDescription* LookupInterface(vector<pair<const InterfaceDescription*, bool> >& ifaces, const char* ifName)
{
    vector<pair <const InterfaceDescription*, bool> >::const_iterator it = ifaces.begin();
    while (it != ifaces.end()) {
        if (0 == strcmp((it->first)->GetName(), ifName)) {
            return it->first;
        } else {
            ++it;
        }
    }
    return NULL;
}

bool BusObject::ImplementsInterface(const char* ifName)
{
    return LookupInterface(components->ifaces, ifName) != NULL;
}

qcc::String BusObject::GetName()
{
    if (!path.empty()) {
        qcc::String name = path;
        size_t pos = name.find_last_of('/');
        if (pos == 0) {
            if (name.length() > 1) {
                name.erase(0, 1);
            }
        } else {
            name.erase(0, pos + 1);
        }
        return name;
    } else {
        return qcc::String("<anonymous>");
    }
}

qcc::String BusObject::GenerateIntrospection(bool deep, size_t indent) const
{
    return BusObject::GenerateIntrospection(NULL, deep, indent);
}

qcc::String BusObject::GenerateIntrospection(const char* requestedLanguageTag, bool deep, size_t indent) const
{
    qcc::String in(indent, ' ');
    qcc::String xml;
    qcc::String buffer;

    /* Iterate over child nodes */
    vector<BusObject*>::const_iterator iter = components->children.begin();
    while (iter != components->children.end()) {
        BusObject* child = *iter++;
        xml += in + "<node name=\"" + child->GetName() + "\"";

        const char* nodeDesc = NULL;
        if (requestedLanguageTag) {
            nodeDesc = child->GetDescription(requestedLanguageTag, buffer);
        }

        if (deep || nodeDesc) {
            xml += ">\n";
            if (nodeDesc) {
                xml += in + "  <description>" + XmlElement::EscapeXml(nodeDesc) + "</description>";
            }
            if (deep) {
                xml += child->GenerateIntrospection(requestedLanguageTag, deep, indent + 2);
            }

            xml += "\n" + in + "</node>\n";
        } else {
            xml += "/>\n";
        }
    }
    if (deep || !isPlaceholder) {
        /* Iterate over interfaces */
        vector<pair<const InterfaceDescription*, bool> >::const_iterator itIf = components->ifaces.begin();
        while (itIf != components->ifaces.end()) {
            /*
             * We need to omit the standard D-Bus interfaces from the
             * introspection data due to a bug in AllJoyn 14.06 and
             * older.  This will allow older versions of AllJoyn to
             * introspect us and not fail.  Sadly, this hack can never
             * be removed.
             */
            if ((strcmp((itIf->first)->GetName(), org::freedesktop::DBus::InterfaceName) == 0) ||
                (strcmp((itIf->first)->GetName(), org::freedesktop::DBus::Properties::InterfaceName) == 0)) {
                ++itIf;
            } else {
                xml += (itIf->first)->Introspect(indent, requestedLanguageTag, bus ? bus->GetDescriptionTranslator() : NULL);
                ++itIf;
            }
        }
    }
    return xml;
}

void BusObject::GetProp(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);

    QStatus status;
    const MsgArg* iface = msg->GetArg(0);
    const MsgArg* property = msg->GetArg(1);
    MsgArg val = MsgArg();

    /* Check property exists on this interface and is readable */
    const InterfaceDescription* ifc = LookupInterface(components->ifaces, iface->v_string.str);
    if (ifc) {
        /*
         * If the object or interface is secure the message must be encrypted
         */
        if (!msg->IsEncrypted() && SecurityApplies(this, ifc)) {
            status = ER_BUS_MESSAGE_NOT_ENCRYPTED;
            QCC_LogError(status, ("Attempt to get a property from a secure %s", isSecure ? "object" : "interface"));
        } else {
            const InterfaceDescription::Property* prop = ifc->GetProperty(property->v_string.str);
            if (prop) {
                if (prop->access & PROP_ACCESS_READ) {
                    status = Get(iface->v_string.str, property->v_string.str, val);
                } else {
                    QCC_DbgPrintf(("No read access on property %s", property->v_string.str));
                    status = ER_BUS_PROPERTY_ACCESS_DENIED;
                }
            } else {
                status = ER_BUS_NO_SUCH_PROPERTY;
            }
        }
    } else {
        status = ER_BUS_UNKNOWN_INTERFACE;
    }
    QCC_DbgPrintf(("Properties.Get %s", QCC_StatusText(status)));
    if (status == ER_OK) {
        /* Properties are returned as variants */
        MsgArg arg = MsgArg(ALLJOYN_VARIANT);
        arg.v_variant.val = &val;
        MethodReply(msg, &arg, 1);
        /* Prevent destructor from attempting to free val */
        arg.v_variant.val = NULL;
    } else {
        MethodReply(msg, status);
    }
}

void BusObject::EmitPropChanged(const char* ifcName, const char* propName, MsgArg& val, SessionId id, uint8_t flags)
{
    QCC_DbgTrace(("BusObject::EmitPropChanged(ifcName = \"%s\", propName = \"%s\", val = <>, id = %u)",
                  ifcName, propName, id));
    QCC_ASSERT(bus);
    const InterfaceDescription* ifc = bus->GetInterface(ifcName);

    qcc::String emitsChanged;
    if (ifc && ifc->GetPropertyAnnotation(propName, org::freedesktop::DBus::AnnotateEmitsChanged, emitsChanged)) {
        QCC_DbgPrintf(("emitsChanged = %s", emitsChanged.c_str()));
        if (SecurityApplies(this, ifc)) {
            flags |= ALLJOYN_FLAG_ENCRYPTED;
        }
        if (emitsChanged == "true") {
            const InterfaceDescription* bus_ifc = bus->GetInterface(org::freedesktop::DBus::Properties::InterfaceName);
            const InterfaceDescription::Member* propChanged = (bus_ifc ? bus_ifc->GetMember("PropertiesChanged") : NULL);

            QCC_DbgPrintf(("propChanged = %s", propChanged ? propChanged->name.c_str() : NULL));
            if (propChanged != NULL) {
                MsgArg args[3];
                args[0].Set("s", ifcName);
                MsgArg str("{sv}", propName, &val);
                args[1].Set("a{sv}", 1, &str);
                args[2].Set("as", 0, NULL);
                vector<qcc::String> vNames;
                vNames.push_back(propName);
                SignalAuthorizationCallback signalAuth(*bus, ifcName, vNames);
                vNames.clear();
                SignalInternal(NULL, id, *propChanged, args, ArraySize(args), 0, flags, NULL, &signalAuth);
            }
        } else if (emitsChanged == "invalidates") {
            const InterfaceDescription* bus_ifc = bus->GetInterface(org::freedesktop::DBus::Properties::InterfaceName);
            const InterfaceDescription::Member* propChanged = (bus_ifc ? bus_ifc->GetMember("PropertiesChanged") : NULL);
            if (NULL != propChanged) {
                // EMPTY array, followed by array of strings
                MsgArg args[3];
                args[0].Set("s", ifcName);
                args[1].Set("a{sv}", 0, NULL);
                args[2].Set("as", 1, &propName);
                vector<qcc::String> vNames;
                vNames.push_back(propName);
                SignalAuthorizationCallback signalAuth(*bus, ifcName, vNames);
                vNames.clear();
                SignalInternal(NULL, id, *propChanged, args, ArraySize(args), 0, flags, NULL, &signalAuth);
            }
        }
    }
}

QStatus BusObject::EmitPropChanged(const char* ifcName, const char** propNames, size_t numProps, SessionId id, uint8_t flags)
{
    QCC_ASSERT(bus);
    qcc::String emitsChanged;
    QStatus status = ER_OK;

    const InterfaceDescription* ifc = bus->GetInterface(ifcName);
    if (!ifc) {
        status = ER_BUS_UNKNOWN_INTERFACE;
    } else {
        MsgArg* updatedProp = new MsgArg[numProps];
        const char** invalidatedProp = new const char*[numProps];
        size_t updatedPropNum = 0;
        size_t invalidatedPropNum = 0;

        if (SecurityApplies(this, ifc)) {
            flags |= ALLJOYN_FLAG_ENCRYPTED;
        }
        vector<qcc::String> vNames;
        for (size_t i = 0; i < numProps; ++i) {
            const char* propName = propNames[i];
            const InterfaceDescription::Property* prop = ifc->GetProperty(propName);
            if (!prop) {
                status = ER_BUS_NO_SUCH_PROPERTY;
                break;
            }
            if ((prop->access & PROP_ACCESS_READ) &&
                ifc->GetPropertyAnnotation(String(propName), org::freedesktop::DBus::AnnotateEmitsChanged,
                                           emitsChanged)) {
                /* property has emitschanged annotation and is readable */
                if (emitsChanged == "true") {
                    /* also emit the value */
                    MsgArg* val = new MsgArg();
                    status = Get(ifcName, propName, *val);
                    if (status != ER_OK) {
                        delete val;
                        status = ER_BUS_NO_SUCH_PROPERTY;
                        break;
                    }
                    updatedProp[updatedPropNum].Set("{sv}", propName, val);
                    updatedProp[updatedPropNum].SetOwnershipFlags(MsgArg::OwnsArgs, true /*deep*/);
                    updatedPropNum++;
                    vNames.push_back(propName);
                } else if (emitsChanged == "invalidates") {
                    /* only emit that it's invalidated */
                    invalidatedProp[invalidatedPropNum] = propName;
                    invalidatedPropNum++;
                    vNames.push_back(propName);
                }
            }
        }
        if (status == ER_OK) {
            const InterfaceDescription* bus_ifc = bus->GetInterface(org::freedesktop::DBus::Properties::InterfaceName);
            QCC_ASSERT(bus_ifc);
            const InterfaceDescription::Member* propChanged = bus_ifc->GetMember("PropertiesChanged");
            QCC_ASSERT(propChanged);

            MsgArg args[3];
            args[0].Set("s", ifcName);
            args[1].Set("a{sv}", updatedPropNum, updatedProp);
            args[2].Set("as", invalidatedPropNum, invalidatedProp);
            SignalAuthorizationCallback signalAuth(*bus, ifcName, vNames);
            /* send the signal */
            status = SignalInternal(NULL, id, *propChanged, args, ArraySize(args), 0, flags, NULL, &signalAuth);
        }
        vNames.clear();

        delete[] updatedProp;
        delete[] invalidatedProp;
    }
    return status;
}

void BusObject::SetProp(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);

    QStatus status = ER_BUS_NO_SUCH_PROPERTY;
    const MsgArg* iface = msg->GetArg(0);
    const MsgArg* property = msg->GetArg(1);
    const MsgArg* val = msg->GetArg(2);

    /* Check property exists on this interface has correct signature and is writeable */
    const InterfaceDescription* ifc = LookupInterface(components->ifaces, iface->v_string.str);
    if (ifc) {
        /*
         * If the object or interface is secure the message must be encrypted
         */
        if (!msg->IsEncrypted() && SecurityApplies(this, ifc)) {
            status = ER_BUS_MESSAGE_NOT_ENCRYPTED;
            QCC_LogError(status, ("Attempt to set a property on a secure %s", isSecure ? "object" : "interface"));
        } else {
            const InterfaceDescription::Property* prop = ifc->GetProperty(property->v_string.str);
            if (prop) {
                if (!val->v_variant.val->HasSignature(prop->signature.c_str())) {
                    QCC_DbgPrintf(("Property value for %s has wrong type %s", property->v_string.str, prop->signature.c_str()));
                    status = ER_BUS_SET_WRONG_SIGNATURE;
                } else if (prop->access & PROP_ACCESS_WRITE) {
                    status = Set(iface->v_string.str, property->v_string.str, *(val->v_variant.val));
                } else {
                    QCC_DbgPrintf(("No write access on property %s", property->v_string.str));
                    status = ER_BUS_PROPERTY_ACCESS_DENIED;
                }
            } else {
                status = ER_BUS_NO_SUCH_PROPERTY;
            }
        }
    } else {
        status = ER_BUS_UNKNOWN_INTERFACE;
    }
    QCC_DbgPrintf(("Properties.Set %s", QCC_StatusText(status)));
    MethodReply(msg, status);
}

void BusObject::GetAllProps(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);

    QStatus status = ER_OK;
    const MsgArg* iface = msg->GetArg(0);
    MsgArg vals;
    const InterfaceDescription::Property** props = NULL;

    /* Check interface exists and has properties */
    const InterfaceDescription* ifc = LookupInterface(components->ifaces, iface->v_string.str);
    if (ifc) {
        /*
         * If the object or interface is secure the message must be encrypted
         */
        if (!msg->IsEncrypted() && SecurityApplies(this, ifc)) {
            status = ER_BUS_MESSAGE_NOT_ENCRYPTED;
            QCC_LogError(status, ("Attempt to get properties from a secure %s", isSecure ? "object" : "interface"));
        } else {
            size_t numProps = ifc->GetProperties();
            props = new const InterfaceDescription::Property*[numProps];
            ifc->GetProperties(props, numProps);
            bool* allowed = new bool[numProps];
            for (size_t i = 0; i < numProps; i++) {
                allowed[i] = true;
            }
            size_t readable = 0;
            /* Count readable properties */
            if (msg->IsEncrypted()) {
                PeerState peerState = bus->GetInternal().GetPeerStateTable()->GetPeerState(msg->GetSender());
                for (size_t i = 0; i < numProps; i++) {
                    if (props[i]->access & PROP_ACCESS_READ) {
                        if (ER_OK == bus->GetInternal().GetPermissionManager().AuthorizeGetProperty(msg->GetObjectPath(), ifc->GetName(), props[i]->name.c_str(), peerState)) {
                            readable++;
                        } else {
                            /* mark the property as not allowed because of permission denied */
                            allowed[i] = false;
                        }
                    }
                }
            } else {
                for (size_t i = 0; i < numProps; i++) {
                    if (props[i]->access & PROP_ACCESS_READ) {
                        readable++;
                    }
                }
            }

            MsgArg* dict = NULL;
            if (readable > 0) {
                dict = new MsgArg[readable];
                MsgArg* entry = dict;
                /* Get readable properties */
                for (size_t i = 0; i < numProps; i++) {
                    if ((props[i]->access & PROP_ACCESS_READ) && allowed[i]) {
                        MsgArg* val = new MsgArg();
                        status = Get(iface->v_string.str, props[i]->name.c_str(), *val);
                        if (status != ER_OK) {
                            delete val;
                            break;
                        }
                        entry->Set("{sv}", props[i]->name.c_str(), val);
                        entry->v_dictEntry.val->SetOwnershipFlags(MsgArg::OwnsArgs, false);
                        entry++;
                    }
                }
            }
            vals.Set("a{sv}", readable, dict);
            vals.SetOwnershipFlags(MsgArg::OwnsArgs, false);
            delete [] allowed;
        }
    } else {
        status = ER_BUS_UNKNOWN_INTERFACE;
    }
    QCC_DbgPrintf(("Properties.GetAll %s", QCC_StatusText(status)));
    if (status == ER_OK) {
        MethodReply(msg, &vals, 1);
    } else {
        MethodReply(msg, status);
    }
    delete [] props;
}

void BusObject::Introspect(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);

    qcc::String xml = org::freedesktop::DBus::Introspectable::IntrospectDocType;
    xml += "<node>\n";
    if (isSecure) {
        xml += "  <annotation name=\"org.alljoyn.Bus.Secure\" value=\"true\"/>\n";
    }
    xml += GenerateIntrospection(false, 2);
    xml += "</node>\n";
    MsgArg arg("s", xml.c_str());
    QStatus status = MethodReply(msg, &arg, 1);
    if (status != ER_OK) {
        QCC_DbgPrintf(("Introspect %s", QCC_StatusText(status)));
    }
}

QStatus BusObject::AddMethodHandler(const InterfaceDescription::Member* member, MessageReceiver::MethodHandler handler, void* handlerContext)
{
    if (!member) {
        return ER_BAD_ARG_1;
    }
    if (!handler) {
        return ER_BAD_ARG_2;
    }
    QStatus status = ER_OK;
    if (isRegistered) {
        status = ER_BUS_CANNOT_ADD_HANDLER;
        QCC_LogError(status, ("Cannot add method handler to an object that is already registered"));
    } else if (ImplementsInterface(member->iface->GetName())) {
        MethodContext ctx = { member, handler, handlerContext };
        if (find(components->methodContexts.begin(), components->methodContexts.end(), ctx) == components->methodContexts.end()) {
            components->methodContexts.push_back(ctx);
        }
    } else {
        status = ER_BUS_NO_SUCH_INTERFACE;
        QCC_LogError(status, ("Cannot add method handler for unknown interface"));
    }
    return status;
}

QStatus BusObject::AddMethodHandlers(const MethodEntry* entries, size_t numEntries)
{
    if (!entries) {
        return ER_BAD_ARG_1;
    }
    QStatus status = ER_OK;
    for (size_t i = 0; i < numEntries; ++i) {
        status = AddMethodHandler(entries[i].member, entries[i].handler);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to add method handler for %s.%s", entries[i].member->iface->GetName(), entries[i].member->name.c_str()));
            break;
        }
    }
    return status;
}

void BusObject::InstallMethods(MethodTable& methodTable)
{
    vector<MethodContext>::iterator iter;
    for (iter = components->methodContexts.begin(); iter != components->methodContexts.end(); iter++) {
        const MethodContext methodContext = *iter;
        methodTable.Add(this, methodContext.handler, methodContext.member, methodContext.context);
    }
}

QStatus BusObject::AddInterface(const InterfaceDescription& iface, AnnounceFlag isAnnounced)
{
    QStatus status = ER_OK;
    if (isRegistered) {
        status = ER_BUS_CANNOT_ADD_INTERFACE;
        QCC_LogError(status, ("Cannot add an interface to an object that is already registered"));
        goto ExitAddInterface;
    }
    /* The Peer interface is implicit on all objects so cannot be explicitly added */
    if (strcmp(iface.GetName(), org::freedesktop::DBus::Peer::InterfaceName) == 0) {
        status = ER_BUS_IFACE_ALREADY_EXISTS;
        QCC_LogError(status, ("%s is implicit on all objects and cannot be added manually", iface.GetName()));
        goto ExitAddInterface;
    }
    /* The Properties interface is automatically added when needed so cannot be explicitly added */
    if (strcmp(iface.GetName(), org::freedesktop::DBus::Properties::InterfaceName) == 0) {
        status = ER_BUS_IFACE_ALREADY_EXISTS;
        QCC_LogError(status, ("%s is automatically added if needed and cannot be added manually", iface.GetName()));
        goto ExitAddInterface;
    }
    /* The DBus.Introspectable interface is automatically added when needed so cannot be explicitly added */
    if (strcmp(iface.GetName(), org::freedesktop::DBus::Introspectable::InterfaceName) == 0) {
        status = ER_BUS_IFACE_ALREADY_EXISTS;
        QCC_LogError(status, ("%s is automatically added if needed and cannot be added manually", iface.GetName()));
        goto ExitAddInterface;
    }
    /* The allseen.Introspectable interface is automatically added when needed so cannot be explicitly added */
    if (strcmp(iface.GetName(), org::allseen::Introspectable::InterfaceName) == 0) {
        status = ER_BUS_IFACE_ALREADY_EXISTS;
        QCC_LogError(status, ("%s is automatically added if needed and cannot be added manually", iface.GetName()));
        goto ExitAddInterface;
    }
    /* Check interface has not already been added */
    if (ImplementsInterface(iface.GetName())) {
        status = ER_BUS_IFACE_ALREADY_EXISTS;
        QCC_LogError(status, ("%s already added to this object", iface.GetName()));
        goto ExitAddInterface;
    }

    /* Add the new interface */
    components->ifaces.push_back(make_pair(&iface, isAnnounced));

ExitAddInterface:

    return status;
}

QStatus BusObject::DoRegistration(BusAttachment& busAttachment)
{
    QStatus status;

    // Set the BusAttachment as part of the object registration. This will
    // overwrite the one from the (deprecated) constructor.
    bus = &busAttachment;

    /* Add the standard DBus interfaces */
    const InterfaceDescription* introspectable = bus->GetInterface(org::freedesktop::DBus::Introspectable::InterfaceName);
    QCC_ASSERT(introspectable);
    pair<const InterfaceDescription*, bool> comp = make_pair(introspectable, false);
    if (find(components->ifaces.begin(), components->ifaces.end(), comp) == components->ifaces.end()) {
        components->ifaces.push_back(comp);
    }

    const InterfaceDescription* allseenIntrospectable = bus->GetInterface(org::allseen::Introspectable::InterfaceName);
    QCC_ASSERT(allseenIntrospectable);
    comp = make_pair(allseenIntrospectable, false);
    if (find(components->ifaces.begin(), components->ifaces.end(), comp) == components->ifaces.end()) {
        components->ifaces.push_back(comp);
    }

    /* Add the standard method handlers */
    const MethodEntry methodEntries[] = {
        { introspectable->GetMember("Introspect"), static_cast<MessageReceiver::MethodHandler>(&BusObject::Introspect) },
        { allseenIntrospectable->GetMember("GetDescriptionLanguages"),
          static_cast<MessageReceiver::MethodHandler>(&BusObject::GetDescriptionLanguages) },
        { allseenIntrospectable->GetMember("IntrospectWithDescription"),
          static_cast<MessageReceiver::MethodHandler>(&BusObject::IntrospectWithDescription) }
    };

    /* If any of the interfaces has properties make sure the Properties interface and its method handlers are registered. */
    for (size_t i = 0; i < components->ifaces.size(); ++i) {
        const InterfaceDescription* iface = components->ifaces[i].first;
        if (iface->HasProperties() && !ImplementsInterface(org::freedesktop::DBus::Properties::InterfaceName)) {

            /* Add the org::freedesktop::DBus::Properties interface to this list of interfaces implemented by this obj */
            const InterfaceDescription* propIntf = bus->GetInterface(org::freedesktop::DBus::Properties::InterfaceName);
            QCC_ASSERT(propIntf);
            comp = make_pair(propIntf, false);
            if (find(components->ifaces.begin(), components->ifaces.end(), comp) == components->ifaces.end()) {
                components->ifaces.push_back(comp);
            }

            /* Attach the handlers */
            const MethodEntry propHandlerList[] = {
                { propIntf->GetMember("Get"),    static_cast<MessageReceiver::MethodHandler>(&BusObject::GetProp) },
                { propIntf->GetMember("Set"),    static_cast<MessageReceiver::MethodHandler>(&BusObject::SetProp) },
                { propIntf->GetMember("GetAll"), static_cast<MessageReceiver::MethodHandler>(&BusObject::GetAllProps) }
            };
            status = AddMethodHandlers(propHandlerList, ArraySize(propHandlerList));
            if (ER_OK != status) {
                QCC_LogError(status, ("Failed to add property getter/setter message receivers for %s", GetPath()));
                return status;
            }
            break;
        }
    }
    status = AddMethodHandlers(methodEntries, ArraySize(methodEntries));
    return status;
}

QStatus BusObject::SignalInternal(const char* destination,
                                  SessionId sessionId,
                                  const InterfaceDescription::Member& signalMember,
                                  const MsgArg* args,
                                  size_t numArgs,
                                  uint16_t timeToLive,
                                  uint8_t flags,
                                  Message* outMsg,
                                  SignalAuthorizationCallback* authorizationCallback)
{
    /* Protect against calling Signal before object is registered */
    if (!bus) {
        return ER_BUS_OBJECT_NOT_REGISTERED;
    }

    /*
     * If the object or interface is secure or encryption is explicitly requested the signal must be encrypted.
     */
    if (SecurityApplies(this, signalMember.iface)) {
        flags |= ALLJOYN_FLAG_ENCRYPTED;
    }
    if ((flags & ALLJOYN_FLAG_ENCRYPTED) && !bus->IsPeerSecurityEnabled()) {
        return ER_BUS_SECURITY_NOT_ENABLED;
    }

    if (signalMember.isSessioncastSignal ||
        signalMember.isSessionlessSignal ||
        signalMember.isUnicastSignal ||
        signalMember.isGlobalBroadcastSignal) {
        // Enforce signal type, since signal type was explicitly set.
        if ((destination == NULL) && (sessionId != 0) && !signalMember.isSessioncastSignal) {
            QCC_LogError(ER_INVALID_SIGNAL_EMISSION_TYPE, ("Attempt to send a sessioncast signal when %s is not sessioncast", signalMember.name.c_str()));
            return ER_INVALID_SIGNAL_EMISSION_TYPE;
        }
        if ((flags & ALLJOYN_FLAG_SESSIONLESS) && !signalMember.isSessionlessSignal) {
            QCC_LogError(ER_INVALID_SIGNAL_EMISSION_TYPE, ("Attempt to send a sessionless signal when %s is not sessionless", signalMember.name.c_str()));
            return ER_INVALID_SIGNAL_EMISSION_TYPE;
        }
        if ((destination != NULL) && !signalMember.isUnicastSignal) {
            QCC_LogError(ER_INVALID_SIGNAL_EMISSION_TYPE, ("Attempt to send a unicast signal when %s is not unicast", signalMember.name.c_str()));
            return ER_INVALID_SIGNAL_EMISSION_TYPE;
        }
        if ((flags & ALLJOYN_FLAG_GLOBAL_BROADCAST) && !signalMember.isGlobalBroadcastSignal) {
            QCC_LogError(ER_INVALID_SIGNAL_EMISSION_TYPE, ("Attempt to send a global broadcast signal when %s is not global broadcast", signalMember.name.c_str()));
            return ER_INVALID_SIGNAL_EMISSION_TYPE;
        }
    }

    /* For sessioncast signals (sessionId != 0 && destination == NULL), we need
     * to perform some tricks here:
     * - if sessionId == SESSION_ID_ALL_HOSTED, we want to emit sessioncast
     *   signals for each hosted session
     * - for individual sessioncast signals in point-to-point sessions, we fill
     *   in the destination (i.e. the other end of the session) explicitly so
     *   that Security 2.0 can perform better permission checking. This
     *   preserves the non-Sec2.0 behavior that a sessioncast signal in a
     *   point-to-point session is functionally equivalent to a unicast signal
     *   over the same session.
     */

    /* declare these in the method scope to keep the strings inside alive until the end of the method */
    std::vector<BusAttachment::Internal::Session> sessions;
    BusAttachment::Internal::Session session;

    std::vector<std::pair<SessionId, const char*> > emissionParams;
    if (sessionId == SESSION_ID_ALL_HOSTED) {
        sessions = bus->GetInternal().GetHostedSessions();
        std::vector<BusAttachment::Internal::Session>::iterator it;
        for (it = sessions.begin(); it != sessions.end(); ++it) {
            if (it->multipoint) {
                emissionParams.push_back(std::make_pair(it->id, (const char*) NULL));
            } else {
                emissionParams.push_back(std::make_pair(it->id, it->otherParticipants.begin()->c_str()));
            }
        }
    } else if (sessionId != 0 && destination == NULL) {
        QStatus status = bus->GetInternal().GetSession(sessionId, session);
        if (status == ER_OK && !session.multipoint) {
            emissionParams.push_back(std::make_pair(sessionId, session.otherParticipants.begin()->c_str()));
        } else {
            emissionParams.push_back(std::make_pair(sessionId, (const char*) NULL));
        }
    } else {
        emissionParams.push_back(std::make_pair(sessionId, destination));
    }

    if (emissionParams.empty()) {
        return ER_OK;
    }

    QStatus status = ER_FAIL;

    std::vector<std::pair<SessionId, const char*> >::iterator it;
    for (it = emissionParams.begin(); it != emissionParams.end(); ++it) {
        Message msg(*bus);
        QStatus aStatus = msg->SignalMsg(signalMember.signature,
                                         it->second,
                                         it->first,
                                         path,
                                         signalMember.iface->GetName(),
                                         signalMember.name,
                                         args,
                                         numArgs,
                                         flags,
                                         timeToLive);
        if (aStatus == ER_OK) {
            if (msg->IsEncrypted()) {
                if (authorizationCallback != NULL) {
                    aStatus = authorizationCallback->Authorize(msg);
                    /* mark the message so the Message::EncryptMessage method does
                     * not need to authorize the message again.
                     */
                    msg->authorizationChecked = true;
                } else if (((destination == NULL) || (strlen(destination) == 0)) && (sessionId == 0)) {
                    /* broadcast signal */
                    msg->authorizationChecked = true; /* skip authorization check */
                } else if (!bus->GetInternal().GetRouter().IsDaemon()) {
                    /* do an earlier permission authorization to make sure this
                     * signal is allowed to send to the router for delivery.
                     */
                    PeerState peerState = bus->GetInternal().GetPeerStateTable()->GetPeerState(msg->GetDestination());
                    aStatus = bus->GetInternal().GetPermissionManager().AuthorizeMessage(true, msg, peerState);
                    /* mark the message so the Message::EncryptMessage method does
                     * not need to authorize the message again.
                     */
                    msg->authorizationChecked = true;
                }
            }
            if (aStatus == ER_OK) {
                BusEndpoint bep = BusEndpoint::cast(bus->GetInternal().GetLocalEndpoint());
                aStatus = bus->GetInternal().GetRouter().PushMessage(msg, bep);
            }
            if ((aStatus == ER_OK) && outMsg) {
                *outMsg = msg;
            }
        }
        if (status != ER_OK) {
            /* Once status is ER_OK, it will return ER_OK */
            status = aStatus;
        }
    }
    return status;
}

QStatus BusObject::Signal(const char* destination,
                          SessionId sessionId,
                          const InterfaceDescription::Member& signalMember,
                          const MsgArg* args,
                          size_t numArgs,
                          uint16_t timeToLive,
                          uint8_t flags,
                          Message* outMsg)
{
    return SignalInternal(destination, sessionId, signalMember, args, numArgs, timeToLive, flags, outMsg, NULL);
}

QStatus BusObject::CancelSessionlessMessage(uint32_t serialNum)
{
    if (!bus) {
        return ER_BUS_OBJECT_NOT_REGISTERED;
    }

    Message reply(*bus);
    MsgArg arg("u", serialNum);
    const ProxyBusObject& alljoynObj = bus->GetAllJoynProxyObj();
    QStatus status = alljoynObj.MethodCall(org::alljoyn::Bus::InterfaceName, "CancelSessionlessMessage", &arg, 1, reply);
    if (ER_OK == status) {
        uint32_t disposition;
        status = reply->GetArgs("u", &disposition);
        if (ER_OK == status) {
            switch (disposition) {
            case ALLJOYN_CANCELSESSIONLESS_REPLY_SUCCESS:
                break;

            case ALLJOYN_CANCELSESSIONLESS_REPLY_NO_SUCH_MSG:
                status = ER_BUS_NO_SUCH_MESSAGE;
                break;

            case ALLJOYN_CANCELSESSIONLESS_REPLY_NOT_ALLOWED:
                status = ER_BUS_NOT_ALLOWED;
                break;

            case ALLJOYN_CANCELSESSIONLESS_REPLY_FAILED:
                status = ER_FAIL;
                break;

            default:
                status = ER_BUS_UNEXPECTED_DISPOSITION;
                break;
            }
        }
    }
    return status;
}

QStatus BusObject::MethodReply(const Message& msg, const MsgArg* args, size_t numArgs, Message* replyMsg)
{
    QStatus status;

    /* Protect against calling before object is registered */
    if (!bus) {
        return ER_BUS_OBJECT_NOT_REGISTERED;
    }

    if (msg->GetFlags() & ALLJOYN_FLAG_NO_REPLY_EXPECTED) {
        /* no reply expected, so we don't send any either */
        return ER_OK;
    }

    if (msg->GetType() != MESSAGE_METHOD_CALL) {
        status = ER_BUS_NO_CALL_FOR_REPLY;
    } else {
        Message reply(*bus);
        status = reply->ReplyMsg(msg, args, numArgs);
        if (status == ER_OK) {
            BusEndpoint bep = BusEndpoint::cast(bus->GetInternal().GetLocalEndpoint());
            status = bus->GetInternal().GetRouter().PushMessage(reply, bep);
        }
        if (NULL != replyMsg) {
            *replyMsg = reply;
        }
    }
    return status;
}

QStatus BusObject::MethodReply(const Message& msg, const char* errorName, const char* errorMessage, Message* replyMsg)
{
    QStatus status;

    /* Protect against calling before object is registered */
    if (!bus) {
        return ER_BUS_OBJECT_NOT_REGISTERED;
    }

    if (msg->GetFlags() & ALLJOYN_FLAG_NO_REPLY_EXPECTED) {
        /* no reply expected, so we don't send any either */
        return ER_OK;
    }

    if (msg->GetType() != MESSAGE_METHOD_CALL) {
        status = ER_BUS_NO_CALL_FOR_REPLY;
        return status;
    } else {
        Message error(*bus);
        status = error->ErrorMsg(msg, errorName, errorMessage ? errorMessage : "");
        if (status == ER_OK) {
            BusEndpoint bep = BusEndpoint::cast(bus->GetInternal().GetLocalEndpoint());
            status = bus->GetInternal().GetRouter().PushMessage(error, bep);
        }
        if (NULL != replyMsg) {
            *replyMsg = error;
        }
    }
    return status;
}

QStatus BusObject::MethodReply(const Message& msg, QStatus status, Message* replyMsg)
{
    /* Protect against calling before object is registered */
    if (!bus) {
        return ER_BUS_OBJECT_NOT_REGISTERED;
    }

    if (msg->GetFlags() & ALLJOYN_FLAG_NO_REPLY_EXPECTED) {
        /* no reply expected, so we don't send any either */
        return ER_OK;
    }

    if (status == ER_OK) {
        /* Casts needed to unambiguously call the correct overload of MethodReply. */
        return MethodReply(msg, (const MsgArg*)NULL, (size_t)0, replyMsg);
    } else {
        if (msg->GetType() != MESSAGE_METHOD_CALL) {
            return ER_BUS_NO_CALL_FOR_REPLY;
        } else {
            Message error(*bus);
            QStatus result = error->ErrorMsg(msg, status);
            QCC_ASSERT(ER_OK == result);
            QCC_UNUSED(result);
            if (NULL != replyMsg) {
                *replyMsg = error;
            }
            BusEndpoint bep = BusEndpoint::cast(bus->GetInternal().GetLocalEndpoint());
            return bus->GetInternal().GetRouter().PushMessage(error, bep);
        }
    }
}

void BusObject::AddChild(BusObject& child)
{
    QCC_DbgPrintf(("AddChild %s to object with path = \"%s\"", child.GetPath(), GetPath()));
    child.parent = this;
    components->children.push_back(&child);
}

QStatus BusObject::RemoveChild(BusObject& child)
{
    QStatus status = ER_BUS_NO_SUCH_OBJECT;
    vector<BusObject*>::iterator it = find(components->children.begin(), components->children.end(), &child);
    if (it != components->children.end()) {
        QCC_ASSERT(*it == &child);
        child.parent = NULL;
        QCC_DbgPrintf(("RemoveChild %s from object with path = \"%s\"", child.GetPath(), GetPath()));
        components->children.erase(it);
        status = ER_OK;
    }
    return status;
}

BusObject* BusObject::RemoveChild()
{
    size_t sz = components->children.size();
    if (sz > 0) {
        BusObject* child = components->children[sz - 1];
        components->children.pop_back();
        QCC_DbgPrintf(("RemoveChild %s from object with path = \"%s\"", child->GetPath(), GetPath()));
        child->parent = NULL;
        return child;
    } else {
        return NULL;
    }
}

void BusObject::Replace(BusObject& object)
{
    QCC_DbgPrintf(("Replacing object with path = \"%s\"", GetPath()));
    object.components->children = components->children;
    vector<BusObject*>::iterator it = object.components->children.begin();
    while (it != object.components->children.end()) {
        (*it++)->parent = &object;
    }
    if (parent) {
        vector<BusObject*>::iterator pit = parent->components->children.begin();
        while (pit != parent->components->children.end()) {
            if ((*pit) == this) {
                parent->components->children.erase(pit);
                break;
            }
            ++pit;
        }
    }
    components->children.clear();
}

void BusObject::InUseIncrement() {
    components->counterLock.Lock(MUTEX_CONTEXT);
    qcc::IncrementAndFetch(&(components->inUseCounter));
    components->counterLock.Unlock(MUTEX_CONTEXT);
}

void BusObject::InUseDecrement() {
    components->counterLock.Lock(MUTEX_CONTEXT);
    qcc::DecrementAndFetch(&(components->inUseCounter));
    components->counterLock.Unlock(MUTEX_CONTEXT);
}

BusObject::BusObject(BusAttachment& bus, const char* path, bool isPlaceholder) :
    bus(&bus),
    components(new Components),
    path(path),
    parent(NULL),
    isRegistered(false),
    isPlaceholder(isPlaceholder),
    isSecure(false),
    languageTag(),
    description(),
    translator(NULL)
{
    components->inUseCounter = 0;
}

BusObject::BusObject(const char* path, bool isPlaceholder) :
    bus(0),
    components(new Components),
    path(path),
    parent(NULL),
    isRegistered(false),
    isPlaceholder(isPlaceholder),
    isSecure(false),
    languageTag(),
    description(),
    translator(NULL)
{
    components->inUseCounter = 0;
}

BusObject::~BusObject()
{
    components->counterLock.Lock(MUTEX_CONTEXT);
    while (components->inUseCounter != 0) {
        components->counterLock.Unlock(MUTEX_CONTEXT);
        qcc::Sleep(5);
        components->counterLock.Lock(MUTEX_CONTEXT);
    }
    components->counterLock.Unlock(MUTEX_CONTEXT);

    QCC_DbgPrintf(("BusObject destructor for object with path = \"%s\"", GetPath()));
    /*
     * If this object has a parent it has not been unregistered so do so now.
     */
    if (bus && parent) {
        bus->GetInternal().GetLocalEndpoint()->UnregisterBusObject(*this);
    }
    delete components;
}


void BusObject::SetDescription(const char* language, const char* text)
{
    languageTag.assign(language);
    description.assign(text);
}

const char* BusObject::GetDescription(const char* toLanguage, qcc::String& buffer) const
{
    Translator* myTranslator = translator;
    if (!myTranslator && bus) {
        myTranslator = bus->GetDescriptionTranslator();
    }

    if (myTranslator) {
        qcc::String bestLanguage;
        myTranslator->GetBestLanguage(toLanguage, languageTag, bestLanguage);
        const char* ret = myTranslator->Translate(languageTag.c_str(), bestLanguage.c_str(), description.c_str(), buffer);
        if (ret) {
            return ret;
        }
    }

    if (!description.empty() && !languageTag.empty()) {
        return description.c_str();
    }

    return NULL;
}

void BusObject::IntrospectWithDescription(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);

    qcc::String buffer;
    char* langTag;
    msg->GetArgs("s", &langTag);

    qcc::String xml = org::allseen::Introspectable::IntrospectDocType;

    xml += "<node>\n";
    const char* desc = GetDescription(langTag, buffer);
    if (desc) {
        xml += qcc::String("  <description>") + XmlElement::EscapeXml(desc) + "</description>\n";
    }
    if (isSecure) {
        xml += "  <annotation name=\"org.alljoyn.Bus.Secure\" value=\"true\"/>\n";
    }

    xml += GenerateIntrospection(langTag, false, 2);
    xml += "</node>\n";
    MsgArg arg("s", xml.c_str());
    QStatus status = MethodReply(msg, &arg, 1);
    if (status != ER_OK) {
        QCC_DbgPrintf(("IntrospectWithDescription %s", QCC_StatusText(status)));
    }
}

void mergeTranslationLanguages(Translator* t, std::set<qcc::String>& langs)
{
    size_t numLangs = t->NumTargetLanguages();
    for (size_t i = 0; i < numLangs; i++) {
        qcc::String s;
        t->GetTargetLanguage(i, s);
        langs.insert(s);
    }
}

void BusObject::GetDescriptionLanguages(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);

    std::set<qcc::String> langs;
    bool hasDescription = false;
    bool someoneHasNoTranslator = false;

    // First merge this object's languages...
    if (!languageTag.empty()) {
        langs.insert(languageTag);
        hasDescription = true;

        if (translator) {
            mergeTranslationLanguages(translator, langs);
        } else {
            someoneHasNoTranslator = true;
        }
    }

    //...then add the languages of all this object's interfaces...
    vector<pair<const InterfaceDescription*, bool> >::const_iterator itIf = components->ifaces.begin();
    for (; itIf != components->ifaces.end(); itIf++) {
        if (!(itIf->first)->HasDescription()) {
            continue;
        }

        hasDescription = true;

        const char* lang = (itIf->first)->GetDescriptionLanguage();
        if (lang && lang[0]) {
            langs.insert(qcc::String(lang));
        }

        Translator* ifTranslator = (itIf->first)->GetDescriptionTranslator();
        if (ifTranslator) {
            mergeTranslationLanguages(ifTranslator, langs);
        } else if (!someoneHasNoTranslator) {
            someoneHasNoTranslator = true;
        }
    }

    //...finally, if this object or one of its interfaces does not have a Translator,
    //add the global Translator's languages
    if (hasDescription && someoneHasNoTranslator) {
        Translator* globalTranslator = bus ? bus->GetDescriptionTranslator() : NULL;
        if (globalTranslator) {
            mergeTranslationLanguages(globalTranslator, langs);
        }
    }

    vector<const char*> tags;
    std::set<qcc::String>::const_iterator it = langs.begin();
    for (; it != langs.end(); it++) {
        char* str = new char[it->size() + 1];
        strcpy(str, it->c_str());
        tags.push_back(str);
    }

    MsgArg replyArg[1];
    if (tags.empty()) {
        replyArg[0].Set("as", 0, NULL);
    } else {
        replyArg[0].Set("as", tags.size(), &tags[0]);
    }
    replyArg[0].SetOwnershipFlags(MsgArg::OwnsData | MsgArg::OwnsArgs, true);
    QStatus status = MethodReply(msg, replyArg, 1);
    if (status != ER_OK) {
        QCC_DbgPrintf(("GetDescriptionLanguages %s", QCC_StatusText(status)));
    }
}

void BusObject::SetDescriptionTranslator(Translator* newTranslator)
{
    this->translator = newTranslator;
}

size_t BusObject::GetAnnouncedInterfaceNames(const char** interfaces, size_t numInterfaces)
{
    size_t retCount = 0;
    for (size_t i = 0; i < components->ifaces.size(); ++i) {
        if (components->ifaces[i].second == true) {
            if (retCount < numInterfaces) {
                interfaces[retCount] = components->ifaces[i].first->GetName();
            }
            ++retCount;
        }
    }
    return retCount;
}

QStatus BusObject::SetAnnounceFlag(const InterfaceDescription* iface, AnnounceFlag isAnnounced) {
    for (size_t i = 0; i < components->ifaces.size(); ++i) {
        if (iface == components->ifaces[i].first) {
            components->ifaces[i].second = isAnnounced;
            return ER_OK;
        }
    }
    return ER_BUS_OBJECT_NO_SUCH_INTERFACE;
}

}
