/**
 * @file
 *
 * OS dependent logging.
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
#ifndef _QCC_OS_LOGGER_H
#define _QCC_OS_LOGGER_H

#include <qcc/platform.h>
#include <qcc/Debug.h>

/**
 * Get the OS specific logger if there is one.
 *
 * @param useOSLog
 *
 * @return  Returns a function pointer to the OS-specific logger.
 */
QCC_DbgMsgCallback QCC_GetOSLogger(bool useOSLog);


#endif