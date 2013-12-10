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

#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/time.h>
#include <qcc/winrt/utility.h>
#include <qcc/winrt/exception.h>
#include <ppltasks.h>
#include <qcc/Debug.h>

#include <HttpServer.h>
#include <Status.h>

#define QCC_MODULE "HTTP_SERVER"

namespace AllJoyn {

using namespace Windows::System::Threading;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;
using namespace Windows::Storage;
using namespace Windows::Foundation;
using namespace concurrency;

using namespace std;
using namespace qcc;

static const uint8_t hex[] = "0123456789ABCDEF";
const size_t maxData = 8192;
const size_t maxHdr = 10;

_HttpServer::_HttpServer() : _running(false)
{
};

_HttpServer::~_HttpServer()
{
    delete _httpListener;
    _httpListener = nullptr;
    _uriSockMap.clear();
    _lock.Lock();
    std::vector<RequestThread*>::iterator it = _requestThreads.begin();
    for (; it != _requestThreads.end(); it++) {
        (*it)->Stop();
    }
    _lock.Unlock();
    _running = false;

    _lock.Lock();
    while (_requestThreads.size() > 0) {
        _lock.Unlock();
        qcc::Sleep(500);
        _lock.Lock();
        continue;
    }
    _lock.Unlock();
}

void _HttpServer::HttpSocketConnectionReceived(StreamSocketListener ^ sender, StreamSocketListenerConnectionReceivedEventArgs ^ args)
{
    QCC_DbgPrintf(("_HttpServer::HttpSocketConnectionReceived()"));

    RequestThread* requestThread = new RequestThread(this, args->Socket);
    QStatus status = requestThread->Start(NULL, this);

    if (ER_OK == status) {
        _lock.Lock();
        _requestThreads.push_back(requestThread);
        _lock.Unlock();
    } else {
        QCC_LogError(status, ("Start request thread failed"));
        delete requestThread;
    }
}

QStatus _HttpServer::Start()
{
    QStatus status = ER_OK;
    QCC_DbgHLPrintf(("_HttpServer::Start"));

    if (_running) {
        return status;
    }

    _running = true;
    _httpListener = ref new StreamSocketListener();
    TypedEventHandler<StreamSocketListener ^, StreamSocketListenerConnectionReceivedEventArgs ^> ^ connectionHandler = ref new TypedEventHandler<StreamSocketListener ^, StreamSocketListenerConnectionReceivedEventArgs ^>(
        [ = ] (StreamSocketListener ^ sender, StreamSocketListenerConnectionReceivedEventArgs ^ args) {
            HttpSocketConnectionReceived(sender, args);
        });

    _evtToken = _httpListener->ConnectionReceived += connectionHandler;
    IAsyncAction ^ op = _httpListener->BindServiceNameAsync("");
    task<void> bindTask(op);
    bindTask.wait();
    _listenPort = _wtoi(_httpListener->Information->LocalPort->Data());
    _origin = "http://127.0.0.1:" + qcc::U32ToString(_listenPort);
    return status;
}

void _HttpServer::CreateObjectURL(SocketStream ^ sock, Platform::WriteOnlyArray<Platform::String ^> ^ url)
{
    QCC_DbgTrace(("%s()", __FUNCTION__));

    QStatus status = ER_OK;

    qcc::String requestUri;

    requestUri = "/" + qcc::RandHexString(32);

    _lock.Lock();
    _uriSockMap[requestUri] = sock;
    _lock.Unlock();
    QCC_DbgTrace(("Added %s", requestUri.c_str()));
    qcc::String fullUrl = _origin + requestUri;
    url[0] = MultibyteToPlatformString(fullUrl.c_str());
}

void _HttpServer::RevokeObjectURL(Platform::String ^ url)
{
    qcc::String urlStr = PlatformToMultibyteString(url);
    QCC_DbgTrace(("%s(url=%s)", __FUNCTION__, urlStr.c_str()));

    qcc::String requestUri = urlStr.substr(urlStr.find_last_of('/'));

    _lock.Lock();
    std::map<qcc::String, SocketStream ^>::iterator it = _uriSockMap.find(requestUri);
    if (it != _uriSockMap.end()) {
        it->second = nullptr;
        _uriSockMap.erase(it);
    }
    _lock.Unlock();
    QCC_DbgTrace(("Removed %s ", requestUri.c_str()));
}

void _HttpServer::ThreadExit(qcc::Thread* thread)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    RequestThread* requestThread = static_cast<RequestThread*>(thread);
    _lock.Lock();
    std::vector<RequestThread*>::iterator it = std::find(_requestThreads.begin(), _requestThreads.end(), requestThread);
    if (it != _requestThreads.end()) {
        _requestThreads.erase(it);
    }
    _lock.Unlock();
    delete requestThread;
}

SocketStream ^ _HttpServer::GetSessionFd(const qcc::String & requestUri)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    _lock.Lock();
    SocketStream ^ sessionSock = nullptr;
    std::map<qcc::String, SocketStream ^>::iterator it = _uriSockMap.find(requestUri);
    if (it != _uriSockMap.end()) {
        sessionSock = it->second;
    }
    _lock.Unlock();
    return sessionSock;
}

_HttpServer::RequestThread::RequestThread(_HttpServer* httpServer, StreamSocket ^ httpSocket) : httpServer(httpServer), httpSocket(httpSocket)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    dataBuf = new uint8_t[maxHdr + maxData + 2];
    if (dataBuf == NULL) {
        QCC_THROW_EXCEPTION(ER_OUT_OF_MEMORY);
    }
}

_HttpServer::RequestThread::~RequestThread()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (httpSocket != nullptr) {
        delete httpSocket;
        httpSocket = nullptr;
    }
    ;

    if (dataReader != nullptr) {
        delete dataReader;
        dataReader = nullptr;
    }
    ;

    if (dataWriter != nullptr) {
        delete dataWriter;
        dataWriter = nullptr;
    }
    ;

    if (dataBuf != NULL) {
        delete [] dataBuf;
        dataBuf = NULL;
    }
}

void _HttpServer::RequestThread::LoadRequest()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (dataReader->UnconsumedBufferLength == 0) {
        auto op = dataReader->LoadAsync(4096);
        task<unsigned int> readTask(op);
        unsigned int bytesRead = readTask.get();
        QCC_DbgPrintf(("Load HTTP Header %d bytes", bytesRead));
    }
}

QStatus _HttpServer::RequestThread::GetLine(qcc::String& line)
{
    QStatus status = ER_OK;
    try{
        if (dataReader != nullptr) {
            line = qcc::String::Empty;
            while (true) {
                if (dataReader->UnconsumedBufferLength > 0) {
                    Platform::Array<unsigned char> ^ buffer = ref new Platform::Array<unsigned char>(1);
                    dataReader->ReadBytes(buffer);
                    unsigned char* p = buffer->Data;
                    line.append(*p);
                    if (*p == '\n') {
                        break;
                    }
                } else {
                    break;
                }
            }
        }
    } catch (Platform::Exception ^ e) {
        status = ER_OS_ERROR;
        auto m = e->Message;
        QCC_LogError(ER_OS_ERROR, ("_HttpServer::RequestThread::GetLine failed"));
    }
    return status;
}

QStatus _HttpServer::RequestThread::PushBytes(uint8_t* buf, size_t numBytes, size_t& sent)
{
    QStatus status = ER_OK;
    sent = 0;
    try{
        while (true) {
            if (dataWriter == nullptr || numBytes <= 0) {
                break;
            }

            Platform::ArrayReference<uint8_t> arrRef(buf, numBytes);
            dataWriter->WriteBytes(arrRef);
            task<unsigned int> storeTask(dataWriter->StoreAsync());
            storeTask.wait();
            sent = storeTask.get();
            if (sent == 0) {
                status = ER_SOCK_OTHER_END_CLOSED;
            }
            break;
        }
    } catch (Platform::Exception ^ e) {
        status = ER_OS_ERROR;
        auto m = e->Message;
        QCC_LogError(status, ("_HttpServer::RequestThread::PushBytes failed"));
    }
    return status;
}

QStatus _HttpServer::RequestThread::SendBadRequestResponse()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String response = "HTTP/1.1 400 Bad Request\r\n";
    size_t sent = 0;
    QStatus status = PushBytes((uint8_t*)response.data(), response.size(), sent);
    return status;
}

QStatus _HttpServer::RequestThread::SendNotFoundResponse()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String response = "HTTP/1.1 404 Not Found\r\n";
    size_t sent = 0;
    QStatus status = PushBytes((uint8_t*)response.data(), response.size(), sent);
    return status;
}

static void ParseRequest(const qcc::String& line, qcc::String& method, qcc::String& requestUri, qcc::String& httpVersion)
{
    QCC_DbgTrace(("%s(%s)", __FUNCTION__, line.c_str()));
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

qcc::ThreadReturn STDCALL _HttpServer::RequestThread::Run(void* arg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    dataReader = ref new DataReader(httpSocket->InputStream);
    dataReader->InputStreamOptions = InputStreamOptions::Partial;
    dataWriter = ref new DataWriter(httpSocket->OutputStream);
    QStatus status = ER_OK;
    while (status == ER_OK) {
        LoadRequest();
        uint32_t startPos = 0;
        qcc::String line;
        status = GetLine(line);
        if (status != ER_OK) {
            SendBadRequestResponse();
            break;
        }

        QCC_DbgHLPrintf(("Streamer got line:%s", line.c_str()));
        /*
         * We are very liberal - so long as this is a GET request we are ok with it.
         */
        qcc::String method, requestUri, httpVersion;
        ParseRequest(line, method, requestUri, httpVersion);
        if (method != "GET") {
            SendBadRequestResponse();
            break;
        }

        sessSock = httpServer->GetSessionFd(requestUri);
        if (sessSock == nullptr) {
            status = ER_FAIL;
            SendNotFoundResponse();
            break;
        }
        while (true) {
            line.clear();
            status = GetLine(line);
            if ((status != ER_OK) || line.empty()) {
                break;
            }
            QCC_DbgHLPrintf(("Streamer got line:%s", line.c_str()));
            if ((line.find("Range: bytes=") == 0)) {
                line.erase(0, 13);
                startPos = qcc::StringToU32(line.erase(line.find("-")), 10, 0);
            }
        }
        if (status == ER_OK) {
            size_t put = 0;
            qcc::String response;
            response = "HTTP/1.1 200 OK\r\n";
            response += "Date: " + qcc::UTCTime() + "\r\n";
            response += "Content-type: application/octet-stream\r\n"; // TODO
            response += "Cache-Control: no-cache\r\n";                // TODO
            response += "Server: AllJoyn HTTP Media Streamer 1.0\r\n";
            response += "Transfer-Encoding: chunked\r\n";
            response += "\r\n";

            QCC_DbgHLPrintf(("Streamer sending line:%s", response.c_str()));
            status = PushBytes((uint8_t*)response.data(), response.size(), put);
        }

        /*
         * Send chunks until we are told to stop or a get a read or write error.
         */

        while ((status == ER_OK) && IsRunning()) {
            Platform::ArrayReference<uint8_t> byteArryRef(dataBuf + maxHdr, maxData);
            Platform::Array<int32_t> ^ received = ref new Platform::Array<int32_t>(1);
            sessSock->Recv(byteArryRef, maxData, received);
            /* Check for a closed socket */
            if (received[0] == 0) {
                /*
                 * Cannot continue after the socket is closed
                 */
                status = ER_SOCK_OTHER_END_CLOSED;
                QCC_LogError(status, ("Recv 0 bytes"));
                break;
            }
            /* If we got an error we send a final chunk to terminate the stream */
            if (status != ER_OK) {
                received[0] = 0;
            }

            uint8_t* ptr = dataBuf + maxHdr;
            /* Chunk length header is ascii hex followed by cr-lf */
            *(--ptr) = '\n';
            *(--ptr) = '\r';
            size_t len = 2;
            size_t n = received[0];
            do {
                *(--ptr) = hex[n & 0xF];
                n >>= 4;
                ++len;
            } while (n);

            len += received[0];

            /* Chunk is terminated with cr-lf */
            ptr[len++] = '\r';
            ptr[len++] = '\n';
            //QCC_DbgPrintf(("Streamer received %d bytes pushing %d bytes", (int)received[0], (int)len));
            while (len) {
                size_t sent = 0;
                QStatus result = PushBytes(ptr, len, sent);
                //QCC_DbgHLPrintf(("Write Data=%d", sent));
                if (result != ER_OK) {
                    status = result;
                    QCC_LogError(status, ("Write Data Fail"));
                    break;
                }
                len -= sent;
                ptr += sent;
            }
        }
        QCC_DbgHLPrintf(("Streamer closing"));
        break;
    }
    httpSocket = nullptr;
    dataReader = nullptr;
    dataWriter = nullptr;
    sessSock = nullptr;
    if (dataBuf != NULL) {
        delete [] dataBuf;
        dataBuf = NULL;
    }
    return 0;
}

void SendStreamToSocket(SocketStream ^ socket, IRandomAccessStream ^ iStream)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    IInputStream ^ inputStream = iStream->GetInputStreamAt(0);
    DataReader ^ reader = ref new DataReader(inputStream);
    reader->InputStreamOptions = InputStreamOptions::Partial;
    try{
        while (true) {
            auto op = reader->LoadAsync(4096);
            task<unsigned int> readTask(op);
            unsigned int bytesRead = readTask.get();
            //QCC_DbgPrintf(("Load %d bytes", bytesRead));

            if (bytesRead == 0) {
                return;
            }
            if (reader->UnconsumedBufferLength > 0) {
                Platform::Array<unsigned char> ^ buffer = ref new Platform::Array<unsigned char>(reader->UnconsumedBufferLength);
                reader->ReadBytes(buffer);
                size_t pending = buffer->Length;
                size_t offset = 0;
                while (pending > 0) {
                    Platform::ArrayReference<uint8_t> arrRef(buffer->Data + offset, pending);
                    Platform::Array<int> ^ sent = ref new Platform::Array<int>(1);
                    socket->Send(arrRef, pending, sent);
                    pending -= sent[0];
                    offset += sent[0];
                }
            }
        }
    } catch (Platform::Exception ^ e) {

    }
}

void StreamSourceHost::Send(SocketStream ^ socket, StorageFile ^ file)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    QStatus status = ER_OK;
    while (true) {
        if (socket == nullptr) {
            status = ER_BAD_ARG_1;
            break;
        }

        if (file == nullptr) {
            status = ER_BAD_ARG_2;
            break;
        }

        auto op = file->OpenAsync(FileAccessMode::Read);
        concurrency::task<IRandomAccessStream ^> openTask(op);
        openTask.then([socket](IRandomAccessStream ^ stream) {
                          try {
                              ThreadPool::RunAsync(ref new WorkItemHandler([stream, socket](IAsyncAction ^ operation) {

                                                                               SendStreamToSocket(socket, stream);

                                                                           }, Platform::CallbackContext::Any));
                          } catch (...) {
                              QCC_LogError(ER_OS_ERROR, ("Creating streaming thread fail"));
                          }

                      });
        break;
    }

    if (status != ER_OK) {
        QCC_THROW_EXCEPTION(status);
    }

}

}