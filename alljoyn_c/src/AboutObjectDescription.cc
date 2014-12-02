/**
 * @file
 * This code is experimental, and as such has not been fully tested.
 * Please help make it more robust by contributing fixes if you find problems.
 */
/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include <alljoyn_c/AboutObjectDescription.h>
#include <alljoyn/AboutObjectDescription.h>
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

struct _alljoyn_aboutobjectdescription {
    /* Empty by design */
};

alljoyn_aboutobjectdescription AJ_CALL alljoyn_aboutobjectdescription_create()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_aboutobjectdescription) new ajn::AboutObjectDescription();
}

alljoyn_aboutobjectdescription AJ_CALL alljoyn_aboutobjectdescription_create_full(const alljoyn_msgarg arg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_aboutobjectdescription) new ajn::AboutObjectDescription(*(ajn::MsgArg*)arg);
}

void AJ_CALL alljoyn_aboutobjectdescription_destroy(alljoyn_aboutobjectdescription description)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    delete (ajn::AboutObjectDescription*)description;
}

QStatus AJ_CALL alljoyn_aboutobjectdescription_createfrommsgarg(alljoyn_aboutobjectdescription description,
                                                                const alljoyn_msgarg arg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutObjectDescription*)description)->CreateFromMsgArg(*(ajn::MsgArg*)arg);
}

size_t AJ_CALL alljoyn_aboutobjectdescription_getpaths(alljoyn_aboutobjectdescription description,
                                                       const char** paths,
                                                       size_t numPaths)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutObjectDescription*)description)->GetPaths(paths, numPaths);
}

size_t AJ_CALL alljoyn_aboutobjectdescription_getinterfaces(alljoyn_aboutobjectdescription description,
                                                            const char* path,
                                                            const char** interfaces,
                                                            size_t numInterfaces)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutObjectDescription*)description)->GetInterfaces(path, interfaces, numInterfaces);
}


size_t AJ_CALL alljoyn_aboutobjectdescription_getinterfacepaths(alljoyn_aboutobjectdescription description,
                                                                const char* interfaceName,
                                                                const char** paths,
                                                                size_t numPaths)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutObjectDescription*)description)->GetInterfacePaths(interfaceName, paths, numPaths);
}

void AJ_CALL alljoyn_aboutobjectdescription_clear(alljoyn_aboutobjectdescription description)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutObjectDescription*)description)->Clear();
}

bool AJ_CALL alljoyn_aboutobjectdescription_haspath(alljoyn_aboutobjectdescription description,
                                                    const char* path)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutObjectDescription*)description)->HasPath(path);
}

bool AJ_CALL alljoyn_aboutobjectdescription_hasinterface(alljoyn_aboutobjectdescription description,
                                                         const char* interfaceName)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutObjectDescription*)description)->HasInterface(interfaceName);
}

bool AJ_CALL alljoyn_aboutobjectdescription_hasinterfaceatpath(alljoyn_aboutobjectdescription description,
                                                               const char* path,
                                                               const char* interfaceName)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutObjectDescription*)description)->HasInterface(path, interfaceName);
}

QStatus AJ_CALL alljoyn_aboutobjectdescription_getmsgarg(alljoyn_aboutobjectdescription description,
                                                         alljoyn_msgarg msgArg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutObjectDescription*)description)->GetMsgArg((ajn::MsgArg*)msgArg);
}