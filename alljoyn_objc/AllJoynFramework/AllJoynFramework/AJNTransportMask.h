////////////////////////////////////////////////////////////////////////////////
// // 
//    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
//    Source Project Contributors and others.
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0

////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import "AvailabilityMacros.h"

/**
 * Transports mask
 */
typedef uint16_t AJNTransportMask;

/**< no transports */
extern const AJNTransportMask kAJNTransportMaskNone;

/**< Local (same device) transport */
extern const AJNTransportMask kAJNTransportMaskLocal;

/**< Transport using TCP transport */
extern const AJNTransportMask kAJNTransportMaskTCP;

/**< Transport using UDP transport */
extern const AJNTransportMask kAJNTransportMaskUDP;

/**< Placeholder for experimental transport */
extern const AJNTransportMask kAJNTransportMaskExperimental;

/**< Transport using IP-based transport */
extern const AJNTransportMask kAJNTransportMaskIP;

/**< ANY transport */
extern const AJNTransportMask kAJNTransportMaskAny;

/*
 * QCC_DEPRECATED is a macro defined in platform.h on Linux and Windows for C++
 * that is used to mark functions, etc., as deprecated. Windows compilers use
 * the __declspec(deprecated) prefix notation and various flavors of gcc use the
 * __attribute__ ((deprecated)) postfix notation.  Objective-C on the other hand
 * uses a postfix to the initialization value for attribtues which isn't
 * compatible with the use of QCC_DEPRECATED.  Therefore we use the functionality
 * defined for the purpose directly in Objective-C: DEPRECATED_ATTRIBUTE.
 */

/**< Wireless local-area network transport */
extern const AJNTransportMask kAJNTransportMaskWLAN DEPRECATED_ATTRIBUTE;

/**< Wireless wide-area network transport */
extern const AJNTransportMask kAJNTransportMaskWWAN DEPRECATED_ATTRIBUTE;

/**< Wired local-area network transport */
extern const AJNTransportMask kAJNTransportMaskLAN DEPRECATED_ATTRIBUTE;

/**< Transport using Wi-Fi Direct transport */
extern const AJNTransportMask kAJNTransportMaskWiFiDirect DEPRECATED_ATTRIBUTE;


