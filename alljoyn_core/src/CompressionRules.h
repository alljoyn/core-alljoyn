/**
 * @file
 * Class for managing header compression information
 */

/******************************************************************************
 * Copyright (c) 2010-2012, AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_COMPRESSION_RULES_H
#define _ALLJOYN_COMPRESSION_RULES_H

#ifndef __cplusplus
#error Only include CompressionRules.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Util.h>
#include <qcc/Mutex.h>

#include <alljoyn/Message.h>

#include <alljoyn/Status.h>

#include <qcc/STLContainer.h>
#include <map>

namespace ajn {

/**
 * Forward declaration
 */
class _CompressionRules;

/**
 * CompressionRules is a reference counted (managed) class to allow it to be shared between multiple
 * bus attachments.
 */
typedef qcc::ManagedObj<_CompressionRules> CompressionRules;

/**
 * This class maintains a list of header compression rules for header field compression and provides
 * methods that map from a expanded header to a compression token and back. This class is used by
 * the marshaling code to compress a header before sending it.
 */
class _CompressionRules {

  public:

    /**
     * Add a new expansion rule to the expansion table. This is an expansion that was received from
     * a remote peer. Note that 0 is an invalid token value.
     *
     * @param hdrFields  The header fields to add.
     * @param token      The compression token for the header fields.
     *
     * @return ER_OK if the expansion rule was added.
     */
    void AddExpansion(const HeaderFields& hdrFields, uint32_t token);

    /**
     * Get the compression token for the specified header fields.
     *
     * @param hdrFields  The header fields to look up.
     *
     * @return  An existing token or a newly allocated token,
     */
    uint32_t GetToken(const HeaderFields& hdrFields);

    /**
     * Perform the lookup of the expansion given a compression token. Note that token must
     * be non-zero.
     *
     * @param token  The compression token to lookup.
     *
     * @return  The expansion for the compression token or NULL if there is no such expansion.
     */
    const HeaderFields* GetExpansion(uint32_t token);

    /**
     * Destructor
     */
    ~_CompressionRules();

  private:

    /**
     * Add a compression/expansion rule.
     */
    void Add(const HeaderFields& hdrFields, uint32_t token);

    /**
     * Mutex to protect compression rules maps
     */
    qcc::Mutex lock;

    /**
     * Hash funcion for header compression. Hash value is computed over member and interface only.
     * on the reasonable assumption that there will only be one compression for a specific message.
     */
    struct HdrFieldHash {
        size_t operator()(const HeaderFields* k) const;
    };

    /**
     * Function for testing compressible message header fields for equality.
     */
    struct HdrFieldsEq {
        bool operator()(const HeaderFields* k1, const HeaderFields* k2) const;
    };

    /**
     * The header compression mapping from header fields to compression token
     */
    std::unordered_map<const ajn::HeaderFields*, uint32_t, HdrFieldHash, HdrFieldsEq> fieldMap;

    /*
     * The header expansion mapping from compression token to header fields
     */
    std::map<uint32_t, const ajn::HeaderFields*> tokenMap;

};

}

#endif
