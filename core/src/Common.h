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

#ifndef COMMON_H_
#define COMMON_H_

#include <qcc/CryptoECC.h>
#include <alljoyn/PermissionConfigurator.h>
#include <alljoyn/Message.h>
#include <stdint.h>

#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
qcc::String ByteArrayToHex(const uint8_t* bytes,
                           const std::size_t len);

qcc::String ByteArrayToString(const AllJoynScalarArray bytes);

qcc::String PubKeyToString(const qcc::ECCPublicKey* pubKey);
}
}
#undef QCC_MODULE
#endif /* COMMON_H_ */
