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

#include <qcc/winrt/utility.h>
#include <qcc/Mutex.h>

#include <map>
#include <vector>
#include <set>
#include <ppltasks.h>
#include <ctxtcall.h>

#include "MediaSink.h"
#include "MediaCommon.h"
#include "Helper.h"

using namespace  Windows::Foundation;
using namespace  concurrency;

namespace AllJoynStreaming {

/*
 * Time to wait for an stream open to complete
 */
static const uint32_t STREAM_OPEN_TIMEOUT = 10 * 1000;

/*
 * Strip the leading object path to give the stream name
 */
static Platform::String ^ StreamName(Platform::String ^ path)
{
    const wchar_t* base;
    const wchar_t* p = path->Data();
    while (*p) {
        if (*p == (wchar_t)'/') {
            base = p + 1;
        }
        p++;
    }
    return ref new Platform::String(base);
}

class SinkInfo {
  public:
    SinkInfo() : socket(nullptr), description(nullptr), renderer(nullptr), openEvent(INVALID_HANDLE_VALUE), status(ER_OK), paused(true) { }

    AllJoyn::SocketStream ^ socket;
    MediaDescription ^ description;
    MediaRenderer ^ renderer;
    HANDLE openEvent;
    QStatus status;
    bool paused;
};

class MediaSink::Internal {
  public:

    typedef enum { INFORM_CLOSE, INFORM_PAUSE, INFORM_PLAY, INFORM_SEEK } InformWhat;

    /*
     * Constructor
     */
    Internal(AllJoyn::BusAttachment ^ bus) : bus(bus), mediaSource(nullptr)
    {
        Windows::UI::Core::CoreWindow ^ window = Windows::UI::Core::CoreWindow::GetForCurrentThread();
        if (nullptr != window) {
            _dispatcher = window->Dispatcher;
        }
        _originSTA = IsOriginSTA();
    };

    void Close()
    {
        lock.Lock();
        streamProxies.clear();
        mediaSource = nullptr;
        streamDescriptions = nullptr;
        lock.Unlock();
    }

    /*
     * Destructor
     */
    ~Internal()
    {
        Close();
    }

    /*
     * Informational callbacks need to be made from another thread
     */
    QStatus Inform(SinkInfo* info, InformWhat what, uint32_t delay = 0)
    {
        if (nullptr == mediaSource) {
            return ER_FAIL;
        }
        /*
         * Update pause state in sink info
         */
        task<void> informTask([ = ] {
                                  switch (what) {
                                  case INFORM_CLOSE:
                                      DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                        info->renderer->OnClose(info->socket);
                                                                                                    }));

                                      info->paused = true;
                                      break;

                                  case INFORM_PAUSE:
                                      DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                        info->renderer->OnPause(info->socket);
                                                                                                    }));

                                      info->paused = true;
                                      break;

                                  case INFORM_PLAY:
                                      DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                        info->renderer->OnPlay(info->socket);
                                                                                                    }));

                                      info->paused = false;
                                      break;

                                  case INFORM_SEEK:
                                      DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                        info->renderer->OnSeek(info->socket);
                                                                                                    }));

                                      info->paused = false;
                                      break;
                                  }
                              });
        return ER_OK;
    }

    SinkInfo* GetSinkInfo(AllJoyn::ProxyBusObject ^ proxy)
    {
        if (proxy != nullptr) {
            std::map<AllJoyn::ProxyBusObject ^, SinkInfo>::iterator iter = streamProxies.find(proxy);
            if (iter != streamProxies.end()) {
                return &(iter->second);
            }
        }
        return NULL;
    }

    bool IsOriginSTA()
    {
        APTTYPE aptType;
        APTTYPEQUALIFIER aptTypeQualifier;
        HRESULT hr = ::CoGetApartmentType(&aptType, &aptTypeQualifier);
        if (SUCCEEDED(hr)) {
            if (aptType == APTTYPE_MAINSTA || aptType == APTTYPE_STA) {
                return true;
            }
        }
        return false;
    }

    void DispatchCallback(Windows::UI::Core::DispatchedHandler ^ callback);
    /*
     * The bus attachment
     */
    AllJoyn::BusAttachment ^ bus;

    /*
     * Proxy bus object for the connected media source.
     */
    AllJoyn::ProxyBusObject ^ mediaSource;

    struct Compare {
        bool operator()(AllJoyn::ProxyBusObject ^ lhs, AllJoyn::ProxyBusObject ^ rhs) const
        {
            return reinterpret_cast<void*>(lhs) < reinterpret_cast<void*>(rhs);
        }
    };

    /*
     * Map of stream proxies to stream info.
     */
    std::map<AllJoyn::ProxyBusObject ^, SinkInfo, Compare> streamProxies;

    /*
     * Descriptions of the media streams.
     */
    Platform::Array<MediaDescription ^> ^ streamDescriptions;

    qcc::Mutex lock;

    Windows::UI::Core::CoreDispatcher ^ _dispatcher;

    bool _originSTA;
};

void MediaSink::Internal::DispatchCallback(Windows::UI::Core::DispatchedHandler ^ callback)
{
    Windows::UI::Core::CoreWindow ^ window = Windows::UI::Core::CoreWindow::GetForCurrentThread();
    Windows::UI::Core::CoreDispatcher ^ dispatcher = nullptr;
    if (nullptr != window) {
        dispatcher = window->Dispatcher;
    }
    if (_originSTA && nullptr != _dispatcher && _dispatcher != dispatcher) {
        // The origin was STA and the thread dispatcher doesn't match up. Move execution to the origin dispatcher thread.
        Windows::Foundation::IAsyncAction ^ op = _dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
                                                                       ref new Windows::UI::Core::DispatchedHandler([callback] () {
                                                                                                                        callback();
                                                                                                                    }));
        // Exceptions are caught by specific handlers. If wait() below throws, that is a bug in the handler wrapper implementation.
        concurrency::task<void> dispatcherOp(op);
        dispatcherOp.wait();
    } else {
        // In this case, our source origin is MTA or we are STA with either no dispatcher (no UI threads involved) or we are already in the dispatcher thread
        // for the STA compartment.
        callback();
    }
}

MediaSink::MediaSink(AllJoyn::BusAttachment ^ bus)
{
    QCC_DbgPrintf(("MEdiaSink::MediaSink()"));
    Bus = bus;
    MediaSinkBusObject = ref new AllJoyn::BusObject(bus, L"/org/alljoyn/MediaSink", false);
    MediaCommon::CreateInterfaces(bus);

    AllJoyn::MessageReceiver ^ streamClosedRecv = ref new AllJoyn::MessageReceiver(bus);
    streamClosedRecv->SignalHandler += ref new AllJoyn::MessageReceiverSignalHandler([&] (AllJoyn::InterfaceMember ^ member, Platform::String ^ srcPath, AllJoyn::Message ^ message) {
                                                                                         StreamClosed(member, srcPath, message);
                                                                                     });
    bus->RegisterSignalHandler(streamClosedRecv, MediaCommon::GetSinkIfc()->GetMember(L"StreamClosed"), nullptr);

    AllJoyn::MessageReceiver ^ streamOpenedRecv = ref new AllJoyn::MessageReceiver(bus);
    streamOpenedRecv->SignalHandler += ref new AllJoyn::MessageReceiverSignalHandler([&] (AllJoyn::InterfaceMember ^ member, Platform::String ^ srcPath, AllJoyn::Message ^ message) {
                                                                                         StreamOpened(member, srcPath, message);
                                                                                     });
    bus->RegisterSignalHandler(streamOpenedRecv, MediaCommon::GetSinkIfc()->GetMember(L"StreamOpened"), nullptr);

    AllJoyn::MessageReceiver ^ streamPausedRecv = ref new AllJoyn::MessageReceiver(bus);
    streamPausedRecv->SignalHandler += ref new AllJoyn::MessageReceiverSignalHandler([&] (AllJoyn::InterfaceMember ^ member, Platform::String ^ srcPath, AllJoyn::Message ^ message) {
                                                                                         StreamPaused(member, srcPath, message);
                                                                                     });
    bus->RegisterSignalHandler(streamPausedRecv, MediaCommon::GetSinkIfc()->GetMember(L"StreamPaused"), nullptr);

    AllJoyn::MessageReceiver ^ streamPlayingRecv = ref new AllJoyn::MessageReceiver(bus);
    streamPlayingRecv->SignalHandler += ref new AllJoyn::MessageReceiverSignalHandler([&] (AllJoyn::InterfaceMember ^ member, Platform::String ^ srcPath, AllJoyn::Message ^ message) {
                                                                                          StreamPlaying(member, srcPath, message);
                                                                                      });
    bus->RegisterSignalHandler(streamPlayingRecv, MediaCommon::GetSinkIfc()->GetMember(L"StreamPlaying"), nullptr);

    _internal = new Internal(bus);
    if (_internal == NULL) {
        QCC_THROW_EXCEPTION(ER_OUT_OF_MEMORY);
    }
}

MediaSink::~MediaSink()
{
    QCC_DbgPrintf(("MEdiaSink::~MediaSink()"));
    if (_internal) {
        delete _internal;
        _internal = NULL;
    }
}


Windows::Foundation::IAsyncAction ^  MediaSink::ConnectSourceAsync(Platform::String ^ busName, uint32_t sessionId)
{
    QCC_DbgPrintf(("MEdiaSink::ConnectSourceAsync(sessionId=%d)", sessionId));
    if (nullptr != _internal->mediaSource) {
        QCC_THROW_EXCEPTION(AllJoyn::QStatus::ER_BUS_ALREADY_CONNECTED);
    }

    IAsyncAction ^ action = create_async([ = ]() {
                                             QCC_DbgPrintf(("MediaSink::ConnectSource sessionId(%d)", sessionId));
                                             _internal->mediaSource = ref new AllJoyn::ProxyBusObject(Bus, busName, L"/org/alljoyn/MediaSource", sessionId);

                                             try {
                                                 IAsyncOperation<AllJoyn::IntrospectRemoteObjectResult ^> ^ op = _internal->mediaSource->IntrospectRemoteObjectAsync(nullptr);
                                                 concurrency::task<AllJoyn::IntrospectRemoteObjectResult ^> introspecTask(op);
                                                 introspecTask.wait();
                                                 AllJoyn::IntrospectRemoteObjectResult ^ result = introspecTask.get();

                                                 /*
                                                  * Get the proxy bus objects for the streams.
                                                  */
                                                 size_t numChildren = _internal->mediaSource->GetChildren(nullptr);
                                                 Platform::Array<AllJoyn::ProxyBusObject ^> ^ children = ref new Platform::Array<AllJoyn::ProxyBusObject ^>(numChildren);
                                                 _internal->mediaSource->GetChildren(children);
                                                 for (size_t i = 0; i < numChildren; ++i) {
                                                     /*
                                                      * Introspect the remote object if we don't yet know its interfaces. Note, that all
                                                      * objects are created with the peer interface (ping) defined so there is always at
                                                      * least on interface.
                                                      */
                                                     if (children[i]->GetInterfaces(nullptr) == 1) {
                                                         IAsyncOperation<AllJoyn::IntrospectRemoteObjectResult ^> ^ op = children[i]->IntrospectRemoteObjectAsync(nullptr);
                                                         concurrency::task<AllJoyn::IntrospectRemoteObjectResult ^> introspecTask(op);
                                                         introspecTask.wait();
                                                     }

                                                     _internal->lock.Lock();
                                                     if (children[i]->ImplementsInterface(L"org.alljoyn.MediaStream")) {
                                                         _internal->streamProxies[children[i]].socket = nullptr;
                                                     }
                                                     _internal->lock.Unlock();
                                                 }
                                                 children = nullptr;
                                             } catch (Platform::Exception ^ e) {
                                                 auto m = e->Message;
                                                 auto h = e->HResult;
                                                 _internal->mediaSource = nullptr;
                                             }
                                         });
    return action;
}

AllJoyn::ProxyBusObject ^ MediaSink::GetStreamProxy(Platform::String ^ streamName)
{
    if (_internal->mediaSource == nullptr) {
        return nullptr;
    }

    std::map<AllJoyn::ProxyBusObject ^, SinkInfo>::iterator it = _internal->streamProxies.begin();
    for (; it != _internal->streamProxies.end(); it++) {
        if (StreamName(it->first->Path)->Equals(streamName)) {
            return it->first;
        }
    }

    return nullptr;
}

Platform::Object ^ GetValueByKey(AllJoyn::MsgArg ^ arg, Platform::String ^ key)
{
    Platform::Object ^ o = arg->Value;
    Platform::IBoxArray<Platform::Object ^> ^ boxArray = dynamic_cast<Platform::IBoxArray<Platform::Object ^> ^>(o);
    Platform::Array<Platform::Object ^> ^ objArray = boxArray->Value;
    uint32_t i = 0;
    for (; i < objArray->Length; i++) {
        AllJoyn::MsgArg ^ el = dynamic_cast<AllJoyn::MsgArg ^>(objArray[i]);
        Platform::String ^ dictKey = dynamic_cast<Platform::String ^>(el->Key);
        if (key->Equals(dictKey)) {
            AllJoyn::MsgArg ^ dictValue = dynamic_cast<AllJoyn::MsgArg ^>(el->Value);
            return dictValue->Value;
        }
    }
    return nullptr;
}

AllJoyn::MsgArg ^ GetInterfaceProperty(AllJoyn::ProxyBusObject ^ stream, Platform::String ^ ifceName)
{
    IAsyncOperation<AllJoyn::GetAllPropertiesResult ^> ^ op = stream->GetAllPropertiesAsync(ifceName, nullptr, 10 * 1000);
    concurrency::task<AllJoyn::GetAllPropertiesResult ^> getPropTask(op);
    getPropTask.wait();
    AllJoyn::GetAllPropertiesResult ^ result = getPropTask.get();
    return result->Value;
}

QStatus MediaSink::GetStreamProperties(AllJoyn::ProxyBusObject ^ stream, MediaDescription ^ description)
{
    QCC_DbgPrintf(("MEdiaSink::GetStreamProperties()"));
    QStatus status = ER_OK;
    AllJoyn::MsgArg ^ props = GetInterfaceProperty(stream, L"org.alljoyn.MediaStream");
    /*
     * Unpack the stream properties
     */
    Platform::String ^ mimeType = dynamic_cast<Platform::String ^>(GetValueByKey(props, L"MimeType"));
    description->MimeType = mimeType;
    description->MType = MediaDescription::ResolveMediaType(PlatformToMultibyteString(mimeType));
    Platform::IBox<uint64_t> ^ size = dynamic_cast<Platform::IBox<uint64_t> ^>(GetValueByKey(props, L"Size"));
    description->Size = size->Value;
    Platform::IBox<uint8_t> ^ sa = dynamic_cast<Platform::IBox<uint8_t> ^>(GetValueByKey(props, L"Seekable"));
    description->Seekable = sa->Value;
    Platform::IBox<Platform::Boolean> ^ pa = dynamic_cast<Platform::IBox<Platform::Boolean> ^>(GetValueByKey(props, L"Pausable"));
    description->Pausable = pa->Value;
    /*
     * Get stream type-specific properties
     */
    switch (description->MType) {
    case MediaType::AUDIO:
    {
        props = GetInterfaceProperty(stream, L"org.alljoyn.MediaStream.Audio");
        Platform::IBox<uint32_t> ^ spf = dynamic_cast<Platform::IBox<uint32_t> ^>(GetValueByKey(props, L"SamplesPerFrame"));
        description->SamplesPerFrame = spf->Value;
        Platform::IBox<uint32_t> ^ sf = dynamic_cast<Platform::IBox<uint32_t> ^>(GetValueByKey(props, L"SampleFrequency"));
        description->SampleFrequency = sf->Value;
        Platform::IBox<uint32_t> ^ br = dynamic_cast<Platform::IBox<uint32_t> ^>(GetValueByKey(props, L"BitRate"));
        description->BitRate = br->Value;
    }
    break;

    case MediaType::VIDEO:
    {
        props = GetInterfaceProperty(stream, L"org.alljoyn.MediaStream.Video");
        Platform::IBox<uint32_t> ^ fr = dynamic_cast<Platform::IBox<uint32_t> ^>(GetValueByKey(props, L"FrameRate"));
        description->FrameRate = fr->Value;
        Platform::IBox<uint32_t> ^ width = dynamic_cast<Platform::IBox<uint32_t> ^>(GetValueByKey(props, L"Width"));
        description->Width = width->Value;
        Platform::IBox<uint32_t> ^ height = dynamic_cast<Platform::IBox<uint32_t> ^>(GetValueByKey(props, L"Height"));
        description->Height = height->Value;
    }
    break;

    case MediaType::IMAGE:
    {
        props = GetInterfaceProperty(stream, L"org.alljoyn.MediaStream.Image");
        Platform::IBox<uint32_t> ^ width = dynamic_cast<Platform::IBox<uint32_t> ^>(GetValueByKey(props, L"Width"));
        description->Width = width->Value;
        Platform::IBox<uint32_t> ^ height = dynamic_cast<Platform::IBox<uint32_t> ^>(GetValueByKey(props, L"Height"));
        description->Height = height->Value;
    }
    break;

    case MediaType::APPLICATION:
    case MediaType::TEXT:
    case MediaType::OTHER:
        break;
    }
    Platform::String ^ path = stream->Path;
    description->StreamName = StreamName(path);
    return status;
}

IAsyncOperation<ListStreamResult ^> ^ MediaSink::ListStreamsAsync()
{
    QCC_DbgPrintf(("MEdiaSink::ListStreamsAsync()"));
    IAsyncOperation<ListStreamResult ^> ^ op = create_async([ = ]() {

                                                                if (nullptr == _internal->mediaSource) {
                                                                    QCC_THROW_EXCEPTION(AllJoyn::QStatus::ER_BUS_NOT_CONNECTED);
                                                                }

                                                                _internal->lock.Lock();
                                                                if (!_internal->streamProxies.empty()) {
                                                                    _internal->streamDescriptions = ref new Platform::Array<MediaDescription ^>(_internal->streamProxies.size());
                                                                    /*
                                                                     * Iterate over the streams and get the stream properties for each stream.
                                                                     */
                                                                    uint32_t count = 0;
                                                                    for (std::map<AllJoyn::ProxyBusObject ^, SinkInfo>::iterator iter = _internal->streamProxies.begin(); iter != _internal->streamProxies.end(); iter++) {
                                                                        MediaDescription ^ description = ref new MediaDescription();
                                                                        GetStreamProperties(iter->first, description);
                                                                        iter->second.description = description;
                                                                        _internal->streamDescriptions[count] = description;
                                                                        ++count;
                                                                    }
                                                                }
                                                                _internal->lock.Unlock();

                                                                ListStreamResult ^ result = ref new ListStreamResult();
                                                                result->Streams = _internal->streamDescriptions;
                                                                return result;
                                                            });
    return op;
}

IAsyncAction ^ MediaSink::OpenStreamAsync(Platform::String ^ streamName, MediaRenderer ^ renderer)
{
    QCC_DbgPrintf(("MEdiaSink::OpenStreamAsync()"));
    AllJoyn::ProxyBusObject ^ streamProxy = GetStreamProxy(streamName);
    if (nullptr == streamProxy) {
        QCC_THROW_EXCEPTION(ER_MEDIA_STREAM_NOT_FOUND);
    }
    if (nullptr != _internal->streamProxies[streamProxy].socket) {
        QCC_THROW_EXCEPTION(ER_MEDIA_STREAM_OPEN);
    }

    IAsyncAction ^ action = create_async([ = ]() {
                                             QStatus status = ER_OK;
                                             /*
                                              * Initialize event that will be used to wait for StreamOpened signal.
                                              */
                                             _internal->lock.Lock();
                                             SinkInfo& info = _internal->streamProxies[streamProxy];
                                             HANDLE openEvt = CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
                                             if (info.openEvent != INVALID_HANDLE_VALUE) {
                                                 _internal->lock.Unlock();
                                                 QCC_THROW_EXCEPTION(ER_MEDIA_STREAM_BUSY);
                                             }
                                             info.openEvent = openEvt;
                                             info.renderer = nullptr;

                                             _internal->lock.Unlock();
                                             /*
                                              * Let the media source know we are attempting to open the stream. The open method will return
                                              * the bus name for the media source and the port for the media stream session.
                                              */
                                             IAsyncOperation<AllJoyn::MethodCallResult ^> ^ op = streamProxy->MethodCallAsync(MediaCommon::GetStreamIfc()->GetMember(L"Open"), nullptr, nullptr, 10 * 1000, 0);
                                             concurrency::task<AllJoyn::MethodCallResult ^> methodCallTask(op);
                                             methodCallTask.wait();
                                             AllJoyn::MethodCallResult ^ methodCallResult = methodCallTask.get();
                                             AllJoyn::Message ^ reply = methodCallResult->Message;
                                             Platform::String ^ busName = dynamic_cast<Platform::String ^>(reply->GetArg(0)->Value);
                                             Platform::IBox<uint16_t> ^ sp = dynamic_cast<Platform::IBox<uint16_t> ^>(reply->GetArg(1)->Value);
                                             uint16_t sessionPort = sp->Value;
                                             Platform::IBox<Platform::Boolean> ^ s = dynamic_cast<Platform::IBox<Platform::Boolean> ^>(reply->GetArg(2)->Value);
                                             bool success = s->Value;
                                             if (!success) {
                                                 QCC_THROW_EXCEPTION(ER_MEDIA_STREAM_OPEN_FAILED);
                                             }

                                             AllJoyn::SocketStream ^ socket = nullptr;
                                             AllJoyn::SessionOpts ^ optsIn = ref new AllJoyn::SessionOpts(AllJoyn::TrafficType::TRAFFIC_RAW_RELIABLE, false, AllJoyn::ProximityType::PROXIMITY_ANY, AllJoyn::TransportMaskType::TRANSPORT_ANY);
                                             Platform::Array<AllJoyn::SessionOpts ^> ^ optsOut = ref new Platform::Array<AllJoyn::SessionOpts ^>(1);
                                             IAsyncOperation<AllJoyn::JoinSessionResult ^> ^ joinOp = Bus->JoinSessionAsync(busName, sessionPort, nullptr, optsIn, optsOut, nullptr);
                                             concurrency::task<AllJoyn::JoinSessionResult ^> joinSessionTask(joinOp);
                                             joinSessionTask.wait();
                                             AllJoyn::JoinSessionResult ^ joinResult = joinSessionTask.get();
                                             uint32_t sessionId = joinResult->SessionId;
                                             try {
                                                 Platform::Array<AllJoyn::SocketStream ^> ^ socks = ref new Platform::Array<AllJoyn::SocketStream ^>(1);
                                                 Bus->GetSessionSocketStream(sessionId, socks);
                                                 socket = socks[0];
                                             }catch (Platform::Exception ^ e) {
                                                 auto m = e->Message;
                                                 auto h = AllJoyn::AllJoynException::GetErrorMessage(e->HResult);
                                                 status = ER_FAIL;
                                                 QCC_LogError(status, ("GetSessionSocketStream Fail"));
                                             }

                                             if (status == ER_OK) {
                                                 /*
                                                  * Wait for the signal that tells us the open is complete.
                                                  */
                                                 WaitForSingleObjectEx(openEvt, STREAM_OPEN_TIMEOUT, false);
                                                 CloseHandle(openEvt);
                                                 _internal->lock.Lock();
                                                 status = info.status;
                                                 if (info.status == ER_OK) {
                                                     info.openEvent = INVALID_HANDLE_VALUE;
                                                     info.socket = socket;
                                                     info.renderer = renderer;
                                                     info.paused = false;
                                                     _internal->lock.Unlock();
                                                     _internal->DispatchCallback(ref new Windows::UI::Core::DispatchedHandler([&]() {
                                                                                                                                  renderer->OnOpen(this, info.description, socket);
                                                                                                                              }));
                                                 } else {
                                                     socket = nullptr;
                                                     _internal->lock.Unlock();
                                                 }
                                             } else {
                                                 IAsyncOperation<AllJoyn::MethodCallResult ^> ^ op = streamProxy->MethodCallAsync(MediaCommon::GetStreamIfc()->GetMember(L"Close"), nullptr, nullptr, 10 * 1000, 0); \
                                                 concurrency::task<AllJoyn::MethodCallResult ^> closeTask(op);
                                                 closeTask.wait();
                                             }
                                         });
    return action;
}

IAsyncAction ^ MediaSink::CloseStreamAsync(Platform::String ^ streamName)
{
    QCC_DbgPrintf(("MEdiaSink::CloseStream()"));
    IAsyncAction ^ action = create_async([this, streamName]() {
                                             CloseStream(streamName);
                                         });
    return action;
}

void MediaSink::CloseStream(Platform::String ^ streamName)
{
    QStatus status = ER_OK;
    _internal->lock.Lock();
    AllJoyn::ProxyBusObject ^ streamProxy = GetStreamProxy(streamName);

    if (nullptr != streamProxy) {
        SinkInfo& info = _internal->streamProxies[streamProxy];
        if (nullptr != info.socket) {
            info.socket = nullptr;
            IAsyncOperation<AllJoyn::MethodCallResult ^> ^ op = streamProxy->MethodCallAsync(MediaCommon::GetStreamIfc()->GetMember(L"Close"), nullptr, nullptr, 10 * 1000, 0);
            concurrency::task<AllJoyn::MethodCallResult ^> closeTask(op);
            closeTask.wait();
            _internal->Inform(&info, Internal::INFORM_CLOSE);
            info.socket = nullptr;
            info.paused = true;
        }
    } else {
        status = ER_MEDIA_STREAM_NOT_FOUND;
    }
    _internal->lock.Unlock();
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void MediaSink::Close()
{
    _internal->lock.Lock();
    for (std::map<AllJoyn::ProxyBusObject ^, SinkInfo>::iterator iter = _internal->streamProxies.begin(); iter != _internal->streamProxies.end(); iter++) {
        SinkInfo& info = iter->second;
        if (nullptr != info.socket) {
            CloseStream(info.description->StreamName);
            info.socket = nullptr;
        }
    }
    _internal->Close();
    _internal->lock.Unlock();
}

IAsyncAction ^ MediaSink::CloseAsync()
{
    QCC_DbgPrintf(("MEdiaSink::Close()"));
    /*
     * Call CloseStream on all open streams
     */
    IAsyncAction ^ action = create_async([this] {
                                             Close();
                                         });
    return action;
}

void MediaSink::Play()
{
    QStatus status = ER_OK;
    size_t playCount = 0;
    /*
     * Call play on all open streams
     */
    _internal->lock.Lock();
    for (std::map<AllJoyn::ProxyBusObject ^, SinkInfo>::iterator iter = _internal->streamProxies.begin(); iter != _internal->streamProxies.end(); iter++) {
        AllJoyn::ProxyBusObject ^ streamProxy = iter->first;
        SinkInfo& info = iter->second;
        if (nullptr != info.socket) {
            IAsyncOperation<AllJoyn::MethodCallResult ^> ^ op = streamProxy->MethodCallAsync(MediaCommon::GetStreamIfc()->GetMember(L"Play"), nullptr, nullptr, 10 * 1000, 0);
            concurrency::task<AllJoyn::MethodCallResult ^> playTask(op);
            playTask.wait();
            AllJoyn::MethodCallResult ^ playResult = playTask.get();
            AllJoyn::Message ^ reply = playResult->Message;
            Platform::IBox<Platform::Boolean> ^ r = dynamic_cast<Platform::IBox<Platform::Boolean> ^>(reply->GetArg(0)->Value);
            bool success = r->Value;
            if (success) {
                status = _internal->Inform(&info, Internal::INFORM_PLAY);
            } else {
                QCC_DbgPrintf(("Play returned false"));
            }
            ++playCount;
        }
    }
    _internal->lock.Unlock();
    if (playCount == 0) {
        status = ER_MEDIA_NO_STREAMS_TO_PLAY;
    }
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void MediaSink::Pause(bool drain)
{
    QStatus status = ER_OK;
    size_t pauseCount = 0;
    /*
     * Call pause on all open streams
     */
    _internal->lock.Lock();
    for (std::map<AllJoyn::ProxyBusObject ^, SinkInfo>::iterator iter = _internal->streamProxies.begin(); iter != _internal->streamProxies.end(); iter++) {
        AllJoyn::ProxyBusObject ^ streamProxy = iter->first;
        SinkInfo& info = iter->second;
        if ((nullptr != info.socket) && info.description->Pausable) {
            IAsyncOperation<AllJoyn::MethodCallResult ^> ^ op = streamProxy->MethodCallAsync(MediaCommon::GetStreamIfc()->GetMember(L"Pause"), nullptr, nullptr, 10 * 1000, 0);
            concurrency::task<AllJoyn::MethodCallResult ^> pauseTask(op);
            pauseTask.wait();
            AllJoyn::MethodCallResult ^ pauseResult = pauseTask.get();
            AllJoyn::Message ^ reply = pauseResult->Message;
            Platform::IBox<Platform::Boolean> ^ r = dynamic_cast<Platform::IBox<Platform::Boolean> ^>(reply->GetArg(0)->Value);
            bool success = r->Value;
            if (success) {
                if (drain) {
                    /* Read and discard until the read blocks */
                    info.socket->SetBlocking(false);
                    try{
                        while (status == ER_OK) {
                            Platform::Array<uint8> ^ buf = ref new Platform::Array<uint8>(256);
                            Platform::Array<int> ^ received = ref new Platform::Array<int>(1);
                            info.socket->Recv(buf, sizeof(buf), received);
                        }
                    }catch (Platform::Exception ^ e) {
                        QCC_DbgPrintf(("Recv return fail %d", e->HResult));
                        if (AllJoyn::QStatus::ER_WOULDBLOCK == AllJoyn::AllJoynException::GetErrorCode(e->HResult)) {
                            status = ER_OK;
                        } else {
                            status = (QStatus) AllJoyn::AllJoynException::GetErrorCode(e->HResult);
                        }
                    }
                }
                info.socket->SetBlocking(true);
                if (status == ER_OK) {
                    status = _internal->Inform(&info, Internal::INFORM_PAUSE);
                }
            } else {
                QCC_DbgPrintf(("Pause returned false"));
            }
            ++pauseCount;
        }
    }
    _internal->lock.Unlock();
    if (pauseCount == 0) {
        status = ER_MEDIA_NO_STREAMS_TO_PAUSE;
    }
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

IAsyncAction ^ MediaSink::PlayAsync()
{
    QCC_DbgPrintf(("MEdiaSink::PlayAsync()"));
    IAsyncAction ^ action = create_async([this] {
                                             Play();
                                         });
    return action;
}

IAsyncAction ^ MediaSink::PauseAsync(bool drain)
{
    QCC_DbgPrintf(("MEdiaSink::PauseAsync()"));
    IAsyncAction ^ action = create_async([this, drain] {
                                             Pause(drain);
                                         });
    return action;
}

static inline bool CanSeekRel(int32_t offset, uint8_t seekable)
{
    if ((offset > 0) && !(seekable & static_cast<uint8_t>(MediaSeekPosition::FORWARDS))) {
        QCC_LogError(ER_MEDIA_SEEK_FAILED, ("Stream does not support seeking forwards"));
        return false;
    }
    if ((offset < 0) && !(seekable & static_cast<uint8_t>(MediaSeekPosition::BACKWARDS))) {
        QCC_LogError(ER_MEDIA_SEEK_FAILED, ("Stream does not support seeking backwards"));
        return false;
    }
    return true;
}

void MediaSink::SeekRelative(int32_t offset, uint8_t units)
{
    QStatus status = ER_OK;
    size_t seekCount = 0;
    /*
     * Call seek on all open streams
     */
    _internal->lock.Lock();
    for (std::map<AllJoyn::ProxyBusObject ^, SinkInfo>::iterator iter = _internal->streamProxies.begin(); iter != _internal->streamProxies.end(); iter++) {
        AllJoyn::ProxyBusObject ^ streamProxy = iter->first;
        SinkInfo& info = iter->second;
        if ((nullptr != info.socket) && CanSeekRel(offset, info.description->Seekable)) {
            Platform::Array<AllJoyn::MsgArg ^> ^ args = ref new Platform::Array<AllJoyn::MsgArg ^>(2);
            args[0] = ref new AllJoyn::MsgArg(L"i", ref new Platform::Array<Platform::Object ^>{ offset });
            args[1] = ref new AllJoyn::MsgArg(L"y", ref new Platform::Array<Platform::Object ^>{ units });
            _internal->Inform(&info, Internal::INFORM_SEEK);
            IAsyncOperation<AllJoyn::MethodCallResult ^> ^ op = streamProxy->MethodCallAsync(MediaCommon::GetStreamIfc()->GetMember(L"SeekRelative"), nullptr, nullptr, 10 * 1000, 0);
            concurrency::task<AllJoyn::MethodCallResult ^> seekrTask(op);
            seekrTask.wait();
            AllJoyn::MethodCallResult ^ seekresult = seekrTask.get();
            AllJoyn::Message ^ reply = seekresult->Message;
            Platform::IBox<Platform::Boolean> ^ r = dynamic_cast<Platform::IBox<Platform::Boolean> ^>(reply->GetArg(0)->Value);
            bool success = r->Value;
            if (success) {
                ++seekCount;
            } else {
                QCC_LogError(status, ("Remote seek method call faied"));
            }
            /*
             * Minimal delay to ensure PLAY comes after PAUSE
             */
            _internal->Inform(&info, Internal::INFORM_PLAY, 1);
        }
    }
    _internal->lock.Unlock();
    if (seekCount == 0) {
        status = ER_MEDIA_SEEK_FAILED;
    }
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}
IAsyncAction ^ MediaSink::SeekRelativeAsync(int32_t offset, uint8_t units)
{
    QCC_DbgPrintf(("MEdiaSink::SeekRelativeAsync(random=%d, units=%d)", offset, units));
    IAsyncAction ^ action = create_async([this, offset, units] {
                                             SeekRelativeAsync(offset, units);
                                         });
    return action;
}

static inline bool CanSeekAbs(uint32_t position, uint8_t seekable)
{
    uint8_t absSeek = seekable & static_cast<uint8_t>(MediaSeekPosition::TO_POSITION);
    if (!absSeek) {
        QCC_LogError(ER_MEDIA_SEEK_FAILED, ("Stream does not support seeking to absolute position"));
        return false;
    }
    if ((position != 0) && (absSeek == static_cast<uint8_t>(MediaSeekPosition::TO_START))) {
        QCC_LogError(ER_MEDIA_SEEK_FAILED, ("Stream only supports seeking to the start"));
        return false;
    }
    return true;
}

void MediaSink::SeekAbsolute(uint32_t position, uint8_t units)
{
    QStatus status = ER_OK;
    size_t seekCount = 0;
    /*
     * Call seek on all open streams
     */
    _internal->lock.Lock();
    for (std::map<AllJoyn::ProxyBusObject ^, SinkInfo>::iterator iter = _internal->streamProxies.begin(); iter != _internal->streamProxies.end(); iter++) {
        AllJoyn::ProxyBusObject ^ streamProxy = iter->first;
        SinkInfo& info = iter->second;
        if ((nullptr != info.socket) && CanSeekAbs(position, info.description->Seekable)) {
            Platform::Array<AllJoyn::MsgArg ^> ^ args = ref new Platform::Array<AllJoyn::MsgArg ^>(2);
            args[0] = ref new AllJoyn::MsgArg(L"u", ref new Platform::Array<Platform::Object ^>{ position });
            args[1] = ref new AllJoyn::MsgArg(L"y", ref new Platform::Array<Platform::Object ^>{ units });
            /*
             * Let renderer know the stream has been interrupted.
             */
            _internal->Inform(&info, Internal::INFORM_SEEK);
            IAsyncOperation<AllJoyn::MethodCallResult ^> ^ op = streamProxy->MethodCallAsync(MediaCommon::GetStreamIfc()->GetMember(L"SeekAbsolute"), args, nullptr, 10 * 1000, 0);
            concurrency::task<AllJoyn::MethodCallResult ^> seekrTask(op);
            seekrTask.wait();
            AllJoyn::MethodCallResult ^ seekresult = seekrTask.get();
            AllJoyn::Message ^ reply = seekresult->Message;
            Platform::IBox<Platform::Boolean> ^ r = dynamic_cast<Platform::IBox<Platform::Boolean> ^>(reply->GetArg(0)->Value);
            bool success = r->Value;
            if (success) {
                ++seekCount;
            } else {
                QCC_LogError(status, ("Remote seek method call faied"));
            }
            /*
             * Minimal delay to ensure PLAY comes after PAUSE
             */
            _internal->Inform(&info, Internal::INFORM_PLAY, 1);
        }
    }
    _internal->lock.Unlock();
    if (seekCount == 0) {
        status = ER_MEDIA_SEEK_FAILED;
    }
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}
IAsyncAction ^ MediaSink::SeekAbsoluteAsync(uint32_t position, uint8_t units)
{
    IAsyncAction ^ action = create_async([this, position, units] {
                                             SeekAbsolute(position, units);

                                         });
    return action;
}

/*
 * Signal handlers
 */

void MediaSink::StreamOpened(AllJoyn::InterfaceMember ^ member, Platform::String ^ sourcePath, AllJoyn::Message ^ msg)
{
    QCC_DbgPrintf(("MediaSink::StreamOpened() signal"));
    if (nullptr != _internal->mediaSource) {
        AllJoyn::ProxyBusObject ^ proxy = GetStreamProxy(StreamName(sourcePath));
        if (nullptr != proxy) {
            _internal->lock.Lock();
            SinkInfo* info = _internal->GetSinkInfo(proxy);
            if (info && info->openEvent != INVALID_HANDLE_VALUE) {
                info->status = ER_OK;
                ::SetEvent(info->openEvent);
                info->openEvent = INVALID_HANDLE_VALUE;
            }
            _internal->lock.Unlock();
        }
    }
}

void MediaSink::StreamClosed(AllJoyn::InterfaceMember ^ member, Platform::String ^ sourcePath, AllJoyn::Message ^ msg)
{
    QCC_DbgPrintf(("Received StreamClosed signal"));
    if (nullptr != _internal->mediaSource) {
        AllJoyn::ProxyBusObject ^ proxy = GetStreamProxy(StreamName(sourcePath));
        if (nullptr == proxy) {
            return;
        }
        _internal->lock.Lock();
        SinkInfo* info = _internal->GetSinkInfo(proxy);
        if (!info) {
            _internal->lock.Unlock();
            return;
        }
        if (info->openEvent != INVALID_HANDLE_VALUE) {
            /*
             * Stream closed before it was opened - set the opeEvent to unblock the thread
             * waiting for the open to complete.
             */
            info->status = ER_MEDIA_STREAM_OPEN_FAILED;
            ::SetEvent(info->openEvent);
            info->openEvent = INVALID_HANDLE_VALUE;
        } else {
            info->status = _internal->Inform(info, Internal::INFORM_CLOSE);
        }
        info->socket = nullptr;
        _internal->lock.Unlock();
    }
}

void MediaSink::StreamPaused(AllJoyn::InterfaceMember ^ member, Platform::String ^ sourcePath, AllJoyn::Message ^ msg)
{
    QCC_DbgPrintf(("Received StreamPaused signal"));
    if (nullptr != _internal->mediaSource) {
        AllJoyn::ProxyBusObject ^ proxy = GetStreamProxy(StreamName(sourcePath));
        if (nullptr != proxy) {
            _internal->lock.Lock();
            SinkInfo* info = _internal->GetSinkInfo(proxy);
            if (info && (info->socket != nullptr)) {
                _internal->Inform(info, Internal::INFORM_PAUSE);
            }
            _internal->lock.Unlock();
        }
    }
}

void MediaSink::StreamPlaying(AllJoyn::InterfaceMember ^ member, Platform::String ^ sourcePath, AllJoyn::Message ^ msg)
{
    QCC_DbgPrintf(("Received StreamPlaying signal"));
    if (nullptr != _internal->mediaSource) {
        AllJoyn::ProxyBusObject ^ proxy = GetStreamProxy(StreamName(sourcePath));
        if (nullptr != proxy) {
            _internal->lock.Lock();
            SinkInfo* info = _internal->GetSinkInfo(proxy);
            if (info && (nullptr != info->socket)) {
                _internal->Inform(info, Internal::INFORM_PLAY);
            }
            _internal->lock.Unlock();
        }
    }
}

bool MediaSink::IsPaused(Platform::String ^ streamName)
{
    AllJoyn::ProxyBusObject ^ streamProxy = GetStreamProxy(streamName);
    if (nullptr != streamProxy) {
        return _internal->streamProxies[streamProxy].paused;
    } else {
        return true;
    }
}

bool MediaSink::IsOpen(Platform::String ^ streamName)
{
    AllJoyn::ProxyBusObject ^ streamProxy = GetStreamProxy(streamName);
    if (nullptr != streamProxy) {
        return _internal->streamProxies[streamProxy].socket != nullptr;
    } else {
        return false;
    }
}

void MediaSink::GetDescription(Platform::String ^ streamName, Platform::WriteOnlyArray<MediaDescription ^> ^ description)
{
    QStatus status = ER_MEDIA_STREAM_NOT_FOUND;
    AllJoyn::ProxyBusObject ^ streamProxy = GetStreamProxy(streamName);
    if (nullptr != streamProxy) {
        MediaDescription ^ d = ref new MediaDescription();
        GetStreamProperties(streamProxy, d);
        description[0] = d;
    }
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

}

