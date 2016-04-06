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

#include <alljoyn/securitymgr/Manifest.h>
#include <alljoyn/securitymgr/Util.h>

#define QCC_MODULE "SECMGR_AGENT"

using namespace std;

namespace ajn {
namespace securitymgr {
Manifest::Manifest()
{
}

Manifest::Manifest(const Manifest& other)
{
    manifest = other.manifest;
}

Manifest::~Manifest()
{
}

Manifest::Manifest(const uint8_t* manifestByteArray, const size_t _size)
{
    QStatus status;
    if (ER_OK != (status = SetFromByteArray(manifestByteArray, _size))) {
        QCC_LogError(status, ("Failed to create manifest"));
    }
}

Manifest::Manifest(const PermissionPolicy::Rule* _rules,
                   const size_t manifestRulesCount)
{
    QStatus status = ER_OK;
    if (_rules && manifestRulesCount != 0) {
        status = SetFromRules(_rules, manifestRulesCount);
    }
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to create manifest"));
    }
}

Manifest::Manifest(const ajn::Manifest& signedManifest)
{
    manifest = signedManifest;
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

    std::vector<uint8_t> serializedForm;

    QStatus status = manifest->Serialize(serializedForm);
    if (ER_OK != status) {
        return status;
    }

    *manifestByteArray = new (std::nothrow) uint8_t[serializedForm.size()];
    if (nullptr == *manifestByteArray) {
        return ER_OUT_OF_MEMORY;
    }

    *_size = serializedForm.size();
    memcpy(*manifestByteArray, serializedForm.data(), serializedForm.size());

    return ER_OK;
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

    size_t numOfRules = manifest->GetRules().size();

    if (numOfRules <= 0) {
        *manifestRules = nullptr;
        *manifestRulesNumber = 0;
        return ER_END_OF_DATA;
    }

    const PermissionPolicy::Rule* localRules = manifest->GetRules().data();
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
    return manifest->GetRules().size();
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

    std::vector<uint8_t> serializedForm(_size);
    serializedForm.assign(manifestByteArray, manifestByteArray + _size);
    status = manifest->Deserialize(serializedForm);

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

    return manifest->SetRules(manifestRules, manifestRulesNumber);
}

QStatus Manifest::SetFromSignedManifest(const ajn::Manifest& signedManifest)
{
    manifest = signedManifest;

    return ER_OK;
}

string Manifest::ToString() const
{
    // The implementation for manifest->ToString() is currently in the feature/ASACORE-2710 branch.
    // When it gets into master, this can be corrected.
    return string(); // manifest->ToString();
}

Manifest& Manifest::operator=(const Manifest& rhs)
{
    if (this == &rhs) {
        return *this;
    }

    manifest = rhs.manifest;

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
            lr->SetMembers(lMembers.size(), &lMembers[0]);
            ++lr;
        }
    }

    if (lRules.size() > 0) {
        status = result.SetFromRules(&lRules[0], lRules.size());
    }

    return status;
}

bool Manifest::TemplateEquals(const Manifest& other) const {
    ajn::Manifest otherManifest = other.manifest;

    return ((manifest->GetVersion() == otherManifest->GetVersion()) &&
            (manifest->GetRules() == otherManifest->GetRules()));
}

ajn::Manifest Manifest::GetManifest() const
{
    return manifest;
}
}
}
#undef QCC_MODULE
