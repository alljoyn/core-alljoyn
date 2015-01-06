/******************************************************************************
 * Copyright (c) 2014,2015, AllSeen Alliance. All rights reserved.
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
#include "AutoPingerInternal.h"
#include <alljoyn/AutoPinger.h>

namespace ajn {

static int autoPingerCounter = 0;
bool AutoPingerInit::cleanedup = false;
AutoPingerInit::AutoPingerInit()
{
    if (autoPingerCounter++ == 0) {
        AutoPingerInternal::Init();
    }
}

AutoPingerInit::~AutoPingerInit()
{
    if (--autoPingerCounter == 0 && !cleanedup) {
        AutoPingerInternal::Cleanup();
        cleanedup = true;
    }
}

void AutoPingerInit::Cleanup()
{
    if (!cleanedup) {
        AutoPingerInternal::Cleanup();
        cleanedup = true;
    }
}

AutoPinger::AutoPinger(ajn::BusAttachment& _busAttachment) :
    internal(new AutoPingerInternal(_busAttachment))
{
}

AutoPinger::~AutoPinger()
{
    delete internal;
}

void AutoPinger::Pause()
{
    internal->Pause();
}

void AutoPinger::Resume()
{
    internal->Resume();
}

void AutoPinger::AddPingGroup(const qcc::String& group, PingListener& listener, uint32_t pingInterval)
{
    internal->AddPingGroup(group, listener, pingInterval);
}

void AutoPinger::RemovePingGroup(const qcc::String& group)
{
    internal->RemovePingGroup(group);
}

QStatus AutoPinger::SetPingInterval(const qcc::String& group, uint32_t pingInterval)
{
    return internal->SetPingInterval(group, pingInterval);
}

QStatus AutoPinger::AddDestination(const qcc::String& group, const qcc::String& destination)
{
    return internal->AddDestination(group, destination);
}

QStatus AutoPinger::RemoveDestination(const qcc::String& group, const qcc::String& destination, bool removeAll)
{
    return internal->RemoveDestination(group, destination, removeAll);
}

}
