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

#include <SecurityManagerFactory.h>
#include <SecurityManager.h>

#include <qcc/FileStream.h>
#include <qcc/KeyBlob.h>
#include <qcc/CryptoECC.h>

#include <IdentityData.h>
#include <StorageConfig.h>

#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
SecurityManagerFactory::SecurityManagerFactory()
{
    ownBa = false;
    ba = NULL;
}

SecurityManagerFactory::~SecurityManagerFactory()
{
    if (ownBa == true) {
        ba->Disconnect();
        ba->Stop();
        ba->Join();
        delete ba;
    }
}

SecurityManager* SecurityManagerFactory::GetSecurityManager(qcc::String userName,
                                                            qcc::String password,
                                                            const StorageConfig& storageCfg,
                                                            IdentityData* id,
                                                            ajn::BusAttachment* _ba)
{
    SecurityManager* sm = NULL;
    // TODO: - which database file to access ?
    //       - where to locate database file ?
    //       - password encrypted stored in separate table ?
    if (/* TODO: check if exists */ true) {
        // TODO: verify password
        //

        // NOTE: keystore.h is located inside alljoyn core "src" and not in the "inc" dir.
        //       We use the public keyblob.h located in qcc "inc" dir to store and
        //       retrieve individual keys (as workaround for this POC).
        //
        if (_ba == NULL) {
            if (ba == NULL) {
                ba = new BusAttachment("test", true);
                ownBa = true;
                do {
                    // \todo provide sensible data or remove name argument
                    status = ba->Start();
                    if (status != ER_OK) {
                        QCC_LogError(status, ("Failed to start bus attachment"));
                        break;
                    }

                    status = ba->Connect();
                    if (status != ER_OK) {
                        QCC_LogError(status, ("Failed to connect bus attachment"));
                        break;
                    }
                } while (0);
                _ba = ba; //TODO: don't override; clean up code
            }
        }
        qcc::Crypto_ECC crypto;

        qcc::KeyBlob pubKey;
        qcc::String filenamePubKey = userName + "pub_keystore";
        qcc::FileSource sourcePubKey(filenamePubKey);
        QStatus pubKeyLoadResult = pubKey.Load(sourcePubKey);

        qcc::KeyBlob privKey;
        qcc::String filenamePrivKey = userName + "priv_keystore";
        qcc::FileSource sourcePrivKey(filenamePrivKey);
        QStatus privKeyLoadResult = privKey.Load(sourcePrivKey);

        if ((ER_OK == pubKeyLoadResult) && (ER_OK == privKeyLoadResult)) {
            // Load both public and private keys

            qcc::ECCPublicKey eccPubKey;
            memcpy((void*)&eccPubKey.data, (void*)pubKey.GetData(), qcc::ECC_PUBLIC_KEY_SZ);
            crypto.SetDHPublicKey(&eccPubKey);

            qcc::ECCPrivateKey eccPrivKey;
            memcpy((void*)&eccPrivKey.data, (void*)privKey.GetData(), qcc::ECC_PRIVATE_KEY_SZ);
            crypto.SetDHPrivateKey(&eccPrivKey);
        } else {
            // There is no key yet (or missing a counter part), generate new pair
            if (ER_OK != crypto.GenerateDHKeyPair()) {
                return NULL;
            }

            pubKey.Erase();
            qcc::FileSink sinkPubKey(filenamePubKey);
            qcc::ECCPublicKey* eccPubKey = const_cast<qcc::ECCPublicKey*>(crypto.GetDHPublicKey());
            pubKey = qcc::KeyBlob(eccPubKey->data, qcc::ECC_PUBLIC_KEY_SZ, qcc::KeyBlob::DSA_PUBLIC);

            privKey.Erase();
            qcc::FileSink sinkPrivKey(filenamePrivKey);
            qcc::ECCPrivateKey* eccPrivKey = const_cast<qcc::ECCPrivateKey*>(crypto.GetDHPrivateKey());
            privKey = qcc::KeyBlob(eccPrivKey->data, qcc::ECC_PRIVATE_KEY_SZ, qcc::KeyBlob::DSA_PRIVATE);

            if ((ER_OK != pubKey.Store(sinkPubKey)) || (ER_OK != privKey.Store(sinkPrivKey))) {
                // unable to store key.
                return NULL;
            }
        }

        sm = new SecurityManager(userName,
                                 password,
                                 id,
                                 _ba,
                                 *crypto.GetDHPublicKey(),
                                 *crypto.GetDHPrivateKey(),
                                 storageCfg);
        if (NULL != sm) {
            if (ER_OK != sm->GetStatus()) {
                delete sm;

                return NULL;
            }
        }

        return sm;
    } else {
        // TODO: - validate given password <> empty
        //               - create new database with user/password
        //               - where to store the database ?
        //               - what name should the database get ?
    }

    return NULL;
}

#if 0
const std::vector<qcc::String> SecurityManagerFactory::GetUsers() const
{
    // Is there an extra database containing the user names <=> database locations??
    return std::vector<qcc::String>();
}

#endif
}
}
#undef QCC_MODULE
