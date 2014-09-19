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

#ifndef ROOTOFTRUST_H_
#define ROOTOFTRUST_H_

#include <qcc/CryptoECC.h>
#include <qcc/Debug.h>

#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
class RootOfTrust {
  private:
    qcc::ECCPublicKey pubKey;

  public:
    RootOfTrust(const qcc::ECCPublicKey& _pubKey);

    RootOfTrust(const char* pemFile);

    ~RootOfTrust();

    /* Export RoT to a file */
    bool Export(const char* pemFile);

    const qcc::ECCPublicKey& GetPublicKey() const;
};
}
}
#undef QCC_MODULE
#endif /* ROOTOFTRUST_H_ */
