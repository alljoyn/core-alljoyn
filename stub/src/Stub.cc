/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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

#include <qcc/Util.h>
#include <qcc/CryptoECC.h>

#include "ECDHEKeyXListener.h"

#define STUB_KEYSTORE "/.alljoyn_keystore/stub.ks"

QStatus Stub::GenerateManifest(PermissionPolicy::Rule** retRules, size_t* count)
{
    *count = 2;
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[*count];
    rules[0].SetInterfaceName("org.allseenalliance.control.TV");
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(2, prms);

    rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);

    *retRules = rules;
    return ER_OK;
}

Stub::Stub(ClaimListener* cl,
           bool dsa) :
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

        if (ba.Connect() != ER_OK) {
            std::cerr << "Could not connect" << std::endl;
            break;
        }

        // BusAttachment needs to be started before PeerSecurity is enabled
        // to deliver NotifyConfig signal
        if (ER_OK !=
            ba.EnablePeerSecurity(dsa ? ECDHE_KEYX : ECDHE_KEYX " " KEYX_ECDHE_NULL, new ECDHEKeyXListener(),
                                  STUB_KEYSTORE, false)) {
            std::cerr << "BusAttachment::EnablePeerSecurity failed." << std::endl;
            break;
        }

        pm = new PermissionMgmt(ba, cl, this);

        // Generate default policy
        PermissionPolicy::Rule* manifestRules;
        size_t manifestRulesCount;
        GenerateManifest(&manifestRules, &manifestRulesCount);
        this->SetUsedManifest(manifestRules, manifestRulesCount);

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
    ba.Disconnect();
    ba.Stop();
    delete pm;
}

QStatus Stub::OpenClaimWindow()
{
    QStatus status = ER_FAIL;
    if (ajn::PermissionConfigurator::STATE_CLAIMED == pm->GetClaimableState()) {
        std::cerr << "Application is already claimed by a RoT" << std::endl;
        return status;
    }
    if (ajn::PermissionConfigurator::STATE_CLAIMABLE == pm->GetClaimableState()) {
        std::cerr << "Claim window already open" << std::endl;
        return status;
    }

    /* make the device claimable */
    pm->SetClaimableState(true);

    return ER_OK;
}

QStatus Stub::CloseClaimWindow()
{
    QStatus status = ER_FAIL;
    if ((0 != pm->GetRoTKeys().size()) || (ajn::PermissionConfigurator::STATE_CLAIMED == pm->GetClaimableState())) {
        std::cerr << "Claim window already closed" << std::endl;
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

    AboutServiceApi::getInstance()->Register(APPLICATION_PORT);
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

std::map<GUID128, qcc::String> Stub::GetMembershipCertificates() const
{
    return pm->GetMembershipCertificates();
}

QStatus Stub::Reset()
{
    // The following implementation does not seem to work.
    ba.ClearKeyStore();
    // This work-around seems to do the trick :)
    qcc::String fname = GetHomeDir();
    fname.append("/");
    fname.append(STUB_KEYSTORE);
    if (0 == (remove(fname.c_str()))) {
        return ER_OK;
    } else {
        return ER_FAIL;
    }
}

void Stub::SetDSASecurity(bool dsa)
{
    QStatus status = ba.EnablePeerSecurity(dsa ? ECDHE_KEYX : KEYX_ECDHE_NULL, new ECDHEKeyXListener(),
                                           STUB_KEYSTORE, false);
    fprintf(stderr, "Stub::SetDSASecurity(%i) result = %i\n", dsa, status);
}
