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

#ifndef ALLJOYN_SECMGR_SECINFOLISTENER_H_
#define ALLJOYN_SECMGR_SECINFOLISTENER_H_

#include "SecurityInfo.h"

namespace ajn {
namespace securitymgr {
class SecurityInfoListener {
  public:
    /**
     * @brief Callback function that is called when security information is
     * discovered, updated or removed.
     *
     * @param[in] oldSecInfo   Previous security information for a busname;
     *                         nullptr if a new security information is discovered.
     * @param[in] newSecInfo   Updated security information for a busname;
     *                         nullptr if a security information has been removed.
     */
    virtual void OnSecurityStateChange(const SecurityInfo* oldSecInfo,
                                       const SecurityInfo* newSecInfo) = 0;

  protected:
    SecurityInfoListener() { };

  public:
    virtual ~SecurityInfoListener() { };
};
}
}
#endif /* ALLJOYN_SECMGR_SECINFOLISTENER_H_ */
