#ifndef _ALLJOYN_DBUSSTD_H
#define _ALLJOYN_DBUSSTD_H

/**
 * @file
 * This file provides definitions for standard DBus interfaces
 *
 */

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

#ifdef __cplusplus

#include <qcc/platform.h>

#include <alljoyn/DBusStdDefines.h>
#include <alljoyn/Status.h>

namespace ajn {
class BusAttachment;

namespace org {
namespace freedesktop {

/** Interface Definitions for org.freedesktop.DBus */
namespace DBus {
extern const char* ObjectPath;                         /**< Object path */
extern const char* InterfaceName;                      /**< Name of the interface */
extern const char* WellKnownName;                      /**< The well known name */

extern const char* AnnotateNoReply;                    /**< Annotation for reply to a method call */
extern const char* AnnotateDeprecated;                 /**< Annotation for marking entry as depreciated  */
extern const char* AnnotateEmitsChanged;               /**< Annotation for when a property is modified {true,false,invalidates} */

/** Definitions for org.freedesktop.DBus.Properties */
namespace Properties {
extern const char* InterfaceName;                          /**< Name of the interface   */
}

/** Definitions for org.freedesktop.DBus.Peer */
namespace Peer {
extern const char* InterfaceName;                          /**< Name of the interface   */
}

/** Definitions for org.freedesktop.DBus.Introspectable */
namespace Introspectable {
extern const char* InterfaceName;                         /**< Name of the interface   */
extern const char* IntrospectDocType;                     /**< Type of introspection document */
}

/** Create the org.freedesktop.DBus interfaces and sub-interfaces */
QStatus AJ_CALL CreateInterfaces(BusAttachment& bus);

}
}
}
}
#endif

#endif