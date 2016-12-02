/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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