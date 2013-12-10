/**
 * @file
 * The SASLEngine class implements the Simple Authentication and Security Layer (SASL)
 * authentication protocol described in RFC2222. This is used by the DBus wire protocol and for AllJoyn
 * peer authentication.
 */

/******************************************************************************
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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

#include "SASLEngine.h"
#include "BusInternal.h"


#define QCC_MODULE "ALLJOYN_AUTH"

using namespace std;
using namespace qcc;

#define CRLF     "\r\n"

/*
 * This is just a sanity check to prevent broken implementations from looping forever.
 */
#define MAX_AUTH_COUNT 64

namespace ajn {

typedef enum {
    CMD_AUTH,
    CMD_CANCEL,
    CMD_BEGIN,
    CMD_DATA,
    CMD_ERROR,
    CMD_REJECTED,
    CMD_OK,
    CMD_INVALID
} AuthCmdType;


typedef struct {
    AuthCmdType cmdType;
    const char* cmdStr;
    uint8_t len;
} AuthCmd;


static const AuthCmd AllJoynAuthCmdList[] = {
    { CMD_AUTH,     "AUTH",     4 },
    { CMD_CANCEL,   "CANCEL",   6 },
    { CMD_BEGIN,    "BEGIN",    5 },
    { CMD_DATA,     "DATA",     4 },
    { CMD_ERROR,    "ERROR",    5 },
    { CMD_REJECTED, "REJECTED", 8 },
    { CMD_OK,       "OK",       2 },
    { CMD_INVALID,  "invalid",  0 }
};


#ifndef NDEBUG
static const char* StateText[] = {
    "SEND_AUTH_REQ",
    "WAIT_FOR_AUTH",
    "WAIT_FOR_BEGIN",
    "WAIT_FOR_DATA",
    "WAIT_FOR_OK",
    "WAIT_FOR_REJECT",
    "WAIT_EXT_RESPONSE",
    "AUTH_SUCCESS",
    "AUTH_FAILED"
};
#endif

#define SetState(s) \
    do { \
        if (authState != (s)) { \
            QCC_DbgPrintf(("New %s state %s\n", (authRole == AuthMechanism::CHALLENGER) ? "Challenger" : "Responder", StateText[s])); \
            authState = (s); \
        } \
    } while (0)



static qcc::String HexToAscii(qcc::String& hex)
{
    qcc::String ascii;
    size_t len = hex.length() / 2;
    char* asciiStr = new char[1 + len];
    if (HexStringToBytes(hex, (uint8_t*)asciiStr, len) == len) {
        asciiStr[len] = 0;
        ascii = asciiStr;
    }
    if ((len > 0) && ascii.empty()) {
        QCC_DbgPrintf(("Expected hex-encoded data got: \"%s\"", hex.c_str()));
    }
    delete [] asciiStr;
    return ascii;

}

static qcc::String AsciiToHex(const qcc::String& ascii)
{
    return BytesToHexString((const uint8_t*)ascii.data(), ascii.length(), true /* toLower */);
}

static void ParseAuthNames(std::set<qcc::String>& authSet, const qcc::String& mechanisms)
{
    qcc::String names = mechanisms;

    while (!names.empty()) {
        size_t pos = names.find_first_of(' ');
        if (pos == 0) {
            names.erase(0, 1);
        } else if (pos == qcc::String::npos) {
            authSet.insert(names);
            names.clear();
        } else {
            authSet.insert(names.substr(0, pos));
            names.erase(0, pos + 1);
        }
    }
}

static qcc::String ExpandAuthNames(const std::set<qcc::String>& authSet)
{
    qcc::String authNames;
    for (std::set<qcc::String>::const_iterator it = authSet.begin(); it != authSet.end(); it++) {
        if (it != authSet.begin()) {
            authNames.push_back(' ');
        }
        authNames += *it;
    }
    return authNames;
}

static AuthCmdType ParseAuth(qcc::String& str)
{
    /* Remove CRLF if present and anything following */
    size_t pos = str.find(CRLF);
    if (pos != qcc::String::npos) {
        str.erase(pos);
    }
    size_t i;
    for (i = 0; i < (ArraySize(AllJoynAuthCmdList) - 1); ++i) {
        if (str.compare(0, AllJoynAuthCmdList[i].len, AllJoynAuthCmdList[i].cmdStr) == 0) {
            str.erase(0, AllJoynAuthCmdList[i].len);
            break;
        }
    }
    return AllJoynAuthCmdList[i].cmdType;
}

static void ComposeAuth(qcc::String& outStr, AuthCmdType cmd, const qcc::String& str1, const qcc::String& str2)
{
    outStr = AllJoynAuthCmdList[cmd].cmdStr;
    if (!str1.empty()) {
        outStr += ' ' + str1;
    }
    if (!str2.empty()) {
        outStr += ' ' + str2;
    }
    outStr.push_back('\r');
    outStr.push_back('\n');
}

static void ComposeAuth(qcc::String& outStr, AuthCmdType cmd, const qcc::String& str1)
{
    ComposeAuth(outStr, cmd, str1, String());
}

static void ComposeAuth(qcc::String& outStr, AuthCmdType cmd)
{
    ComposeAuth(outStr, cmd, String(), String());
}

/*
 * Composes an AUTH command using the current set of authentication methods.
 */
QStatus SASLEngine::NewAuthRequest(qcc::String& authCmd)
{
    QStatus status = ER_OK;

    authCmd.clear();
    /*
     * If there is a current mechanism and it is no longer in the auth set delete it, otherwise remove the name.
     */
    if (authMechanism && (authSet.erase(authMechanism->GetName()) == 0)) {
        delete authMechanism;
        authMechanism = NULL;
    }
    /*
     * Use current or get next auth mechanism
     */
    while (true) {
        if (authMechanism) {
            status = authMechanism->Init(authRole, authPeer);
            if (status == ER_OK) {
                QCC_DbgPrintf(("Initialized authMechanism %s", authMechanism->GetName()));
                AuthMechanism::AuthResult authResult;
                qcc::String response = AsciiToHex(authMechanism->InitialResponse(authResult));
                if ((authResult == AuthMechanism::ALLJOYN_AUTH_OK) || (authResult == AuthMechanism::ALLJOYN_AUTH_CONTINUE)) {
                    SetState((authResult == AuthMechanism::ALLJOYN_AUTH_OK) ?  ALLJOYN_WAIT_FOR_OK : ALLJOYN_WAIT_FOR_DATA);
                    ComposeAuth(authCmd, CMD_AUTH, authMechanism->GetName(), response);
                    break;
                }
                QCC_LogError(ER_AUTH_FAIL, ("InitialReponse failed authMechanism %s", authMechanism->GetName()));
            } else {
                QCC_LogError(status, ("Failed to initialize authMechanism %s", authMechanism->GetName()));
            }
            /*
             * Permanently skip mechanisms that fail to initialize.
             */
            delete authMechanism;
            authMechanism = NULL;
        }
        /*
         * If we ran out of mechanisms to try we cannot authenticate and we are done.
         */
        if (authSet.empty()) {
            status = ER_AUTH_FAIL;
            QCC_DbgPrintf(("No auth mechanism"));
            break;
        }
        std::set<qcc::String>::iterator it = authSet.begin();
        authMechanism = bus.GetInternal().GetAuthManager().GetMechanism(*it, listener);
        authSet.erase(it);
    }
    return status;
}

QStatus SASLEngine::Response(qcc::String& inStr, qcc::String& outStr)
{
    AuthMechanism::AuthResult authResult;
    qcc::String challenge;
    qcc::String response;
    QStatus status = ER_OK;
    AuthCmdType cmd;

    outStr.clear();

    /*
     * Sanity check to prevent broken implementations from looping forever.
     */
    if (authCount > MAX_AUTH_COUNT) {
        SetState(ALLJOYN_AUTH_FAILED);
    }
    if (authState == ALLJOYN_AUTH_FAILED) {
        return ER_AUTH_FAIL;
    }

    if (authState == ALLJOYN_SEND_AUTH_REQ) {
        qcc::String authNames = ExpandAuthNames(authSet);
        QCC_DbgPrintf(("Responder starting auth conversation %s", authNames.c_str()));
        /*
         * This will cause the initial AUTH request to be sent.
         */
        cmd = CMD_REJECTED;
        inStr = " " + authNames;
    } else {
        QCC_DbgPrintf(("Responder read %s", inStr.c_str()));
        cmd = ParseAuth(inStr);
    }

    switch (cmd) {
    case CMD_REJECTED:
        /*
         * If we didn't solicit this reject retry the current auth mechanism.
         */
        if (authMechanism && (authState != ALLJOYN_WAIT_FOR_REJECT)) {
            authSet.insert(authMechanism->GetName());
        }
        QCC_DbgPrintf(("Current authSet %s", ExpandAuthNames(authSet).c_str()));
        {
            std::set<qcc::String> inAuth;
            ParseAuthNames(inAuth, inStr);
            /*
             * Remove all authentication mechanisms that are not in the rejected message.
             */
            std::set<qcc::String>::iterator it = authSet.begin();
            while (it != authSet.end()) {
                if (inAuth.count(*it) == 0) {
                    authSet.erase(*it);
                    it = authSet.begin();
                } else {
                    ++it;
                }
            }
            status = NewAuthRequest(outStr);
        }
        break;

    case CMD_DATA:
        switch (authState) {
        case ALLJOYN_WAIT_FOR_DATA:
            /*
             * Allow a null challenge string
             */
            if (!inStr.empty()) {
                /*
                 * Decode the hex-encoded challenge (minus leading space)
                 */
                challenge = HexToAscii(inStr.erase(0, 1));
                if (challenge.empty()) {
                    response = "Expected hex-encoded data";
                    cmd = CMD_ERROR;
                    break;
                }
                QCC_DbgPrintf(("Challenge: %s", challenge.c_str()));
            }
            response = AsciiToHex(authMechanism->Response(challenge, authResult));
            if (authResult == AuthMechanism::ALLJOYN_AUTH_OK) {
                SetState(ALLJOYN_WAIT_FOR_OK);
                cmd = CMD_DATA;
            } else if (authResult == AuthMechanism::ALLJOYN_AUTH_ERROR) {
                cmd = CMD_ERROR;
                SetState(ALLJOYN_WAIT_FOR_DATA);
            } else if (authResult == AuthMechanism::ALLJOYN_AUTH_CONTINUE) {
                cmd = CMD_DATA;
                SetState(ALLJOYN_WAIT_FOR_DATA);
            } else {
                if (authResult == AuthMechanism::ALLJOYN_AUTH_RETRY) {
                    /* Retry the current authentication mechanism */
                    authSet.insert(authMechanism->GetName());
                }
                cmd = CMD_CANCEL;
                response.clear();
                SetState(ALLJOYN_WAIT_FOR_REJECT);
            }
            break;

        case ALLJOYN_WAIT_FOR_OK:
            cmd = CMD_CANCEL;
            SetState(ALLJOYN_WAIT_FOR_REJECT);
            break;

        default:
            status = ER_AUTH_FAIL;
            break;
        }
        break;

    case CMD_OK:
        switch (authState) {
        case ALLJOYN_WAIT_FOR_DATA:
        case ALLJOYN_WAIT_FOR_OK:
            /*
             * Succesfully authenticated - check for extension commands.
             */
            remoteId = inStr.erase(0, 1);
            if (extHandler) {
                outStr = extHandler->SASLCallout(*this, "");
                if (!outStr.empty()) {
                    SetState(ALLJOYN_WAIT_EXT_RESPONSE);
                    outStr += CRLF;
                }
            }
            if (outStr.empty()) {
                response = localId;
                cmd = CMD_BEGIN;
                SetState(ALLJOYN_AUTH_SUCCESS);
            }
            break;

        default:
            status = ER_AUTH_FAIL;
            break;
        }
        break;

    case CMD_ERROR:
        switch (authState) {
        case ALLJOYN_WAIT_FOR_DATA:
            cmd = CMD_CANCEL;
            SetState(ALLJOYN_WAIT_FOR_REJECT);
            break;

        case ALLJOYN_WAIT_FOR_OK:
            response = localId;
            cmd = CMD_BEGIN;
            SetState(ALLJOYN_WAIT_FOR_REJECT);
            break;

        case ALLJOYN_WAIT_EXT_RESPONSE:
            outStr = extHandler->SASLCallout(*this, "ERROR");
            if (!outStr.empty()) {
                outStr += CRLF;
            } else {
                response = localId;
                cmd = CMD_BEGIN;
                SetState(ALLJOYN_AUTH_SUCCESS);
            }
            break;

        default:
            status = ER_AUTH_FAIL;
            break;
        }
        break;

    default:
        switch (authState) {
        case ALLJOYN_WAIT_EXT_RESPONSE:
            outStr = extHandler->SASLCallout(*this, inStr);
            if (!outStr.empty()) {
                outStr += CRLF;
            } else {
                response = localId;
                cmd = CMD_BEGIN;
                SetState(ALLJOYN_AUTH_SUCCESS);
            }
            break;

        case ALLJOYN_WAIT_FOR_DATA:
        case ALLJOYN_WAIT_FOR_OK:
            response = "Unexpected Command";
            cmd = CMD_ERROR;
            break;

        default:
            status = ER_AUTH_FAIL;
            break;
        }
        break;
    }

    if (status == ER_OK) {
        if (outStr.empty()) {
            ComposeAuth(outStr, cmd, response);
        }
        QCC_DbgPrintf(("Responder sending %s", outStr.c_str()));
    } else {
        QCC_DbgPrintf(("Responder auth failed: %s", QCC_StatusText(status)));
        SetState(ALLJOYN_AUTH_FAILED);
        if (authCount > 0) {
            /*
             * This should cause the server to terminate the authentication conversation
             */
            ComposeAuth(outStr, CMD_BEGIN);
            status = ER_OK;
        }
    }
    return status;
}


QStatus SASLEngine::Challenge(qcc::String& inStr, qcc::String& outStr)
{
    qcc::String response;
    QStatus status = ER_OK;
    AuthCmdType cmd;

    outStr.clear();

    /*
     * Sanity check to prevent broken implementations from looping forever.
     */
    if (authCount > MAX_AUTH_COUNT) {
        SetState(ALLJOYN_AUTH_FAILED);
    }
    if (authState == ALLJOYN_AUTH_FAILED) {
        return ER_AUTH_FAIL;
    }

    QCC_DbgPrintf(("Challenger read %s", inStr.c_str()));
    cmd = ParseAuth(inStr);

    switch (cmd) {
    case CMD_AUTH:
        if (authState == ALLJOYN_WAIT_FOR_AUTH) {
            qcc::String mechanismName;
            qcc::String challenge;
            AuthMechanism::AuthResult authResult;

            inStr.erase(0, 1);
            size_t nameEnd = inStr.find_first_of(' ');
            mechanismName = inStr.substr(0, nameEnd);
            /* Check for data following the auth mechanism name */
            if (!inStr.erase(0, nameEnd).empty()) {
                response = HexToAscii(inStr.erase(0, 1));
                if (response.empty()) {
                    response = "Expected hex-encoded data";
                    cmd = CMD_ERROR;
                    break;
                }
            }
            /* Check the requested authentication mechanism is supported */
            if (authSet.count(mechanismName) == 0) {
                response = ExpandAuthNames(authSet);
                cmd = CMD_REJECTED;
                break;
            }
            /* Check if we are retrying the current auth mechanism or starting a new one */
            if (authMechanism && (mechanismName == authMechanism->GetName())) {
                QCC_DbgPrintf(("Challenger retrying auth mechanism %s", mechanismName.c_str()));
            } else {
                delete authMechanism;
                QCC_DbgPrintf(("Challenger trying new auth mechanism %s", mechanismName.c_str()));
                authMechanism = bus.GetInternal().GetAuthManager().GetMechanism(mechanismName, listener);
            }
            if (authMechanism) {
                status = authMechanism->Init(authRole, authPeer);
                if (status != ER_OK) {
                    delete authMechanism;
                    authMechanism = NULL;
                }
            }
            /* If we still don't have an authentication mechanism send a reject */
            if (!authMechanism) {
                response = ExpandAuthNames(authSet);
                cmd = CMD_REJECTED;
                break;
            }
            if (response.empty()) {
                challenge = authMechanism->InitialChallenge(authResult);
            } else {
                QCC_DbgPrintf(("Initial response: %s", response.c_str()));
                challenge = authMechanism->Challenge(response, authResult);
            }
            if (authResult == AuthMechanism::ALLJOYN_AUTH_OK) {
                response = localId;
                cmd = CMD_OK;
                SetState(ALLJOYN_WAIT_FOR_BEGIN);
            } else if (authResult == AuthMechanism::ALLJOYN_AUTH_CONTINUE) {
                response = AsciiToHex(challenge);
                cmd = CMD_DATA;
                SetState(ALLJOYN_WAIT_FOR_DATA);
            } else if (authResult == AuthMechanism::ALLJOYN_AUTH_ERROR) {
                response = challenge.empty() ? "Invalid response" : challenge.c_str();
                cmd = CMD_ERROR;
            } else if (authResult == AuthMechanism::ALLJOYN_AUTH_RETRY) {
                response = ExpandAuthNames(authSet);
                cmd = CMD_REJECTED;
            } else {
                SetState(ALLJOYN_AUTH_FAILED);
                status = ER_AUTH_FAIL;
            }
        } else {
            response = "Unexpected";
            cmd = CMD_ERROR;
        }
        break;

    case CMD_BEGIN:
        if (authState == ALLJOYN_WAIT_FOR_BEGIN) {
            /*
             * Succesfully authenticated
             */
            remoteId = inStr.erase(0, 1);
            SetState(ALLJOYN_AUTH_SUCCESS);
        } else {
            /*
             * Failure to authenticate
             */
            status = ER_AUTH_FAIL;
        }
        break;

    case CMD_CANCEL:
    case CMD_ERROR:
        if (authState == ALLJOYN_WAIT_FOR_AUTH) {
            response = "Expecting AUTH";
            cmd = CMD_ERROR;
        } else {
            response = ExpandAuthNames(authSet);
            cmd = CMD_REJECTED;
            if (authState != ALLJOYN_AUTH_FAILED) {
                SetState(ALLJOYN_WAIT_FOR_AUTH);
            }
        }
        break;

    case CMD_DATA:
        if (authState == ALLJOYN_WAIT_FOR_DATA) {
            /*
             * Decode the hex-encoded response (minus leading space)
             */
            response = HexToAscii(inStr.erase(0, 1));
            if (response.empty()) {
                response = "Expected hex-encoded data";
                cmd = CMD_ERROR;
            } else {
                QCC_DbgPrintf(("Response: %s", response.c_str()));
                AuthMechanism::AuthResult authResult;
                qcc::String challenge = authMechanism->Challenge(response, authResult);
                if (authResult == AuthMechanism::ALLJOYN_AUTH_OK) {
                    response = localId;
                    cmd = CMD_OK;
                    SetState(ALLJOYN_WAIT_FOR_BEGIN);
                } else if (authResult == AuthMechanism::ALLJOYN_AUTH_CONTINUE) {
                    response = AsciiToHex(challenge);
                    cmd = CMD_DATA;
                    SetState(ALLJOYN_WAIT_FOR_DATA);
                } else if (authResult == AuthMechanism::ALLJOYN_AUTH_RETRY) {
                    response = ExpandAuthNames(authSet);
                    cmd = CMD_REJECTED;
                    SetState(ALLJOYN_WAIT_FOR_AUTH);
                } else {
                    SetState(ALLJOYN_AUTH_FAILED);
                    status = ER_AUTH_FAIL;
                }
            }
        } else {
            /* No state change */
            response = "Unexpected";
            cmd = CMD_ERROR;
        }
        break;

    default:
        if (extHandler && (authState == ALLJOYN_WAIT_FOR_BEGIN)) {
            /*
             * Commands received after main authentication conversation is complete may be
             * extension commands.
             */
            outStr = extHandler->SASLCallout(*this, inStr);
            if (!outStr.empty()) {
                outStr += CRLF;
            }
        }
        if (outStr.empty()) {
            response = "Unknown";
            cmd = CMD_ERROR;
        }
        break;
    }

    if (outStr.empty()) {
        ComposeAuth(outStr, cmd, response);
    }

    QCC_DbgPrintf(("Challenger sending %s", outStr.c_str()));
    return status;
}


QStatus SASLEngine::Advance(qcc::String authIn, qcc::String& authOut, AuthState& state)
{
    QStatus status;

    if ((authState == ALLJOYN_AUTH_SUCCESS) || (authState == ALLJOYN_AUTH_FAILED)) {
        return ER_BUS_NOT_AUTHENTICATING;
    }
    if (authRole == AuthMechanism::RESPONDER) {
        status = Response(authIn, authOut);
    } else {
        status = Challenge(authIn, authOut);
    }
    if (status == ER_OK) {
        state = authState;
        if (state == ALLJOYN_AUTH_SUCCESS) {
            /*
             * Depending on the authentication mechanism used the responder may or may not have
             * been authenticated to the challenger. Save this information so it can be reported
             * to the upper layer.
             */
            authIsMutual = authMechanism->IsMutual();
        }
    } else {
        SetState(ALLJOYN_AUTH_FAILED);
    }
    ++authCount;
    return status;
}

SASLEngine::SASLEngine(BusAttachment& bus, AuthMechanism::AuthRole authRole, const qcc::String& mechanisms, const char* authPeer, ProtectedAuthListener& listener, ExtensionHandler* extHandler) :
    bus(bus),
    authRole(authRole),
    authPeer(authPeer),
    listener(listener),
    authCount(0),
    authMechanism(NULL),
    authState((authRole == AuthMechanism::RESPONDER) ? ALLJOYN_SEND_AUTH_REQ : ALLJOYN_WAIT_FOR_AUTH),
    extHandler(extHandler),
    authIsMutual(false)
{
    ParseAuthNames(authSet, mechanisms);
    QCC_DbgPrintf(("SASL %s mechanisms %s", (authRole == AuthMechanism::RESPONDER) ? "Responder" : "Challenger", mechanisms.c_str()));
}


SASLEngine::~SASLEngine()
{
    delete authMechanism;
}

}
