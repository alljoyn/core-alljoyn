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

#ifndef ALLJOYN_SECMGR_APPLICATIONLISTENER_H_
#define ALLJOYN_SECMGR_APPLICATIONLISTENER_H_

#include <qcc/Debug.h>

#include "Application.h"
#include "SyncError.h"
#include "ManifestUpdate.h"

namespace ajn {
namespace securitymgr {
class ApplicationListener {
  public:
    /**
     * @brief Callback that is triggered when an application state change has
     *        been detected. The execution of this method should be short, as all
     *        registered listeners will be called synchronously.
     *
     * @param[in] oldApp   The previously known information of this app or
     *                     nullptr if no info was known.
     * @param[in] newApp   The new information of this app or nullptr when
     *                     the security agent is no longer tracking the
     *                     application.
     */
    virtual void OnApplicationStateChange(const OnlineApplication* oldApp,
                                          const OnlineApplication* newApp) = 0;

    /**
     * @brief Callback that is triggered when an application could not be
     *        synchronized with the persisted state.
     *
     * @param[in] syncError  The error that occurred when synchronizing an
     *                       application.
     */
    virtual void OnSyncError(const SyncError* syncError) = 0;

    /**
     * @brief Callback that is triggered when an application has a new manifest
     *        requesting additional rights from the administrator.
     *
     * @param[in] manifestUpdate   The manifest update containing all information
     *                             related to the specific manifest update.
     */
    virtual void OnManifestUpdate(const ManifestUpdate* manifestUpdate) = 0;

    /**
     * @brief Virtual destructor for derivable class.
     */
    virtual ~ApplicationListener() { };

  protected:
    ApplicationListener() { };
};
}
}
#endif /* ALLJOYN_SECMGR_APPLICATIONLISTENER_H_ */
