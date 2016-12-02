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