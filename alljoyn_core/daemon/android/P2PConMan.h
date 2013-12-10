/**
 * @file
 * Singleton for the AllJoyn Android Wi-Fi Direct (Wi-Fi P2P) connection manager
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

#ifndef _P2P_CON_MAN_H
#define _P2P_CON_MAN_H

#ifndef __cplusplus
#error Only include P2PConMan.h in C++ code.
#endif

#include <vector>
#include <qcc/String.h>
#include <alljoyn/Status.h>
#include <Callback.h>
#include "P2PHelperInterface.h"

namespace ajn {

class P2PConManImpl;
class BusAttachment;

/**
 * @brief API to provide an implementation dependent P2P (Layer 2) Connection
 * Manager singleton for AllJoyn.
 */
class P2PConMan {
  public:

    /**
     * @brief Timeout for temporary network creation.
     *
     * This is the timeout used to check to see if a Wi-Fi Direct link is
     * established in a reasonable amount of time.  Although the Wi-Fi Direct
     * spec indicates that it "shall take no more than fifteen seconds to
     * complete Group Formation" there is a second part of this which is the
     * Configuration Timeout.  This can take as long as two minutes to complete
     * due to a wait for user input.  We add these two times to come up with 135
     * seconds.
     */
    static const uint32_t TEMPORARY_NETWORK_ESTABLISH_TIMEOUT = 135000;

    /**
     * Timeout for IP address discovery.  We use inside knowledge of the IP name
     * service to construct this timeout.  When we do a FindAdvertisedName("*")
     * to try and discover a remote daemon, we know that this will result in
     * three who-has requests retried every five seconds.  We expect the remote
     * daemon to respond to each of these requests; so from one perspective it
     * seems that a tad over fifteen seconds might be sufficient.  If, however,
     * the remote side doesn't receive our multicasts for some reason, it will
     * gratuitously multicast all of its well-known names every forty seconds.
     * So to be very generous, we wait around for both of these kinds of events,
     * for a grand total of fifty-five seconds.
     */
    static const uint32_t CREATE_CONNECT_SPEC_TIMEOUT = 55000;

    static const int DEVICE_SHOULD_BE_GO = P2PHelperInterface::DEVICE_SHOULD_BE_GO;
    static const int DEVICE_SHOULD_BE_STA = P2PHelperInterface::DEVICE_SHOULD_BE_STA;

    /**
     * @brief Return a reference to the P2PConMan singleton.
     */
    static P2PConMan& Instance()
    {
        static P2PConMan p2pConMan;
        return p2pConMan;
    }

    /**
     * @brief Notify the singleton that there is a transport coming up that will
     * be using the P2P name service.
     *
     * Whenever a transport comes up and wants to interact with the
     * P2PNameService it calls our static Instance() method to get a reference
     * to the underlying name service object.  This accomplishes the
     * construction on first use idiom.  This is a very lightweight operation
     * that does almost nothing.  The first thing that a transport must do is to
     * Acquire() the instance of the name service, which is going to bump a
     * reference count and do the hard work of actually starting the
     * P2PNameService.  A transport author can think of this call as performing
     * a reference-counted Start()
     *
     * @param bus The bus attachment that will be used to communicate with an
     *     underlying P2P Helper Service.
     * @param guid A string containing the GUID assigned to the daemon which is
     *     hosting the name service.
     */
    void Acquire(BusAttachment* bus, const qcc::String& guid);

    /**
     * @brief Notify the singleton that there a transport is going down and will no
     * longer be using the P2P connection manager.
     */
    void Release();

    /**
     * @brief Determine if the P2PConman singleton has been started.
     *
     * Basically, this determines if the reference count is strictly positive.
     *
     * @return True if the singleton has been started, false otherwise.
     */
    bool Started();

    enum LinkState {
        ESTABLISHED,
        LOST
    };

    /**
     * @brief Set the callback function that is called to notify a transport about
     *     the coming and going of a Wi-Fi Direct link.
     *
     * @param cb The callback method on the transport that will be called to notify
     *     a transport about link state changes.
     */
    void SetStateCallback(Callback<void, LinkState, const qcc::String&>* cb);

    /**
     * @brief Set the callback function that is called to notify a transport about
     *     the coming and going of well-known names found using the IP name service
     *     (which is accessible from here).
     *
     * @param cb The callback method on the transport that will be called to notify
     *     a transport about well-known names discovered over IP.
     */
    void SetNameCallback(Callback<void, const qcc::String&, const qcc::String&, std::vector<qcc::String>&, uint8_t>* cb);

    /**
     * @brief Create a temporary physical network connection to the provided
     *     device MAC address using Wi-Fi Direct.
     *
     * @param[in] device The MAC address of the remote device presented as a string.
     * @param[in] intent The Wi-Fi Direct group owner intent value.
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
     * @brief Determine if the P2PConman is connected to the device with the
     *     given MAC address
     *
     * @param[in] device The MAC address of the remote device presented as a string.
     *
     * @return True if a physical network has been created that allows us to
     *     access <device>.
     */
    bool IsConnected(const qcc::String& device);

    /**
     * @brief Determine if the P2PConman is in the connected state.
     *
     * @return True if a physical network has been created.
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
     * This is a singleton so the constructor is marked private to prevent
     * construction of a P2PConMan instance in any other sneaky way than the
     * Meyers singleton mechanism.
     */
    P2PConMan();

    /**
     * This is a singleton so the destructor is marked private to prevent
     * destruction of an P2PConMan instance in any other sneaky way than the
     * Meyers singleton mechanism.
     */
    virtual ~P2PConMan();

    /**
     * This is a singleton so the copy constructor is marked private to prevent
     * construction of an P2PNameService instance in any other sneaky way than
     * the Meyers singleton mechanism.
     */
    P2PConMan(const P2PConMan& other);

    /**
     * This is a singleton so the assignment constructor is marked private to
     * prevent construction of an P2PNameService instance in any other sneaky
     * way than the Meyers singleton mechanism.
     */
    P2PConMan& operator =(const P2PConMan& other);

    /**
     * @brief Start the P2PConMan singleton.
     *
     * @return ER_OK if the start operation completed successfully, or an error code
     *     if not.
     */
    QStatus Start();

    /**
     * @brief Stop the P2PConMan singleton.
     *
     * @return ER_OK if the stop operation completed successfully, or an error code
     *     if not.
     */
    QStatus Stop();

    /**
     * @brief Join the P2PConMan singleton.
     *
     * @return ER_OK if the join operation completed successfully, or an error code
     *     if not.
     */
    QStatus Join();

    /**
     * @brief Initialize the P2PNameService singleton.
     *
     * Since the P2PNameService is shared among transports, the responsibility for
     * initializing the shared name service should not reside with any single
     * transport.  We provide a reference counting mechanism to deal with this and
     * so the actual Init() method is private and called from the public Acquire().
     * The first transport to Acquire() provides the GUID, which must be unchanging
     * across transports since they are all managed by a single daemon.
     *
     * @param guid A string containing the GUID assigned to the daemon which is
     *     hosting the name service.
     */
    QStatus Init(BusAttachment* bus, const qcc::String& guid);

    bool m_constructed;      /**< State variable indicating the singleton has been constructed */
    bool m_destroyed;        /**< State variable indicating the singleton has been destroyed */
    int32_t m_refCount;      /**< The number of transports that have registered as users of the singleton */
    P2PConManImpl* m_pimpl;  /**< A pointer to the private implementation of the connection manager */
};

} // namespace ajn

#endif // _P2P_CON_MAN_H
