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
#include <ppltasks.h>

#include "Status.h"
#include "MediaHTTPStreamer.h"
#include "Helper.h"

namespace AllJoynStreaming {

using namespace Windows::System::Threading;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;
using namespace Windows::Foundation;
using namespace concurrency;

using namespace std;
using namespace qcc;

static const uint8_t hex[] = "0123456789ABCDEF";

const size_t maxData = 8192;
const size_t maxHdr = 10;

class MediaHTTPStreamer::Internal {
  public:

    Internal(MediaHTTPStreamer ^ httpStreamer) :
        streamer(httpStreamer),
        listenPort(0),
        httpListener(nullptr),
        httpSock(nullptr),
        dataReader(nullptr),
        dataWriter(nullptr),
        sessSock(nullptr),
        sessSockEvent(nullptr),
        discarding(true),
        running(false)
    {
        dataBuf = new uint8_t[maxHdr + maxData + 2];
        if (dataBuf == NULL) {
            QCC_THROW_EXCEPTION(ER_OUT_OF_MEMORY);
        }
    }

    void InitializeListener();
    void HttpSocketConnectionReceived(StreamSocketListener ^ sender, StreamSocketListenerConnectionReceivedEventArgs ^ args);
    void SessionSocketDataReceivedHandler();
    QStatus GetLine(qcc::String& line);
    QStatus PushBytes(uint8_t* buf, size_t numBytes, size_t& sent);
    void ServeHttpMediaClient();
    void LoadData();
    bool IsRunning() { return running; }

    MediaHTTPStreamer ^ streamer;
    uint16_t listenPort;
    qcc::String mimeType;
    StreamSocketListener ^ httpListener;
    StreamSocket ^ httpSock;
    DataReader ^ dataReader;
    DataWriter ^ dataWriter;
    AllJoyn::SocketStream ^ sessSock;
    AllJoyn::SocketStreamEvent ^ sessSockEvent;
    bool discarding;
    bool running;
    EventRegistrationToken evtToken;          /**< The token used to remove ConnectionRequested event handler */

    uint32_t length;
    uint8_t* dataBuf;
};

void MediaHTTPStreamer::Internal::InitializeListener()
{
    QCC_DbgPrintf(("MediaHTTPStreamer::Internal::InitializeListener()"));
    httpListener = ref new StreamSocketListener();
    TypedEventHandler<StreamSocketListener ^, StreamSocketListenerConnectionReceivedEventArgs ^> ^ connectionHandler = ref new TypedEventHandler<StreamSocketListener ^, StreamSocketListenerConnectionReceivedEventArgs ^>(
        [ = ] (StreamSocketListener ^ sender, StreamSocketListenerConnectionReceivedEventArgs ^ args) {
            HttpSocketConnectionReceived(sender, args);
        });
    evtToken = httpListener->ConnectionReceived += connectionHandler;
    Platform::String ^ serviceName = MultibyteToPlatformString(U32ToString(listenPort).c_str());
    IAsyncAction ^ op = httpListener->BindServiceNameAsync(serviceName);
    task<void> bindTask(op);
    bindTask.then([this] (task<void> resultTask) {
                      try {
                          resultTask.get();
                          streamer->StateChange(HTTPState::HTTP_LISTEN, nullptr);
                      }catch (Platform::Exception ^ e) {
                          streamer->StateChange(HTTPState::FATAL_ERROR, nullptr);
                      }
                  });

}

void MediaHTTPStreamer::Internal::HttpSocketConnectionReceived(StreamSocketListener ^ sender, StreamSocketListenerConnectionReceivedEventArgs ^ args)
{
    QCC_DbgPrintf(("MediaHTTPStreamer::Internal::HttpSocketConnectionReceived()"));
    // Only accept one connection
    if (httpSock != nullptr) {
        return;
    }
    httpSock = args->Socket;
    dataReader = ref new DataReader(httpSock->InputStream);
    dataReader->InputStreamOptions = InputStreamOptions::Partial;
    dataWriter = ref new DataWriter(httpSock->OutputStream);

    task<void> ([this] () {
                    try {
                        ThreadPool::RunAsync(ref new WorkItemHandler([this](IAsyncAction ^ operation) {
                                                                         ServeHttpMediaClient();
                                                                     }, Platform::CallbackContext::Any));
                        this->running = true;
                    } catch (...) {
                        this->running = false;
                        httpSock = nullptr;
                        QCC_LogError(ER_OS_ERROR, ("Creating streaming thread fail"));
                    }
                });
    httpListener->ConnectionReceived -= evtToken;
}

void MediaHTTPStreamer::Internal::LoadData()
{
    if (dataReader->UnconsumedBufferLength == 0) {
        auto op = dataReader->LoadAsync(4096);
        task<unsigned int> readTask(op);
        unsigned int bytesRead = readTask.get();
        QCC_DbgPrintf(("Load HTTP Header %d bytes", bytesRead));
    }
}

QStatus MediaHTTPStreamer::Internal::GetLine(qcc::String& line)
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
        QCC_LogError(ER_OS_ERROR, ("MediaHTTPStreamer::Internal::GetLine failed"));
    }
    return status;
}

QStatus MediaHTTPStreamer::Internal::PushBytes(uint8_t* buf, size_t numBytes, size_t& sent)
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
        QCC_LogError(status, ("MediaHTTPStreamer::Internal::PushBytes failed"));
    }
    return status;
}

void MediaHTTPStreamer::Internal::SessionSocketDataReceivedHandler()
{
    // QCC_DbgHLPrintf(("MediaHTTPStreamer::Internal::SessionSocketDataReceivedHandler(discarding=%d)", discarding));
    if (discarding) {
        Platform::ArrayReference<uint8_t> byteArryRef(dataBuf + maxHdr, maxData);
        Platform::Array<int> ^ rcv = ref new Platform::Array<int>(1);
        try{
            while (sessSock->CanRead()) {
                sessSock->Recv(byteArryRef, maxData, rcv);
                int32_t received = rcv[0];
            }
        }catch (Platform::Exception ^ e) {
            QCC_LogError(ER_OS_ERROR, ("SessionSocketDataReceivedHandler Recv() failed"));
            discarding = false;
        }
    }
}

void MediaHTTPStreamer::Internal::ServeHttpMediaClient()
{
    discarding = false;
    while (IsRunning()) {
        QStatus status = ER_OK;
        LoadData();
        uint32_t startPos = 0;
        qcc::String line;
        status = GetLine(line);
        if (status == ER_OK) {
            QCC_DbgHLPrintf(("Streamer got line:%s", line.c_str()));
            /*
             * We are very liberal - so long as this is a GET request we are ok with it.
             */
            if (line.find("GET ") == 0) {
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
                    if (startPos > 0) {
                        response = "HTTP/1.1 206 Partial Content\r\n";
                        response += "Content-Range: bytes " + U32ToString(startPos) + "-" + U32ToString(length - 1) + "/" + U32ToString(length) + "\r\n";
                        response += "Content-Length: " + U32ToString(length - startPos) + "\r\n";
                    } else {
                        response = "HTTP/1.1 200 OK\r\n";
                        response += "Content-Length: " + U32ToString(length) + "\r\n";
                    }
                    response += "Server: AllJoyn HTTP Media Streamer 1.0\r\n";
                    response += "Date: " + UTCTime() + "\r\n";
                    response += "Transfer-Encoding: chunked\r\n";
                    response += "Accept-Ranges: bytes\r\n";
                    if (mimeType.size() > 0) {
                        response += "Content-Type: " + mimeType + "\r\n";
                    }
                    response += "\r\n";
                    QCC_DbgHLPrintf(("Streamer sending line:%s", response.c_str()));
                    status = PushBytes((uint8_t*)response.data(), response.size(), put);
                }
            } else {
                status = ER_FAIL;
            }
        }
        if (status == ER_OK) {
            streamer->StateChange(HTTPState::HTTP_GETTING, startPos);
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
            QCC_DbgPrintf(("Streamer received %d bytes pushing %d bytes", (int)received[0], (int)len));
            while (len) {
                size_t sent = 0;
                QStatus result = PushBytes(ptr, len, sent);
                QCC_DbgHLPrintf(("Write Data=%d", sent));
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
        if (IsRunning()) {
            streamer->StateChange(HTTPState::HTTP_DISCONNECT, nullptr);
        }
    }

    httpSock = nullptr;
    dataReader = nullptr;
    dataWriter = nullptr;
    sessSock = nullptr;
    sessSockEvent = nullptr;
    if (dataBuf != NULL) {
        delete [] dataBuf;
        dataBuf = NULL;
    }
    return;
}

MediaHTTPStreamer::MediaHTTPStreamer(MediaSink ^ mediaSink) : _internal(*(new Internal(this))), _mediaSink(mediaSink)
{
    QCC_DbgHLPrintf(("MediaHTTPStreamer constructor"));
    UseDefaultStateChangedHandler = true;
    StateChange += ref new MediaHTTPStreamListenerStateChange([&] (HTTPState state, Platform::Object ^ arg) {
                                                                  DefaultMediaHTTPStreamListenerStateChangeHandler(state, arg);
                                                              });
}

MediaHTTPStreamer::~MediaHTTPStreamer()
{
    QCC_DbgHLPrintf(("MediaHTTPStreamer destructor"));
    Stop();
    delete &_internal;
}

void MediaHTTPStreamer::Start(AllJoyn::SocketStream ^ sock, Platform::String ^ mimeType, uint32_t length, uint16_t port)
{
    QStatus status = ER_OK;
    QCC_DbgHLPrintf(("MediaHTTPStreamer::Start"));
    if (_internal.sessSock != nullptr) {
        status = ER_MEDIA_HTTPSTREAMER_ALREADY_STARTED;
    } else {
        _internal.sessSock = sock;
        _internal.sessSockEvent = ref new AllJoyn::SocketStreamEvent(sock);
        _internal.sessSockEvent->DataReceived += ref new AllJoyn::SocketStreamDataReceivedHandler([&] {
                                                                                                      _internal.SessionSocketDataReceivedHandler();
                                                                                                  });
        _internal.mimeType = PlatformToMultibyteString(mimeType);
        _internal.listenPort = port;
        _internal.length = length;
        _internal.InitializeListener();
    }

    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void MediaHTTPStreamer::Stop()
{
    QCC_DbgHLPrintf(("MediaHTTPStreamer::Stop"));
    _internal.running = false;
}

void MediaHTTPStreamer::DefaultMediaHTTPStreamListenerStateChangeHandler(HTTPState state, Platform::Object ^ arg)
{
    if (!UseDefaultStateChangedHandler) {
        return;
    }
    switch (state) {
    case HTTPState::FATAL_ERROR:
        _mediaSink->Close();
        break;

    case HTTPState::HTTP_LISTEN:
        break;

    case HTTPState::HTTP_GETTING:
        try {
            Platform::IBox<uint32_t> ^ r = dynamic_cast<Platform::IBox<uint32_t> ^>(arg);
            uint32_t pos = r->Value;
            _mediaSink->SeekAbsolute(pos, (uint8_t) MediaSeekUnits::BYTES);
        } catch (Platform::Exception ^ e) {
            auto m = e->Message;
            auto h = e->HResult;
            _mediaSink->Close();
        }
        break;

    case HTTPState::HTTP_DISCONNECT:
        if (_mediaSink != nullptr) {
            _mediaSink->Pause(true /*drain*/);
        }
        break;

    case HTTPState::SOCKET_CLOSED:
        _mediaSink->Close();
        break;
    }
}

}

