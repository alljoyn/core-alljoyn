#ifndef _STUNMESSAGE_H
#define _STUNMESSAGE_H
/**
 * @file
 *
 * This file defines the STUN Message class.
 */

/******************************************************************************
 * Copyright (c) 2009,2012 AllSeen Alliance. All rights reserved.
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

#ifndef __cplusplus
#error Only include StunMessage.h in C++ code.
#endif

#include <assert.h>
#include <list>
#include <map>
#include <string>
#include <qcc/platform.h>
#include <StunAttributeBase.h>
#include <StunAttributeFingerprint.h>
#include <StunAttributeMessageIntegrity.h>
#include <StunAttributeXorMappedAddress.h>
#include <StunIOInterface.h>
#include <StunTransactionID.h>
#include <alljoyn/Status.h>

using namespace qcc;

/** @internal */
#define QCC_MODULE "STUN_MESSAGE"


/**
 * Macro use to pre-format the STUN message type method bit patterns.
 *
 * @param _method STUN message type method number.
 *
 * @return STUN message type method number with bits arranged for easy
 *         combination with STUN message type classes.
 */
#define StunFmtMethod(_method) ((((_method) & 0xf80) << 2) | \
                                (((_method) & 0x070) << 1) | \
                                ((_method) & 0x00f))

/**
 * Macro use to pre-format the STUN message type class bit patterns.
 *
 * @param _class STUN message type class number.
 *
 * @return STUN message type class number with bits arranged for easy
 *         combination with STUN message type methods.
 */
#define StunFmtClass(_class) ((((_class) & 0x2) << 7) | \
                              (((_class) & 0x1) << 4))

/**
 * STUN message type methods as defined in RFC 5389.
 */
enum StunMsgTypeMethod {
    STUN_MSG_BINDING_METHOD =           StunFmtMethod(0x001), ///< Binding message method.
    STUN_MSG_ALLOCATE_METHOD =          StunFmtMethod(0x003), ///< Allocate message method.
    STUN_MSG_REFRESH_METHOD =           StunFmtMethod(0x004), ///< Refresh message method.
    STUN_MSG_SEND_METHOD =              StunFmtMethod(0x006), ///< Send message method.
    STUN_MSG_DATA_METHOD =              StunFmtMethod(0x007), ///< Data message method.
    STUN_MSG_CREATE_PERMISSION_METHOD = StunFmtMethod(0x008), ///< Create Permission message method.
    STUN_MSG_CHANNEL_BIND_METHOD =      StunFmtMethod(0x009)  ///< Channel Bind message method.
};

/**
 * STUN message type classes as defined in RFC 5389.
 */
enum StunMsgTypeClass {
    STUN_MSG_REQUEST_CLASS =    StunFmtClass(0x0), ///< Request message class.
    STUN_MSG_INDICATION_CLASS = StunFmtClass(0x1), ///< Indication message class.
    STUN_MSG_RESPONSE_CLASS =   StunFmtClass(0x2), ///< Success Response message class.
    STUN_MSG_ERROR_CLASS =      StunFmtClass(0x3)  ///< Error Response message class.
};


/**
 * The StunMessage class is primarily just a container for a list of
 * StunAttributes.
 */
class StunMessage : public StunIOInterface {
  public:
    /// Define type of dereferenced iterators.
    typedef StunAttribute* type_value;

    /// iterator typedef.
    typedef std::list<StunAttribute*>::iterator iterator;

    /// const_iterator typedef.
    typedef std::list<StunAttribute*>::const_iterator const_iterator;

    typedef struct _keyInfo {
        uint8_t* key;
        size_t keyLen;
    } keyInfo;

    typedef std::map<StunTransactionID, keyInfo> ExpectedResponseMap;

  private:

    StunMessage(const StunMessage& msg);
    StunMessage operator=(const StunMessage& msg);

    std::list<StunAttribute*> attrs; ///< List of STUN message attributes.
    std::list<uint16_t> badAttrs;    ///< List of Unrecognized attributes (used in parsing).
    StunMsgTypeClass msgClass;      ///< Message type class.
    StunMsgTypeMethod msgMethod;    ///< Message type method.
    StunTransactionID transaction;  ///< Transaction ID of the STUN message.

    const uint8_t* rawMsg;         ///< Raw message pointer (for fingerprint and message-integrity).
    const String username;    ///< Expected value of USERNAME attribute for incoming requests.
    const uint8_t* hmacKey;        ///< HMAC key for computing the message integrity value.
    size_t hmacKeyLen;

    static const uint16_t METHOD_MASK = 0x3eef;   ///< STUN message type method bit mask.
    static const uint16_t CLASS_MASK =  0x0110;   ///< STUN message type class bit mask.

#if !defined(NDEBUG)
    /**
     * Helper function to convert STUN message type classes to NUL terminated
     * strings.  This is only available when building in debug mode.
     *
     * @param msgClass STUN message type class.
     *
     * @return A NUL terminated string representation of the STUN message type class.
     */
  public:
    static String MessageClassToString(StunMsgTypeClass msgClass);

    /**
     * Helper function to convert STUN message type methods to NUL terminated
     * strings.  This is only available when building in debug mode.
     *
     * @param msgMethod STUN message type method.
     *
     * @return A NUL terminated string representation of the STUN message type method.
     */
    static String MessageMethodToString(StunMsgTypeMethod msgMethod);
  private:
#endif

    /**
     * Combines the STUN message type class and method into the STUN message
     * type field.
     *
     * @param msgClass The STUN message type class.
     * @param msgMethod The STUN message type method.
     *
     * @return Bit pattern for the STUN message type field.
     */
    static uint16_t FormatMsgType(StunMsgTypeClass msgClass, StunMsgTypeMethod msgMethod)
    {
        return (msgClass | msgMethod);
    }

    /**
     * StunAttributeFingerprint::Parse needs access to the entire raw message
     * buffer to compute a CRC-32 value for comparison.
     *
     * @param buf       Buffer to be parsed.
     * @param bufSize   Buffer size.
     *
     * @return  Indication of success or failure.
     */
    friend QStatus StunAttributeFingerprint::Parse(const uint8_t*& buf, size_t& bufSize);

    /**
     * StunAttributeMessageIntegrity::Parse needs access to the entire raw
     * message buffer and the HMAC key for calculating the SHA1 hash and
     * performing a comparison.
     *
     * @param buf       Buffer to be parsed.
     * @param bufSize   Buffer size.
     *
     * @return  Indication of success or failure.
     */
    friend QStatus StunAttributeMessageIntegrity::Parse(const uint8_t*& buf, size_t& bufSize);

    /**
     * StunAttributeMessageIntegrity::RenderBinary need access to the message
     * rendering buffer information and the HMAC key for calculating the SHA1
     * hash.
     *
     * @param buf       Buffer where data gets rendered.
     * @param bufSize   Buffer size.
     * @param sg        Scatter-gather list where additional buffers are appended.
     *
     * @return  Indication of success or failure.
     */
    friend QStatus StunAttributeMessageIntegrity::RenderBinary(uint8_t*& buf,
                                                               size_t& bufSize,
                                                               ScatterGatherList& sg) const;

    /**
     * StunAttributeXorMappedAddress::Parse need access to the Magic Cookie
     * value and transaction ID in order to perform the XOR function.
     *
     * @param buf       Buffer to be parsed.
     * @param bufSize   Buffer size.
     *
     * @return  Indication of success or failure.
     */
    friend QStatus StunAttributeXorMappedAddress::Parse(const uint8_t*& buf,
                                                        size_t& bufSize);

    /**
     * StunAttributeXorMappedAddress::RenderBinary need access to the Magic
     * Cookie value and transaction ID in order to perform the XOR function.
     *
     * @param buf       Buffer where data gets rendered.
     * @param bufSize   Buffer size.
     * @param sg        Scatter-gather list where additional buffers are appended.
     *
     * @return  Indication of success or failure.
     */
    friend QStatus StunAttributeXorMappedAddress::RenderBinary(uint8_t*& buf,
                                                               size_t& bufSize,
                                                               ScatterGatherList& sg) const;

  public:

    /**
     * Determines if the message type is valid.
     *
     * @param rawMsgType    Value to determine if it is a valid message type or not.
     *
     * @return  "true" if the passed in message type is valid.
     */
    static bool IsTypeOK(uint16_t rawMsgType);

    /**
     * STUN Message constructor intended for receiving STUN messages.
     *
     * @param username  Expected value of USERNAME attribute in requests sent by
     *                  the peer.  This should be "LFRAG:RFRAG" in accordance
     *                  with section 7.1.1.3 of the ICE spec.
     * @param hmacKey   HMAC Key used for computing message integrity values.
     *                  This should be computed from the username of
     *                  "LFRAG:RFRAG" and password of LPASS in accordance with
     *                  section 7.1.1.3 of the ICE spec.
     */
    StunMessage(const String username,
                const uint8_t* hmackey,
                size_t keyLen) :
        rawMsg(NULL),
        username(username),
        hmacKey(hmackey),
        hmacKeyLen(keyLen)
    { }

    /**
     * STUN Message constructor intended for sending STUN Requests and Indications.
     *
     * @param msgClass  STUN message class.
     * @param msgMethod STUN message method.
     * @param hmacKey   HMAC Key used for computing message integrity values.
     */
    StunMessage(StunMsgTypeClass msgClass,
                StunMsgTypeMethod msgMethod,
                const uint8_t* hmackey,
                size_t keyLen) :
        msgClass(msgClass),
        msgMethod(msgMethod),
        rawMsg(NULL),
        hmacKey(hmackey),
        hmacKeyLen(keyLen)
    {
        assert(msgClass == STUN_MSG_REQUEST_CLASS || msgClass == STUN_MSG_INDICATION_CLASS);
        transaction.SetValue();
    }

    /**
     * STUN Message constructor intended for sending STUN Responses (and
     * retransmits of STUN Requests and Indications using same transaction ID).
     *
     * @param msgClass  STUN message class.
     * @param msgMethod STUN message method.
     * @param hmacKey   HMAC Key used for computing message integrity values.
     * @param tid       Use this Transaction ID.
     */
    StunMessage(StunMsgTypeClass msgClass,
                StunMsgTypeMethod msgMethod,
                const uint8_t* hmackey,
                size_t keyLen,
                StunTransactionID& tid) :
        msgClass(msgClass),
        msgMethod(msgMethod),
        transaction(tid),
        rawMsg(NULL),
        hmacKey(hmackey),
        hmacKeyLen(keyLen)
    {
    }

    ~StunMessage(void);

    QStatus Parse(const uint8_t*& buf, size_t& bufSize, ExpectedResponseMap& expectedResponses);

    QStatus Parse(const uint8_t*& buf, size_t& bufSize)
    {
        ExpectedResponseMap erm;
        return Parse(buf, bufSize, erm);
    }
    QStatus RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const;
#ifndef NDEBUG
    String ToString(void) const;
#endif
    size_t RenderSize(void) const;
    size_t Size(void) const;

    /**
     * Quickly check if a the beginning of a buffer is a STUN message.
     *
     * @param buf      Pointer into a buffer to check for a STUN message.
     * @param bufSize  Size of the buffer to check.
     *
     * @return true if the buffer contents indicate a STUN message; otherwise false.
     */
    static bool IsStunMessage(const uint8_t* buf, size_t bufSize);

    /**
     * This function extracts the STUN message size from a buffer.  It is
     * assumed that the buffer has been confirmed to be a STUN message with
     * StunMessage::IsStunMessage().
     *
     * @param buf   Pointer to the buffer containing the STUN message.
     */
    static uint16_t ParseMessageSize(const uint8_t* buf)
    {
        return (static_cast<uint16_t>(buf[2]) << 8) | static_cast<uint16_t>(buf[3]);
    }

    /**
     * Retrieve an iterator to beginning of the list of STUN attributes.
     *
     * @return An iterator to the first attribute.
     */
    const_iterator Begin(void) const { return attrs.begin(); }

    /**
     * Retrieve an iterator to the end of the list of STUN attributes.
     *
     * @return An iterator to the postion just past the last attribute.
     */
    const_iterator End(void) const { return attrs.end(); }

    /**
     * Get a copy of the message transaction ID.
     *
     * @param tid OUT: A reference to where the STUN Message Transaction ID
     *            should be copied.
     */
    void GetTransactionID(StunTransactionID& tid) const { tid = transaction; }

    /**
     * Adds a STUN Message attribute to the message.  This is used for
     * building up a STUN message that will later be later sent out.
     * Attributes may added in any order.  This function will ensure that
     * those attributes that require specific ordering (i.e., @ref
     * StunAttributeMessageIntegrity "MESSAGE-INTEGRITY" and @ref
     * StunAttributeFingerprint "FINGERPRINT") will be placed in the correct
     * order when rendered.
     *
     * @param attr reference to a StunAttribute.
     *
     * @return Indication of whether adding the attribute succeeded or failed.
     */
    QStatus AddAttribute(StunAttribute* attr);

    StunMsgTypeClass GetTypeClass(void) const { return msgClass; }

    StunMsgTypeMethod GetTypeMethod(void) const { return msgMethod; }

    const uint8_t* GetHMACKey(void) const { return hmacKey; }

    const size_t GetHMACKeyLength(void) const { return hmacKeyLen; }

    /**
     * Simple function to extract the STUN message type method.
     *
     * @param msgType The message type field from a STUN message.
     *
     * @return The bit pattern for just the STUN message type method.
     */
    static StunMsgTypeMethod ExtractMessageMethod(uint16_t msgType)
    {
        return (StunMsgTypeMethod)(msgType & METHOD_MASK);
    }

    /**
     * Simple function to extract the STUN message type class.
     *
     * @param msgType The message type field from a STUN message.
     *
     * @return The bit pattern for just the STUN message type class.
     */
    static StunMsgTypeClass ExtractMessageClass(uint16_t msgType)
    {
        return (StunMsgTypeClass)(msgType & CLASS_MASK);
    }

    static const uint32_t MAGIC_COOKIE = 0x2112A442;   ///< STUN Magic Cookie defined in RFC 5389.


    /// STUN message header size.
    static const uint32_t HEADER_SIZE = (sizeof(uint16_t) +   // STUN Message type size
                                         sizeof(uint16_t) +   // STUN Message length size
                                         sizeof(MAGIC_COOKIE));

    /// Minimum message size.
    static const size_t MIN_MSG_SIZE = HEADER_SIZE + StunTransactionID::SIZE;

};

#undef QCC_MODULE
#endif
