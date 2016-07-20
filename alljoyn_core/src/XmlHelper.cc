/**
 * @file
 *
 * This file implements the XMlHelper class.
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


#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/XmlElement.h>

#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/Message.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/InterfaceDescription.h>

#include "BusUtil.h"
#include "XmlHelper.h"
#include "SignatureUtils.h"

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN"

using namespace qcc;
using namespace std;

namespace ajn {

const qcc::String& GetSecureAnnotation(const XmlElement* elem)
{
    vector<XmlElement*>::const_iterator ifIt = elem->GetChildren().begin();
    while (ifIt != elem->GetChildren().end()) {
        const XmlElement* ifChildElem = *ifIt++;
        qcc::String ifChildName = ifChildElem->GetName();
        if ((ifChildName == "annotation") && (ifChildElem->GetAttribute("name") == org::alljoyn::Bus::Secure)) {
            return ifChildElem->GetAttribute("value");
        }
    }
    return qcc::String::Empty;
}

QStatus XmlHelper::ParseInterface(const XmlElement* elem, InterfaceDescription& interface)
{
    QStatus status = ER_OK;
    InterfaceSecurityPolicy secPolicy;
    const qcc::String docString("org.alljoyn.Bus.DocString");

    QCC_ASSERT(elem->GetName() == "interface");

    qcc::String ifName = elem->GetAttribute("name");
    if (!IsLegalInterfaceName(ifName.c_str())) {
        status = ER_BUS_BAD_INTERFACE_NAME;
        QCC_LogError(status, ("Invalid interface name \"%s\" in XML introspection data for %s", ifName.c_str(), ident));
        return status;
    }

    /*
     * Due to a bug in AllJoyn 14.06 and previous, we need to ignore
     * the introspected versions of the standard D-Bus interfaces.
     * This will allow a maximal level of interoperability between
     * this code and 14.06.  This hack should be removed when
     * interface evolution is better supported.
     */
    if ((ifName == org::freedesktop::DBus::InterfaceName) ||
        (ifName == org::freedesktop::DBus::Properties::InterfaceName)) {
        return ER_OK;
    }

    /*
     * Security on an interface can be "true", "inherit", or "off"
     * Security is implicitly off on the standard DBus interfaces.
     */
    qcc::String sec = GetSecureAnnotation(elem);
    if (sec == "true") {
        secPolicy = AJ_IFC_SECURITY_REQUIRED;
    } else if ((sec == "off") || (ifName.find(org::freedesktop::DBus::InterfaceName) == 0)) {
        secPolicy = AJ_IFC_SECURITY_OFF;
    } else {
        if ((sec != qcc::String::Empty) && (sec != "inherit")) {
            QCC_DbgHLPrintf(("Unknown annotation %s is defaulting to 'inherit'. Valid values: 'true', 'inherit', or 'off'.", org::alljoyn::Bus::Secure));
        }
        secPolicy = AJ_IFC_SECURITY_INHERIT;
    }

    interface.SetName(ifName);
    interface.SetSecurityPolicy(secPolicy);

    /* Iterate over <method>, <signal> and <property> elements */
    vector<XmlElement*>::const_iterator ifIt = elem->GetChildren().begin();
    while ((ER_OK == status) && (ifIt != elem->GetChildren().end())) {
        const XmlElement* ifChildElem = *ifIt++;
        const qcc::String& ifChildName = ifChildElem->GetName();
        const qcc::String& memberName = ifChildElem->GetAttribute("name");
        if ((ifChildName == "method") || (ifChildName == "signal")) {
            if (IsLegalMemberName(memberName.c_str())) {

                bool isMethod = (ifChildName == "method");
                bool isSignal = (ifChildName == "signal");
                bool isFirstArg = true;
                qcc::String inSig;
                qcc::String outSig;
                qcc::String argNames;
                bool isArgNamesEmpty = true;
                std::map<String, String> annotations;
                bool isSessioncastSignal = false;
                bool isSessionlessSignal = false;
                bool isUnicastSignal = false;
                bool isGlobalBroadcastSignal = false;
                const vector<XmlElement*>& argChildren = ifChildElem->GetChildren();
                vector<XmlElement*>::const_iterator argIt = argChildren.begin();
                map<pair<qcc::String, qcc::String>, qcc::String> argAnnotations;

                if (isSignal) {
                    const qcc::String& sessioncastStr = ifChildElem->GetAttribute("sessioncast");
                    isSessioncastSignal = (sessioncastStr == "true");
                    const qcc::String& sessionlessStr = ifChildElem->GetAttribute("sessionless");
                    isSessionlessSignal = (sessionlessStr == "true");
                    const qcc::String& unicastStr = ifChildElem->GetAttribute("unicast");
                    isUnicastSignal = (unicastStr == "true");
                    const qcc::String& globalBroadcastStr = ifChildElem->GetAttribute("globalbroadcast");
                    isGlobalBroadcastSignal = (globalBroadcastStr == "true");
                }

                /* Iterate over member children */
                while ((ER_OK == status) && (argIt != argChildren.end())) {
                    const XmlElement* argElem = *argIt++;
                    if (argElem->GetName() == "arg") {
                        if (!isFirstArg) {
                            argNames += ',';
                        }
                        isFirstArg = false;
                        const qcc::String& typeAtt = argElem->GetAttribute("type");

                        if (typeAtt.empty()) {
                            status = ER_BUS_BAD_XML;
                            QCC_LogError(status, ("Malformed <arg> tag (bad attributes)"));
                            break;
                        }

                        const qcc::String& nameAtt = argElem->GetAttribute("name");
                        if (!nameAtt.empty()) {
                            isArgNamesEmpty = false;
                            argNames += nameAtt;

                            /* Add argument annotations and descriptions */
                            vector<XmlElement*>::const_iterator it = argElem->GetChildren().begin();
                            for (; it != argElem->GetChildren().end(); it++) {
                                if ((*it)->GetName() == "annotation") {
                                    const qcc::String& argNameAtt = (*it)->GetAttribute("name");
                                    const qcc::String& argValueAtt = (*it)->GetAttribute("value");
                                    pair<qcc::String, qcc::String> item(nameAtt, argNameAtt);
                                    argAnnotations[item] = argValueAtt;
                                } else if ((*it)->GetName() == "description") {
                                    const qcc::String& desc = (*it)->GetContent();
                                    const qcc::String& lang = (*it)->GetAttribute("language");
                                    /* Add argument description as annotation. */
                                    if (!lang.empty()) {
                                        const pair<qcc::String, qcc::String> item(nameAtt, docString + "." + lang);
                                        argAnnotations[item] = desc;
                                    }
                                }
                            }
                        }

                        if (isSignal || (argElem->GetAttribute("direction") == "in")) {
                            inSig += typeAtt;
                        } else {
                            outSig += typeAtt;
                        }
                    } else if (argElem->GetName() == "annotation") {
                        const qcc::String& nameAtt = argElem->GetAttribute("name");
                        const qcc::String& valueAtt = argElem->GetAttribute("value");
                        annotations[nameAtt] = valueAtt;

                        /* Unified XML signal emission behaviors */
                        if (isSignal) {
                            if (nameAtt == "org.alljoyn.Bus.Signal.Sessionless") {
                                isSessionlessSignal = (valueAtt == "true");
                            } else if (nameAtt == "org.alljoyn.Bus.Signal.Sessioncast") {
                                isSessioncastSignal = (valueAtt == "true");
                            } else if (nameAtt == "org.alljoyn.Bus.Signal.Unicast") {
                                isUnicastSignal = (valueAtt == "true");
                            } else if (nameAtt == "org.alljoyn.Bus.Signal.GlobalBroadcast") {
                                isGlobalBroadcastSignal = (valueAtt == "true");
                            }
                        }
                    } else if (argElem->GetName() == "description") {
                        const qcc::String& memberDescription = argElem->GetContent();
                        /* Add description as annotation */
                        const qcc::String& lang = argElem->GetAttribute("language");
                        if (!lang.empty()) {
                            const qcc::String nameAtt = docString + "." + lang;
                            annotations[nameAtt] = memberDescription;
                        }
                    }
                }

                /* Add the member */
                if ((ER_OK == status) && (isMethod || isSignal)) {
                    uint8_t annotation = 0;
                    if (isSessioncastSignal) {
                        annotation |= MEMBER_ANNOTATE_SESSIONCAST;
                    }
                    if (isSessionlessSignal) {
                        annotation |= MEMBER_ANNOTATE_SESSIONLESS;
                    }
                    if (isUnicastSignal) {
                        annotation |= MEMBER_ANNOTATE_UNICAST;
                    }
                    if (isGlobalBroadcastSignal) {
                        annotation |= MEMBER_ANNOTATE_GLOBAL_BROADCAST;
                    }
                    status = interface.AddMember(isMethod ? MESSAGE_METHOD_CALL : MESSAGE_SIGNAL,
                                                 memberName.c_str(),
                                                 inSig.c_str(),
                                                 outSig.c_str(),
                                                 isArgNamesEmpty ? NULL : argNames.c_str(),
                                                 annotation);

                    for (std::map<String, String>::const_iterator it = annotations.begin(); it != annotations.end(); ++it) {
                        interface.AddMemberAnnotation(memberName.c_str(), it->first, it->second);
                    }

                    for (std::map<std::pair<qcc::String, qcc::String>, qcc::String>::const_iterator it = argAnnotations.begin(); it != argAnnotations.end(); it++) {
                        interface.AddArgAnnotation(memberName.c_str(), it->first.first.c_str(), it->first.second, it->second);
                    }
                }
            } else {
                status = ER_BUS_BAD_MEMBER_NAME;
                QCC_LogError(status, ("Illegal member name \"%s\" introspection data for %s", memberName.c_str(), ident));
            }
        } else if (ifChildName == "property") {
            const qcc::String& sig = ifChildElem->GetAttribute("type");
            const qcc::String& accessStr = ifChildElem->GetAttribute("access");
            if (!SignatureUtils::IsCompleteType(sig.c_str())) {
                status = ER_BUS_BAD_SIGNATURE;
                QCC_LogError(status, ("Invalid signature for property %s in introspection data from %s", memberName.c_str(), ident));
            } else if (memberName.empty()) {
                status = ER_BUS_BAD_BUS_NAME;
                QCC_LogError(status, ("Invalid name attribute for property in introspection data from %s", ident));
            } else {
                uint8_t access = 0;
                if (accessStr == "read") {
                    access = PROP_ACCESS_READ;
                }
                if (accessStr == "write") {
                    access = PROP_ACCESS_WRITE;
                }
                if (accessStr == "readwrite") {
                    access = PROP_ACCESS_RW;
                }
                status = interface.AddProperty(memberName.c_str(), sig.c_str(), access);

                // add Property annotations and descriptions
                const vector<XmlElement*>& argChildren = ifChildElem->GetChildren();
                vector<XmlElement*>::const_iterator argIt = argChildren.begin();
                while ((ER_OK == status) && (argIt != argChildren.end())) {
                    const XmlElement* argElem = *argIt++;
                    if (argElem->GetName() == "annotation") {
                        status = interface.AddPropertyAnnotation(memberName, argElem->GetAttribute("name"), argElem->GetAttribute("value"));
                    } else if (argElem->GetName() == "description") {
                        const qcc::String& content = argElem->GetContent();
                        const qcc::String& lang = argElem->GetAttribute("language");
                        if (!lang.empty()) {
                            interface.AddPropertyAnnotation(memberName, docString + "." + lang, content);
                        }
                    }
                }
            }
        } else if (ifChildName == "annotation") {
            const qcc::String& nameAtt = ifChildElem->GetAttribute("name");
            const qcc::String& valueAtt = ifChildElem->GetAttribute("value");
            status = interface.AddAnnotation(nameAtt, valueAtt);
        } else if (ifChildName == "description") {
            qcc::String const& description = ifChildElem->GetContent();
            qcc::String const& language = ifChildElem->GetAttribute("language");
            if (!language.empty()) {
                /* Add description as annotation */
                const qcc::String nameAtt = docString + "." + language;
                interface.AddAnnotation(nameAtt, description);
            }
        } else {
            status = ER_FAIL;
            QCC_LogError(status, ("Unknown element \"%s\" found in introspection data from %s", ifChildName.c_str(), ident));
            break;
        }
    }

    return status;
}

QStatus XmlHelper::ParseNode(const XmlElement* root, ProxyBusObject* obj, const ProxyBusObject::XmlToLanguageMap* legacyDescriptions)
{
    QStatus status = ER_OK;

    QCC_ASSERT(root->GetName() == "node");

    if (GetSecureAnnotation(root) == "true") {
        if (obj) {
            obj->SetSecure(true);
        }
    }
    /* Iterate over <interface> and <node> elements */
    const vector<XmlElement*>& rootChildren = root->GetChildren();
    vector<XmlElement*>::const_iterator it = rootChildren.begin();
    while ((ER_OK == status) && (it != rootChildren.end())) {
        const XmlElement* elem = *it++;
        const qcc::String& elemName = elem->GetName();
        if (elemName == "interface") {
            InterfaceDescription interface;
            status = ParseInterface(elem, interface);
            if (status == ER_OK && legacyDescriptions != nullptr && !legacyDescriptions->empty()) {
                /* Legacy descriptions present. This means that we are dealing with a pre-16.04
                 * object and we need to get the descriptions for this interface from the
                 * legacyDescriptions map. See ASACORE-2744 and the documentation for
                 * ajn::ProxyBusObject::ParseLegacyXml().
                 */
                status = AddLegacyDescriptions(interface, legacyDescriptions);
                if (status != ER_OK) {
                    QCC_LogError(status, ("Failed to add legacy descriptions for interface \"%s\"",
                                          interface.GetName()));
                }
            }
            if (status == ER_OK) {
                status = AddInterface(interface, obj);
            }
        } else if (elemName == "node") {
            if (obj) {
                const qcc::String& relativePath = elem->GetAttribute("name");
                qcc::String childObjPath = obj->GetPath();
                if (0 || childObjPath.size() > 1) {
                    childObjPath += '/';
                }
                childObjPath += relativePath;
                if (!relativePath.empty() && IsLegalObjectPath(childObjPath.c_str())) {
                    /* Check for existing child with the same name. Use this child if found, otherwise create a new one */
                    ProxyBusObject* childObj = obj->GetChild(relativePath.c_str());
                    if (childObj) {
                        status = ParseNode(elem, childObj, legacyDescriptions);
                    } else {
                        ProxyBusObject newChild(*bus, obj->GetServiceName().c_str(), obj->GetUniqueName().c_str(), childObjPath.c_str(), obj->GetSessionId(), obj->IsSecure());
                        status = ParseNode(elem, &newChild, legacyDescriptions);
                        if (ER_OK == status) {
                            obj->AddChild(newChild);
                        }
                    }
                    if (status != ER_OK) {
                        QCC_LogError(status, ("Failed to parse child object %s in introspection data for %s", childObjPath.c_str(), ident));
                    }
                } else {
                    status = ER_FAIL;
                    QCC_LogError(status, ("Illegal child object name \"%s\" specified in introspection for %s", relativePath.c_str(), ident));
                }
            } else {
                status = ParseNode(elem, nullptr, legacyDescriptions);
            }
        }
    }
    return status;
}

QStatus XmlHelper::AddInterface(const InterfaceDescription& interface, ProxyBusObject* obj)
{
    QStatus status;
    InterfaceDescription* newIntf = nullptr;

    status = bus->CreateInterface(interface.GetName(), newIntf);
    if (status == ER_OK) {
        /* Assign new interface */
        *newIntf = interface;
        newIntf->Activate();
        if (obj) {
            obj->AddInterface(*newIntf);
        }
    } else if (ER_BUS_IFACE_ALREADY_EXISTS == status) {
        /* Make sure definition matches existing one */
        const InterfaceDescription* existingIntf = bus->GetInterface(interface.GetName());
        if (existingIntf) {
            if (*existingIntf == interface) {
                if (obj) {
                    obj->AddInterface(*existingIntf);
                }
                status = ER_OK;
            } else {
                status = ER_BUS_INTERFACE_MISMATCH;
                QCC_LogError(status, ("XML interface does not match existing definition for \"%s\"", interface.GetName()));
            }
        } else {
            status = ER_FAIL;
            QCC_LogError(status, ("Failed to retrieve existing interface \"%s\"", interface.GetName()));
        }
    } else {
        QCC_LogError(status, ("Failed to create new interface \"%s\"", interface.GetName()));
    }

    return status;
}

QStatus XmlHelper::AddLegacyDescriptions(InterfaceDescription& interface,
                                         const ProxyBusObject::XmlToLanguageMap* legacyDescriptions) const
{
    QStatus status;
    if (legacyDescriptions == nullptr) {
        status = ER_INVALID_ADDRESS;
        QCC_LogError(status, ("Legacy descriptions not available."));
        return status;
    }

    for (ProxyBusObject::XmlToLanguageMap::const_iterator itD = legacyDescriptions->begin();
         itD != legacyDescriptions->end();
         itD++) {
        const qcc::String& language = itD->first;
        const qcc::XmlParseContext& xmlWithDescriptions = *(itD->second);

        /* Find the interface in the XML with descriptions to parse descriptions for the interface
         * and all its children.
         */
        const qcc::XmlElement* interfaceElement = FindInterfaceElement(xmlWithDescriptions.GetRoot(), interface.GetName());
        if (interfaceElement == nullptr) {
            /* No description in this language for this interface - OK, continue.
             */
            continue;
        }

        status = AddLegacyDescriptionsForLanguage(interface, language, interfaceElement);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to add legacy description in language \"%s\" for interface \"%s\".",
                                  language.c_str(), interface.GetName()));
            return ER_BUS_BAD_XML;
        }
    }
    return ER_OK;
}

QStatus XmlHelper::AddLegacyDescriptionsForLanguage(InterfaceDescription& interface,
                                                    const qcc::String& languageTag,
                                                    const qcc::XmlElement* interfaceElement) const
{
    QStatus status = ER_OK;
    QCC_ASSERT(languageTag.size() > 0u);
    QCC_ASSERT(interfaceElement);

    vector<XmlElement*>::const_iterator itCh = interfaceElement->GetChildren().begin();
    while ((status == ER_OK) && (itCh != interfaceElement->GetChildren().end())) {
        const XmlElement* ifChildElem = *itCh;
        const qcc::String& ifChildName = ifChildElem->GetName();

        if (ifChildName == "description") {
            const qcc::String& description = ifChildElem->GetContent();
            status = SetDescriptionForInterface(interface, description, languageTag);
            if (status != ER_OK) {
                return status;
            }
        } else if ((ifChildName == "method") || (ifChildName == "signal")) {
            status = AddLegacyMemberDescriptions(interface, languageTag, ifChildElem);
            if (status != ER_OK) {
                return status;
            }
        } else if (ifChildName == "property") {
            status = AddLegacyPropertyDescription(interface, languageTag, ifChildElem);
            if (status != ER_OK) {
                return status;
            }
        } else if (ifChildName != "annotation") {
            status = ER_FAIL;
            QCC_LogError(status, ("Unknown element \"%s\" found in introspection data from %s", ifChildName.c_str(), ident));
            return status;
        }
        itCh++;
    }
    return status;
}

QStatus XmlHelper::AddLegacyMemberDescriptions(InterfaceDescription& interface,
                                               const qcc::String& languageTag,
                                               const qcc::XmlElement* memberElem) const
{
    QStatus status = ER_OK;
    const qcc::String& memberName = memberElem->GetAttribute("name");
    if (!IsLegalMemberName(memberName.c_str())) {
        status = ER_BUS_BAD_MEMBER_NAME;
        QCC_LogError(status, ("Illegal member name \"%s\" introspection data for %s", memberName.c_str(), ident));
        return status;
    }

    const vector<XmlElement*>& memberChildren = memberElem->GetChildren();
    vector<XmlElement*>::const_iterator itCh = memberChildren.begin();

    while ((status == ER_OK) && (itCh != memberChildren.end())) {
        const XmlElement* memberChildElem = *itCh;
        const qcc::String& memberChildName = memberChildElem->GetName();
        if (memberChildName == "description") {
            const qcc::String& description = memberChildElem->GetContent();
            status = SetMemberDescriptionForInterface(interface, memberName, description, languageTag);
            if (status != ER_OK) {
                return status;
            }
        } else if (memberChildName == "arg") {
            status = AddLegacyArgDescription(interface, languageTag, memberName, memberChildElem);
            if (status != ER_OK) {
                return status;
            }
        }
        itCh++;
    }

    return status;
}

QStatus XmlHelper::AddLegacyArgDescription(InterfaceDescription& interface,
                                           const qcc::String& languageTag,
                                           const qcc::String& parentName,
                                           const qcc::XmlElement* argElem) const
{
    QStatus status;
    const qcc::String& argType = argElem->GetAttribute("type");
    if (argType.empty()) {
        status = ER_BUS_BAD_XML;
        QCC_LogError(status, ("Malformed <arg> tag (bad attributes)"));
        return status;
    }
    const qcc::String& argName = argElem->GetAttribute("name");
    if (argName.empty()) {
        status = ER_BUS_BAD_BUS_NAME;
        QCC_LogError(status, ("Invalid name attribute for argument in introspection data from %s", ident));
        return status;
    }

    vector<XmlElement*>::const_iterator itArgCh = argElem->GetChildren().begin();
    for (; itArgCh != argElem->GetChildren().end(); itArgCh++) {
        if ((*itArgCh)->GetName() == "description") {
            const qcc::String& description = (*itArgCh)->GetContent();
            return SetArgDescriptionForInterface(interface, parentName, argName, description, languageTag);
        }
    }
    return ER_OK;
}

QStatus XmlHelper::AddLegacyPropertyDescription(InterfaceDescription& interface,
                                                const qcc::String& languageTag,
                                                const qcc::XmlElement* propertyElem) const
{
    QStatus status;
    const qcc::String& sig = propertyElem->GetAttribute("type");
    const qcc::String& propertyName = propertyElem->GetAttribute("name");
    if (!SignatureUtils::IsCompleteType(sig.c_str())) {
        status = ER_BUS_BAD_SIGNATURE;
        QCC_LogError(status, ("Invalid signature for property %s in introspection data from %s",
                              propertyName.empty() ? "(Undefined name)" : propertyName.c_str(), ident));
        return status;
    }
    if (propertyName.empty()) {
        status = ER_BUS_BAD_BUS_NAME;
        QCC_LogError(status, ("Invalid name attribute for property in introspection data from %s", ident));
        return status;
    }

    const vector<XmlElement*>& propertyChildren = propertyElem->GetChildren();
    vector<XmlElement*>::const_iterator itCh = propertyChildren.begin();
    while (itCh != propertyChildren.end()) {
        const XmlElement* propertyChildElem = *itCh++;
        if (propertyChildElem->GetName() == "description") {
            const qcc::String& description = propertyChildElem->GetContent();
            return SetPropertyDescriptionForInterface(interface, propertyName, description, languageTag);
        }
    }
    return ER_OK;
}

QStatus XmlHelper::SetDescriptionForInterface(InterfaceDescription& interface,
                                              const qcc::String& description,
                                              const qcc::String& languageTag) const
{
    QStatus status = interface.SetDescriptionForLanguage(description, languageTag);
    if (status != ER_OK && status != ER_BUS_DESCRIPTION_ALREADY_EXISTS) {
        QCC_LogError(status, ("Failed to set description for interface \"%s\"",
                              interface.GetName()));
        return status;
    }

    return ER_OK;
}

QStatus XmlHelper::SetMemberDescriptionForInterface(InterfaceDescription& interface,
                                                    const qcc::String& memberName,
                                                    const qcc::String& description,
                                                    const qcc::String& languageTag) const
{
    QStatus status = interface.SetMemberDescriptionForLanguage(memberName, description, languageTag);
    if (status != ER_OK && status != ER_BUS_DESCRIPTION_ALREADY_EXISTS) {
        QCC_LogError(status, ("Failed to set description for member \"%s\" of interface \"%s\"",
                              memberName.c_str(), interface.GetName()));
        return status;
    }

    return ER_OK;
}

QStatus XmlHelper::SetArgDescriptionForInterface(InterfaceDescription& interface,
                                                 const qcc::String& parentName,
                                                 const qcc::String& argName,
                                                 const qcc::String& description,
                                                 const qcc::String& languageTag) const
{
    QStatus status = interface.SetArgDescriptionForLanguage(parentName, argName, description, languageTag);
    if (status != ER_OK && status != ER_BUS_DESCRIPTION_ALREADY_EXISTS) {
        QCC_LogError(status, ("Failed to set description for argument \"%s\" of member \"%s\" of interface \"%s\"",
                              argName.c_str(), parentName.c_str(), interface.GetName()));
        return status;
    }

    return ER_OK;
}

QStatus XmlHelper::SetPropertyDescriptionForInterface(InterfaceDescription& interface,
                                                      const qcc::String& propertyName,
                                                      const qcc::String& description,
                                                      const qcc::String& languageTag) const
{
    QStatus status = interface.SetPropertyDescriptionForLanguage(propertyName, description, languageTag);
    if (status != ER_OK && status != ER_BUS_DESCRIPTION_ALREADY_EXISTS) {
        QCC_LogError(status, ("Failed to set description for property \"%s\" of interface \"%s\"",
                              propertyName.c_str(), interface.GetName()));
        return status;
    }

    return ER_OK;
}

const qcc::XmlElement* XmlHelper::FindInterfaceElement(const qcc::XmlElement* root, const qcc::String& interfaceName) const
{
    QCC_ASSERT(root->GetName() == "node");

    const vector<XmlElement*>& rootChildren = root->GetChildren();
    for (std::vector<XmlElement*>::const_iterator itCh = rootChildren.begin();
         itCh != rootChildren.end(); itCh++) {
        const XmlElement* elem = *itCh;
        const qcc::String& elemName = elem->GetName();
        if (elemName == "interface") {
            if (elem->GetAttribute("name") == interfaceName) {
                return elem;
            }
        } else if (elemName == "node") {
            const XmlElement* interfaceInNode = FindInterfaceElement(elem, interfaceName);
            if (interfaceInNode != nullptr) {
                return interfaceInNode;
            }
        }
    }
    return nullptr;
}

} // ajn::
