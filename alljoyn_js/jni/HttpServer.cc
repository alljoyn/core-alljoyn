/*
 * Copyright (c) 2011-2013, AllSeen Alliance. All rights reserved.
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
 */
#include "HttpServer.h"

#include "HttpListenerNative.h"
#include "PluginData.h"
#include <qcc/Debug.h>
#include <qcc/SocketStream.h>
#include <qcc/StringUtil.h>
#include <qcc/time.h>
#include <algorithm>

#define QCC_MODULE "ALLJOYN_JS"

static const uint8_t hex[] = "0123456789ABCDEF";

static const size_t maxData = 8192;
static const size_t maxHdr = 8;

static void ParseRequest(const qcc::String& line, qcc::String& method, qcc::String& requestUri, qcc::String& httpVersion)
{
    size_t pos = 0;
    size_t begin = 0;
    do {
        size_t sp = line.find_first_of(' ', begin);
        if (qcc::String::npos == sp) {
            sp = line.size();
        }
        qcc::String token = line.substr(begin, sp - begin);
        switch (pos++) {
        case 0:
            method = token;
            break;

        case 1:
            requestUri = token;
            break;

        case 2:
            httpVersion = token;
            break;

        default:
            break;
        }
        begin = sp + 1;
    } while (begin < line.size());
}

static QStatus PushBytes(qcc::SocketStream& stream, const char* buf, size_t numBytes)
{
    /*
     * TODO It looks like PushBytes on SocketStream is not guaranteed to send all the bytes,
     * but it also looks like our existing code relies on that.  What am I missing?
     */
    QStatus status = ER_OK;
    size_t numSent = 0;
    for (size_t pos = 0; pos < numBytes; pos += numSent) {
        status = stream.PushBytes(&buf[pos], numBytes - pos, numSent);
        if (ER_OK != status) {
            QCC_LogError(status, ("PushBytes failed"));
            break;
        }
    }
    return status;
}

static QStatus SendBadRequestResponse(qcc::SocketStream& stream)
{
    qcc::String response = "HTTP/1.1 400 Bad Request\r\n";
    QStatus status = PushBytes(stream, response.data(), response.size());
    if (ER_OK == status) {
        QCC_DbgTrace(("[%d] %s", stream.GetSocketFd(), response.c_str()));
    }
    return status;
}

static QStatus SendNotFoundResponse(qcc::SocketStream& stream)
{
    qcc::String response = "HTTP/1.1 404 Not Found\r\n";
    QStatus status = PushBytes(stream, response.data(), response.size());
    if (ER_OK == status) {
        QCC_DbgTrace(("[%d] %s", stream.GetSocketFd(), response.c_str()));
    }
    return status;
}

_HttpServer::RequestThread::RequestThread(_HttpServer* httpServer, qcc::SocketFd requestFd) :
    httpServer(httpServer),
    stream(requestFd)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    stream.SetSendTimeout(qcc::Event::WAIT_FOREVER);
}

class OnRequestContext : public PluginData::CallbackContext {
  public:
    Plugin plugin;
    HttpServer httpServer;
    qcc::String requestUri;
    Http::Headers requestHeaders;
    qcc::SocketStream stream;
    qcc::SocketFd sessionFd;
    OnRequestContext(Plugin& plugin, _HttpServer* httpServer, qcc::String& requestUri, Http::Headers& requestHeaders, qcc::SocketStream& stream, qcc::SocketFd sessionFd) :
        plugin(plugin),
        httpServer(HttpServer::wrap(httpServer)),
        requestUri(requestUri),
        requestHeaders(requestHeaders),
        stream(stream),
        sessionFd(sessionFd) { }
};

static void _OnRequest(PluginData::CallbackContext* ctx)
{
    OnRequestContext* context = static_cast<OnRequestContext*>(ctx);
    HttpListenerNative* httpListener = context->httpServer->GetObjectUrl(context->requestUri).httpListener;
    if (httpListener) {
        HttpRequestHost httpRequest(context->plugin, context->httpServer, context->requestHeaders, context->stream, context->sessionFd);
        httpListener->onRequest(httpRequest);
    } else {
        /* Send the default response */
        uint16_t status = 200;
        qcc::String statusText = "OK";
        Http::Headers responseHeaders;
        responseHeaders["Date"] = qcc::UTCTime();
        responseHeaders["Content-Type"] = "application/octet-stream";
        context->httpServer->SendResponse(context->stream, status, statusText, responseHeaders, context->sessionFd);
    }
}

qcc::ThreadReturn STDCALL _HttpServer::RequestThread::Run(void* arg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    qcc::String line;
    QStatus status;
    qcc::String method, requestUri, httpVersion;
    qcc::SocketFd sessionFd = qcc::INVALID_SOCKET_FD;
    Http::Headers requestHeaders;
    PluginData::Callback callback(httpServer->plugin, _OnRequest);

    status = stream.GetLine(line);
    if (ER_OK != status) {
        SendBadRequestResponse(stream);
        goto exit;
    }

    QCC_DbgTrace(("[%d] %s", stream.GetSocketFd(), line.c_str()));
    ParseRequest(line, method, requestUri, httpVersion);
    if (method != "GET") {
        SendBadRequestResponse(stream);
        goto exit;
    }

    sessionFd = httpServer->GetObjectUrl(requestUri).fd;
    if (qcc::INVALID_SOCKET_FD == sessionFd) {
        SendNotFoundResponse(stream);
        goto exit;
    }

    /*
     * Read (and discard) the rest of the request headers.
     */
    while ((ER_OK == status) && !line.empty()) {
        line.clear();
        status = stream.GetLine(line);
        if (ER_OK == status) {
            size_t begin;
            size_t pos = 0;
            while ((pos < line.size()) && isspace(line[pos]))
                ++pos;
            begin = pos;
            while ((pos < line.size()) && !isspace(line[pos]) && (':' != line[pos]))
                ++pos;
            qcc::String header = line.substr(begin, pos - begin);
            while ((pos < line.size()) && (isspace(line[pos]) || (':' == line[pos])))
                ++pos;
            begin = pos;
            qcc::String value = line.substr(begin);
            QCC_DbgTrace(("[%d] %s", stream.GetSocketFd(), line.c_str()));
            if (!header.empty() || !value.empty()) {
                requestHeaders[header] = value;
            }
        }
    }
    if (ER_OK != status) {
        SendBadRequestResponse(stream);
        goto exit;
    }

    callback->context = new OnRequestContext(httpServer->plugin, httpServer, requestUri, requestHeaders, stream, sessionFd);
    PluginData::DispatchCallback(callback);

exit:
    if (ER_OK != status) {
        QCC_LogError(status, ("Request thread exiting"));
    }
    return 0;
}

_HttpServer::ResponseThread::ResponseThread(_HttpServer* httpServer, qcc::SocketStream& stream, uint16_t status, qcc::String& statusText, Http::Headers& responseHeaders, qcc::SocketFd sessionFd) :
    httpServer(httpServer),
    stream(stream),
    status(status),
    statusText(statusText),
    responseHeaders(responseHeaders),
    sessionFd(sessionFd)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

qcc::ThreadReturn STDCALL _HttpServer::ResponseThread::Run(void* arg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    qcc::String response;
    QStatus sts;
    char* buffer = new char[maxHdr + maxData + 2];

    response = "HTTP/1.1 " + qcc::U32ToString(status) + " " + statusText + "\r\n";
    for (Http::Headers::iterator it = responseHeaders.begin(); it != responseHeaders.end(); ++it) {
        response += it->first + ": " + it->second + "\r\n";
    }
    response += "\r\n";
    sts = PushBytes(stream, response.data(), response.size());
    if (ER_OK != sts) {
        goto exit;
    }
    QCC_DbgTrace(("[%d] %s", stream.GetSocketFd(), response.c_str()));

    /*
     * Now pump out data.
     */
    while (ER_OK == sts) {
        size_t received = 0;
        char* ptr = &buffer[maxHdr];

        sts = qcc::Recv(sessionFd, ptr, maxData, received);
        if (ER_OK == sts) {
            if (0 == received) {
                if (responseHeaders["Transfer-Encoding"] == "chunked") {
                    sts = PushBytes(stream, "0\r\n", 3);
                    if (ER_OK != sts) {
                        QCC_LogError(sts, ("PushBytes failed"));
                    }
                }
                sts = ER_SOCK_OTHER_END_CLOSED;
                QCC_LogError(sts, ("Recv failed"));
            } else {
                size_t len;
                if (responseHeaders["Transfer-Encoding"] == "chunked") {
                    /* Chunk length header is ascii hex followed by cr-lf */
                    *(--ptr) = '\n';
                    *(--ptr) = '\r';
                    len = 2;
                    size_t n = received;
                    do {
                        *(--ptr) = hex[n & 0xF];
                        n >>= 4;
                        ++len;
                    } while (n);
                    /* Chunk is terminated with cr-lf */
                    len += received;
                    ptr[len++] = '\r';
                    ptr[len++] = '\n';
                } else {
                    len = received;
                }
                sts = PushBytes(stream, ptr, len);
                if (ER_OK != sts) {
                    QCC_LogError(sts, ("PushBytes failed"));
                }
            }
        } else if (ER_WOULDBLOCK == sts) {
            qcc::Event recvEvent(sessionFd, qcc::Event::IO_READ);
            sts = qcc::Event::Wait(recvEvent);
            if (ER_OK != sts) {
                QCC_LogError(sts, ("Wait failed"));
            }
        } else {
            QCC_LogError(sts, ("Recv failed"));
        }
    }

exit:
    delete[] buffer;
    if (ER_OK != sts) {
        QCC_LogError(sts, ("Response thread exiting"));
    }
    return 0;
}

_HttpServer::_HttpServer(Plugin& plugin) :
    plugin(plugin)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

_HttpServer::~_HttpServer()
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    lock.Lock();
    std::vector<qcc::Thread*>::iterator tit;
    for (tit = threads.begin(); tit != threads.end(); ++tit) {
        (*tit)->Stop();
    }
    lock.Unlock();
    Stop();

    lock.Lock();
    while (threads.size() > 0) {
        lock.Unlock();
        qcc::Sleep(50);
        lock.Lock();
    }
    lock.Unlock();
    Join();

    std::map<qcc::String, ObjectUrl>::iterator oit;
    for (oit = objectUrls.begin(); oit != objectUrls.end(); ++oit) {
        QCC_DbgTrace(("Removed %s -> %d", oit->first.c_str(), oit->second.fd));
        qcc::Close(oit->second.fd);
        delete oit->second.httpListener;
    }

    QCC_DbgTrace(("-%s", __FUNCTION__));
}

void _HttpServer::ThreadExit(qcc::Thread* thread)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    RequestThread* requestThread = static_cast<RequestThread*>(thread);
    lock.Lock();
    std::vector<qcc::Thread*>::iterator it = std::find(threads.begin(), threads.end(), requestThread);
    if (it != threads.end()) {
        threads.erase(it);
    }
    lock.Unlock();
    delete requestThread;
}

QStatus _HttpServer::CreateObjectUrl(qcc::SocketFd fd, HttpListenerNative* httpListener, qcc::String& url)
{
    QCC_DbgTrace(("%s(fd=%d)", __FUNCTION__, fd));

    QStatus status;
    qcc::SocketFd sessionFd = qcc::INVALID_SOCKET_FD;
    qcc::String requestUri;

    status = Start();
    if (ER_OK != status) {
        goto exit;
    }
    status = qcc::SocketDup(fd, sessionFd);
    if (ER_OK != status) {
        QCC_LogError(status, ("SocketDup failed"));
        goto exit;
    }

    requestUri = "/" + qcc::RandHexString(256);

    lock.Lock();
    objectUrls[requestUri] = ObjectUrl(sessionFd, httpListener);
    lock.Unlock();
    QCC_DbgTrace(("Added %s -> %d", requestUri.c_str(), sessionFd));

    url = origin + requestUri;

exit:
    if (ER_OK != status) {
        if (qcc::INVALID_SOCKET_FD != sessionFd) {
            qcc::Close(sessionFd);
            delete httpListener;
        }
    }
    return status;
}

void _HttpServer::RevokeObjectUrl(const qcc::String& url)
{
    QCC_DbgTrace(("%s(url=%s)", __FUNCTION__, url.c_str()));

    qcc::SocketFd sessionFd = qcc::INVALID_SOCKET_FD;
    HttpListenerNative* httpListener = NULL;
    qcc::String requestUri = url.substr(url.find_last_of('/'));

    lock.Lock();
    std::map<qcc::String, ObjectUrl>::iterator it = objectUrls.find(requestUri);
    if (it != objectUrls.end()) {
        sessionFd = it->second.fd;
        httpListener = it->second.httpListener;
        objectUrls.erase(it);
    }
    lock.Unlock();
    QCC_DbgTrace(("Removed %s -> %d", requestUri.c_str(), sessionFd));

    if (qcc::INVALID_SOCKET_FD != sessionFd) {
        qcc::Close(sessionFd);
        delete httpListener;
    }
}

QStatus _HttpServer::Start()
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    if (IsStopping()) {
        return ER_THREAD_STOPPING;
    } else if (IsRunning()) {
        return ER_OK;
    }

    QStatus status;
    qcc::SocketFd listenFd = qcc::INVALID_SOCKET_FD;
    qcc::IPAddress localhost("127.0.0.1");
    uint16_t listenPort = 0;

    status = qcc::Socket(qcc::QCC_AF_INET, qcc::QCC_SOCK_STREAM, listenFd);
    if (ER_OK != status) {
        QCC_LogError(status, ("Socket failed"));
        goto exit;
    }
    status = qcc::Bind(listenFd, localhost, listenPort);
    if (ER_OK != status) {
        QCC_LogError(status, ("Find failed"));
        goto exit;
    }
    status = qcc::GetLocalAddress(listenFd, localhost, listenPort);
    if (ER_OK != status) {
        QCC_LogError(status, ("GetLocalAddress failed"));
        goto exit;
    }
    status = qcc::Listen(listenFd, SOMAXCONN);
    if (ER_OK != status) {
        QCC_LogError(status, ("Listen failed"));
        goto exit;
    }
    status = qcc::SetBlocking(listenFd, false);
    if (ER_OK != status) {
        QCC_LogError(status, ("SetBlocking(false) failed"));
        goto exit;
    }
    status = qcc::Thread::Start(reinterpret_cast<void*>(listenFd));
    if (ER_OK != status) {
        QCC_LogError(status, ("Start failed"));
        goto exit;
    }

    origin = "http://" + localhost.ToString() + ":" + qcc::U32ToString(listenPort);
    QCC_DbgTrace(("%s", origin.c_str()));

exit:
    if (ER_OK != status) {
        if (qcc::INVALID_SOCKET_FD != listenFd) {
            qcc::Close(listenFd);
        }
    }
    return status;
}

_HttpServer::ObjectUrl _HttpServer::GetObjectUrl(const qcc::String& requestUri)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    lock.Lock();
    ObjectUrl objectUrl;
    std::map<qcc::String, ObjectUrl>::iterator it = objectUrls.find(requestUri);
    if (it != objectUrls.end()) {
        objectUrl = it->second;
    }
    lock.Unlock();
    return objectUrl;
}

void _HttpServer::SendResponse(qcc::SocketStream& stream, uint16_t status, qcc::String& statusText, Http::Headers& responseHeaders, qcc::SocketFd fd)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    ResponseThread* responseThread = new ResponseThread(this, stream, status, statusText, responseHeaders, fd);
    QStatus sts = responseThread->Start(NULL, this);
    if (ER_OK == sts) {
        lock.Lock();
        threads.push_back(responseThread);
        lock.Unlock();
    } else {
        QCC_LogError(sts, ("Start response thread failed"));
        delete responseThread;
    }
}

qcc::ThreadReturn STDCALL _HttpServer::Run(void* arg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    qcc::SocketFd listenFd = reinterpret_cast<intptr_t>(arg);
    QStatus status = ER_OK;

    while (!IsStopping()) {
        qcc::SocketFd requestFd;
        do {
            qcc::IPAddress addr;
            uint16_t remotePort;
            status = qcc::Accept(listenFd, addr, remotePort, requestFd);
            if (ER_OK == status) {
                break;
            } else if (ER_WOULDBLOCK == status) {
                qcc::Event listenEvent(listenFd, qcc::Event::IO_READ);
                status = qcc::Event::Wait(listenEvent);
            } else {
                QCC_LogError(status, ("Accept failed"));
                status = ER_OK;
            }
        } while (ER_OK == status);
        if (ER_OK != status) {
            /*
             * qcc:Event::Wait returned an error.  This means the thread is stopping or was alerted
             * or the underlying platform-specific wait failed.  In any case we'll just try again.
             */
            QCC_LogError(status, ("Wait failed"));
            continue;
        }

        RequestThread* requestThread = new RequestThread(this, requestFd);
        status = requestThread->Start(NULL, this);
        if (ER_OK == status) {
            lock.Lock();
            threads.push_back(requestThread);
            lock.Unlock();
        } else {
            QCC_LogError(status, ("Start request thread failed"));
            delete requestThread;
            continue;
        }
    }

    QCC_DbgTrace(("%s exiting", __FUNCTION__));
    qcc::Close(listenFd);
    return 0;
}
