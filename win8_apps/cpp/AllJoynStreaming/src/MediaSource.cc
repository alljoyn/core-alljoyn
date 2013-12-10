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

#include <map>

#include <qcc/Mutex.h>
#include <qcc/winrt/utility.h>
#include <ppltasks.h>

#include "MediaSource.h"
#include "Helper.h"

namespace AllJoynStreaming {

class MediaSource::Internal {
  public:

    /*
     * Constructor
     */
    Internal() { }

    /*
     * Destructor
     */
    ~Internal() { }

    /*
     * Configured streams.
     */
    std::map<Platform::String ^, MediaStream ^> streams;

    qcc::Mutex lock;

};

/*
 * Class for a stream subscriber.
 */
ref struct StreamSubscriber sealed {
  public:
    StreamSubscriber() {
        socket = nullptr;
        controlSession = 0;
    }
    property AllJoyn::SocketStream ^ socket;     /* Socket file descriptor for the media stream */
    property uint32_t controlSession; /* Session id for the media control session */
};

class MediaStream::Internal {
  public:

    Internal(AllJoyn::BusAttachment ^  bus, MediaStream ^ mediaStream);
    ~Internal();

    /*
     * Called when a remote media source joins a media stream session.
     */
    bool AcceptSessionJoiner(uint16_t sessionPort, Platform::String ^ joiner, AllJoyn::SessionOpts ^ opts);
    /*
     * Called when a remote media source joins a media stream session.
     */
    void SessionJoined(uint16_t sessionPort, uint32_t id, Platform::String ^ joiner);

    /**
     * Close the media socket if a joiner leaves.
     */
    void NameOwnerChanged(Platform::String ^ busName, Platform::String ^ previousOwner, Platform::String ^ newOwne);

    qcc::Mutex lock;
    std::map<Platform::String ^, StreamSubscriber ^> subscribers;
    Platform::String ^ sessionName;

    AllJoyn::BusListener ^ busListener;
    AllJoyn::SessionPortListener ^ sessionPortLisenter;

  private:
    AllJoyn::BusAttachment ^ bus;
    MediaStream ^ mediaStream;
};

MediaStream::Internal::Internal(AllJoyn::BusAttachment ^  bus, MediaStream ^ mediaStream) : bus(bus), mediaStream(mediaStream)
{
    QStatus status = ER_OK;
    while (true) {
        busListener = ref new AllJoyn::BusListener(bus);
        if (nullptr == busListener) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        sessionPortLisenter = ref new AllJoyn::SessionPortListener(bus);
        if (nullptr == sessionPortLisenter) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        break;
    }
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
    busListener->NameOwnerChanged += ref new AllJoyn::BusListenerNameOwnerChangedHandler([&] (Platform::String ^ busName, Platform::String ^ previousOwner, Platform::String ^ newOwner) {
                                                                                             NameOwnerChanged(busName, previousOwner, newOwner);
                                                                                         });

    sessionPortLisenter->AcceptSessionJoiner += ref new AllJoyn::SessionPortListenerAcceptSessionJoinerHandler([&](uint16_t sessionPort, Platform::String ^ joiner, AllJoyn::SessionOpts ^ opts) {
                                                                                                                   return AcceptSessionJoiner(sessionPort, joiner, opts);
                                                                                                               });
    sessionPortLisenter->SessionJoined += ref new AllJoyn::SessionPortListenerSessionJoinedHandler([&](uint16_t sessionPort, uint32_t id, Platform::String ^ joiner) {
                                                                                                       SessionJoined(sessionPort, id, joiner);
                                                                                                   });
}

MediaStream::Internal::~Internal()
{
    /*
     * Closes any open sockets.
     */
    for (std::map<Platform::String ^, StreamSubscriber ^>::iterator iter = subscribers.begin(); iter != subscribers.end(); ++iter) {
        if (nullptr != iter->second->socket) {
            iter->second->socket = nullptr;
        }
    }
}

void MediaStream::Internal::NameOwnerChanged(Platform::String ^ busName, Platform::String ^ previousOwner, Platform::String ^ newOwner)
{
    QCC_DbgHLPrintf(("MediaStream::Internal::NameOwnerChanged()"));
    if (nullptr != newOwner) {
        bool found = false;
        lock.Lock();
        std::map<Platform::String ^, StreamSubscriber ^>::iterator iter = subscribers.find(busName);
        if (iter != subscribers.end()) {
            found = true;
            iter->second->socket = nullptr;
            subscribers.erase(iter);
        }
        lock.Unlock();
        if (found) {
            mediaStream->OnClose();
        }
    }
}

bool MediaStream::Internal::AcceptSessionJoiner(uint16_t sessionPort, Platform::String ^ joiner, AllJoyn::SessionOpts ^ opts)
{
    QCC_DbgHLPrintf(("Stream joiner %s accepted on port %d", PlatformToMultibyteString(joiner).c_str(), sessionPort));
    return true;
}

void MediaStream::Internal::SessionJoined(uint16_t sessionPort, uint32_t id, Platform::String ^ joiner)
{
    QCC_DbgHLPrintf(("Stream %s joined by %s", PlatformToMultibyteString(mediaStream->GetStreamName()).c_str(),  PlatformToMultibyteString(joiner).c_str()));
    QStatus status = ER_OK;
    bus->EnableConcurrentCallbacks();
    AllJoyn::SocketStream ^ socket = nullptr;
    Platform::Array<AllJoyn::SocketStream ^> ^ ssArray = ref new Platform::Array<AllJoyn::SocketStream ^>(1);
    bus->GetSessionSocketStream(id, ssArray);
    socket = ssArray[0];
    mediaStream->paused = true;
    mediaStream->OnOpen(socket);
    try {
        lock.Lock();
        subscribers[joiner]->socket = socket;
        /*
         * Send a signal to let the media sink know that media is now opened.
         */
        AllJoyn::InterfaceMember ^ signal = MediaCommon::GetSinkIfc()->GetMember(L"StreamOpened");
        mediaStream->StreamBusObject->Signal(joiner, subscribers[joiner]->controlSession, signal, nullptr, 0, 0);
    } catch (Platform::Exception ^ e) {
        status = ER_FAIL;
    }
    lock.Unlock();
    if (status == ER_OK) {
        return;
    }

    /*
     * If we got here the stream was not succesfully opened so signal that it is closed.
     */
    lock.Lock();
    subscribers[joiner]->socket = nullptr;
    AllJoyn::InterfaceMember ^ signal = MediaCommon::GetSinkIfc()->GetMember(L"StreamClosed");
    mediaStream->StreamBusObject->Signal(joiner, subscribers[joiner]->controlSession, signal, nullptr, 0, 0);
    bus->EnableConcurrentCallbacks();
    bus->UnbindSessionPort(sessionPort);
    lock.Unlock();
}

MediaSource::MediaSource(AllJoyn::BusAttachment ^ bus)
{
    QCC_DbgHLPrintf(("MediaSource::MediaSource()"));
    ::QStatus status = ER_OK;
    while (true) {
        if (nullptr == bus) {
            status = ER_BAD_ARG_1;
            break;
        }
        Bus = bus;
        MediaSourceBusObject = ref new AllJoyn::BusObject(bus, L"/org/alljoyn/MediaSource", false);
        if (nullptr == MediaSourceBusObject) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        _internal = new Internal();
        if (NULL == _internal) {
            status = ER_OUT_OF_MEMORY;
            break;
        }

        MediaCommon::CreateInterfaces(bus);
        MediaSourceBusObject->AddInterface(MediaCommon::GetSourceIfc());
        break;
    }

    MediaSourceBusObject->Get += ref new AllJoyn::BusObjectGetHandler([&](Platform::String ^ ifcName, Platform::String ^ propName, Platform::WriteOnlyArray<AllJoyn::MsgArg ^> ^ val) {
                                                                          return Get(ifcName, propName, val);
                                                                      });

    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

MediaSource::~MediaSource()
{
    QCC_DbgHLPrintf(("MediaSource::~MediaSource()"));
    if (_internal) {
        delete _internal;
        _internal = NULL;
    }
}

static inline Platform::String ^ StreamPath(Platform::String ^ name)
{
    return Platform::String::Concat(L"/org/alljoyn/MediaSource/", name);
}

void MediaSource::AddStream(MediaStream ^ mediaStream)
{
    QCC_DbgHLPrintf(("MediaSource::AddStream()"));
    QStatus status = ER_OK;
    _internal->lock.Lock();
    Platform::String ^ streamPath = StreamPath(mediaStream->GetStreamName());
    if (_internal->streams.count(streamPath) != 0) {
        status = ER_MEDIA_STREAM_EXISTS;
    } else {
        Bus->RegisterBusObject(mediaStream->StreamBusObject);
        _internal->streams[streamPath] = mediaStream;
        mediaStream->Source = this;
    }
    _internal->lock.Unlock();

    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void MediaSource::RemoveStream(MediaStream ^ mediaStream)
{
    QCC_DbgHLPrintf(("MediaSource::RemoveStream()"));
    _internal->lock.Lock();
    std::map<Platform::String ^, MediaStream ^>::iterator iter = _internal->streams.find(mediaStream->StreamBusObject->Path);
    if (iter != _internal->streams.end()) {
        _internal->streams.erase(iter);
        Bus->UnregisterBusObject(mediaStream->StreamBusObject);
    }
    _internal->lock.Unlock();
}

AllJoyn::QStatus MediaSource::Get(Platform::String ^ ifcName, Platform::String ^ propName, Platform::WriteOnlyArray<AllJoyn::MsgArg ^> ^ val)
{
    return AllJoyn::QStatus::ER_BUS_NO_SUCH_PROPERTY;
}

MediaStream::MediaStream(AllJoyn::BusAttachment ^ bus, Platform::String ^ name, MediaDescription ^ description)
{
    QCC_DbgHLPrintf(("MediaStream::MediaStream()"));
    StreamBusObject = ref new AllJoyn::BusObject(bus, StreamPath(name), false);
    Description = description;
    Source = nullptr;
    Bus = bus;
    paused = true;

    MediaCommon::CreateInterfaces(bus);
    Description->StreamName = name;
    _internal = new Internal(bus, this);

    StreamBusObject->AddInterface(MediaCommon::GetStreamIfc());
    /* Add media type-specific interfaces */
    switch (Description->MType) {
    case MediaType::AUDIO:
        StreamBusObject->AddInterface(MediaCommon::GetAudioIfc());
        break;

    case MediaType::VIDEO:
        StreamBusObject->AddInterface(MediaCommon::GetVideoIfc());
        break;

    case MediaType::IMAGE:
        StreamBusObject->AddInterface(MediaCommon::GetImageIfc());
        break;

    case MediaType::APPLICATION:
    case MediaType::TEXT:
    case MediaType::OTHER:
        break;
    }

    AllJoyn::MessageReceiver ^ openReceiver = ref new AllJoyn::MessageReceiver(bus);
    openReceiver->MethodHandler += ref new AllJoyn::MessageReceiverMethodHandler([&] (AllJoyn::InterfaceMember ^ member, AllJoyn::Message ^ message) {
                                                                                     OpenHandler(member, message);
                                                                                 });
    StreamBusObject->AddMethodHandler(MediaCommon::GetStreamIfc()->GetMember(L"Open"), openReceiver);

    AllJoyn::MessageReceiver ^ closeReceiver = ref new AllJoyn::MessageReceiver(bus);
    closeReceiver->MethodHandler += ref new AllJoyn::MessageReceiverMethodHandler([&] (AllJoyn::InterfaceMember ^ member, AllJoyn::Message ^ message) {
                                                                                      CloseHandler(member, message);
                                                                                  });
    StreamBusObject->AddMethodHandler(MediaCommon::GetStreamIfc()->GetMember(L"Close"), closeReceiver);

    AllJoyn::MessageReceiver ^ playReceiver = ref new AllJoyn::MessageReceiver(bus);
    playReceiver->MethodHandler += ref new AllJoyn::MessageReceiverMethodHandler([&] (AllJoyn::InterfaceMember ^ member, AllJoyn::Message ^ message) {
                                                                                     PlayHandler(member, message);
                                                                                 });
    StreamBusObject->AddMethodHandler(MediaCommon::GetStreamIfc()->GetMember(L"Play"), playReceiver);

    AllJoyn::MessageReceiver ^ pauseReceiver = ref new AllJoyn::MessageReceiver(bus);
    pauseReceiver->MethodHandler += ref new AllJoyn::MessageReceiverMethodHandler([&] (AllJoyn::InterfaceMember ^ member, AllJoyn::Message ^ message) {
                                                                                      PauseHandler(member, message);
                                                                                  });
    StreamBusObject->AddMethodHandler(MediaCommon::GetStreamIfc()->GetMember(L"Pause"), pauseReceiver);

    AllJoyn::MessageReceiver ^ seekRelativeReceiver = ref new AllJoyn::MessageReceiver(bus);
    seekRelativeReceiver->MethodHandler += ref new AllJoyn::MessageReceiverMethodHandler([&] (AllJoyn::InterfaceMember ^ member, AllJoyn::Message ^ message) {
                                                                                             SeekRelativeHandler(member, message);
                                                                                         });
    StreamBusObject->AddMethodHandler(MediaCommon::GetStreamIfc()->GetMember(L"SeekRelative"), seekRelativeReceiver);

    AllJoyn::MessageReceiver ^ seekAbsoluteReceiver = ref new AllJoyn::MessageReceiver(bus);
    seekAbsoluteReceiver->MethodHandler += ref new AllJoyn::MessageReceiverMethodHandler([&] (AllJoyn::InterfaceMember ^ member, AllJoyn::Message ^ message) {
                                                                                             SeekAbsoluteHandler(member, message);
                                                                                         });
    StreamBusObject->AddMethodHandler(MediaCommon::GetStreamIfc()->GetMember(L"SeekAbsolute"), seekAbsoluteReceiver);

    StreamBusObject->Get +=  ref new AllJoyn::BusObjectGetHandler([&](Platform::String ^ ifcName, Platform::String ^ propName, Platform::WriteOnlyArray<AllJoyn::MsgArg ^> ^ val) {
                                                                      return Get(ifcName, propName, val);
                                                                  });
    bus->RegisterBusListener(_internal->busListener);
}

AllJoyn::QStatus MediaStream::Get(Platform::String ^ ifcName, Platform::String ^ propName, Platform::WriteOnlyArray<AllJoyn::MsgArg ^> ^ val)
{
    AllJoyn::QStatus status = AllJoyn::QStatus::ER_OK;

    if (ifcName->Equals(L"org.alljoyn.MediaStream")) {
        if (propName->Equals(L"MimeType")) {
            val[0] = ref new AllJoyn::MsgArg(L"s", ref new Platform::Array<Platform::Object ^>{ Description->MimeType });
        } else if (propName->Equals(L"Seekable")) {
            val[0] = ref new AllJoyn::MsgArg(L"y", ref new Platform::Array<Platform::Object ^>{ Description->Seekable });
        } else if (propName->Equals(L"Pausable")) {
            val[0] = ref new AllJoyn::MsgArg(L"b", ref new Platform::Array<Platform::Object ^>{ Description->Pausable });
        } else if (propName->Equals(L"Size")) {
            val[0] = ref new AllJoyn::MsgArg(L"t", ref new Platform::Array<Platform::Object ^>{ Description->Size });
        } else {
            status = AllJoyn::QStatus::ER_BUS_NO_SUCH_PROPERTY;
        }
    } else if (ifcName->Equals(L"org.alljoyn.MediaStream.Audio")) {
        if (propName->Equals(L"SamplesPerFrame")) {
            val[0] = ref new AllJoyn::MsgArg(L"u", ref new Platform::Array<Platform::Object ^>{ Description->SamplesPerFrame });
        } else if (propName->Equals(L"SampleFrequency")) {
            val[0] = ref new AllJoyn::MsgArg(L"u", ref new Platform::Array<Platform::Object ^>{ Description->SampleFrequency });
        } else if (propName->Equals(L"BitRate")) {
            val[0] = ref new AllJoyn::MsgArg(L"u", ref new Platform::Array<Platform::Object ^>{ Description->BitRate });
        } else {
            status = AllJoyn::QStatus::ER_BUS_NO_SUCH_PROPERTY;
        }
    } else if (ifcName->Equals(L"org.alljoyn.MediaStream.Video")) {
        if (propName->Equals(L"Width")) {
            val[0] = ref new AllJoyn::MsgArg(L"q", ref new Platform::Array<Platform::Object ^>{ Description->Width });
        } else if (propName->Equals(L"Height")) {
            val[0] = ref new AllJoyn::MsgArg(L"q", ref new Platform::Array<Platform::Object ^>{ Description->Height });
        } else if (propName->Equals(L"FrameRate")) {
            val[0] = ref new AllJoyn::MsgArg(L"u", ref new Platform::Array<Platform::Object ^>{ Description->FrameRate });
        } else if (propName->Equals(L"AspectRatio")) {
            val[0] = ref new AllJoyn::MsgArg(L"yy", ref new Platform::Array<Platform::Object ^>{ Description->AspectRatio0, Description->AspectRatio1 });
        } else if (propName->Equals(L"BitRate")) {
            val[0] = ref new AllJoyn::MsgArg(L"u", ref new Platform::Array<Platform::Object ^>{ Description->BitRate });
        } else {
            status = AllJoyn::QStatus::ER_BUS_NO_SUCH_PROPERTY;
        }
    } else if (ifcName->Equals(L"org.alljoyn.MediaStream.Image")) {
        if (propName->Equals(L"Width")) {
            val[0] = ref new AllJoyn::MsgArg(L"q", ref new Platform::Array<Platform::Object ^>{ Description->Width });
        } else if (propName->Equals(L"Height")) {
            val[0] = ref new AllJoyn::MsgArg(L"q", ref new Platform::Array<Platform::Object ^>{ Description->Height });
        } else {
            status = AllJoyn::QStatus::ER_BUS_NO_SUCH_PROPERTY;
        }
    } else {
        status = AllJoyn::QStatus::ER_BUS_NO_SUCH_PROPERTY;
    }
    return status;
}

MediaStream::~MediaStream()
{
    QCC_DbgHLPrintf(("MediaStream::~MediaStream()"));
    if (_internal) {
        delete _internal;
        _internal = NULL;
    }
}

void MediaStream::Pause()
{
    QCC_DbgHLPrintf(("MediaStream::Pause()"));
    QStatus status = ER_OK;
    _internal->lock.Lock();
    if (IsOpen()) {
        if (!paused) {
            /*
             * Send a signal to let each sink know that the media is paused.
             */
            AllJoyn::InterfaceMember ^ signal = MediaCommon::GetSinkIfc()->GetMember(L"StreamPaused");
            std::map<Platform::String ^, StreamSubscriber ^>::iterator iter = _internal->subscribers.begin();
            while (iter != _internal->subscribers.end()) {
                StreamBusObject->Signal(iter->first, iter->second->controlSession, signal, nullptr, 0, 0);
                ++iter;
            }
            paused = true;
        }
        status = ER_OK;
    } else {
        status = ER_MEDIA_STREAM_CLOSED;
    }
    _internal->lock.Unlock();
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void MediaStream::Play()
{
    QCC_DbgHLPrintf(("MediaStream::Play()"));
    QStatus status = ER_OK;
    _internal->lock.Lock();
    if (IsOpen()) {
        if (paused) {
            /*
             * Send a signal to let each sink know that media is playing.
             */
            AllJoyn::InterfaceMember ^ signal = MediaCommon::GetSinkIfc()->GetMember(L"StreamPlaying");
            std::map<Platform::String ^, StreamSubscriber ^>::iterator iter = _internal->subscribers.begin();
            while (iter != _internal->subscribers.end()) {
                StreamBusObject->Signal(iter->first, iter->second->controlSession, signal, nullptr, 0, 0);
                ++iter;
            }
            paused = false;
        }
        status = ER_OK;
    } else {
        status = ER_MEDIA_STREAM_CLOSED;
    }
    _internal->lock.Unlock();
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

void MediaStream::Close()
{
    QCC_DbgHLPrintf(("MediaStream::Close()"));
    QStatus status = ER_OK;
    _internal->lock.Lock();
    if (IsOpen()) {
        /*
         * Send a signal to let each sink know that media is being closed.
         */
        AllJoyn::InterfaceMember ^ signal = MediaCommon::GetSinkIfc()->GetMember(L"StreamClosed");
        std::map<Platform::String ^, StreamSubscriber ^>::iterator iter = _internal->subscribers.begin();
        while (iter != _internal->subscribers.end()) {
            StreamBusObject->Signal(iter->first, iter->second->controlSession, signal, nullptr, 0, 0);
            ++iter;
        }
        status = ER_OK;
    } else {
        status = ER_MEDIA_STREAM_CLOSED;
    }
    _internal->lock.Unlock();

    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

Platform::String ^ MediaStream::GetStreamName()
{
    return Description->StreamName;
}

MediaDescription ^ MediaStream::GetDescription()
{
    return Description;
}

void MediaStream::OpenHandler(AllJoyn::InterfaceMember ^ member, AllJoyn::Message ^ message)
{
    QCC_DbgHLPrintf(("MediaStream::OpenHandler()"));
    Concurrency::task<void> openTask([ = ] {
                                         try {
                                             /*
                                              * Allocate a session port number for the new stream.
                                              */
                                             AllJoyn::SessionOpts ^ opts = ref new AllJoyn::SessionOpts(AllJoyn::TrafficType::TRAFFIC_RAW_RELIABLE, false, AllJoyn::ProximityType::PROXIMITY_ANY, AllJoyn::TransportMaskType::TRANSPORT_ANY);
                                             uint16_t port = (uint16_t) AllJoyn::SessionPortType::SESSION_PORT_ANY;
                                             _internal->sessionName = Bus->UniqueName;
                                             /*
                                              * Add the requester as a subscriber to this stream.
                                              */
                                             QStatus status = ER_OK;
                                             Platform::String ^ sender = message->Sender;
                                             StreamSubscriber ^ subscriber = _internal->subscribers[sender];
                                             if (subscriber == nullptr) {
                                                 subscriber = ref new StreamSubscriber();
                                                 _internal->subscribers[sender] = subscriber;
                                             }
                                             /*
                                              * The control session will be used for all communication back to the subscriber.
                                              */
                                             if (subscriber->controlSession == 0) {
                                                 subscriber->controlSession = message->SessionId;
                                                 Platform::Array<uint16_t> ^ portOut = ref new Platform::Array<uint16_t>(1);
                                                 Bus->BindSessionPort(port, portOut, opts, _internal->sessionPortLisenter);
                                                 port = portOut[0];
                                             } else {
                                                 status = ER_MEDIA_STREAM_BUSY;
                                                 QCC_LogError(status, ("Stream is already open"));
                                             }
                                             if (status != ER_OK) {
                                                 QCC_LogError(status, ("Failed to open stream %s", PlatformToMultibyteString(StreamBusObject->Path).c_str()));
                                                 StreamBusObject->MethodReplyWithQStatus(message, (AllJoyn::QStatus)status);
                                             } else {
                                                 /*
                                                  * Return the session name and port to the audio sink.
                                                  */
                                                 Platform::Array<AllJoyn::MsgArg ^> ^ args = ref new Platform::Array<AllJoyn::MsgArg ^>(3);
                                                 args[0] = ref new AllJoyn::MsgArg(L"s", ref new Platform::Array<Platform::Object ^> { _internal->sessionName });
                                                 args[1] = ref new AllJoyn::MsgArg(L"q", ref new Platform::Array<Platform::Object ^> { port });
                                                 args[2] = ref new AllJoyn::MsgArg(L"b", ref new Platform::Array<Platform::Object ^> { true });
                                                 StreamBusObject->MethodReply(message, args);
                                             }
                                             /*
                                              * Unbind the session and other cleanup if we got an error.
                                              */
                                             if (status != ER_OK) {
                                                 _internal->subscribers.erase(sender);
                                                 // We did not advertise name, so don't need to CancelAdvertiseName()
                                                 Bus->EnableConcurrentCallbacks();
                                                 Bus->UnbindSessionPort(port);
                                             }
                                         } catch (Platform::Exception ^ e) {
                                             auto message = e->Message;
                                             auto hresult = e->HResult;
                                         }
                                     });
}

void MediaStream::CloseHandler(AllJoyn::InterfaceMember ^ member, AllJoyn::Message ^ message)
{
    QCC_DbgHLPrintf(("MediaStream::CloseHandler()"));
    AllJoyn::SocketStream ^ socket = nullptr;

    _internal->lock.Lock();
    bool found = false;
    std::map<Platform::String ^, StreamSubscriber ^>::iterator iter = _internal->subscribers.find(message->Sender);
    if (iter != _internal->subscribers.end()) {
        found = true;
        _internal->subscribers.erase(iter);
    }
    if (found) {
        paused = true;
        _internal->lock.Unlock();
        OnClose();
        StreamBusObject->MethodReply(message, nullptr);
    } else {
        _internal->lock.Unlock();
        StreamBusObject->MethodReply(message, L"org.alljoyn.MediaStream.Error", L"Stream is not open");
    }
}

void MediaStream::PlayHandler(AllJoyn::InterfaceMember ^ member, AllJoyn::Message ^ message)
{
    QCC_DbgHLPrintf(("MediaStream::PlayHandler()"));
    _internal->lock.Lock();
    if (IsOpen()) {
        Platform::Array<AllJoyn::MsgArg ^> ^ args = ref new Platform::Array<AllJoyn::MsgArg ^>(1);
        args[0] = ref new AllJoyn::MsgArg(L"b", ref new Platform::Array<Platform::Object ^>{ paused ? OnPlay() : true });
        StreamBusObject->MethodReply(message, args);
        paused = false;
    } else {
        StreamBusObject->MethodReply(message, L"org.alljoyn.MediaStream.Error", L"Stream is not open");
    }
    _internal->lock.Unlock();
}

void MediaStream::PauseHandler(AllJoyn::InterfaceMember ^ member, AllJoyn::Message ^ message)
{
    QCC_DbgHLPrintf(("MediaStream::PauseHandler()"));
    _internal->lock.Lock();
    if (IsOpen()) {
        if (!Description->Pausable) {
            StreamBusObject->MethodReply(message, L"org.alljoyn.MediaStream.Error", L"Stream is not pausable");
        } else {
            Platform::Array<AllJoyn::MsgArg ^> ^ args = ref new Platform::Array<AllJoyn::MsgArg ^>(1);
            args[0] = ref new AllJoyn::MsgArg(L"b", ref new Platform::Array<Platform::Object ^>{ !paused ? OnPause() : true });
            StreamBusObject->MethodReply(message, args);
            paused = true;
        }
    } else {
        StreamBusObject->MethodReply(message, L"org.alljoyn.MediaStream.Error", L"Stream is not open");
    }
    _internal->lock.Unlock();
}

void MediaStream::SeekRelativeHandler(AllJoyn::InterfaceMember ^ member, AllJoyn::Message ^ message)
{
    QCC_DbgHLPrintf(("MediaStream::SeekRelativeHandler()"));
    _internal->lock.Lock();
    if (IsOpen()) {
        Platform::IBox<int32_t> ^ o = dynamic_cast<Platform::IBox<int32_t> ^>(message->GetArg(0)->Value);
        int32_t offset = o->Value;
        Platform::IBox<uint8_t> ^ u = dynamic_cast<Platform::IBox<uint8_t> ^>(message->GetArg(1)->Value);
        uint8_t units = u->Value;
        if ((offset > 0) && !(Description->Seekable & (uint8_t) MediaSeekPosition::FORWARDS)) {
            StreamBusObject->MethodReply(message, L"org.alljoyn.MediaStream.Error", L"Stream does support seeking forwards");
        } else if ((offset < 0) && !(Description->Seekable & (uint8_t) MediaSeekPosition::BACKWARDS)) {
            StreamBusObject->MethodReply(message, L"org.alljoyn.MediaStream.Error", L"Stream does support seeking backwards");
        } else {
            Platform::Array<AllJoyn::MsgArg ^> ^ args = ref new Platform::Array<AllJoyn::MsgArg ^>(1);
            args[0] = ref new AllJoyn::MsgArg(L"b", ref new Platform::Array<Platform::Object ^>{ OnSeekRelative(offset, (MediaSeekUnits)units) });
            StreamBusObject->MethodReply(message, args);
            paused = false;
        }
    } else {
        StreamBusObject->MethodReply(message, L"org.alljoyn.MediaStream.Error", L"Stream is not open");
    }
    _internal->lock.Unlock();
}

void MediaStream::SeekAbsoluteHandler(AllJoyn::InterfaceMember ^ member, AllJoyn::Message ^ message)
{
    QCC_DbgHLPrintf(("MediaStream::SeekAbsoluteHandler()"));
    _internal->lock.Lock();
    if (IsOpen()) {
        uint8_t absSeek = Description->Seekable & (uint8_t) MediaSeekPosition::TO_POSITION;
        Platform::IBox<uint32_t> ^ p = dynamic_cast<Platform::IBox<uint32_t> ^>(message->GetArg(0)->Value);
        uint32_t position = p->Value;
        Platform::IBox<uint8_t> ^ u = dynamic_cast<Platform::IBox<uint8_t> ^>(message->GetArg(1)->Value);
        uint8_t units = u->Value;
        if (!absSeek) {
            StreamBusObject->MethodReply(message, "org.alljoyn.MediaStream.Error", "Stream does support seeking to an absolute position");
        } else if ((position != 0) && (absSeek == (uint8_t) MediaSeekPosition::TO_START)) {
            StreamBusObject->MethodReply(message, "org.alljoyn.MediaStream.Error", "Stream only supports seeking to the start");
        } else {
            Platform::Array<AllJoyn::MsgArg ^> ^ args = ref new Platform::Array<AllJoyn::MsgArg ^>(1);
            args[0] = ref new AllJoyn::MsgArg(L"b", ref new Platform::Array<Platform::Object ^>{ OnSeekAbsolute(position, (MediaSeekUnits)units) });
            StreamBusObject->MethodReply(message, args);
            paused = false;
        }
    } else {
        StreamBusObject->MethodReply(message, L"org.alljoyn.MediaStream.Error", L"Stream is not open");
    }
    _internal->lock.Unlock();
}

bool MediaStream::IsOpen()
{
    _internal->lock.Lock();
    bool open = !_internal->subscribers.empty();
    _internal->lock.Unlock();
    return open;
}

}

