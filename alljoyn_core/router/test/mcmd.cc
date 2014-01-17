/**
 * @file
 * "Simple" tool for sending method call commands to the bus.
 *
 * NOTE: This tool (ab)uses some AllJoyn interfaces in inappropriate ways and
 * contains some bad programing constructs (i.e., known memory leaks, etc.).
 * The code in this tool should not be used as an example of how to use the
 * AllJoyn API.
 */

/******************************************************************************
 * Copyright (c) 2010-2011, AllSeen Alliance. All rights reserved.
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
#include <signal.h>
#include <stdio.h>
#include <vector>
#include <deque>

#include <qcc/Debug.h>
#include <qcc/Environ.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/version.h>

#if !defined(QCC_OS_DARWIN)
#include "BTTransport.h"
#endif

#include "Bus.h"
#include "SignatureUtils.h"

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;
using namespace ajn;


class Token {
  public:
    enum Type {
        ARGUMENT,
        GROUPING
    };

    Token(Type t, qcc::String a) : type(t), arg(a) { }
    Type GetType() const { return type; }
    const qcc::String& GetArg() const { return arg; }

  private:
    Type type;
    qcc::String arg;
};


enum Actions {
    NO_ACTION,
    LIST,
    LIST_ALL,
    LIST_OBJECTS,
    INTROSPECT,
    METHOD_CALL,
    METHOD_CALL_INTROSPECT
};


enum GroupType {
    GT_NONE,
    GT_ARRAY,
    GT_DICT,
    GT_STRUCT
};


static Actions action;
static qcc::String busAddr;
static qcc::String dest;
static qcc::String objPath;
static qcc::String method;
static qcc::String signature;
static int callArgc;
static char** callArgv;

#if !defined(QCC_OS_GROUP_WINDOWS) && !defined(QCC_OS_DARWIN)
static BTTransport* btTrans;
#endif

class AutoConnect : public TransportListener {

  public:
    AutoConnect(BusAttachment& bus, Transport& transport) : bus(bus), transport(transport) { }

    void FoundBus(const qcc::String& busAddr, const vector<qcc::String>& names);
    QStatus Wait(uint32_t timeout = 30000);

  private:
    BusAttachment& bus;
    Transport& transport;
    Event found;
    QStatus status;
};

void AutoConnect::FoundBus(const qcc::String& busAddr, const vector<qcc::String>& names)
{
    printf("Connecting to discovered bus: %s\n", busAddr.c_str());
    status = bus.Connect(busAddr.c_str());
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to connect"));
    }

    found.SetEvent();
}

QStatus AutoConnect::Wait(uint32_t timeout)
{
    QStatus s = Event::Wait(found, timeout);
    if (s != ER_OK) {
        transport.DisableDiscovery("I am broken");
    } else {
        s = status;
    }
    return s;
}


struct BNComp {
    bool operator()(const qcc::String& lhs, const qcc::String& rhs) const
    {
        int count = 0;
        if (lhs == rhs) {
            return false;
        } else if ((lhs[0] == ':') && (rhs[0] != ':')) {
            return false;
        } else if ((lhs[0] != ':') && (rhs[0] == ':')) {
            return true;
        } else if ((lhs[0] != ':') && (rhs[0] != ':')) {
            return lhs < rhs;
        } else {
            size_t lstart(1), lend(1);
            size_t rstart(1), rend(1);

            while ((lstart < lhs.size()) && (rstart < rhs.size())) {
                lend = lhs.find_first_of('.', lstart);
                rend = rhs.find_first_of('.', rstart);
                static const uint64_t INVALID(static_cast<uint64_t>(-1));
                uint64_t ln = StringToU64(lhs.substr(lstart, min(lend - lstart, sizeof(uint64_t))), 0, INVALID);
                uint64_t rn = StringToU64(rhs.substr(rstart, min(rend - rstart, sizeof(uint64_t))), 0, INVALID);
                if (ln == INVALID) {
                    ln = StringToU64(lhs.substr(lstart, min(lend - lstart, sizeof(uint64_t))), 16, INVALID);
                }
                if (rn == INVALID) {
                    rn = StringToU64(rhs.substr(rstart, min(rend - rstart, sizeof(uint64_t))), 16, INVALID);
                }
                if (++count > 100) {
                    printf("BusName comparison overflow.\n");
                    exit(1);
                }
                if (ln != rn) {
                    return ln < rn;
                }
                lstart = lend + ((lend == qcc::String::npos) ? 0 : 1);
                rstart = rend + ((rend == qcc::String::npos) ? 0 : 1);
            }
            return (lstart == qcc::String::npos);
        }
    }
};

typedef map<qcc::String, qcc::String, BNComp> BusNameMap;


static QStatus ListBusNames(BusAttachment& bus, BusNameMap& names)
{
    QStatus status;
    ProxyBusObject robj(bus, "org.freedesktop.DBus", "/org/freedesktop/DBus", 0);
    const InterfaceDescription* ifc = bus.GetInterface(org::freedesktop::DBus::InterfaceName);
    const InterfaceDescription::Member* listNames = ifc ? ifc->GetMember("ListNames") : NULL;
    const InterfaceDescription::Member* getNameOwner = ifc ? ifc->GetMember("GetNameOwner") : NULL;
    Message rsp(bus);

    if (!listNames || !getNameOwner) {
        return ER_FAIL;
    }

    robj.AddInterface(org::freedesktop::DBus::InterfaceName);
    robj.AddInterface(org::freedesktop::DBus::Introspectable::InterfaceName);

    status = robj.MethodCall(*listNames, NULL, 0, rsp);

    if (status == ER_OK) {
        const MsgArg* array = rsp->GetArg(0);
        const MsgArg* nameArray = array->v_array.GetElements();
        for (size_t i = 0; (status == ER_OK) && (i < array->v_array.GetNumElements()); ++i, ++nameArray) {
            const char* name = nameArray->v_string.str;
            if (name[0] == ':') {
                if (names.find(name) == names.end()) {
                    names[name] = "";
                }
            } else {
                if (strcmp(name, "org.freedesktop.DBus") == 0) {
                    names["----"] = name;
                } else {
                    Message rsp(bus);
                    MsgArg arg("s", name);
                    ProxyBusObject dbusObj(bus.GetDBusProxyObj());
                    dbusObj.AddInterface(org::freedesktop::DBus::InterfaceName);
                    dbusObj.AddInterface(org::freedesktop::DBus::Introspectable::InterfaceName);

                    status = dbusObj.MethodCall(*getNameOwner, &arg, 1, rsp);
                    if (status == ER_OK) {
                        names[rsp->GetArg(0)->v_string.str] = name;
                    }
                }
            }
        }
    } else {
        printf("Error:\n%s\n", rsp->ToString().c_str());
    }

    return status;
}


static QStatus ListObjectPaths(BusAttachment& bus, ProxyBusObject& robj)
{
    QStatus status = robj.IntrospectRemoteObject();

    if (status == ER_OK) {
        size_t numChildren = robj.GetChildren();
        ProxyBusObject** children = new ProxyBusObject *[numChildren];
        robj.GetChildren(children, numChildren);

        Message rsp(bus);
        QStatus istatus = ER_OK;
        const InterfaceDescription* ifc = bus.GetInterface(org::freedesktop::DBus::Introspectable::InterfaceName);
        if (ifc == NULL) {
            istatus = ER_BUS_NO_SUCH_INTERFACE;
            QCC_LogError(istatus, ("Failed to get Introspect interface"));
        }
        if (istatus == ER_OK) {
            const InterfaceDescription::Member* member = ifc->GetMember("Introspect");

            istatus =  robj.MethodCall(*member, NULL, 0, rsp);
        }

        if (istatus == ER_OK) {
            const MsgArg* data = rsp->GetArg(0);
            const qcc::String s(data->v_string.str);
            if (s.find("<interface") != qcc::String::npos) {
                printf("%s\n", robj.GetPath().c_str());
            }
        } else {
            if (status == ER_OK) {
                status = istatus;
            } else {
                QCC_LogError(istatus, ("Failed IntrospectObject()"));
            }
        }

        for (size_t i = 0; i < numChildren; i++) {
            istatus = ListObjectPaths(bus, *(children[i]));
            if (status == ER_OK) {
                status = istatus;
            } else {
                QCC_LogError(istatus, ("Failed ListObjectPaths()"));
            }
        }
        delete [] children;
    } else {
        QCC_LogError(status, ("Failed IntrospectRemoteObject(%s)", robj.GetPath().c_str()));
    }
    return status;
}


static MsgArg* ParseCallArgToken(deque<AllJoynTypeId>& sigTokens, deque<const Token*>& argTokens, GroupType groupType = GT_NONE);

static MsgArg* ProcessArray(deque<AllJoynTypeId>& sigTokens, deque<const Token*>& argTokens) {
    const Token* token;
    deque<AllJoynTypeId> sigTokensCopy;
    vector<MsgArg*> args;
    MsgArg* arg = NULL;
    qcc::String sig;

    do {
        sigTokensCopy = sigTokens;
        arg = ParseCallArgToken(sigTokensCopy, argTokens, GT_ARRAY);
        if (arg) {
            args.push_back(arg);
        }
        if (argTokens.size() == 0) {
            printf("Missing ']' grouping token for array.\n");
            exit(1);
        }
        token = argTokens.front();
    } while ((token->GetType() != Token::GROUPING) || (token->GetArg() != "]"));

    for (size_t i = 0; i < (sigTokens.size() - sigTokensCopy.size()); ++i) {
        sig += sigTokens[i];
    }

    argTokens.pop_front();
    sigTokens = sigTokensCopy;

    MsgArg* elements = new MsgArg[args.size()];
    for (size_t i = 0; i < args.size(); ++i) {
        MsgArg* arg = args[i];
        args[i] = NULL;
        elements[i] = *arg;
        delete arg;
    }
    arg = new MsgArg(ALLJOYN_ARRAY);
    arg->v_array.SetElements(sig.c_str(), args.size(), elements);

    return arg;
}


static MsgArg* ParseCallArgToken(deque<AllJoynTypeId>& sigTokens, deque<const Token*>& argTokens, GroupType groupType) {
    MsgArg* arg = NULL;
    if (sigTokens.empty()) {
        printf("Too few tokens for signature.\n");
        exit(1);
    }
    AllJoynTypeId sig = sigTokens.front();

    if (argTokens.empty()) {
        printf("Too few tokens for signature.\n");
        exit(1);
    }

    if (((groupType != GT_STRUCT) && (sig == ALLJOYN_STRUCT_CLOSE)) ||
        ((groupType != GT_DICT) && (sig == ALLJOYN_DICT_ENTRY_CLOSE))) {
        printf("Unbalanced '%c' signature token encountered.\n", sig);
        exit(1);
    }

    const Token& token = *argTokens.front();
    bool typeOK = false;

    sigTokens.pop_front();
    argTokens.pop_front();

    switch (sig) {
    case ALLJOYN_BOOLEAN:
    case ALLJOYN_BYTE:
    case ALLJOYN_DOUBLE:
    case ALLJOYN_INT16:
    case ALLJOYN_INT32:
    case ALLJOYN_INT64:
    case ALLJOYN_OBJECT_PATH:
    case ALLJOYN_SIGNATURE:
    case ALLJOYN_STRING:
    case ALLJOYN_UINT16:
    case ALLJOYN_UINT32:
    case ALLJOYN_UINT64:
    case ALLJOYN_VARIANT:
        typeOK = token.GetType() == Token::ARGUMENT;
        break;

    case ALLJOYN_ARRAY:
    case ALLJOYN_DICT_ENTRY_OPEN:
    case ALLJOYN_DICT_ENTRY_CLOSE:
    case ALLJOYN_STRUCT_OPEN:
    case ALLJOYN_STRUCT_CLOSE:
        typeOK = token.GetType() == Token::GROUPING;
        break;


    case ALLJOYN_INVALID:
        printf("Invalid signature element: '\\0' (NUL terminator is invalid).\n");
        exit(1);

    case ALLJOYN_DICT_ENTRY:
        printf("Invalid signature element: 'e' (use '{' and '}' for dict entries).\n");
        exit(1);

    case ALLJOYN_STRUCT:
        printf("Invalid signature element: 'r' (use '(' and ')' for struct entries).\n");
        exit(1);

    default:
        assert(sig < 256);  // should be impossible for the parser to allow this to happen
    }

    if (!typeOK) {
        switch (token.GetType()) {
        case Token::ARGUMENT:
            printf("Expected grouping token ('[' or ']') instead of '%s'\n",
                   token.GetArg().c_str());
            break;

        case Token::GROUPING:
            printf("Expected call argument value token instead of grouping token '%s'\n",
                   token.GetArg().c_str());
            break;

        default:
            printf("Unknown token type: %d\n", token.GetType());
            break;
        }
        exit(1);
    }

    if ((token.GetType() == Token::GROUPING) && (token.GetArg() == "]")) {
        if (groupType == GT_NONE) {
            printf("Unbalanced ']' grouping token encountered.\n");
            exit(1);
        } else if (groupType == GT_ARRAY) {
            return NULL;
        }
    }

    switch (sig) {
    case ALLJOYN_BOOLEAN:
        arg = new MsgArg(sig);
        if (strcmp(token.GetArg().c_str(), "false") == 0) {
            arg->v_bool = false;
        } else if (strcmp(token.GetArg().c_str(), "true") == 0) {
            arg->v_bool = true;
        } else {
            arg->v_bool = static_cast<bool>(StringToU32(token.GetArg(), 0, 1));
        }
        break;

    case ALLJOYN_BYTE:
        arg = new MsgArg(sig);
        arg->v_byte = static_cast<uint8_t>(StringToU32(token.GetArg()));
        break;

    case ALLJOYN_INT16:
        arg = new MsgArg(sig);
        arg->v_int16 = static_cast<int16_t>(StringToI32(token.GetArg()));
        break;

    case ALLJOYN_UINT16:
        arg = new MsgArg(sig);
        arg->v_uint16 = static_cast<uint16_t>(StringToU32(token.GetArg()));
        break;

    case ALLJOYN_INT32:
        arg = new MsgArg(sig);
        arg->v_int32 = StringToI32(token.GetArg());
        break;

    case ALLJOYN_UINT32:
        arg = new MsgArg(sig);
        arg->v_uint32 = StringToU32(token.GetArg());
        break;

    case ALLJOYN_INT64:
        arg = new MsgArg(sig);
        arg->v_int64 = StringToI64(token.GetArg());
        break;

    case ALLJOYN_UINT64:
        arg = new MsgArg(sig);
        arg->v_uint64 = StringToU64(token.GetArg());
        break;

    case ALLJOYN_DOUBLE:
        arg = new MsgArg(sig);
        arg->v_double = StringToDouble(token.GetArg());
        break;

    case ALLJOYN_STRING:
        arg = new MsgArg("s", token.GetArg().c_str());
        break;

    case ALLJOYN_OBJECT_PATH:
        arg = new MsgArg("o", token.GetArg().c_str());
        break;

    case ALLJOYN_SIGNATURE:
        arg = new MsgArg("g", token.GetArg().c_str());
        break;

    case ALLJOYN_ARRAY:
        if (token.GetArg() == "[") {
            arg = ProcessArray(sigTokens, argTokens);
        } else {
            printf("Missing expected '[' grouping token.\n");
            exit(1);
        }
        break;

    case ALLJOYN_DICT_ENTRY_OPEN:
        if (token.GetArg() == "[") {
            arg = new MsgArg(ALLJOYN_DICT_ENTRY);
            arg->v_dictEntry.key = ParseCallArgToken(sigTokens, argTokens, GT_DICT);
            arg->v_dictEntry.val = ParseCallArgToken(sigTokens, argTokens, GT_DICT);
            // Verify and eat the closing ']'
            if (sigTokens.empty() || (sigTokens.front() != ALLJOYN_DICT_ENTRY_CLOSE) ||
                (ParseCallArgToken(sigTokens, argTokens, GT_DICT) != NULL)) {
                printf("A dict entry type may only have 2 complete types.\n");
                exit(1);
            }
        } else {
            printf("Missing expected '[' grouping token for dict entry.\n");
            exit(1);
        }
        break;

    case ALLJOYN_DICT_ENTRY_CLOSE:
        if (token.GetArg() != "]") {
            printf("Expected ']' grouping token for end of dict entry (got \"%s\").\n", token.GetArg().c_str());
            exit(1);
        }
        break;

    case ALLJOYN_STRUCT_OPEN:
        if (token.GetArg() == "[") {
            qcc::String ts("(");
            while (((ts.size() - 1) < sigTokens.size()) &&
                   (SignatureUtils::CountCompleteTypes(ts.c_str()) == 0)) {
                ts += sigTokens[ts.size() - 1];
            }
            if (SignatureUtils::CountCompleteTypes(ts.c_str()) == 1) {
                qcc::String structSig(ts.substr(1, ts.size() - 2));
                size_t numMembers = SignatureUtils::CountCompleteTypes(structSig.c_str());
                arg = new MsgArg(ALLJOYN_STRUCT);
                arg->v_struct.numMembers = numMembers;
                arg->v_struct.members = new MsgArg[numMembers];
                for (size_t i = 0; i < numMembers; ++i) {
                    MsgArg* a = ParseCallArgToken(sigTokens, argTokens, GT_STRUCT);
                    arg->v_struct.members[i] = *a;
                    delete a;
                }
                // Verify and eat the closing ']'
                if (sigTokens.empty() || (sigTokens.front() != ALLJOYN_STRUCT_CLOSE) ||
                    (ParseCallArgToken(sigTokens, argTokens, GT_STRUCT) != NULL)) {
                    printf("A dict entry type may only have 2 complete types.\n");
                    exit(1);
                }
            } else {
                printf("Could not determine number of complete types in struct (%s)\n",
                       ts.c_str());
                exit(1);
            }
        } else {
            printf("Missing expected '[' grouping token for struct.\n");
            exit(1);
        }
        break;


    case ALLJOYN_STRUCT_CLOSE:
        if (token.GetArg() != "]") {
            printf("Expected ']' grouping token for end of struct (got \"%s\").\n", token.GetArg().c_str());
            exit(1);
        }
        break;

    case ALLJOYN_VARIANT: {
            qcc::String vsig(token.GetArg());
            if (vsig[vsig.size() - 1] == ':') {
                vsig.resize(vsig.size() - 1);
                vsig.resize(1);
            }
            if (SignatureUtils::CountCompleteTypes(vsig.c_str()) != 1) {
                printf("Variant parameters must resolve to a single complete type.\n");
                exit(1);
            }
            deque<const Token*> vargTokens(argTokens.begin(), argTokens.end());
            deque<AllJoynTypeId> vsigTokens;
            for (qcc::String::const_iterator vs = vsig.begin(); vs != vsig.end(); ++vs) {
                vsigTokens.push_back(static_cast<AllJoynTypeId>(*vs));
            }
            arg = new MsgArg(sig);
            arg->v_variant.val = ParseCallArgToken(vsigTokens, vargTokens);
            assert(vsigTokens.empty());
            while (argTokens.size() > vargTokens.size()) {
                argTokens.pop_front();
            }
            break;
        }

    default:
        break;
    }

    return arg;
}


static void TokenizeCmdLineArgs(vector<Token>& tlist)
{
    qcc::String tok;
    for (int i = 0; i < callArgc; ++i) {
        size_t argLen = strlen(callArgv[i]);
        bool esc = false;
        tok.reserve(argLen + 1);
        tok.clear();
        for (char* t = callArgv[i]; t < (callArgv[i] + argLen); ++t) {
            if (esc) {
                esc = false;
                tok += *t;
            } else {
                if (*t == '\\') {
                    esc = true;
                } else if (*t == '[' || *t == ']') {
                    if (!tok.empty()) {
                        tlist.push_back(Token(Token::ARGUMENT, tok));
                        tok.clear();
                    }
                    tlist.push_back(Token(Token::GROUPING, qcc::String(t, 1)));
                } else {
                    tok += *t;
                }
            }
        }
        if (!tok.empty() || (argLen == 0)) {
            tlist.push_back(Token(Token::ARGUMENT, tok));
        }
    }
}


static void ParseMethodCallArgs(qcc::String signature, MsgArg* argList)
{
    deque<const Token*> argTokens;
    deque<AllJoynTypeId> sigTokens;
    vector<Token> tokenList;

    for (qcc::String::const_iterator sig = signature.begin(); sig != signature.end(); ++sig) {
        sigTokens.push_back(static_cast<AllJoynTypeId>(*sig));
    }

    TokenizeCmdLineArgs(tokenList);

    for (vector<Token>::const_iterator it = tokenList.begin(); it != tokenList.end(); ++it) {
        argTokens.push_back(&(*it));
    }

    while (!sigTokens.empty()) {
        MsgArg* arg = ParseCallArgToken(sigTokens, argTokens);
        if (arg) {
            *argList++ = *arg;
            delete arg;
        }
    }

    if (!argTokens.empty()) {
        printf("%u extra token%s do%s not match up with call signature.\n",
               static_cast<uint32_t>(argTokens.size()),
               argTokens.size() == 1 ? "" : "s",
               argTokens.size() == 1 ? "es" : "");
        exit(1);
    }
}


static QStatus MethodCall(BusAttachment& bus, bool introspect)
{
    QStatus status;
    ProxyBusObject robj(bus, dest.c_str(), objPath.c_str(), 0);
    const InterfaceDescription* ifc = NULL;
    const InterfaceDescription::Member* member;
    Message rsp(bus);
    MsgArg* argList;
    size_t numArgs;
    size_t ifcEnd = method.find_last_of('.');
    qcc::String ifcStr(method.substr(0, ifcEnd));
    qcc::String memberStr(method.substr(ifcEnd + 1));

    robj.AddInterface(org::freedesktop::DBus::InterfaceName);
    robj.AddInterface(org::freedesktop::DBus::Introspectable::InterfaceName);

    if (introspect) {
        status = robj.IntrospectRemoteObject();
        if (status != ER_OK) {
            goto exit;
        }

        ifc = robj.GetInterface(ifcStr.c_str());
        if (!ifc) {
            printf("Failed to lookup interface %s on %s\n",
                   ifcStr.c_str(), robj.GetPath().c_str());
            status = ER_FAIL;
            goto exit;
        }

    } else {
        InterfaceDescription* newifc = NULL;
        status = bus.CreateInterface(ifcStr.c_str(), newifc);
        if ((status == ER_OK) && newifc) {
            newifc->AddMember(MESSAGE_METHOD_CALL,
                              memberStr.c_str(),
                              signature.c_str(), "*",
                              NULL, 0);
            newifc->Activate();
            ifc = newifc;
        } else {
            if (status == ER_BUS_IFACE_ALREADY_EXISTS) {
                ifc = bus.GetInterface(ifcStr.c_str());
                if (ifc) {
                    status = ER_OK;
                }
            }
        }
        if ((status != ER_OK) || !ifc) {
            status = (status == ER_OK) ? ER_FAIL : status;
            printf("Interface definition failure: %s\n", QCC_StatusText(status));
            goto exit;
        }

        status = robj.AddInterface(*ifc);
        if (status != ER_OK) {
            printf("Failed to add interface %s to %s\n",
                   ifcStr.c_str(), robj.GetPath().c_str());
            goto exit;
        }
    }

    member = ifc->GetMember(memberStr.c_str());

    if (introspect) {
        signature = member->signature;
    }

    if (callArgc == 0) {
        if (!signature.empty()) {
            printf("Missing parameters for call signature: \"%s\"\n", signature.c_str());
            exit(1);
        }
    } else if (signature.empty()) {
        printf("No call signature for given parameters.\n");
        exit(1);
    }

    numArgs = SignatureUtils::CountCompleteTypes(signature.c_str());
    argList = new MsgArg[numArgs];
    ParseMethodCallArgs(signature, argList);

    printf("Calling %s with:\n%s\n", method.c_str(), MsgArg::ToString(argList, numArgs).c_str());

    status = robj.MethodCall(*member, argList, numArgs, rsp);
    if (status == ER_OK) {
        const MsgArg* data = rsp->GetArg(0);

        if (data) {
            printf("Reply:\n%s\n", data->ToString().c_str());
        } else {
            printf("No reply data\n");
        }
    } else {
        printf("Error:\n%s\n", rsp->ToString().c_str());
    }
    delete [] argList;

exit:
    return status;
}


static QStatus List(BusAttachment& bus)
{
    BusNameMap names;
    QStatus status = ListBusNames(bus, names);
    map<qcc::String, qcc::String> sorted;

    if (status == ER_OK) {
        BusNameMap::const_iterator it;
        it = names.end();
        --it;
        size_t width = it->first.size();
        for (it = names.begin(); it != names.end(); ++it) {
            if (!it->second.empty()) {
                sorted[it->second] = it->first;
            }
        }

        map<qcc::String, qcc::String>::const_iterator sit;
        for (sit = sorted.begin(); sit != sorted.end(); ++sit) {
            printf("%-*s   %s\n", static_cast<uint32_t>(width), sit->second.c_str(), sit->first.c_str());
        }

    }
    return status;
}


static QStatus ListAll(BusAttachment& bus)
{
    BusNameMap names;
    QStatus status = ListBusNames(bus, names);

    if (status == ER_OK) {
        BusNameMap::const_iterator it;
        it = names.end();
        --it;
        size_t width = it->first.size();

        for (it = names.begin(); it != names.end(); ++it) {
            if (it->second.empty()) {
                printf("%s\n", it->first.c_str());
            } else {
                printf("%-*s   %s\n", static_cast<uint32_t>(width), it->first.c_str(), it->second.c_str());
            }
        }
    }
    return status;
}



static QStatus ListObjects(BusAttachment& bus)
{
    ProxyBusObject robj(bus, dest.c_str(), "/", 0);
    robj.AddInterface(org::freedesktop::DBus::InterfaceName);
    robj.AddInterface(org::freedesktop::DBus::Introspectable::InterfaceName);
    return ListObjectPaths(bus, robj);
}


static QStatus Introspect(BusAttachment& bus)
{
    QStatus status;
    ProxyBusObject robj(bus, dest.c_str(), objPath.c_str(), 0);
    Message rsp(bus);

    robj.AddInterface(org::freedesktop::DBus::InterfaceName);
    robj.AddInterface(org::freedesktop::DBus::Introspectable::InterfaceName);
    status = robj.IntrospectRemoteObject();
    if (status == ER_OK) {
        size_t numIfaces = robj.GetInterfaces();
        const InterfaceDescription** ifaces = new const InterfaceDescription *[numIfaces];
        robj.GetInterfaces(ifaces, numIfaces);

        for (size_t i = 0; i < numIfaces; i++) {
            const InterfaceDescription* ifc = ifaces[i];
            size_t numMembers = ifc->GetMembers();
            const InterfaceDescription::Member** members = new const InterfaceDescription::Member *[numMembers];
            ifc->GetMembers(members, numMembers);

            for (size_t m = 0; m < numMembers; m++) {
                if (members[m]->memberType == MESSAGE_METHOD_CALL) {
                    const qcc::String inSig(members[m]->signature);
                    const qcc::String outSig(members[m]->returnSignature);
                    if (outSig.empty()) {
                        printf("METHOD: %s.%s(%s)\n", ifc->GetName(), members[m]->name.c_str(), inSig.c_str());
                    } else {
                        printf("METHOD: %s.%s(%s) -> %s\n", ifc->GetName(), members[m]->name.c_str(), inSig.c_str(), outSig.c_str());
                    }
                }
            }
            for (size_t m = 0; m < numMembers; m++) {
                if (members[m]->memberType == MESSAGE_SIGNAL) {
                    const qcc::String inSig(members[m]->signature);
                    printf("SIGNAL: %s.%s(%s)\n", ifc->GetName(), members[m]->name.c_str(), inSig.c_str());
                }
            }
            if (i != (numIfaces - 1)) {
                printf("\n");
            }
        }
        delete [] ifaces;
    } else {
        printf("Error: Failed to introspect %s at %s.\n", objPath.c_str(), dest.c_str());
    }

    return status;
}


static void VersionInfo(void)
{
    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());
}


static void Usage(void)
{
    printf("Usage: mcmd <options> [method call parameters...]\n"
           "\n"
           "    -h        Print this help message\n"
           "    -v        Print version information\n"
           "    -b <bus>  Use specified bus address\n"
           "    -d <dest> Specify bus destination (i.e. well known name)\n"
           "    -o <obj>  Specify object path\n"
           "    -l        List well known names on bus\n"
           "    -lo       List objects on an endpoint (requires -d)\n"
           "    -a        List all nodes on bus\n"
           "    -i        Introspect Object\n"
           "    -c <mcn>  Make method call to <mcn> (requires -d and -o)\n"
           "\n"
           "Method call parameters are prefixed by the parameter signature.  If the \"-i\"\n"
           "option is specified along with the \"-c\" option then the parameter signature\n"
           "must be omitted.  The \"-i\" option will automatically introspect the\n"
           "interface for the method call.  If a parameter signature includes the variant\n"
           "type, then the associated parameter value must be prefixed with the actual\n"
           "type (this applies to introspected interfaces as well).  The square brackets,\n"
           "'[' and ']', are used for grouping arrays, structs, and dict entries.  All\n"
           "parameters must be space separated.  Spaces around grouping tokens are\n"
           "optional.  Colons after the signatures are also optional.  Parameter examples\n"
           "are given below:\n"
           "\n"
           "aiai: [1 2 3] [4 5 6 7 8]\n"
           "aai: [[1 2 3] [4 5] [6 7 8 9]]\n"
           "\"a((ii)i):\" [[[123 456] 789] [[111 222] 333]]\n"
           "\"a{si}:\" [[one 1] [two 2] [three 3]]\n"
           "av: [s: \"hello world\" i: 42 o: /path/to/obj]\n"
           "\"a{sv}:\" [[key1 s: value1] [key2 i: 123] [key3 as: [one two three]]]\n"
           "\n"
           "NOTE: Shell quoting rules apply - e.g., \"hello world\" is 1 parameter, not 2.\n"
           );
}


static void ParseCmdLine(int argc, char** argv)
{
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0) {
            Usage();
            exit(0);
        } else if (strcmp(argv[i], "-v") == 0) {
            VersionInfo();
            exit(0);
        } else if (strcmp(argv[i], "-b") == 0) {
            ++i;
            if (i == argc) {
                printf("Option \"-b\" requires a bus specification\n");
                exit(1);
            }
            busAddr = argv[i];
        } else if (strcmp(argv[i], "-d") == 0) {
            ++i;
            if (i == argc) {
                printf("Option \"-d\" requires a bus node destination\n");
                exit(1);
            }
            dest = argv[i];
        } else if (strcmp(argv[i], "-o") == 0) {
            ++i;
            if (i == argc) {
                printf("Option \"-o\" requires an object path\n");
                exit(1);
            }
            objPath = argv[i];
        } else if (strcmp(argv[i], "-lo") == 0) {
            if (action != NO_ACTION) {
                printf("Only one of \"-f\", \"-l\", \"-lo\", \"-a\", \"-i\", \"-c\" may be used.\n");
                exit(1);
            }
            if (dest.empty()) {
                printf("Must specify a destination from which to list objects.\n");
                exit(1);
            }
            action = LIST_OBJECTS;
        } else if (strcmp(argv[i], "-l") == 0) {
            if (action != NO_ACTION) {
                printf("Only one of \"-f\", \"-l\", \"-lo\", \"-a\", \"-i\", \"-c\" may be used.\n");
                exit(1);
            }
            action = LIST;
        } else if (strcmp(argv[i], "-a") == 0) {
            if (action != NO_ACTION) {
                printf("Only one of \"-f\", \"-l\", \"-lo\", \"-a\", \"-i\", \"-c\" may be used.\n");
                exit(1);
            }
            action = LIST_ALL;
        } else if (strcmp(argv[i], "-i") == 0) {
            if (action != NO_ACTION) {
                printf("Only one of \"-f\", \"-l\", \"-lo\", \"-a\", \"-i\", \"-c\" may be used.\n");
                exit(1);
            }
            if (dest.empty()) {
                printf("Must specify a destination to introspect\n");
                exit(1);
            }
            action = INTROSPECT;
        } else if (strcmp(argv[i], "-c") == 0) {
            if ((action != NO_ACTION) && (action != INTROSPECT)) {
                printf("Cannot use \"-c\" with \"-l\", \"-lo\", \"-a\".\n");
                exit(1);
            }
            if (i == argc) {
                printf("Option \"-c\" requires an interface.method_name to call.\n");
                exit(1);
            }
            if (dest.empty() || objPath.empty()) {
                printf("Must specify a destination and object path implementing the method to call\n");
                exit(1);
            }
            ++i;
            method = argv[i];
            ++i;
            if (action == INTROSPECT) {
                action = METHOD_CALL_INTROSPECT;
            } else {
                action = METHOD_CALL;
                if (i < argc) {
                    signature = argv[i];
                    if (signature.find_last_of(':') == (signature.size() - 1)) {
                        // Strip off trailing ':'.  Signature will be validated later.
                        signature.resize(signature.size() - 1);
                    }
                    ++i;
                }
            }
            callArgc = argc - i;
            callArgv = &argv[i];
            i += callArgc;  // all remaining args are hereby consumed.
        } else {
            printf("Unkown command line argument: \"%s\"\n", argv[i]);
            exit(1);
        }
    }
}


/** Main entry point */
int main(int argc, char** argv)
{
    QStatus status = ER_OK;
    qcc::GUID128 guid;
    Environ* env;

    /* Parse command line args */
    ParseCmdLine(argc, argv);


    /* Create message bus */
    // Bus bus("mcmd", &BTTransportFactory);
    BusAttachment bus("mcmd");

#if !defined(QCC_OS_GROUP_WINDOWS) && !defined(QCC_OS_DARWIN)
    btTrans = new BTTransport(bus);
#endif

    /* Get env vars */
    if (busAddr.empty()) {
        env = Environ::GetAppEnviron();
        busAddr = env->Find("DBUS_SESSION_BUS_ADDRESS", "unix:abstract=alljoyn");
    }

    /* Start the msg bus */
    status = bus.Start();
    if (ER_OK != status) {
        QCC_LogError(status, ("BusAttachment::Start failed"));
        goto cleanup;
    }

    if (!busAddr.empty()) {
        /* Create the client-side endpoint */
        status = bus.Connect(busAddr.c_str());
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to connect"));
            goto cleanup;
        }
    }

    switch (action) {
    case NO_ACTION:
        break;

    case LIST:
        status = List(bus);
        break;

    case LIST_ALL:
        status = ListAll(bus);
        break;

    case LIST_OBJECTS:
        status = ListObjects(bus);
        break;

    case INTROSPECT:
        status = Introspect(bus);
        break;

    case METHOD_CALL:
    case METHOD_CALL_INTROSPECT:
        status = MethodCall(bus, action == METHOD_CALL_INTROSPECT);
        break;
    }

cleanup:
    return (int) status;
}
