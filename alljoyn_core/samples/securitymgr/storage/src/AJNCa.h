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

#ifndef ALLJOYN_SECMGR_STORAGE_AJNCA_H_
#define ALLJOYN_SECMGR_STORAGE_AJNCA_H_

#include <string>

#include <qcc/CryptoECC.h>
#include <qcc/CertificateECC.h>

#include <qcc/FileStream.h>
#include <qcc/KeyBlob.h>
#include <alljoyn/Status.h>

using namespace qcc;
using namespace std;

namespace ajn {
namespace securitymgr {
class AJNCa {
  public:
    AJNCa() : store(), pkey(nullptr)
    {
    };

    ~AJNCa()
    {
        delete pkey;
        pkey = nullptr;
    }

    QStatus Reset();

    QStatus Init(string storeName);

    QStatus GetDSAPublicKey(ECCPublicKey& publicKey) const;

    QStatus GetDSAPrivateKey(ECCPrivateKey& privateKey) const;

    QStatus SignCertificate(CertificateX509& certificate) const;

  private:

    QStatus GetPrivateKeyBlob(ECCPrivateKey& key,
                              FileSource& fs) const;

    String store;
    ECCPublicKey* pkey;
};
}
}

#endif /* ALLJOYN_SECMGR_STORAGE_AJNCA_H_ */
