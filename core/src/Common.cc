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
#include <alljoyn/PermissionConfigurator.h>

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
    qcc::String str = "";

    for (size_t i = 0; i < len; i++) {
        char buff[4];
        sprintf(buff, "%02X", bytes[i]);
        str = str + buff;
    }
    return str;
}

qcc::String PubKeyToString(const qcc::ECCPublicKey* pubKey)
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

const char* ToString(const ajn::PermissionConfigurator::ClaimableState acs)
{
    switch (acs) {
    case ajn::PermissionConfigurator::STATE_UNCLAIMABLE:
        return "NOT CLAIMED";

    case ajn::PermissionConfigurator::STATE_CLAIMABLE:
        return "CLAIMABLE";

    case ajn::PermissionConfigurator::STATE_CLAIMED:
        return "CLAIMED";

    case ajn::PermissionConfigurator::STATE_UNKNOWN:
        return "UNKNOWN CLAIM STATE";
    }

    return "UNKNOWN CLAIM STATE";
}

const char* ToString(ApplicationRunningState acs)
{
    switch (acs) {
    case STATE_UNKNOWN_RUNNING:
        return "UNKNOWN RUNNING STATE";

    case STATE_NOT_RUNNING:
        return "NOT RUNNING STATE";

    case STATE_RUNNING:
        return "RUNNING STATE";
    }

    return "";
}

ApplicationRunningState ToRunningState(const unsigned char byte)
{
    switch (byte) {
    case 0:
        return STATE_UNKNOWN_RUNNING;

    case 1:
        return STATE_NOT_RUNNING;

    case 2:
        return STATE_RUNNING;
    }
    return STATE_UNKNOWN_RUNNING;
}

void PrettyPrintStateChangeSignal(const char* sourcePath, const Message& msg)
{
    printf("--==## State changed signal received ##==--\n");
    printf("\t State '%s'.\n",
           ToString(ajn::PermissionConfigurator::ClaimableState(const_cast<Message&>(msg)->GetArg(1)->v_byte)));
    printf("\t SourcePath: '%s'.\n", sourcePath);
    printf("\t ObjectPath: '%s'.\n", msg->GetObjectPath());
    printf("\t Sender: '%s'.\n", msg->GetSender());
}
}
}
#undef QCC_MODULE
