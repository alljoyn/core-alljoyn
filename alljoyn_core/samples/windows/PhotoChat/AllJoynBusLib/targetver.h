/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 */
#pragma once

// The following macros define the minimum required platform.  The minimum required platform
// is the earliest version of Windows, Internet Explorer etc. that has the necessary features to run
// your application.  The macros work by enabling all features available on platform versions up to and
// including the version specified.

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef _WIN32_WINNT                   // Specifies that the minimum required platform is Windows 7.
#define _WIN32_WINNT _WIN32_WINNT_WIN7 // Change this to the appropriate value to target other versions of Windows.
#endif
#ifndef NTDDI_VERSION                  // Specifies that the minimum required platform is Windows 7.
#define NTDDI_VERSION NTDDI_WIN7       // Change this to the appropriate value to target other versions of Windows.
#endif