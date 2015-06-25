#ifndef _KEYBLOB_H
#define _KEYBLOB_H
/**
 * @file
 *
 * This file provides a generic key blob implementation.
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

#include <algorithm>

#include <qcc/platform.h>

#include <assert.h>
#include <qcc/String.h>
#include <qcc/GUID.h>
#include <string.h>

#include <qcc/time.h>
#include <qcc/Stream.h>

namespace qcc {

/**
 * Generic encryption key storage.
 */
class KeyBlob {

  public:

    static const uint16_t MAX_TAG_LEN = 255;

    /**
     * Type of key blob.
     */
    typedef enum {
        EMPTY,          ///< Key blob is empty.
        GENERIC,        ///< Generic key blob - unknown type.
        AES,            ///< An AES key (length is obtained from the blob size)
        PRIVATE,        ///< An encrypted private key.
        PEM,            ///< PEM encoded public key cert.
        PUBLIC,         ///< public key.
        SPKI_CERT,      ///< SPKI cert
        DSA_PRIVATE,    ///< DSA private key
        DSA_PUBLIC,     ///< DSA public key
        INVALID         ///< Invalid key blob - This must be the last type.
    } Type;

    typedef enum {
        NO_ROLE,    ///< Key blob creator has no role
        INITIATOR,  ///< Key blob creator was an initiator
        RESPONDER   ///< Key blob creator was a reponder
    } Role;

    typedef enum {
        ASSOCIATE_NONE,     ///< not associated with any node
        ASSOCIATE_HEAD,     ///< the header node
        ASSOCIATE_MEMBER,   ///< the member node
        ASSOCIATE_BOTH      ///< both header and member node
    } AssociationMode;

    /**
     * Default constructor.
     */
    KeyBlob() : version(1), blobType(EMPTY), role(NO_ROLE), associationMode(ASSOCIATE_NONE), association(0) { }

    /**
     * KeyBlob with specific version
     * @param version   Valid versions are 0 and 1.
     */
    KeyBlob(uint8_t version) : version(version), blobType(EMPTY), role(NO_ROLE), associationMode(ASSOCIATE_NONE), association(0) { }

    /**
     * Constructor that initializes the key blob from a byte array.
     *
     * @param version   Valid versions are 0 and 1.
     * @param key       Pointer to memory containing data to initialize the key blob.
     * @param len       The length of the key in bytes.
     * @param initType  The type for the key blob.
     */
    KeyBlob(uint8_t version, const uint8_t* key, size_t len, const Type initType) : version(version), blobType(EMPTY), associationMode(ASSOCIATE_NONE), association(0) { Set(key, len, initType); }

    /**
     * Constructor that initializes the key blob from a byte array.
     *
     * @param key       Pointer to memory containing data to initialize the key blob.
     * @param len       The length of the key in bytes.
     * @param initType  The type for the key blob.
     */
    KeyBlob(const uint8_t* key, size_t len, const Type initType) : version(1), blobType(EMPTY), associationMode(ASSOCIATE_NONE), association(0) { Set(key, len, initType); }

    /**
     * Constructor that initializes the key blob from a string.
     *
     * @param version   Valid versions are 0 and 1.
     * @param str       String containing data to initialize the key blob.
     * @param blobType  The type for the key blob.
     */
    KeyBlob(uint8_t version, const qcc::String& str, const Type blobType) : version(version), blobType(EMPTY), associationMode(ASSOCIATE_NONE), association(0) { Set((const uint8_t*)str.data(), str.size(), blobType); }
    /**
     * Constructor that initializes the key blob from a string.
     *
     * @param str       String containing data to initialize the key blob.
     * @param blobType  The type for the key blob.
     */
    KeyBlob(const qcc::String& str, const Type blobType) : version(1), blobType(EMPTY), associationMode(ASSOCIATE_NONE), association(0) { Set((const uint8_t*)str.data(), str.size(), blobType); }

    /**
     * Copy constructor.
     *
     * @param other  Key blob to copy from.
     */
    KeyBlob(const KeyBlob& other);

    /**
     * Set a keyblob to a random value.
     *
     * @param len       Length of the randomly generated key blob
     * @param blobType  The type for the key blob.
     */
    void Rand(const size_t len, const Type initType);

    /**
     * Sets a key blob with data derived from a password.
     *
     * @param password  The password to generate the key from.
     * @param len       Length of the generated key blob
     * @param initType  The type for the key blob.
     */
    void Derive(const qcc::String& password, size_t len, const Type initType);

    /**
     * Xor a keyblob with another key blob.
     *
     * @param other  The other key blob to xor.
     */
    KeyBlob& operator^=(const KeyBlob& other);

    /**
     * Xor a keyblob with some other data.
     *
     * @param data The data to xor with the key blob.
     * @param len  The length of the data.
     *
     * @return  The number of bytes that were xor'd.
     */
    size_t Xor(const uint8_t* data, size_t len);

    /**
     * Erase this key blob.
     */
    void Erase();

    /**
     * Destructor erases the key blob contents
     */
    ~KeyBlob() { Erase(); }

    /**
     * Accessor function that returns the length of the key data.
     *
     * @return The length of the key blob.
     */
    size_t GetSize() const { return (blobType == EMPTY) ? 0 : size; }

    /**
     * Accessor function that returns the type of the key blob.
     */
    Type GetType() const { return blobType; }

    /**
     * Access function that returns the a pointer to the key blob contents.
     *
     * @return  Return a pointer to the key blob contents, or NULL if the key blob is not valid.
     */
    const uint8_t* GetData() const { return IsValid() ? data : NULL; }

    /**
     * Determine if the key is valid.
     *
     * @return true if the key is valid, otherwise false.
     */
    bool IsValid() const { return blobType != EMPTY; }

    /**
     * Set the key blob from a byte array.
     *
     * @param key       Pointer to memory containing the key blob.
     * @param len       The length of the key in bytes.
     * @param blobType  The type of the key blob
     *
     * @return ER_OK if the key was set
     */
    QStatus Set(const uint8_t* key, size_t len, Type blobType);

    /**
     * Accessor function to set the new blob type
     */
    void SetType(Type newType) { blobType = newType; }

    /**
     * Store a key blob in a sink.
     *
     * @param sink      The data sink to write the blob to.
     */
    QStatus Store(qcc::Sink& sink) const;

    /**
     * Load a key blob from a source.
     *
     * @param source    The data source to read the blob from.
     */
    QStatus Load(qcc::Source& source);

    /**
     * Assignment of a key blob
     */
    KeyBlob& operator=(const KeyBlob& other);

    /**
     * Set an absolute expiration date/time on a key blob.
     *
     * @param expires The expiration date/time.
     */
    void SetExpiration(const Timespec& expires) { expiration = expires; }

    /**
     * Default minimun expiration time for a key. If keys are expired too quickly they can end up
     * expiring before they get used for the first time so the default should allow for this.
     */
    static const uint32_t MIN_EXPIRATION_DEFAULT = 30;

    /**
     * Set a relative expiration date/time on a key blob.
     *
     * @param expiresInSeconds The number of seconds that the key will expire in. The max uint32
     *                         value indicates there is no expiration time.
     * @param minExpiration    Optional minimum expiration time.
     */
    void SetExpiration(uint32_t expiresInSeconds, uint32_t minExpiration = MIN_EXPIRATION_DEFAULT) {
        if (expiresInSeconds == 0xFFFFFFFF) {
            expiration.seconds = 0;
        } else {
            expiration = Timespec((uint64_t) std::max(minExpiration, expiresInSeconds) * 1000, TIME_RELATIVE);
        }
    }

    /**
     * Get an expiration date/time of a key blob if one was set.
     *
     * @param expires Retruns the expiration date/time if one was set.
     *
     * @return  true if an expiration time is set for this key blob.
     */
    bool GetExpiration(Timespec& expires) const {
        expires = expiration;
        return expiration.seconds != 0;
    }

    /**
     * Set a tag on the keyblob. A tag in an arbitrary string of 255 characters or less. The
     * role indicates if the creator of the key blob was an initiator or a responder.
     *
     * @param keyTag   The tag value to set.
     * @param keyRole     The role of the key blob creator
     */
    void SetTag(const qcc::String& keyTag, Role keyRole = NO_ROLE) { this->tag = keyTag.substr(0, MAX_TAG_LEN); this->role = keyRole; }

    /**
     * Gets the creator's role from a key blob
     *
     * @return  The creator's role
     */
    Role GetRole() const { return role; }

    /**
     * Get the opposite role of the creator's role
     *
     * @return  The opposite of the creator's role
     */
    Role GetAntiRole() const { return role == NO_ROLE ? role : (role == RESPONDER ? INITIATOR : RESPONDER); }

    /**
     * Get the tag from the keyblob. A tag in an arbitrary string that associates user specified
     * information with a keyblob. The tag is stored and loaded when the keyblob is stored or
     * loaded.
     *
     * @return   The tag if there is one.
     */
    const qcc::String& GetTag() const { return tag; }

    /**
     * Check if this keyblob has expired.
     *
     * @return Returns true if the keyblob has expired.
     */
    bool HasExpired();

    /**
     * Set the association guid.
     * @param  associatedGuid
     */
    void SetAssociation(qcc::GUID128 associatedGuid) {
        if (associationMode == ASSOCIATE_HEAD) {
            associationMode = ASSOCIATE_BOTH;
        } else {
            associationMode = ASSOCIATE_MEMBER;
        }
        association = associatedGuid;
    }
    /**
     * Get the association guid.
     * @return  associatedGuid
     */
    qcc::GUID128 GetAssociation() { return association; }
    /**
     * Set the association guid.
     * @param  associatedGuid
     */
    void SetAssociationMode(AssociationMode mode) {
        associationMode = mode;
    }
    /**
     * Set the association guid.
     * @param  associatedGuid
     */
    AssociationMode GetAssociationMode() { return associationMode; }
  private:

    uint8_t version;
    Type blobType;
    Timespec expiration;
    uint8_t* data;
    uint16_t size;
    qcc::String tag;
    Role role;
    AssociationMode associationMode;
    qcc::GUID128 association;

};

}

#endif
