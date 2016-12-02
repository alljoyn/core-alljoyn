/**
 * @file
 * Static global creation and destruction
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

#include <qcc/platform.h>
#include <qcc/StaticGlobals.h>
#include <qcc/LockLevel.h>
#include <alljoyn/Init.h>
#include <alljoyn/PasswordManager.h>
#include "AutoPingerInternal.h"
#include "BusInternal.h"
#include "KeyStoreListener.h"
#include "NamedPipeClientTransport.h"
#include "ProtectedAuthListener.h"
#include "XmlManifestTemplateConverter.h"
#include "XmlManifestTemplateValidator.h"
#include "XmlPoliciesConverter.h"
#include "XmlPoliciesValidator.h"
#include "XmlRulesConverter.h"
#include "XmlRulesValidator.h"
#include <limits>

namespace ajn {

/* Avoid including these internal methods in public header PermissionPolicy.h */
void PermissionPolicyInit();
void PermissionPolicyShutdown();

class StaticGlobals {
  public:
    static void Init()
    {
        ProtectedAuthListener::Init();
        KeyStore::Init();
        NamedPipeClientTransport::Init();
        AutoPingerInternal::Init();
        PasswordManager::Init();
        BusAttachment::Internal::Init();
        XmlManifestTemplateValidator::Init();
        XmlManifestTemplateConverter::Init();
        XmlPoliciesConverter::Init();
        XmlPoliciesValidator::Init();
        XmlRulesConverter::Init();
        XmlRulesValidator::Init();
        PermissionPolicyInit();
    }

    static void Shutdown()
    {
        PermissionPolicyShutdown();
        XmlRulesValidator::Shutdown();
        XmlRulesConverter::Shutdown();
        XmlPoliciesValidator::Shutdown();
        XmlPoliciesConverter::Shutdown();
        XmlManifestTemplateConverter::Shutdown();
        XmlManifestTemplateValidator::Shutdown();
        BusAttachment::Internal::Shutdown();
        PasswordManager::Shutdown();
        AutoPingerInternal::Shutdown();
        NamedPipeClientTransport::Shutdown();
        KeyStore::Shutdown();
        ProtectedAuthListener::Shutdown();
    }
};

}

extern "C" {

static uint32_t allJoynInitCount = 0;

/*
 * Disable LockOrderChecker for the Init lock, because this lock is being used
 * before LockOrderChecker & Thread modules have been initialized.
 */
static qcc::Mutex allJoynInitLock(qcc::LOCK_LEVEL_CHECKING_DISABLED);

QStatus AJ_CALL AllJoynInit(void)
{
    QStatus status = ER_OK;

    allJoynInitLock.Lock();

    if (allJoynInitCount == 0) {
        status = qcc::Init();
        if (status == ER_OK) {
            ajn::StaticGlobals::Init();
            allJoynInitCount = 1;
        }
    } else if (allJoynInitCount == std::numeric_limits<uint32_t>::max()) {
        QCC_ASSERT(!"Incorrect allJoynInitCount count");
        status = ER_INVALID_APPLICATION_STATE;
    } else {
        allJoynInitCount++;
    }

    allJoynInitLock.Unlock();

    return status;
}

QStatus AJ_CALL AllJoynShutdown(void)
{
    allJoynInitLock.Lock();

    QCC_ASSERT(allJoynInitCount > 0);
    allJoynInitCount--;

    if (allJoynInitCount == 0) {
        ajn::StaticGlobals::Shutdown();
        qcc::Shutdown();
    }

    allJoynInitLock.Unlock();

    return ER_OK;
}

}