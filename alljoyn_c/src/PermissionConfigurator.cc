/**
 * @file
 * PermissionConfigurator is responsible for managing an application's Security 2.0 settings.
 */

/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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

#include <alljoyn/BusAttachment.h>
#include <alljoyn/PermissionConfigurator.h>
#include <alljoyn/PermissionPolicy.h>
#include <alljoyn_c/PermissionConfigurator.h>
#include <stdio.h>
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

using namespace qcc;
using namespace ajn;

const alljoyn_claimcapabilities CLAIM_CAPABILITIES_DEFAULT = PermissionConfigurator::CLAIM_CAPABILITIES_DEFAULT;

QStatus AJ_CALL alljoyn_permissionconfigurator_getapplicationstate(const alljoyn_permissionconfigurator configurator, alljoyn_applicationstate* state)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator::ApplicationState returnedState;
    QStatus status = ((const PermissionConfigurator*)configurator)->GetApplicationState(returnedState);

    if (ER_OK == status) {
        *state = (alljoyn_applicationstate)returnedState;
    }

    return status;
}

QStatus AJ_CALL alljoyn_permissionconfigurator_setapplicationstate(alljoyn_permissionconfigurator configurator, const alljoyn_applicationstate state)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    return ((PermissionConfigurator*)configurator)->SetApplicationState((PermissionConfigurator::ApplicationState)state);
}

QStatus AJ_CALL alljoyn_permissionconfigurator_setmanifestfromxml(alljoyn_permissionconfigurator configurator, AJ_PCSTR manifestXml)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    return ((PermissionConfigurator*)configurator)->SetManifestTemplateFromXml(manifestXml);
}

QStatus AJ_CALL alljoyn_permissionconfigurator_getclaimcapabilities(const alljoyn_permissionconfigurator configurator, alljoyn_claimcapabilities* claimCapabilities)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator::ClaimCapabilities returnedClaimCapabilities;
    QStatus status = ((const PermissionConfigurator*)configurator)->GetClaimCapabilities(returnedClaimCapabilities);

    if (ER_OK == status) {
        *claimCapabilities = (alljoyn_claimcapabilities)returnedClaimCapabilities;
    }

    return status;
}

QStatus AJ_CALL alljoyn_permissionconfigurator_setclaimcapabilities(alljoyn_permissionconfigurator configurator, alljoyn_claimcapabilities claimCapabilities)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    return ((PermissionConfigurator*)configurator)->SetClaimCapabilities((PermissionConfigurator::ClaimCapabilities)claimCapabilities);
}

QStatus AJ_CALL alljoyn_permissionconfigurator_getclaimcapabilitiesadditionalinfo(const alljoyn_permissionconfigurator configurator, alljoyn_claimcapabilitiesadditionalinfo* additionalInfo)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    PermissionConfigurator::ClaimCapabilityAdditionalInfo returnedClaimCapabilitiesAdditionalInfo;
    QStatus status = ((const PermissionConfigurator*)configurator)->GetClaimCapabilityAdditionalInfo(returnedClaimCapabilitiesAdditionalInfo);

    if (ER_OK == status) {
        *additionalInfo = (alljoyn_claimcapabilitiesadditionalinfo)returnedClaimCapabilitiesAdditionalInfo;
    }

    return status;
}

QStatus AJ_CALL alljoyn_permissionconfigurator_setclaimcapabilitiesadditionalinfo(alljoyn_permissionconfigurator configurator, alljoyn_claimcapabilitiesadditionalinfo additionalInfo)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    return ((PermissionConfigurator*)configurator)->SetClaimCapabilityAdditionalInfo((PermissionConfigurator::ClaimCapabilityAdditionalInfo)additionalInfo);
}

QStatus AJ_CALL alljoyn_permissionconfigurator_reset(alljoyn_permissionconfigurator configurator)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    return ((PermissionConfigurator*)configurator)->Reset();
}