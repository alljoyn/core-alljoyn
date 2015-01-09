/**
 * @file
 *
 * This file implements the XMlHelper class.
 */

/******************************************************************************
 * Copyright (c) 2009-2014 AllSeen Alliance. All rights reserved.
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

bool GetDescription(const XmlElement* elem, qcc::String& description)
{
    vector<XmlElement*>::const_iterator it = elem->GetChildren().begin();
    for (; it != elem->GetChildren().end(); it++) {
        if ((*it)->GetName().compare("description")) {
            description = (*it)->GetContent();
            return true;
        }
    }

    return false;
}

QStatus XmlHelper::ParseInterface(const XmlElement* elem, ProxyBusObject* obj)
{
    QStatus status = ER_OK;
    InterfaceSecurityPolicy secPolicy;

    assert(elem->GetName() == "interface");

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

    /* Create a new interface */
    InterfaceDescription intf(ifName.c_str(), secPolicy);

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
                bool isSessionlessSignal = false;

                /* Iterate over member children */
                const vector<XmlElement*>& argChildren = ifChildElem->GetChildren();
                vector<XmlElement*>::const_iterator argIt = argChildren.begin();
                map<qcc::String, qcc::String> argDescriptions;
                qcc::String memberDescription;
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

                            qcc::String description;
                            if (GetDescription(argElem, description)) {
                                argDescriptions.insert(pair<qcc::String, qcc::String>(nameAtt, description));
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
                    } else if (argElem->GetName() == "description") {
                        memberDescription = argElem->GetContent();
                    }

                }
                if (isSignal) {
                    const qcc::String& sessionlessStr = ifChildElem->GetAttribute("sessionless");
                    isSessionlessSignal = (sessionlessStr == "true");
                }

                /* Add the member */
                if ((ER_OK == status) && (isMethod || isSignal)) {
                    status = intf.AddMember(isMethod ? MESSAGE_METHOD_CALL : MESSAGE_SIGNAL,
                                            memberName.c_str(),
                                            inSig.c_str(),
                                            outSig.c_str(),
                                            isArgNamesEmpty ? NULL : argNames.c_str());

                    for (std::map<String, String>::const_iterator it = annotations.begin(); it != annotations.end(); ++it) {
                        intf.AddMemberAnnotation(memberName.c_str(), it->first, it->second);
                    }

                    if (!memberDescription.empty()) {
                        intf.SetMemberDescription(memberName.c_str(), memberDescription.c_str(), isSessionlessSignal);
                    }

                    for (std::map<String, String>::const_iterator it = argDescriptions.begin(); it != argDescriptions.end(); it++) {
                        intf.SetArgDescription(memberName.c_str(), it->first.c_str(), it->second.c_str());
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
                status = intf.AddProperty(memberName.c_str(), sig.c_str(), access);

                // add Property annotations
                const vector<XmlElement*>& argChildren = ifChildElem->GetChildren();
                vector<XmlElement*>::const_iterator argIt = argChildren.begin();
                while ((ER_OK == status) && (argIt != argChildren.end())) {
                    const XmlElement* argElem = *argIt++;
                    status = intf.AddPropertyAnnotation(memberName, argElem->GetAttribute("name"), argElem->GetAttribute("value"));
                }

                qcc::String description;
                if (GetDescription(ifChildElem, description)) {
                    intf.SetPropertyDescription(memberName.c_str(), description.c_str());
                }
            }
        } else if (ifChildName == "annotation") {
            status = intf.AddAnnotation(ifChildElem->GetAttribute("name"), ifChildElem->GetAttribute("value"));
        } else if (ifChildName == "description") {
            qcc::String const& description = ifChildElem->GetContent();
            qcc::String const& language = ifChildElem->GetAttribute("language");
            intf.SetDescriptionLanguage(language.c_str());
            intf.SetDescription(description.c_str());
        } else {
            status = ER_FAIL;
            QCC_LogError(status, ("Unknown element \"%s\" found in introspection data from %s", ifChildName.c_str(), ident));
            break;
        }
    }
    /* Add the interface with all its methods, signals and properties */
    if (ER_OK == status) {
        InterfaceDescription* newIntf = NULL;
        status = bus->CreateInterface(intf.GetName(), newIntf);
        if (ER_OK == status) {
            /* Assign new interface */
            *newIntf = intf;
            newIntf->Activate();
            if (obj) {
                obj->AddInterface(*newIntf);
            }
        } else if (ER_BUS_IFACE_ALREADY_EXISTS == status) {
            /* Make sure definition matches existing one */
            const InterfaceDescription* existingIntf = bus->GetInterface(intf.GetName());
            if (existingIntf) {
                if (*existingIntf == intf) {
                    if (obj) {
                        obj->AddInterface(*existingIntf);
                    }
                    status = ER_OK;
                } else {
                    status = ER_BUS_INTERFACE_MISMATCH;
                    QCC_LogError(status, ("XML interface does not match existing definition for \"%s\"", intf.GetName()));
                }
            } else {
                status = ER_FAIL;
                QCC_LogError(status, ("Failed to retrieve existing interface \"%s\"", intf.GetName()));
            }
        } else {
            QCC_LogError(status, ("Failed to create new inteface \"%s\"", intf.GetName()));
        }
    }
    return status;
}

QStatus XmlHelper::ParseNode(const XmlElement* root, ProxyBusObject* obj)
{
    QStatus status = ER_OK;

    (void)ident; /* suppress compiler warning */
    assert(root->GetName() == "node");

    if (GetSecureAnnotation(root) == "true") {
        if (obj) {
            obj->isSecure = true;
        }
    }
    /* Iterate over <interface> and <node> elements */
    const vector<XmlElement*>& rootChildren = root->GetChildren();
    vector<XmlElement*>::const_iterator it = rootChildren.begin();
    while ((ER_OK == status) && (it != rootChildren.end())) {
        const XmlElement* elem = *it++;
        const qcc::String& elemName = elem->GetName();
        if (elemName == "interface") {
            status = ParseInterface(elem, obj);
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
                        status = ParseNode(elem, childObj);
                    } else {
                        ProxyBusObject newChild(*bus, obj->GetServiceName().c_str(), obj->GetUniqueName().c_str(), childObjPath.c_str(), obj->sessionId, obj->isSecure);
                        status = ParseNode(elem, &newChild);
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
                status = ParseNode(elem, NULL);
            }
        }
    }
    return status;
}

} // ajn::
