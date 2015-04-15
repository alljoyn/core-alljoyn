#ifndef _ALLJOYN_MESSAGE_H
#define _ALLJOYN_MESSAGE_H
/**
 * @file
 * This file defines a class for parsing and generating message bus messages
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

#ifndef __cplusplus
#error Only include Message.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/ManagedObj.h>

#include <alljoyn/MsgArg.h>
#include <alljoyn/Session.h>
#include <alljoyn/Status.h>

namespace ajn {

static const size_t ALLJOYN_MAX_NAME_LEN   =     255;  /*!<  The maximum length of certain bus names */
static const size_t ALLJOYN_MAX_ARRAY_LEN  =  131072;  /*!<  DBus limits array length to 2^26. AllJoyn limits it to 2^17 */
static const size_t ALLJOYN_MAX_PACKET_LEN =  (ALLJOYN_MAX_ARRAY_LEN + 4096);  /*!<  DBus limits packet length to 2^27. AllJoyn limits it further to 2^17 + 4096 to allow for 2^17 payload */

/** @name Endianness indicators */
// @{
/** indicates the bus is little endian */
static const uint8_t ALLJOYN_LITTLE_ENDIAN = 'l';
/** indicates the bus is big endian */
static const uint8_t ALLJOYN_BIG_ENDIAN    = 'B';
// @}


/** @name Flag types */
// @{
/** No reply is expected*/
static const uint8_t ALLJOYN_FLAG_NO_REPLY_EXPECTED          = 0x01;
/** Auto start the service */
static const uint8_t ALLJOYN_FLAG_AUTO_START                 = 0x02;
/** Allow messages from remote hosts (valid only in Hello message) */
static const uint8_t ALLJOYN_FLAG_ALLOW_REMOTE_MSG           = 0x04;
/** Sessionless message  */
static const uint8_t ALLJOYN_FLAG_SESSIONLESS                = 0x10;
/** Global (bus-to-bus) broadcast */
static const uint8_t ALLJOYN_FLAG_GLOBAL_BROADCAST           = 0x20;
/**
 * Header is compressed
 *
 * @deprecated March 2015 for 15.04 release
 */
QCC_DEPRECATED(static const uint8_t ALLJOYN_FLAG_COMPRESSED) = 0x40;
/** Body is encrypted */
static const uint8_t ALLJOYN_FLAG_ENCRYPTED                  = 0x80;
// @}

/** ALLJOYN protocol version */
static const uint8_t ALLJOYN_MAJOR_PROTOCOL_VERSION  = 1;

/** Message types */
typedef enum {
    MESSAGE_INVALID     = 0, ///< an invalid message type
    MESSAGE_METHOD_CALL = 1, ///< a method call message type
    MESSAGE_METHOD_RET  = 2, ///< a method return message type
    MESSAGE_ERROR       = 3, ///< an error message type
    MESSAGE_SIGNAL      = 4  ///< a signal message type
} AllJoynMessageType;



/** AllJoyn Header field types  */
typedef enum {

    /* Wire-protocol defined header field types */
    ALLJOYN_HDR_FIELD_INVALID = 0,              ///< an invalid header field type
    ALLJOYN_HDR_FIELD_PATH,                     ///< an object path header field type
    ALLJOYN_HDR_FIELD_INTERFACE,                ///< a message interface header field type
    ALLJOYN_HDR_FIELD_MEMBER,                   ///< a member (message/signal) name header field type
    ALLJOYN_HDR_FIELD_ERROR_NAME,               ///< an error name header field type
    ALLJOYN_HDR_FIELD_REPLY_SERIAL,             ///< a reply serial number header field type
    ALLJOYN_HDR_FIELD_DESTINATION,              ///< message destination header field type
    ALLJOYN_HDR_FIELD_SENDER,                   ///< senders well-known name header field type
    ALLJOYN_HDR_FIELD_SIGNATURE,                ///< message signature header field type
    ALLJOYN_HDR_FIELD_HANDLES,                  ///< number of file/socket handles that accompany the message
    /* AllJoyn defined header field types */
    ALLJOYN_HDR_FIELD_TIMESTAMP,                ///< time stamp header field type
    ALLJOYN_HDR_FIELD_TIME_TO_LIVE,             ///< messages time-to-live header field type
    ALLJOYN_HDR_FIELD_COMPRESSION_TOKEN,        ///< message compression token header field type @deprecated
    ALLJOYN_HDR_FIELD_SESSION_ID,               ///< Session id field type
    ALLJOYN_HDR_FIELD_UNKNOWN                   ///< unknown header field type also used as maximum number of header field types.
} AllJoynFieldType;

/** Message states */
typedef enum {
    MESSAGE_NEW,
    MESSAGE_HEADERFIELDS,
    MESSAGE_HEADER_BODY,
    MESSAGE_COMPLETE
}AllJoynMessageState;


/** AllJoyn header fields */
class HeaderFields {

  public:

    /**
     * The header field values.
     */
    MsgArg field[ALLJOYN_HDR_FIELD_UNKNOWN];

    /**
     * Table to identify which header fields can be compressed.
     *
     * @deprecated March 2015 for 15.04 release
     */
    static const bool Compressible[ALLJOYN_HDR_FIELD_UNKNOWN + 1];

    /**
     * Table to map the header field to a AllJoynTypeId
     */
    static const AllJoynTypeId FieldType[ALLJOYN_HDR_FIELD_UNKNOWN + 1];

    /**
     * Returns a string representation of the header fields.
     *
     * @param indent   Indentation level.
     *
     * @return  The string representation of the header fields.
     */
    qcc::String ToString(size_t indent =  0) const;

    /** Default constructor */
    HeaderFields() { }

    /** Copy constructor */
    HeaderFields(const HeaderFields& other);

    /** Assignment */
    HeaderFields& operator=(const HeaderFields& other);
};


/**
 * Forward definition
 */
class _Message;
class _RemoteEndpoint;
class BusAttachment;
class PeerStateTable;

/**
 * @cond ALLJOYN_DEV
 * @internal
 */
typedef qcc::ManagedObj<_RemoteEndpoint> RemoteEndpoint;
/// @endcond
/**
 * Message is a reference counted (managed) version of _Message
 */
typedef qcc::ManagedObj<_Message> Message;


/**
 * This class implements the functionality underlying the #Message class. Instances
 * of #_Message should not be declared directly by applications. Rather applications
 * create instances of the class #Message which handles reference counting for the
 * underlying #_Message instance. The members of #_Message are always accessed
 * indirectly via #Message.
 */
class _Message {

    friend class BusObject;
    friend class ProxyBusObject;
    friend class EndpointAuth;
    friend class _RemoteEndpoint;
    friend class _LocalEndpoint;
    friend class _NullEndpoint;
    friend class _UDPEndpoint;
    friend class UDPTransport;
    friend class DaemonRouter;
    friend class AllJoynObj;
    friend class DeferredMsg;
    friend class AllJoynPeerObj;
    friend class Crypto;
    friend struct Rule;

  public:
    /**
     * Constructor for a message
     *
     * @param bus  The bus that this message is sent or received on.
     */
    _Message(BusAttachment& bus);

    /**
     * Determine if message is a broadcast signal.
     *
     * @return  Return true if this is a broadcast signal.
     */
    bool IsBroadcastSignal() const {
        return (GetType() == MESSAGE_SIGNAL) && (hdrFields.field[ALLJOYN_HDR_FIELD_DESTINATION].typeId == ALLJOYN_INVALID);
    }

    /**
     * Messages broadcast to all devices are global broadcast messages.
     *
     * @return  Return true if this is a global broadcast message.
     */
    bool IsGlobalBroadcast() const { return IsBroadcastSignal() && (msgHeader.flags & ALLJOYN_FLAG_GLOBAL_BROADCAST); }

    /**
     * Determin if message is sessionless.
     *
     * @return   Return true if this message was sent sessionless
     */
    bool IsSessionless() const { return (msgHeader.flags & ALLJOYN_FLAG_SESSIONLESS) != 0; }

    /**
     * Returns the flags for the message.
     * @return flags for the message
     *
     * @see flag types in Message.h file
     */
    uint8_t GetFlags() const { return msgHeader.flags; }

    /**
     * Return true if message's TTL header indicates that it is expired
     *
     * @param[out] tillExpireMS  Written with number of milliseconds before message expires if non-null
     *                           If message never expires value is set to the maximum uint32_t value.
     *
     * @return Returns true if the message's TTL header indicates that is has expired.
     */
    bool IsExpired(uint32_t* tillExpireMS = NULL) const;

    /**
     * Determine if the message is marked as unreliable. Unreliable messages have a non-zero
     * time-to-live and may be silently discarded.
     *
     * @return  Returns true if the message is unreliable, that is, has a non-zero time-to-live.
     */
    bool IsUnreliable() const { return ttl != 0; }

    /**
     * Determine if the message was encrypted.
     *
     * @return  Returns true if the message was encrypted.
     */
    bool IsEncrypted() const { return (msgHeader.flags & ALLJOYN_FLAG_ENCRYPTED) != 0; }

    /**
     * Get the name of the authentication mechanism that was used to generate the encryption key if
     * the message is encrypted.
     *
     * @return  the name of an authentication mechanism or an empty string.
     */
    const qcc::String& GetAuthMechanism() const { return authMechanism; }

    /**
     * Return the type of the message
     *
     * @return  The type of message which can be one of MESSAGE_INVALID, MESSAGE_METHOD_CALL, MESSAGE_METHOD_RET,
     *          MESSAGE_ERROR, or MESSAGE_SIGNAL.
     */
    AllJoynMessageType GetType() const { return (AllJoynMessageType)msgHeader.msgType; }

    /**
     * Return the arguments for this message.
     *
     * @param[out] args  Returns the arguments
     * @param[out] numArgs The number of arguments
     */
    void GetArgs(size_t& numArgs, const MsgArg*& args) { args = msgArgs; numArgs = numMsgArgs; }

    /**
     * Return a specific argument.
     *
     * @param argN  The index of the argument to get.
     *
     * @return
     *      - The argument
     *      - NULL if unmarshal failed or there is not such argument.
     */
    const MsgArg* GetArg(size_t argN = 0) { return (argN < numMsgArgs) ? &msgArgs[argN] : NULL; }

    /**
     * Unpack and return the arguments for this message. This method uses the functionality from
     * MsgArg::Get() see MsgArg.h for documentation.
     *
     * @param signature  The signature to match against the message arguments.
     * @param ...        Pointers to return references to the unpacked values.
     * @return  ER_OK if successful.
     */
    QStatus GetArgs(const char* signature, ...);

    /**
     * Accessor function to get serial number for the message. Usually only important for
     * #MESSAGE_METHOD_CALL for matching up the reply to the call.
     *
     * @return the serial number of the %Message
     */
    uint32_t GetCallSerial() const { return msgHeader.serialNum; }

    /**
     * Get a reference to all of the header fields for this message.
     *
     * @return A const reference to the header fields for this message.
     */
    const HeaderFields& GetHeaderFields() const { return hdrFields; }

    /**
     * Accessor function to get the signature for this message
     * @return
     *      - The AllJoyn SIGNATURE string stored in the AllJoyn header field
     *      - An empty string if unable to find the AllJoyn signature
     */
    const char* GetSignature() const {
        if (hdrFields.field[ALLJOYN_HDR_FIELD_SIGNATURE].typeId == ALLJOYN_SIGNATURE &&
            hdrFields.field[ALLJOYN_HDR_FIELD_SIGNATURE].v_signature.sig) {
            return hdrFields.field[ALLJOYN_HDR_FIELD_SIGNATURE].v_signature.sig;
        } else {
            return "";
        }
    }

    /**
     * Accessor function to get the object path for this message
     *
     * @return
     *      - The AllJoyn object path string stored in the AllJoyn header field
     *      - An empty string if unable to find the AllJoyn object path
     */
    const char* GetObjectPath() const {
        if (hdrFields.field[ALLJOYN_HDR_FIELD_PATH].typeId == ALLJOYN_OBJECT_PATH &&
            hdrFields.field[ALLJOYN_HDR_FIELD_PATH].v_objPath.str) {
            return hdrFields.field[ALLJOYN_HDR_FIELD_PATH].v_objPath.str;
        } else {
            return "";
        }
    }

    /**
     * Accessor function to get the interface for this message
     *
     * @return
     *      - The AllJoyn interface string stored in the AllJoyn header field
     *      - An empty string if unable to find the interface
     */
    const char* GetInterface() const {
        if (hdrFields.field[ALLJOYN_HDR_FIELD_INTERFACE].typeId == ALLJOYN_STRING &&
            hdrFields.field[ALLJOYN_HDR_FIELD_INTERFACE].v_string.str) {
            return hdrFields.field[ALLJOYN_HDR_FIELD_INTERFACE].v_string.str;
        } else {
            return "";
        }
    }

    /**
     * Accessor function to get the member (method/signal) name for this message
     * @return
     *      - The AllJoyn member (method/signal) name string stored in the AllJoyn header field
     *      - An empty string if unable to find the member name
     */
    const char* GetMemberName() const {
        if (hdrFields.field[ALLJOYN_HDR_FIELD_MEMBER].typeId == ALLJOYN_STRING &&
            hdrFields.field[ALLJOYN_HDR_FIELD_MEMBER].v_string.str) {
            return hdrFields.field[ALLJOYN_HDR_FIELD_MEMBER].v_string.str;
        } else {
            return "";
        }
    }

    /**
     * Accessor function to get the reply serial number for the message. Only meaningful for #MESSAGE_METHOD_RET
     * @return
     *      - The serial number for the message stored in the AllJoyn header field
     *      - Zero if unable to find the serial number. Note that 0 is an invalid serial number.
     */
    uint32_t GetReplySerial() const {
        if (hdrFields.field[ALLJOYN_HDR_FIELD_REPLY_SERIAL].typeId == ALLJOYN_UINT32) {
            return hdrFields.field[ALLJOYN_HDR_FIELD_REPLY_SERIAL].v_uint32;
        } else {
            return 0;
        }
    }

    /**
     * Accessor function to get the sender for this message.
     *
     * @return
     *      - The senders well-known name string stored in the AllJoyn header field.
     *      - An empty string if the message did not specify a sender.
     */
    const char* GetSender() const {
        if (hdrFields.field[ALLJOYN_HDR_FIELD_SENDER].typeId == ALLJOYN_STRING &&
            hdrFields.field[ALLJOYN_HDR_FIELD_SENDER].v_string.str) {
            return hdrFields.field[ALLJOYN_HDR_FIELD_SENDER].v_string.str;
        } else {
            return "";
        }
    }

    /**
     * Get the unique name of the endpoint that the message was received on.
     *
     * @return
     *     - The unique name of the endpoint that the message was received on.
     */
    const char* GetRcvEndpointName() const {
        return rcvEndpointName.c_str();
    }

    /**
     * Accessor function to get the destination for this message
     * @return
     *      - The message destination string stored in the AllJoyn header field.
     *      - An empty string if unable to find the message destination.
     */
    const char* GetDestination() const {
        if (hdrFields.field[ALLJOYN_HDR_FIELD_DESTINATION].typeId == ALLJOYN_STRING &&
            hdrFields.field[ALLJOYN_HDR_FIELD_DESTINATION].v_string.str) {
            return hdrFields.field[ALLJOYN_HDR_FIELD_DESTINATION].v_string.str;
        } else {
            return "";
        }
    }

    /**
     * Accessor function to determine if a non empty destination has been set.
     *
     * @return
     *      - True if and only if a non empty destination has been set for this message.
     *
     */
    bool HasDestination() const {
        return ((hdrFields.field[ALLJOYN_HDR_FIELD_DESTINATION].typeId == ALLJOYN_STRING) &&
                (NULL != hdrFields.field[ALLJOYN_HDR_FIELD_DESTINATION].v_string.str) &&
                (0 != hdrFields.field[ALLJOYN_HDR_FIELD_DESTINATION].v_string.len));
    }

    /**
     * Accessor function to get the compression token for the message.
     *
     * @return
     *      - 0 'zero' if there is no compression token.
     *
     * @deprecated Header compression was deprecated in March 2015 for 15.04
     * release
     */
    uint32_t GetCompressionToken() const {
        return 0;
    }

    /**
     * Accessor function to get the session id for the message.
     *
     * @return
     *      - Session id for the message
     *      - 0 'zero' if sender did not specify a session
     */
    uint32_t GetSessionId() const {
        if (hdrFields.field[ALLJOYN_HDR_FIELD_SESSION_ID].typeId == ALLJOYN_UINT32) {
            return hdrFields.field[ALLJOYN_HDR_FIELD_SESSION_ID].v_uint32;
        } else {
            return 0;
        }
    }

    /**
     * If the message is an error message returns the error name and optionally the error message string
     * @param[out] errorMessage
     *                      - Return the error message string stored
     *                      - leave errorMessage unchanged if error message string not found
     * @return
     *      - If the message is an error message return the error name from the AllJoyn header
     *      - NULL if the message type is not MESSAGE_ERROR.
     */
    const char* GetErrorName(qcc::String* errorMessage = NULL) const;

    /**
     * Returns a complete description of an error by concatenating the error name and the error
     * message together.
     *
     * @return
     *      - If the message is an error message return the error description
     *      - An empty string if the message type is not MESSAGE_ERROR.
     */
    qcc::String GetErrorDescription() const;

    /**
     * Destructor for the %_Message class.
     */
    ~_Message();

    /**
     * In debug builds returns an XML string representation of the message. In release builds
     * returns an empty string.
     *
     * @return an XML string representation of the message or an empty string.
     */
    qcc::String ToString() const;

    /**
     * In debug builds returns a string that provides a brief description of the message. In release
     * builds returns and empty string.
     * @return a brief description of the message or an empty string.
     */
    qcc::String Description() const;

    /**
     * Returns the timestamp (in milliseconds) for this message. If the message header contained a
     * timestamp this is the estimated timestamp for when the message was sent by the remote device,
     * otherwise it is the timestamp for when the message was unmarshaled. Note that the timestamp
     * is always relative to local time.
     *
     * @return The timestamp for this message.
     */
    uint32_t GetTimeStamp() { return timestamp; };

    /**
     * Equality operator for messages. Messages are equivalent iff they are the same message.
     *
     * @param other  The other message to compare.
     *
     * @return  Returns true if this message is the same message as the other message.
     */
    bool operator==(const _Message& other) { return this == &other; }

    /**
     * Set the endianess for outgoing messages. This is mainly for testing purposes.
     *
     * @param endian  Either ALLJOYN_LITTLE_ENDIAN or ALLJOYN_BIG_ENDIAN. Any other value
     *                sets the endianess to the native endianess for this platform.
     *
     *
     */
    static void SetEndianess(const char endian) {
        if ((endian == ALLJOYN_LITTLE_ENDIAN) || (endian == ALLJOYN_BIG_ENDIAN)) {
            outEndian = endian;
        } else {
            outEndian = myEndian;
        }
    }

    /**
     * Get the Authentication Version of the message.
     *
     * @return
     *      - If the Authentication Version has been set, the value is returned.
     *      - Otherwise -1.
     */
    int32_t GetAuthVersion() const { return authVersion; }

    /**
     * Copy constructor.
     *
     * @param other   The other message to copy.
     */
    _Message(const _Message& other);

  protected:

    /*
     * These methods and members are protected rather than private to facilitate unit testing.
     */
    /// @cond ALLJOYN_DEV

    /**
     * Constructor for a message
     *
     * @param bus       The bus that this message is sent or received on.
     * @param hdrFields The header fields for this message.
     */
    _Message(BusAttachment& bus, const HeaderFields& hdrFields);

    /**
     * @internal
     * Generate a method reply message from a method call.
     *
     * @param call        The call message - can be this message.
     * @param args        The arguments for the reply (can be NULL)
     * @param numArgs     The number of arguments
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus ReplyMsg(const Message& call, const MsgArg* args, size_t numArgs);

    /**
     * @internal
     * Generate a method reply message from a method call.
     *
     * @param call        The call message - can be this message.
     * @param sender      The sender of the message
     * @param args        The arguments for the reply (can be NULL)
     * @param numArgs     The number of arguments
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus ReplyMsg(const Message& call, const qcc::String& sender, const MsgArg* args, size_t numArgs);

    /**
     * @internal
     * Generate an error message from a method call.
     *
     * @param call        The call message - can be this message.
     * @param errorName   The name of this error
     * @param description Informational string describing details of the error
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus ErrorMsg(const Message& call, const char* errorName, const char* description);

    /**
     * @internal
     * Generate an error message from a method call.
     *
     * @param call        The call message - can be this message.
     * @param sender      The sender of the message
     * @param errorName   The name of this error
     * @param description Informational string describing details of the error
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus ErrorMsg(const Message& call, const qcc::String& sender, const char* errorName, const char* description);

    /**
     * @internal
     * Generate an error message from a method call.
     *
     * @param call        The call message - can be this message.
     * @param status      The status code for this error
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus ErrorMsg(const Message& call, QStatus status);

    /**
     * @internal
     * Generate an error message from a method call.
     *
     * @param call        The call message - can be this message.
     * @param sender      The sender of the message
     * @param status      The status code for this error
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus ErrorMsg(const Message& call, const qcc::String& sender, QStatus status);

    /**
     * @internal
     * Compose a new internally generated error message.
     *
     * @param errorName   The name of this error
     * @param replySerial The serial number the method call this message is replying to.
     */
    void ErrorMsg(const char* errorName, uint32_t replySerial);

    /**
     * @internal
     * Compose a new internally generated error message.
     *
     * @param sender      The sender of this error message.
     * @param errorName   The name of this error
     * @param replySerial The serial number the method call this message is replying to.
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus ErrorMsg(const qcc::String& sender, const char* errorName, uint32_t replySerial);

    /**
     * @internal
     * Compose a new internally generated error message from a status code
     *
     * @param status      The status code for this error
     * @param replySerial The serial number the method call this message is replying to.
     */
    void ErrorMsg(QStatus status, uint32_t replySerial);

    /**
     * @internal
     * Compose a new internally generated error message from a status code
     *
     * @param sender      The sender of this error message.
     * @param status      The status code for this error
     * @param replySerial The serial number the method call this message is replying to.
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus ErrorMsg(const qcc::String& sender, QStatus status, uint32_t replySerial);

    /**
     * @internal
     * Compose a method call message
     *
     * @param signature   The signature (checked against the args)
     * @param destination The destination for this message
     * @param sessionId   The sessionId to use for this method call or 0 for any
     * @param objPath     The object the method call is being sent to
     * @param iface       The interface for the method (can be NULL)
     * @param methodName  The name of the method to call
     * @param args        The method call argument list (can be NULL)
     * @param numArgs     The number of arguments
     * @param flags       A logical OR of the AllJoyn flags
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus CallMsg(const qcc::String& signature,
                    const qcc::String& destination,
                    SessionId sessionId,
                    const qcc::String& objPath,
                    const qcc::String& iface,
                    const qcc::String& methodName,
                    const MsgArg* args,
                    size_t numArgs,
                    uint8_t flags);

    /**
     * @internal
     * Compose a method call message
     *
     * @param signature   The signature (checked against the args)
     * @param sender      sender of the message
     * @param destination The destination for this message
     * @param sessionId   The sessionId to use for this method call or 0 for any
     * @param objPath     The object the method call is being sent to
     * @param iface       The interface for the method (can be NULL)
     * @param methodName  The name of the method to call
     * @param args        The method call argument list (can be NULL)
     * @param numArgs     The number of arguments
     * @param flags       A logical OR of the AllJoyn flags
     *
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus CallMsg(const qcc::String& signature,
                    const qcc::String& sender,
                    const qcc::String& destination,
                    SessionId sessionId,
                    const qcc::String& objPath,
                    const qcc::String& iface,
                    const qcc::String& methodName,
                    const MsgArg* args,
                    size_t numArgs,
                    uint8_t flags);

    /**
     * @internal
     * Compose a signal message
     *
     * @param signature   The signature (checked against the args)
     * @param destination The destination for this message
     * @param sessionId   The sessionId to use for this signal msg or 0 for any
     * @param objPath     The object sending the signal
     * @param iface       The interface for the method (can be NULL)
     * @param signalName  The name of the signal being sent
     * @param args        The signal argument list (can be NULL)
     * @param numArgs     The number of arguments
     * @param flags       A logical OR of the AllJoyn flags.
     * @param timeToLive  Time-to-live. Units are seconds for sessionless signals. Milliseconds for non-sessionless signals.
     *                    Signals that cannot be sent within this time limit are discarded. Zero indicates reliable delivery.
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus SignalMsg(const qcc::String& signature,
                      const char* destination,
                      SessionId sessionId,
                      const qcc::String& objPath,
                      const qcc::String& iface,
                      const qcc::String& signalName,
                      const MsgArg* args,
                      size_t numArgs,
                      uint8_t flags,
                      uint16_t timeToLive);

    /**
     * @internal
     * Compose a signal message
     *
     * @param signature   The signature (checked against the args)
     * @param sender      sender of the message
     * @param destination The destination for this message
     * @param sessionId   The sessionId to use for this signal msg or 0 for any
     * @param objPath     The object sending the signal
     * @param iface       The interface for the method (can be NULL)
     * @param signalName  The name of the signal being sent
     * @param args        The signal argument list (can be NULL)
     * @param numArgs     The number of arguments
     * @param flags       A logical OR of the AllJoyn flags.
     * @param timeToLive  Time-to-live. Units are seconds for sessionless signals. Milliseconds for non-sessionless signals.
     *                    Signals that cannot be sent within this time limit are discarded. Zero indicates reliable delivery.
     *
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus SignalMsg(const qcc::String& signature,
                      const qcc::String& sender,
                      const char* destination,
                      SessionId sessionId,
                      const qcc::String& objPath,
                      const qcc::String& iface,
                      const qcc::String& signalName,
                      const MsgArg* args,
                      size_t numArgs,
                      uint8_t flags,
                      uint16_t timeToLive);

    /**
     * @internal
     * Unmarshal the message arguments.
     *
     * @param expectedSignature       The expected signature for this message.
     * @param expectedReplySignature  The expected reply signature for this message if it is a
     *                                method call message or NULL otherwise.
     *
     * @return
     *         - #ER_OK if the message was unmarshaled
     *         - Error status indicating why the unmarshal failed.
     */
    QStatus UnmarshalArgs(const qcc::String& expectedSignature,
                          const char* expectedReplySignature = NULL);

    /**
     * @internal
     * Unmarshal the message arguments.
     *
     * @param peerStateTable          The peer state table used when unmarshalling encrypted arguments.
     * @param expectedSignature       The expected signature for this message.
     * @param expectedReplySignature  The expected reply signature for this message if it is a
     *                                method call message or NULL otherwise.
     *
     * @return
     *         - #ER_OK if the message was unmarshaled
     *         - Error status indicating why the unmarshal failed.
     */
    QStatus UnmarshalArgs(PeerStateTable* peerStateTable, const qcc::String& expectedSignature, const char* expectedReplySignature = NULL);

    /**
     * @internal
     * Reads a message from a remote endpoint.
     *
     * @param endpoint       The endpoint to marshal the message data from.
     * @param checkSender    True if message's sender field should be validated against the endpoint's unique name.
     * @param pedantic       Perform detailed checks on the header fields.
     * @param timeout        If non-zero, a timeout in milliseconds to wait for a message to unmarshal.
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus Read(RemoteEndpoint& endpoint, bool checkSender, bool pedantic = true, uint32_t timeout = 0);

    /**
     * @internal
     * Reads a message from a remote endpoint. If data is not available it returns immediately
     *
     * @param endpoint       The endpoint to marshal the message data from.
     * @param checkSender    True if message's sender field should be validated against the endpoint's unique name.
     * @param pedantic       Perform detailed checks on the header fields.
     * @return
     *      - #ER_OK if successful i.e. message is complete
     *      - #ER_END_OF_DATA if message is incomplete
     *      - An error status otherwise
     */
    QStatus ReadNonBlocking(RemoteEndpoint& endpoint, bool checkSender, bool pedantic = true);

    /**
     * @internal
     * Unmarshals a message from a remote endpoint. Only the message header is unmarshaled at this
     * time.
     *
     * @param endpoint       The endpoint to marshal the message data from.
     * @param checkSender    True if message's sender field should be validated against the endpoint's unique name.
     * @param pedantic       Perform detailed checks on the header fields.
     * @param timeout        If non-zero, a timeout in milliseconds to wait for a message to unmarshal.
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus Unmarshal(RemoteEndpoint& endpoint, bool checkSender, bool pedantic = true, uint32_t timeout = 0);

    /**
     * @internal
     * Unmarshals a message. Only the message header is unmarshaled at this
     * time.
     *
     * @param endpointName   The uinique name of the endpoint that this message came from.
     * @param handlePassing  True if handle passing is allowed.
     * @param checkSender    True if message's sender field should be validated against the endpoint's unique name.
     * @param pedantic       Perform detailed checks on the header fields.
     * @param timeout        If non-zero, a timeout in milliseconds to wait for a message to unmarshal.
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus Unmarshal(qcc::String& endpointName, bool handlePassing, bool checkSender, bool pedantic = true, uint32_t timeout = 0);

    /**
     * @internal
     * Deliver a marshaled message to a remote endpoint.
     *
     * @param endpoint   Endpoint to receive marshaled message.
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus Deliver(RemoteEndpoint& endpoint);

    /**
     * @internal
     * Deliver a marshaled message to a remote endpoint. Non-blocking
     *
     * @param endpoint   Endpoint to receive marshaled message.
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus DeliverNonBlocking(RemoteEndpoint& endpoint);
    /**
     * @internal
     * Marshal the message again with the new sender name if one was provided.
     *
     * @param senderName  Option sender name to replace the current sender name in the message.
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus ReMarshal(const char* senderName = NULL);

    /**
     * @internal
     * Sets the serial number to the next available value for the bus attachment for this message.
     */
    void SetSerialNumber();

    /// @endcond

  private:

    /**
     * Common initialization called by constructor.  Calling constructors from other constructors is
     * not supported in some compilers.
     */
    void Init(BusAttachment& bus);

    /**
     * Message assignment is disallowed.
     */
    _Message operator=(const _Message& other);

    /**
     * Compose the special hello method call required to establish a connection
     *
     * @param isBusToBus   true iff connection attempt is between two AllJoyn instances (bus joining).
     * @param allowRemote  true iff connection allows messages from remote devices.
     * @param nametype     specify what names are transfered
     *
     * @return
     *      - #ER_OK if hello method call was sent successfully.
     *      - An error status otherwise
     */
    QStatus HelloMessage(bool isBusToBus, bool allowRemote, int nametype);

    /**
     * Compose the special hello method call required to establish a connection
     *
     * @param isBusToBus   true iff connection attempt is between two AllJoyn instances (bus joining).
     * @param sender       sender of the message
     * @param allowRemote  true iff connection allows messages from remote devices.
     * @param guid         GUID of sender of message
     * @param nametype     specify what names are transfered
     *
     * @return
     *      - #ER_OK if hello method call was sent successfully.
     *      - An error status otherwise
     */
    QStatus HelloMessage(bool isBusToBus, const qcc::String& sender, bool allowRemote,
                         const qcc::String& guid, int nameType);

    /**
     * Compose the reply to the hello method call
     *
     * @param isBusToBus  true iff connection attempt is between two AllJoyn instances (bus joining).
     * @param uniqueName  The new unique name for the sender.
     * @return
     *      - #ER_OK if reply to the hello method call was successful
     *      - An error status otherwise
     */
    QStatus HelloReply(bool isBusToBus, const qcc::String& uniqueName, int nametype);

    /**
     * Compose the reply to the hello method call
     *
     * @param isBusToBus  true iff connection attempt is between two AllJoyn instances (bus joining).
     * @param sender      sender of the message
     * @param uniqueName  The new unique name for the sender.
     * @param guid        GUID of sender of message
     * @return
     *      - #ER_OK if reply to the hello method call was successful
     *      - An error status otherwise
     */
    QStatus HelloReply(bool isBusToBus, const qcc::String& sender, const qcc::String& uniqueName, const qcc::String& guid, int nameType);

    /**
     * Get a pointer to the current backing buffer for the message.
     *
     * @return pointer to the message backing buffer
     */
    uint8_t const* const GetBuffer() const { return const_cast<uint8_t const* const>(reinterpret_cast<uint8_t*>(msgBuf));  }

    /**
     * Get the number of bytes of data currently in the message backing buffer.
     *
     * @return pointer to the message backing buffer
     */
    size_t GetBufferSize() const { return bufEOD - reinterpret_cast<uint8_t*>(msgBuf); }

    /**
     * Struct representing the header for the AllJoyn Message
     */
    typedef struct MessageHeader {
        char endian;           ///< The endianness of this message
        uint8_t msgType;       ///< Indicates if the message is method call, signal, etc.
        uint8_t flags;         ///< Flag bits
        uint8_t majorVersion;  ///< Major version of this message
        uint32_t bodyLen;      ///< Length of the body data
        uint32_t serialNum;    ///< serial of this message
        uint32_t headerLen;    ///< Length of the header fields
        MessageHeader() : endian(0), msgType(MESSAGE_INVALID), flags(0), majorVersion(0), bodyLen(0), serialNum(0), headerLen(0) { }
    } MessageHeader;


#if (QCC_TARGET_ENDIAN == QCC_LITTLE_ENDIAN)
    static const char myEndian = ALLJOYN_LITTLE_ENDIAN; ///< Native endianness of host system we are running on: little endian.
#else
    static const char myEndian = ALLJOYN_BIG_ENDIAN;    ///< Native endianness of host system we are running on: big endian.
#endif

    static char outEndian;       ///< Endianess for outgoing messages

    BusAttachment* bus;          ///< The bus this message was received or will be sent on.

    bool endianSwap;             ///< true if endianness will be swapped.

    MessageHeader msgHeader;     ///< Current message header.
    uint8_t* _msgBuf;            ///< Pointer to the current msg buffer.
    uint64_t* msgBuf;            ///< Pointer to the current msg buffer (8 byte aligned pointer into _msgBuf).
    MsgArg* msgArgs;             ///< Pointer to the unmarshaled arguments.
    uint8_t numMsgArgs;          ///< Number of message args (signature cannot be longer than 255 chars).

    size_t bufSize;              ///< The current allocated size of the msg buffer.
    uint8_t* bufEOD;             ///< End of data currently in buffer.
    uint8_t* bufPos;             ///< Pointer to the position in buffer.
    uint8_t* bodyPtr;            ///< Pointer to start of message body.

    uint16_t ttl;                ///< Time to live (units of seconds for sessionless. MS for everything else)
    uint32_t timestamp;          ///< Timestamp (local time) for messages with a ttl (time to live).

    qcc::String replySignature;  ///< Expected reply signature for a method call

    qcc::String authMechanism;   ///< For secure messages indicates the authentication mechanism that was used

    qcc::String rcvEndpointName; ///< Name of Endpoint that received this message.

    qcc::SocketFd* handles;      ///< Array of file/socket descriptors.
    size_t numHandles;           ///< Number of handles in the handles array
    bool encrypt;                ///< True if the message is to be encrypted
    int32_t authVersion;         ///< signed int representing the authVersion of this message.  -1 indicates not yet set.

    AllJoynMessageState readState;  ///< The current state of the message during read.
    size_t pktSize;                 ///< Packet size for this message.
    size_t countRead;               ///< Number of bytes remaining to read for completion of the message.
    size_t maxFds;                  ///< Store the number of max FDs for the endpoint, so it doesnt need to be calculated each time.

    AllJoynMessageState writeState; ///< The current state of the message during write.
    uint8_t* writePtr;              ///< Pointer to the current write position in the buffer.
    size_t countWrite;              ///< Number of bytes remaining to write for completion of the message.

    /**
     * The header fields for this message. Which header fields are present depends on the message
     * type defined in the message header.
     */
    HeaderFields hdrFields;

    /**
     * @defgroup internal_methods_message_unmarshal Internal methods unmarshal side
     *
     * Methods used to unmarshal and AllJoyn header
     *
     * @see Unmarshal
     * @see UnmarshalArgs
     *
     * @{
     */

    /**
     * Clear the header fields
     *
     * This also frees any data allocated to the header fields.
     */
    void ClearHeader();

    /**
     * Parse the MsgArg value from the AllJoyn Message
     *
     * @param[out] arg MsgArg that will hold the value from the AllJoyn Message
     * @param[in]  sigPtr the signature of the MsgArg
     * @param[in]  arrayElem true if the value being parsed is an array element
     *
     * @see Unmarshal
     * @see UnmarshalArgs
     *
     * @return
     *      - #ER_OK if successful
     *      - #ER_BUS_BAD_LENGTH if the length of a sting object is too long
     *      - #ER_BUS_NOT_NUL_TERMINATED if a string object is not nul terminated
     *      - #ER_BUS_BAD_SIGNATURE signature does not match value type
     *      - An error status otherwise
     */
    QStatus ParseValue(MsgArg* arg, const char*& sigPtr, bool arrayElem = false);

    /**
     * Parse a Struct from the AllJoyn Message
     *
     * @param[out] arg MsgArg that will hold the value from the AllJoyn Message
     * @param[in]  sigPtr the signature of the MsgArg
     *
     * @see Unmarshal
     * @see UnmarshalArgs
     *
     * @return
     *      - #ER_OK if successful
     *      - #ER_BUS_BAD_SIGNATURE signature does not match value type
     *      - An error status otherwise
     */
    QStatus ParseStruct(MsgArg* arg, const char*& sigPtr);

    /**
     * Parse a single dictionary entry from the AllJoyn Message
     *
     * @param[out] arg MsgArg that will hold the value from the AllJoyn Message
     * @param[in]  sigPtr the signature of the MsgArg
     *
     * @see Unmarshal
     * @see UnmarshalArgs
     *
     * @return
     *      - #ER_OK if successful
     *      - #ER_BUS_BAD_SIGNATURE signature does not match value type
     *      - An error status otherwise
     */
    QStatus ParseDictEntry(MsgArg* arg, const char*& sigPtr);

    /**
     * Parse an array from the AllJoyn Message
     *
     * @param[out] arg MsgArg that will hold the value from the AllJoyn Message
     * @param[in]  sigPtr the signature of the MsgArg
     *
     * @see Unmarshal
     * @see UnmarshalArgs
     *
     * @return
     *      - #ER_OK if successful
     *      - #ER_BUS_BAD_SIGNATURE signature does not match value type
     *      - An error status otherwise
     */
    QStatus ParseArray(MsgArg* arg, const char*& sigPtr);

    /**
     * Parse the MsgArg signature from the AllJoyn Message
     *
     * @param[out] arg assign the message arg signature to this MsgArg
     *
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus ParseSignature(MsgArg* arg);

    /**
     * Parse a variant MsgArg from an AllJoyn Message
     *
     * @param[out] arg assign the variant to this MsgArg
     *
     * @see Unmarshal
     * @see UnmarshalArgs
     *
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus ParseVariant(MsgArg* arg);

    /**
     * Check that the header fields are valid. This check is automatically performed when a header
     * is successfully unmarshaled.
     *
     * @param pedantic   Perform more detailed checks on the header fields.
     *
     * @return
     *      - #ER_OK if the header fields are valid
     *      - An error status otherwise
     */
    QStatus HeaderChecks(bool pedantic);
    /// @}
    // end internal_methods_message_unmarshal defgroup

    /**
     *  @defgroup internal_methods_message_marshal Internal methods marshal side
     *
     *  Methods used when marshaling the AllJoyn Message
     *
     *  @see Deliver
     *  @see DeliverNonBlocking
     *  @{
     */

    /**
     * If the Message should be encrypted this will encrypt the message
     *
     * If authentication has not been done this will request authentications
     *
     * @return
     *    - #ER_OK if the header fields are valid
     *    - #ER_BUS_NOT_AUTHORIZED not authorized to send encrypted messages
     *    - #ER_BUS_AUTHENTICATION_PENDING authentication is in progress must
     *                                     retry once authentication is complete
     *    - An error status otherwise
     *
     */
    QStatus EncryptMessage();

    /**
     * Marshal (serialize) the Message so it is in the wire format
     *
     * @param signature   message signature
     * @param sender      sender of the message
     * @param destination destination of the message
     * @param msgType     what type of message this is
     * @param args        pointer to an array of MsgArgs contained in the message
     * @param numArgs     number of MsgArg
     * @param flags       A logical OR of the AllJoyn flags
     * @param sessionId   The session id that the Message will be sent to
     *
     *  @return
     *    - #ER_OK if successful
     *    - An error status otherwise
     */
    QStatus MarshalMessage(const qcc::String& signature,
                           const qcc::String& sender,
                           const qcc::String& destination,
                           AllJoynMessageType msgType,
                           const MsgArg* args,
                           uint8_t numArgs,
                           uint8_t flags,
                           SessionId sessionId);

    /**
     * Marshal the MsgArg arguments into the message
     *
     * This will Marshal an array of MsgArgs into the Message adding any padding
     * and formating needed for the Message wire format.
     *
     * @param[in] arg pointer to an array of MsgArgs to be marshaled
     * @param[in] numArgs the number of MsgArgs in arg
     *
     * @return
     *    - #ER_OK if successful
     *    - An error status otherwise
     */
    QStatus MarshalArgs(const MsgArg* arg, size_t numArgs);
    /**
     * Marshal the header fields
     *
     * @note We relocate the string pointers in the fields to point to the
     *       marshaled versions to so the lifetime of the message is not bound
     *       to the lifetime of values passed in.
     */
    void MarshalHeaderFields();
    /**
     * Calculate space required for the header fields
     *
     * @return the space required for the header fields
     */
    size_t ComputeHeaderLen();
    /// @}
    // end internal_methods_message_marshal defgroup

    /**
     * Get string representation of the message
     *
     * In debug builds return a string representation of the message
     * An empty string is returned otherwise.
     *
     * @see ToString()
     *
     * @param[in] args the MsgArgs contained in the message
     * @param[in] numArgs the number of MsgArgs contained in the message
     *
     * @return string representation of the message
     */
    qcc::String ToString(const MsgArg* args, size_t numArgs) const;

    /* @defgroup internal_methods_message_read "Internal methods for read"
     * @{
     */
    /**
     * Read a header and check it is a valid header
     * check for things like endianness, header length, and body length
     *
     * If the endianness is incorrect this will perform an endian swap.
     *
     * @return
     *    - #ER_OK if successful
     *    - #ER_BUS_BAD_HEADER_FIELD if the Message header has invalid endian flag
     *    - #ER_BUS_BAD_HEADER_LEN if the Message header length is invalid
     *    - #ER_BUS_BAD_BODY_LEN if the Message body length is invalid
     */
    inline QStatus InterpretHeader();

    /**
     * Read a Message from a RemoteEndpoint
     *
     * @param endpoint      The endpoint the message will be read from
     * @param checkSender   True if message's sender field should be validated
     *                      against the endpoint's unique name.
     * @param pedantic      Perform detailed checks on the header fields.
     * @param timeout       If non-zero, a timeout in milliseconds to wait for
     *                      the bytes to be pulled.
     *
     * @see Read
     * @see ReadNonBlocking
     *
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus PullBytes(RemoteEndpoint& endpoint, bool checkSender, bool pedantic = true, uint32_t timeout = 0);

    /**
     * Load a Message from a Buffer
     *
     * @param buf     The buffer to read the message from
     * @param buflen  The length of the message data in the buffer.
     *
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus LoadBytes(uint8_t* buf, size_t buflen);

    /// @}
    // end internal_methods_message_read

    /*
     * This is the authentication version that we will fall back to,
     * if we are unable to determine the auth version of the destination
     * of the message (in the case of broadcast and multicast.)
     */
    static const uint32_t AUTH_FALLBACK_VERSION;


};

}

#endif
