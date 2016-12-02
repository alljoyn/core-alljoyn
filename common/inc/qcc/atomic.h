/**
 * @file
 *
 * Toolchain specific wrapper for atomic.h
 */

/******************************************************************************
 *
 *
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/
#ifndef _QCC_ATOMIC_H
#define _QCC_ATOMIC_H

#include <qcc/platform.h>

#if defined(QCC_OS_GROUP_POSIX)
#include <qcc/posix/atomic.h>
#elif defined(QCC_OS_GROUP_WINDOWS)
#include <qcc/windows/atomic.h>
#else
#error No OS GROUP defined.
#endif

#endif