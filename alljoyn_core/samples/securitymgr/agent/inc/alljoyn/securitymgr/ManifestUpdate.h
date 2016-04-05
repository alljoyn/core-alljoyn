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

#ifndef ALLJOYN_SECMGR_MANIFESTUPDATE_H_
#define ALLJOYN_SECMGR_MANIFESTUPDATE_H_

#include <alljoyn/securitymgr/Manifest.h>

namespace ajn {
namespace securitymgr {
/**
 * Represents an event in which a remote application changes its state to NEED_UPDATE.
 */
class ManifestUpdate {
  public:

    /**
     * @brief Default constructor for ManifestUpdate.
     */
    ManifestUpdate()
    {
    }

    /**
     * @brief Constructor for ManifestUpdate.
     *
     * @param[in] _app          The application that is requesting additional rights in its
     *                          manifest.
     * @param[in] _oldManifest  The manifest that was previously approved by the administrator.
     * @param[in] _newManifest  The new manifest that is requested by the application.
     */
    ManifestUpdate(const OnlineApplication& _app,
                   const Manifest& _oldManifest,
                   const Manifest& _newManifest) :
        app(_app), oldManifest(_oldManifest), newManifest(_newManifest)
    {
        newManifest.Difference(oldManifest, additionalRules);
        oldManifest.Difference(newManifest, removedRules);
    }

    OnlineApplication app; /**< The application that is requesting a new manifest to be approved.*/
    Manifest oldManifest; /**< The manifest that was previously approved by the administrator.*/
    Manifest newManifest; /**< The new manifest that is requested by the application.*/
    Manifest additionalRules; /**< The rules that have not yet been approved by the administrator.*/
    Manifest removedRules; /**< The rules that are no longer required by the application.*/
};
}
}

#endif /* ALLJOYN_SECMGR_MANIFESTUPDATE_H_ */
