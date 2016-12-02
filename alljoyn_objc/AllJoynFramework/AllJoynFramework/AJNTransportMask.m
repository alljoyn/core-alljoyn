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

#import "AJNTransportMask.h"

const AJNTransportMask kAJNTransportMaskNone         = 0x0000;   /**< no transports */
const AJNTransportMask kAJNTransportMaskLocal        = 0x0001;   /**< Local (same device) transport */
const AJNTransportMask kAJNTransportMaskTCP          = 0x0004;   /**< TCP/IP transport */
const AJNTransportMask kAJNTransportMaskUDP          = 0x0100;   /**< UDP/IP transport */
const AJNTransportMask kAJNTransportMaskExperimental = 0x8000;   /**< Placeholder for experimental transport */
const AJNTransportMask kAJNTransportMaskIP           = (kAJNTransportMaskTCP + kAJNTransportMaskUDP); /**< An IP-based transport */
const AJNTransportMask kAJNTransportMaskAny          = (kAJNTransportMaskLocal + kAJNTransportMaskIP); /**< Any non-experimental transport */

const AJNTransportMask kAJNTransportMaskWLAN         = 0x0004;   /**< Wireless local-area network transport */
const AJNTransportMask kAJNTransportMaskWWAN         = 0x0008;   /**< Wireless wide-area network transport */
const AJNTransportMask kAJNTransportMaskLAN          = 0x0010;   /**< Wired local-area network transport */
const AJNTransportMask kAJNTransportMaskWiFiDirect   = 0x0080;   /**< Transport using Wi-Fi Direct transport */