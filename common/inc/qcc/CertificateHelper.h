#ifndef _CERTIFICATE_HELPER_H_
#define _CERTIFICATE_HELPER_H_
/**
 * @file
 *
 * Certificate Helper class
 */

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
#include <qcc/platform.h>

namespace qcc {

class CertificateHelper {

  public:

    /**
     * Count the number of certificates in a PEM string representing a cert chain.
     * @param encoded the input string holding the PEM string
     * @param[out] count the number of certs
     * @return ER_OK for sucess; otherwise, error code.
     */
    static QStatus AJ_CALL GetCertCount(const String& encoded, size_t* count);

};

} /* namespace qcc */

#endif
