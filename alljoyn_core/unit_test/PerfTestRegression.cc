/**
 * @file
 * This program tests the basic features of Alljoyn.It uses google test as the test
 * automation framework.
 */
/******************************************************************************
 * Copyright (c) 2009-2011,2014 AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

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
