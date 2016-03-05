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

#ifndef GUID_UTIL_H_
#define GUID_UTIL_H_


#include <qcc/Debug.h>
#include <qcc/String.h>

#if defined(QCC_OS_GROUP_WINDOWS)
/* Disabling warning C 4100. Function doesnt use all passed in parameters */
#pragma warning(push)
#pragma warning(disable: 4100)
#endif


static const unsigned int GUID_STRING_MAX_LENGTH = 32;
static const unsigned int GUID_HYPHEN_MAX_LENGTH = 4;
static const unsigned int END_OF_STRING_LENGTH = 1;

namespace ajn {
namespace services {

/**
 * implements GUID utilities
 * Generation, saving, exposing - 128 bit unique number
 */
class GuidUtil {
  public:
    /**
     * GetInstance
     * @return singleton of GuidUtil pointer
     */
    static GuidUtil* GetInstance();
    /**
     * GetDeviceIdString
     * @param deviceId
     */
    void GetDeviceIdString(qcc::String* deviceId);

    /**
     * GenerateGUID
     * @param guid
     */
    void GenerateGUID(qcc::String* guid);

  private:
    /**
     * Constructor for GuidUtil
     */
    GuidUtil();
    /**
     * Destructor for GuidUtil
     */
    virtual ~GuidUtil();
    /**
     * NormalizeString
     * @param strGUID
     */
    void NormalizeString(char* strGUID);

    /**
     * GetDeviceIdFileName
     * @param strGUID
     * @return device id file name
     */
    const char* GetDeviceIdFileName();

    /**
     * ReadGuidOfDeviceID
     * @param strGUID
     */
    bool ReadGuidOfDeviceID(char* strGUID);

    /**
     * WriteGUIDToFile
     * @param strGUID
     */
    void WriteGUIDToFile(char* strGUID);

    /**
     * GenerateGUIDUtil
     * @param strGUID
     */
    void GenerateGUIDUtil(char* strGUID);
    /**
     * a pointer to singleton GuidUtil
     */
    static GuidUtil* pGuidUtil;

};
} //namespace services
} //namespace ajn

#endif /* GUID_UTIL_H_ */
