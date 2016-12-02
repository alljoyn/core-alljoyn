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


#ifndef LOGMODULE_H_
#define LOGMODULE_H_

#include <qcc/Debug.h>
#include <qcc/platform.h>

namespace ajn {
namespace services {
namespace logModules {

static char const* const CONFIG_MODULE_LOG_NAME = "Config";
static const uint32_t ALL_LOG_LEVELS = 15;
}

static char const* const QCC_MODULE = logModules::CONFIG_MODULE_LOG_NAME;
}
}

#endif /* LOGMODULE_H_ */