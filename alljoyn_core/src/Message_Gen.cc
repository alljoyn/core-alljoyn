/**
 * @file
 *
 * This file implements the message generation side of the message bus message class
 */

/******************************************************************************
 * Copyright (c) 2009-2012, 2014-2015, AllSeen Alliance. All rights reserved.
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

#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Debug.h>
#include <qcc/Socket.h>
#include <qcc/time.h>
#include <qcc/Util.h>

#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/Message.h>
#include <alljoyn/MsgArg.h>

#include "LocalTransport.h"
#include "PeerState.h"
#include "KeyStore.h"
#include "CompressionRules.h"
#include "BusUtil.h"
#include "AllJoynCrypto.h"
#include "AllJoynPeerObj.h"
#include "SignatureUtils.h"
#include "BusInternal.h"

#define QCC_MODULE "ALLJOYN"

using namespace qcc;
using namespace std;

namespace ajn {


#define Marshal8(n) \
    do { \
        *((uint64_t*)bufPos) = n; \
        bufPos += 8; \
    } while (0)

#define Marshal4(n) \
    do { \
        *((uint32_t*)bufPos) = n; \
        bufPos += 4; \
    } while (0)

#define Marshal2(n) \
    do { \
        *((uint16_t*)bufPos) = n; \
        bufPos += 2; \
    } while (0)

#define Marshal1(n) \
    do { \
        *bufPos++ = n; \
    } while (0)

#define MarshalBytes(data, len) \
    do { \
        memcpy(bufPos, data, len); \
        bufPos += len; \
    } while (0)

#define MarshalReversed(data, len) \
    do { \
        uint8_t* p = ((uint8_t*)(void*)data) + len; \
        while (p-- != (uint8_t*)(void*)data) { \
            *bufPos++ = *p; \
        } \
    } while (0)

#define MarshalPad(_alignment) \
    do { \
        size_t pad = PadBytes(bufPos, _alignment); \
        if (pad & 1) { Marshal1(0); } \
        if (pad & 2) { Marshal2(0); } \
        if (pad & 4) { Marshal4(0); } \
    } while (0)

/*
 * Round up to a multiple of 8
 */
#define ROUNDUP8(n)  (((n) + 7) & ~7)

static inline QStatus CheckedArraySize(size_t sz, uint32_t& len)
{
    if (sz > ALLJOYN_MAX_ARRAY_LEN) {
        QStatus status = ER_BUS_BAD_LENGTH;
        QCC_LogError(status, ("Array too big"));
        return status;
    } else {
        len = (uint32_t)sz;
        return ER_OK;
    }
}

QStatus _Message::MarshalArgs(const MsgArg* arg, size_t numArgs)
{
    QStatus status = ER_OK;
    size_t alignment;
    uint32_t len;

    while (numArgs--) {
        if (!arg) {
            status = ER_BUS_BAD_VALUE;
            break;
        }
        /*
         * Align on boundary for type as specified in the wire protocol
         */
        MarshalPad(SignatureUtils::AlignmentForType(arg->typeId));

        switch (arg->typeId) {
        case ALLJOYN_DICT_ENTRY:
            status = MarshalArgs(arg->v_dictEntry.key, 1);
            if (status == ER_OK) {
                status = MarshalArgs(arg->v_dictEntry.val, 1);
            }
            break;

        case ALLJOYN_STRUCT:
            status = MarshalArgs(arg->v_struct.members, arg->v_struct.numMembers);
            break;

        case ALLJOYN_ARRAY:
            if (!arg->v_array.elemSig) {
                status = ER_BUS_BAD_VALUE;
                break;
            }
            alignment = SignatureUtils::AlignmentForType((AllJoynTypeId)(arg->v_array.elemSig[0]));
            if (arg->v_array.numElements > 0) {
                if (!arg->v_array.elements) {
                    status = ER_BUS_BAD_VALUE;
                    break;
                }
                /*
                 * Check elements conform to the expected signature type
                 */
                for (size_t i = 0; i < arg->v_array.numElements; i++) {
                    if (!arg->v_array.elements[i].HasSignature(arg->v_array.GetElemSig())) {
                        status = ER_BUS_BAD_VALUE;
                        QCC_LogError(status, ("Array element[%d] does not have expected signature \"%s\"", i, arg->v_array.GetElemSig()));
                        break;
                    }
                }
                if (status == ER_OK) {
                    uint8_t* lenPos = bufPos;
                    bufPos += 4;
                    /* Length does not include padding for first element, so pad to 8 byte boundary if required. */
                    if (alignment == 8) {
                        MarshalPad(8);
                    }
                    uint8_t* elemPos = bufPos;
                    status = MarshalArgs(arg->v_array.elements, arg->v_array.numElements);
                    if (status != ER_OK) {
                        break;
                    }
                    status = CheckedArraySize(bufPos - elemPos, len);
                    if (status != ER_OK) {
                        break;
                    }
                    /* Patch in length */
                    uint8_t* tmpPos = bufPos;
                    bufPos = lenPos;
                    if (endianSwap) {
                        MarshalReversed(&len, 4);
                    } else {
                        Marshal4(len);
                    }
                    bufPos = tmpPos;
                }
            } else {
                Marshal4(0);
                if (alignment == 8) {
                    MarshalPad(8);
                }
            }
            break;

        case ALLJOYN_BOOLEAN_ARRAY:
            status = CheckedArraySize(4 * arg->v_scalarArray.numElements, len);
            if (status != ER_OK) {
                break;
            }
            if (len && !arg->v_scalarArray.v_bool) {
                status = ER_BUS_BAD_VALUE;
                break;
            }
            if (endianSwap) {
                MarshalReversed(&len, 4);
            } else {
                Marshal4(len);
            }
            for (size_t i = 0; i < arg->v_scalarArray.numElements; i++) {
                uint32_t b = arg->v_scalarArray.v_bool[i] ? 1 : 0;
                if (endianSwap) {
                    MarshalReversed(&b, 4);
                } else {
                    Marshal4(b);
                }
            }
            break;

        case ALLJOYN_INT32_ARRAY:
        case ALLJOYN_UINT32_ARRAY:
            status = CheckedArraySize(4 * arg->v_scalarArray.numElements, len);
            if (status != ER_OK) {
                break;
            }
            if (len && !arg->v_scalarArray.v_uint32) {
                status = ER_BUS_BAD_VALUE;
                break;
            }
            if (endianSwap) {
                MarshalReversed(&len, 4);
                for (size_t i = 0; i < arg->v_scalarArray.numElements; i++) {
                    MarshalReversed(&arg->v_scalarArray.v_uint32[i], 4);
                }
            } else {
                Marshal4(len);
                if (arg->v_scalarArray.v_uint32) {
                    MarshalBytes(arg->v_scalarArray.v_uint32, len);
                }
            }
            break;

        case ALLJOYN_DOUBLE_ARRAY:
        case ALLJOYN_UINT64_ARRAY:
        case ALLJOYN_INT64_ARRAY:
            status = CheckedArraySize(8 * arg->v_scalarArray.numElements, len);
            if (status != ER_OK) {
                break;
            }
            if (len > 0) {
                if (!arg->v_scalarArray.v_uint64) {
                    status = ER_BUS_BAD_VALUE;
                    break;
                }
                if (endianSwap) {
                    MarshalReversed(&len, 4);
                    MarshalPad(8);
                    for (size_t i = 0; i < arg->v_scalarArray.numElements; i++) {
                        MarshalReversed(&arg->v_scalarArray.v_uint64[i], 8);
                    }
                } else {
                    Marshal4(len);
                    MarshalPad(8);
                    if (arg->v_scalarArray.v_uint64) {
                        MarshalBytes(arg->v_scalarArray.v_uint64, len);
                    }
                }
            } else {
                /* Even empty arrays are padded to the element type alignment boundary */
                Marshal4(0);
                MarshalPad(8);
            }
            break;

        case ALLJOYN_INT16_ARRAY:
        case ALLJOYN_UINT16_ARRAY:
            status = CheckedArraySize(2 * arg->v_scalarArray.numElements, len);
            if (status != ER_OK) {
                break;
            }
            if (len && !arg->v_scalarArray.v_uint16) {
                status = ER_BUS_BAD_VALUE;
                break;
            }
            if (endianSwap) {
                MarshalReversed(&len, 4);
                for (size_t i = 0; i < arg->v_scalarArray.numElements; i++) {
                    MarshalReversed(&arg->v_scalarArray.v_uint16[i], 2);
                }
            } else {
                Marshal4(len);
                if (arg->v_scalarArray.v_uint16) {
                    MarshalBytes(arg->v_scalarArray.v_uint16, len);
                }
            }
            break;

        case ALLJOYN_BYTE_ARRAY:
            status = CheckedArraySize(arg->v_scalarArray.numElements, len);
            if (status != ER_OK) {
                break;
            }
            if (len && !arg->v_scalarArray.v_byte) {
                status = ER_BUS_BAD_VALUE;
                break;
            }
            if (endianSwap) {
                MarshalReversed(&len, 4);
            } else {
                Marshal4(len);
            }
            if (arg->v_scalarArray.v_byte) {
                MarshalBytes(arg->v_scalarArray.v_byte, arg->v_scalarArray.numElements);
            }
            break;

        case ALLJOYN_BOOLEAN:
            if (arg->v_bool) {
                if (endianSwap) {
                    uint32_t b = 1;
                    MarshalReversed(&b, 4);
                } else {
                    Marshal4(1);
                }
            } else {
                Marshal4(0);
            }
            break;

        case ALLJOYN_INT32:
        case ALLJOYN_UINT32:
            if (endianSwap) {
                MarshalReversed(&arg->v_uint32, 4);
            } else {
                Marshal4(arg->v_uint32);
            }
            break;

        case ALLJOYN_DOUBLE:
        case ALLJOYN_UINT64:
        case ALLJOYN_INT64:
            if (endianSwap) {
                MarshalReversed(&arg->v_uint64, 8);
            } else {
                Marshal8(arg->v_uint64);
            }
            break;

        case ALLJOYN_SIGNATURE:
            if (arg->v_signature.sig) {
                if (arg->v_signature.sig[arg->v_signature.len]) {
                    status = ER_BUS_NOT_NUL_TERMINATED;
                    break;
                }
                Marshal1(arg->v_signature.len);
                MarshalBytes((void*)arg->v_signature.sig, arg->v_signature.len + 1);
            } else {
                Marshal1(0);
                Marshal1(0);
            }
            break;

        case ALLJOYN_INT16:
        case ALLJOYN_UINT16:
            if (endianSwap) {
                MarshalReversed(&arg->v_uint16, 2);
            } else {
                Marshal2(arg->v_uint16);
            }
            break;

        case ALLJOYN_OBJECT_PATH:
            if (!arg->v_objPath.str || (arg->v_objPath.len == 0)) {
                status = ER_BUS_BAD_OBJ_PATH;
                break;
            }
        /* no break FALLTHROUGH*/

        case ALLJOYN_STRING:
            if (arg->v_string.str) {
                if (arg->v_string.str[arg->v_string.len]) {
                    status = ER_BUS_NOT_NUL_TERMINATED;
                    break;
                }
                if (endianSwap) {
                    MarshalReversed(&arg->v_string.len, 4);
                } else {
                    Marshal4(arg->v_string.len);
                }
                MarshalBytes((void*)arg->v_string.str, arg->v_string.len + 1);
            } else {
                Marshal4(0);
                Marshal1(0);
            }
            break;

        case ALLJOYN_VARIANT:
            {
                /* First byte is reserved for the length */
                char sig[257];
                size_t len = 0;
                status = SignatureUtils::MakeSignature(arg->v_variant.val, 1, sig + 1, len);
                if (status == ER_OK) {
                    sig[0] = (char)len;
                    MarshalBytes(sig, len + 2);
                    status = MarshalArgs(arg->v_variant.val, 1);
                }
            }
            break;

        case ALLJOYN_BYTE:
            Marshal1(arg->v_byte);
            break;

        case ALLJOYN_HANDLE:
            {
                uint32_t index = 0;
                /* Check if handle is already listed */
                while ((index < numHandles) && (handles[index] != arg->v_handle.fd)) {
                    ++index;
                }
                /* If handle was not found expand handle array */
                if (index == numHandles) {
                    qcc::SocketFd* h = new qcc::SocketFd[numHandles + 1];
                    memcpy(h, handles, numHandles * sizeof(qcc::SocketFd));
                    delete [] handles;
                    handles = h;
                    status = qcc::SocketDup(arg->v_handle.fd, handles[numHandles++]);
                    if (status != ER_OK) {
                        --numHandles;
                        break;
                    }
                }
                /* Marshal the index of the handle */
                if (endianSwap) {
                    MarshalReversed(&index, 4);
                } else {
                    Marshal4(index);
                }
            }
            break;

        default:
            status = ER_BUS_BAD_VALUE_TYPE;
            break;
        }
        if (status != ER_OK) {
            break;
        }
        ++arg;
    }
    return status;
}

QStatus _Message::Deliver(RemoteEndpoint& endpoint)
{
    QStatus status = ER_OK;
    Sink& sink = endpoint->GetSink();
    uint8_t* buf = reinterpret_cast<uint8_t*>(msgBuf);
    size_t len = bufEOD - buf;
    size_t pushed;

    QCC_DbgPrintf(("Deliver %s", this->Description().c_str()));

    if (len == 0) {
        status = ER_BUS_EMPTY_MESSAGE;
        QCC_LogError(status, ("Message is empty"));
        return status;
    }
    /*
     * Handles can only be passed if that feature was negotiated.
     */
    if (handles && !endpoint->GetFeatures().handlePassing) {
        status = ER_BUS_HANDLES_NOT_ENABLED;
        QCC_LogError(status, ("Handle passing was not negotiated on this connection"));
        return status;
    }
    /*
     * If the message has a TTL, check if it has expired
     */
    if (ttl && IsExpired()) {
        QCC_DbgHLPrintf(("TTL has expired - discarding message %s", Description().c_str()));
        return ER_OK;
    }
    /*
     * Check if message needs to be encrypted
     */
    if (encrypt) {
        status = EncryptMessage();
        /*
         * Delivery is retried when the authentication completes
         */
        if (status == ER_BUS_AUTHENTICATION_PENDING) {
            return ER_OK;
        }
    }
    /*
     * Push the message to the endpoint sink (only push handles in the first chunk)
     */
    if (status == ER_OK) {
        if (handles) {
            status = sink.PushBytesAndFds(buf, len, pushed, handles, numHandles, endpoint->GetProcessId());
        } else {
            status = sink.PushBytes(buf, len, pushed, (msgHeader.flags & ALLJOYN_FLAG_SESSIONLESS) ? (ttl * 1000) : ttl);
        }
    }
    /*
     * Continue pushing until we are done
     */
    while ((status == ER_OK) && (pushed != len)) {
        len -= pushed;
        buf += pushed;
        status = sink.PushBytes(buf, len, pushed);
    }
    if (status == ER_OK) {
        QCC_DbgHLPrintf(("Deliver message %s to %s", Description().c_str(), endpoint->GetUniqueName().c_str()));
        QCC_DbgPrintf(("%s", ToString().c_str()));
    } else {
        QCC_LogError(status, ("Failed to deliver message %s", Description().c_str()));
    }
    return status;
}

QStatus _Message::DeliverNonBlocking(RemoteEndpoint& endpoint)
{
    size_t pushed;
    QStatus status = ER_OK;
    Sink& sink = endpoint->GetSink();

    switch (writeState) {
    case MESSAGE_NEW:
        writePtr = reinterpret_cast<uint8_t*>(msgBuf);
        countWrite = bufEOD - writePtr;
        pushed = 0;

        if (countWrite == 0) {
            status = ER_BUS_EMPTY_MESSAGE;
            QCC_LogError(status, ("Message is empty"));
            return status;
        }
        /*
         * Handles can only be passed if that feature was negotiated.
         */
        if (handles && !endpoint->GetFeatures().handlePassing) {
            status = ER_BUS_HANDLES_NOT_ENABLED;
            QCC_LogError(status, ("Handle passing was not negotiated on this connection"));
            return status;
        }
        /*
         * If the message has a TTL, check if it has expired
         */
        if (ttl && IsExpired()) {
            QCC_DbgHLPrintf(("TTL has expired - discarding message %s", Description().c_str()));
            return ER_OK;
        }
        /*
         * Check if message needs to be encrypted
         */
        if (encrypt) {
            status = EncryptMessage();
            /*
             * Delivery is retried when the authentication completes
             */
            if (status == ER_BUS_AUTHENTICATION_PENDING) {
                return ER_OK;
            }
        }
        writeState = MESSAGE_HEADERFIELDS;
    /* no break  FALLTHROUGH*/

    case MESSAGE_HEADERFIELDS:
        if (handles) {
            status = sink.PushBytesAndFds(writePtr, countWrite, pushed, handles, numHandles, endpoint->GetProcessId());
        } else {
            status = sink.PushBytes(writePtr, countWrite, pushed, (msgHeader.flags & ALLJOYN_FLAG_SESSIONLESS) ? (ttl * 1000) : ttl);
        }

        if (status == ER_OK) {
            countWrite -= pushed;
            writePtr += pushed;
            writeState = MESSAGE_HEADER_BODY;
        } else { break; }
    /* no break FALLTHROUGH*/

    case MESSAGE_HEADER_BODY:
        status = ER_OK;
        while (status == ER_OK && countWrite > 0) {
            status = sink.PushBytes(writePtr, countWrite, pushed);
            if (status == ER_OK) {
                countWrite -= pushed;
                writePtr += pushed;
            }
        }
        if (countWrite == 0) {
            writeState = MESSAGE_COMPLETE;
        }
        break;

    case MESSAGE_COMPLETE:
        status = ER_OK;
        break;

    }
    return status;
}
/*
 * Map from our enumeration type to the wire protocol values
 */
static const uint8_t FieldTypeMapping[] = {
    0,  /* ALLJOYN_HDR_FIELD_INVALID           */
    1,  /* ALLJOYN_HDR_FIELD_PATH              */
    2,  /* ALLJOYN_HDR_FIELD_INTERFACE         */
    3,  /* ALLJOYN_HDR_FIELD_MEMBER            */
    4,  /* ALLJOYN_HDR_FIELD_ERROR_NAME        */
    5,  /* ALLJOYN_HDR_FIELD_REPLY_SERIAL      */
    6,  /* ALLJOYN_HDR_FIELD_DESTINATION       */
    7,  /* ALLJOYN_HDR_FIELD_SENDER            */
    8,  /* ALLJOYN_HDR_FIELD_SIGNATURE         */
    9,  /* ALLJOYN_HDR_FIELD_HANDLES           */
    16, /* ALLJOYN_HDR_FIELD_TIMESTAMP         */
    17, /* ALLJOYN_HDR_FIELD_TIME_TO_LIVE      */
    18, /* ALLJOYN_HDR_FIELD_COMPRESSION_TOKEN */
    19  /* ALLJOYN_HDR_FIELD_SESSION_ID        */
};

/*
 * After the header fields are marshaled all of the strings in the MsgArgs point into the buffer.
 */
void _Message::MarshalHeaderFields()
{
    /*
     * Marshal the header fields
     */
    for (uint32_t fieldId = ALLJOYN_HDR_FIELD_PATH; fieldId < ArraySize(hdrFields.field); fieldId++) {
        MsgArg* field = &hdrFields.field[fieldId];
        if (field->typeId != ALLJOYN_INVALID) {
            if ((msgHeader.flags & ALLJOYN_FLAG_COMPRESSED) && HeaderFields::Compressible[fieldId]) {
                /*
                 * Stabilize the field to ensure that any strings etc, are copied into the message.
                 */
                field->Stabilize();
                continue;
            }
            /*
             * Header fields align on an 8 byte boundary
             */
            MarshalPad(8);
            Marshal1(FieldTypeMapping[fieldId]);
            /*
             * We relocate the string pointers in the fields to point to the marshaled versions to
             * so the lifetime of the message is not bound to the lifetime of values passed in.
             */
            const char* tPos;
            uint32_t tLen;
            AllJoynTypeId id = field->typeId;
            switch (id) {
            case ALLJOYN_SIGNATURE:
                Marshal1(1);
                Marshal1((uint8_t)ALLJOYN_SIGNATURE);
                Marshal1(0);
                Marshal1(field->v_signature.len);
                tPos = (char*)bufPos;
                tLen = field->v_signature.len;
                if ((void*)field->v_signature.sig) {
                    MarshalBytes((void*)field->v_signature.sig, field->v_signature.len + 1);
                }
                field->Clear();
                field->typeId = ALLJOYN_SIGNATURE;
                field->v_signature.sig = tPos;
                field->v_signature.len = tLen;
                break;

            case ALLJOYN_UINT32:
                Marshal1(1);
                Marshal1((uint8_t)ALLJOYN_UINT32);
                Marshal1(0);
                if (endianSwap) {
                    MarshalReversed(&field->v_uint32, 4);
                } else {
                    Marshal4(field->v_uint32);
                }
                break;

            case ALLJOYN_OBJECT_PATH:
            case ALLJOYN_STRING:
                Marshal1(1);
                Marshal1((uint8_t)id);
                Marshal1(0);
                if (endianSwap) {
                    MarshalReversed(&field->v_string.len, 4);
                } else {
                    Marshal4(field->v_string.len);
                }
                tPos = (char*)bufPos;
                tLen = field->v_string.len;
                if ((void*)field->v_string.str) {
                    MarshalBytes((void*)field->v_string.str, field->v_string.len + 1);
                }
                field->Clear();
                field->typeId = id;
                field->v_string.str = tPos;
                field->v_string.len = tLen;
                break;

            default:
                /*
                 * Use standard variant marshaling for the other cases.
                 */
                {
                    MsgArg variant(ALLJOYN_VARIANT);
                    variant.v_variant.val = field;
                    MarshalArgs(&variant, 1);
                    variant.v_variant.val = NULL;
                }
                break;
            }
        }
    }
    /*
     * Header must be zero-padded to end on an 8 byte boundary
     */
    MarshalPad(8);
}


/*
 * Calculate space required for the header fields
 */
size_t _Message::ComputeHeaderLen()
{
    size_t hdrLen = 0;
    for (uint32_t fieldId = ALLJOYN_HDR_FIELD_PATH; fieldId < ArraySize(hdrFields.field); fieldId++) {
        if ((msgHeader.flags & ALLJOYN_FLAG_COMPRESSED) && HeaderFields::Compressible[fieldId]) {
            continue;
        }
        MsgArg* field = &hdrFields.field[fieldId];
        if (field->typeId != ALLJOYN_INVALID) {
            hdrLen = ROUNDUP8(hdrLen) + SignatureUtils::GetSize(field, 1, 4);
        }
    }
    msgHeader.headerLen = static_cast<uint32_t>(hdrLen);
    return ROUNDUP8(sizeof(msgHeader) + hdrLen);
}

QStatus _Message::EncryptMessage()
{
    KeyBlob key;
    PeerState peerState = bus->GetInternal().GetPeerStateTable()->GetPeerState(GetDestination());
    QStatus status = peerState->GetKey(key, PEER_SESSION_KEY);

    if (status == ER_OK) {
        /*
         * Check we are authorized to send messages of this type to the remote peer.
         */
        if (!peerState->IsAuthorized((AllJoynMessageType)msgHeader.msgType, _PeerState::ALLOW_SECURE_TX)) {
            status = ER_BUS_NOT_AUTHORIZED;
            encrypt = false;
        }
    }
    if (status == ER_OK) {
        size_t argsLen = msgHeader.bodyLen - ajn::Crypto::MACLength;
        size_t hdrLen = ROUNDUP8(sizeof(msgHeader) + msgHeader.headerLen);
        status = ajn::Crypto::Encrypt(*this, key, (uint8_t*)msgBuf, hdrLen, argsLen);
        if (status == ER_OK) {
            QCC_DbgHLPrintf(("EncryptMessage: %s", Description().c_str()));
            /*
             * Save the authentication mechanism that was used.
             */
            authMechanism = key.GetTag();
            encrypt = false;
            assert(msgHeader.bodyLen == argsLen);
        }
    }
    /*
     * Need to request an authentication if we don't have a key.
     */
    if (status == ER_BUS_KEY_UNAVAILABLE) {
        QCC_DbgHLPrintf(("Deliver: No key - requesting authentication %s", Description().c_str()));
        Message msg = Message::wrap(this);
        status = bus->GetInternal().GetLocalEndpoint()->GetPeerObj()->RequestAuthentication(msg);
        if (status == ER_OK) {
            status = ER_BUS_AUTHENTICATION_PENDING;
        } else {
            encrypt = false;
        }
    }
    return status;
}

QStatus _Message::MarshalMessage(const qcc::String& expectedSignature,
                                 const qcc::String& destination,
                                 AllJoynMessageType msgType,
                                 const MsgArg* args,
                                 uint8_t numArgs,
                                 uint8_t flags,
                                 uint32_t sessionId)
{
    char signature[256];
    QStatus status = ER_OK;
    // if the MsgArg passed in is NULL force the numArgs to be zero.
    if (args == NULL) {
        numArgs = 0;
    }
    size_t argsLen = (numArgs == 0) ? 0 : SignatureUtils::GetSize(args, numArgs);
    size_t hdrLen = 0;

    if (!bus->IsStarted()) {
        return ER_BUS_BUS_NOT_STARTED;
    }
    /*
     * Check if endianess needs to be swapped.
     */
    endianSwap = outEndian != myEndian;
    /*
     * We marshal new messages in native endianess
     */
    encrypt = (flags & ALLJOYN_FLAG_ENCRYPTED) ? true : false;
    msgHeader.endian = outEndian;
    msgHeader.flags = flags;
    msgHeader.msgType = (uint8_t)msgType;
    msgHeader.majorVersion = ALLJOYN_MAJOR_PROTOCOL_VERSION;
    /*
     * Encryption will typically make the body length slightly larger because the encryption
     * algorithm appends a MAC block to the end of the encrypted data.
     */
    if (encrypt) {
        msgHeader.bodyLen = static_cast<uint32_t>(argsLen + ajn::Crypto::MACLength);
    } else {
        msgHeader.bodyLen = static_cast<uint32_t>(argsLen);
    }
    /*
     * Keep the old message buffer around until we are done because some of the strings we are
     * marshaling may point into the old message.
     */
    uint8_t* _oldMsgBuf = _msgBuf;
    /*
     * Clear out stale message data
     */
    bodyPtr = NULL;
    bufPos = NULL;
    bufEOD = NULL;
    msgBuf = NULL;
    _msgBuf = NULL;
    /*
     * There should be a mapping for every field type
     */
    assert(ArraySize(FieldTypeMapping) == ArraySize(hdrFields.field));
    /*
     * Set the serial number. This may be changed later if the message gets delayed.
     */
    SetSerialNumber();
    /*
     * Add the destination if there is one.
     */
    hdrFields.field[ALLJOYN_HDR_FIELD_DESTINATION].Clear();
    if (!destination.empty()) {
        hdrFields.field[ALLJOYN_HDR_FIELD_DESTINATION].typeId = ALLJOYN_STRING;
        hdrFields.field[ALLJOYN_HDR_FIELD_DESTINATION].v_string.str = destination.c_str();
        hdrFields.field[ALLJOYN_HDR_FIELD_DESTINATION].v_string.len = destination.size();
    }
    /*
     * Sender is obtained from the bus
     */
    const qcc::String& sender = bus->GetInternal().GetLocalEndpoint()->GetUniqueName();
    hdrFields.field[ALLJOYN_HDR_FIELD_SENDER].Clear();
    if (!sender.empty()) {
        hdrFields.field[ALLJOYN_HDR_FIELD_SENDER].typeId = ALLJOYN_STRING;
        hdrFields.field[ALLJOYN_HDR_FIELD_SENDER].v_string.str = sender.c_str();
        hdrFields.field[ALLJOYN_HDR_FIELD_SENDER].v_string.len = sender.size();
    }
    /*
     * If there are arguments build the signature
     */
    hdrFields.field[ALLJOYN_HDR_FIELD_SIGNATURE].Clear();
    if (numArgs > 0) {
        size_t sigLen = 0;
        status = SignatureUtils::MakeSignature(args, numArgs, signature, sigLen);
        if (status != ER_OK) {
            goto ExitMarshalMessage;
        }
        if (sigLen > 0) {
            hdrFields.field[ALLJOYN_HDR_FIELD_SIGNATURE].typeId = ALLJOYN_SIGNATURE;
            hdrFields.field[ALLJOYN_HDR_FIELD_SIGNATURE].v_signature.sig = signature;
            hdrFields.field[ALLJOYN_HDR_FIELD_SIGNATURE].v_signature.len = (uint8_t)sigLen;
        }
    } else {
        signature[0] = 0;
    }
    /*
     * Check the signature computed from the args matches the expected signature.
     */
    if (expectedSignature != signature) {
        status = ER_BUS_UNEXPECTED_SIGNATURE;
        QCC_LogError(status, ("MarshalMessage expected signature \"%s\" got \"%s\"", expectedSignature.c_str(), signature));
        goto ExitMarshalMessage;
    }
    /* Check if we are adding a session id */
    hdrFields.field[ALLJOYN_HDR_FIELD_SESSION_ID].Clear();
    if (sessionId != 0) {
        hdrFields.field[ALLJOYN_HDR_FIELD_SESSION_ID].v_uint32 = sessionId;
        hdrFields.field[ALLJOYN_HDR_FIELD_SESSION_ID].typeId = ALLJOYN_UINT32;
    }
    /*
     * Check if we are to do header compression. We must do this last after all the other fields
     * have been initialized.
     */
    hdrFields.field[ALLJOYN_HDR_FIELD_COMPRESSION_TOKEN].Clear();
    if ((msgHeader.flags & ALLJOYN_FLAG_COMPRESSED)) {
        hdrFields.field[ALLJOYN_HDR_FIELD_COMPRESSION_TOKEN].v_uint32 = bus->GetInternal().GetCompressionRules()->GetToken(hdrFields);
        hdrFields.field[ALLJOYN_HDR_FIELD_COMPRESSION_TOKEN].typeId = ALLJOYN_UINT32;
    }
    /*
     * Calculate space required for the header fields
     */
    hdrLen = ComputeHeaderLen();
    /*
     * Check that total packet size is within limits
     */
    if ((hdrLen + argsLen) > ALLJOYN_MAX_PACKET_LEN) {
        status = ER_BUS_BAD_BODY_LEN;
        QCC_LogError(status, ("Message size %d exceeds maximum size", hdrLen + argsLen));
        goto ExitMarshalMessage;
    }
    /*
     * Allocate buffer for entire message.
     */
    bufSize = (hdrLen + msgHeader.bodyLen + 7);
    _msgBuf = new uint8_t[bufSize + 7];
    msgBuf = (uint64_t*)((uintptr_t)(_msgBuf + 7) & ~7); /* Align to 8 byte boundary */
    /*
     * Initialize the buffer and copy in the message header
     */
    bufPos = (uint8_t*)msgBuf;
    /*
     * Toggle the autostart flag bit which is a 0 over the air but internally we prefer as a 1.
     */
    msgHeader.flags ^= ALLJOYN_FLAG_AUTO_START;
    memcpy(bufPos, &msgHeader, sizeof(msgHeader));
    msgHeader.flags ^= ALLJOYN_FLAG_AUTO_START;
    bufPos += sizeof(msgHeader);
    /*
     * Perform endian-swap on the buffer so the header member is in message endianess.
     */
    if (endianSwap) {
        MessageHeader* hdr = (MessageHeader*)msgBuf;
        hdr->bodyLen = EndianSwap32(hdr->bodyLen);
        hdr->serialNum = EndianSwap32(hdr->serialNum);
        hdr->headerLen = EndianSwap32(hdr->headerLen);
    }
    /*
     *
     */
    msgHeader.flags = flags;
    /*
     * Marshal the header fields
     */
    MarshalHeaderFields();
    assert((bufPos - (uint8_t*)msgBuf) == static_cast<ptrdiff_t>(hdrLen));
    if (msgHeader.bodyLen == 0) {
        bufEOD = bufPos;
        bodyPtr = NULL;
        goto ExitMarshalMessage;
    }
    /*
     * Marshal the message body
     */
    bodyPtr = bufPos;
    status = MarshalArgs(args, numArgs);
    if (status != ER_OK) {
        goto ExitMarshalMessage;
    }
    /*
     * If there handles to be marshalled we need to patch up the message header to add the
     * ALLJOYN_HDR_FIELD_HANDLES field. Since handles are rare it is more efficient to do a
     * re-marshal than to parse out handle occurences in every message.
     */
    if (handles) {
        hdrFields.field[ALLJOYN_HDR_FIELD_HANDLES].Set("u", numHandles);
        status = ReMarshal(NULL);
        if (status != ER_OK) {
            goto ExitMarshalMessage;
        }
    }
    /*
     * Assert that our two different body size computations agree
     */
    assert((bufPos - bodyPtr) == (ptrdiff_t)argsLen);
    bufEOD = bodyPtr + msgHeader.bodyLen;
    while (numArgs--) {
        QCC_DbgPrintf(("\n%s\n", args->ToString().c_str()));
        ++args;
    }

ExitMarshalMessage:

    /*
     * Don't need the old message buffer any more
     */
    delete [] _oldMsgBuf;

    if (status == ER_OK) {
        QCC_DbgHLPrintf(("MarshalMessage: %d+%d %s %s", hdrLen, msgHeader.bodyLen, Description().c_str(), encrypt ? " (encrypted)" : ""));
    } else {
        QCC_LogError(status, ("MarshalMessage: %s", Description().c_str()));
        msgBuf = NULL;
        delete [] _msgBuf;
        _msgBuf = NULL;
        bodyPtr = NULL;
        bufPos = NULL;
        bufEOD = NULL;
        ClearHeader();
    }
    return status;
}


QStatus _Message::HelloMessage(bool isBusToBus, bool allowRemote, SessionOpts::NameTransferType nameType)
{
    QStatus status;
    /*
     * Clear any stale header fields
     */
    ClearHeader();
    if (isBusToBus) {
        /* org.alljoyn.Bus.BusHello */
        hdrFields.field[ALLJOYN_HDR_FIELD_PATH].Set("o", org::alljoyn::Bus::ObjectPath);
        hdrFields.field[ALLJOYN_HDR_FIELD_INTERFACE].Set("s", org::alljoyn::Bus::InterfaceName);
        hdrFields.field[ALLJOYN_HDR_FIELD_MEMBER].Set("s", "BusHello");

        qcc::String guid = bus->GetInternal().GetGlobalGUID().ToString();
        MsgArg args[2];
        args[0].Set("s", guid.c_str());
        args[1].Set("u", nameType << 30 | ALLJOYN_PROTOCOL_VERSION);
        status = MarshalMessage("su",
                                org::alljoyn::Bus::WellKnownName,
                                MESSAGE_METHOD_CALL,
                                args,
                                ArraySize(args),
                                ALLJOYN_FLAG_AUTO_START | (allowRemote ? ALLJOYN_FLAG_ALLOW_REMOTE_MSG : 0),
                                0);
    } else {
        /* Standard org.freedesktop.DBus.Hello */
        hdrFields.field[ALLJOYN_HDR_FIELD_PATH].Set("o", org::freedesktop::DBus::ObjectPath);
        hdrFields.field[ALLJOYN_HDR_FIELD_INTERFACE].Set("s", org::freedesktop::DBus::InterfaceName);
        hdrFields.field[ALLJOYN_HDR_FIELD_MEMBER].Set("s", "Hello");

        status = MarshalMessage("",
                                org::freedesktop::DBus::WellKnownName,
                                MESSAGE_METHOD_CALL,
                                NULL,
                                0,
                                ALLJOYN_FLAG_AUTO_START | (allowRemote ? ALLJOYN_FLAG_ALLOW_REMOTE_MSG : 0),
                                0);

    }
    return status;
}


QStatus _Message::HelloReply(bool isBusToBus, const qcc::String& uniqueName, SessionOpts::NameTransferType nameType)
{
    QStatus status;
    qcc::String guidStr;

    assert(msgHeader.msgType == MESSAGE_METHOD_CALL);
    /*
     * Clear any stale header fields
     */
    ClearHeader();
    /*
     * Return serial number
     */
    hdrFields.field[ALLJOYN_HDR_FIELD_REPLY_SERIAL].Set("u", msgHeader.serialNum);

    if (isBusToBus) {
        guidStr = bus->GetInternal().GetGlobalGUID().ToString();
        MsgArg args[3];
        args[0].Set("s", uniqueName.c_str());
        args[1].Set("s", guidStr.c_str());
        args[2].Set("u", nameType << 30 | ALLJOYN_PROTOCOL_VERSION);
        status = MarshalMessage("ssu", uniqueName, MESSAGE_METHOD_RET, args, ArraySize(args), 0, 0);
        QCC_DbgPrintf(("\n%s", ToString(args, 2).c_str()));
    } else {
        /* Destination and argument are both the unique name passed in. */
        MsgArg arg("s", uniqueName.c_str());
        status = MarshalMessage("s", uniqueName, MESSAGE_METHOD_RET, &arg, 1, 0, 0);
        QCC_DbgPrintf(("\n%s", ToString(&arg, 1).c_str()));
    }
    return status;
}


QStatus _Message::CallMsg(const qcc::String& signature,
                          const qcc::String& destination,
                          SessionId sessionId,
                          const qcc::String& objPath,
                          const qcc::String& iface,
                          const qcc::String& methodName,
                          const MsgArg* args,
                          size_t numArgs,
                          uint8_t flags)
{
    QStatus status;

    /*
     * Validate flags
     */
    if (flags & ~(ALLJOYN_FLAG_NO_REPLY_EXPECTED | ALLJOYN_FLAG_AUTO_START | ALLJOYN_FLAG_ENCRYPTED | ALLJOYN_FLAG_COMPRESSED | ALLJOYN_FLAG_SESSIONLESS)) {
        return ER_BUS_BAD_HDR_FLAGS;
    }
    /*
     * Clear any stale header fields
     */
    ClearHeader();
    /*
     * Check object name
     */
    if (!IsLegalObjectPath(objPath.c_str())) {
        status = ER_BUS_BAD_OBJ_PATH;
        goto ExitCallMsg;
    }
    hdrFields.field[ALLJOYN_HDR_FIELD_PATH].Clear();
    hdrFields.field[ALLJOYN_HDR_FIELD_PATH].typeId = ALLJOYN_OBJECT_PATH;
    hdrFields.field[ALLJOYN_HDR_FIELD_PATH].v_objPath.str = objPath.c_str();
    hdrFields.field[ALLJOYN_HDR_FIELD_PATH].v_objPath.len = objPath.size();

    /*
     * Member name is required
     */
    hdrFields.field[ALLJOYN_HDR_FIELD_MEMBER].Clear();
    hdrFields.field[ALLJOYN_HDR_FIELD_MEMBER].typeId = ALLJOYN_STRING;
    hdrFields.field[ALLJOYN_HDR_FIELD_MEMBER].v_string.str = methodName.c_str();
    hdrFields.field[ALLJOYN_HDR_FIELD_MEMBER].v_string.len = methodName.size();
    /*
     * Interface name can be NULL
     */
    hdrFields.field[ALLJOYN_HDR_FIELD_INTERFACE].Clear();
    if (!iface.empty()) {
        hdrFields.field[ALLJOYN_HDR_FIELD_INTERFACE].typeId = ALLJOYN_STRING;
        hdrFields.field[ALLJOYN_HDR_FIELD_INTERFACE].v_string.str = iface.c_str();
        hdrFields.field[ALLJOYN_HDR_FIELD_INTERFACE].v_string.len = iface.size();
    }
    /*
     * Destination is required
     */
    if (destination.empty()) {
        status = ER_BUS_BAD_BUS_NAME;
        goto ExitCallMsg;
    }
    /*
     * Build method call message
     */
    status = MarshalMessage(signature, destination, MESSAGE_METHOD_CALL, args, numArgs, flags, sessionId);

ExitCallMsg:
    return status;
}


QStatus _Message::SignalMsg(const qcc::String& signature,
                            const char* destination,
                            SessionId sessionId,
                            const qcc::String& objPath,
                            const qcc::String& iface,
                            const qcc::String& signalName,
                            const MsgArg* args,
                            size_t numArgs,
                            uint8_t flags,
                            uint16_t timeToLive)
{
    QStatus status;

    /*
     * Validate flags - ENCRYPTED, COMPRESSED, ALLJOYN_FLAG_GLOBAL_BROADCAST and ALLJOYN_FLAG_SESSIONLESS are the flags applicable to signals
     */
    if (flags & ~(ALLJOYN_FLAG_ENCRYPTED | ALLJOYN_FLAG_COMPRESSED | ALLJOYN_FLAG_GLOBAL_BROADCAST | ALLJOYN_FLAG_SESSIONLESS)) {
        return ER_BUS_BAD_HDR_FLAGS;
    }
    /*
     * Clear any stale header fields
     */
    ClearHeader();
    /*
     * Check object name is legal
     */
    if (!IsLegalObjectPath(objPath.c_str())) {
        status = ER_BUS_BAD_OBJ_PATH;
        goto ExitSignalMsg;
    }

    /* NULL destination is allowed */
    if (!destination) {
        destination = "";
    }

    /*
     * If signal has a ttl timestamp the message and set the ttl and timestamp headers.
     */
    hdrFields.field[ALLJOYN_HDR_FIELD_TIME_TO_LIVE].Clear();
    hdrFields.field[ALLJOYN_HDR_FIELD_TIMESTAMP].Clear();
    if (timeToLive) {
        timestamp = GetTimestamp();
        ttl = timeToLive;
        hdrFields.field[ALLJOYN_HDR_FIELD_TIME_TO_LIVE].typeId = ALLJOYN_UINT16;
        hdrFields.field[ALLJOYN_HDR_FIELD_TIME_TO_LIVE].v_uint16 = ttl;
        hdrFields.field[ALLJOYN_HDR_FIELD_TIMESTAMP].typeId = ALLJOYN_UINT32;
        hdrFields.field[ALLJOYN_HDR_FIELD_TIMESTAMP].v_uint32 = timestamp;
    }

    hdrFields.field[ALLJOYN_HDR_FIELD_PATH].Clear();
    hdrFields.field[ALLJOYN_HDR_FIELD_PATH].typeId = ALLJOYN_OBJECT_PATH;
    hdrFields.field[ALLJOYN_HDR_FIELD_PATH].v_objPath.str = objPath.c_str();
    hdrFields.field[ALLJOYN_HDR_FIELD_PATH].v_objPath.len = objPath.size();

    hdrFields.field[ALLJOYN_HDR_FIELD_MEMBER].Clear();
    hdrFields.field[ALLJOYN_HDR_FIELD_MEMBER].typeId = ALLJOYN_STRING;
    hdrFields.field[ALLJOYN_HDR_FIELD_MEMBER].v_string.str = signalName.c_str();
    hdrFields.field[ALLJOYN_HDR_FIELD_MEMBER].v_string.len = signalName.size();

    hdrFields.field[ALLJOYN_HDR_FIELD_INTERFACE].Clear();
    hdrFields.field[ALLJOYN_HDR_FIELD_INTERFACE].typeId = ALLJOYN_STRING;
    hdrFields.field[ALLJOYN_HDR_FIELD_INTERFACE].v_string.str = iface.c_str();
    hdrFields.field[ALLJOYN_HDR_FIELD_INTERFACE].v_string.len = iface.size();

    /*
     * Build signal message
     */
    status = MarshalMessage(signature, destination, MESSAGE_SIGNAL, args, numArgs, flags, sessionId);

ExitSignalMsg:
    return status;
}


QStatus _Message::ReplyMsg(const Message& call, const MsgArg* args, size_t numArgs)
{
    QStatus status;
    SessionId sessionId = call->GetSessionId();

    /*
     * Destination is sender of method call
     */
    qcc::String destination = call->hdrFields.field[ALLJOYN_HDR_FIELD_SENDER].v_string.str;

    assert(call->msgHeader.msgType == MESSAGE_METHOD_CALL);

    /*
     * Clear any stale header fields
     */
    ClearHeader();
    /*
     * Return serial number from call
     */
    hdrFields.field[ALLJOYN_HDR_FIELD_REPLY_SERIAL].Clear();
    hdrFields.field[ALLJOYN_HDR_FIELD_REPLY_SERIAL].typeId = ALLJOYN_UINT32;
    hdrFields.field[ALLJOYN_HDR_FIELD_REPLY_SERIAL].v_uint32 = call->msgHeader.serialNum;
    /*
     * Build method return message (encrypted if the method call was encrypted)
     */
    status = MarshalMessage(call->replySignature, destination, MESSAGE_METHOD_RET, args,
                            numArgs, call->msgHeader.flags & ALLJOYN_FLAG_ENCRYPTED, sessionId);

    return status;
}


QStatus _Message::ErrorMsg(const Message& call, const char* errorName, const char* description)
{
    QStatus status;
    qcc::String destination = call->hdrFields.field[ALLJOYN_HDR_FIELD_SENDER].v_string.str;
    SessionId sessionId = call->GetSessionId();

    assert(call->msgHeader.msgType == MESSAGE_METHOD_CALL);

    /*
     * Clear any stale header fields
     */
    ClearHeader();
    /*
     * Error name is required
     */
    if ((errorName == NULL) || (*errorName == 0)) {
        status = ER_BUS_BAD_ERROR_NAME;
        goto ExitErrorMsg;
    }
    hdrFields.field[ALLJOYN_HDR_FIELD_ERROR_NAME].Set("s", errorName);
    /*
     * Set the reply serial number
     */
    hdrFields.field[ALLJOYN_HDR_FIELD_REPLY_SERIAL].Set("u", call->msgHeader.serialNum);
    /*
     * Build error message
     */
    if ('\0' == description[0]) {
        status = MarshalMessage("", destination, MESSAGE_ERROR, NULL, 0, call->msgHeader.flags & ALLJOYN_FLAG_ENCRYPTED, sessionId);
    } else {
        MsgArg arg("s", description);
        status = MarshalMessage("s", destination, MESSAGE_ERROR, &arg, 1, call->msgHeader.flags & ALLJOYN_FLAG_ENCRYPTED, sessionId);
    }

ExitErrorMsg:
    return status;
}


QStatus _Message::ErrorMsg(const Message& call, QStatus status)
{
    qcc::String destination = call->hdrFields.field[ALLJOYN_HDR_FIELD_SENDER].v_string.str;
    qcc::String msg = QCC_StatusText(status);
    uint16_t msgStatus = status;

    assert(call->msgHeader.msgType == MESSAGE_METHOD_CALL);
    /*
     * Clear any stale header fields
     */
    ClearHeader();
    /*
     * Error name is required
     */
    hdrFields.field[ALLJOYN_HDR_FIELD_ERROR_NAME].Set("s", org::alljoyn::Bus::ErrorName);
    /*
     * Set the reply serial number
     */
    hdrFields.field[ALLJOYN_HDR_FIELD_REPLY_SERIAL].Set("u", call->msgHeader.serialNum);
    /*
     * Build error message
     */
    MsgArg args[2];
    size_t numArgs = 2;
    MsgArg::Set(args, numArgs, "sq", msg.c_str(), msgStatus);
    return MarshalMessage("sq", destination, MESSAGE_ERROR, args, numArgs, call->msgHeader.flags & ALLJOYN_FLAG_ENCRYPTED, GetSessionId());
}


void _Message::ErrorMsg(const char* errorName,
                        uint32_t replySerial)
{
    /*
     * Clear any stale header fields
     */
    ClearHeader();
    /*
     * An error name is required so add one if necessary.
     */
    if ((errorName == NULL) || (*errorName == 0)) {
        errorName = "err.unknown";
    }
    hdrFields.field[ALLJOYN_HDR_FIELD_ERROR_NAME].Set("s", errorName);
    /*
     * Set the reply serial number
     */
    hdrFields.field[ALLJOYN_HDR_FIELD_REPLY_SERIAL].Set("u", replySerial);
    /*
     * Build error message
     */
    MarshalMessage("", "", MESSAGE_ERROR, NULL, 0, 0, 0);
}

void _Message::ErrorMsg(QStatus status,
                        uint32_t replySerial)
{
    qcc::String msg = QCC_StatusText(status);
    uint16_t msgStatus = status;

    /*
     * Clear any stale header fields
     */
    ClearHeader();

    hdrFields.field[ALLJOYN_HDR_FIELD_ERROR_NAME].Set("s", org::alljoyn::Bus::ErrorName);
    hdrFields.field[ALLJOYN_HDR_FIELD_REPLY_SERIAL].Set("u", replySerial);

    MsgArg args[2];
    size_t numArgs = 2;
    MsgArg::Set(args, numArgs, "sq", msg.c_str(), msgStatus);
    MarshalMessage("sq", "", MESSAGE_ERROR, args, numArgs, 0, 0);
}

QStatus _Message::GetExpansion(uint32_t token, MsgArg& replyArg)
{
    QStatus status = ER_OK;
    const HeaderFields* expFields = bus->GetInternal().GetCompressionRules()->GetExpansion(token);
    if (expFields) {
        MsgArg* hdrArray = new MsgArg[ALLJOYN_HDR_FIELD_UNKNOWN];
        size_t numElements = 0;
        /*
         * Reply arg is an array of structs with signature "(yv)"
         */
        for (uint32_t fieldId = ALLJOYN_HDR_FIELD_PATH; fieldId < ArraySize(expFields->field); fieldId++) {
            MsgArg* val = NULL;
            const MsgArg* exp = &expFields->field[fieldId];
            switch (exp->typeId) {
            case ALLJOYN_OBJECT_PATH:
                val = new MsgArg("o", exp->v_string.str);
                break;

            case ALLJOYN_STRING:
                val = new MsgArg("s", exp->v_string.str);
                break;

            case  ALLJOYN_SIGNATURE:
                val = new MsgArg("g", exp->v_signature.sig);
                break;

            case  ALLJOYN_UINT32:
                val = new MsgArg("u", exp->v_uint32);
                break;

            case  ALLJOYN_UINT16:
                val = new MsgArg("q", exp->v_uint16);
                break;

            default:
                break;
            }
            if (val) {
                uint8_t id = FieldTypeMapping[fieldId];
                hdrArray[numElements].Set("(yv)", id, val);
                hdrArray[numElements].SetOwnershipFlags(MsgArg::OwnsArgs);
                numElements++;
            }
        }
        replyArg.Set("a(yv)", numElements, hdrArray);
    } else {
        status = ER_BUS_CANNOT_EXPAND_MESSAGE;
        QCC_LogError(status, ("No expansion rule for token %u", token));
    }
    return status;
}

void _Message::SetSerialNumber()
{
    msgHeader.serialNum = bus->GetInternal().NextSerial();
    if (msgBuf) {
        ((MessageHeader*)msgBuf)->serialNum = endianSwap ? EndianSwap32(msgHeader.serialNum) : msgHeader.serialNum;
    }
}

}
