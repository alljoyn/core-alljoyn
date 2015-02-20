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

#include "PermissionMgmt.h"

#include "qcc/Crypto.h"

#include <SecurityManagerImpl.h>
#include <X509CertificateParser.h>

qcc::String PermissionMgmt::PubKeyToString(const qcc::ECCPublicKey* pubKey)
{
    qcc::String str = "";

    if (pubKey) {
        for (int i = 0; i < (int)qcc::ECC_COORDINATE_SZ; ++i) {
            char buff[4];
            sprintf(buff, "%02x", (unsigned char)(pubKey->x[i]));
            str = str + buff;
        }
        for (int i = 0; i < (int)qcc::ECC_COORDINATE_SZ; ++i) {
            char buff[4];
            sprintf(buff, "%02x", (unsigned char)(pubKey->y[i]));
            str = str + buff;
        }
    }
    return str;
}

void PermissionMgmt::Claim(const ajn::InterfaceDescription::Member* member, ajn::Message& msg)
{
    printf("========> CLAIM CALLED <=========\n");
    bool ok = false;

    qcc::String errorStr = "Claim: ";
    do {
        /* if not claimable break and error */
        if (ajn::PermissionConfigurator::STATE_CLAIMABLE != claimableState) {
            printf("Claim: claim request, but not allowed.\n");
            errorStr.append("Claiming not allowed");
            break;
        }
        // Check if already claimed
        // TODO: what to do when already claimed ??
        if (0 != pubKeyRoTs.size()) {
            printf("Claim: claim request, but already claimed.\n");
            // for now we just continue
        }

        //====================================================
        // Step 1: Get input argument and verify Rot
        //
        const ajn::MsgArg* inputArg = msg->GetArg(0);
        if (NULL == inputArg) {
            printf("Claim: Error missing input argument.\n");
            errorStr.append("RoT key missing");
            break;
        }

        QStatus status;
        qcc::ECCPublicKey* pubKeyRoT = new qcc::ECCPublicKey();

        status = ajn::securitymgr::SecurityManagerImpl::UnmarshalPublicKey(inputArg, *pubKeyRoT);
        if (ER_OK != status) {
            printf("Claim: failed to unmarshal public key");
            break;
        }
        pubKeyRoTs.push_back(pubKeyRoT);

        // Printout valid RoT pub key
        qcc::String printableRoTKey = PubKeyToString(pubKeyRoTs.back());
        printf("\nReceived RoT pubKey: %s \n", printableRoTKey.c_str());

        if (NULL != cl && false == cl->OnClaimRequest(pubKeyRoTs.back(), ctx)) {
            printf("User refused to be claimed..\r\n");
            return;
        }

        //====================================================
        // Step 1b: Install new identity certificate

        if (ER_OK != (status = InstallIdentityCertificate((MsgArg &) * msg->GetArg(1)))) {
            printf("Failed to install identity certificate\n");
            break;
        }

        //====================================================
        // Step 2: send public key as response back.

        // Send public Key back as response
        ajn::MsgArg output;
        status = ajn::securitymgr::SecurityManagerImpl::MarshalPublicKey(crypto->GetDHPublicKey(),
                                                                         peerID, output);

        if (ER_OK != MethodReply(msg, &output, 1)) {
            printf("Claim: Error sending reply.\n");
        }

        // Printout valid pub key
        qcc::String printablePubKey = PubKeyToString(crypto->GetDHPublicKey());
        printf("\nSending App public Key: %s \n", printablePubKey.c_str());

        ok = true;
        printf("========> CLAIM RETURNS <=========\n");
    } while (0);

    // Send error response back
    if (true == ok) {
        if (cl != NULL) {
            cl->OnClaimed(ctx);
            /* This device is claimed */
            claimableState = ajn::PermissionConfigurator::STATE_CLAIMED;
            /* Sometimes this signal is not received on the other side */
            SendClaimDataSignal();
        }
    } else {
        if (ER_OK != MethodReply(msg, "org.alljoyn.Security.PermissionMgmt.ClaimError", errorStr.c_str())) {
            printf("Claim: Error sending reply.\n");
        }
    }
}

QStatus PermissionMgmt::InstallIdentityCertificate(ajn::MsgArg& msgArg)
{
    uint8_t encoding;
    uint8_t* encoded;
    size_t encodedLen;
    QStatus status = msgArg.Get("(yay)", &encoding, &encodedLen, &encoded);
    if (ER_OK != status) {
        printf("PermissionMgmtObj::InstallIdentity failed to retrieve PEM status 0x%x", status);
        return status;
    }
    if ((encoding != Certificate::ENCODING_X509_DER) && (encoding != Certificate::ENCODING_X509_DER_PEM)) {
        printf("PermissionMgmtObj::InstallIdentity does not support encoding %d", encoding);
        status = ER_NOT_IMPLEMENTED;
        return status;
    }

    pemIdentityCertificate = qcc::String((const char*)encoded, encodedLen);
    printf("\nInstalled Identity certificate (PEM): '%s'\n", pemIdentityCertificate.c_str());

    if (cl != NULL) {
        cl->OnIdentityInstalled(pemIdentityCertificate);
    }

    return ER_OK;
}

void PermissionMgmt::InstallIdentity(const ajn::InterfaceDescription::Member* member, ajn::Message& msg)
{
    QStatus status = InstallIdentityCertificate((MsgArg &) * msg->GetArg(0));
    MethodReply(msg, status);
}

void PermissionMgmt::InstallMembership(const ajn::InterfaceDescription::Member* member, ajn::Message& msg)
{
    size_t certChainCount;
    MsgArg* certChain;
    QStatus status = msg->GetArg(0)->Get("a(yay)", &certChainCount, &certChain);
    if (status != ER_OK || certChainCount != 1) {
        printf("Bad message status = %d,  count = %d\n", status, (int)certChainCount);
        MethodReply(msg, status);
        return;
    }
    uint8_t encoding;
    uint8_t* encoded;
    size_t encodedLen;
    status = certChain[0].Get("(yay)", &encoding, &encodedLen, &encoded);
    if (status != ER_OK || encoding != Certificate::ENCODING_X509_DER_PEM) {
        printf("Bad Cert in message status = %d,  encoding = %d\n", status, (int)encoding);
        MethodReply(msg, status);
        return;
    }
    qcc::String certificate((char*)encoded, encodedLen);
    qcc::String serialNumber = ajn::securitymgr::X509CertificateParser::GetSerialNumber(certificate);
    qcc::GUID128 guildID = ajn::securitymgr::X509CertificateParser::GetGuildID(certificate);

    printf("\nInstalling Membership certificate for guild %s with serial number %s\n%s\n",
           guildID.ToString().c_str(), serialNumber.c_str(), certificate.c_str());

    memberships[guildID] = certificate;

    MethodReply(msg, ER_OK);

    if (cl != NULL) {
        cl->OnMembershipInstalled(certificate);
    }
}

void PermissionMgmt::RemoveMembership(const ajn::InterfaceDescription::Member* member, ajn::Message& msg)
{
    QStatus status;
    const char* serial;
    status = msg->GetArg(0)->Get("s", &serial);
    if (ER_OK != status) {
        printf("Could not get serial.\n");
        MethodReply(msg, status);
        return;
    }

    uint8_t* issuer;
    size_t issuerLen;
    status = msg->GetArg(1)->Get("ay", &issuerLen, &issuer);
    if (ER_OK != status) {
        printf("Gould not get issuer.\n");
        MethodReply(msg, status);
        return;
    }
    if (issuerLen != GUID128::SIZE) {
        MethodReply(msg, ER_INVALID_DATA);
        return;
    }

    qcc::String serialNum(serial);

    std::map<qcc::GUID128, qcc::String>::iterator it = memberships.begin();
    while (it != memberships.end()) {
        if (ajn::securitymgr::X509CertificateParser::GetSerialNumber(it->second) == serialNum) {
            break;
        }
        ++it;
    }
    if (it != memberships.end()) {
        qcc::GUID128 guildID = ajn::securitymgr::X509CertificateParser::GetGuildID(it->second);
        memberships.erase(it);
        printf("Removed membership certificate for guild %s with serial number %s\n",
               guildID.ToString().c_str(), serialNum.c_str());
        status = ER_OK;
    } else {
        printf("Could not find membership certificate with serial number %s\n", serialNum.c_str());
        status = ER_CERTIFICATE_NOT_FOUND;
    }

    MethodReply(msg, status);
}

void PermissionMgmt::InstallMembershipAuthData(const ajn::InterfaceDescription::Member* member,
                                               ajn::Message& msg)
{
    printf("InstallMembershipAuthData\n");

    // serial number
    const char* serial;
    QStatus status = msg->GetArg(0)->Get("s", &serial);
    if (ER_OK != status) {
        printf("Could not get serial.\n");
        MethodReply(msg, status);
        return;
    }
    printf("serial: %s\n", serial);

    // issuer
    uint8_t* issuer;
    size_t issuerLen;
    status = msg->GetArg(1)->Get("ay", &issuerLen, &issuer);
    if (ER_OK != status) {
        printf("Could not get issuer.\n");
        MethodReply(msg, status);
        return;
    }
    if (issuerLen != GUID128::SIZE) {
        MethodReply(msg, ER_INVALID_DATA);
        printf("Invalid issuer size.\n");
        return;
    }
    GUID128 issuerGUID(0);
    issuerGUID.SetBytes(issuer);
    printf("issuerGuid: %s\n", issuerGUID.ToString().c_str());

    // authorization data
    PermissionPolicy policy;
    uint8_t version;
    MsgArg* variant;
    status = msg->GetArg(2)->Get("(yv)", &version, &variant);
    if (ER_OK != status) {
        MethodReply(msg, status);
        printf("Could not get version/variant.\n");
        return;
    }
    policy.Import(version, *variant);
    printf("authData: %s\n", policy.ToString().c_str());

    if (cl != NULL) {
        cl->OnAuthData(&policy);
    }

    MethodReply(msg, status);
}

void PermissionMgmt::GetManifest(const ajn::InterfaceDescription::Member* member, ajn::Message& msg)
{
    printf("Received GetManifest request\n");
    ajn::MsgArg outArg;

    uint8_t type = 0;
    MsgArg variant;
    PermissionPolicy::GenerateRules(manifestRules, manifestRulesCount, variant);
    outArg.Set("(yv)", &type, &variant);

    if (ER_OK != MethodReply(msg, &outArg, 1)) {
        printf("GetManifest: Error sending reply.\n");
    }
}

void PermissionMgmt::InstallPolicy(const ajn::InterfaceDescription::Member* member, ajn::Message& msg)
{
    QStatus status = ER_FAIL;
    uint8_t version;
    MsgArg* variant;
    msg->GetArg(0)->Get("(yv)", &version, &variant);

    if (ER_OK != (status = policy.Import(version, *variant))) {
        printf("InstallPolicy: Failed to unmarshal policy.");
        //return;
    } else {
        printf("InstallPolicy: Received policy\n %s", policy.ToString().data());
    }
    if (ER_OK != ((status == ER_OK) ? MethodReply(msg) : MethodReply(msg, status))) {
        printf("InstallPolicy: Error sending reply.\n");
    }

    if (cl != NULL) {
        cl->OnPolicyInstalled(policy);
    }
}

void PermissionMgmt::GetPolicy(const ajn::InterfaceDescription::Member* member, ajn::Message& msg)
{
    MsgArg replyArg;
    policy.Export(replyArg);

    printf("GetPolicy: Received request\n");

    MethodReply(msg, &replyArg, 1);
}

PermissionMgmt::PermissionMgmt(ajn::BusAttachment& ba,
                               ClaimListener* _cl,
                               void* _ctx) :
    ajn::BusObject("/security/PermissionMgmt"),
    pubKeyRoTs(),
    cl(_cl),
    claimableState(ajn::PermissionConfigurator::STATE_UNCLAIMABLE),
    ctx(_ctx)
{
    crypto = NULL;

    /* Secure permissions interface */
    const ajn::InterfaceDescription* secPermIntf = ba.GetInterface(SECINTFNAME);
    assert(secPermIntf);
    AddInterface(*secPermIntf);

    /* unsecure permissions interface */
    const ajn::InterfaceDescription* unsecPermIntf = ba.GetInterface(UNSECINTFNAME);
    assert(unsecPermIntf);
    AddInterface(*unsecPermIntf);

    /* Register the method handlers with the object */
    const MethodEntry methodEntries[] = {
        { secPermIntf->GetMember("Claim"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmt::Claim) },
        { secPermIntf->GetMember("InstallIdentity"),
          static_cast<MessageReceiver::MethodHandler>(&PermissionMgmt::InstallIdentity) },
        { secPermIntf->GetMember("InstallMembership"),
          static_cast<MessageReceiver::MethodHandler>(&PermissionMgmt::InstallMembership) },
        { secPermIntf->GetMember("RemoveMembership"),
          static_cast<MessageReceiver::MethodHandler>(&PermissionMgmt::RemoveMembership) },

        { secPermIntf->GetMember("InstallMembershipAuthData"),
          static_cast<MessageReceiver::MethodHandler>(&PermissionMgmt::InstallMembershipAuthData) },
        { secPermIntf->GetMember("GetManifest"),
          static_cast<MessageReceiver::MethodHandler>(&PermissionMgmt::GetManifest) },
        { secPermIntf->GetMember("InstallPolicy"),
          static_cast<MessageReceiver::MethodHandler>(&PermissionMgmt::InstallPolicy) },
        { secPermIntf->GetMember("GetPolicy"),
          static_cast<MessageReceiver::MethodHandler>(&PermissionMgmt::GetPolicy) }
    };
    QStatus status = AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
    if (ER_OK != status) {
        printf("Failed to register method handlers for PermissionMgmt.\n");
    }

    /* Get the signal member */
    unsecInfoSignalMember = unsecPermIntf->GetMember("NotifyConfig");
    assert(unsecInfoSignalMember);

    /* Create a new public key, for the stub this can be done,
     * for a real application it would be needed the public key is persistent
     */
    printf("PermissionMgmt() crypto = '%p'", crypto);

    if (!crypto) {
        crypto = new qcc::Crypto_ECC();
        if (ER_OK != crypto->GenerateDHKeyPair()) {
            printf("Claim: Error generating key pair for reply.\n");
        }
    }
}

PermissionMgmt::~PermissionMgmt()
{
    if (0 != pubKeyRoTs.size()) {
        while (!pubKeyRoTs.empty()) {
            delete pubKeyRoTs.back();
            pubKeyRoTs.pop_back();
        }
    }
    printf("~PermissionMgmt() crypto = '%p'", crypto);
    if (NULL != crypto) {
        delete crypto;
        crypto = NULL;
    }
}

QStatus PermissionMgmt::SendClaimDataSignal()
{
    printf("Send the claimingInfo.\n");
    /* Create a MsgArg array of 6 elements */
    //"NotifyConfig", "qa(yv)ya(yv)ua(ayay)", "version,publicKeyInfo,claimableState,trustAnchors,serialNumber,memberships"

    MsgArg claimData[6];
    claimData[0].Set("q", 0);

    /* Fill the second element with the public key info ID */
    MsgArg elements[1];
    QStatus status = ajn::securitymgr::SecurityManagerImpl::MarshalPublicKey(crypto->GetDHPublicKey(),
                                                                             peerID, elements[0]);
    claimData[1].Set("a(yv)", 1, elements);

    /* The third element is the current claimable state */
    claimData[2].Set("y", claimableState);

    claimData[3].Set("a(yv)", 0, NULL);

    /* The fifth element should be the serial number of the policy */
    claimData[4].Set("u", 0);

    /* The fourth elements should contain a list of membership certificates */
    MsgArg memberCerts[1];
    memberCerts[0].Set("(ayay)", 0, NULL);
    claimData[5].Set("a(ayay)", 0, memberCerts);

    uint8_t flags = ALLJOYN_FLAG_SESSIONLESS;
    status = Signal(NULL, 0, *unsecInfoSignalMember, &claimData[0], 6, 0, flags);
    if (ER_OK != status) {
        printf("Signal returned an error %s.\n", QCC_StatusText(status));
    }
    return status;
}

/* This creates the interface for that specific BusAttachment */
QStatus PermissionMgmt::CreateInterface(BusAttachment& ba)
{
    InterfaceDescription* secIntf = NULL;
    InterfaceDescription* unsecIntf = NULL;
    QStatus status = ba.CreateInterface(SECINTFNAME, secIntf, AJ_IFC_SECURITY_REQUIRED);
    if (ER_OK == status) {
        printf("Secure Interface created.\n");
        secIntf->AddMethod("Claim", "(yv)(yay)", "(yv)", "adminPublicKey,GUID,identityCert,publicKey");
        secIntf->AddMethod("InstallIdentity", "(yay)", NULL, "PEMofIdentityCert", 0);
        secIntf->AddMethod("InstallMembership", "a(yay)", NULL, "cert", 0);
        secIntf->AddMethod("RemoveMembership",     "say", NULL, "serialNum,issuer");
        secIntf->AddMethod("GetManifest", NULL, "(yv)",  "manifest");
        secIntf->AddMethod("InstallMembershipAuthData", "say(yv)",  NULL, "serialNum,issuer,authorization");
        secIntf->AddMethod("InstallPolicy", "(yv)", NULL, "authorization");
        secIntf->AddMethod("GetPolicy", NULL, "(yv)", "authorization");
        secIntf->Activate();
    } else {
        printf("Failed to create Secure PermissionMgmt interface.\n");
        return status;
    }

    /* Create an unsecured interface for the purpose of sending a broadcast, sessionless signal */
    status = ba.CreateInterface(UNSECINTFNAME, unsecIntf);

    if (ER_OK == status) {
        printf("Unsecured Interface created.\n");

        /* publicKey: own public key
         * claimableState: enum ClaimableState
         * rotPublicKeys: array of rot public keys
         */
        unsecIntf->AddSignal("NotifyConfig",
                             "qa(yv)ya(yv)ua(ayay)",
                             "version,publicKeyInfo,claimableState,trustAnchors,serialNumber,memberships",
                             0);
        unsecIntf->Activate();
    } else {
        printf("Failed to create Unsecured PermissionsMgmt interface.\n");
    }

    return status;
}

/* TODO: maybe add a timer that keeps the claimable state open for x time */
void PermissionMgmt::SetClaimableState(bool on)
{
    if (true == on) {
        claimableState = ajn::PermissionConfigurator::STATE_CLAIMABLE;
        SendClaimDataSignal();
    } else {
        claimableState =
            (pubKeyRoTs.size()) ? ajn::PermissionConfigurator::STATE_CLAIMED : ajn::PermissionConfigurator::
            STATE_UNCLAIMABLE;
        SendClaimDataSignal();
    }
}

/* TODO: add a function that allows the application to reset the claimable state
 * and maybe even the public keys and certificates */

ajn::PermissionConfigurator::ClaimableState PermissionMgmt::GetClaimableState() const
{
    return claimableState;
}

std::vector<qcc::ECCPublicKey*> PermissionMgmt::GetRoTKeys() const
{
    return pubKeyRoTs;
}

qcc::String PermissionMgmt::GetInstalledIdentityCertificate() const
{
    return pemIdentityCertificate;
}

std::map<GUID128, qcc::String> PermissionMgmt::GetMembershipCertificates() const
{
    return memberships;
}

void PermissionMgmt::SetUsedManifest(PermissionPolicy::Rule* manifestRules,
                                     size_t manifestRulesCount)
{
    this->manifestRules = manifestRules;
    this->manifestRulesCount = manifestRulesCount;
}

void PermissionMgmt::GetUsedManifest(PermissionPolicy::Rule* manifestRules,
                                     size_t manifestRulesCount) const
{
    manifestRules = this->manifestRules;
    manifestRulesCount = this->manifestRulesCount;
}
