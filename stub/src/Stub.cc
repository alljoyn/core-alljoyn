/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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
    ba("mystub", true), pm(nullptr), aboutData("en"), aboutObj(ba), opts(SessionOpts::TRAFFIC_MESSAGES,
                                                                         false,
                                                                         SessionOpts::PROXIMITY_ANY,
                                                                         TRANSPORT_ANY), port(APPLICATION_PORT)
{
    do {
        /* Call static function to create the PermissionMgmt interface */
        if (PermissionMgmt::CreateInterface(ba) != ER_OK) {
            cerr << "Could not create interface" << endl;
            break;
        }

        if (ba.Start() != ER_OK) {
            cerr << "Could not start" << endl;
            break;
        }

        if (ba.Connect() != ER_OK) {
            cerr << "Could not connect" << endl;
            break;
        }

        if (ba.BindSessionPort(port, opts, spl) != ER_OK) {
            cout << "Could not bind session port" << endl;
            break;
        }

        char guid[33];
        snprintf(guid, sizeof(guid), "A0%X%026X", rand(), rand());         /* yes, I know this is not a real guid - good enough for a stub */

        if (AdvertiseApplication(guid) != ER_OK) {
            cerr << "Could not advertise" << endl;
            break;
        }

        // BusAttachment needs to be started before PeerSecurity is enabled
        // to deliver NotifyConfig signal
        if (ER_OK !=
            ba.EnablePeerSecurity(dsa ? ECDHE_KEYX : ECDHE_KEYX " " KEYX_ECDHE_NULL, new ECDHEKeyXListener(),
                                  STUB_KEYSTORE, false)) {
            cerr << "BusAttachment::EnablePeerSecurity failed." << endl;
            break;
        }

        pm = new PermissionMgmt(ba, cl, this);

        // Generate default policy
        PermissionPolicy::Rule* manifestRules;
        size_t manifestRulesCount;
        GenerateManifest(&manifestRules, &manifestRulesCount);
        this->SetUsedManifest(manifestRules, manifestRulesCount);

        ba.RegisterBusObject(*pm);
        pm->SendClaimDataSignal();
    } while (0);
}

Stub::~Stub()
{
    delete pm;
}

QStatus Stub::OpenClaimWindow()
{
    QStatus status = ER_FAIL;
    if (PermissionConfigurator::CLAIMED == pm->GetClaimableState()) {
        cerr << "Application is already claimed by a RoT" << endl;
        return status;
    }
    if (PermissionConfigurator::CLAIMABLE == pm->GetClaimableState()) {
        cerr << "Claim window already open" << endl;
        return status;
    }

    /* make the device claimable */
    pm->SetClaimableState(true);

    return ER_OK;
}

QStatus Stub::CloseClaimWindow()
{
    QStatus status = ER_FAIL;
    if ((0 != pm->GetRoTKeys().size()) || (PermissionConfigurator::CLAIMED == pm->GetClaimableState())) {
        cerr << "Claim window already closed" << endl;
        return status;
    }

    /* make this device no longer claimable */
    pm->SetClaimableState(false);

    return status;
}

QStatus Stub::SetAboutData(AboutData& aboutData, const char* guid)
{
    aboutData.SetAppId(guid);

    char buf[64];
    gethostname(buf, sizeof(buf));
    aboutData.SetDeviceName(buf);

    GUID128 deviceId;
    aboutData.SetDeviceId(deviceId.ToString().c_str());

    aboutData.SetAppName("Security Stub");
    aboutData.SetManufacturer("QEO LLC");
    aboutData.SetModelNumber("1");
    aboutData.SetDescription("This is a Security stub");
    aboutData.SetDateOfManufacture("2015-04-14");
    aboutData.SetSoftwareVersion("");
    aboutData.SetHardwareVersion("0.0.1");
    aboutData.SetSupportUrl("http://www.alljoyn.org");

    if (!aboutData.IsValid()) {
        cerr << "Invalid about data." << endl;
        return ER_FAIL;
    }
    return ER_OK;
}

QStatus Stub::AdvertiseApplication(const char* guid)
{
    QStatus status = SetAboutData(aboutData, guid);
    if (status != ER_OK) {
        cerr << "Could not set AboutData" << endl;
        return status;
    }

    status = aboutObj.Announce(APPLICATION_PORT, aboutData);
    if (status != ER_OK) {
        cerr << "Announcing stub failed with status = " << QCC_StatusText(status) << endl;
    }
    return status;
}

map<GUID128, string> Stub::GetMembershipCertificates() const
{
    return pm->GetMembershipCertificates();
}

QStatus Stub::Reset()
{
    // The following implementation does not seem to work.
    ba.ClearKeyStore();
    // This work-around seems to do the trick :)
    string fname = GetHomeDir().c_str();
    fname.append("/");
    fname.append(STUB_KEYSTORE);
    cerr << "Resetting stub fname : " << fname.c_str() << "\n";
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
