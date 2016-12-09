/******************************************************************************
 * Copyright (c) 2016 Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright 2016 Open Connectivity Foundation and Contributors to
 *    AllSeen Alliance. All rights reserved.
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
/*
 * The following code was developed with the documentation. Please don't modify
 * this code unless fixing errors in the code or documentation.
 */
/*
 * This sample demonstrates how to create and add an alljoyn interface.
 */

#include <cstdio>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/Init.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/Status.h>

/// [xml_node_adding_namespace]
namespace com {
namespace example {
static const char* name = "com.example.interface";
static const char* node = "<node name='/example/xml/interface'>"
                          "  <interface name='com.example.interface'>"
                          "    <method name='Echo'>"
                          "      <arg name='input_arg' type='s' direction='in' />"
                          "      <arg name='return_arg' type='s' direction='out' />"
                          "    </method>"
                          "    <signal name='Chirp'>"
                          "      <arg name='sound' type='s' />"
                          "    </signal>"
                          "    <property name='Volume' type='i' access='readwrite'/>"
                          "  </interface>"
                          "</node>";
}
}
/// [xml_node_adding_namespace]

using namespace ajn;

int CDECL_CALL main()
{
    if (AllJoynInit() != ER_OK) {
        return 1;
    }
#ifdef ROUTER
    if (AllJoynRouterInit() != ER_OK) {
        AllJoynShutdown();
        return 1;
    }
#endif
    {
        BusAttachment busAttachment("AddInterfaceFromXml");

        /// [xml_node_adding_to_busattachment]
        QStatus status = busAttachment.CreateInterfacesFromXml(::com::example::node);
        if (status != ER_OK) {
            // error handling code goes here
            printf("Failed to create %s\n interface from an xml node", ::com::example::name);
        } else {
            printf("%s has been created from xml node\n", ::com::example::name);
        }
        /// [xml_node_adding_to_busattachment]

        const InterfaceDescription* interfaceFromBus = busAttachment.GetInterface(::com::example::name);
        if (interfaceFromBus == NULL) {
            printf("Failed to Get %s\n", ::com::example::name);
        } else {
            printf("Read the %s interface back from the busAttachment.\n%s\n", ::com::example::name, interfaceFromBus->Introspect().c_str());
        }
    }
#ifdef ROUTER
    AllJoynRouterShutdown();
#endif
    AllJoynShutdown();
    return 0;
}