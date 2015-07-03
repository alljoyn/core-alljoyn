/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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

#ifndef ALLJOYN_SECMGR_APPLICATION_H_
#define ALLJOYN_SECMGR_APPLICATION_H_

#include <vector>
#include <string>

#include <qcc/GUID.h>
#include <qcc/CryptoECC.h>
#include <qcc/String.h>
#include <qcc/KeyInfoECC.h>

#include <alljoyn/securitymgr/Util.h>
#include <alljoyn/PermissionConfigurator.h>

using namespace std;
using namespace qcc;

namespace ajn {
namespace securitymgr {
/**
 * @brief Application represents an application with its updates pending state.
 */
struct Application {
  public:

    Application() : updatesPending(false) { }

    /**
     * @brief NIST P-256 ECC KeyInfo.
     *        It is the unique key to identify an application.
     */
    KeyInfoNISTP256 keyInfo;

    /**
     * @brief Indicates whether there are still changes to the security
     *        configuration of the application that are known to the security agent
     *        but have not yet been applied to the remote application itself yet.
     *        It is set to true whenever an administrator changes the configuration of
     *        an application, and set to false only when the configuration of the remote
     *        application matches the configuration known to the security agent.
     */
    bool updatesPending;

    /**
     * @brief Equality operator for Application.
     *
     * @param[in] rhs  The application to compare with.
     */
    bool operator==(const Application& rhs) const
    {
        return keyInfo ==  rhs.keyInfo;
    }

    /**
     * @brief Inequality operator for Application.
     *
     * @param[in] rhs  The application to compare with.
     */
    bool operator!=(const Application& rhs) const
    {
        return keyInfo !=  rhs.keyInfo;
    }
};

/*
 * @brief OnlineApplication adds the online nature; (i.e., ApplicationState, busName) as it inherits from Application.
 */
struct OnlineApplication :
    public Application {
    OnlineApplication(PermissionConfigurator::ApplicationState _applicationState,
                      string _busName) : applicationState(
            _applicationState), busName(_busName) { }

    OnlineApplication() : applicationState(PermissionConfigurator::NOT_CLAIMABLE) { }

    /**
     * @brief The claim state of the application. An application can only be
     *        claimed if it is in the CLAIMABLE state, and can only be managed by a
     *        security agent if it is in the CLAIMED state.
     */
    PermissionConfigurator::ApplicationState applicationState;

    /**
     * @brief The bus name of the online application.
     */
    string busName;

    /**
     * @brief Returns a string representation of the application.
     */
    string ToString() const
    {
        string toString("OnlineApplication: ");
        toString += "Busname: ";
        toString += busName;
        toString += ", Claim state: ";
        toString += PermissionConfigurator::ToString(applicationState);
        toString += ", Updates pending: ";
        toString += (updatesPending ? "true" : "false");
        return toString;
    }

  private:
    /**
     * Private copy constructor from parent instance and
     * assignment operator to avoid incomplete online information.
     **/
    OnlineApplication(const Application& app);
    OnlineApplication& operator=(const Application& app);
};
}
}

#endif /* ALLJOYN_SECMGR_APPLICATION_H_ */
