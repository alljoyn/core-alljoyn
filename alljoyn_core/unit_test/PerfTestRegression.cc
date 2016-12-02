/**
 * @file
 * This program tests the basic features of Alljoyn.It uses google test as the test
 * automation framework.
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

#include "ServiceSetup.h"
#include "ServiceTestObject.h"
#include "ajTestCommon.h"

#include <qcc/time.h>
/* Header files included for Google Test Framework */
#include <gtest/gtest.h>

TEST(PerfTestRegression, Security_ALLJOYN_294_AddLogonEntry_Without_EnablePeerSecurity)
{
    Environ*env = qcc::Environ::GetAppEnviron();
    qcc::String clientArgs = env->Find("BUS_ADDRESS", ajn::getConnectArg().c_str());

    /* Create a Bus Attachment Object */
    BusAttachment* serviceBus = new BusAttachment("ALLJOYN-294", true);
    ASSERT_TRUE(serviceBus != NULL);
    serviceBus->Start();

    QStatus status = serviceBus->Connect(clientArgs.c_str());
    ASSERT_EQ(ER_OK, status);

    status = serviceBus->AddLogonEntry("ALLJOYN_SRP_LOGON", "sleepy", "123456");
    ASSERT_EQ(status, ER_BUS_KEYSTORE_NOT_LOADED);

    status = serviceBus->AddLogonEntry("ALLJOYN_SRP_LOGON", "happy", "123456");
    ASSERT_EQ(status, ER_BUS_KEYSTORE_NOT_LOADED);

    /* Clean up msg bus */
    if (serviceBus) {
        BusAttachment* deleteMe = serviceBus;
        serviceBus = NULL;
        delete deleteMe;
    }
}