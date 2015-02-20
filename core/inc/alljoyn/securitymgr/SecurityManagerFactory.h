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

#ifndef SECURITYMANAGERFACTORY_H_
#define SECURITYMANAGERFACTORY_H_

#include <vector>
#include <memory>

#include <alljoyn/BusAttachment.h>
#include <qcc/String.h>
#include <alljoyn/securitymgr/SecurityManager.h>

#include <qcc/Debug.h>
#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
class SecurityManager;
class IdentityData;
struct NativeStorageConfig;

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
        status = ER_OK;
    }

    void operator=(SecurityManagerFactory const&) { }

  public:
    /**
     * \brief Get a singleton instance of the security manager factory.
     *
     * \retval SecurityManagerFactory reference to the singleton security manager factory.
     */
    static SecurityManagerFactory& GetInstance()
    {
        static SecurityManagerFactory smf;
        return smf;
    }

    /* \brief Get a security manager instance.
     *
     * \param[in] storage a pointer to the storage implementation to be used. (No ownership)
     * \param[in] ba the bus attachment to be used.
     *
     * \retval SecurityManager a pointer to a security manager instance
     * \retval NULL otherwise
     */
    SecurityManager* GetSecurityManager(const Storage* storage = NULL,
                                        ajn::BusAttachment* ba = NULL);
};
}
}
#undef QCC_MODULE
#endif /* SECURITYMANAGERFACTORY_H_ */
