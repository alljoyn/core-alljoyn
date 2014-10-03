/**
 * @file
 *
 * This file implements the _Message class
 */

/******************************************************************************
 * Copyright (c) 2009-2014 AllSeen Alliance. All rights reserved.
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

#include <assert.h>
#include <ctype.h>

#include <alljoyn/Message.h>
#include "BusAttachmentC.h"
#include "MsgArgC.h"
#include <alljoyn_c/Message.h>
#include <alljoyn_c/BusAttachment.h>
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

struct _alljoyn_message_handle {
    _alljoyn_message_handle(ajn::BusAttachmentC& bus) : msg(bus) { }
    ajn::Message msg;
};

alljoyn_message AJ_CALL alljoyn_message_create(alljoyn_busattachment bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return new struct _alljoyn_message_handle (*((ajn::BusAttachmentC*)bus));
}

void AJ_CALL alljoyn_message_destroy(alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    delete msg;
}

QCC_BOOL AJ_CALL alljoyn_message_isbroadcastsignal(alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return msg->msg->IsBroadcastSignal();
}

QCC_BOOL AJ_CALL alljoyn_message_isglobalbroadcast(alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return msg->msg->IsGlobalBroadcast();
}

QCC_BOOL AJ_CALL alljoyn_message_issessionless(alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return msg->msg->IsSessionless();
}

uint8_t AJ_CALL alljoyn_message_getflags(alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return msg->msg->GetFlags();
}

QCC_BOOL AJ_CALL alljoyn_message_isexpired(alljoyn_message msg, uint32_t* tillExpireMS)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return msg->msg->IsExpired(tillExpireMS);
}

QCC_BOOL AJ_CALL alljoyn_message_isunreliable(alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return msg->msg->IsUnreliable();
}

QCC_BOOL AJ_CALL alljoyn_message_isencrypted(alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return msg->msg->IsEncrypted();
}

const char* AJ_CALL alljoyn_message_getauthmechanism(alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return msg->msg->GetAuthMechanism().c_str();
}

alljoyn_messagetype AJ_CALL alljoyn_message_gettype(alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_messagetype)msg->msg->GetType();
}

void AJ_CALL alljoyn_message_getargs(alljoyn_message msg, size_t* numArgs, alljoyn_msgarg* args)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    const ajn::MsgArg* tmpArgs;
    msg->msg->GetArgs(*numArgs, tmpArgs);
    *args = (alljoyn_msgarg)tmpArgs;
}

const alljoyn_msgarg AJ_CALL alljoyn_message_getarg(alljoyn_message msg, size_t argN)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_msgarg)msg->msg->GetArg(argN);
}

QStatus AJ_CALL alljoyn_message_parseargs(alljoyn_message msg, const char* signature, ...)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    size_t sigLen = (signature ? strlen(signature) : 0);
    if (sigLen == 0) {
        return ER_BAD_ARG_2;
    }
    const ajn::MsgArg* msgArgs;
    size_t numMsgArgs;

    msg->msg->GetArgs(numMsgArgs, msgArgs);

    va_list argp;
    va_start(argp, signature);
    QStatus status = ajn::MsgArgC::VParseArgsC(signature, sigLen, msgArgs, numMsgArgs, &argp);
    va_end(argp);
    return status;
}

uint32_t AJ_CALL alljoyn_message_getcallserial(alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return msg->msg->GetCallSerial();
}

const char* AJ_CALL alljoyn_message_getsignature(alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return msg->msg->GetSignature();
}

const char* AJ_CALL alljoyn_message_getobjectpath(alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return msg->msg->GetObjectPath();
}

const char* AJ_CALL alljoyn_message_getinterface(alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return msg->msg->GetInterface();
}

const char* AJ_CALL alljoyn_message_getmembername(alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return msg->msg->GetMemberName();
}

uint32_t AJ_CALL alljoyn_message_getreplyserial(alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return msg->msg->GetReplySerial();
}

const char* AJ_CALL alljoyn_message_getsender(alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return msg->msg->GetSender();
}

const char* AJ_CALL alljoyn_message_getreceiveendpointname(alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return msg->msg->GetRcvEndpointName();
}

const char* AJ_CALL alljoyn_message_getdestination(alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return msg->msg->GetDestination();
}

uint32_t AJ_CALL alljoyn_message_getcompressiontoken(alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return msg->msg->GetCompressionToken();
}

alljoyn_sessionid AJ_CALL alljoyn_message_getsessionid(alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_sessionid)msg->msg->GetSessionId();
}

const char* AJ_CALL alljoyn_message_geterrorname(alljoyn_message msg, char* errorMessage, size_t* errorMessage_size)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String* str = new qcc::String("");
    const char* ret = msg->msg->GetErrorName(str);

    if (errorMessage != NULL && errorMessage_size > 0) {
        strncpy(errorMessage, str->c_str(), *errorMessage_size);
        //Make sure the string is always nul terminated.
        errorMessage[*errorMessage_size - 1] = '\0';
    }
    //string plus nul character
    *errorMessage_size = str->size() + 1;
    delete str;
    return ret;
}

size_t AJ_CALL alljoyn_message_tostring(alljoyn_message msg, char* str, size_t buf)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!msg) {
        return (size_t)0;
    }
    qcc::String s = msg->msg->ToString();
    /*
     * it is ok to send in NULL for str when the user is only interested in the
     * size of the resulting string.
     */
    if (str) {
        strncpy(str, s.c_str(), buf);
        str[buf - 1] = '\0'; //prevent sting not being null terminated.
    }
    //string plus nul character
    return s.size() + 1;
}

size_t AJ_CALL alljoyn_message_description(alljoyn_message msg, char* str, size_t buf)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!msg) {
        return (size_t)0;
    }
    qcc::String s = msg->msg->Description();
    /*
     * it is ok to send in NULL for str when the user is only interested in the
     * size of the resulting string.
     */
    if (str) {
        strncpy(str, s.c_str(), buf);
        str[buf - 1] = '\0'; //prevent sting not being null terminated.
    }
    //string plus nul character
    return s.size() + 1;
}

uint32_t AJ_CALL alljoyn_message_gettimestamp(alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return msg->msg->GetTimeStamp();
}

QCC_BOOL AJ_CALL alljoyn_message_eql(const alljoyn_message one, const alljoyn_message other)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (one->msg == other->msg);
}

void AJ_CALL alljoyn_message_setendianess(const char endian)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ajn::_Message::SetEndianess(endian);
}
