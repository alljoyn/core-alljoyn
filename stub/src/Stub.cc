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

#include "Stub.h"
#include "MySessionListener.h"

#include <qcc/CryptoECC.h>

#include "ECDHEKeyXListener.h"

Stub::Stub(ClaimListener* cl) :
    ba("mystub", true)
{
    do {
        /* Call static function to create the PermissionMgmt interface */
        if (PermissionMgmt::CreateInterface(ba) != ER_OK) {
            std::cerr << "Could not create interface" << std::endl;
            break;
        }

        if (ba.Start() != ER_OK) {
            std::cerr << "Could not start" << std::endl;
            break;
        }

        if (ER_OK !=
            ba.EnablePeerSecurity(ECDHE_KEYX, new ECDHEKeyXListener(),
                                  "/.alljoyn_keystore/s_ecdhe.ks", false)) {
            std::cerr << "BusAttachment::EnablePeerSecurity failed." << std::endl;
            break;
        }

        if (ba.Connect() != ER_OK) {
            std::cerr << "Could not connect" << std::endl;
            break;
        }

        pm = new PermissionMgmt(ba, cl, this);

        char guid[33];
        snprintf(guid, sizeof(guid), "A0%X%026X", rand(), rand()); /* yes, i know this is not a real guid - good enough for a stub */

        if (AdvertiseApplication(guid) != ER_OK) {
            std::cerr << "Could not advertise" << std::endl;
            break;
        }

        ba.RegisterBusObject(*pm);
        pm->SendClaimDataSignal();
    } while (0);
}

Stub::~Stub()
{
    if (AboutServiceApi::getInstance() != NULL) {
        AboutServiceApi::getInstance()->Unregister();
        ba.UnregisterBusObject(*AboutServiceApi::getInstance());
        AboutServiceApi::DestroyInstance();
    }
    delete pm;
}

/** Create the session, report the result to stdout, and return the status code. */
QStatus Stub::StartListeningForSession(TransportMask mask)
{
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, mask);
    SessionPort sp = SERVICE_PORT;
    QStatus status = ba.BindSessionPort(sp, opts, ml);

    if (ER_OK == status) {
        printf("BindSessionPort succeeded.\n");
    } else {
        printf("BindSessionPort failed (%s).\n", QCC_StatusText(status));
    }

    return status;
}

QStatus Stub::StopListeningForSession()
{
    SessionPort sp = SERVICE_PORT;
    QStatus status = ba.UnbindSessionPort(sp);

    if (ER_OK == status) {
        printf("UnBindSessionPort succeeded.\n");
    } else {
        printf("UnBindSessionPort failed (%s).\n", QCC_StatusText(status));
    }

    return status;
}

QStatus Stub::OpenClaimWindow()
{
    QStatus status = ER_FAIL;
    if (CLAIMED == pm->GetClaimableState()) {
        std::cerr << "Application is already claimed by a RoT" << std::endl;
        return status;
    }
    if (CLAIMABLE == pm->GetClaimableState()) {
        std::cerr << "Claim window already open" << std::endl;
        return status;
    }
    status = StartListeningForSession(TRANSPORT_ANY);
    if (status != ER_OK) {
        printf("Could not listen for session");
        return status;
    }

    /* make the device claimable */
    pm->SetClaimableState(true);

    return status;
}

QStatus Stub::CloseClaimWindow()
{
    QStatus status = ER_FAIL;
    if ((0 != pm->GetRoTKeys().size()) || (CLAIMED == pm->GetClaimableState())) {
        std::cerr << "Claim window already closed" << std::endl;
        return status;
    }
    status = StopListeningForSession();
    if (status != ER_OK) {
        printf("Could not listen for session\r\n");
        return status;
    }

    /* make this device no longer claimable */
    pm->SetClaimableState(false);

    return status;
}

QStatus Stub::FillAboutPropertyStoreImplData(AboutPropertyStoreImpl* propStore, const char* guid)
{
    QStatus status = ER_OK;

    status = propStore->setDeviceId("Linux");
    if (status != ER_OK) {
        return status;
    }

    qcc::String str(guid);
    std::cout << str << std::endl;
    status = propStore->setAppId(str, "en");
    if (status != ER_OK) {
        return status;
    }

    std::vector<qcc::String> languages(1);
    languages[0] = "en";
    status = propStore->setSupportedLangs(languages);
    if (status != ER_OK) {
        return status;
    }
    status = propStore->setDefaultLang("en");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setAppName("Security Stub", "en");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setModelNumber("1");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setSoftwareVersion("");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setAjSoftwareVersion(ajn::GetVersion());
    if (status != ER_OK) {
        return status;
    }

    char buf[64];
    gethostname(buf, sizeof(buf));
    status = propStore->setDeviceName(buf, "en");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setDescription("This is a Security stub", "en");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setManufacturer("QEO LLC", "en");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setSupportUrl("http://www.alljoyn.org");
    if (status != ER_OK) {
        return status;
    }
    return status;
}

static AboutPropertyStoreImpl aboutPropertyStore;

QStatus Stub::AdvertiseApplication(const char* guid)
{
    QStatus status = FillAboutPropertyStoreImplData(&aboutPropertyStore, guid);
    if (status != ER_OK) {
        std::cerr << "Could not fill propertystore" << std::endl;
        return status;
    }

    AboutServiceApi::Init(ba, aboutPropertyStore);
    if (!AboutServiceApi::getInstance()) {
        std::cerr << "Could not get instance" << std::endl;
        return ER_FAIL;
    }

    AboutServiceApi::getInstance()->Register(SERVICE_PORT);
    status = ba.RegisterBusObject(*AboutServiceApi::getInstance());
    if (status != ER_OK) {
        std::cerr << "Could not register about bus object" << std::endl;
        return status;
    }

    status = AboutServiceApi::getInstance()->Announce();
    if (status != ER_OK) {
        std::cerr << "Could not announce" << std::endl;
        return status;
    }

    return ER_OK;
}

std::map<qcc::String, qcc::String> Stub::GetMembershipCertificates() const
{
    return pm->GetMembershipCertificates();
}
