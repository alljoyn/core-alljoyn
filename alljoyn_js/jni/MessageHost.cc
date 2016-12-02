/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 */
#include "MessageHost.h"

#include "TypeMapping.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

_MessageHost::_MessageHost(Plugin& plugin, BusAttachment& busAttachment, ajn::Message& message) :
    ScriptableObject(plugin, _MessageInterface::Constants()),
    busAttachment(busAttachment),
    message(message)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    ATTRIBUTE("sender", &_MessageHost::getSender, 0);
    ATTRIBUTE("destination", &_MessageHost::getDestination, 0);
    ATTRIBUTE("flags", &_MessageHost::getFlags, 0);
    ATTRIBUTE("interfaceName", &_MessageHost::getInterfaceName, 0);
    ATTRIBUTE("objectPath", &_MessageHost::getObjectPath, 0);
    ATTRIBUTE("authMechanism", &_MessageHost::getAuthMechanism, 0);
    ATTRIBUTE("isUnreliable", &_MessageHost::getIsUnreliable, 0);
    ATTRIBUTE("memberName", &_MessageHost::getMemberName, 0);
    ATTRIBUTE("signature", &_MessageHost::getSignature, 0);
    ATTRIBUTE("sessionId", &_MessageHost::getSessionId, 0);
}

_MessageHost::~_MessageHost()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

bool _MessageHost::getSender(NPVariant* result)
{
    ToDOMString(plugin, message->GetSender(), *result, TreatEmptyStringAsUndefined);
    return true;
}

bool _MessageHost::getDestination(NPVariant* result)
{
    ToDOMString(plugin, message->GetDestination(), *result, TreatEmptyStringAsUndefined);
    return true;
}

bool _MessageHost::getFlags(NPVariant* result)
{
    ToUnsignedLong(plugin, message->GetFlags(), *result);
    return true;
}

bool _MessageHost::getInterfaceName(NPVariant* result)
{
    ToDOMString(plugin, message->GetInterface(), *result, TreatEmptyStringAsUndefined);
    return true;
}

bool _MessageHost::getObjectPath(NPVariant* result)
{
    ToDOMString(plugin, message->GetObjectPath(), *result, TreatEmptyStringAsUndefined);
    return true;
}

bool _MessageHost::getAuthMechanism(NPVariant* result)
{
    ToDOMString(plugin, message->GetAuthMechanism(), *result, TreatEmptyStringAsUndefined);
    return true;
}

bool _MessageHost::getIsUnreliable(NPVariant* result)
{
    ToBoolean(plugin, message->IsUnreliable(), *result);
    return true;
}

bool _MessageHost::getMemberName(NPVariant* result)
{
    ToDOMString(plugin, message->GetMemberName(), *result, TreatEmptyStringAsUndefined);
    return true;
}

bool _MessageHost::getSignature(NPVariant* result)
{
    ToDOMString(plugin, message->GetSignature(), *result, TreatEmptyStringAsUndefined);
    return true;
}

bool _MessageHost::getSessionId(NPVariant* result)
{
    ToUnsignedLong(plugin, message->GetSessionId(), *result);
    return true;
}

bool _MessageHost::getTimestamp(NPVariant* result)
{
    ToUnsignedLong(plugin, message->GetTimeStamp(), *result);
    return true;
}