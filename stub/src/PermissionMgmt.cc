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

#include "PermissionMgmt.h"

qcc::String PermissionMgmt::PubKeyToString(const qcc::ECCPublicKey* pubKey)
{
    qcc::String str;
    for (int i = 0; i < (int)qcc::ECC_PUBLIC_KEY_SZ; ++i) {
        char buff[4];
        sprintf(buff, "%02x", (unsigned char)(pubKey->data[i]));
        str = str + buff;
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
        if (CLAIMABLE != claimableState) {
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
        uint8_t* roTKey;
        size_t numOfBytes;
        QStatus extraction = inputArg->Get("ay", &numOfBytes, &roTKey);
        if (ER_OK != extraction) {
            printf("Claim: Error extracting RoT key from input argument.\n");
            errorStr.append("RoT key invalid: extraction error");
            break;
        }

        // Verify key
        if (qcc::ECC_PUBLIC_KEY_SZ != numOfBytes) {
            printf("Claim: Error RoT key has wrong number of bytes: %zu iso. %zu. \n",
                   numOfBytes,
                   qcc::ECC_PUBLIC_KEY_SZ);
            errorStr.append("RoT key invalid: wrong number of bytes");
            break;
        }

        // Copy the key data
        qcc::ECCPublicKey* pubKeyRoT = new qcc::ECCPublicKey();
        memcpy((void*)&pubKeyRoT->data, (void*)roTKey, qcc::ECC_PUBLIC_KEY_SZ);
        pubKeyRoTs.push_back(pubKeyRoT);

        // Printout valid RoT pub key
        qcc::String printableRoTKey = PubKeyToString(pubKeyRoTs.back());
        printf("\nReceived RoT pubKey: %s \n", printableRoTKey.c_str());

        if (NULL != cl && false == cl->OnClaimRequest(pubKeyRoTs.back(), ctx)) {
            printf("User refused to be claimed..\r\n");
            return;
        }

        //====================================================
        // Step 2: send public key as response back.

        // Send public Key back as response
        ajn::MsgArg output;
        const uint8_t(&keyBytes)[qcc::ECC_PUBLIC_KEY_SZ] = PermissionMgmt::crypto->GetDHPublicKey()->data;
        output.Set("ay", qcc::ECC_PUBLIC_KEY_SZ, keyBytes);
        if (ER_OK != MethodReply(msg, &output, 1)) {
            printf("Claim: Error sending reply.\n");
        }

        // Printout valid pub key
        qcc::String printablePubKey = PubKeyToString(crypto->GetDHPublicKey());
        printf("\nSending App public Key: %s \n", printablePubKey.c_str());

        ok = true;
        printf("========> CLAIM RETURNS <=========\n");
    } while (0);

    // Send error response back:tl2i4VrV.2
    if (true == ok) {
        if (cl != NULL) {
            cl->OnClaimed(ctx);
            /* This device is claimed */
            claimableState = CLAIMED;
            /* Sometimes this signal is not received on the other side */
            SendClaimDataSignal();
        }
    } else {
        if (ER_OK != MethodReply(msg, "org.alljoyn.Security.PermissionMgmt.ClaimError", errorStr.c_str())) {
            printf("Claim: Error sending reply.\n");
        }
    }
}

void PermissionMgmt::InstallIdentity(const ajn::InterfaceDescription::Member* member, ajn::Message& msg)
{
    pemIdentityCertificate = qcc::String(msg->GetArg(0)->v_string.str);

    printf("\nReceived Identity certificate (PEM): %s\n", pemIdentityCertificate.c_str());

    ajn::MsgArg outArg("b", true);
    if (ER_OK != MethodReply(msg, &outArg, 1)) {
        printf("InstallIdentity: Error sending reply.\n");
    }
    if (cl != NULL) {
        cl->OnIdentityInstalled(pemIdentityCertificate);
    }
}

PermissionMgmt::PermissionMgmt(ajn::BusAttachment& ba,
                               ClaimListener* _cl,
                               void* _ctx) :
    ajn::BusObject("/security/PermissionMgmt"),
    pubKeyRoTs(),
    cl(_cl),
    claimableState(UNCLAIMED),
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
          static_cast<MessageReceiver::MethodHandler>(&PermissionMgmt::InstallIdentity) }
    };
    QStatus status = AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
    if (ER_OK != status) {
        printf("Failed to register method handlers for PermissionMgmt.\n");
    }

    /* Get the signal member */
    unsecInfoSignalMember = unsecPermIntf->GetMember("SecInfo");
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
#if ONLYCLAIMSTATE
    MsgArg claimData("y", claimableState);
#else
    /* Create a MsgArg array of 3 elements */
    MsgArg claimData[3];

    /* Fill the first element with a byte array of your own public key or 0 */
    claimData[0].Set("ay", (crypto) ? qcc::ECC_PUBLIC_KEY_SZ : 0, (crypto) ? crypto->GetDHPublicKey()->data : NULL);

    /* The second element is the current claimable state */
    claimData[1].Set("y", claimableState);

    /* The third element is an array of all the RoTs public keys */
    MsgArg RoTs[(pubKeyRoTs.size()) ? pubKeyRoTs.size() : 1];
    int i = 0;
    if (!pubKeyRoTs.size()) {
        /* if there are not yet RoTs, fill with 0 */
        RoTs[0].Set("ay", 0, NULL);
    } else {
        for (std::vector<qcc::ECCPublicKey*>::const_iterator it = pubKeyRoTs.begin();
             it != pubKeyRoTs.end();
             ++it) {
            RoTs[i].Set("ay", qcc::ECC_PUBLIC_KEY_SZ, (*it)->data);
            i++;
        }
    }

    /* use the RoTs array to fill the third element */
    claimData[2].Set("aay", pubKeyRoTs.size(), RoTs);
#endif
    uint8_t flags = ALLJOYN_FLAG_SESSIONLESS;

#if ONLYCLAIMSTATE
    QStatus status = Signal(NULL, 0, *unsecInfoSignalMember, &claimData, 1, 0, flags);
#else
    QStatus status = Signal(NULL, 0, *unsecInfoSignalMember, &claimData[0], 3, 0, flags);
#endif
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
#if ONLYCLAIMSTATE

    QStatus status = ba.CreateInterface(SECINTFNAME, secIntf);
#else
    QStatus status = ba.CreateInterface(SECINTFNAME, secIntf, AJ_IFC_SECURITY_REQUIRED);
#endif
    if (ER_OK == status) {
        printf("Secure Interface created.\n");
        secIntf->AddMethod("Claim", "ay",  "ay", "rotPublicKey,appPublicKey", 0);
        secIntf->AddMethod("InstallIdentity", "s", "b", "PEMofIdentityCert,result", 0);
        secIntf->Activate();
    } else {
        printf("Failed to create Secure PermissionMgmt interface.\n");
        return status;
    }

    /* Create an unsecured interface for the purpose of sending a broadcast, sessionless signal */
    status = ba.CreateInterface(UNSECINTFNAME, unsecIntf);

    if (ER_OK == status) {
        printf("Unsecured Interface created.\n");

#if ONLYCLAIMSTATE
        unsecIntf->AddSignal("SecInfo", "y", "claimableState", 0);
#else
        /* publicKey: own public key
         * claimableState: enum ClaimableState
         * rotPublicKeys: array of rot public keys
         */
        unsecIntf->AddSignal("SecInfo", "ayyaay", "publicKey,claimableState,rotPublicKeys", 0);
#endif
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
        claimableState = CLAIMABLE;
        SendClaimDataSignal();
    } else {
        claimableState = (pubKeyRoTs.size()) ? CLAIMED : UNCLAIMED;
        SendClaimDataSignal();
    }
}

/* TODO: add a function that allows the application to reset the claimable state
 * and maybe even the public keys and certificates */

ClaimableState PermissionMgmt::GetClaimableState() const
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
