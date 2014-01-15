/**
 * @file
 *
 * This file implements the STUN Message class
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

#include <assert.h>
#include <list>
#include <string>
#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <StunAttribute.h>
#include <StunMessage.h>
#include <StunTransactionID.h>
#include <alljoyn/Status.h>

#define QCC_MODULE "STUN_MESSAGE"

using namespace qcc;

bool StunMessage::IsTypeOK(uint16_t rawMsgType)
{
    QCC_DbgTrace(("StunMessage::IsTypeOK(rawMsgType = %04x)", rawMsgType));
    QCC_DbgPrintf(("    rawMsgType:  %04x => Method: %03x (%s)  Class: %x (%s)",
                   rawMsgType,
                   ExtractMessageMethod(rawMsgType),
                   MessageMethodToString(ExtractMessageMethod(rawMsgType)).c_str(),
                   ExtractMessageClass(rawMsgType),
                   MessageClassToString(ExtractMessageClass(rawMsgType)).c_str()));

    switch (ExtractMessageMethod(rawMsgType)) {
    case STUN_MSG_BINDING_METHOD:
        // Binding method supports all message classes.
        break;

    case STUN_MSG_ALLOCATE_METHOD:
    case STUN_MSG_REFRESH_METHOD:
    case STUN_MSG_CREATE_PERMISSION_METHOD:
    case STUN_MSG_CHANNEL_BIND_METHOD:
        // These methods only support request/response message classes.
        if (ExtractMessageClass(rawMsgType) == STUN_MSG_INDICATION_CLASS) {
            return false;
        }
        break;

    case STUN_MSG_SEND_METHOD:
    case STUN_MSG_DATA_METHOD:
        // Send and Data methods only support indication message classes.
        if (ExtractMessageClass(rawMsgType) != STUN_MSG_INDICATION_CLASS) {
            return false;
        }
        break;

    default:
        return false;
    }
    return true;
}



static QStatus ParseAttribute(const StunMessage& msg,
                              const uint8_t*& buf,
                              size_t& bufSize,
                              StunAttribute*& attr)
{
    QStatus status = ER_OK;
    size_t attrSize;
    uint16_t rawAttrSize = 0;
    uint16_t rawType = 0;
    size_t padding;

    QCC_DbgTrace(("ParseAttribute(msg = %s, *buf, bufSize = %lu, *attr)",
                  msg.ToString().c_str(), static_cast<unsigned long>(bufSize)));

    assert(buf != NULL);

    if (bufSize < (2 * sizeof(uint16_t))) {
        status = ER_BUFFER_TOO_SMALL;
        QCC_LogError(status, ("Parsing attribute header"));
        goto exit;
    }

    msg.ReadNetToHost(buf, bufSize, rawType);
    msg.ReadNetToHost(buf, bufSize, rawAttrSize);

    attrSize = rawAttrSize;

    padding = (-attrSize) & 0x3;
    QCC_DbgPrintf(("attrSize = %u  padding = %u  bufSize = %u", attrSize, padding, bufSize));

    if ((attrSize + padding) > bufSize) {
        status = ER_BUFFER_TOO_SMALL;
        QCC_LogError(status, ("Parsing attribute %04x (%u bytes missing)", rawType, (attrSize + padding) - bufSize));
        goto exit;
    }

    switch (static_cast<StunAttrType>(rawType)) {
    case STUN_ATTR_MAPPED_ADDRESS:
        attr = new StunAttributeMappedAddress();
        break;

    case STUN_ATTR_USERNAME:
        attr = new StunAttributeUsername();
        break;

    case STUN_ATTR_MESSAGE_INTEGRITY:
        attr = new StunAttributeMessageIntegrity(msg);
        break;

    case STUN_ATTR_ERROR_CODE:
        attr = new StunAttributeErrorCode();
        break;

    case STUN_ATTR_UNKNOWN_ATTRIBUTES:
        attr = new StunAttributeUnknownAttributes();
        break;

    case STUN_ATTR_XOR_MAPPED_ADDRESS:
        attr = new StunAttributeXorMappedAddress(msg);
        break;

    case STUN_ATTR_SOFTWARE:
        attr = new StunAttributeSoftware();
        break;

    case STUN_ATTR_ALTERNATE_SERVER:
        attr = new StunAttributeAlternateServer();
        break;

    case STUN_ATTR_FINGERPRINT:
        attr = new StunAttributeFingerprint(msg);
        break;

    case STUN_ATTR_PRIORITY:
        attr = new StunAttributePriority();
        break;

    case STUN_ATTR_USE_CANDIDATE:
        attr = new StunAttributeUseCandidate();
        break;

    case STUN_ATTR_ICE_CHECK_FLAG:
        attr = new StunAttributeIceCheckFlag();
        break;

    case STUN_ATTR_ICE_CONTROLLED:
        attr = new StunAttributeIceControlled();
        break;

    case STUN_ATTR_ICE_CONTROLLING:
        attr = new StunAttributeIceControlling();
        break;

    case STUN_ATTR_CHANNEL_NUMBER:
        attr = new StunAttributeChannelNumber();
        break;

    case STUN_ATTR_LIFETIME:
        attr = new StunAttributeLifetime();
        break;

    case STUN_ATTR_XOR_PEER_ADDRESS:
        attr = new StunAttributeXorPeerAddress(msg);
        break;

    case STUN_ATTR_ALLOCATED_XOR_SERVER_REFLEXIVE_ADDRESS:
        attr = new StunAttributeAllocatedXorServerReflexiveAddress(msg);
        break;

    case STUN_ATTR_DATA:
        attr = new StunAttributeData();
        break;

    case STUN_ATTR_XOR_RELAYED_ADDRESS:
        attr = new StunAttributeXorRelayedAddress(msg);
        break;

    case STUN_ATTR_EVEN_PORT:
        attr = new StunAttributeEvenPort();
        break;

    case STUN_ATTR_REQUESTED_TRANSPORT:
        attr = new StunAttributeRequestedTransport();
        break;

    case STUN_ATTR_DONT_FRAGMENT:
        attr = new StunAttributeDontFragment();
        break;

    case STUN_ATTR_RESERVATION_TOKEN:
        attr = new StunAttributeReservationToken();
        break;

    default:
        status = ER_STUN_INVALID_ATTR_TYPE;
        QCC_LogError(status, ("Parsing attribute"));
        break;
    }

    bufSize -= attrSize;

    if ((attr) &&
        (!((((static_cast<StunAttrType>(rawType)) == STUN_ATTR_MESSAGE_INTEGRITY) && (msg.GetTypeClass() == STUN_MSG_INDICATION_CLASS) && (msg.GetTypeMethod() == STUN_MSG_DATA_METHOD))))) {
        status = attr->Parse(buf, attrSize);
        QCC_DbgPrintf(("Parsed attribute: %s", attr->ToString().c_str()));
    } else {
        // Skip past the unknown attribute
        QCC_DbgPrintf(("Skipping unknown attribute or message integrity for data indications"));
        buf += attrSize;
        status = ER_OK;
    }

    if ((status == ER_OK) || (status == ER_STUN_INVALID_MESSAGE_INTEGRITY)) {
        // Skip over any padding.
        buf += padding;
        bufSize -= padding;
    } else {
        bufSize += attrSize;
    }

exit:
    return status;
}



StunMessage::~StunMessage(void)
{
    QCC_DbgTrace(("%s(%p)", __FUNCTION__, this));

    // Reclaim memory consumed by the attributes.
    while (!attrs.empty()) {
        StunAttribute* attr = attrs.front();
        attrs.pop_front();
        delete attr;
    }
}

QStatus StunMessage::Parse(const uint8_t*& buf, size_t& bufSize,
                           ExpectedResponseMap& expectedResponses)
{
    QStatus status = ER_OK;
    uint16_t rawMsgType = 0;  // Initializer to make GCC 4.3.2 happy.
    uint16_t rawMsgSize = 0;
    size_t msgSize;
    uint32_t magicCookie;
    bool isResponse = false;
    StunAttributeUsername* usernameAttr = NULL;
    StunAttributeMessageIntegrity* miAttr = NULL;

    QCC_DbgTrace(("StunMessage::Parse(*buf, bufSize = %u)", bufSize));

    assert(buf != NULL);

    if (bufSize < HEADER_SIZE + transaction.Size()) {
        status = ER_BUFFER_TOO_SMALL;
        QCC_LogError(status, ("Checking header size"));
        goto exit;
    }

    rawMsg = buf;

    ReadNetToHost(buf, bufSize, rawMsgType);
    ReadNetToHost(buf, bufSize, rawMsgSize);
    ReadNetToHost(buf, bufSize, magicCookie);

    msgSize = rawMsgSize;

    if (!IsTypeOK(rawMsgType)) {
        status = ER_STUN_INVALID_MSG_TYPE;
        QCC_DbgRemoteError(("Invalid message type: %04x (%s, %s)",
                            rawMsgType,
                            MessageClassToString(ExtractMessageClass(rawMsgType)).c_str(),
                            MessageMethodToString(ExtractMessageMethod(rawMsgType)).c_str()));
        QCC_DbgRemoteData(rawMsg, msgSize + MIN_MSG_SIZE);
        goto exit;
    }

    msgClass = ExtractMessageClass(rawMsgType);
    msgMethod = ExtractMessageMethod(rawMsgType);

    status = transaction.Parse(buf, bufSize);
    if (status != ER_OK) {
        // The transaction parser should have printed the error.
        goto exit;
    }

    if (msgClass == STUN_MSG_RESPONSE_CLASS || msgClass == STUN_MSG_ERROR_CLASS) {
        isResponse = true;
        ExpectedResponseMap::iterator iter = expectedResponses.find(transaction);
        if (iter != expectedResponses.end()) {
            if (msgClass == STUN_MSG_RESPONSE_CLASS) {
                hmacKey = iter->second.key;
                hmacKeyLen = iter->second.keyLen;
                QCC_DbgLocalData(hmacKey, hmacKeyLen);
            } else {
                // No MESSAGE-INTEGRITY for Error Responses.
                hmacKey = NULL;
            }
            expectedResponses.erase(iter);
        } else {
            // No MESSAGE-INTEGRITY expected since request did not have MESSAGE-INTEGRITY.
            hmacKey = NULL;
        }
    }


    if (msgSize > bufSize) {
        status = ER_BUFFER_TOO_SMALL;
        QCC_DbgRemoteError(("Checking message size (missing %u bytes)", msgSize - bufSize));
        goto exit;
    }

    // Modify out-parameter bufSize to indicate amount of buffer unread
    // assuming whole message was parsed successfully.
    bufSize -= msgSize;

    while (msgSize > 0) {
        StunAttribute* attr = NULL;
        QStatus parseStatus;

        parseStatus = ParseAttribute(*this, buf, msgSize, attr);
        if ((parseStatus != ER_OK) && (parseStatus != ER_STUN_INVALID_MESSAGE_INTEGRITY)) {
            // Add unread amount back to bufSize.
            bufSize += msgSize;
            // The attribute parser should have printed the error.
            goto exit;
        }

        if (attr) {
            if (attr->GetType() == STUN_ATTR_USERNAME) {
                usernameAttr = reinterpret_cast<StunAttributeUsername*>(attr);
            } else if (attr->GetType() == STUN_ATTR_MESSAGE_INTEGRITY) {
                miAttr = reinterpret_cast<StunAttributeMessageIntegrity*>(attr);
            }
        }

        if (status == ER_OK) {
            status = parseStatus;
        }

        if (attr) {
            attrs.push_back(attr);
        }
    }

    // Section 10.1.2 checks.
    if (isResponse) {
        if (usernameAttr != NULL) {
            status = ER_STUN_RESPONSE_WITH_USERNAME;
        }
    } else if ((usernameAttr == NULL) && (miAttr == NULL)) {
        // RFC 5389 seems to indicate that this is an error for requests and
        // indications but the TURN draft spec essentially requires this to be
        // acceptable in certain circumstances.
    } else if ((usernameAttr == NULL) || (miAttr == NULL)) {
        switch (msgClass) {
        case STUN_MSG_REQUEST_CLASS:
            status = ER_STUN_ERR400_BAD_REQUEST;
            break;

        case STUN_MSG_INDICATION_CLASS:
            //status = ER_STUN_BAD_INDICATION;
            break;

        default:
            break;
        }
    } else {
        String u;
        usernameAttr->GetUsername(u);
        QCC_DbgPrintf(("u=%s, username=%s", u.c_str(), username.c_str()));
        /* Allow Stun messages without a registered username to pass */
        if ((!username.empty() && (u != username)) || (status == ER_STUN_INVALID_MESSAGE_INTEGRITY)) {
            switch (msgClass) {
            case STUN_MSG_REQUEST_CLASS:
                //status = ER_STUN_ERR401_UNAUTHORIZED_REQUEST;
                break;

            case STUN_MSG_INDICATION_CLASS:
                //status = ER_STUN_UNAUTHORIZED_INDICATION;
                break;

            default:
                break;
            }
        }
    }


    QCC_DbgPrintf(("Parsed Message: %s", ToString().c_str()));

exit:
    rawMsg = NULL;
    return status;
}

QStatus StunMessage::RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const
{
    QStatus status = ER_OK;
    std::list<StunAttribute*>::const_iterator iter;
    size_t size = RenderSize();

    QCC_DbgTrace(("StunMessage::RenderBinary(*buf, bufSize = %lu, sg = <ScatterGatherList>)",
                  static_cast<unsigned long>(bufSize)));
    QCC_DbgPrintf(("        [message: %s]", ToString().c_str()));

    assert(buf != NULL);

    if (size > bufSize) {
        status = ER_BUFFER_TOO_SMALL;
        QCC_LogError(status, ("Checking buffer size"));
        goto exit;
    }

    if (size > MAX_IPV6_MTU) {
        status = ER_STUN_TOO_MANY_ATTRIBUTES;
        QCC_LogError(status, ("Checking message size"));
        goto exit;
    }

    WriteHostToNet(buf, bufSize, FormatMsgType(msgClass, msgMethod), sg);
    WriteHostToNet(buf, bufSize, static_cast<uint16_t>(Size() - MIN_MSG_SIZE), sg);
    WriteHostToNet(buf, bufSize, MAGIC_COOKIE, sg);

    status = transaction.RenderBinary(buf, bufSize, sg);
    if (status != ER_OK) {
        goto exit;
    }

    for (iter = Begin(); iter != End(); ++iter) {
        QCC_DbgPrintf(("Rendering %s (%u:%u)",
                       (*iter)->ToString().c_str(), (*iter)->AttrSize(), (*iter)->Size()));
        status = (*iter)->RenderBinary(buf, bufSize, sg);
        if (status != ER_OK) {
            QCC_LogError(status, ("Rendering %s", (*iter)->ToString().c_str()));
            goto exit;
        }
    }

exit:
    return status;
}

size_t StunMessage::RenderSize(void) const
{
    std::list<StunAttribute*>::const_iterator iter;
    size_t size = HEADER_SIZE + transaction.RenderSize();

    for (iter = Begin(); iter != End(); ++iter) {
        size += (*iter)->RenderSize();
    }

    return size;
}

size_t StunMessage::Size(void) const
{
    std::list<StunAttribute*>::const_iterator iter;
    size_t size = MIN_MSG_SIZE;

    for (iter = Begin(); iter != End(); ++iter) {
        size += (*iter)->Size();
    }

    return size;
}


bool StunMessage::IsStunMessage(const uint8_t* buf, size_t bufSize)
{
    uint32_t msgTypeSize = 0;   // Initializer to make GCC 4.3.2 happy.
    uint32_t magicCookie = 0;   // Initializer to make GCC 4.3.2 happy.

    assert(buf != NULL);

    //  If the buffer size is too small then its not a STUN message.
    if (bufSize < MIN_MSG_SIZE) {
        return false;
    }

    // Check that the 2 MSB are 0 and that the size is a mulitple of 4.
    ReadNetToHost(buf, bufSize, msgTypeSize);
    if ((msgTypeSize & 0xC003) != 0) {
        return false;
    }

    // The primary check from RFC 5389 is to verify that the magic cookie matches.
    ReadNetToHost(buf, bufSize, magicCookie);
    if (magicCookie != MAGIC_COOKIE) {
        return false;
    }

    return true;
}


QStatus StunMessage::AddAttribute(StunAttribute* attr)
{
    QStatus status = ER_OK;
    std::list<StunAttribute*>::iterator pos;
    StunAttrType attrType;

    QCC_DbgTrace(("StunMessage::AddAttribute(attr = %s)", attr ? attr->ToString().c_str() : "<null>"));
    assert(attr != NULL);

    attrType = attr->GetType();

    pos = attrs.end();
    if (pos != attrs.begin()) {
        --pos;
        if ((*pos)->GetType() == STUN_ATTR_FINGERPRINT) {
            if (attrType == STUN_ATTR_FINGERPRINT) {
                status = ER_STUN_DUPLICATE_ATTRIBUTE;
                QCC_LogError(status, ("Adding attribute %s", attr->ToString().c_str()));
                goto exit;
            }

            --pos;
            if ((*pos)->GetType() == STUN_ATTR_MESSAGE_INTEGRITY) {
                if (attrType == STUN_ATTR_MESSAGE_INTEGRITY) {
                    status = ER_STUN_DUPLICATE_ATTRIBUTE;
                    QCC_LogError(status, ("Adding attribute %s", attr->ToString().c_str()));
                    goto exit;
                }
            } else {
                ++pos;  // Restore insert position;
            }
        } else {
            ++pos;  // Restore insert position;
        }
    }
    attrs.insert(pos, attr);

exit:
    return status;
}



#if !defined(NDEBUG)
String StunMessage::MessageClassToString(StunMsgTypeClass msgClass)
{
    switch (msgClass) {
    case STUN_MSG_REQUEST_CLASS:
        return String("Request");

    case STUN_MSG_RESPONSE_CLASS:
        return String("Response");

    case STUN_MSG_INDICATION_CLASS:
        return String("Indication");

    case STUN_MSG_ERROR_CLASS:
        return String("Error Response");

    default:
        return String("<Unknown>");
    }
}

String StunMessage::MessageMethodToString(StunMsgTypeMethod msgMethod)
{
    switch (msgMethod) {
    case STUN_MSG_BINDING_METHOD:
        return String("Binding");

    case STUN_MSG_ALLOCATE_METHOD:
        return String("Allocate");

    case STUN_MSG_REFRESH_METHOD:
        return String("Refresh");

    case STUN_MSG_SEND_METHOD:
        return String("Send");

    case STUN_MSG_DATA_METHOD:
        return String("Data");

    case STUN_MSG_CREATE_PERMISSION_METHOD:
        return String("Create Permission");

    case STUN_MSG_CHANNEL_BIND_METHOD:
        return String("Channel Bind");

    default:
        return String("<Unknown>");
    }
}


String StunMessage::ToString(void) const
{
    size_t msgLen = Size() - MIN_MSG_SIZE;
    String oss;

    oss.append("STUN Message: [Class: ");
    oss.append(MessageClassToString(msgClass));
    oss.append("   Method: ");
    oss.append(MessageMethodToString(msgMethod));
    oss.append("   Length: ");
    oss.append(U32ToString(msgLen, 10));
    oss.push_back(']');

    return oss;
}
#endif
