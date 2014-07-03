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
#ifndef _HTTPSERVER_H
#define _HTTPSERVER_H

#include "Plugin.h"
#include <qcc/Socket.h>
#include <qcc/SocketStream.h>
#include <qcc/String.h>
#include <qcc/Thread.h>
#include <map>
#include <vector>
class HttpListenerNative;

namespace Http {
struct less {
    bool operator()(const qcc::String& a, const qcc::String& b) const {
        return strcasecmp(a.c_str(), b.c_str()) < 0;
    }
};
typedef std::map<qcc::String, qcc::String, less> Headers;
}

/*
 * Some care needs to be taken to ensure that a rogue entity cannot intercept the raw session data.
 * This consists of two parts: a shared secret (the request URI) that is transmitted over a secure
 * channel (the NPAPI interface), and an encrypted connection between the HTTP server and the
 * requestor.
 *
 * This means the rogue entity cannot sniff the generated URI, so the chance of them circumventing
 * the security is limited to brute-forcing a 256 bit key.
 *
 * TODO This second part is not yet implemented.
 */
class _HttpServer : public qcc::Thread, public qcc::ThreadListener {
  public:
    _HttpServer(Plugin& plugin);
    virtual ~_HttpServer();
    virtual void ThreadExit(qcc::Thread* thread);

    QStatus CreateObjectUrl(qcc::SocketFd fd, HttpListenerNative* httpListener, qcc::String& url);
    void RevokeObjectUrl(const qcc::String& url);

    class ObjectUrl {
      public:
        qcc::SocketFd fd;
        HttpListenerNative* httpListener;
        ObjectUrl() : fd(qcc::INVALID_SOCKET_FD), httpListener(NULL) { }
        ObjectUrl(qcc::SocketFd fd, HttpListenerNative* httpListener) : fd(fd), httpListener(httpListener) { }
    };
    ObjectUrl GetObjectUrl(const qcc::String& requestUri);
    void SendResponse(qcc::SocketStream& stream, uint16_t status, qcc::String& statusText, Http::Headers& responseHeaders, qcc::SocketFd fd);

  protected:
    virtual qcc::ThreadReturn STDCALL Run(void* arg);

  private:
    class RequestThread : public qcc::Thread {
      public:
        RequestThread(_HttpServer* httpServer, qcc::SocketFd fd);
        virtual ~RequestThread() { }
      protected:
        virtual qcc::ThreadReturn STDCALL Run(void* arg);
      private:
        _HttpServer* httpServer;
        qcc::SocketStream stream;
    };
    class ResponseThread : public qcc::Thread {
      public:
        ResponseThread(_HttpServer* httpServer, qcc::SocketStream& stream, uint16_t status, qcc::String& statusText, Http::Headers& responseHeaders, qcc::SocketFd sessionFd);
        virtual ~ResponseThread() { }
      protected:
        virtual qcc::ThreadReturn STDCALL Run(void* arg);
      private:
        _HttpServer* httpServer;
        qcc::SocketStream stream;
        uint16_t status;
        qcc::String statusText;
        Http::Headers responseHeaders;
        qcc::SocketFd sessionFd;
    };

    Plugin plugin;
    qcc::String origin;
    std::map<qcc::String, ObjectUrl> objectUrls;
    qcc::Mutex lock;
    std::vector<qcc::Thread*> threads;

    QStatus Start();
};

typedef qcc::ManagedObj<_HttpServer> HttpServer;

#endif
