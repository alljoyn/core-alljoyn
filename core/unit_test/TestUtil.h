/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#ifndef TESTUTIL_H_
#define TESTUTIL_H_

#include "gtest/gtest.h"

#include "SecurityManagerFactory.h"
#include "PermissionMgmt.h"
#include <qcc/String.h>

#include <semaphore.h>

using namespace ajn::securitymgr;
using namespace std;
namespace secmgrcoretest_unit_testutil {

class ClaimTest :
    public::testing::Test {
  private:

  protected:

    virtual void SetUp();
    virtual void TearDown();
  public:
    ajn::securitymgr::SecurityManager* secMgr;
    ajn::BusAttachment* ba;
    ClaimTest();
};

class TestClaimListener :
    public ClaimListener {
  public:
    TestClaimListener(bool& _claimAnswer) :
        claimAnswer(_claimAnswer) { }

  private:
    bool& claimAnswer;

    bool OnClaimRequest(const qcc::ECCPublicKey* pubKeyRot,
                        void* ctx)
    {
        return claimAnswer;
    }

    /* This function is called when the claiming process has completed successfully */
    void OnClaimed(void* ctx)
    {
    }
};

class TestApplicationListener :
    public ApplicationListener {
  public:
    TestApplicationListener(sem_t& _sem);

    ApplicationInfo _lastAppInfo;

  private:
    sem_t& sem;

    void OnApplicationStateChange(const ApplicationInfo* old,
                                  const ApplicationInfo* updated);
};
}
#endif /* TESTUTIL_H_ */
