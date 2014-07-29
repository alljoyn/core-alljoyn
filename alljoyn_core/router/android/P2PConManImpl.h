/**
 * @file
 * Implementation of the AllJoyn Android Wi-Fi Direct (Wi-Fi P2P) connection manager
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

#ifndef _P2P_CON_MAN_IMPLH
#define _P2P_CON_MAN_IMPLH

#ifndef __cplusplus
#error Only include P2PConManImpl.h in C++ code.
#endif

#include <vector>

#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <qcc/Thread.h>
#include <qcc/time.h>

#include <alljoyn/Status.h>

#include "P2PHelperInterface.h"

namespace ajn {

/**
 * @brief API to provide an implementation dependent P2P (Layer 2) Connection
 * Manager for AllJoyn.
 */
class P2PConManImpl {
  public:
    /**
     * @brief Construct a P2P connection manager implementation object.
     */
    P2PConManImpl();

    /**
     * @brief Destroy a P2P connection manager implementation object.
     */
    virtual ~P2PConManImpl();

    /**
     * @brief Initialize the P2PConManImpl.
     *
     * @param guid A string containing the GUID assigned to the daemon which is
     *     hosting the connection manager.
     */
    QStatus Init(BusAttachment* bus, const qcc::String& guid);

    /**
     * @brief Stop any name service threads.
     *
     * We don't have any threads here, but it may be the case that one of the objects we use
     * does.  Currently does nothing.
     */
    QStatus Start() { m_state = IMPL_RUNNING; return ER_OK; }

    /**
     * @brief Determine if the connection manager has been started.
     *
     * @return True if the connection manager has been started, false otherwise.
     */
    bool Started() { return m_state == IMPL_RUNNING; }

    /**
     * @brief Stop any name service threads.
     *
     * We don't have any threads here, but it may be the case that we have a
     * thread wandering around in one of our methods, so we need to make sure
     * that it is told to go away.
     */
    QStatus Stop();

    /**
     * @brief Join any name service threads.
     *
     * We don't have any threads here, but it may be the case that we have a
     * thread wandering around in one of our methods, so we need to make sure
     * that it is gone.  In particular, we need to ensure that no thread holds
     * our locks since that will cause an assert in our destructor.
     */
    QStatus Join();

    /**
     * @brief Set the callback function that is called to notify a transport about
     *     the coming and going of a Wi-Fi Direct link.
     *
     * @param cb The callback method on the transport that will be called to notify
     *     a transport about link state changes.
     */
    QStatus SetStateCallback(Callback<void, P2PConMan::LinkState, const qcc::String&>* cb);

    /**
     * @brief Set the callback function that is called to notify a transport about
     *     the coming and going of well-known names found using the IP name service
     *     (which is accessible from here).
     *
     * @param cb The callback method on the transport that will be called to notify
     *     a transport about well-known names discovered over IP.
     */
    QStatus SetNameCallback(Callback<void, const qcc::String&, const qcc::String&, std::vector<qcc::String>&, uint8_t>* cb);

    /**
     * @brief Create a temporary physical network connection to the provided
     *     device MAC address using Wi-Fi Direct.
     *
     * @param[in] device The MAC address of the remote device presented as a string.
     * @param[in] intent The Wi-Fi Direcct group owner intent value.
     *
     * @return ER_OK if the network is successfully created, otherwise (hopefully)
     *     appropriate error code reflecting outcome.
     */
    QStatus CreateTemporaryNetwork(const qcc::String& device, int32_t intent);

    /**
     * @brief Destroy the current temporary physical network connection.
     *
     * Assumes that it is only possible to have one Wi-Fi Direct network running
     * at a time.
     *
     * @return ER_OK if the network is successfully created, otherwise (hopefully)
     *     appropriate error code reflecting outcome.
     */
    QStatus DestroyTemporaryNetwork(void);

    /**
     * @brief Determine if the P2PConman is connected to a group led by the device
     *     with the provided MAC address
     *
     * @param[in] device The MAC address of the remote device presented as a string.
     *
     * @return True if a physical network has been created that allows us to
     *     access <device>.
     */
    bool IsConnected(const qcc::String& device);

    /**
     * @brief Determine if the P2PConman is in the connected state to any device.
     *
     * @return True if a physical network is created.
     */
    bool IsConnected(void);

    /**
     * @brief Determine if the P2PConman is in the connected state to any device
     *     and we think it is acting as a Station (STA) node in the group.
     *
     * @return True if a physical network is created and we are a STA.
     */
    bool IsConnectedSTA(void);

    /**
     * @brief Determine if the P2PConman is in the connected state and we think
     *     it is acting as the Group Owner (GO) of the group.
     *
     * @return True if a physical network is created and we are the GO.
     */
    bool IsConnectedGO(void);

    /**
     * @brief Return an appropriate connect spec <spec> for use in making a TCP
     *     to a daemon specified by <guid> that is running on the device with
     *     MAC address specified by <device>.
     *
     * @param[in]  device The MAC address of the remote device presented as a string.
     * @param[in]  guid The GUID of the remote daemon presented as a string.
     * @param[out] spec A connect spec that can be used to connect to the remote
     *     daemon.
     *
     * @return ER_OK if the connect spec can be determined.
     */
    QStatus CreateConnectSpec(const qcc::String& device, const qcc::String& guid, qcc::String& spec);

  private:
    /**
     * @brief A private "alert code" to distinguish the connection manager as
     * the source of the event when waking threads.
     */
    static const uint32_t PRIVATE_ALERT_CODE = 0xfeedbeef;

    static const uint32_t TEMPORARY_NETWORK_ESTABLISH_TIMEOUT = P2PConMan::TEMPORARY_NETWORK_ESTABLISH_TIMEOUT;
    static const uint32_t CREATE_CONNECT_SPEC_TIMEOUT = P2PConMan::CREATE_CONNECT_SPEC_TIMEOUT;

    /**
     * @brief Copying an IpConManImpl object is forbidden.
     */
    P2PConManImpl(const P2PConManImpl& other);

    /**
     * @brief Assigning a P2PConManImpl object is forbidden.
     */
    P2PConManImpl& operator =(const P2PConManImpl& other);

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
     * @brief
     * Private notion of what state an underlying Wi-Fi Direct connection is in.
     */
    enum ConnState {
        CONN_INVALID,           /**< Should never be seen on a constructed object */
        CONN_IDLE,              /**< No connection and no connection in progress */
        CONN_READY,             /**< Ready to accept new connections (applies to GO side) */
        CONN_CONNECTING,        /**< A connection attempt is in progress (applies to STA side) */
        CONN_CONNECTED,         /**< We think we have a temporary network up and running */
    };

    /**
     * @brief
     * Private notion of what kind of underlying Wi-Fi Direct connection we are using.
     */
    enum ConnType {
        CONN_NEITHER,  /**< Link not established, so neither GO or STA */
        CONN_GO,       /**< Link established and we are the GO */
        CONN_STA,      /**< Link established and we are the STA */
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

    void OnFoundAdvertisedName(qcc::String& name, qcc::String& namePrefix, qcc::String& guid, qcc::String& device) { }
    void OnLostAdvertisedName(qcc::String& name, qcc::String& namePrefix, qcc::String& guid, qcc::String& device) { }
    void OnLinkEstablished(int32_t handle, qcc::String& interface);
    void OnLinkError(int32_t handle, int32_t error);
    void OnLinkLost(int32_t handle);
    void HandleFindAdvertisedNameReply(int32_t result) { }
    void HandleCancelFindAdvertisedNameReply(int32_t result) { }
    void HandleAdvertiseNameReply(int32_t result) { }
    void HandleCancelAdvertiseNameReply(int32_t result) { }
    void HandleEstablishLinkReply(int32_t handle);
    void HandleReleaseLinkReply(int32_t result);
    void HandleGetInterfaceNameFromHandleReply(qcc::String& interface);

    /**
     * A listener class to receive events from an underlying Wi-Fi Direct
     * helper service.  The helper actually talks to an AllJoyn service
     * which, in turn, talks to the Android Application Framework.  Events
     * from the framework are sent back to the helper as AllJoyn signals
     * which then find their way to this listener class.  We just forward
     * them on back to the P2PConMan which digests them.
     */
    class MyP2PHelperListener : public P2PHelperListener {
      public:
        MyP2PHelperListener(P2PConManImpl* cmi) : m_cmi(cmi) { }
        ~MyP2PHelperListener() { }

        virtual void OnFoundAdvertisedName(qcc::String& name, qcc::String& namePrefix, qcc::String& guid, qcc::String& device)
        {
            assert(m_cmi);
            m_cmi->OnFoundAdvertisedName(name, namePrefix, guid, device);
        }

        virtual void OnLostAdvertisedName(qcc::String& name, qcc::String& namePrefix, qcc::String& guid, qcc::String& device)
        {
            assert(m_cmi);
            m_cmi->OnLostAdvertisedName(name, namePrefix, guid, device);
        }

        virtual void OnLinkEstablished(int32_t handle, qcc::String& interface)
        {
            assert(m_cmi);
            m_cmi->OnLinkEstablished(handle, interface);
        }

        virtual void OnLinkError(int32_t handle, int32_t error)
        {
            assert(m_cmi);
            m_cmi->OnLinkError(handle, error);
        }

        virtual void OnLinkLost(int32_t handle)
        {
            assert(m_cmi);
            m_cmi->OnLinkLost(handle);
        }

        virtual void HandleFindAdvertisedNameReply(int32_t result)
        {
            assert(m_cmi);
            m_cmi->HandleFindAdvertisedNameReply(result);
        }

        virtual void HandleCancelFindAdvertisedNameReply(int32_t result)
        {
            assert(m_cmi);
            m_cmi->HandleCancelFindAdvertisedNameReply(result);
        }

        virtual void HandleAdvertiseNameReply(int32_t result)
        {
            assert(m_cmi);
            m_cmi->HandleAdvertiseNameReply(result);
        }

        virtual void HandleCancelAdvertiseNameReply(int32_t result)
        {
            assert(m_cmi);
            m_cmi->HandleCancelAdvertiseNameReply(result);
        }

        virtual void HandleEstablishLinkReply(int32_t handle)
        {
            assert(m_cmi);
            m_cmi->HandleEstablishLinkReply(handle);
        }

        virtual void HandleReleaseLinkReply(int32_t result)
        {
            assert(m_cmi);
            m_cmi->HandleReleaseLinkReply(result);
        }

        virtual void HandleGetInterfaceNameFromHandleReply(qcc::String& interface)
        {
            assert(m_cmi);
            m_cmi->HandleGetInterfaceNameFromHandleReply(interface);
        }

      private:
        P2PConManImpl* m_cmi;
    };

    MyP2PHelperListener* m_myP2pHelperListener; /**< The listener that receives events from the P2P Helper Service */
    P2PHelperInterface* m_p2pHelperInterface;   /**< The AllJoyn interface used to talk to the P2P Helper Service */

    BusAttachment* m_bus;                       /**< The AllJoyn bus attachment that we use to talk to the P2P Helper Service */

    qcc::Mutex m_establishLock;                 /**< Mutex that limits one link establishment at a time */

    int32_t m_establishLinkResult;  /**< The result from an EstablishLinkAsync call done during network connection */
    int32_t m_linkError;            /**< The error reported from an OnLinkError callback done during network connection */

    int32_t m_handle;         /**< The handle returned by the P2P Helper Service that identifies the network connection */
    qcc::String m_device;     /**< The device (which is really the remote MAC address) to which we are connected */
    qcc::String m_interface;  /**< The interface name of the net device supporting our connection (e.g. "p2p0") */
    ConnState m_connState;    /**< The state of the one and only suported temporary network connection */
    ConnType m_connType;      /**< The type of the one and only suported temporary network connection (GO or STA) */
    qcc::Thread* m_l2thread;  /**< A single thread that is blocked waiting for a temporary network to form */
    qcc::Thread* m_l3thread;  /**< A single thread that is blocked waiting for address and port discovery */

    qcc::Mutex m_threadLock;  /**< Mutex that serializes access to Alert() */

    bool m_handleEstablishLinkReplyFired;  /**< Indicates that a HandleEstablishLinkReply() callback happened */
    bool m_onLinkErrorFired;               /**< Indicates that an OnLinkError() callback happened */
    bool m_onLinkEstablishedFired;         /**< Indicates that an OnLinkEstablished() callback happened */

    qcc::Mutex m_discoverLock;             /**< Mutex that limits one link connect spec creation at a time */

    qcc::String m_searchedGuid;            /*<< The GUID that we are searching for IP address and port info about */
    qcc::String m_busAddress;              /*<< The IP name service returned this bus address to match our searchedGuid */
    bool m_foundAdvertisedNameFired;       /**< Indicates that we found IP addressing information corresponding to searchedGuid */

    void FoundAdvertisedName(const qcc::String& busAddr, const qcc::String& guid,
                             std::vector<qcc::String>& nameList, uint32_t timer);

    Callback<void, P2PConMan::LinkState, const qcc::String&>* m_stateCallback;
    Callback<void, const qcc::String&, const qcc::String&, std::vector<qcc::String>&, uint8_t>* m_nameCallback;
};

} // namespace ajn

#endif // _P2P_CON_MAN_IMPL_H
