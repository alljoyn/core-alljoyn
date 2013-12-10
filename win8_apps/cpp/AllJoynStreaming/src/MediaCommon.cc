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
#include "MediaCommon.h"

namespace AllJoynStreaming {

AllJoyn::InterfaceDescription ^ MediaCommon::mediaSink;
AllJoyn::InterfaceDescription ^ MediaCommon::mediaSource;
AllJoyn::InterfaceDescription ^ MediaCommon::mediaStream;
AllJoyn::InterfaceDescription ^ MediaCommon::videoProps;
AllJoyn::InterfaceDescription ^ MediaCommon::audioProps;
AllJoyn::InterfaceDescription ^ MediaCommon::imageProps;

static const char* interfaces[] = {

    "<interface name=\"org.alljoyn.MediaSource\">"
    "</interface>",

    "<interface name=\"org.alljoyn.MediaSink\">"
    "   <signal name=\"StreamClosed\">"
    "   </signal>"
    "   <signal name=\"StreamPaused\">"
    "   </signal>"
    "   <signal name=\"StreamPlaying\">"
    "   </signal>"
    "   <signal name=\"StreamOpened\">"
    "   </signal>"
    "</interface>",

    "<interface name=\"org.alljoyn.MediaStream\">"
    "   <method name=\"Close\">"
    "   </method>"
    "   <method name=\"Open\">"
    "      <arg name=\"sessionName\" type=\"s\" direction=\"out\"/>"
    "      <arg name=\"sessionPort\" type=\"q\" direction=\"out\"/>"
    "      <arg name=\"success\" type=\"b\" direction=\"out\"/>"
    "   </method>"
    "   <method name=\"Pause\">"
    "      <arg name=\"success\" type=\"b\" direction=\"out\"/>"
    "   </method>"
    "   <method name=\"Play\">"
    "      <arg name=\"success\" type=\"b\" direction=\"out\"/>"
    "   </method>"
    "   <method name=\"SeekAbsolute\">"
    "      <arg name=\"position\" type=\"u\" direction=\"in\"/>"
    "      <arg name=\"units\" type=\"y\" direction=\"in\"/>"
    "      <arg name=\"success\" type=\"b\" direction=\"out\"/>"
    "   </method>"
    "   <method name=\"SeekRelative\">"
    "      <arg name=\"offset\" type=\"i\" direction=\"in\"/>"
    "      <arg name=\"units\" type=\"y\" direction=\"in\"/>"
    "      <arg name=\"success\" type=\"b\" direction=\"out\"/>"
    "   </method>"
    "   <property name=\"MimeType\" type=\"s\" access=\"read\"/>"
    "   <property name=\"Size\" type=\"t\" access=\"read\"/>"
    "   <property name=\"Seekable\" type=\"y\" access=\"read\"/>"
    "   <property name=\"Pausable\" type=\"b\" access=\"read\"/>"
    "</interface>",

    "<interface name=\"org.alljoyn.MediaStream.Audio\">"
    "   <property name=\"SampleFrequency\" type=\"u\" access=\"read\"/>"
    "   <property name=\"SamplesPerFrame\" type=\"u\" access=\"read\"/>"
    "   <property name=\"BitRate\" type=\"u\" access=\"read\"/>"
    "</interface>",

    "<interface name=\"org.alljoyn.MediaStream.Video\">"
    "   <property name=\"AspectRatio\" type=\"(yy)\" access=\"read\"/>"
    "   <property name=\"FrameRate\" type=\"u\" access=\"read\"/>"
    "   <property name=\"Height\" type=\"q\" access=\"read\"/>"
    "   <property name=\"Width\" type=\"q\" access=\"read\"/>"
    "   <property name=\"BitRate\" type=\"u\" access=\"read\"/>"
    "</interface>",

    "<interface name=\"org.alljoyn.MediaStream.Image\">"
    "   <property name=\"Height\" type=\"q\" access=\"read\"/>"
    "   <property name=\"Width\" type=\"q\" access=\"read\"/>"
    "</interface>"
};

MediaDescription::MediaDescription(Platform::String ^ mimeType, uint64_t size, bool pausable, uint8_t seekable)
{
    MimeType = mimeType;
    Size = size;
    Pausable = pausable;
    Seekable = seekable;
    MType = ResolveMediaType(PlatformToMultibyteString(mimeType));
}

void MediaDescription::SetAudioProperties(uint32_t samplesPerFrame, uint32_t sampleFrequency, uint32_t bitRate)
{
    SamplesPerFrame = samplesPerFrame;
    SampleFrequency = sampleFrequency;
    BitRate = bitRate;
}

void MediaDescription::SetVideoProperties(uint32_t frameRate, uint16_t width, uint16_t height, const Platform::Array<uint8_t> ^ aspectRatio, uint32_t bitRate)
{
}

void MediaDescription::SetImageProperties(uint16_t width, uint16_t height)
{
}

MediaType MediaDescription::ResolveMediaType(const qcc::String& mt)
{
    if (mt.find("audio/") == 0) {
        return MediaType::AUDIO;
    }
    if (mt.find("video/") == 0) {
        return MediaType::VIDEO;
    }
    if (mt.find("image/") == 0) {
        return MediaType::IMAGE;
    }
    if (mt.find("application/") == 0) {
        return MediaType::APPLICATION;
    }
    if (mt.find("text/") == 0) {
        return MediaType::TEXT;
    }
    return MediaType::OTHER;
}

void MediaCommon::CreateInterfaces(AllJoyn::BusAttachment ^ bus)
{
    if (nullptr == mediaSink) {
        uint32 count = sizeof(interfaces) / sizeof(interfaces[0]);
        for (size_t i = 0; i < count; ++i) {
            Platform::String ^ description = MultibyteToPlatformString(interfaces[i]);
            bus->CreateInterfacesFromXml(description);
        }
        mediaSink = bus->GetInterface(L"org.alljoyn.MediaSink");
        mediaSource = bus->GetInterface(L"org.alljoyn.MediaSource");
        mediaStream = bus->GetInterface(L"org.alljoyn.MediaStream");
        videoProps = bus->GetInterface(L"org.alljoyn.MediaStream.Video");
        audioProps = bus->GetInterface(L"org.alljoyn.MediaStream.Audio");
        imageProps = bus->GetInterface(L"org.alljoyn.MediaStream.Image");
    }
    if (mediaSource == nullptr || mediaStream == nullptr || videoProps == nullptr || audioProps == nullptr || imageProps == nullptr) {
        QCC_THROW_EXCEPTION(AllJoyn::QStatus::ER_FAIL);
    }
}

}

