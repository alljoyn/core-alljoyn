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

#include <alljoyn/securitymgr/Manifest.h>
#include <alljoyn/securitymgr/Util.h>

#define QCC_MODULE "SECMGR_AGENT"

using namespace std;

namespace ajn {
namespace securitymgr {
Manifest::Manifest() : byteArray(nullptr), size(0)
{
}

Manifest::Manifest(const Manifest& other)
{
    if (other.size > 0) {
        size = other.size;
        byteArray = new uint8_t[size];
        memcpy(byteArray, other.byteArray, size);
        manifest = other.manifest;
    } else {
        byteArray = nullptr;
        size = 0;
    }
}

Manifest::~Manifest()
{
    delete[]byteArray;
    byteArray = nullptr;
}

Manifest::Manifest(const uint8_t* manifestByteArray, const size_t _size) : byteArray(nullptr), size(0)
{
    QStatus status;
    if (ER_OK != (status = SetFromByteArray(manifestByteArray, _size))) {
        QCC_LogError(status, ("Failed to create manifest"));
    }
}

Manifest::Manifest(const PermissionPolicy::Rule* _rules,
                   const size_t manifestRulesCount) : byteArray(nullptr), size(0)
{
    QStatus status = ER_OK;
    if (_rules && manifestRulesCount != 0) {
        status = SetFromRules(_rules, manifestRulesCount);
    }
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to create manifest"));
    }
}

QStatus Manifest::GetByteArray(uint8_t** manifestByteArray, size_t* _size) const
{
    if (!manifestByteArray) {
        return ER_BAD_ARG_1;
    }
    if (!_size) {
        return ER_BAD_ARG_2;
    }

    *manifestByteArray = nullptr;
    *_size = 0;
    if (byteArray != nullptr && size > 0) {
        *manifestByteArray = new uint8_t[size];
        *_size = size;
        memcpy(*manifestByteArray, byteArray, size);
        return ER_OK;
    }

    return ER_END_OF_DATA;
}

QStatus Manifest::GetRules(PermissionPolicy::Rule** manifestRules,
                           size_t* manifestRulesNumber) const
{
    if (!(manifestRules)) {
        return ER_BAD_ARG_1;
    }
    if (!manifestRulesNumber) {
        return ER_BAD_ARG_2;
    }

    QStatus status = ER_OK;

    if (manifest.GetAclsSize() <= 0) {
        return ER_END_OF_DATA;
    }

    size_t numOfRules = manifest.GetAcls()[0].GetRulesSize();

    if (numOfRules <= 0) {
        *manifestRules = nullptr;
        *manifestRulesNumber = 0;
        return ER_END_OF_DATA;
    }

    PermissionPolicy::Rule* localRules =  const_cast<PermissionPolicy::Rule*>(manifest.GetAcls()[0].GetRules());
    PermissionPolicy::Rule* ruleArray = new PermissionPolicy::Rule[numOfRules];

    for (size_t i = 0; i < numOfRules; i++) {
        ruleArray[i] = localRules[i];
    }
    *manifestRulesNumber = numOfRules;
    *manifestRules = ruleArray;

    return status;
}

const size_t Manifest::GetRulesSize() const
{
    if (manifest.GetAclsSize() <= 0) {
        return 0;
    }

    return manifest.GetAcls()[0].GetRulesSize();
}

QStatus Manifest::GetDigest(uint8_t* digest) const
{
    if (!digest) {
        return ER_BAD_ARG_1;
    }
    if (byteArray == nullptr || size <= 0) {
        return ER_END_OF_DATA;
    }

    QStatus status = ER_FAIL;

    PermissionPolicy::Rule* rules;
    size_t count;
    status = GetRules(&rules, &count);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to GetRules"));
    }

    Message* msg = nullptr;
    DefaultPolicyMarshaller* marshaller = Util::GetDefaultMarshaller(&msg);

    if (marshaller) {
        status = marshaller->Digest(rules, count, digest, Crypto_SHA256::DIGEST_SIZE);
    }

    delete[] rules;
    delete msg;
    delete marshaller;

    return status;
}

QStatus Manifest::SetFromByteArray(const uint8_t* manifestByteArray, const size_t _size)
{
    QStatus status = ER_FAIL;

    if (!manifestByteArray) {
        return ER_BAD_ARG_1;
    }

    if (0 == _size) {
        return ER_BAD_ARG_2;
    }

    // Reconstruct policy containing manifest
    Message* msg = nullptr;
    DefaultPolicyMarshaller* marshaller = Util::GetDefaultMarshaller(&msg);
    if (marshaller) {
        if (ER_OK == (status = manifest.Import(*marshaller, manifestByteArray, _size))) {
            size = _size;
            byteArray = new uint8_t[size];
            memcpy(byteArray, manifestByteArray, size);
        }
    }

    delete msg;
    delete marshaller;

    return status;
}

QStatus Manifest::SetFromRules(const PermissionPolicy::Rule* manifestRules, const size_t manifestRulesNumber)
{
    if (!manifestRules) {
        return ER_BAD_ARG_1;
    }

    if (0 == manifestRulesNumber) {
        return ER_BAD_ARG_2;
    }

    QStatus status = ER_FAIL;

    PermissionPolicy::Acl* acls = new PermissionPolicy::Acl[1];
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[manifestRulesNumber];

    for (size_t i = 0; i < manifestRulesNumber; i++) {
        rules[i] = manifestRules[i]; //copies members
    }
    acls[0].SetRules(manifestRulesNumber, rules);
    manifest.SetAcls(1, acls);

    delete[] acls;
    acls = nullptr;
    delete[] rules;
    rules = nullptr;

    // Serialize wrapped manifest to a byte array
    uint8_t* buf = nullptr;
    size_t _size = 0;

    Message* msg = nullptr;
    DefaultPolicyMarshaller* marshaller = Util::GetDefaultMarshaller(&msg);
    if (marshaller) {
        status = manifest.Export(*marshaller, &buf, &_size);

        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to serialize manifest"));
            goto Exit;
        }

        size = _size;
        byteArray = new uint8_t[size];
        memcpy(byteArray, buf, size);
    }

Exit:
    delete[]buf;
    buf = nullptr;
    delete msg;
    delete marshaller;

    return status;
}

string Manifest::ToString() const
{
    return manifest.ToString().c_str();
}

Manifest& Manifest::operator=(const Manifest& rhs)
{
    if (this == &rhs) {
        return *this;
    }

    manifest = rhs.manifest;
    delete[]byteArray;
    byteArray = nullptr;
    size = 0;

    if (rhs.size > 0) {
        size = rhs.size;
        byteArray = new uint8_t[size];
        memcpy(byteArray, rhs.byteArray, size);
    }
    return *this;
}

bool Manifest::operator==(const Manifest& other) const
{
    return (manifest == other.manifest);
}

bool Manifest::operator!=(const Manifest& other) const
{
    return (manifest != other.manifest);
}

QStatus Manifest::Difference(const Manifest& rhs,
                             Manifest& result) const
{
    QStatus status = ER_FAIL;

    PermissionPolicy::Rule* lhsRules = nullptr;
    size_t lhsSize;
    status = GetRules(&lhsRules, &lhsSize);
    if (ER_OK != status) {
        return status;
    }

    PermissionPolicy::Rule* rhsRules = nullptr;
    size_t rhsSize;
    status = rhs.GetRules(&rhsRules, &rhsSize);
    if (ER_OK != status) {
        return status;
    }

    vector<PermissionPolicy::Rule> lRules(lhsRules, lhsRules + lhsSize);
    vector<PermissionPolicy::Rule> rRules(rhsRules, rhsRules + rhsSize);

    // iterate over lhs rules and remove rules that can be matched on rhs
    vector<PermissionPolicy::Rule>::iterator lr;
    for (lr = lRules.begin(); lr != lRules.end();) {
        vector<PermissionPolicy::Rule::Member> lMembers(lr->GetMembers(),
                                                        lr->GetMembers() + lr->GetMembersSize());
        vector<PermissionPolicy::Rule>::iterator rr;
        for (rr = rRules.begin(); rr != rRules.end(); ++rr) {
            if ((lr->GetInterfaceName() != rr->GetInterfaceName()) ||
                (lr->GetObjPath() != rr->GetObjPath())) {
                continue;
            }

            // found a matching rhs rule
            vector<PermissionPolicy::Rule::Member>::iterator lm;
            for (lm = lMembers.begin(); lm != lMembers.end();) {
                vector<PermissionPolicy::Rule::Member> rMembers(rr->GetMembers(),
                                                                rr->GetMembers() + rr->GetMembersSize());
                vector<PermissionPolicy::Rule::Member>::iterator rm;
                for (rm = rMembers.begin(); rm != rMembers.end(); ++rm) {
                    if ((lm->GetMemberName() != rm->GetMemberName()) ||
                        (lm->GetMemberType() != rm->GetMemberType())) {
                        continue;
                    }

                    // found a matching rhs member
                    uint8_t actionMask = (~rm->GetActionMask()) & lm->GetActionMask();
                    lm->SetActionMask(actionMask); // update action mask
                }

                // remove member with empty action masks
                if (lm->GetActionMask() == 0) {
                    lm = lMembers.erase(lm);
                } else {
                    ++lm;
                }
            }
        }

        // remove rule without remaining members
        if (lMembers.size() == 0) {
            lr = lRules.erase(lr);
        } else {
            ++lr;
        }
    }

    if (lRules.size() > 0) {
        status = result.SetFromRules(&lRules[0], lRules.size());
    }

    return status;
}
}
}
#undef QCC_MODULE
