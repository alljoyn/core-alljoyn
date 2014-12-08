/**
 * @file
 * ClientTransport is an implementation of Transport that listens
 * on an AF_UNIX socket.
 */

/******************************************************************************
 * Copyright (c) 2009-2014, AllSeen Alliance. All rights reserved.
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

#include <list>

#include <errno.h>
#include <qcc/Socket.h>
#include <qcc/SocketStream.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>

#include <sys/un.h>
#if defined(QCC_OS_DARWIN)
#include <sys/ucred.h>
#endif

#include <alljoyn/BusAttachment.h>

#include "BusInternal.h"
#include "RemoteEndpoint.h"
#include "Router.h"
#include "ClientTransport.h"

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;

namespace ajn {
class _ClientEndpoint;
typedef qcc::ManagedObj<_ClientEndpoint> ClientEndpoint;
/*
 * The name of this transport
 */
const char* ClientTransport::TransportName = "unix";

class _ClientEndpoint : public _RemoteEndpoint {
  public:
    /* Unix endpoint constructor */
    _ClientEndpoint(BusAttachment& bus, bool incoming, const qcc::String connectSpec, SocketFd sock) :
        _RemoteEndpoint(bus, incoming, connectSpec, &stream, ClientTransport::TransportName),
        processId(-1),
        stream(sock)
    {
    }

    /* Destructor */
    virtual ~_ClientEndpoint() { }

    /**
     * Set the process id of the endpoint.
     *
     * @param   processId   Process ID number.
     */
    void SetProcessId(uint32_t processId) { this->processId = processId; }

    /**
     * Return the process id of the endpoint.
     *
     * @return  Process ID number.
     */
    uint32_t GetProcessId() const { return processId; }

    /**
     * Indicates if the endpoint supports reporting UNIX style user, group, and process IDs.
     *
     * @return  'true' if UNIX IDs supported, 'false' if not supported.
     */
    bool SupportsUnixIDs() const { return true; }


  private:
    uint32_t processId;
    SocketStream stream;
};

QStatus ClientTransport::NormalizeTransportSpec(const char* inSpec, qcc::String& outSpec, map<qcc::String, qcc::String>& argMap) const
{
    /*
     * Take the string in inSpec, which must start with "unix:" and parse it,
     * looking for comma-separated "key=value" pairs and initialize the
     * argMap with those pairs.
     */
    QStatus status = ParseArguments("unix", inSpec, argMap);
    if (status != ER_OK) {
        return status;
    }

    qcc::String path = Trim(argMap["path"]);
    qcc::String abstract = Trim(argMap["abstract"]);
    if (ER_OK == status) {
        // @@ TODO: Path normalization?
        outSpec = "unix:";
        if (!path.empty()) {
            outSpec.append("path=");
            outSpec.append(path);
            argMap["_spec"] = path;
        } else if (!abstract.empty()) {
            outSpec.append("abstract=");
            outSpec.append(abstract);
            argMap["_spec"] = qcc::String("@") + abstract;
        } else {
            status = ER_BUS_BAD_TRANSPORT_ARGS;
        }
    }

    return status;
}

static QStatus SendSocketCreds(SocketFd sockFd, uid_t uid, gid_t gid, pid_t pid)
{
    int enableCred = 1;
    int rc = setsockopt(sockFd, SOL_SOCKET, SO_PASSCRED, &enableCred, sizeof(enableCred));
    if (rc == -1) {
        QCC_LogError(ER_OS_ERROR, ("ClientTransport(): setsockopt(SO_PASSCRED) failed"));
        qcc::Close(sockFd);
        return ER_OS_ERROR;
    }

    /*
     * Compose a header that includes the local user credentials and a single NUL byte.
     */
    ssize_t ret;
    char nulbuf = 0;
    struct cmsghdr* cmsg;
    struct ucred* cred;
    struct iovec iov[] = { { &nulbuf, sizeof(nulbuf) } };
    char cbuf[CMSG_SPACE(sizeof(struct ucred))];
    ::memset(cbuf, 0, sizeof(cbuf));
    struct msghdr msg;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = ArraySize(iov);
    msg.msg_control = cbuf;
    msg.msg_controllen = ArraySize(cbuf);
    msg.msg_flags = 0;

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_CREDENTIALS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(struct ucred));
    cred = reinterpret_cast<struct ucred*>(CMSG_DATA(cmsg));
    cred->uid = uid;
    cred->gid = gid;
    cred->pid = pid;

    QCC_DbgHLPrintf(("Sending UID: %u  GID: %u  PID %u", cred->uid, cred->gid, cred->pid));

    ret = sendmsg(sockFd, &msg, 0);
    if (ret != 1) {
        return ER_OS_ERROR;
    }

    /*
     * If we don't disable this every read will have credentials which adds overhead if have
     * enabled unix file descriptor passing.
     */
    int disableCred = 0;
    rc = setsockopt(sockFd, SOL_SOCKET, SO_PASSCRED, &disableCred, sizeof(disableCred));
    if (rc == -1) {
        QCC_LogError(ER_OS_ERROR, ("ClientTransport(): setsockopt(SO_PASSCRED) failed"));
    }

    return ER_OK;
}

QStatus ClientTransport::Connect(const char* connectArgs, const SessionOpts& opts, BusEndpoint& newep)
{
    if (!m_running) {
        return ER_BUS_TRANSPORT_NOT_STARTED;
    }
    if (m_endpoint->IsValid()) {
        return ER_BUS_ALREADY_CONNECTED;
    }

    /*
     * Parse and normalize the connectArgs.  For a client or service, there are
     * no reasonable defaults and so the addr and port keys MUST be present or
     * an error is returned.
     */
    QStatus status;
    qcc::String normSpec;
    map<qcc::String, qcc::String> argMap;
    status = ClientTransport::NormalizeTransportSpec(connectArgs, normSpec, argMap);
    if (ER_OK != status) {
        QCC_LogError(status, ("ClientTransport::Connect(): Invalid Unix connect spec \"%s\"", connectArgs));
        return status;
    }
    /*
     * Attempt to connect to the endpoint specified in the connectSpec.
     */
    SocketFd sockFd = qcc::INVALID_SOCKET_FD;
    status = Socket(QCC_AF_UNIX, QCC_SOCK_STREAM, sockFd);
    if (status != ER_OK) {
        QCC_LogError(status, ("ClientTransport(): socket Create() failed"));
        return status;
    }
    /*
     * Got a socket, now Connect() to it.
     */
    qcc::String& spec = argMap["_spec"];
    status = qcc::Connect(sockFd, spec.c_str());
    if (status != ER_OK) {
        QCC_DbgHLPrintf(("ClientTransport(): socket Connect(%d, %s) failed: %s", sockFd, spec.c_str(), QCC_StatusText(status)));
        qcc::Close(sockFd);
        return status;
    }

    status = SendSocketCreds(sockFd, GetUid(), GetGid(), GetPid());
    static const bool falsiness = false;
    ClientEndpoint ep = ClientEndpoint(m_bus, falsiness, normSpec, sockFd);

    /* Initialized the features for this endpoint */
    ep->GetFeatures().isBusToBus = false;
    ep->GetFeatures().allowRemote = m_bus.GetInternal().AllowRemoteMessages();
    ep->GetFeatures().handlePassing = true;

    qcc::String authName;
    qcc::String redirection;
    status = ep->Establish("EXTERNAL", authName, redirection);
    if (status == ER_OK) {
        ep->SetListener(this);
        status = ep->Start();
        if (status != ER_OK) {
            QCC_LogError(status, ("ClientTransport::Connect(): Start ClientEndpoint failed"));
        }
    }
    /*
     * If we got an error, we need to cleanup the socket and zero out the
     * returned endpoint.  If we got this done without a problem, we return
     * a pointer to the new endpoint. We do not close the socket since the
     * endpoint that was created is responsible for doing so.
     */
    if (status != ER_OK) {
        ep->Invalidate();
    } else {
        newep = BusEndpoint::cast(ep);
        m_endpoint = RemoteEndpoint::cast(ep);
    }

    return status;
}

} // namespace ajn
