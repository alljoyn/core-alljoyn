/**
 * @file
 * EndpointAuth is a utility class responsible for adding authentication
 * to BusEndpoint implementations.
 */

/******************************************************************************
 * Copyright (c) 2009-2011, 2014-2015, AllSeen Alliance. All rights reserved.
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

#include <algorithm>

#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Debug.h>
#include <qcc/Util.h>

#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/Message.h>
#include <alljoyn/version.h>

#include "RemoteEndpoint.h"
#include "EndpointAuth.h"
#include "BusUtil.h"
#include "SASLEngine.h"
#include "BusInternal.h"


#define QCC_MODULE "ENDPOINT_AUTH"

using namespace qcc;
using namespace std;

namespace ajn {


static const uint32_t HELLO_RESPONSE_TIMEOUT = 5000;

static const char* RedirectError = "org.alljoyn.error.redirect";
static const char* UntrustedError = "org.alljoyn.error.untrusted";


QStatus EndpointAuth::Hello(qcc::String& redirection)
{
    QCC_DbgTrace(("EndpointAuth::Hello(redirection=\"%s\")", redirection.c_str()));

    QStatus status;
    Message hello(bus);
    Message response(bus);
    nameTransfer = endpoint->GetFeatures().nameTransfer;
    status = hello->HelloMessage(endpoint->GetFeatures().isBusToBus, endpoint->GetFeatures().allowRemote, endpoint->GetFeatures().nameTransfer);
    if (status != ER_OK) {
        return status;
    }
    /*
     * Send the hello message and wait for a response
     */
    status = hello->Deliver(endpoint);
    if (status != ER_OK) {
        return status;
    }

    status = response->Read(endpoint, false, true, HELLO_RESPONSE_TIMEOUT);
    if (status != ER_OK) {
        return status;
    }

    status = response->Unmarshal(endpoint, false, true, HELLO_RESPONSE_TIMEOUT);
    if (status != ER_OK) {
        return status;
    }
    if (response->GetType() == MESSAGE_ERROR) {
        status = response->UnmarshalArgs("*");
        if (status != ER_OK) {
            return status;
        }
        qcc::String msg;
        if (response->GetErrorName(&msg) != NULL && strcmp(response->GetErrorName(&msg), RedirectError) == 0) {
            QCC_DbgPrintf(("Endpoint redirected: %s", msg.c_str()));
            redirection = msg;
            return ER_BUS_ENDPOINT_REDIRECTED;
        } else {
            QCC_DbgPrintf(("error: %s", response->GetErrorName(&msg)));
            QCC_DbgPrintf(("%s", msg.c_str()));
            return ER_BUS_ESTABLISH_FAILED;
        }
    }
    if (response->GetType() != MESSAGE_METHOD_RET) {
        return ER_BUS_ESTABLISH_FAILED;
    }
    if (response->GetReplySerial() != hello->GetCallSerial()) {
        return ER_BUS_UNKNOWN_SERIAL;
    }
    /*
     * Remote name for the endpoint is the sender of the reply.
     */
    remoteName = response->GetSender();
    QCC_DbgHLPrintf(("EP remote %sname %s", endpoint->GetFeatures().isBusToBus ? "(bus-to-bus) " : "", remoteName.c_str()));

    /*
     * bus-to-bus establishment uses an extended "hello" method.
     */
    if (endpoint->GetFeatures().isBusToBus) {
        status = response->UnmarshalArgs("ssu");
        if (ER_OK == status) {
            uniqueName = response->GetArg(0)->v_string.str;
            remoteGUID = qcc::GUID128(response->GetArg(1)->v_string.str);
            remoteProtocolVersion = response->GetArg(2)->v_uint32;
            if (remoteGUID == bus.GetInternal().GetGlobalGUID()) {
                QCC_DbgPrintf(("BusHello was sent to self"));
                return ER_BUS_SELF_CONNECT;
            } else {
                QCC_DbgPrintf(("Connection id: \"%s\", remoteGUID: \"%s\"\n", uniqueName.c_str(), remoteGUID.ToString().c_str()));
            }
        } else {
            return status;
        }
    } else {
        status = response->UnmarshalArgs("s");
        uniqueName = response->GetArg(0)->v_string.str;
        QCC_DbgPrintf(("Connection id: %s\n", response->GetArg(0)->v_string.str));
        if (status != ER_OK) {
            return status;
        }
    }
    /*
     * Validate the unique name
     */
    if (!IsLegalUniqueName(uniqueName.c_str())) {
        status = ER_BUS_BAD_BUS_NAME;
    }
    return status;
}


static const uint32_t REDIRECT_TIMEOUT = 30 * 1000;


QStatus EndpointAuth::WaitHello(qcc::String& authUsed)
{
    QCC_DbgTrace(("EndpointAuth::WaitHello(authUsed=\"%s\")", authUsed.c_str()));

    qcc::String redirection;
    QStatus status;
    Message hello(bus);

    status = hello->Read(endpoint, false);

    if (status != ER_OK) {
        return status;
    }
    status = hello->Unmarshal(endpoint, false);
    if (ER_OK == status) {
        if (hello->GetType() != MESSAGE_METHOD_CALL) {
            QCC_DbgPrintf(("First message must be Hello/BusHello method call"));
            return ER_BUS_ESTABLISH_FAILED;
        }

        if (strcmp(hello->GetInterface(), org::freedesktop::DBus::InterfaceName) == 0) {
            if (hello->GetCallSerial() == 0) {
                QCC_DbgPrintf(("Hello expected non-zero serial"));
                return ER_BUS_ESTABLISH_FAILED;
            }
            if (strcmp(hello->GetDestination(), org::freedesktop::DBus::WellKnownName) != 0) {
                QCC_DbgPrintf(("Hello expected destination \"%s\"", org::freedesktop::DBus::WellKnownName));
                return ER_BUS_ESTABLISH_FAILED;
            }
            if (strcmp(hello->GetObjectPath(), org::freedesktop::DBus::ObjectPath) != 0) {
                QCC_DbgPrintf(("Hello expected object path \"%s\"", org::freedesktop::DBus::ObjectPath));
                return ER_BUS_ESTABLISH_FAILED;
            }
            if (strcmp(hello->GetMemberName(), "Hello") != 0) {
                QCC_DbgPrintf(("Hello expected member \"Hello\""));
                return ER_BUS_ESTABLISH_FAILED;
            }
            endpoint->GetFeatures().isBusToBus = false;

            bool trusted = (authUsed != "ANONYMOUS");

            if (isAccepting && !trusted) {
                /* If this is an incoming connection that is not bus-to-bus and is untrusted,
                 * We need to make sure that the transport is accepting untrusted clients.
                 */
                status = endpoint->UntrustedClientStart();
                if (status != ER_OK) {
                    QCC_DbgPrintf(("Untrusted client is being rejected"));
                    hello->ErrorMsg(hello, UntrustedError, "");
                    hello->Deliver(endpoint);
                    return status;
                }
            }
            endpoint->GetFeatures().allowRemote = (0 != (hello->GetFlags() & ALLJOYN_FLAG_ALLOW_REMOTE_MSG));
            /*
             * Remote name for the endpoint is the unique name we are allocating.
             */
            remoteName = uniqueName;
        } else if (strcmp(hello->GetInterface(), org::alljoyn::Bus::InterfaceName) == 0) {
            if (hello->GetCallSerial() == 0) {
                QCC_DbgPrintf(("Hello expected non-zero serial"));
                return ER_BUS_ESTABLISH_FAILED;
            }
            if (strcmp(hello->GetDestination(), org::alljoyn::Bus::WellKnownName) != 0) {
                QCC_DbgPrintf(("Hello expected destination \"%s\"", org::alljoyn::Bus::WellKnownName));
                return ER_BUS_ESTABLISH_FAILED;
            }
            if (strcmp(hello->GetObjectPath(), org::alljoyn::Bus::ObjectPath) != 0) {
                QCC_DbgPrintf(("Hello expected object path \"%s\"", org::alljoyn::Bus::ObjectPath));
                return ER_BUS_ESTABLISH_FAILED;
            }
            if (strcmp(hello->GetMemberName(), "BusHello") != 0) {
                QCC_DbgPrintf(("Hello expected member \"BusHello\""));
                return ER_BUS_ESTABLISH_FAILED;
            }
            size_t numArgs;
            const MsgArg* args;
            status = hello->UnmarshalArgs("su");
            hello->GetArgs(numArgs, args);
            if ((ER_OK == status) && (2 == numArgs) && (ALLJOYN_STRING == args[0].typeId) && (ALLJOYN_UINT32 == args[1].typeId)) {
                remoteGUID = qcc::GUID128(args[0].v_string.str);
                uint32_t temp = args[1].v_uint32;
                remoteProtocolVersion = temp & 0x3FFFFFFF;
                nameTransfer = static_cast<SessionOpts::NameTransferType>(temp >> 30);
                if (remoteGUID == bus.GetInternal().GetGlobalGUID()) {
                    QCC_DbgPrintf(("BusHello was sent by self"));
                    return ER_BUS_SELF_CONNECT;
                }
            } else {
                QCC_DbgPrintf(("BusHello expected 2 args with signature \"su\""));
                return ER_BUS_ESTABLISH_FAILED;
            }
            endpoint->GetFeatures().isBusToBus = true;
            endpoint->GetFeatures().allowRemote = true;

            /*
             * Remote name for the endpoint is the sender of the hello.
             */
            remoteName = hello->GetSender();
        } else {
            QCC_DbgPrintf(("Hello expected interface \"%s\" or \"%s\"", org::freedesktop::DBus::InterfaceName,
                           org::alljoyn::Bus::InterfaceName));
            return ER_BUS_ESTABLISH_FAILED;
        }
        redirection = endpoint->RedirectionAddress();
        if (redirection.empty()) {
            QCC_DbgHLPrintf(("Endpoint remote %sname %s", endpoint->GetFeatures().isBusToBus ? "(bus-to-bus) " : "", remoteName.c_str()));
            status = hello->HelloReply(endpoint->GetFeatures().isBusToBus, uniqueName, nameTransfer);
        } else {
            QCC_DbgHLPrintf(("Endpoint redirecting name %s to %d", remoteName.c_str(), redirection.c_str()));
            status = hello->ErrorMsg(hello, RedirectError, redirection.c_str());
        }
    }
    if (ER_OK == status) {
        status = hello->Deliver(endpoint);
        if (ER_OK != status) {
            QCC_LogError(status, ("%s", __FUNCTION__));
        }
    }
    if ((ER_OK == status) && !redirection.empty()) {
        /*
         * We expect the other end to shutdown the endpoint socket as soon as it receives the
         * redirection error response. The only way we can tell if the socket is closed is by
         * attempting to read or write to it. We do a read with a timeout. If we actually read data
         * or the timeout expires it means the socket wasn't closed by the other end so we assume
         * the the redirection failed.
         */
        uint8_t buf[1];
        size_t sz;
        Source& source = endpoint->GetSource();
        status = source.PullBytes(buf, sizeof(buf), sz, REDIRECT_TIMEOUT);
        if (status == ER_OK || status == ER_TIMEOUT) {
            status = ER_BUS_ESTABLISH_FAILED;
        } else {
            status = ER_BUS_ENDPOINT_REDIRECTED;
        }
    }
    return status;
}


static const char NegotiateUnixFd[] = "NEGOTIATE_UNIX_FD";
static const char AgreeUnixFd[] = "AGREE_UNIX_FD";

static const char NegotiateVersion[] = "EXTENSION_NEGOTIATE_VERSION";
static const char AgreeVersion[] = "EXTENSION_AGREE_VERSION";

static const char InformProtocolVersion[] = "INFORM_PROTO_VERSION";

qcc::String EndpointAuth::SASLCallout(SASLEngine& sasl, const qcc::String& extCmd)
{
    QCC_DbgTrace(("EndpointAuth::SASLCallout(sasl=0x%p, extCmd=\"%s\")", &sasl, extCmd.c_str()));

    qcc::String rsp;

    if (sasl.GetRole() == AuthMechanism::RESPONDER) {
        // step 1: client receives empty command and replies with "NEGOTIATE_UNIX_FD [<pid>]"
        if (extCmd.empty() && endpoint->GetFeatures().handlePassing) {
            rsp = NegotiateUnixFd;
#ifdef QCC_OS_GROUP_WINDOWS
            rsp += " " + qcc::U32ToString(qcc::GetPid());
#endif
            endpoint->GetFeatures().handlePassing = false;
        } else if (extCmd.find(AgreeUnixFd) == 0) {
            // step 3: client receives "AGREE_UNIX_FD [<pid>]" and sets options
            endpoint->GetFeatures().handlePassing = true;
            endpoint->GetFeatures().processId = (qcc::StringToU32(extCmd.substr(sizeof(AgreeUnixFd) - 1), 0, -1));

            // step 4: client sends "EXTENSION_NEGOTIATE_VERSION <version>"
            rsp = NegotiateVersion;
            rsp += " " + qcc::U32ToString(ajn::GetNumericVersion());
        } else if (extCmd.find(AgreeVersion) == 0) {
            // step 7: client receives negotiated version from the client
            // pre-2.5 daemons will not send this message, leaving endpoint->alljoynVersion with default value of 0
            const uint32_t version = qcc::StringToU32(extCmd.substr(sizeof(AgreeVersion) - 1), 0, -1);
            endpoint->GetFeatures().ajVersion = version;

            // step 8: Send the protocol version
            // pre-3.1 clients will not send this message leaving endpoint.remoteProtocolVersion with default of 0
            rsp = InformProtocolVersion;
            rsp += " " + qcc::U32ToString(ALLJOYN_PROTOCOL_VERSION);
        } else if (extCmd.find(InformProtocolVersion) == 0) {
            // step 10: Store daemon's protocol version
            remoteProtocolVersion = qcc::StringToU32(extCmd.substr(sizeof(InformProtocolVersion) - 1), 0, 0);
        }
    } else {
        // step 2: daemon receives "NEGOTIATE_UNIX_FD [<pid>]", sets options, and replies with "AGREE_UNIX_FD [<pid>]"
        if (extCmd.find(NegotiateUnixFd) == 0) {
            rsp = AgreeUnixFd;
#ifdef QCC_OS_GROUP_WINDOWS
            rsp += " " + qcc::U32ToString(qcc::GetPid());
#endif
            endpoint->GetFeatures().handlePassing = true;
            endpoint->GetFeatures().processId = qcc::StringToU32(extCmd.substr(sizeof(NegotiateUnixFd) - 1), 0, -1);
        } else if (extCmd.find(NegotiateVersion) == 0) {
            // step 5: daemon receives "EXTENSION_NEGOTIATE_VERSION <version>", negotiates lowest common version
            rsp = AgreeVersion;
            const uint32_t clientVersion = qcc::StringToU32(extCmd.substr(sizeof(NegotiateVersion) - 1), 0, -1);

            // step 6: daemon responds with "EXTENSION_AGREE_VERSION <min ver>"
            const uint32_t negotiatedVersion = std::min(clientVersion, ajn::GetNumericVersion());
            endpoint->GetFeatures().ajVersion = negotiatedVersion;
            rsp += " " + qcc::U32ToString(negotiatedVersion);
        } else if (extCmd.find(InformProtocolVersion) == 0) {
            // step 9: daemon stores client's ALLJOYN_PROTOCOL_VERSION and responds with its own ALLJOYN_PROTOCOL_VERSION
            remoteProtocolVersion = qcc::StringToU32(extCmd.substr(sizeof(InformProtocolVersion) - 1), 0, 0);
            rsp = InformProtocolVersion;
            rsp += " " + qcc::U32ToString(ALLJOYN_PROTOCOL_VERSION);
        }
    }
    return rsp;
}

QStatus EndpointAuth::Establish(const qcc::String& authMechanisms,
                                qcc::String& authUsed,
                                qcc::String& redirection,
                                AuthListener* listener)
{
    QCC_DbgTrace(("EndpointAuth::Establish(authMechanism=\"%s\", authUsed=\"%s\", redirection=\"%s\", listener=0x%p)",
                  authMechanisms.c_str(), authUsed.c_str(), redirection.c_str(), listener));

    endpoint->SetFlowType(_BusEndpoint::ENDPOINT_FLOW_CHARS);

    QStatus status = ER_OK;
    size_t numPushed;
    SASLEngine::AuthState state;
    qcc::String inStr;
    qcc::String outStr;

    QCC_DbgPrintf(("EndpointAuth::Establish(): authMechanisms=\"%s\"", authMechanisms.c_str()));

    if (listener) {
        authListener.Set(listener);
    }

    if (isAccepting) {
        QCC_DbgPrintf(("EndpointAuth::Establish(): Accepting"));
        SASLEngine sasl(bus, AuthMechanism::CHALLENGER, authMechanisms, NULL, authListener, this);
        /*
         * The server's GUID is sent to the client when the authentication succeeds
         */
        String guidStr = bus.GetInternal().GetGlobalGUID().ToString();
        sasl.SetLocalId(guidStr);
        while (true) {
            /*
             * Get the challenge
             */
            inStr.clear();
            status = endpoint->GetSource().GetLine(inStr);
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed to read from stream"));
                goto ExitEstablish;
            }
            QCC_DbgPrintf(("EndpointAuth::Establish(): Got \"%s\" from stream", inStr.c_str()));
            status = sasl.Advance(inStr, outStr, state);
            if (status != ER_OK) {
                QCC_DbgPrintf(("Server authentication failed %s", QCC_StatusText(status)));
                goto ExitEstablish;
            }
            if (state == SASLEngine::ALLJOYN_AUTH_SUCCESS) {
                /*
                 * Remember the authentication mechanism that was used
                 */
                authUsed = sasl.GetMechanism();
                break;
            }
            /*
             * Send the response
             */
            QCC_DbgPrintf(("EndpointAuth::Establish(): Responding with \"%s\" to stream", outStr.c_str()));
            status = endpoint->GetSink().PushBytes((void*)(outStr.data()), outStr.length(), numPushed);
            if (status == ER_OK) {
                QCC_DbgPrintf(("Sent %s", outStr.c_str()));
            } else {
                QCC_LogError(status, ("Failed to write to stream"));
                goto ExitEstablish;
            }
        }
        /*
         * Wait for the hello message
         */
        QCC_DbgPrintf(("EndpointAuth::Establish(): WaitHello()"));
        endpoint->SetFlowType(_BusEndpoint::ENDPOINT_FLOW_HELLO);
        status = WaitHello(authUsed);
        endpoint->SetFlowType(_BusEndpoint::ENDPOINT_FLOW_MSGS);
    } else {
        QCC_DbgPrintf(("EndpointAuth::Establish(): Not accepting"));
        SASLEngine sasl(bus, AuthMechanism::RESPONDER, authMechanisms, NULL, authListener, endpoint->GetFeatures().isBusToBus ? NULL : this);
        while (true) {
            QCC_DbgPrintf(("EndpointAuth::Establish(): Advance()"));
            status = sasl.Advance(inStr, outStr, state);
            if (status != ER_OK) {
                QCC_DbgPrintf(("Client authentication failed %s", QCC_StatusText(status)));
                goto ExitEstablish;
            }
            /*
             * Send the response
             */
            QCC_DbgPrintf(("EndpointAuth::Establish(): Responding with \"%s\" to stream", outStr.c_str()));
            status = endpoint->GetSink().PushBytes((void*)(outStr.data()), outStr.length(), numPushed);
            if (status == ER_OK) {
                QCC_DbgPrintf(("Sent %s", outStr.c_str()));
            } else {
                QCC_LogError(status, ("Failed to write to stream"));
                goto ExitEstablish;
            }
            if (state == SASLEngine::ALLJOYN_AUTH_SUCCESS) {
                /*
                 * Get the server's GUID
                 */
                qcc::String id = sasl.GetRemoteId();
                if (!qcc::GUID128::IsGUID(id)) {
                    QCC_DbgPrintf(("Expected GUID got: %s", id.c_str()));
                    status = ER_BUS_ESTABLISH_FAILED;
                    goto ExitEstablish;
                }
                remoteGUID = qcc::GUID128(id);
                /*
                 * Remember the authentication mechanism that was used
                 */
                authUsed = sasl.GetMechanism();
                break;
            }
            /*
             * Get the challenge
             */
            inStr.clear();
            status = endpoint->GetSource().GetLine(inStr);
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed to read from stream"));
                goto ExitEstablish;
            }
            QCC_DbgPrintf(("EndpointAuth::Establish(): Got \"%s\" from stream", inStr.c_str()));
        }
        /*
         * Send the hello message and wait for a response
         */
        QCC_DbgPrintf(("EndpointAuth::Establish(): Hello()"));
        endpoint->SetFlowType(_BusEndpoint::ENDPOINT_FLOW_HELLO);
        status = Hello(redirection);
        endpoint->SetFlowType(_BusEndpoint::ENDPOINT_FLOW_MSGS);
    }

ExitEstablish:

    authListener.Set(NULL);

    QCC_DbgPrintf(("Establish complete %s", QCC_StatusText(status)));

    return status;
}

}
