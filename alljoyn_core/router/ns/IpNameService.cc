/**
 * @file
 * The lightweight name service implementation
 */

/******************************************************************************
 * Copyright (c) 2010-2011, 2014, AllSeen Alliance. All rights reserved.
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

#include "IpNameService.h"
#include "IpNameServiceImpl.h"

#define QCC_MODULE "IPNS"

namespace ajn {

const uint16_t IpNameService::MULTICAST_MDNS_PORT = 5353;

static IpNameService* singletonIpNameService = NULL;
static int ipNsCounter = 0;
bool IpNameServiceInit::cleanedup = false;
typedef void (*RouterCleanupFunction)();

extern void RegisterRouterCleanup(RouterCleanupFunction r);

IpNameServiceInit::IpNameServiceInit()
{
    if (0 == ipNsCounter++) {
        singletonIpNameService = new IpNameService();
        RegisterRouterCleanup(&IpNameServiceInit::Cleanup);

    }
}

IpNameServiceInit::~IpNameServiceInit()
{
    if (0 == --ipNsCounter && !cleanedup) {
        delete singletonIpNameService;
        singletonIpNameService = NULL;
        cleanedup = true;
    }
}

void IpNameServiceInit::Cleanup()
{
    if (!cleanedup) {
        delete singletonIpNameService;
        singletonIpNameService = NULL;
        cleanedup = true;
    }
}

IpNameService& IpNameService::Instance()
{
    return *singletonIpNameService;
}

IpNameService::IpNameService()
    : m_constructed(false), m_destroyed(false), m_refCount(0), m_pimpl(NULL)
{
    //
    // AllJoyn is a multithreaded system.  Since the name service instance is
    // created on first use, the first use is in the Start() method of each
    // IP-related transport, and the starting of the transports happens on a
    // single thread, assume we are single-threaded here and don't do anything
    // fancy to prevent interleaving scenarios on the private implementation
    // constructor.
    //
    m_pimpl = new IpNameServiceImpl;
    m_constructed = true;
}

IpNameService::~IpNameService()
{
    //
    // We get here (on Linux) because when main() returns, the function
    // __run_exit_handlers() is called which, in turn, calls the destructors of
    // all of the static objects in whatever order the linker has decided will
    // be best.
    //
    // For us, the troublesome object is going to be the BundledRouter.  It is
    // torn down by the call to its destructor which may happen before or after
    // the call to our destructor.  If we are destroyed first, the bundled
    // daemon may happily continue to call into the name service singleton since
    // it may have no idea that it is about to go away.
    //
    // Eventually, we want to explicitly control the construction and
    // destruction order, but for now we have to live with the insanity (and
    // complexity) of dealing with out-of-order destruction.
    //
    // The name service singleton is a static, so its underlying memory won't go
    // away.  So by marking the singleton as destroyed we will have a lasting
    // indication that it has become unusable in case some bozo (the bundled
    // daemon) accesses us during destruction time after we have been destroyed.
    //
    // The exit handlers are going to be called by the main thread (that
    // originally called the main function), so the destructors will be called
    // sequentially.  The interesting problem, though, is that the BundledRouter
    // is going to have possibly more than one transport running, and typically
    // each of those transports has multiple threads that could conceivably be
    // making name service calls.  So, while our destructor is being called by
    // the main thread, it is possible that other transport threads will also
    // be calling.  Like in hunting rabbits, We've got to be very, very careful.
    //
    // First, make sure no callbacks leak out of the private implementation
    // during this critical time by turning off ALL callbacks to ALL transports.
    //
    if (m_pimpl) {
        m_pimpl->ClearCallbacks();
        m_pimpl->ClearNetworkEventCallbacks();
    }

    //
    // Now we slam shut an entry gate so that no new callers can get through and
    // try to do things while we are destroying the private implementation.
    //
    m_destroyed = true;

    //
    // No new callers will now be let in, but there may be existing callers
    // rummaging around in the object.  If the private implemetation is not
    // careful about multithreading, it can begin destroying itself with
    // existing calls in progress.  Thankfully, that's not our problem here.
    // We assume it will do the right thing.
    //
    if (m_pimpl) {

        //
        // Deleting the private implementation must accomplish an orderly shutdown
        // with an impled Stop() and Join().
        //
        delete m_pimpl;
        m_pimpl = NULL;
    }
}

#define ASSERT_STATE(function) \
    { \
        assert(m_constructed && "IpNameService::" # function "(): Singleton not constructed"); \
        assert(!m_destroyed && "IpNameService::" # function "(): Singleton destroyed"); \
        assert(m_pimpl && "IpNameService::" # function "(): Private impl is NULL"); \
    }

void IpNameService::Acquire(const qcc::String& guid, bool loopback)
{
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
    assert(m_constructed && "IpNameService::Acquire(): Singleton not constructed");

    ASSERT_STATE("Acquire");
    int refs = qcc::IncrementAndFetch(&m_refCount);
    if (refs == 1) {
        //
        // The first transport in gets to set the GUID and the loopback mode.
        // There should be only one GUID associated with a daemon process, so
        // this should never change; and loopback is a test mode set by a test
        // program pretending to be a single transport, so this is fine as well.
        //
        Init(guid, loopback);
        Start();
    }
}

void IpNameService::Release()
{
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
        //
        // The last transport to release its interest in the name service gets
        // pay the price for waiting for the service to exit.  Since we do a
        // Join(), this method is expected to be called out of a transport's
        // Join() so the price is expected.
        //
        Stop();
        Join();
    }
}

QStatus IpNameService::Start()
{
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

bool IpNameService::Started()
{
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

QStatus IpNameService::Stop()
{
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

QStatus IpNameService::Join()
{
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

QStatus IpNameService::Init(const qcc::String& guid, bool loopback)
{
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
    return m_pimpl->Init(guid, loopback);
}

void IpNameService::SetCallback(TransportMask transportMask,
                                Callback<void, const qcc::String&, const qcc::String&, std::vector<qcc::String>&, uint32_t>* cb)
{
    //
    // If the entry gate has been closed, we do not allow a SetCallback to actually
    // set anything.  The singleton is going away and so we assume we are
    // running __run_exit_handlers() so main() has returned.  We are definitely
    // shutting down, and the process is going to exit, so tricking callers who
    // may be temporarily running is okay.
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

void IpNameService::SetNetworkEventCallback(TransportMask transportMask,
                                            Callback<void, const std::map<qcc::String, qcc::IPAddress>&>* cb)
{
    if (m_destroyed) {
        return;
    }

    ASSERT_STATE("SetNetworkEventCallback");
    m_pimpl->SetNetworkEventCallback(transportMask, cb);
}

QStatus IpNameService::CreateVirtualInterface(const qcc::IfConfigEntry& entry)
{
    //
    // If the entry gate has been closed, we do not allow an OpenInterface to
    // actually open anything.  The singleton is going away and so we assume we
    // are running __run_exit_handlers() so main() has returned.  We are
    // definitely shutting down, and the process is going to exit, so tricking
    // callers who may be temporarily running is okay.
    //
    if (m_destroyed) {
        return ER_OK;
    }

    ASSERT_STATE("CreateVirtualInterface");
    return m_pimpl->CreateVirtualInterface(entry);
}

QStatus IpNameService::DeleteVirtualInterface(const qcc::String& ifceName)
{
    //
    // If the entry gate has been closed, we do not allow an OpenInterface to
    // actually open anything.  The singleton is going away and so we assume we
    // are running __run_exit_handlers() so main() has returned.  We are
    // definitely shutting down, and the process is going to exit, so tricking
    // callers who may be temporarily running is okay.
    //
    if (m_destroyed) {
        return ER_OK;
    }

    ASSERT_STATE("DeleteVirtualInterface");
    return m_pimpl->DeleteVirtualInterface(ifceName);
}

QStatus IpNameService::OpenInterface(TransportMask transportMask, const qcc::String& name)
{
    //
    // If the entry gate has been closed, we do not allow an OpenInterface to
    // actually open anything.  The singleton is going away and so we assume we
    // are running __run_exit_handlers() so main() has returned.  We are
    // definitely shutting down, and the process is going to exit, so tricking
    // callers who may be temporarily running is okay.
    //
    if (m_destroyed) {
        return ER_OK;
    }

    ASSERT_STATE("OpenInterface");
    return m_pimpl->OpenInterface(transportMask, name);
}

QStatus IpNameService::OpenInterface(TransportMask transportMask, const qcc::IPAddress& address)
{
    //
    // If the entry gate has been closed, we do not allow an OpenInterface to
    // actually open anything.  The singleton is going away and so we assume we
    // are running __run_exit_handlers() so main() has returned.  We are
    // definitely shutting down, and the process is going to exit, so tricking
    // callers who may be temporarily running is okay.
    //
    if (m_destroyed) {
        return ER_OK;
    }

    ASSERT_STATE("OpenInterface");
    return m_pimpl->OpenInterface(transportMask, address);
}

QStatus IpNameService::CloseInterface(TransportMask transportMask, const qcc::String& name)
{
    //
    // If the entry gate has been closed, we do not allow a CloseInterface to
    // actually close anything.  The singleton is going away and so we assume we
    // are running __run_exit_handlers() so main() has returned.  We are
    // definitely shutting down, and the process is going to exit, so tricking
    // callers who may be temporarily running is okay.
    //
    if (m_destroyed) {
        return ER_OK;
    }

    ASSERT_STATE("CloseInterface");
    return m_pimpl->CloseInterface(transportMask, name);
}

QStatus IpNameService::CloseInterface(TransportMask transportMask, const qcc::IPAddress& address)
{
    //
    // If the entry gate has been closed, we do not allow a CloseInterface to
    // actually close anything.  The singleton is going away and so we assume we
    // are running __run_exit_handlers() so main() has returned.  We are
    // definitely shutting down, and the process is going to exit, so tricking
    // callers who may be temporarily running is okay.
    //
    if (m_destroyed) {
        return ER_OK;
    }
    ASSERT_STATE("CloseInterface");
    return m_pimpl->CloseInterface(transportMask, address);
}

QStatus IpNameService::Enable(TransportMask transportMask,
                              const std::map<qcc::String, uint16_t>& reliableIPv4PortMap, uint16_t reliableIPv6Port,
                              const std::map<qcc::String, uint16_t>& unreliableIPv4PortMap, uint16_t unreliableIPv6Port,
                              bool enableReliableIPv4, bool enableReliableIPv6,
                              bool enableUnreliableIPv4, bool enableUnreliableIPv6)
{
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
    m_pimpl->Enable(transportMask, reliableIPv4PortMap, reliableIPv6Port, unreliableIPv4PortMap, unreliableIPv6Port,
                    enableReliableIPv4, enableReliableIPv6, enableUnreliableIPv4, enableUnreliableIPv6);
    return ER_OK;
}

QStatus IpNameService::Enabled(TransportMask transportMask,
                               std::map<qcc::String, uint16_t>& reliableIPv4PortMap, uint16_t& reliableIPv6Port,
                               std::map<qcc::String, uint16_t>& unreliableIPv4PortMap, uint16_t& unreliableIPv6Port)
{
    //
    // If the entry gate has been closed, we do not allow an Enabled to actually
    // do anything.  The singleton is going away and so we assume we are
    // running __run_exit_handlers() so main() has returned.  We are definitely
    // shutting down, and the process is going to exit, so tricking callers who
    // may be temporarily running is okay.
    //
    if (m_destroyed) {
        reliableIPv6Port = unreliableIPv6Port = 0;
        reliableIPv4PortMap.clear();
        unreliableIPv4PortMap.clear();
        return ER_OK;
    }

    ASSERT_STATE("Enabled");
    return m_pimpl->Enabled(transportMask, reliableIPv4PortMap, reliableIPv6Port, unreliableIPv4PortMap, unreliableIPv6Port);
}

QStatus IpNameService::FindAdvertisement(TransportMask transportMask, const qcc::String& matching, TransportMask completeTransportMask)
{
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

    ASSERT_STATE("FindAdvertisement");
    return m_pimpl->FindAdvertisement(transportMask, matching, IpNameServiceImpl::ALWAYS_RETRY, completeTransportMask);
}

QStatus IpNameService::CancelFindAdvertisement(TransportMask transportMask, const qcc::String& matching, TransportMask completeTransportMask)
{
    if (m_destroyed) {
        return ER_OK;
    }
    ASSERT_STATE("CancelFindAdvertisement");
    return m_pimpl->CancelFindAdvertisement(transportMask, matching, IpNameServiceImpl::ALWAYS_RETRY, completeTransportMask);
}

QStatus IpNameService::RefreshCache(TransportMask transportMask, const qcc::String& guid, const qcc::String& matching)
{
    if (m_destroyed) {
        return ER_OK;
    }
    ASSERT_STATE("RefreshCache");
    return m_pimpl->RefreshCache(transportMask, guid, matching);
}

QStatus IpNameService::AdvertiseName(TransportMask transportMask, const qcc::String& wkn, bool quietly, TransportMask completeTransportMask)
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
    return m_pimpl->AdvertiseName(transportMask, wkn, quietly, completeTransportMask);
}

QStatus IpNameService::CancelAdvertiseName(TransportMask transportMask, const qcc::String& wkn, TransportMask completeTransportMask)
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
    return m_pimpl->CancelAdvertiseName(transportMask, wkn, completeTransportMask);
}

QStatus IpNameService::OnProcSuspend()
{
    if (m_destroyed) {
        return ER_OK;
    }
    ASSERT_STATE("OnProcSuspend");
    return m_pimpl->OnProcSuspend();
}

QStatus IpNameService::OnProcResume()
{
    if (m_destroyed) {
        return ER_OK;
    }
    ASSERT_STATE("OnProcResume");
    return m_pimpl->OnProcResume();
}

void IpNameService::RegisterListener(IpNameServiceListener& listener)
{
    if (m_destroyed) {
        return;
    }
    ASSERT_STATE("RegisterListener");
    m_pimpl->RegisterListener(listener);
}

void IpNameService::UnregisterListener(IpNameServiceListener& listener)
{
    if (m_destroyed) {
        return;
    }
    ASSERT_STATE("UnregisterListener");
    m_pimpl->UnregisterListener(listener);
}

QStatus IpNameService::Ping(TransportMask transportMask, const qcc::String& guid, const qcc::String& name)
{
    if (m_destroyed) {
        return ER_OK;
    }
    ASSERT_STATE("Ping");
    return m_pimpl->Ping(transportMask, guid, name);
}

QStatus IpNameService::Query(TransportMask transportMask, MDNSPacket mdnsPacket)
{
    if (m_destroyed) {
        return ER_OK;
    }
    ASSERT_STATE("Query");
    return m_pimpl->Query(transportMask, mdnsPacket);
}

QStatus IpNameService::Response(TransportMask transportMask, uint32_t ttl, MDNSPacket mdnsPacket)
{
    if (m_destroyed) {
        return ER_OK;
    }
    ASSERT_STATE("Response");
    return m_pimpl->Response(transportMask, ttl, mdnsPacket);
}

bool IpNameService::RemoveFromPeerInfoMap(const qcc::String& guid)
{
    if (m_destroyed) {
        return false;
    }
    ASSERT_STATE("RemoveFromPeerInfoMap");
    return m_pimpl->RemoveFromPeerInfoMap(guid);
}

} // namespace ajn
