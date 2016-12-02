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

#ifndef AJINITIALIZER_H_
#define AJINITIALIZER_H_

#include <alljoyn/Status.h>

/**
 * class AJInitializer
 * Utility class for handling AllJoyn lifecycle methods
 */
class AJInitializer {
  public:
    /**
     * Calls AllJoynInit(). If bundled router is enabled it also calls AllJoynRouterInit()
     * @return ER_OK if initialization succeeded
     */
    QStatus Initialize();

    /**
     * ~AJInitializer
     * Calls AllJoynRouterShutdown() if bundled router is enabled and then calls AllJoynShutdown()
     */
    ~AJInitializer();
};

#endif /* AJINITIALIZER_H_ */