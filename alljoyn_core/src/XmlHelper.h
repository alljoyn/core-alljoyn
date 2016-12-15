#ifndef _ALLJOYN_XMLHELPER_H
#define _ALLJOYN_XMLHELPER_H
/**
 * @file
 *
 * This file defines a class for traversing introspection XML.
 *
 */

/******************************************************************************
 *  * Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
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
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#ifndef __cplusplus
#error Only include BusUtil.h in C++ code.
#endif

#include <memory>
#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/XmlElement.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/InterfaceDescription.h>

#include <alljoyn/Status.h>

namespace ajn {

/**
 * XmlHelper is a utility class for traversing introspection XML.
 */
class XmlHelper {
  public:
    /**
     * XMLs obtained by calling IntrospectWithDescription() on a legacy object
     * paired with language tags of the descriptions in the given XML.
     *
     * @see ASACORE-2744
     */
    typedef std::map<qcc::String, std::unique_ptr<qcc::XmlParseContext> > XmlToLanguageMap;

    XmlHelper(BusAttachment* bus, const char* ident) : bus(bus), ident(ident) { }

    /**
     * Traverse the XML tree adding all interfaces to the bus. Nodes are ignored.
     *
     * @param root  The root can be an <interface> or <node> element.
     *
     * @return #ER_OK if the XML was well formed and the interfaces were added.
     *         #ER_BUS_BAD_XML if the XML was not as expected.
     *         #Other errors indicating the interfaces were not succesfully added.
     */
    QStatus AddInterfaceDefinitions(const qcc::XmlElement* root) {
        if (root) {
            if (root->GetName() == "interface") {
                InterfaceDescription intf;
                QStatus status = ParseInterface(root, intf);
                if (status == ER_OK) {
                    status = AddInterface(intf, nullptr);
                }
                return status;
            } else if (root->GetName() == "node") {
                return ParseNode(root, nullptr);
            }
        }
        return ER_BUS_BAD_XML;
    }

    /**
     * Traverse the XML tree recursively adding all nodes as children of a parent proxy object.
     *
     * @param[in, out] parent  The parent proxy object to add the children too.
     * @param[in] root The root must be a <node> element.
     * @param[in] legacyDescriptions XMLs containing descriptions in different languages obtained
     *            by calling IntrospectWithDescription() on a legacy (pre-16.04) object. See
     *            the documentation for ajn::ProxyBusObject::ParseLegacyXml().
     *
     * @return #ER_OK if the XML was well formed and the children were added.
     *         #ER_BUS_BAD_XML if the XML was not as expected.
     *         #Other errors indicating the children were not succesfully added.
     */
    QStatus AddProxyObjects(ProxyBusObject& parent, const qcc::XmlElement* root,
                            const XmlToLanguageMap* legacyDescriptions = nullptr) {
        if (root && (root->GetName() == "node")) {
            return ParseNode(root, &parent, legacyDescriptions);
        } else {
            return ER_BUS_BAD_XML;
        }
    }

  private:

    /**
     * @internal
     * Parses a provided introspection XML node.
     *
     * This method parses an introspection XML provided as a qcc::XmlElement representation
     * of a <node> element and creates the found interfaces on the bus.
     * If a ProxyBusObject is provided (obj argment is not nullptr),
     * the node's children: interfaces and nested nodes are added to the object.
     *
     * @param[in] elem The introspection XML <node> element.
     * @param[in, out] obj A pointer to an ajn::ProxyBusObject. If it is not a nullptr, it will be updated
     *                 with the children found in the XML. If it is a nullptr, the method will only
     *                 create the interfaces on the bus.
     * @param[in] legacyDescriptions XMLs containing descriptions in different languages obtained
     *        by calling IntrospectWithDescriptions() on a legacy (pre-16.04) object. See
     *        the documentation for ajn::ProxyBusObject::ParseLegacyXml().
     *
     * @return
     *       - ER_OK if the XML was parsed and the bus and ProxyBusObject were updated successfully.
     *       - An error status otherwise.
     *          - If an interface from the provided XML is already found on the bus and it does not match,
     *            the bus is not updated and ER_BUS_INTERFACE_MISMATCH is returned.
     */
    QStatus ParseNode(const qcc::XmlElement* elem, ProxyBusObject* obj,
                      const XmlToLanguageMap* legacyDescriptions = nullptr);

    /**
     * @internal
     * Parses a provided introspection XML for an interface.
     *
     * This method parses an introspection XML provided as a qcc::XmlElement representation
     * of an <interface> element and creates its ajn::InterfaceDescription representation
     * (interface argument).
     *
     * @param[in] elem The introspection XML <interface> element.
     * @param[out] intf The InterfaceDescription to be filled in based on the XML.
     *
     * @return
     *       - ER_OK if the XML was parsed  successfully.
     *       - An error status otherwise.
     */
    QStatus ParseInterface(const qcc::XmlElement* elem, InterfaceDescription& intf);

    /**
     * @internal
     * Checks the given interface against the standard DBus interface names.
     *
     * Standard DBus interfaces should be ignored by our introspection code
     * due to bugs in legacy Alljoyn versions (please see the comment in
     * XmlHelper::ParseInterface()).
     *
     * @param[in] intf The InterfaceDescription to be checked.
     *
     * @return
     *       - True if a standard DBus interface is provided.
     *       - False otherwise.
     */
    bool IsStandardDbusInterface(const InterfaceDescription& intf) const;

    /**
     * @internal
     * Adds descriptions stored in the legacyDescriptions map to the interface.
     *
     * The purpose of this function is to provide compatibility with pre-16.04 objects.
     * For 16.04+ objects, ProxyBusObject::IntrospectRemoteObject() only calls the remote object's Introspect() method.
     * Pre-16.04 implementations of Introspect() do not include descriptions in the returned
     * introspection XML. To obtain descriptions from pre-16.04 nodes, ProxyBusObject::IntrospectRemoteObject()
     * calls IntrospectWithDescription() for each available description language and stores the XMLs with
     * descriptions in a map. This map is then passed to this function (legacyDescriptions argument) so that
     * all the descriptions are added to the interface.
     *
     * @see ASACORE-2744
     *
     * @param[out] intf The interface to be decorated with the descriptions.
     * @param[in] legacyDescriptions A map of language-XML pairs with the descriptions.
     *
     * @return
     *       - ER_OK if the XMLs were successfully parsed and the descriptions added.
     *       - An error status otherwise.
     */
    QStatus AddLegacyDescriptions(InterfaceDescription& intf,
                                  const XmlToLanguageMap* legacyDescriptions) const;

    /**
     * @internal
     * Adds descriptions in the given language to the interface.
     *
     * The method parses the XML element for the given interface (interfaceElement argument, found in an XML
     * with descriptions by FindInterfaceElement()), adding the descriptions for the interface and its children.
     *
     * @param[out] intf The interface to be decorated with the descriptions.
     * @param[in] languageTag The language of the descriptions.
     * @param[in] interfaceElement An <interface> element for the given interface within an XML with descriptions.
     *
     * @return
     *       - ER_OK if the element was successfully parsed and the descriptions added.
     *       - An error status otherwise.
     */
    QStatus AddLegacyDescriptionsForLanguage(InterfaceDescription& intf,
                                             const qcc::String& languageTag,
                                             const qcc::XmlElement* interfaceElement) const;

    /**
     * @internal
     * Finds an <interface> element for a given interface within an introspection XML.
     *
     * The method searches the XML, provided via its root element (the root argument), for an
     * <interface> element which has the same name as the provided interfaceName argument.
     * If it finds such an element, it is returned as a pointer.
     *
     * @param[in] root The root element within the XML, from which the searching will start.
     * @param[in] interfaceName The name of the interface to be found.
     *
     * @return
     *       - A pointer to the <interface> element, if found.
     *       - nullptr otherwise.
     */
    const qcc::XmlElement* FindInterfaceElement(const qcc::XmlElement* root, const qcc::String& interfaceName) const;


    /**
     * @internal
     * Parses the given member element (<method> or <signal>) of an introspection XML and adds
     * descriptions for the member and its children.
     *
     * @param[out] intf The interface to be decorated with the descriptions.
     * @param[in] languageTag The language of the descriptions.
     * @param[in] memberElem The member element within an XML with descriptions.
     *
     * @return
     *       - ER_OK if the element was successfully parsed and the descriptions added.
     *       - An error status otherwise.
     */
    QStatus AddLegacyMemberDescriptions(InterfaceDescription& intf,
                                        const qcc::String& languageTag,
                                        const qcc::XmlElement* memberElem) const;

    /**
     * @internal
     * Parses the given argument element (<arg>) of an introspection XML and adds
     * descriptions for the argument.
     *
     * @param[out] intf The interface to be decorated with the descriptions.
     * @param[in] languageTag The language of the descriptions.
     * @param[in] parentName The name of the member which is the parent of argElem.
     * @param[in] argElem The argument element within an XML with descriptions.
     *
     * @return
     *       - ER_OK if the element was successfully parsed and the description added.
     *       - An error status otherwise.
     */
    QStatus AddLegacyArgDescription(InterfaceDescription& intf,
                                    const qcc::String& languageTag,
                                    const qcc::String& parentName,
                                    const qcc::XmlElement* argElem) const;

    /**
     * @internal
     * Parses the given property element (<property>) of an introspection XML and adds
     * descriptions for the property.
     *
     * @param[out] intf The interface to be decorated with the descriptions.
     * @param[in] languageTag The language of the description.
     * @param[in] propertyElem The property element within an XML with descriptions.
     *
     * @return
     *       - ER_OK if the element was successfully parsed and the description added.
     *       - An error status otherwise.
     */
    QStatus AddLegacyPropertyDescription(InterfaceDescription& intf,
                                         const qcc::String& languageTag,
                                         const qcc::XmlElement* propertyElem) const;

    /**
     * @internal
     * Sets the given description in the given language for the given interface.
     *
     * @param[out] intf The interface to be decorated with the description.
     * @param[in] description The description.
     * @param[in] languageTag The language of the description.
     *
     * @return
     *       - ER_OK if the description was successfully set.
     *       - An error status otherwise.
     */
    QStatus SetDescriptionForInterface(InterfaceDescription& intf,
                                       const qcc::String& description,
                                       const qcc::String& languageTag) const;

    /**
     * @internal
     * Sets the given description in the given language for the given interface member.
     *
     * @param[out] intf The interface to be decorated with the description.
     * @param[in] memberName The name of the member.
     * @param[in] description The description.
     * @param[in] languageTag The language of the description.
     *
     * @return
     *       - ER_OK if the description was successfully set.
     *       - An error status otherwise.
     */
    QStatus SetMemberDescriptionForInterface(InterfaceDescription& intf,
                                             const qcc::String& memberName,
                                             const qcc::String& description,
                                             const qcc::String& languageTag) const;

    /**
     * @internal
     * Sets the given description in the given language for the given member argument.
     *
     * @param[out] intf The interface to be decorated with the description.
     * @param[in] parentName The name of the member which is the parent of the argument.
     * @param[in] argName The name of the argument.
     * @param[in] description The description.
     * @param[in] languageTag The language of the description.
     *
     * @return
     *       - ER_OK if the description was successfully set.
     *       - An error status otherwise.
     */
    QStatus SetArgDescriptionForInterface(InterfaceDescription& intf,
                                          const qcc::String& parentName,
                                          const qcc::String& argName,
                                          const qcc::String& description,
                                          const qcc::String& languageTag) const;

    /**
     * @internal
     * Sets the given description in the given language for the given interface property.
     *
     * @param[out] intf The interface to be decorated with the description.
     * @param[in] propertyName The name of the property.
     * @param[in] description The description.
     * @param[in] languageTag The language of the description.
     *
     * @return
     *       - ER_OK if the description was successfully set.
     *       - An error status otherwise.
     */
    QStatus SetPropertyDescriptionForInterface(InterfaceDescription& intf,
                                               const qcc::String& propertyName,
                                               const qcc::String& description,
                                               const qcc::String& languageTag) const;

    /**
     * @internal
     * Adds the created interface to the ProxyBusObject.
     *
     * @param[in] intf The interface to be added.
     * @param[in] proxyBusObject The proxy object to which the interface is to be added.
     *
     * @return
     *       - ER_OK if the interface was successfully added.
     *       - An error status otherwise.
     */
    QStatus AddInterface(const InterfaceDescription& intf, ProxyBusObject* obj);

    BusAttachment* bus;
    const char* ident;
};
}

#endif