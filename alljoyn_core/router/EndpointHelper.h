/**
 * @file
 * Helper functions for dealing with endpoints
 */

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
#ifndef _ENDPOINT_HELPER_H
#define _ENDPOINT_HELPER_H

#include <qcc/platform.h>

#include "BusEndpoint.h"
#include "RemoteEndpoint.h"
#include "VirtualEndpoint.h"
#include "LocalTransport.h"

namespace ajn {

inline bool operator ==(const BusEndpoint& ep, const VirtualEndpoint& vep) {
    return ep.iden(vep);
}
inline bool operator ==(const VirtualEndpoint& vep, const BusEndpoint& ep) {
    return ep.iden(vep);
}
inline bool operator !=(const BusEndpoint& ep, const VirtualEndpoint& vep) {
    return !ep.iden(vep);
}
inline bool operator !=(const VirtualEndpoint& vep, const BusEndpoint& ep) {
    return !ep.iden(vep);
}

inline bool operator ==(const BusEndpoint& ep, const RemoteEndpoint& rep)  {
    return ep.iden(rep);
}
inline bool operator ==(const RemoteEndpoint& rep, const BusEndpoint& ep)  {
    return ep.iden(rep);
}
inline bool operator !=(const BusEndpoint& ep, const RemoteEndpoint& rep)  {
    return !ep.iden(rep);
}
inline bool operator !=(const RemoteEndpoint& rep, const BusEndpoint& ep)  {
    return !ep.iden(rep);
}

inline bool operator ==(const BusEndpoint& ep, const LocalEndpoint& lep)   {
    return ep.iden(lep);
}
inline bool operator ==(const LocalEndpoint& lep, const BusEndpoint& ep)   {
    return ep.iden(lep);
}
inline bool operator !=(const BusEndpoint& ep, const LocalEndpoint& lep)   {
    return !ep.iden(lep);
}
inline bool operator !=(const LocalEndpoint& lep, const BusEndpoint& ep)   {
    return !ep.iden(lep);
}

}

#endif