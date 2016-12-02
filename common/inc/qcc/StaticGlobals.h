/**
 * @file
 * File for holding static global variables.
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
#ifndef _QCC_STATICGLOBALS_H
#define _QCC_STATICGLOBALS_H

#include <qcc/platform.h>
#include <Status.h>

namespace qcc {

/**
 * This must be called prior to instantiating or using any qcc
 * functionality.
 *
 * This function is not thread-safe.
 */
QStatus Init();

/**
 * Call this to release any resources acquired in Init().  No qcc
 * functionality may be used after calling this.
 *
 * This function is not thread-safe.
 */
QStatus Shutdown();

}

#endif