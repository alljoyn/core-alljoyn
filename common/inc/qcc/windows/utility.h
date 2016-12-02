/**
 * @file
 *
 * Define utility functions for Windows.
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
#ifndef _OS_QCC_UTILITY_H
#define _OS_QCC_UTILITY_H

#include <qcc/platform.h>
#include <windows.h>

void strerror_r(uint32_t errCode, char* ansiBuf, uint16_t ansiBufSize);

wchar_t* MultibyteToWideString(const char* str);

/**
 * Ensure that Winsock API is loaded.
 * Called before any operation that might be called before winsock has been started.
 */
void WinsockCheck();

#endif