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

#include <gtest/gtest.h>

#include "TestUtil.h"
#include "AgentStorageWrapper.h"
#include "AJNCa.h"

#include <qcc/GUID.h>
#include <alljoyn/Status.h>
#include <alljoyn/securitymgr/CertificateUtil.h>

/** @file CertChainHandlingTests.cc */
namespace secmgr_tests {
class CertChainAgentStorageWrapper :
    public AgentStorageWrapper {
  public:
    CertChainAgentStorageWrapper(shared_ptr<AgentCAStorage>& _ca, CertificateX509& rootCert)
        : AgentStorageWrapper(_ca), addIdRootCert(false), addMembershipRootCert(false)
    {
        qcc::String der;
        rootCert.EncodeCertificateDER(der);
        rootIdCert.DecodeCertificateDER(der);
        rootMembership.DecodeCertificateDER(der);
    }

    QStatus RegisterAgent(const KeyInfoNISTP256& agentKey,
                          const Manifest& manifest,
                          GroupInfo& adminGroup,
                          IdentityCertificateChain& identityCertificates,
                          vector<MembershipCertificateChain>& adminGroupMemberships)
    {
        QStatus status = ca->RegisterAgent(agentKey, manifest, adminGroup, identityCertificates, adminGroupMemberships);
        if (ER_OK == status) {
            identityCertificates.push_back(rootIdCert);
            agentIdChain = identityCertificates;
            for (int i = 0; i < 5; i++) {
                MembershipCertificateChain chain;
                char serial[6];
                memset(serial, i + 100, 5);
                serial[5] = 0;
                CreateMembershipChain(chain, serial, agentKey);
                adminGroupMemberships.push_back(chain);
            }
            agentMembershipCertificates = adminGroupMemberships;
        }
        return status;
    }

    virtual QStatus GetMembershipCertificates(const Application& app,
                                              vector<MembershipCertificateChain>& membershipCertificates) const
    {
        QStatus status = ca->GetMembershipCertificates(app, membershipCertificates);
        if ((ER_OK == status) && addMembershipRootCert) {
            for (size_t i = 0; i < membershipCertificates.size(); i++) {
                membershipCertificates[i].push_back(rootMembership);
            }
        }
        return status;
    }

    QStatus GetIdentityCertificatesAndManifest(const Application& app,

                                               IdentityCertificateChain& identityCertificates,
                                               Manifest& manifest) const
    {
        QStatus status = ca->GetIdentityCertificatesAndManifest(app, identityCertificates, manifest);
        if ((ER_OK == status) && addIdRootCert) {
            identityCertificates.push_back(rootIdCert);
        }
        return status;
    }

    virtual QStatus StartApplicationClaiming(const Application& app,
                                             const IdentityInfo& idInfo,
                                             const Manifest& manifest,
                                             GroupInfo& adminGroup,
                                             IdentityCertificateChain& identityCertificates)
    {
        QStatus status = ca->StartApplicationClaiming(app, idInfo, manifest, adminGroup, identityCertificates);
        if ((ER_OK == status) && addIdRootCert) {
            identityCertificates.push_back(rootIdCert);
        }
        return status;
    }

    bool addIdRootCert;
    bool addMembershipRootCert;

    IdentityCertificateChain agentIdChain;
    vector<MembershipCertificateChain> agentMembershipCertificates;

  private:
    CertChainAgentStorageWrapper& operator=(const CertChainAgentStorageWrapper);

    void CreateMembershipChain(MembershipCertificateChain& chain, const char* serial, const KeyInfoNISTP256& agentKey)
    {
        AJNCa tmpCa;
        tmpCa.Init("tmpCA");
        ECCPrivateKey privateKey;
        ECCPublicKey rootKey;
        tmpCa.GetDSAPrivateKey(privateKey);
        tmpCa.GetDSAPublicKey(rootKey);
        MembershipCertificate cert;
        cert.SetCA(false);
        cert.SetSerial((const uint8_t*)serial, strlen(serial));
        CertificateUtil::SetValityPeriod(36000, cert);
        cert.SetSubjectPublicKey(agentKey.GetPublicKey());
        GUID128 group(serial[0]);
        cert.SetGuild(group);
        qcc::String rootkAki;
        CertificateX509::GenerateAuthorityKeyId(&rootKey, rootkAki);
        cert.SetIssuerCN((const uint8_t*)rootkAki.data(), rootkAki.size());
        cert.SetSubjectCN(agentKey.GetKeyId(), agentKey.GetKeyIdLen());
        EXPECT_EQ(ER_OK, cert.SignAndGenerateAuthorityKeyId(&privateKey, &rootKey));
        chain.push_back(cert);

        MembershipCertificate rootCert;
        rootCert.SetCA(true);
        rootCert.SetSerial((const uint8_t*)serial, strlen(serial));
        CertificateUtil::SetValityPeriod(36000, rootCert);
        rootCert.SetGuild(group);
        rootCert.SetSubjectPublicKey(&rootKey);
        rootCert.SetIssuerCN((const uint8_t*)rootkAki.data(), rootkAki.size());
        rootCert.SetSubjectCN((const uint8_t*)rootkAki.data(), rootkAki.size());
        EXPECT_EQ(ER_OK, rootCert.SignAndGenerateAuthorityKeyId(&privateKey, &rootKey));
        chain.push_back(rootCert);
        tmpCa.Reset();
    }

    IdentityCertificate rootIdCert;
    MembershipCertificate rootMembership;
};

#define CHECK_IDENTITY_CHAIN(c) \
    { \
        bool failure = true; \
        CheckIdentyCertificateChain((c), failure, __FUNCTION__, __LINE__); \
        if (failure) { return; } \
    }

#define CHECK_MEMBERSHIP_SUMMARIES() \
    { \
        bool failure = true; \
        CheckMembershipSummaries(failure,  __FUNCTION__, __LINE__); \
        if (failure) { return; } \
    }

class CertChainHandlingTests :
    public ClaimedTest {
  public:
    CertChainHandlingTests() : wrappedCa(nullptr)
    {
        groupInfo.name = "Test";
        groupInfo.desc = "This is a test group";

        policyGroups.push_back(groupInfo.guid);
        AJNCa ajnCa;
        ajnCa.Init(TEST_STORAGE_NAME);
        ECCPrivateKey privateKey;
        ECCPublicKey publicKey;
        ajnCa.GetDSAPrivateKey(privateKey);
        ajnCa.GetDSAPublicKey(publicKey);

        rootCert.SetCA(true);
        rootCert.SetSerial((const uint8_t*)"12345", 5);
        CertificateUtil::SetValityPeriod(36000, rootCert);
        rootCert.SetSubjectPublicKey(&publicKey);
        qcc::String aki;
        CertificateX509::GenerateAuthorityKeyId(&publicKey, aki);
        rootCert.SetIssuerCN((const uint8_t*)aki.data(), aki.size());
        rootCert.SetSubjectCN((const uint8_t*)aki.data(), aki.size());
        rootCert.SignAndGenerateAuthorityKeyId(&privateKey, &publicKey);
    }

    shared_ptr<AgentCAStorage>& GetAgentCAStorage()
    {
        wrappedCa = shared_ptr<CertChainAgentStorageWrapper>(new CertChainAgentStorageWrapper(ca, rootCert));
        ca = wrappedCa;
        return ca;
    }

    void CheckIdentyCertificateChain(IdentityCertificateChain chain, bool& failure, const char* function, int line)
    {
        Manifest ignored;
        IdentityCertificateChain expectedChain;
        ASSERT_EQ(ER_OK, wrappedCa->GetIdentityCertificatesAndManifest(lastAppInfo, expectedChain, ignored))
            << "failure from " << function << "@" << line;
        ASSERT_EQ(expectedChain.size(), chain.size()) << "failure from " << function << "@" << line;

        for (size_t i = 0; i < chain.size(); i++) {
            qcc::String der;
            ASSERT_EQ(ER_OK,
                      chain[i].EncodeCertificateDER(der)) << "failure in cert [" << i << "] " << function << "@" <<
                line;
            qcc::String eDer;
            ASSERT_EQ(ER_OK,
                      expectedChain[i].EncodeCertificateDER(eDer)) << "failure in cert [" << i << "] " << function <<
                "@" <<
                line;;
            ASSERT_EQ(eDer, der) << "failure in cert [" << i << "] " << function << "@" << line;
        }
        failure = false;
    }

    void CheckMembershipSummaries(bool& failure, const char* function, int line)
    {
        vector<MembershipSummary> summaries;
        ASSERT_EQ(ER_OK, GetMembershipSummaries(lastAppInfo, summaries))
            << "failure from " << function << "@" << line;
        vector<MembershipCertificateChain> chains;
        ASSERT_EQ(ER_OK,
                  wrappedCa->GetMembershipCertificates(lastAppInfo,
                                                       chains)) << "failure from " << function << "@" << line;
        ASSERT_EQ(chains.size(), summaries.size()) << "failure from " << function << "@" << line;

        vector<MembershipCertificateChain>::const_iterator localIt;
        for (localIt = chains.begin(); localIt != chains.end(); ++localIt) {
            MembershipCertificate leafCert = (*localIt)[0];
            string serial = string((const char*)leafCert.GetSerial(), leafCert.GetSerialLen());

            bool found = false;
            vector<MembershipSummary>::const_iterator remoteIt;
            for (remoteIt = summaries.begin(); remoteIt != summaries.end(); ++remoteIt) {
                if (serial == remoteIt->serial) {
                    found = true;
                    break;
                }
            }
            ASSERT_TRUE(found) << "failure from " << function << "@" << line << " --> did not find serial number '" <<
                serial << "'";
        }
        failure = false;
    }

    GroupInfo groupInfo;
    vector<GUID128> policyGroups;
    CertificateX509 rootCert;
    shared_ptr<CertChainAgentStorageWrapper> wrappedCa;
};

/**
 * @test Claim an application by presenting the agent an identity certificate chain.
 *       -# Claim an application and provide the agent an identity certificate chain
 *       -# Check whether the application is CLAIMED.
 *       -# Check if the application returns the full identity certificate chain
 **/

TEST_F(CertChainHandlingTests, ClaimChain) {
    IdentityCertificateChain singleIdCertChain;
    ASSERT_EQ(ER_OK, GetIdentity(lastAppInfo, singleIdCertChain));
    CHECK_IDENTITY_CHAIN(singleIdCertChain);
    //Reset the application as it is already claimed.
    ASSERT_EQ(ER_OK, storage->ResetApplication(lastAppInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE));

    wrappedCa->addIdRootCert = true;

    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_OK));
    IdentityCertificateChain idCertChain;
    ASSERT_EQ(ER_OK, GetIdentity(lastAppInfo, idCertChain));
    ASSERT_EQ((size_t)2, idCertChain.size());
    CHECK_IDENTITY_CHAIN(idCertChain);
}

/**
 * @test Install the membership certificates by presenting the agent membership certificate chains.
 *       -# Install a membership certificate chain on application and provide the agent a membership certificate chain
 *       -# Check whether the application is updated successfully.
 *       -# Check if the application returns the full membership certificate chain
 *       -# Install 2 more chains and verify they are found as well
 *       -# Check if these chains can be removed and the application presents an empty list of membership certificates
 **/
TEST_F(CertChainHandlingTests, InstallMembershipChain) {
    wrappedCa->addMembershipRootCert = true;
    storage->StoreGroup(groupInfo);
    ASSERT_EQ(ER_OK, storage->InstallMembership(lastAppInfo, groupInfo));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    vector<MembershipSummary> summaries;
    ASSERT_EQ(ER_OK, GetMembershipSummaries(lastAppInfo, summaries));
    ASSERT_EQ((size_t)1, summaries.size());
    CHECK_MEMBERSHIP_SUMMARIES();
    GroupInfo group2;
    group2.name = "group2";
    GroupInfo group3;
    group2.name = "group3";
    storage->StoreGroup(group2);
    storage->StoreGroup(group3);

    ASSERT_EQ(ER_OK, storage->InstallMembership(lastAppInfo, group2));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    CHECK_MEMBERSHIP_SUMMARIES();
    ASSERT_EQ(ER_OK, storage->InstallMembership(lastAppInfo, group3));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    CHECK_MEMBERSHIP_SUMMARIES();
    ASSERT_EQ(ER_OK, storage->RemoveMembership(lastAppInfo, group3));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    CHECK_MEMBERSHIP_SUMMARIES();
    ASSERT_EQ(ER_OK, storage->RemoveMembership(lastAppInfo, groupInfo));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    CHECK_MEMBERSHIP_SUMMARIES();
    ASSERT_EQ(ER_OK, storage->RemoveMembership(lastAppInfo, group2));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    CHECK_MEMBERSHIP_SUMMARIES();
}

/**
 * @test Update the identity of an already claimed application by presenting the agent an identity
 *    certificate chain.
 *       -# update the identity of an application and provide the agent an identity certificate chain
 *       -# Check whether the application is updated successfully.
 *       -# Check if the application returns the full identity certificate chain
 **/
TEST_F(CertChainHandlingTests, UpdateIdentityChains) {
    IdentityCertificateChain singleIdCertChain;
    ASSERT_EQ(ER_OK, GetIdentity(lastAppInfo, singleIdCertChain));
    CHECK_IDENTITY_CHAIN(singleIdCertChain);
    wrappedCa->addIdRootCert = true;

    ASSERT_EQ(ER_OK, storage->UpdateIdentity(lastAppInfo, idInfo, aa.lastManifest));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    IdentityCertificateChain idCertChain;
    ASSERT_EQ(ER_OK, GetIdentity(lastAppInfo, idCertChain));
    ASSERT_EQ((size_t)2, idCertChain.size());
    CHECK_IDENTITY_CHAIN(idCertChain);
}

/**
 * @test Validate that the register agent is able to handle a identity certificate chain and
 *    multiple membership certificate chains
 *       -# update the identity of an application and provide the agent an identity certificate chain
 *       -# Check if the agent returns a correct membership list
 *       -# Check if the agent returns the full identity certificate chain
 **/
TEST_F(CertChainHandlingTests, DISABLED_RegisterAgent) { // See ASACORE-2543
    OnlineApplication agent;
    agent.busName = ba->GetUniqueName().c_str();

    IdentityCertificateChain idChain;
    ASSERT_EQ(ER_OK, GetIdentity(agent, idChain));
    ASSERT_EQ(wrappedCa->agentIdChain.size(), idChain.size());
    ASSERT_EQ((size_t)2, idChain.size()); //extra check to make sure the test passed the full chain.
    for (size_t i = 0; i < idChain.size(); i++) {
        qcc::String exp;
        qcc::String got;
        idChain[i].EncodeCertificateDER(got);
        wrappedCa->agentIdChain[i].EncodeCertificateDER(exp);
        ASSERT_EQ(exp, got) << "i = " << i;
    }

    vector<MembershipSummary> summaries;
    ASSERT_EQ(ER_OK, GetMembershipSummaries(agent, summaries));
    ASSERT_EQ(wrappedCa->agentMembershipCertificates.size(), summaries.size());
    ASSERT_EQ((size_t)6, summaries.size()); //extra check to make sure the test passed the extra chains.
    for (size_t i = 0; i < wrappedCa->agentMembershipCertificates.size(); i++) {
        MembershipCertificate mCert = (wrappedCa->agentMembershipCertificates[i])[0];
        string serial = string((const char*)mCert.GetSerial(), mCert.GetSerialLen());
        bool found = false;
        for (vector<MembershipSummary>::iterator summaryIt = summaries.begin();
             summaryIt != summaries.end();
             ++summaryIt) {
            if (serial == summaryIt->serial) {
                found = true;
                summaries.erase(summaryIt);
                break;
            }
        }
        ASSERT_TRUE(found) << "loop " << i << ": serialNr = " << serial;
    }
    ASSERT_EQ((size_t)0, summaries.size());
}
}
