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

#import "AJNBus.h"

/**< RequestName input flag: Allow others to take ownership of this name */
const AJNBusNameFlag kAJNBusNameFlagAllowReplacement = 0x01;     
/**< RequestName input flag: Attempt to take ownership of name if already taken */
const AJNBusNameFlag kAJNBusNameFlagReplaceExisting  = 0x02;     
/**< RequestName input flag: Fail if name cannot be immediately obtained */
const AJNBusNameFlag kAJNBusNameFlagDoNotQueue       = 0x04;     