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

#define QCC_MODULE "SEC_MGR"

#include <alljoyn/securitymgr/ApplicationState.h>

namespace ajn {
namespace securitymgr {
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
};

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
};

const char* ToString(const PermissionConfigurator::ClaimableState acs)
{
    switch (acs) {
    case PermissionConfigurator::STATE_UNCLAIMABLE:
        return "NOT CLAIMED";

    case PermissionConfigurator::STATE_CLAIMABLE:
        return "CLAIMABLE";

    case PermissionConfigurator::STATE_CLAIMED:
        return "CLAIMED";

    case PermissionConfigurator::STATE_UNKNOWN:
        return "UNKNOWN CLAIM STATE";
    }

    return "UNKNOWN CLAIM STATE";
};
}
}
#undef QCC_MODULE
