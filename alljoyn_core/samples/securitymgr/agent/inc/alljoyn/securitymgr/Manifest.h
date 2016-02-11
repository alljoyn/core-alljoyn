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

#ifndef ALLJOYN_SECMGR_MANIFEST_H_
#define ALLJOYN_SECMGR_MANIFEST_H_

#include <string>
#include <stdint.h>

#include <qcc/Crypto.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/Status.h>

using namespace qcc;
using namespace std;

namespace ajn {
namespace securitymgr {
/**
 * @brief A class that encapsulates manifest's byte data and the
 *        corresponding rules as well as the digest.
 *        A manifest object is always complete; the manifest is
 *        always matching the byte array representation.
 *        It uses the static Util class to achieve the needed
 *        serialization and de-serialization.
 */
class Manifest {
  public:

    Manifest();

    Manifest(const Manifest&);

    ~Manifest();

    /**
     * @brief Construct manifest from a byte array.
     *
     * @param[in] manifestByteArray        A byte array representing a manifest.
     * @param[in] size                     Size of manifestByteArray.
     */
    Manifest(const uint8_t* manifestByteArray,
             const size_t size);

    /**
     * @brief Construct manifest from an array of rules.
     *       The new object does NOT take ownership of the
     *       passed in rules.
     *
     * @param[in] manifestRules            An array of rules.
     * @param[in] manifestRulesCount       The count of rules.
     */
    Manifest(const PermissionPolicy::Rule* manifestRules,
             const size_t manifestRulesCount);

    /**
     * @brief Retrieve the byte array representing the manifest.
     *
     * @param[in,out] manifestByteArray    A byte array to be filled-in.
     * @param[in,out] size                 Size of manifestByteArray to be filled-in.
     *
     * @return ER_OK                       If a byte array can be returned.
     * @return ER_END_OF_DATA              In case the manifest is empty.
     */
    QStatus GetByteArray(uint8_t** manifestByteArray,
                         size_t* size) const;

    /**
     * @brief Retrieve the rules representing the manifest.
     *
     * @param[in,out] manifestRules        Pointer to an array of rules that
     *                                     will be newly allocated by this function.
     *                                     Manifest has no ownership on the returned array.
     * @param[in,out] manifestRulesCount   The count of rules that will be set by this function.
     *
     * @return ER_OK                       If an array of rules can be returned.
     * @return ER_END_OF_DATA              In case there are no rules.
     */
    QStatus GetRules(PermissionPolicy::Rule** manifestRules,
                     size_t* manifestRulesCount) const;

    /**
     * @brief Retrieve the digest representing the manifest.
     *
     * @param[in,out] digest               A previously allocated (new) byte array with the correct size.
     *
     * @return ER_OK                       If the digest was computed correctly.
     * @return ER_END_OF_DATA              In case the manifest is empty.
     * @return others
     */
    QStatus GetDigest(uint8_t* digest) const;

    /**
     * @brief Populate the manifest based on the passed on byte array.
     *
     * @param[in] manifestByteArray        A byte array representing a manifest.
     * @param[in] size                     Size of manifestByteArray.
     *
     * @return ER_OK                       If the manifest was populated successfully.
     * @return others
     */
    QStatus SetFromByteArray(const uint8_t* manifestByteArray,
                             const size_t size);

    /**
     * @brief Populate the manifest from an array of rules.
     *       The object does NOT take ownership of the
     *       passed in rules.
     *
     * @param[in] manifestRules            An array of rules
     * @param[in] manifestRulesCount       The count of rules
     *
     * @return ER_OK                       If the manifest was populated successfully.
     * @return others
     */
    QStatus SetFromRules(const PermissionPolicy::Rule* manifestRules,
                         const size_t manifestRulesCount);

    /**
     * @brief Return a string representation of this Manifest.
     *
     * @return  A string representation of this manifest.
     */
    string ToString() const;

    /**
     * @brief The assignment operator for Manifest.
     *
     * @param[in] rhs   The manifest to assign from.
     */
    Manifest& operator=(const Manifest& rhs);

    /**
     * @brief The equality operator for Manifest.
     *
     * @param[in] other  The other Manifest to compare with.
     */
    bool operator==(const Manifest& other) const;

    /**
     * @brief The inequality operator for Manifest.
     *
     * @param[in] other  The other Manifest to compare with.
     */
    bool operator!=(const Manifest& other) const;

    /**
     * @brief Computes a manifest containing all rules that are not defined in another manifest.
     *
     * @param[in]  other   The other manifest.
     * @param[out] result  A manifest containing all the rules that are defined in this
     *                     manifest, but not in the other manifest.
     *
     * @return ER_OK    If the difference was computed successfully.
     * @return others   On failure
     *
     */
    QStatus Difference(const Manifest& other,
                       Manifest& result) const;

    /**
     * @brief Get the number of rules in the manifest.
     *
     * @return the number of Rules in the manifest
     */
    const size_t GetRulesSize() const;

  private:

    /**
     * @brief A PermissionPolicy encapsulating the manifest rules.
     */
    mutable PermissionPolicy manifest;
    /**
     * @brief A byte array representation of the manifest rules.
     */
    uint8_t* byteArray;
    /**
     * @brief The size of the byte array representation of the manifest rules.
     */
    size_t size;
};
}
}

#endif /* ALLJOYN_SECMGR_MANIFEST_H_ */
