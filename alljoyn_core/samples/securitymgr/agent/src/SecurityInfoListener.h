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

#ifndef ALLJOYN_SECMGR_SECINFOLISTENER_H_
#define ALLJOYN_SECMGR_SECINFOLISTENER_H_

#include "SecurityInfo.h"

namespace ajn {
namespace securitymgr {
class SecurityInfoListener {
  public:
    /**
     * @brief Callback function that is called when security information is
     * discovered, updated or removed.
     *
     * @param[in] oldSecInfo   Previous security information for a busname;
     *                         nullptr if a new security information is discovered.
     * @param[in] newSecInfo   Updated security information for a busname;
     *                         nullptr if a security information has been removed.
     */
    virtual void OnSecurityStateChange(const SecurityInfo* oldSecInfo,
                                       const SecurityInfo* newSecInfo) = 0;

  protected:
    SecurityInfoListener() { };

  public:
    virtual ~SecurityInfoListener() { };
};
}
}
#endif /* ALLJOYN_SECMGR_SECINFOLISTENER_H_ */