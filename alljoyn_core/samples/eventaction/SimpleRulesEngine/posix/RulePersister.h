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

#include "../Rule.h"

#ifndef _RULE_PERSISTER_POSIX_
#define _RULE_PERSISTER_POSIX_

class RulePersister {
  public:
    RulePersister();

    ~RulePersister();

    void saveRule(Rule* rule);

    void loadRules();

    void clearRules();
};

#endif //_RULE_PERSISTER_POSIX_
