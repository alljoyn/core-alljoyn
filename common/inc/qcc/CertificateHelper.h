#ifndef _CERTIFICATE_HELPER_H_
#define _CERTIFICATE_HELPER_H_
/**
 * @file
 *
 * Certificate Helper class
 */

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