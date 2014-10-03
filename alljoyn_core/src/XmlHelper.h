#ifndef _ALLJOYN_XMLHELPER_H
#define _ALLJOYN_XMLHELPER_H
/**
 * @file
 *
 * This file defines a class for traversing introspection XML.
 *
 */

/******************************************************************************
 * Copyright (c) 2011, 2014, AllSeen Alliance. All rights reserved.
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

#ifndef __cplusplus
#error Only include BusUtil.h in C++ code.
#endif

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
                return ParseInterface(root, NULL);
            } else if (root->GetName() == "node") {
                return ParseNode(root, NULL);
            }
        }
        return ER_BUS_BAD_XML;
    }

    /**
     * Traverse the XML tree recursively adding all nodes as children of a parent proxy object.
     *
     * @param parent  The parent proxy object to add the children too.
     * @param root    The root must be a <node> element.
     *
     * @return #ER_OK if the XML was well formed and the children were added.
     *         #ER_BUS_BAD_XML if the XML was not as expected.
     *         #Other errors indicating the children were not succesfully added.
     */
    QStatus AddProxyObjects(ProxyBusObject& parent, const qcc::XmlElement* root) {
        if (root && (root->GetName() == "node")) {
            return ParseNode(root, &parent);
        } else {
            return ER_BUS_BAD_XML;
        }
    }

  private:

    QStatus ParseNode(const qcc::XmlElement* elem, ProxyBusObject* obj);
    QStatus ParseInterface(const qcc::XmlElement* elem, ProxyBusObject* obj);

    BusAttachment* bus;
    const char* ident;
};
}

#endif
