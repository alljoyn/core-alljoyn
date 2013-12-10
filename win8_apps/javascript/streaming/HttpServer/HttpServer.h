/**
 * @file
 * This file defines a class that emualates an HTTP media server.
 */

// ****************************************************************************
// Copyright (c) 2012, AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
// ******************************************************************************

#pragma once

#include <qcc/platform.h>
#include <SocketStream.h>
#include <qcc/Mutex.h>
#include <assert.h>

namespace AllJoyn {

class _HttpServer : public qcc::ThreadListener {
  public:
    _HttpServer();
    ~_HttpServer();

    QStatus Start();
    void CreateObjectURL(SocketStream ^ sock, Platform::WriteOnlyArray<Platform::String ^> ^ url);
    void RevokeObjectURL(Platform::String ^ url);

  private:
    void HttpSocketConnectionReceived(Windows::Networking::Sockets::StreamSocketListener ^ sender, Windows::Networking::Sockets::StreamSocketListenerConnectionReceivedEventArgs ^ args);
    virtual void ThreadExit(qcc::Thread* thread);
    SocketStream ^ GetSessionFd(const qcc::String & requestUri);

    class RequestThread : public qcc::Thread {
      public:
        RequestThread(_HttpServer* httpServer, Windows::Networking::Sockets::StreamSocket ^ httpSocket);
        virtual ~RequestThread();

      protected:
        virtual qcc::ThreadReturn STDCALL Run(void* arg);

      private:
        _HttpServer* httpServer;
        Windows::Networking::Sockets::StreamSocket ^ httpSocket;
        Windows::Storage::Streams::DataReader ^ dataReader;
        Windows::Storage::Streams::DataWriter ^ dataWriter;
        SocketStream ^ sessSock;
        uint32_t length;
        uint8_t* dataBuf;
        const static size_t maxData = 8192;
        const static size_t maxHdr = 10;

        QStatus PushBytes(uint8_t* buf, size_t numBytes, size_t& sent);
        void LoadRequest();
        QStatus GetLine(qcc::String& line);
        QStatus SendBadRequestResponse();
        QStatus SendNotFoundResponse();
    };

    std::map<qcc::String, SocketStream ^> _uriSockMap;
    qcc::Mutex _lock;
    std::vector<RequestThread*> _requestThreads;
    bool _running;
    Windows::Foundation::EventRegistrationToken _evtToken;          /**< The token used to remove ConnectionRequested event handler */
    Windows::Networking::Sockets::StreamSocketListener ^ _httpListener;
    uint16_t _listenPort;
    qcc::String _origin;
};

/// <summary> A basic HTTP server that will listen for and respond to HTTP streaming requests </summary>
public ref class HttpServer sealed {

  public:
    HttpServer() {
        _httpServer = new _HttpServer();
        assert(_httpServer);
    }

	Windows::Foundation::IAsyncAction ^ StartAsync() {
        Windows::Foundation::IAsyncAction ^ action = concurrency::create_async([ = ]() {
			_httpServer->Start();
		});
		return action;
	}

    void CreateObjectURL(SocketStream ^ sock, Platform::WriteOnlyArray<Platform::String ^> ^ url) {
        if (_httpServer) {
            _httpServer->CreateObjectURL(sock, url);
        }
    }

    void RevokeObjectURL(Platform::String ^ url) {
        if (_httpServer) {
            _httpServer->RevokeObjectURL(url);
        }
    }

  private:
    ~HttpServer() {
        delete _httpServer;
        _httpServer = NULL;
    }
    _HttpServer* _httpServer;
};


public ref class StreamSourceHost sealed {
  public:
    static void Send(SocketStream ^ socket, Windows::Storage::StorageFile ^ file);
};

}
