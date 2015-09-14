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
enum ApplicationSyncState {
    SYNC_UNKNOWN = 0,    ///< Unknown whether the application is managed by this security manager..
    SYNC_UNMANAGED = 1,  ///< The application is not managed by this security manager.
    SYNC_OK = 2,         ///< The application is claimed and there are no pending changes to its security configuration.
    SYNC_PENDING = 3,    ///< The security configuration of the application will be updated when it comes online.
    SYNC_WILL_RESET = 4, ///< The application will be reset when it comes online.
    SYNC_RESET = 5       ///< The application is successfully reset.
};

/**
 * @brief Returns the string representation of an application sync state.
 *
 * @param[in] state  application sync state
 *
 * @return string    a string representation of the application sync state
 */
static const char* ToString(const ApplicationSyncState state)
{
    switch (state) {
    case SYNC_UNKNOWN:
        return "SYNC_UNKNOWN";

    case SYNC_UNMANAGED:
        return "SYNC_UNMANAGED";

    case SYNC_OK:
        return "SYNC_OK";

    case SYNC_PENDING:
        return "SYNC_PENDING";

    case SYNC_WILL_RESET:
        return "SYNC_WILL_RESET";

    case SYNC_RESET:
        return "SYNC_RESET";
    }

    return "UNKNOWN";
}

/**
 * @brief Application represents an application with its update status.
 */
struct Application {
  public:

    Application() : syncState(SYNC_UNKNOWN) { }

    /**
     * @brief NIST P-256 ECC KeyInfo.
     *        It is the unique key to identify an application.
     */
    KeyInfoNISTP256 keyInfo;

    /**
     * @brief Indicates the current sync state of the application.
     */
    ApplicationSyncState syncState;

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

    /**
     * @brief less then operator for Application.
     *
     * @param[in] rhs  The application to compare with.
     */
    bool operator<(const Application& rhs) const
    {
        return keyInfo <  rhs.keyInfo;
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
        toString += ", Sync state: ";
        toString += ajn::securitymgr::ToString(syncState);
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
