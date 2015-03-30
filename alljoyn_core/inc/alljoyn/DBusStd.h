#ifndef _ALLJOYN_DBUSSTD_H
#define _ALLJOYN_DBUSSTD_H

/**
 * @file
 * This file provides definitions for standard DBus interfaces
 *
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
