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

#ifndef SECURITYMANAGERFACTORY_H_
#define SECURITYMANAGERFACTORY_H_

#include <vector>
#include <memory>

#include <alljoyn/BusAttachment.h>
#include <qcc/String.h>
#include <SecurityManager.h>

#include <qcc/Debug.h>
#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
class SecurityManager;
class IdentityData;
class StorageConfig;

/* This is a singleton */
class SecurityManagerFactory {
  private:

    ajn::BusAttachment* ba;
    bool ownBa;
    QStatus status;

    SecurityManagerFactory();

    ~SecurityManagerFactory();

    SecurityManagerFactory(SecurityManagerFactory const& sf)
    {
        ba = sf.ba;
        ownBa = false;
    }

    void operator=(SecurityManagerFactory const&) { }

  public:
    static SecurityManagerFactory& GetInstance()
    {
        static SecurityManagerFactory smf;
        return smf;
    }

    /* Responsibilities
     * - Check in the keystore if we already had a key pair for this user
     * -- if yes: construct a new SecurityManager for this user with the data
     * -- if no: generate a new key pair, store it and construct a new SecurityManager for this user
     *
     *
     */
    /* REMARK: It's still under discussion whether we will already include username/password support, but
     * I think we will need it at some point to know which RoT we are managing applications for */
    //How about passing keystore here
    //
    SecurityManager* GetSecurityManager(qcc::String userName,
                                        qcc::String password,
                                        const StorageConfig& storageCfg,
                                        const SecurityManagerConfig& smCfg,
                                        IdentityData* id = NULL,
                                        ajn::BusAttachment* ba = NULL);

    /* Allows you to show all Users in the UI */
#if 0
    /* TODO LATER */
    const std::vector<qcc::String> GetUsers() const;

#endif
};
}
}
#undef QCC_MODULE
#endif /* SECURITYMANAGERFACTORY_H_ */
