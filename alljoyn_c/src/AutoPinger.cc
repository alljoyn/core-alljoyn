/**
 * @file
 *
 * AutoPinger
 */
/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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

#include <alljoyn_c/AutoPinger.h>
#include <alljoyn/AutoPinger.h>
#include <alljoyn/PingListener.h>
#include <alljoyn/BusAttachment.h>

#include <qcc/Debug.h>
#define QCC_MODULE "ALLJOYN_C"

namespace ajn {

class PingListenerC : public PingListener {
  public:

    PingListenerC(const alljoyn_pinglistener_callback* _callback, const void* context_in) :
        context(context_in)
    {
        memcpy(&callback, _callback, sizeof(alljoyn_pinglistener_callback));
    }

    virtual void DestinationLost(const qcc::String& group, const qcc::String& destination)
    {
        if (NULL != callback.destination_lost) {
            callback.destination_lost(context, group.c_str(), destination.c_str());
        }
    }

    virtual void DestinationFound(const qcc::String& group, const qcc::String& destination)
    {
        if (NULL != callback.destination_found) {
            callback.destination_found(context, group.c_str(), destination.c_str());
        }
    }
  private:
    const void* context;
    alljoyn_pinglistener_callback callback;
};

class AutoPingerC : public AutoPinger {
  private:
    /* Private assigment operator - does nothing */
    AutoPingerC operator=(const AutoPingerC&);
  public:
    AutoPingerC(alljoyn_busattachment cbus);
    ~AutoPingerC();
};

AutoPingerC::AutoPingerC(alljoyn_busattachment cbus) :
    AutoPinger(*(BusAttachment*) cbus)
{
}

AutoPingerC::~AutoPingerC()
{
}
}

/* {{{ alljoyn_pinglistener */

struct _alljoyn_pinglistener_handle {
};

alljoyn_pinglistener AJ_CALL alljoyn_pinglistener_create(const alljoyn_pinglistener_callback* callback,
                                                         const void* context)
{
    if (NULL == callback || NULL == context) {
        return NULL;
    }
    return (alljoyn_pinglistener) new ajn::PingListenerC(callback, context);
}

void AJ_CALL alljoyn_pinglistener_destroy(alljoyn_pinglistener listener)
{
    delete (ajn::PingListenerC*) listener;
}
/* }}} */

/* {{{ alljoyn_autopinger */

struct _alljoyn_autopinger_handle {
};

alljoyn_autopinger AJ_CALL alljoyn_autopinger_create(alljoyn_busattachment cbus)
{
    if (NULL != cbus) {
        return (alljoyn_autopinger) new ajn::AutoPingerC(cbus);
    }
    return NULL;
}

void AJ_CALL alljoyn_autopinger_destroy(alljoyn_autopinger autopinger)
{
    delete (ajn::AutoPingerC*) autopinger;
    autopinger = NULL;
}

void AJ_CALL alljoyn_autopinger_pause(alljoyn_autopinger autopinger)
{

    if (NULL != autopinger) {
        ((ajn::AutoPingerC*) autopinger)->Pause();
    }
}

void AJ_CALL alljoyn_autopinger_resume(alljoyn_autopinger autopinger)
{
    if (NULL != autopinger) {
        ((ajn::AutoPingerC*) autopinger)->Resume();
    }
}

void AJ_CALL alljoyn_autopinger_addpinggroup(alljoyn_autopinger autopinger,
                                             const char* group,
                                             alljoyn_pinglistener listener,
                                             uint32_t pinginterval)
{
    if (NULL != autopinger && listener != NULL) {
        ((ajn::AutoPingerC*) autopinger)->AddPingGroup(group, *((ajn::PingListenerC*) listener),
                                                       pinginterval);
    }
}

void AJ_CALL alljoyn_autopinger_removepinggroup(alljoyn_autopinger autopinger, const char* group)
{
    if (NULL != autopinger) {
        ((ajn::AutoPingerC*) autopinger)->RemovePingGroup(group);
    }
}

QStatus AJ_CALL alljoyn_autopinger_setpinginterval(alljoyn_autopinger autopinger,
                                                   const char* group,
                                                   uint32_t pinginterval)
{
    if (NULL != autopinger) {
        return ((ajn::AutoPingerC*) autopinger)->SetPingInterval(group, pinginterval);
    }
    return ER_FAIL;
}

QStatus AJ_CALL alljoyn_autopinger_adddestination(alljoyn_autopinger autopinger,
                                                  const char* group,
                                                  const char* destination)
{
    if (NULL != autopinger) {
        return ((ajn::AutoPingerC*) autopinger)->AddDestination(group, destination);
    }
    return ER_FAIL;
}

QStatus AJ_CALL alljoyn_autopinger_removedestination(alljoyn_autopinger autopinger,
                                                     const char* group,
                                                     const char* destination,
                                                     QCC_BOOL removeall)
{
    if (NULL != autopinger) {
        return ((ajn::AutoPingerC*) autopinger)->RemoveDestination(group, destination,
                                                                   (bool) removeall);
    }
    return ER_FAIL;
}
/* }}} */
