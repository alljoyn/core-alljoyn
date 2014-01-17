/*
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
 */
#include "npn.h"

#include "NativeObject.h"
#include "PluginData.h"
#include <qcc/Debug.h>
#include <qcc/Log.h>
#include <qcc/SocketStream.h>
#include <errno.h>

#define QCC_MODULE "ALLJOYN_JS"

#define MAXREADY (64 * 1024)
#define DATA_DELIVERY_DELAY_MS 10 /* Android-only */

NPError NPP_New(NPMIMEType pluginType, NPP npp, uint16_t mode, int16_t argc, char* argn[], char* argv[], NPSavedData* saved)
{
    QCC_DbgTrace(("%s(pluginType=%s,npp=%p,mode=%u,argc=%d,argn=%p,argv=%p,saved=%p)", __FUNCTION__,
                  pluginType, npp, mode, argc, argn, argv, saved));
#if !defined(NDEBUG)
    for (int16_t i = 0; i < argc; ++i) {
        QCC_DbgTrace(("%s=%s", argn[i], argv[i]));
        if (!strcmp("debug", argn[i])) {
            QCC_SetLogLevels(argv[i]);
        }
    }
#endif
    if (!npp) {
        return NPERR_INVALID_INSTANCE_ERROR;
    }
    /*
     * Example UserAgents:
     *   UserAgent=Mozilla/5.0 (X11; U; Linux x86_64; en-US; rv:1.9.2.23) Gecko/20110921 Ubuntu/10.04 (lucid) Firefox/3.6.23
     *   Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/535.1 (KHTML, like Gecko) Chrome/14.0.835.202 Safari/535.1
     *   Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/535.1 (KHTML, like Gecko) Ubuntu/10.04 Chromium/14.0.835.202 Chrome/14.0.835.202 Safari/535.1
     */
    QCC_DbgTrace(("UserAgent=%s", NPN_UserAgent(npp)));

    Plugin plugin(npp);
    QStatus status = plugin->Initialize();
    if (ER_OK == status) {
        npp->pdata = new PluginData(plugin);
        return NPERR_NO_ERROR;
    } else {
        return NPERR_GENERIC_ERROR;
    }
}

NPError NPP_Destroy(NPP npp, NPSavedData** save)
{
    QCC_DbgTrace(("%s(npp=%p,save=%p)", __FUNCTION__, npp, save));
    if (!npp) {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    delete reinterpret_cast<PluginData*>(npp->pdata);
    npp->pdata = 0;
    QCC_DbgTrace(("-%s", __FUNCTION__));
    return NPERR_NO_ERROR;
}

NPError NPP_SetWindow(NPP npp, NPWindow* window)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!npp) {
        return NPERR_INVALID_INSTANCE_ERROR;
    }
    return NPERR_NO_ERROR;
}

NPError NPP_NewStream(NPP npp, NPMIMEType type, NPStream* stream, NPBool seekable, uint16_t* stype)
{
    QCC_DbgTrace(("%s(npp=%p,type=%s,stream={url=%s,end=%u,lastmodified=%u,notifyData=%p,headers=%s},seekable=%d,stype=%p)", __FUNCTION__, npp, type,
                  stream->url, stream->end, stream->lastmodified, stream->notifyData, stream->headers, seekable, stype));
    if (!npp) {
        return NPERR_INVALID_INSTANCE_ERROR;
    }
    //*stype = NP_ASFILEONLY; // TODO May be able to do this workaround for chrome local files, but it will copy the whole file
    *stype = NP_NORMAL;
    return NPERR_NO_ERROR;
}

NPError NPP_DestroyStream(NPP npp, NPStream* stream, NPReason reason)
{
    QCC_DbgTrace(("%s(npp=%p,stream={url=%s,notifyData=%p},reason=%d)", __FUNCTION__, npp, stream->url, stream->notifyData, reason));
    if (!npp) {
        return NPERR_INVALID_INSTANCE_ERROR;
    }
    qcc::SocketFd streamFd = reinterpret_cast<intptr_t>(stream->notifyData);
    qcc::Close(streamFd);
    return NPERR_NO_ERROR;
}

int32_t NPP_WriteReady(NPP npp, NPStream* stream)
{
    //QCC_DbgTrace(("%s(npp=%p,stream={url=%s,notifyData=%p})", __FUNCTION__, npp, stream->url, stream->notifyData));
    if (!npp) {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

#if defined(QCC_OS_GROUP_WINDOWS)
    return NP_MAXREADY;
#else
    /* TODO Chrome doesn't correctly support returning 0. */
    int32_t numReady;
    qcc::SocketFd streamFd = reinterpret_cast<intptr_t>(stream->notifyData);
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(streamFd, &writefds);
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    int ret = select(streamFd + 1, NULL, &writefds, NULL, &timeout);
    if (-1 != ret) {
        numReady = FD_ISSET(streamFd, &writefds) ? MAXREADY : 0;
    } else {
        QCC_LogError(ER_OS_ERROR, ("%d - %s", errno, strerror(errno)));
        numReady = 0;
    }
    //QCC_DbgTrace(("%s()=%d", __FUNCTION__, numReady));
    return numReady;
#endif
}

int32_t NPP_Write(NPP npp, NPStream* stream, int32_t offset, int32_t len, void* buffer)
{
    QCC_DbgTrace(("%s(npp=%p,stream={url=%s,notifyData=%p},offset=%d,len=%d,buffer=%p)", __FUNCTION__, npp, stream->url, stream->notifyData, offset, len, buffer));
    if (!npp) {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

#if defined(QCC_OS_GROUP_WINDOWS)
    return NP_MAXREADY;
#else
    /*
     * Under posix, qcc::Send() always blocks.  That is a no-no here, so use platform-specific code
     * to do the correct non-blocking write.
     */
    qcc::SocketFd streamFd = reinterpret_cast<intptr_t>(stream->notifyData);
    int32_t numSent;
    int ret = send(static_cast<int>(streamFd), buffer, len, MSG_NOSIGNAL);
    if (-1 != ret) {
        numSent = ret;
    } else if (EAGAIN == errno) {
        numSent = 0;
    } else {
        QCC_LogError(ER_OS_ERROR, ("%d - %s", errno, strerror(errno)));
        numSent = -1;
    }
    /* TODO Chrome doesn't correctly support returning 0. */
    QCC_DbgTrace(("%s()=%d", __FUNCTION__, numSent));
    return numSent;
#endif
}

void NPP_StreamAsFile(NPP npp, NPStream* stream, const char* fname)
{
    QCC_DbgTrace(("%s(npp=%p,stream=%p,fname=%s)", __FUNCTION__, npp, stream, fname));
    if (!npp) {
        return;
    }
}

void NPP_Print(NPP npp, NPPrint* platformPrint)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!npp) {
        return;
    }
}

int16_t NPP_HandleEvent(NPP npp, void* evt)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!npp) {
        return 0;
    }
    return 0;
}

void NPP_URLNotify(NPP npp, const char* url, NPReason reason, void* notifyData)
{
    QCC_DbgTrace(("%s(npp=%p,url=%s,reason=%d,notifyData=%p)", __FUNCTION__, npp, url, reason, notifyData));
    if (!npp) {
        return;
    }
}

NPError NPP_GetValue(NPP npp, NPPVariable var, void* value)
{
    int variable = var;
    switch (variable) {
#if !defined(NDEBUG)
    case NPPVpluginNameString:
        QCC_DbgTrace(("%s(variable=%s)", __FUNCTION__, "NPPVpluginNameString"));
        break;

    case NPPVpluginDescriptionString:
        QCC_DbgTrace(("%s(variable=%s)", __FUNCTION__, "NPPVpluginDescriptionString"));
        break;

    case NPPVpluginWindowBool:
        QCC_DbgTrace(("%s(variable=%s)", __FUNCTION__, "NPPVpluginWindowBool"));
        break;

    case NPPVpluginTransparentBool:
        QCC_DbgTrace(("%s(variable=%s)", __FUNCTION__, "NPPVpluginTransparentBool"));
        break;

    case NPPVjavaClass:
        QCC_DbgTrace(("%s(variable=%s)", __FUNCTION__, "NPPVjavaClass"));
        break;

    case NPPVpluginWindowSize:
        QCC_DbgTrace(("%s(variable=%s)", __FUNCTION__, "NPPVpluginWindowSize"));
        break;

    case NPPVpluginTimerInterval:
        QCC_DbgTrace(("%s(variable=%s)", __FUNCTION__, "NPPVpluginTimerInterval"));
        break;

    case NPPVpluginScriptableInstance:
        QCC_DbgTrace(("%s(variable=%s)", __FUNCTION__, "NPPVpluginScriptableInstance"));
        break;

    case NPPVpluginScriptableIID:
        QCC_DbgTrace(("%s(variable=%s)", __FUNCTION__, "NPPVpluginScriptableIID"));
        break;

    case NPPVjavascriptPushCallerBool:
        QCC_DbgTrace(("%s(variable=%s)", __FUNCTION__, "NPPVjavascriptPushCallerBool"));
        break;

    case NPPVpluginKeepLibraryInMemory:
        QCC_DbgTrace(("%s(variable=%s)", __FUNCTION__, "NPPVpluginKeepLibraryInMemory"));
        break;

    case NPPVpluginNeedsXEmbed:
        QCC_DbgTrace(("%s(variable=%s)", __FUNCTION__, "NPPVpluginNeedsXEmbed"));
        break;

    case NPPVpluginScriptableNPObject:
        QCC_DbgTrace(("%s(variable=%s)", __FUNCTION__, "NPPVpluginScriptableNPObject"));
        break;

    case NPPVformValue:
        QCC_DbgTrace(("%s(variable=%s)", __FUNCTION__, "NPPVformValue"));
        break;

    case NPPVpluginUrlRequestsDisplayedBool:
        QCC_DbgTrace(("%s(variable=%s)", __FUNCTION__, "NPPVpluginUrlRequestsDisplayedBool"));
        break;

    case NPPVpluginWantsAllNetworkStreams:
        QCC_DbgTrace(("%s(variable=%s)", __FUNCTION__, "NPPVpluginWantsAllNetworkStreams"));
        break;

#ifdef XP_MACOSX
    case NPPVpluginDrawingModel:
        QCC_DbgTrace(("%s(variable=%s)", __FUNCTION__, "NPPVpluginDrawingModel"));
        break;
#endif

#if (MOZ_PLATFORM_MAEMO == 5)
    case NPPVpluginWindowlessLocalBool:
        QCC_DbgTrace(("%s(variable=%s)", __FUNCTION__, "NPPVpluginWindowlessLocalBool"));
        break;
#endif

#endif // !NDEBUG

    default:
        QCC_DbgTrace(("%s(variable=%d)", __FUNCTION__, variable));
        break;
    }

    NPError ret = NPERR_NO_ERROR;
    switch (variable) {
#if defined(XP_UNIX)
    case NPPVpluginNeedsXEmbed:
        *((PRBool*)value) = PR_TRUE;
        break;
#endif

    case NPPVpluginScriptableNPObject: {
            if (!npp) {
                ret = NPERR_INVALID_INSTANCE_ERROR;
                break;
            }
            PluginData* pluginData = reinterpret_cast<PluginData*>(npp->pdata);
            if (!pluginData) {
                ret = NPERR_GENERIC_ERROR;
                break;
            }
            *(NPObject** )value = pluginData->GetScriptableObject();
            break;
        }

    default:
        ret = NPERR_GENERIC_ERROR;
        break;
    }
    return ret;
}

NPError NPP_SetValue(NPP npp, NPNVariable variable, void* value)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!npp) {
        return NPERR_INVALID_INSTANCE_ERROR;
    }
    return NPERR_NO_ERROR;
}
