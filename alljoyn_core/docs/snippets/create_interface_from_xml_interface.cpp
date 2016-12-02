/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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

/// [xml_interface_adding_namespace]
namespace com {
namespace example {
static const char* name = "com.example.interface";
static const char* interface = "<interface name='com.example.interface'>"
                               "  <method name='Echo'>"
                               "    <arg name='input_arg' type='s' direction='in' />"
                               "    <arg name='return_arg' type='s' direction='out' />"
                               "  </method>"
                               "  <signal name='Chirp'>"
                               "    <arg name='sound' type='s' />"
                               "  </signal>"
                               "  <property name='Volume' type='i' access='readwrite'/>"
                               "</interface>";
}
}
/// [xml_interface_adding_namespace]

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

        /// [xml_interface_adding_to_busattachment]
        QStatus status = busAttachment.CreateInterfacesFromXml(::com::example::interface);
        if (status != ER_OK) {
            printf("Failed to create %s\n interface from an xml interface", ::com::example::name);
        } else {
            printf("%s has been created from xml node\n", ::com::example::name);
        }
        /// [xml_interface_adding_to_busattachment]

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