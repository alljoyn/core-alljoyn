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

#ifndef APPLICATIONLISTENER_H_
#define APPLICATIONLISTENER_H_

#include <alljoyn/securitymgr/ApplicationInfo.h>

#include <qcc/Debug.h>
#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
class ApplicationListener {
  private:
    /**
     * \brief Callback that is triggered when an application state change has
     * been detected. The execution of this method should be short, as all
     * registered listeners will be called synchronously.
     *
     * \param[in] oldAppInfo   the previously known information of this app or NULL if no info was known.
     * \param[in] newAppInfo   the new information of this app or NULL when the security manager is no longer tracking the application.
     */
    virtual void OnApplicationStateChange(const ApplicationInfo* oldAppInfo,
                                          const ApplicationInfo* newAppInfo) = 0;

    friend class SecurityManagerImpl;

  protected:
    ApplicationListener() { };

  public:
    virtual ~ApplicationListener() { };

    static void PrintStateChangeEvent(const ApplicationInfo* oldAppInfo,
                                      const ApplicationInfo* newAppInfo);
};
}
}
#undef QCC_MODULE
#endif /* APPLICATIONLISTENER_H_ */
