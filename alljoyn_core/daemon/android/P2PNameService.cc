/**
 * @file
 * The Android Wi-Fi Direct (Wi-Fi P2P) name service implementation
 */

/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#include <qcc/Debug.h>

#include "P2PNameService.h"
#include "P2PNameServiceImpl.h"

#define QCC_MODULE "P2PNS"

namespace ajn {

P2PNameService::P2PNameService()
    : m_constructed(false), m_destroyed(false), m_refCount(0), m_pimpl(NULL)
{
    QCC_DbgPrintf(("P2PNameService::P2PNameService()"));
    m_constructed = true;
}

P2PNameService::~P2PNameService()
{
    QCC_DbgPrintf(("P2PNameService::~P2PNameService()"));

    //
    // Unfortunately, at global static object destruction time, it is too late
    // to be calling into the private implementation which is going to be
    // indirectly talking to another helper object which is talking to the
    // AllJoyn DBus interface.  We have to ensure that this object goes away
    // while there is enough infrastructure left in the AllJoyn bus to acquire
    // locks, etc., that it may need.
    //
    // So, by the time we get here, there had better not be a private
    // implementation object left around, since we will most likely crash if we
    // try to delete it.
    //
    assert(m_pimpl == NULL && "P2PConMan::P2PConMan(): private implementation not deleted");
}

#define ASSERT_STATE(function) \
    { \
        assert(m_constructed && "P2PNameService::" # function "(): Singleton not constructed"); \
        assert(!m_destroyed && "P2PNameService::" # function "(): Singleton destroyed"); \
        assert(m_pimpl && "P2PNameService::" # function "(): Private impl is NULL"); \
    }

void P2PNameService::Acquire(BusAttachment* bus, const qcc::String& guid)
{
    QCC_DbgPrintf(("P2PNameService::Acquire()"));

    //
    // If the entry gate has been closed, we do not allow an Acquire to actually
    // acquire a reference.  The singleton is going away and so we assume we are
    // running __run_exit_handlers() so main() has returned.  We are definitely
    // shutting down, and the process is going to exit, so tricking callers who
    // may be temporarily running is okay.
    //
    if (m_destroyed) {
        return;
    }

    //
    // The only way someone can get to us is if they call Instance() which will
    // cause the object to be constructed.
    //
    assert(m_constructed && "P2PNameService::Acquire(): Singleton not constructed");

    int refs = qcc::IncrementAndFetch(&m_refCount);
    if (refs == 1) {
        m_pimpl = new P2PNameServiceImpl;

        ASSERT_STATE("Acquire");

        //
        // The first transport in gets to set the GUID.  There should be only
        // one GUID associated with a daemon process, so this should never
        // change.
        //
        Init(bus, guid);
        Start();
    }
}

void P2PNameService::Release()
{
    QCC_DbgPrintf(("P2PNameService::Release()"));

    //
    // If the entry gate has been closed, we do not allow a Release to actually
    // release a reference.  The singleton is going away and so we assume we are
    // running __run_exit_handlers() so main() has returned.  We are definitely
    // shutting down, and the process is going to exit, so tricking callers who
    // may be temporarily running is okay.
    //
    if (m_destroyed) {
        return;
    }

    ASSERT_STATE("Release");
    int refs = qcc::DecrementAndFetch(&m_refCount);
    if (refs == 0) {
        QCC_DbgPrintf(("P2PNameService::Release(): refs == 0"));

        //
        // The last transport to release its interest in the name service gets
        // pay the price for waiting for the service to exit.  Since we do a
        // Join(), this method is expected to be called out of a transport's
        // Join() so the price is expected.
        //
        Stop();
        Join();

        //
        // Unfortunately, at global static object destruction time, it is too
        // late to be calling into the private implementation which is going to
        // be indirectly talking to another helper object which is talking to
        // the AllJoyn DBus interface.  We have to ensure that this object goes
        // away while there is enough infrastructure left in the AllJoyn bus to
        // acquire locks, etc., that it may need.  That is here and now.
        //
        delete m_pimpl;
        m_pimpl = NULL;
    }
}

QStatus P2PNameService::Start()
{
    QCC_DbgPrintf(("P2PNameService::Start()"));

    //
    // If the entry gate has been closed, we do not allow a Start to actually
    // start anything.  The singleton is going away and so we assume we are
    // running __run_exit_handlers() so main() has returned.  We are definitely
    // shutting down, and the process is going to exit, so tricking callers who
    // may be temporarily running is okay.
    //
    if (m_destroyed) {
        return ER_OK;
    }

    ASSERT_STATE("Start");
    return m_pimpl->Start();
}

bool P2PNameService::Started()
{
    QCC_DbgPrintf(("P2PNameService::Started()"));

    //
    // If the entry gate has been closed, we do not allow a Started to actually
    // test anything.  We just return false.  The singleton is going away and so
    // we assume we are running __run_exit_handlers() so main() has returned.
    // We are definitely shutting down, and the process is going to exit, so
    // tricking callers who may be temporarily running is okay.
    //
    if (m_destroyed) {
        return false;
    }

    ASSERT_STATE("Started");
    return m_pimpl->Started();
}

QStatus P2PNameService::Stop()
{
    QCC_DbgPrintf(("P2PNameService::Stop()"));

    //
    // If the entry gate has been closed, we do not allow a Stop to actually
    // stop anything.  The singleton is going away and so we assume we are
    // running __run_exit_handlers() so main() has returned.  We are definitely
    // shutting down, and the process is going to exit, so tricking callers who
    // may be temporarily running is okay.  The destructor is going to handle
    // the actual Stop() and Join().
    //
    if (m_destroyed) {
        return ER_OK;
    }

    ASSERT_STATE("Stop");
    return m_pimpl->Stop();
}

QStatus P2PNameService::Join()
{
    QCC_DbgPrintf(("P2PNameService::Join()"));

    //
    // If the entry gate has been closed, we do not allow a Join to actually
    // join anything.  The singleton is going away and so we assume we are
    // running __run_exit_handlers() so main() has returned.  We are definitely
    // shutting down, and the process is going to exit, so tricking callers who
    // may be temporarily running is okay.  The destructor is going to handle
    // the actual Stop() and Join().
    //
    if (m_destroyed) {
        return ER_OK;
    }

    ASSERT_STATE("Join");
    return m_pimpl->Join();
}

QStatus P2PNameService::Init(BusAttachment* bus, const qcc::String& guid)
{
    QCC_DbgPrintf(("P2PNameService::Init()"));

    //
    // If the entry gate has been closed, we do not allow an Init to actually
    // init anything.  The singleton is going away and so we assume we are
    // running __run_exit_handlers() so main() has returned.  We are definitely
    // shutting down, and the process is going to exit, so tricking callers who
    // may be temporarily running is okay.
    //
    if (m_destroyed) {
        return ER_OK;
    }

    ASSERT_STATE("Init");
    return m_pimpl->Init(bus, guid);
}

void P2PNameService::SetCallback(TransportMask transportMask,
                                 Callback<void, const qcc::String&, qcc::String&, uint8_t>* cb)
{
    QCC_DbgPrintf(("P2PNameService::SetCallback()"));

    //
    // If the entry gate has been closed, we do not allow a SetCallback to
    // actually set anything.  The singleton is going away and so we assume we
    // are running __run_exit_handlers() so main() has returned.  We are
    // definitely shutting down, and the process is going to exit, so tricking
    // callers who may be temporarily running is okay.
    //
    // The gotcha is that if there is a valid callback set, and the caller is
    // now setting the callback to NULL to prevent any new callbacks, the caller
    // will expect that no callbacks will follow this call.  This is taken care
    // of by calling SetCallback(NULL) on the private implemtation BEFORE
    // setting m_destroyed in our destructor.  In other words, the possible set
    // to NULL has already been done.
    //
    if (m_destroyed) {
        return;
    }

    ASSERT_STATE("SetCallback");
    m_pimpl->SetCallback(transportMask, cb);
}

QStatus P2PNameService::Enable(TransportMask transportMask)
{
    QCC_DbgPrintf(("P2PNameService::Enable()"));

    //
    // If the entry gate has been closed, we do not allow an Enable to actually
    // enable anything.  The singleton is going away and so we assume we are
    // running __run_exit_handlers() so main() has returned.  We are definitely
    // shutting down, and the process is going to exit, so tricking callers who
    // may be temporarily running is okay.
    //
    if (m_destroyed) {
        return ER_OK;
    }

    ASSERT_STATE("Enable");
    m_pimpl->Enable(transportMask);
    return ER_OK;
}

QStatus P2PNameService::Enabled(TransportMask transportMask, bool& enabled)
{
    QCC_DbgPrintf(("P2PNameService::Enabled()"));

    //
    // If the entry gate has been closed, we do not allow an Enabled to actually
    // do anything.  The singleton is going away and so we assume we are
    // running __run_exit_handlers() so main() has returned.  We are definitely
    // shutting down, and the process is going to exit, so tricking callers who
    // may be temporarily running is okay.
    //
    if (m_destroyed) {
        return ER_OK;
    }

    ASSERT_STATE("Enabled");
    return m_pimpl->Enabled(transportMask, enabled);
}

QStatus P2PNameService::Disable(TransportMask transportMask)
{
    QCC_DbgPrintf(("P2PNameService::Disable()"));

    //
    // If the entry gate has been closed, we do not allow an Enable to actually
    // enable anything.  The singleton is going away and so we assume we are
    // running __run_exit_handlers() so main() has returned.  We are definitely
    // shutting down, and the process is going to exit, so tricking callers who
    // may be temporarily running is okay.
    //
    if (m_destroyed) {
        return ER_OK;
    }

    ASSERT_STATE("Disable");
    m_pimpl->Disable(transportMask);
    return ER_OK;
}

QStatus P2PNameService::FindAdvertisedName(TransportMask transportMask, const qcc::String& prefix)
{
    QCC_DbgPrintf(("P2PNameService::FindAdvertisedName()"));

    //
    // If the entry gate has been closed, we do not allow a FindAdvertisedName
    // to actually find anything.  The singleton is going away and so we assume
    // we are running __run_exit_handlers() so main() has returned.  We are
    // definitely shutting down, and the process is going to exit, so tricking
    // callers who may be temporarily running is okay.
    //
    if (m_destroyed) {
        return ER_OK;
    }

    ASSERT_STATE("FindAdvertisedName");
    return m_pimpl->FindAdvertisedName(transportMask, prefix);
}

QStatus P2PNameService::CancelFindAdvertisedName(TransportMask transportMask, const qcc::String& prefix)
{
    QCC_DbgPrintf(("P2PNameService::CancelFindAdvertisedName()"));

    //
    // If the entry gate has been closed, we do not allow a FindAdvertisedName
    // to actually find anything.  The singleton is going away and so we assume
    // we are running __run_exit_handlers() so main() has returned.  We are
    // definitely shutting down, and the process is going to exit, so tricking
    // callers who may be temporarily running is okay.
    //
    if (m_destroyed) {
        return ER_OK;
    }

    ASSERT_STATE("CancelFindAdvertisedName");
    return m_pimpl->CancelFindAdvertisedName(transportMask, prefix);
}

QStatus P2PNameService::AdvertiseName(TransportMask transportMask, const qcc::String& wkn)
{
    //
    // If the entry gate has been closed, we do not allow an AdvertiseName to
    // actually advertise anything.  The singleton is going away and so we
    // assume we are running __run_exit_handlers() so main() has returned.  We
    // are definitely shutting down, and the process is going to exit, so
    // tricking callers who may be temporarily running is okay.
    //
    if (m_destroyed) {
        return ER_OK;
    }

    ASSERT_STATE("AdvertiseName");
    return m_pimpl->AdvertiseName(transportMask, wkn);
}

QStatus P2PNameService::CancelAdvertiseName(TransportMask transportMask, const qcc::String& wkn)
{
    //
    // If the entry gate has been closed, we do not allow a CancelAdvertiseName
    // to actually cancel anything.  The singleton is going away and so we
    // assume we are running __run_exit_handlers() so main() has returned.  We
    // are definitely shutting down, and the process is going to exit, so
    // tricking callers who may be temporarily running is okay.
    //
    if (m_destroyed) {
        return ER_OK;
    }

    ASSERT_STATE("CancelAdvertiseName");
    return m_pimpl->CancelAdvertiseName(transportMask, wkn);
}

QStatus P2PNameService::GetDeviceForGuid(const qcc::String& guid, qcc::String& device)
{
    //
    // If the entry gate has been closed, we do not allow a CancelAdvertiseName
    // to actually cancel anything.  The singleton is going away and so we
    // assume we are running __run_exit_handlers() so main() has returned.  We
    // are definitely shutting down, and the process is going to exit, so
    // tricking callers who may be temporarily running is okay.
    //
    if (m_destroyed) {
        return ER_OK;
    }

    ASSERT_STATE("GetDeviceForGuid");
    return m_pimpl->GetDeviceForGuid(guid, device);
}

} // namespace ajn
