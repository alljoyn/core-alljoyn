/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/
#include "AutoPingerInternal.h"
#include <alljoyn/AutoPinger.h>

namespace ajn {

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