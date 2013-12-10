/******************************************************************************
 *
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
 *
 *****************************************************************************/

#pragma once

#include <Status_CPP0x.h>
#include <alljoyn/Message.h>
#include <qcc/ManagedObj.h>
#include <MsgArg.h>

namespace AllJoyn {

ref class MessageHeaderFields;

public enum class AllJoynMessageType {
    /// <summary>an invalid message type</summary>
    MESSAGE_INVALID     = ajn::AllJoynMessageType::MESSAGE_INVALID,
    /// <summary>a method call message type</summary>
    MESSAGE_METHOD_CALL = ajn::AllJoynMessageType::MESSAGE_METHOD_CALL,
    /// <summary>a method return message type</summary>
    MESSAGE_METHOD_RET  = ajn::AllJoynMessageType::MESSAGE_METHOD_RET,
    /// <summary>an error message type</summary>
    MESSAGE_ERROR       = ajn::AllJoynMessageType::MESSAGE_ERROR,
    /// <summary>a signal message type</summary>
    MESSAGE_SIGNAL      = ajn::AllJoynMessageType::MESSAGE_SIGNAL
};

[Platform::Metadata::Flags]
public enum class AllJoynFlagType : uint32_t {
    /// <summary>No reply is expected</summary>
    ALLJOYN_FLAG_NO_REPLY_EXPECTED  = ajn::ALLJOYN_FLAG_NO_REPLY_EXPECTED,
    /// <summary>Auto start the service</summary>
    ALLJOYN_FLAG_AUTO_START         = ajn::ALLJOYN_FLAG_AUTO_START,
    /// <summary>Allow messages from remote hosts (valid only in Hello message)</summary>
    ALLJOYN_FLAG_ALLOW_REMOTE_MSG   = ajn::ALLJOYN_FLAG_ALLOW_REMOTE_MSG,
    /// <summary>Sessionless message</summary>
    ALLJOYN_FLAG_SESSIONLESS        = ajn::ALLJOYN_FLAG_SESSIONLESS,
    /// <summary>Global (bus-to-bus) broadcast</summary>
    ALLJOYN_FLAG_GLOBAL_BROADCAST   = ajn::ALLJOYN_FLAG_GLOBAL_BROADCAST,
    /// <summary>Header is compressed</summary>
    ALLJOYN_FLAG_COMPRESSED         = ajn::ALLJOYN_FLAG_COMPRESSED,
    /// <summary>Body is encrypted</summary>
    ALLJOYN_FLAG_ENCRYPTED          = ajn::ALLJOYN_FLAG_ENCRYPTED
};

ref class __Message {
  private:
    friend ref class Message;
    friend class _Message;
    __Message();
    ~__Message();

    property Platform::String ^ AuthMechanism;
    property AllJoynMessageType Type;
    property uint8_t Flags;
    property uint32_t CallSerial;
    property MessageHeaderFields ^ HeaderFields;
    property Platform::String ^ Signature;
    property Platform::String ^ ObjectPath;
    property Platform::String ^ Interface;
    property Platform::String ^ MemberName;
    property uint32_t ReplySerial;
    property Platform::String ^ Sender;
    property Platform::String ^ RcvEndpointName;
    property Platform::String ^ Destination;
    property uint32_t CompressionToken;
    property uint32_t SessionId;
    property Platform::String ^ Description;
    property uint32_t Timestamp;
};

class _Message {
  protected:
    friend class qcc::ManagedObj<_Message>;
    friend ref class Message;
    friend class _AuthListener;
    friend ref class BusObject;
    friend class _BusObject;
    _Message(const ajn::Message* msg);
    ~_Message();

    operator ajn::Message * ();
    operator ajn::_Message * ();

    __Message ^ _eventsAndProperties;
    ajn::Message* _mMessage;
    const ajn::_Message* _message;
};

/// <summary>
///This class implements an AllJoyn message
/// </summary>
public ref class Message sealed {
  public:
    /// <summary>
    ///Determine if message is a broadcast signal.
    /// </summary>
    /// <returns>
    ///Return true if this is a broadcast signal.
    /// </returns>
    bool IsBroadcastSignal();
    /// <summary>
    ///Messages broadcast to all devices are global broadcast messages.
    /// </summary>
    /// <returns>
    ///   Return true if this is a global broadcast message.
    /// </returns>
    bool IsGlobalBroadcast();
    /// <summary>
    ///Determine if message is a sessionless signal.
    /// </summary>
    /// <returns>
    ///Return true if this is a sessionless signal.
    /// </returns>
    bool IsSessionless();

    /// <summary>
    ///Return true if message's TTL header indicates that it is expired
    /// </summary>
    /// <param name="tillExpireMS">
    ///Written with number of milliseconds before message expires if non-null
    ///If message never expires value is set to the maximum uint32_t value.
    /// </param>
    /// <returns>
    ///Returns true if the message's TTL header indicates that is has expired.
    /// </returns>
    bool IsExpired(Platform::WriteOnlyArray<uint32_t> ^ tillExpireMS);

    /// <summary>
    ///Determine if the message is marked as unreliable. Unreliable messages have a non-zero
    ///time-to-live and may be silently discarded.
    /// </summary>
    /// <returns>
    ///Returns true if the message is unreliable, that is, has a non-zero time-to-live.
    /// </returns>
    bool IsUnreliable();

    /// <summary>
    ///Determine if the message was encrypted
    /// </summary>
    /// <returns>
    ///Returns true if the message was encrypted.
    /// </returns>
    bool IsEncrypted();

    /// <summary>
    ///Return the arguments for this message.
    /// </summary>
    /// <param name="args">Returns the arguments</param>
    uint32_t GetArgs(Platform::WriteOnlyArray<MsgArg ^> ^ args);

    /// <summary>
    ///Return a specific argument for this message.
    /// </summary>
    /// <param name="argN">The index of the argument to get</param>
    /// <returns>
    ///- The argument
    ///- NULL if unmarshal failed or there is not such argument.
    /// </returns>
    MsgArg ^ GetArg(uint32_t argN);

    /// <summary>
    ///Get a string representation of the message
    /// </summary>
    /// <returns>string representation of the message</returns>
    Platform::String ^ ConvertToString();

    /// <summary>
    ///If the message is an error message returns the error name and optionally the error message string
    /// </summary>
    /// <param name="errorMessage">
    /// - Return the error message string stored
    /// - leave errorMessage unchanged if error message string not found
    /// </param>
    /// <returns>
    /// - If the message is an error message return the error name from the AllJoyn header
    /// - NULL if the message type is not MESSAGE_ERROR.
    /// </returns>
    Platform::String ^ GetErrorName(Platform::String ^ errorMessage);

    /// <summary>
    ///Get the name of the authentication mechanism that was used to generate the encryption key if
    ///the message is encrypted.
    /// </summary>
    /// <returns>The name of an authentication mechanism or an empty string.</returns>
    property Platform::String ^ AuthMechanism
    {
        Platform::String ^ get();
    }

    /// <summary>
    ///Return the type of the <c>Message</c>
    /// </summary>
    property AllJoynMessageType Type
    {
        AllJoynMessageType get();
    }

    /// <summary>Returns the flags for the message.</summary>
    /// <returns>flags for the message</returns>
    property uint8_t Flags
    {
        uint8_t get();
    }

    /// <summary>
    ///Get serial number for the message. Usually only important for
    /// #MESSAGE_METHOD_CALL for matching up the reply to the call.
    /// </summary>
    /// <returns>
    ///the serial number of the <c>Message</c>
    /// </returns>
    property uint32_t CallSerial
    {
        uint32_t get();
    }

    /// <summary>Get a reference to all of the header fields for this message.</summary>
    /// <returns>A const reference to the header fields for this message.</returns>
    property MessageHeaderFields ^ HeaderFields
    {
        MessageHeaderFields ^ get();
    }

    /// <summary>
    ///Accessor function to get the signature for this message
    /// </summary>
    /// <returns>
    ///- The AllJoyn SIGNATURE string stored in the AllJoyn header field
    /// - An empty string if unable to find the AllJoyn signature
    /// </returns>
    property Platform::String ^ Signature
    {
        Platform::String ^ get();
    }

    /// <summary>
    ///Accessor function to get the object path for this message
    /// </summary>
    /// <returns>
    /// - The AllJoyn object path string stored in the AllJoyn header field
    /// - An empty string if unable to find the AllJoyn object path
    /// </returns>
    property Platform::String ^ ObjectPath
    {
        Platform::String ^ get();
    }

    /// <summary>
    ///Accessor function to get the interface for this message
    /// </summary>
    /// <returns>
    /// - The AllJoyn interface string stored in the AllJoyn header field
    /// - An empty string if unable to find the interface
    /// </returns>
    property Platform::String ^ Interface
    {
        Platform::String ^ get();
    }

    /// <summary>
    ///Accessor function to get the member (method/signal) name for this message
    /// </summary>
    /// <returns>
    /// - The AllJoyn member (method/signal) name string stored in the AllJoyn header field
    /// - An empty string if unable to find the member name
    /// </returns>
    property Platform::String ^ MemberName
    {
        Platform::String ^ get();
    }

    /// <summary>
    ///Accessor function to get the reply serial number for the message. Only meaningful for #MESSAGE_METHOD_RET
    /// </summary>
    /// <returns>
    /// - The serial number for the message stored in the AllJoyn header field
    /// - Zero if unable to find the serial number. Note that 0 is an invalid serial number.
    /// </returns>
    property uint32_t ReplySerial
    {
        uint32_t get();
    }

    /// <summary>
    ///Accessor function to get the sender for this message.
    /// </summary>
    /// <returns>
    /// - The senders well-known name string stored in the AllJoyn header field.
    /// - An empty string if the message did not specify a sender.
    /// </returns>
    property Platform::String ^ Sender
    {
        Platform::String ^ get();
    }

    /// <summary>
    ///Get the unique name of the endpoint that the message was received on.
    /// </summary>
    /// <returns>The unique name of the endpoint that the message was received on.</returns>
    property Platform::String ^ RcvEndpointName
    {
        Platform::String ^ get();
    }

    /// <summary>
    ///Accessor function to get the destination for this message
    /// </summary>
    /// <returns>
    /// - The message destination string stored in the AllJoyn header field.
    /// - An empty string if unable to find the message destination.
    /// </returns>
    property Platform::String ^ Destination
    {
        Platform::String ^ get();
    }

    /// <summary>
    ///Accessor function to get the compression token for the message.
    /// </summary>
    /// <returns>
    /// - Compression token for the message stored in the AllJoyn header field
    /// - 0 'zero' if there is no compression token.
    /// </returns>
    property uint32_t CompressionToken
    {
        uint32_t get();
    }

    /// <summary>
    ///Accessor function to get the session id for the message.
    /// </summary>
    /// <returns>
    /// - Session id for the message
    /// - 0 'zero' if sender did not specify a session
    /// </returns>
    property uint32_t SessionId
    {
        uint32_t get();
    }

    /// <summary>
    /// Returns a complete description of an error by concatenating the error name and the error
    /// message together.
    /// </summary>
    /// <returns>
    /// - If the message is an error message return the error description
    /// - An empty string if the message type is not MESSAGE_ERROR.
    /// </returns>
    property Platform::String ^ Description
    {
        Platform::String ^ get();
    }

    /// <summary>
    ///Returns the timestamp (in milliseconds) for this message. If the message header contained a
    ///timestamp this is the estimated timestamp for when the message was sent by the remote device,
    ///otherwise it is the timestamp for when the message was unmarshaled. Note that the timestamp
    ///is always relative to local time.
    /// </summary>
    /// <returns>
    ///The timestamp for this message.
    /// </returns>
    property uint32_t Timestamp
    {
        uint32_t get();
    }

  private:
    friend class _AuthListener;
    friend ref class BusObject;
    friend class _BusObject;
    friend class _MessageReceiver;
    friend class _ProxyBusObject;
    Message(const ajn::Message * message);
    ~Message();

    qcc::ManagedObj<_Message>* _mMessage;
    _Message* _message;
};

}
// Message.h
