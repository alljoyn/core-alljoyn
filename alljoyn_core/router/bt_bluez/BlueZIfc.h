/**
 * @file
 * org.bluez interface table definitions
 */

/******************************************************************************
 * Copyright (c) 2009-2011, 2014, AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_BLUEZIFC_H
#define _ALLJOYN_BLUEZIFC_H

#include <qcc/platform.h>

#include <alljoyn/Message.h>


namespace ajn {
namespace bluez {

struct InterfaceDesc {
    AllJoynMessageType type;
    const char* name;
    const char* inputSig;
    const char* outSig;
    const char* argNames;
    uint8_t annotation;
};

struct InterfaceTable {
    const char* ifcName;
    const InterfaceDesc* desc;
    size_t tableSize;
};

#define ALLJOYN_UUID "414a0000-1c25-481f-9dfb-59193d238280"

extern const InterfaceDesc bzObjMgrIfcTbl[];
extern const InterfaceDesc bzAdapter1IfcTbl[];
extern const InterfaceDesc bzDevice1IfcTbl[];
extern const InterfaceDesc bzAllJoynMgrIfcTbl[];
extern const InterfaceDesc bzAllJoynIfcTbl[];
extern const char* bzObjMgrIfc;
extern const char* bzAdapter1Ifc;
extern const char* bzAllJoynIfc;
extern const char* bzDevice1Ifc;

extern const InterfaceTable ifcTables[];
extern const size_t ifcTableSize;

extern const char* bzBusName;
extern const char* bzMgrObjPath;


} // namespace bluez
} // namespace ajn

#endif
