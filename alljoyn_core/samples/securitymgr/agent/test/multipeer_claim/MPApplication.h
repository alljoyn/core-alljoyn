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

#ifndef ALLJOYN_SECMGR_MPAPPLICATION_H_
#define ALLJOYN_SECMGR_MPAPPLICATION_H_

#include <string>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/securitymgr/Manifest.h>

using namespace std;
using namespace ajn;
using namespace ajn::securitymgr;
using namespace qcc;

namespace secmgr_tests {
class MPApplication {
  public:
    /**
     * Creates a new MPApplication.
     */
    MPApplication(pid_t ownPid);

    /**
     * Starts this MPApplication.
     */
    QStatus Start();

    QStatus WaitUntilFinished();

    /*
     * Stops this MPApplication.
     */
    QStatus Stop();

    ~MPApplication();

  private:
    void Reset();

    BusAttachment* busAttachment;
    DefaultECDHEAuthListener authListener;
    string appName;
};
} // namespace
#endif //ALLJOYN_SECMGR_MPAPPLICATION_H_