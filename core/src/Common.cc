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

#include <Common.h>

#include <algorithm>
#include <iostream>

#include <qcc/Debug.h>

#define QCC_MODULE "SEC_MGR"

using namespace ajn;
using namespace std;

namespace ajn {
namespace securitymgr {
qcc::String ByteArrayToString(const AllJoynScalarArray bytes)
{
    qcc::String str;
    for (size_t i = 0; i < bytes.numElements; ++i) {
        char buff[4];
        sprintf(buff, "%02x", bytes.v_byte[i]);
        str = str + buff;
    }
    return str;
}

qcc::String ByteArrayToHex(const uint8_t* bytes, const size_t len)
{
    char buf[len * 2 + 1];

    char* ptr = &buf[0];
    for (unsigned int i = 0; i < len; i++, ptr += 2) {
        sprintf(ptr, "%02X", bytes[i]);
    }

    return qcc::String(buf);
}

qcc::String PubKeyToString(const qcc::ECCPublicKey* pubKey)
{
    qcc::String str;
    for (int i = 0; i < (int)qcc::ECC_PUBLIC_KEY_SZ; ++i) {
        char buff[4];
        sprintf(buff, "%02x", (unsigned char)(pubKey->data[i]));
        str = str + buff;
    }

    return str;
}

const char* ToString(const ApplicationClaimState acs)
{
    switch (acs) {
    case ApplicationClaimState::UNKNOWN:
        return "UNKNOWN";

    case ApplicationClaimState::NOT_CLAIMED:
        return "NOT CLAIMED";

    case ApplicationClaimState::CLAIMED:
        return "CLAIMED";

    case ApplicationClaimState::CLAIMABLE:
        return "CLAIMABLE";
    }
    return "";
}

ApplicationClaimState ToClaimState(const unsigned char byte)
{
    switch (byte) {
    case 0:
        return ApplicationClaimState::NOT_CLAIMED;

    case 1:
        return ApplicationClaimState::CLAIMED;

    case 2:
        return ApplicationClaimState::CLAIMABLE;
    }
    return ApplicationClaimState::UNKNOWN;
}

;

const char* ToString(ApplicationRunningState acs)
{
    switch (acs) {
    case ApplicationRunningState::UNKNOWN:
        return "UNKNOWN";

    case ApplicationRunningState::NOT_RUNNING:
        return "NOT RUNNING";

    case ApplicationRunningState::RUNNING:
        return "RUNNING";
    }

    return "";
}

ApplicationRunningState ToRunningState(const unsigned char byte)
{
    switch (byte) {
    case 0:
        return ApplicationRunningState::NOT_RUNNING;

    case 1:
        return ApplicationRunningState::RUNNING;

    case 2:
        return ApplicationRunningState::UNKNOWN;
    }
    return ApplicationRunningState::UNKNOWN;
}

void PrettyPrintStateChangeSignal(const char* sourcePath, const Message& msg)
{
    printf("--==## State changed signal received ##==--\n");
    printf("\t State '%s'.\n", ToString(ToClaimState(const_cast<Message&>(msg)->GetArg(1)->v_byte)));
    printf("\t SourcePath: '%s'.\n", sourcePath);
    printf("\t ObjectPath: '%s'.\n", msg->GetObjectPath());
    printf("\t Sender: '%s'.\n", msg->GetSender());
}
}
}
#undef QCC_MODULE
