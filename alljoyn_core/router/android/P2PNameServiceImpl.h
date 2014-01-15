/**
 * @file
 * Data structures used for the AllJoyn P2P Name Service Implementation
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

#ifndef _P2P_NAME_SERVICE_IMPL_H
#define _P2P_NAME_SERVICE_IMPL_H

#ifndef __cplusplus
#error Only include P2PNameServiceImpl.h in C++ code.
#endif

#include <map>

#include <qcc/String.h>
#include <qcc/StringMapKey.h>

#include <alljoyn/TransportMask.h>

#include <alljoyn/Status.h>
#include <Callback.h>

#include "P2PHelperInterface.h"

namespace ajn {

/**
 * @brief API to provide an implementation dependent Wi-Fi P2P (Layer 2) Name
 * Service for AllJoyn.
 *
 * The basic goal of this class is to provide a way for AllJoyn clients to
 * determine that services exist using Wi-Fi P2P (Direct) pre-association service
 * discovery.  In the Android world, the P2P framework is part of the Android
 * Application Framework which is written in Java.  Because we are running in a
 * (daemon) process where Java may be completely unavailable, we must communicate
 * with a process that does have Java.  AllJoyn is made for communication with
 * possibly remote processes, so we rely on AllJoyn method calls and signals to
 * talk to a service which is guraranteed to be running in a process that has
 * Java and the Android framework available.
 *
 * This class only solves the pre-association service discovery part of the
 * puzzle.  Upper level code relies on us to discover services, a layer two
 * connection manager to create temporary (Wi-Fi P2P) networks, and the
 * IpNameService to discover IP addressing information on those networks.  The
 * Wi-Fi Direct transport (in the AllJoyn sense of the word transport) ties all
 * of these pieces together.
 *
 * We rely on a class that is implements the proxy bus object required to talk
 * to the remote bus object which will do the calls into the Android Application
 * Framework which will, in turn, do the actual advertisement and discovery
 * operations.
 */
class P2PNameServiceImpl {
  public:

    /**
     * @brief The maximum size of a well-known name, in general.
     */
    static const uint32_t MAX_NAME_SIZE = 255;

    /**
     * @brief The default time for which an advertisement is valid, in seconds.
     */
    static const uint32_t DURATION_DEFAULT = (120);

    /**
     * @brief The time value indicating an advertisement is valid forever.
     */
    static const uint32_t DURATION_INFINITE = 255;

    /**
     * @brief Construct a P2P name service implementation object.
     */
    P2PNameServiceImpl();

    /**
     * @brief Destroy a P2P name service implementation object.
     */
    virtual ~P2PNameServiceImpl();

    /**
     * @brief Initialize the P2PNameServiceImpl.
     *
     * @param guid A string containing the GUID assigned to the daemon which is
     *     hosting the name service.
     */
    QStatus Init(BusAttachment* bus, const qcc::String& guid);

    /**
     * @brief Stop any name service threads.
     *
     * We don't have any threads here, but having Start(), Stop() and Join() is
     * useful for providing opportunities to create or destroy objects that may
     * cause problems if we wait until static destructor time to get rid of
     * them.
     */
    QStatus Start();

    /**
     * @brief Determine if the P2PNameServiceImpl has been started.
     *
     * @return True if the service has been started, false otherwise.
     */
    bool Started() { return m_state == IMPL_RUNNING; }

    /**
     * @brief Stop any name service threads.
     *
     * We don't have any threads here, but having Start(), Stop() and Join() is
     * useful for providing opportunities to create or destroy objects that may
     * cause problems if we wait until static destructor time to get rid of
     * them.
     */
    QStatus Stop();

    /**
     * @brief Join any name service threads.
     *
     * We don't have any threads here, but having Start(), Stop() and Join() is
     * useful for providing opportunities to create or destroy objects that may
     * cause problems if we wait until static destructor time to get rid of
     * them.
     */
    QStatus Join();

    /**
     * @brief Set the callback function that is called to notify a transport about
     *     found and lost well-known names.
     *
     * @param transportMask A bitmask containing the transport handling the specified
     *     endpoints.  This is provided for API consistency and to check assumptions.
     * @param cb The callback method on the transport that will be called to notify
     *     a transport about found and lost well-known names.
     */
    void SetCallback(TransportMask transportMask,
                     Callback<void, const qcc::String&, qcc::String&, uint8_t>* cb);

    /**
     * @brief Notify the name service that that it should start advertising over
     *     Wi-Fi Direct pre-association service discovery.
     *
     * Enable() communicates the fact that there is some transport that will be able
     * to receive and deal with connection attempts as a result of the advertisements
     * which may be generated as a result of the call.
     *
     * @param transportMask A bitmask containing the transport handling the specified
     *     endpoints.  Primarily for expectation checking.
     */
    QStatus Enable(TransportMask transportMask);

    /**
     * @brief Notify the name service that that it should stop advertising over
     *     pre-association service discovery.
     *
     * @param transportMask A bitmask containing the transport handling the specified
     *     endpoints.  Primarily for expectation checking.
     *
     * @see Enable()
     */
    QStatus Disable(TransportMask transportMask);

    /**
     * @brief Ask the name service whether or not it is enabled
     *
     * @param transportMask A bitmask containing the transport handling the specified
     *     endpoints.  Primarily for expectation checking.
     * @param enabled If the returned value is false, the name service is not enabled;
     *     if true it is
     */
    QStatus Enabled(TransportMask transportMask, bool& enabled);

    /**
     * @brief Discover well-known names starting with the specified prefix using
     * Wi-Fi Direct pre-association service discovery.
     *
     * @param transportMask A bitmask containing the transport requesting the
     *     discovery operation.  Primarily for expectation checking.
     * @param prefix The well-known name prefix to find.
     */
    QStatus FindAdvertisedName(TransportMask transportMask, const qcc::String& prefix);

    /**
     * @brief Stop discovering well-known names starting with the specified
     * prefix over Wi-Fi Direct pre-association service discovery.
     *
     * @param transportMask A bitmask containing the transport requesting the
     *     discovery operation.  Primarily for expectation checking.
     * @param prefix The well-known name prefix to stop finding.
     */
    QStatus CancelFindAdvertisedName(TransportMask transportMask, const qcc::String& prefix);

    /**
     * @brief Advertise a well-known name using Wi-Fi Direct pre-association
     * service discovery.
     *
     * @param transportMask A bitmask containing the transport requesting the
     *     advertisement.  Primarily for expectation checking.
     * @param wkn The well-known name to advertise.
     */
    QStatus AdvertiseName(TransportMask transportMask, const qcc::String& wkn);

    /**
     * @brief Stop advertising a well-known name over Wi-Fi Direct
     * pre-association service discovery.
     *
     * @param transportMask A bitmask containing the transport requesting the
     *     advertisement be canceled.  Primarily for expectation checking.
     * @param wkn The well-known name to stop advertising.
     */
    QStatus CancelAdvertiseName(TransportMask transportMask, const qcc::String& wkn);

    /**
     * @brief Advertise a well-known name using Wi-Fi Direct pre-association
     * service discovery.
     *
     * This method allows the caller to specify multiple well-known names in a
     * single call.
     *
     * @param[in] transportMask A bitmask containing the transport requesting the
     *     advertisement.
     * @param[in] wkn A vector of AllJoyn well-known names (e.g., "org.freedesktop.Sensor").
     *
     * @return Status of the operation.  Returns ER_OK on success.
     */
    QStatus AdvertiseName(TransportMask transportMask, std::vector<qcc::String>& wkn);

    /**
     * @brief Cancel an AllJoyn daemon service advertisement.
     *
     * If an AllJoyn daemon wants to cancel an advertisement of a well-known name
     * over pre-association service discovery it calls this function.
     *
     * @param[in] transportMask A bitmask containing the transport requesting the
     *     advertisement be canceled.
     * @param[in] wkn A vector of AllJoyn well-known names (e.g., "org.freedesktop.Sensor").
     *
     * @return Status of the operation.  Returns ER_OK on success.
     */
    QStatus CancelAdvertiseName(TransportMask transportMask, std::vector<qcc::String>& wkn);

    /**
     * @brief Given a GUID that we have discovered and reported back, return
     * the device that was associated with that GUID when we received the
     * advdrtisement.
     *
     * @param[in] guid The GUID reported back to the caller when we got the
     *     advertisement.
     * @param[out] device The device we heard the advertisement from.
     */
    QStatus GetDeviceForGuid(const qcc::String& guid, qcc::String& device);

  private:
    /**
     * @brief Copying an IpNameServiceImpl object is forbidden.
     */
    P2PNameServiceImpl(const P2PNameServiceImpl& other);

    /**
     * @brief Assigning a P2PNameServiceImpl object is forbidden.
     */
    P2PNameServiceImpl& operator =(const P2PNameServiceImpl& other);

    /**
     * @brief
     * Private notion of what state the implementation object is in.
     */
    enum State {
        IMPL_INVALID,           /**< Should never be seen on a constructed object */
        IMPL_SHUTDOWN,          /**< Nothing is running and object may be destroyed */
        IMPL_INITIALIZING,      /**< Object is in the process of coming up and may be inconsistent */
        IMPL_RUNNING,           /**< Object is running and ready to go */
        IMPL_STOPPING,          /**< Object is stopping */
    };

    /**
     * @brief State variable to indicate what the implementation is doing or is
     * capable of doing.
     */
    State m_state;

    /**
     * @brief The daemon GUID string of the daemon associated with this instance
     * of the name service.
     */
    qcc::String m_guid;

    /**
     * @brief If true, allow the name service to communicate with the outside
     * world.  If false, ensure that no packets are sent and no sockets are
     * listening for connections.  For Android Compatibility Test Suite (CTS)
     * conformance.
     */
    bool m_enabled;

    /**
     * The callback used to indicate FoundAdvertisedName to the client.  Should
     * really be a vector of callback in case more than one transport hooks us.
     */
    Callback<void, const qcc::String&, qcc::String&, uint8_t>* m_callback;

    void OnFoundAdvertisedName(qcc::String& name, qcc::String& namePrefix, qcc::String& guid, qcc::String& device);
    void OnLostAdvertisedName(qcc::String& name, qcc::String& namePrefix, qcc::String& guid, qcc::String& device);
    void OnLinkEstablished(int32_t handle, qcc::String& interface) { }
    void OnLinkError(int32_t handle, int32_t error) { }
    void OnLinkLost(int32_t handle) { }
    void HandleFindAdvertisedNameReply(int32_t result);
    void HandleCancelFindAdvertisedNameReply(int32_t result);
    void HandleAdvertiseNameReply(int32_t result);
    void HandleCancelAdvertiseNameReply(int32_t result);
    void HandleEstablishLinkReply(int32_t handle) { }
    void HandleReleaseLinkReply(int32_t result) { }
    void HandleGetInterfaceNameFromHandleReply(qcc::String& interface) { }

    /**
     * A listener class to receive events from an underlying Wi-Fi Direct
     * helper service.  The helper actually talks to an AllJoyn service
     * which, in turn, talks to the Android Application Framework.  Events
     * from the framework are sent back to the helper as AllJoyn signals
     * which then find their way to this listener class.  We just forward
     * them on back to the P2PNameService which digests them and possibly
     * forwards them again up to a transport.
     */
    class MyP2PHelperListener : public P2PHelperListener {
      public:
        MyP2PHelperListener(P2PNameServiceImpl* nsi) : m_nsi(nsi) { }
        ~MyP2PHelperListener() { }

        virtual void OnFoundAdvertisedName(qcc::String& name, qcc::String& namePrefix, qcc::String& guid, qcc::String& device)
        {
            assert(m_nsi);
            m_nsi->OnFoundAdvertisedName(name, namePrefix, guid, device);
        }

        virtual void OnLostAdvertisedName(qcc::String& name, qcc::String& namePrefix, qcc::String& guid, qcc::String& device)
        {
            assert(m_nsi);
            m_nsi->OnLostAdvertisedName(name, namePrefix, guid, device);
        }

        virtual void OnLinkEstablished(int32_t handle, qcc::String& interface)
        {
            assert(m_nsi);
            m_nsi->OnLinkEstablished(handle, interface);
        }

        virtual void OnLinkError(int32_t handle, int32_t error)
        {
            assert(m_nsi);
            m_nsi->OnLinkError(handle, error);
        }

        virtual void OnLinkLost(int32_t handle)
        {
            assert(m_nsi);
            m_nsi->OnLinkLost(handle);
        }

        virtual void HandleFindAdvertisedNameReply(int32_t result)
        {
            assert(m_nsi);
            m_nsi->HandleFindAdvertisedNameReply(result);
        }

        virtual void HandleCancelFindAdvertisedNameReply(int32_t result)
        {
            assert(m_nsi);
            m_nsi->HandleCancelFindAdvertisedNameReply(result);
        }

        virtual void HandleAdvertiseNameReply(int32_t result)
        {
            assert(m_nsi);
            m_nsi->HandleAdvertiseNameReply(result);
        }

        virtual void HandleCancelAdvertiseNameReply(int32_t result)
        {
            assert(m_nsi);
            m_nsi->HandleCancelAdvertiseNameReply(result);
        }

        virtual void HandleEstablishLinkReply(int32_t handle)
        {
            assert(m_nsi);
            m_nsi->HandleEstablishLinkReply(handle);
        }

        virtual void HandleReleaseLinkReply(int32_t result)
        {
            assert(m_nsi);
            m_nsi->HandleReleaseLinkReply(result);
        }

        virtual void HandleGetInterfaceNameFromHandleReply(qcc::String& interface)
        {
            assert(m_nsi);
            m_nsi->HandleGetInterfaceNameFromHandleReply(interface);
        }

      private:
        P2PNameServiceImpl* m_nsi;
    };

    MyP2PHelperListener* m_myP2pHelperListener; /**< The listener that receives events from the P2P Helper Service */
    P2PHelperInterface* m_p2pHelperInterface;   /**< The AllJoyn interface used to talk to the P2P Helper Service */

    BusAttachment* m_bus;                       /**< The AllJoyn bus attachment that we use to talk to the P2P Helper Service */

    std::map<qcc::StringMapKey, qcc::String> m_devices;  /**< map of guids to advertising devices */
};

} // namespace ajn

#endif // _P2P_NAME_SERVICE_IMPL_H
