/**
 * @file
 * Data structures used for the AllJoyn IP Name Service
 */

/******************************************************************************
 * Copyright (c) 2010-2014, AllSeen Alliance. All rights reserved.
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

#ifndef _IP_NAME_SERVICE_IMPL_H
#define _IP_NAME_SERVICE_IMPL_H

#ifndef __cplusplus
#error Only include IpNameService.h in C++ code.
#endif

#include <vector>
#include <list>

#include <qcc/String.h>
#include <qcc/Thread.h>
#include <qcc/Mutex.h>
#include <qcc/Socket.h>
#include <qcc/IfConfig.h>
#include <qcc/Timer.h>
#include <qcc/STLContainer.h>
#include <qcc/platform.h>
#include <qcc/StringMapKey.h>
#include <qcc/Thread.h>
#include <qcc/time.h>

#include <alljoyn/TransportMask.h>

#include <alljoyn/Status.h>
#include <Callback.h>

#include "IpNsProtocol.h"
#include "IpNameService.h"

namespace ajn {

/**
 * @brief API to provide an implementation dependent IP (Layer 3) Name Service
 * for AllJoyn.
 *
 * The basic goal of this class is to provide a way for AllJoyn daemons, clients
 * and services to find an IP address and socket to use when connecting to
 * other daemons, clients and services.
 *
 * To first approximation, what we want is to allow a user of AllJoyn to search
 * for IP addresses and ports of daemons that provide some AllJoyn service, as
 * defined by a well-known or bus name.
 *
 * For example, a client may come up and ask, "where is an AllJoyn daemon that
 * implements the org.freedesktop.yadda bus name?  The name service may
 * respond, for example, "one is at IP address 10.0.0.1, listening on port
 * 9955 and another is at IP address 10.0.0.2, listening on port 9955".  The
 * client can then do a TCP connect to one of those addresses and ports.
 */
class IpNameServiceImpl : public qcc::Thread {
  public:
    class PacketScheduler : public qcc::Thread {
      public:
        PacketScheduler(IpNameServiceImpl& impl) : m_impl(impl) { };
        virtual qcc::ThreadReturn STDCALL Run(void* arg);
      private:
        IpNameServiceImpl& m_impl;
    };
    friend class PacketScheduler;
    static const TransportMask TRANSPORT_FIRST_OF_PAIR = TRANSPORT_TCP;
    static const TransportMask TRANSPORT_SECOND_OF_PAIR = TRANSPORT_UDP;
    uint32_t TRANSPORT_INDEX_TCP;
    uint32_t TRANSPORT_INDEX_UDP;

    /**
     * @brief The property value used to specify the wildcard interface name.
     */
    static const char* INTERFACES_WILDCARD;

    /**
     * @brief The maximum size of a name, in general.
     */
    static const uint32_t MAX_NAME_SIZE = 255;

    /**
     * @brief The default time for which an advertisement is valid, in seconds.
     */
    static const uint32_t DEFAULT_DURATION = (120);

    /**
     * @brief The time at which an advertising daemon will retransmit its
     * advertisements.  The advertising daemon should retransmit three
     * times during a default advertisement lifetime.  The 2/3 is really
     * two thirds.  This means that when the countdown time reaches two thirds
     * of the default duration value, one third of the time has expired and we
     * will retransmit.  This, in turn, means we retransmit twice before a
     * remote daemon times out an entry since the timer is set to back to
     * DEFAULT_DURATION after every retransmission.  Units are seconds.
     */
    static const uint32_t RETRANSMIT_TIME = (DEFAULT_DURATION * 2 / 3);

    /**
     * @brief The time at which a daemon using an advertisement begins to think
     * that a remote daemon may be history.  The remote daemon is supposed to
     * retransmit its well-known names periodically.  If we don't receive one
     * of those keepalives, we will start to poke the remote daemon for a
     * keepalive.  Units are seconds.
     */
    static const uint32_t QUESTION_TIME = (DEFAULT_DURATION / 4);

    /**
     * @brief The interval at which the local service will ask a remote daemon
     * if it is alive.
     *
     * When
     */
    static const uint32_t QUESTION_MODULUS = 10;

    /**
     * @brief The number of times we resend WhoHas requests with the time
     * between each request.
     *
     * Legacy 802.11 MACs do not do backoff and retransmission of packets
     * destined for multicast addresses.  Therefore if there is a collision
     * on the air, a multicast packet will be silently dropped.  We get
     * no indication of this at all up at the Socket level.  To avoid this
     * unfortunately common occurrence, which would force a user to wait
     * for the next successful retransmission of exported names, we resend
     * each Locate request multiple times.
     *
     * The number of retries is based on the size of the RETRY_INTERVALS array.
     * The time between each Locate retries is the number of seconds indicated.
     *
     * If the array is { 1, 2, 6, 18 }
     * the first retry will occure in 1 second, second retry 3 seconds later, and
     * so on up till all intervals have been tried.
     *
     * RETRY_INTERVALS is set to { 1, 2, 6, 18 } by default
     */
    static const uint32_t RETRY_INTERVALS[];
    /**
     * The modulus indicating the minimum time between interface lazy updates.
     * Units are seconds.
     */
    static const uint32_t LAZY_UPDATE_MIN_INTERVAL = 5;

    /**
     * The modulus indicating the maximum time between interface lazy updates.
     * Units are seconds.
     */
    static const uint32_t LAZY_UPDATE_MAX_INTERVAL = 15;

    /**
     * @brief The time value indicating an advertisement is valid forever.
     */
    static const uint32_t DURATION_INFINITE = 255;

    /**
     * The modulus indicating minimum time interval between the initial burst of
     * unsolicited IS-AT packets. Units are in mill-seconds.
     */
    static const uint32_t BURST_RESPONSE_INTERVAL = 100;

    /**
     * The number burst responses sent each time the burst response is sent.
     * The BURST_RESPONSE_RETRIES must be greater than or equal 1
     */
    static const uint32_t BURST_RESPONSE_RETRIES = 3;

    /**
     * @brief The maximum size of the payload of a name service message.
     *
     * An easy choice for this number would be 64K - 8 bytes (the max size of
     * a UDP payload).  The problem is we need to allocate a buffer of that
     * size on the receiver, and we really expect that payloads will be quite
     * small.
     *
     * Another option is to look at the maximum (or minimum) MTUs of the
     * interfaces over which we will send messages.  This leads to possibly
     * confusing behaviors as different combinations of MTUs in different
     * machines can successfully support different numbers of names at
     * different times based on different configurations.
     *
     * It seems better to have a hard limit that can be easily worked around
     * than a possibly confusing limit that implies flakiness.  We can always
     * support 1500 bytes through UDP fragmentation, and we will be using
     * IP and multicast-capable devices, so we expect an MTU of 1500 in the
     * typical case.  So we just work with that as a compromise.  We then take
     * the typical MTU and subtrace UDP, IP and Ethernet Type II overhead.
     *
     * 1500 - 8 -20 - 18 = 1454
     *
     * TODO:  This should probably end up a configurable item for a daemon in
     * case we underestimated the numbers and sizes of exported names.
     */
    static const size_t NS_MESSAGE_MAX = (1454);

    /**
     * @brief Which protocol is of interest.  When making discovery calls, the
     * client must choose whether it is interested in IPv4 or IPv6 addresses.
     * Use these constants to specify which is desired.
     */
    enum Protocol {
        UNSPEC = 0, /** Unspecified */
        IPV4 = 1,   /**< Return the address in IPv4 suitable form */
        IPV6 = 2    /**< Return the address in IPv6 suitable form */
    };

    const static uint8_t TRANSMIT_V0 = 1;
    const static uint8_t TRANSMIT_V1 = 2;
    const static uint8_t TRANSMIT_V2 = 4;
    const static uint8_t TRANSMIT_V0_V1 = (TRANSMIT_V0 | TRANSMIT_V1);

    /**
     * @internal
     *
     * @brief Construct an IP name service object.
     */
    IpNameServiceImpl();

    /**
     * @internal
     *
     * @brief Destroy an IP name service object.
     */
    virtual ~IpNameServiceImpl();

    /**
     * @brief Initialize the name service.
     *
     * Some operations relating to initializing the name service and
     * arranging the communication with an underlying network can fail.
     * These operations are broken out into an Init method so we can
     * return an error condition.  You may be able to try and Init()
     * at a later time if an error is returned.
     *
     * @param guid The daemon guid of the daemon using this service.
     * @param loopback If true, receive our own advertisements.
     *     Typically used for test programs to listen to themselves talk.
     *
     * @return Status of the operation.  Returns ER_OK on success.
     *
     * @see OpenInterface()
     */
    QStatus Init(const qcc::String& guid, bool loopback = false);

    /**
     * @brief Start any required name service threads.
     *
     * @param arg        Parameter not used (defaults to NULL).
     * @param listener   Listener to be informed of Thread events (defaults to NULL).
     *
     * @return  Indication of whether creation of the thread succeeded or not.
     */
    QStatus Start(void* arg = NULL, qcc::ThreadListener* listener = NULL);

    /**
     * @brief return true if name service threads are running
     */
    bool Started();

    /**
     * @brief Stop any name service threads.
     */
    QStatus Stop();

    /**
     * @brief Join any name service threads.
     */
    QStatus Join();

    /**
     * @brief Provide parameters to define the general operation of the protocol.
     *
     * @warning Calling this method is not recommended unless for testing.
     */
    void SetCriticalParameters(uint32_t tDuration, uint32_t tRetransmit, uint32_t tQuestion,
                               uint32_t modulus, uint32_t retries);

    /**
     * @brief Creat a virtual network interface. In normal cases WiFi-Direct
     * creates a soft-AP for a temporary network. In some OSs like WinRT, there is
     * no API to detect the presence of the soft-AP. Thus we need to manually create
     * a virtual network interface for it.
     *
     * @param entry Contains the network interface parameters
     *
     * @return Status of the operation.  Returns ER_OK on success.
     *
     * @see qcc::IfConfig()
     * @see qcc::IfConfigEntry
     */
    QStatus CreateVirtualInterface(const qcc::IfConfigEntry& entry);

    QStatus CreateUnicastSocket();

    /**
     * @brief Delete a virtual network interface. In normal cases WiFi-Direct
     * creates a soft-AP for a temporary network. Once the P2P keep-alive connection is
     * terminated, we delete the virtual network interface that represents the
     * soft-AP.
     *
     * @param ifceName The virtual network interface name
     *
     * @return Status of the operation.  Returns ER_OK on success.
     *
     */
    QStatus DeleteVirtualInterface(const qcc::String& ifceName);

    /**
     * @brief Tell the name service to begin listening and transmitting
     * on the provided network interface.
     *
     * There may be a choice of network interfaces available to run the name
     * service protocol over.  A user of the name service can find these
     * interfaces and explore their characteristics using qcc::IfConfig().
     * When it is decided to actually use one of the interfaces, one can pass
     * the m_name (interface name) variable provided in the selected
     * qcc::IfConfigEntry into this method to enable the name service for that
     * interface (or pass a configured name).
     *
     * If the interface is not IFF_UP, the name service will periodically
     * check to see if one comes up and will begin to use it whenever it
     * can.
     *
     * @param transportMask A bitmask containing the transport requesting the
     *     advertisements.
     * @param name The interface name of the interface to start talking to
     *     and listening on (e.g., eth1 or wlan0).
     *
     * @return Status of the operation.  Returns ER_OK on success.
     *
     * @see qcc::IfConfig()
     * @see qcc::IfConfigEntry
     */
    QStatus OpenInterface(TransportMask transportMask, const qcc::String& name);

    /**
     * @brief Tell the name service to begin listening and transmitting
     * on the provided network interface.
     *
     * There may be a choice of network interfaces available to run the name
     * service protocol over.  A user of the name service can find these
     * interfaces and explore their characteristics using qcc::IfConfig().
     * When it is decided to actually use one of the interfaces, pass an
     * IPAddress constructed using the m_addr (interface address) variable
     * provided in the selected qcc::IfConfigEntry into this method to enable
     * the name service for that address.
     *
     * If there is no interface that is IFF_UP with the specific address
     * the name service will periodically check to see if one comes up
     * and will begin to use it whenever it can.
     *
     * @param transportMask A bitmask containing the transport requesting the
     *     advertisements.
     * @param name The interface name of the interface to start talking to
     *     and listening on (e.g., eth1 or wlan0).
     *
     * @return Status of the operation.  Returns ER_OK on success.
     *
     * @see qcc::IfConfig()
     * @see qcc::IfConfigEntry
     */
    QStatus OpenInterface(TransportMask transportMask, const qcc::IPAddress& address);

    /**
     * @brief Tell the name service to stop listening and transmitting
     * on the provided network interface.
     *
     * @param transportMask A bitmask containing the transport requesting the
     *     advertisements.
     * @param address The interface name of the interface to stop talking
     *     to and listening on.
     *
     * @return Status of the operation.  Returns ER_OK on success.
     *
     * @see OpenInterface
     */
    QStatus CloseInterface(TransportMask transportMask, const qcc::String& name);

    /**
     * @brief Tell the name service to stop listening and transmitting
     * on the provided network interface.
     *
     * @param transportMask A bitmask containing the transport requesting the
     *     advertisements.
     * @param address The IP address the interface to stop talking
     *     to and listening on.
     *
     * @return Status of the operation.  Returns ER_OK on success.
     *
     * @see OpenInterface
     */
    QStatus CloseInterface(TransportMask transportMask, const qcc::IPAddress& address);

    /**
     * @brief Notify the name service that that there is or is not a listener on
     *     the specified endpoints.
     *
     * The IpNameService is shared among several transports.  In order to
     * advertise the presence of a network endpoint managed by a transport, the
     * transports need to advise us of the IP addresses and ports on which it
     * can be contacted.  Each transport may use a different set of addresses
     * and ports, and so each transport must identify itself to the name
     * service.  This is done using the TransportMask.  There is a bit in the
     * TransportMask for each transport in the system.
     *
     * A transport is defined as a module that provides reliable and/or
     * unreliable data transfer over IPv4 and/or IPv6.  Support for reliable and
     * unreliable modes is optional, as is support for IPv4 and IPv6.  Port
     * numbers for reliable, unreliable, IPv4 and IPv6 may also differ.  An
     * enable bit is conceptually required since support for each endpoint type
     * is optional.  This makes for thirteen parameters.
     *
     *     transportMask,
     *     enableReliableIPv4, reliableIPv4Addr, reliableIPv4Port
     *     enableReliableIPv6, reliableIPv6Addr, reliableIPv6Port
     *     enableUnreliableIPv4, unreliableIPv4Addr, unreliableIPv4Port
     *     enableUnReliableIPv6, unreliableIPv6Addr, unreliableIPv6Port
     *
     * It turns out that since AllJoyn lives in a mobile environment, the
     * transport must listen on the "any" address.  This is because an IP
     * address assigned to a given network interface cannot generally be
     * predicted in advance.
     *
     * The mechanism used to "control" which network interfaces can accept
     * incoming connections, is the presence of outgoing advertisements on those
     * interfaces.  If a transport desires to accept connections over an
     * interface, it performs an OpenInterface() on that interface which begins
     * the process of sending advertisements.  The IP (V4 and or V6) address of
     * the interface is added to advertisements and sent out over the network.
     * This which provides the receiver of the advertisements with an IP address
     * and port to connect to.  The advertising transport, since it is listening
     * on the "any" address, will respond to all connect requests coming from
     * clients on the associated network.  If the transport wants to filter
     * further on source IP addresses, it certainly can; but this is not a
     * concern of the name service.
     *
     * This all means that we do not need to specify the IP addresses on which
     * the reliable and unreliable protocols are listening. This means that
     * there are nine required parameters to this call instead of thirteen:
     *
     *     transportMask,
     *     reliableIPv4Port, reliableIPv6Port,
     *     unreliableIPv4Port, unreliableIPv6Port,
     *     enableReliableIPv4, enableReliableIPv6,
     *     enableUnreliableIPv4, enableUnreliableIPv6
     *
     * In many cases, the transports will not support all combinations.  For
     * example, the tcp transport currently only supports reliable IPv4
     * connections, and so the call for this transport might be:
     *
     *     Enable(TRANSPORT_TCP, 9955, 0, 0, 0, true, false, false, false);
     *
     * The Android Compatibility Test Suite demands that an Android phone may
     * not hold an open socket in the quiescent state.  Since we provide a
     * native daemon that always runs on the phone, and the daemon has
     * transports that want to listen on sockets, there must be a way to close
     * those sockets (and the multicast sockets in the name service) and not
     * use them at all when there are no advertisements or discovery operations
     * in progress.  This also helps with power consumption.
     *
     * Enable() communicates the fact that there is or is not a listener for the
     * specified transport port and this, in turn, implies whether or not
     * advertisements should be sent and received over the interfaces opened by
     * the given transport.
     *
     * @param transportMask A bitmask containing the transport handling the specified
     *     endpoints.
     * @param reliableIPv4PortMap Indicates a map of interfaces to port numbers
     *     of a server listening for connections if enableReliableIPv4 is true
     * @param reliableIPv6Port Indicates the port number of a server listening for
     *     connections if enableUnreliableIPv6 is true
     * @param unreliableIPv4Port Indicates a map of interfaces to port numbers
     *     of a server listening for connections if enableUnreliableIPv4 is true
     * @param unreliableIPv6Port Indicates the port number of a server listening for
     *     connections if enableUnreliableIPv6 is true.
     * @param enableReliableIPv4
     *     - true indicates this protocol is enabled.
     *     - false indicates this protocol is not enabled.
     * @param enableReliableIPv6
     *     - true indicates this protocol is enabled.
     *     - false indicates this protocol is not enabled.
     * @param enableUnreliableIPv4
     *     - true indicates this protocol is enabled.
     *     - false indicates this protocol is not enabled.
     * @param enableUnreliableIPv4
     *     - true indicates this protocol is enabled.
     *     - false indicates this protocol is not enabled.
     */
    QStatus Enable(TransportMask transportMask,
                   const std::map<qcc::String, uint16_t>& reliableIPv4PortMap, uint16_t reliableIPv6Port,
                   const std::map<qcc::String, uint16_t>& unreliableIPv4PortMap, uint16_t unreliableIPv6Port,
                   bool enableReliableIPv4, bool enableReliableIPv6,
                   bool enableUnreliableIPv4, bool enableUnreliableIPv6);

    /**
     * @brief Ask the name service whether or not it thinks there is or is not a
     *     listener on the specified ports for the given transport.
     *
     * @param transportMask A bitmask containing the transport handling the specified
     *     endpoints.
     * @param reliableIPv4PortMap If empty, indicates this protocol is not enabled.  If
     *     not empty, indicates the interfaces/port numbers of a server listening for connections.
     * @param reliableIPv6Port If zero, indicates this protocol is not enabled.  If
     *     non-zero, indicates the port number of a server listening for connections.
     * @param unreliableIPv4PortMap If empty, indicates this protocol is not enabled.  If
     *     not empty, indicates the interfaces/port numbers of a server listening for connections.
     * @param unreliableIPv6Port If zero, indicates this protocol is not enabled.  If
     *     non-zero, indicates the port number of a server listening for connections.
     */
    QStatus Enabled(TransportMask transportMask,
                    std::map<qcc::String, uint16_t>& reliableIPv4PortMap, uint16_t& reliableIPv6Port,
                    std::map<qcc::String, uint16_t>& unreliableIPv4PortMap, uint16_t& unreliableIPv6Port);

    /**
     * Allow a user to select what kind of retry policy should be used when
     * trying to locate names.  There really isn't one obvious policy.  Consider
     * what happens if the question is locate well-known name N from local
     * daemon L.  If the locate is transmitted and all remote daemons having N
     * hear the WHO-HAS and respond, we certainly do not need to retransmit.
     * If all but one remote daemons that have N hear the question, and respond,
     * is it okay to decide not to ping the remaining daemon?  We cannot
     * possibly know that this situation has happened.  To try and ping a
     * remote daemon that could have missed our request, we would necessarily
     * have to just continue retrying.  One can imagine the case where a
     * single response from any remote daemon would satisfy a user "enough" to
     * satisfy an end-user.  One could imagine a situation where one of a
     * list of names would be satisfactory (several services that accomplish
     * basically the same thing).  One could imagine a situation where the
     * entire list must be found to do the useful thing.
     *
     * To avoid trying to make a single pronouncement on the best way to do
     * things, we provide a selectable policy.
     */
    enum LocatePolicy {
        ALWAYS_RETRY = 1,       /**< Always send the default number of retries */
        RETRY_UNTIL_PARTIAL,    /**< Retry until we get at least one of the names, or run out of retries */
        RETRY_UNTIL_COMPLETE    /**< Retry until we get all of the names, or run out of retries */
    };

    /**
     * @brief Express an interest in locating instances of AllJoyn daemons
     * which support the provided well-known name.
     *
     * Calling this method will result in a name resolution request being
     * multicast to the local subnet.  Other instances of the name service that
     * know about daemons that match the constraints will respond to this
     * request.
     *
     * Responses to this request will be filtered after the first response from
     * each remote daemon.  If, for some reason, the local daemon wants to be
     * re-notified of remote names, it can call this method.  In that case, all
     * state information regarding previous notifications will be dropped and
     * the daemon will get a single repeat notification for each remote name.
     *
     * If users of the name service are interested in being notified of these
     * of services, they are expected to set the Found callback function using
     * SetFoundCallback().
     *
     * @see SetFoundCallback()
     *
     * There is also a corresponding Lost callback used to be notified when
     * services vanish off the net.
     *
     * @see SetLostCallback()
     * @see LocatePolicy
     *
     * @param[in] transportMask A bitmask containing the transport requesting the
     *     discovery operation.
     * @param[in] matching The Key, value match criteria that the caller wants
     *     to be notified of (via signal) when a remote Bus instance is found
     *     with an advertisement that matches the criteria.
     * @param[in] policy The retransmission policy for this Locate event.
     *
     * @return Status of the operation.  Returns ER_OK on success.
     */
    QStatus FindAdvertisement(TransportMask transportMask, const qcc::String& matching, LocatePolicy policy, TransportMask completeTransportMask);

    /**
     * @brief Cancel an interest in locating instances of AllJoyn daemons
     * which support the provided well-known name.
     *
     * If an AllJoyn daemon wants to cancel an interest in locating instances
     * of AllJoyn daemons which support the provided well-known name
     * it calls this function.
     *
     * @param[in] transportMask A bitmask containing the transport
     * @param[in] matching The Key, value match criteria that the caller wants
     *     no longer be notified of (via signal) when a remote Bus instance is
     *     found with an advertisement that matches the criteria.
     * @param[in] policy The retransmission policy for the Locate event
     *     to be cancelled.
     *
     * @return Status of the operation.  Returns ER_OK on success.
     */
    QStatus CancelFindAdvertisement(TransportMask transportMask, const qcc::String& matching, LocatePolicy policy, TransportMask completeTransportMask);

    /**
     * @brief Refresh the cache of names that we have seen and maintain the
     * updated state of PeerInfoMap
     * @param[in] guid Guid for which Cache refresh has been initiated
     * @param[in] matchingStr The name for which we are sending the cache refresh
     * @param[in] policy Policy for refresh
     */

    QStatus RefreshCache(TransportMask transportMask, const qcc::String& guid, const qcc::String& matchingStr, LocatePolicy policy = ALWAYS_RETRY, bool ping = false);

    /**
     * @brief Set the Callback for notification of discovery events.
     *
     * When using an asynchronous service discovery process, a caller will
     * need to specify how to called back when a service appears, disappears
     * or reaffirms its existence on the network.  This method provides the
     * mechanism for specifying the callback.
     *
     * The method signature for the Callback method must be:
     *
     * @code
     *     void Found(const qcc::String &, const qcc::String &, std::vector<qcc::String> &, uint32_t);
     * @endcode
     *
     * The first parameter is the address and port of the found service,
     * formatted as a bus address the way AllJoyn likes, for example,
     * "tcp:addr=192.168.0.1,port=9955".  The second parameter is the daemon
     * guid string exported by the remote daemon service, or the empty string
     * if that daemon didn't bother to export the string.  The third parameter
     * is a vector of qcc::String that represent the well-known names that the
     * remote daemon is referring to, for example, "org.freedesktop.Yadda".
     * The fourth parameter is the timer value.  A timer value of zero indicates
     * that the names provided in the vectoror qcc::String are no longer
     * available.  A timer value of 255 indicates that the names provided should
     * be interpreted as always available, to the extent that is possible.
     * A timer value between 0 and 255 indicates the number of seconds that the
     * name is expected to be valid.  There will be keepalive messages provided
     * which may extend this time periodically.
     *
     * The typical idiom for using the callback facility is to define a method
     * to receive a found callback in your class and instantiate your class
     * (and this one) somewhere:
     *
     * @code
     *     class MyClass:
     *     public:
     *         void Found(const qcc::String &s, const qcc::String &g, std::vector<qcc::String>& wkn, uint32_t timer) {}
     *     }
     *
     *     ...
     *
     *     MyClass myClass;
     *     IpNameServiceImpl nameService;
     * @endcode
     *
     * The callback is connected using the SetCallback() method of this
     * class.
     *
     * @code
     *    nameService.SetCallback(new CallbackImpl<MyClass, void,
     *         const qcc::String &, const qcc::String &, std::vector<qcc::string>, uint32_t>(MyClass, myclass));
     * @endcode
     *
     * From this point forward, all changes to services specified by the
     * Locate() call will be set to you through the callback mechanism.
     *
     * To stop notifications, set the callback (cb) to the pointer value 0.
     *
     * @warning The callback will be in the context of a different thread than
     * your thread, so your Found callback code must be multithread safe (or aware
     * at least).
     *
     * @warning Services may come and go constantly during real network operation.
     * Just because a service was found on the network it does not mean that there
     * will be a service waiting on the provided IP address and port.  This
     * service may be gone by the time you connect; and this is a perfectly
     * legal and reasonable situation.
     *
     * @param[in] cb the Callback pointer.
     *
     * @return Status of the operation.  Returns ER_OK on success.
     */
    QStatus SetCallback(TransportMask transportMask,
                        Callback<void, const qcc::String&, const qcc::String&, std::vector<qcc::String>&, uint32_t>* cb);

    /**
     * @brief Set the Callback for notification of network interface events.
     *
     */
    QStatus SetNetworkEventCallback(TransportMask transportMask,
                                    Callback<void, const std::map<qcc::String, qcc::IPAddress>&>* cb);

    /**
     * @brief Clear the callbacks for all transports.
     *
     * Used during the shutdown of the IpNameService singleton to make sure that
     * no more callbacks escape from the implementation.
     */
    void ClearCallbacks(void);

    /**
     * @brief Clear the network interface event callbacks for all transports.
     *
     */
    void ClearNetworkEventCallbacks(void);

    /**
     * @brief Advertise an AllJoyn daemon service.
     *
     * If an AllJoyn daemon wants to advertise the presence of a well-known name
     * on the local subnet(s) it calls this function.  It must have previously
     * provided an appropriately formatted address in presentation format (IPV4 or
     * IPV6) and port over which it may be contacted.
     *
     * This method allows the caller to specify a single well-known interface
     * name supported by the exporting AllJoyn. If the AllJoyn supports multiple
     * interfaces, it is more efficient to call the overloaded method which
     * takes a vector of string.
     *
     * @see SetEndpoints()
     *
     * @param[in] transportMask A bitmask containing the transport requesting the
     *     advertisement.
     * @param[in] wkn The AllJoyn interface (e.g., "org.freedesktop.Sensor").
     * @param[in] quietly The quietly parameter, if true, specifies to not do
     *     gratuitous advertisements (send periodic is-at messages indicating we
     *     have the provided name available) but do respond to who-has requests
     *     for the name.
     *
     * @return Status of the operation.  Returns ER_OK on success.
     */
    QStatus AdvertiseName(TransportMask transportMask, const qcc::String& wkn, bool quietly, TransportMask completeTransportMask);

    /**
     * @brief Cancel an AllJoyn daemon service advertisement.
     *
     * If an AllJoyn daemon wants to cancel an advertisement of a well-known name
     * on the local subnet(s) it calls this function.
     *
     * @param[in] wkn The AllJoyn interface (e.g., "org.freedesktop.Sensor").
     *
     * @return Status of the operation.  Returns ER_OK on success.
     */
    QStatus CancelAdvertiseName(TransportMask transportMask, const qcc::String& wkn, TransportMask completeTransportMask);

    /**
     * @internal
     * @brief Advertise an AllJoyn daemon service.
     *
     * If an AllJoyn daemon wants to advertise the presence of a well-known name
     * on the local subnet(s) it calls this function.  It must have previously
     * provided an appropriately formatted address in presentation format (IPV4 or
     * IPV6) and port over which it may be contacted.
     *
     * This method allows the caller to specify multiple well-known interface
     * name supported by the exporting AllJoyn.  If the AllJoyn supports multiple
     * interfaces, this is the preferred method.
     *
     * @see SetEndpoints()
     *
     * @param[in] transportMask A bitmask containing the transport requesting the
     *     advertisement.
     * @param[in] wkn A vector of AllJoyn well-known names (e.g., "org.freedesktop.Sensor").
     *
     * @return Status of the operation.  Returns ER_OK on success.
     */
    QStatus AdvertiseName(TransportMask transportMask, std::vector<qcc::String>& wkn, bool quietly, TransportMask completeTransportMask);

    /**
     * @brief Cancel an AllJoyn daemon service advertisement.
     *
     * If an AllJoyn daemon wants to cancel an advertisement of a well-known name
     * on the local subnet(s) it calls this function.
     *
     * @param[in] transportMask A bitmask containing the transport requesting the
     *     advertisement be canceled.
     * @param[in] wkn A vector of AllJoyn well-known names (e.g., "org.freedesktop.Sensor").
     *
     * @return Status of the operation.  Returns ER_OK on success.
     */
    QStatus CancelAdvertiseName(TransportMask transportMask, std::vector<qcc::String>& wkn, TransportMask completeTransportMask);

    /**
     * @brief Returns a count of the number of names currently being advertised
     *
     * @param[in] transportMask A bitmask containing the transport requesting the
     *     advertisement be canceled.
     *
     * @return  Returns the number of names currently being advertised
     */
    size_t NumAdvertisements(TransportMask transportMask);

    /**
     * @brief Handle the suspending event of the process. Release exclusive held socket file descriptor and port.
     */
    QStatus OnProcSuspend();

    /**
     * @brief Handle the resuming event of the process. Re-acquire exclusive held socket file descriptor and port.
     */
    QStatus OnProcResume();

    void RegisterListener(IpNameServiceListener& listener);

    void UnregisterListener(IpNameServiceListener& listener);

    QStatus Ping(TransportMask transportMask, const qcc::String& guid, const qcc::String& name);

    QStatus Query(TransportMask transportMask, MDNSPacket mdnsPacket);

    QStatus Response(TransportMask transportMask, uint32_t ttl, MDNSPacket mdnsPacket);

    /**
     * @internal
     * @brief Update the AddToPeerInfoMap
     *
     */
    bool AddToPeerInfoMap(const qcc::String& guid, const qcc::IPEndpoint& ipEndpoint);

    /**
     * @internal
     * @brief Remove an entry from the PeerInfoMap
     *        This will be useful during a cache refresh expiry for a guid
     *
     */
    bool RemoveFromPeerInfoMap(const qcc::String& guid);


  private:
    /**
     * @brief Copying an IpNameServiceImpl object is forbidden.
     */
    IpNameServiceImpl(const IpNameServiceImpl& other);

    /**
     * @brief Assigning an IpNameServiceImpl object is forbidden.
     */
    IpNameServiceImpl& operator =(const IpNameServiceImpl& other);

    /**
     * @brief The number of transports that can be represented in a 16-bit
     * transport mask.
     */
    static const uint32_t N_TRANSPORTS = 16;

    /**
     * @brief The temporary IPv4 multicast address for the multicast name
     * service.
     */
    static const char* IPV4_MULTICAST_GROUP;

    /**
     * @brief The IANA assigned IPv4 multicast address for the multicast name
     * service.
     *
     * @see IPv4 Multicast Address space Registry IANA
     */
    static const char* IPV4_ALLJOYN_MULTICAST_GROUP;

    /**
     * @brief The temporary IPv6 multicast address for the multicast name
     * service.
     *
     * @see IPv4 Multicast Address space Registry IANA
     */
    static const char* IPV6_MULTICAST_GROUP;

    /**
     * @brief The IANA assigned IPv6 multicast address for the multicast name
     * service.
     *
     * @see IPv4 Multicast Address space Registry IANA
     */
    static const char* IPV6_ALLJOYN_MULTICAST_GROUP;

    /**
     * @brief The IANA assigned IPv4 multicast address for the MDNS
     * service.
     *
     * @see IPv4 Multicast Address space Registry IANA
     */
    static const char* IPV4_MDNS_MULTICAST_GROUP;

    /**
     * @brief The IANA assigned IPv6 multicast address for the MDNS
     * service.
     *
     * @see IPv4 Multicast Address space Registry IANA
     */
    static const char* IPV6_MDNS_MULTICAST_GROUP;

    /**
     * @brief The port number for the  multicast name service.
     * Should eventually be registered with IANA.
     *
     * @see IPv4 Multicast Address space Registry IANA
     */
    static const uint16_t MULTICAST_PORT;

    /**
     * @brief The port number for the broadcast name service packets.
     * Typically the same port as the multicast case, but can be made
     * different (with a little work).
     */
    static const uint16_t BROADCAST_PORT;

    /**
     * @brief The port number for the  MDNS name service.
     *
     * @see IPv4 Multicast Address space Registry IANA
     */
    static const uint16_t MULTICAST_MDNS_PORT;

    /**
     * @brief The port number for the broadcast MDNS name service packets.
     * Typically the same port as the multicast case, but can be made
     * different (with a litle work).
     */
    static const uint16_t BROADCAST_MDNS_PORT;

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
     * @internal
     * @brief State variable to indicate what the implementation is doing or is
     * capable of doing.
     */
    State m_state;

    /**
     * @internal
     * @brief State variable to indicate whether the OS is suspending the process.
     */
    bool m_isProcSuspending;
    /**
     * @internal
     * @brief State variable to indicate that the name service is in the process
     * of sending its terminal is-at messages indicating that any currently
     * advertised names are becoming invalid.
     */
    bool m_terminal;

    class InterfaceSpecifier {
      public:
        TransportMask m_transportMask;      /**< The transport mask that identifies the transport talking to the interface */
        qcc::String m_interfaceName;        /**< The interface (cf. eth0) we want to talk to */
        qcc::IPAddress m_interfaceAddr;     /**< The address (cf. 1.2.3.4) we want to talk to */
    };

    class LiveInterface : public InterfaceSpecifier {
      public:
        qcc::IPAddress m_address;   /**< The address of the interface we are talking to */
        uint16_t m_multicastPort;       /**< The multicast port we are using to talk */
        uint16_t m_unicastPort;         /**< The unicast port we are using to talk */
        uint16_t m_multicastMDNSPort;   /**< The multicast MDNS port we are using to talk */
        uint32_t m_prefixlen;       /**< The address prefix (cf netmask) of the interface we are talking to */

        qcc::SocketFd m_multicastsockFd;     /**< The multicast socket we are using to talk over */
        qcc::SocketFd m_multicastMDNSsockFd; /**< The multicast MDNS socket we are using to talk over */

        qcc::Event* m_multicastevent;      /**< The event for the multicast socket we use to get read notifications over */
        qcc::Event* m_multicastMDNSevent;  /**< The event for the multicast MDNS socket we use to get read notifications over */

        uint32_t m_mtu;             /**< The MTU of the protocol/device we are using */
        uint32_t m_index;           /**< The interface index of the protocol/device we are using if IPv6 */
        uint32_t m_flags;           /**< The flags we found during the qcc::IfConfig() that originally discovered this iface */
    };

    /**
     * @internal
     * @brief A vector of information specifying any interfaces we have manual created
     */
    std::vector<qcc::IfConfigEntry> m_virtualInterfaces;

    /**
     * @internal
     * @brief A vector of information specifying any interfaces we may want to
     * send or receive multicast packets over for a particular transport.
     * Interfaces in this vector may be up or down, or may be completely
     * unrelated to any interface in the actual host system.  These are what the
     * user is telling us to use.
     */
    std::vector<InterfaceSpecifier> m_requestedInterfaces[N_TRANSPORTS];

    /**
     * @internal
     * @brief A vector of information specifying any interfaces we have actually
     * decided to send or receive multicast packets over.  Interfaces in this
     * must have been up when they were added, but may have since gone down.
     * These are interfaces we decided to use based on what the user told us
     * to use.
     */
    std::vector<LiveInterface> m_liveInterfaces;

    /**
     * @internal
     * @brief Mutex object used to protect various lists that may be accessed
     * by multiple threads.
     */
    qcc::Mutex m_mutex;

    /**
     * @internal
     * @brief Variable used to indicate that callbacks are currently in use and
     * may not be deleted.
     */
    bool m_protect_callback;

    /**
     * @internal
     * @brief Variable used to indicate that network event callbacks are currently
     * in use and may not be deleted.
     */
    bool m_protect_net_callback;

    /**
     * Send outbound name service messages out the current live interfaces.
     */
    void SendOutboundMessages(void);

    /**
     * Send outbound name service messages over unicast, out the network
     * interfaces implied by the destination address in the header.
     */
    void SendOutboundMessageQuietly(Packet packet);

    /**
     * Send outbound name service messages over multicast, out the list of live
     * interfaces implied by the transport masks in the message.
     */
    void SendOutboundMessageActively(Packet packet, const qcc::IPAddress& localAddress = qcc::IPAddress("0.0.0.0"));

    /**
     * Main thread entry point.
     *
     * @param arg  Unused thread entry arg.
     */
    qcc::ThreadReturn STDCALL Run(void* arg);

    /**
     * @internal
     * @brief Queue a protocol message for transmission out on the multicast
     * group.
     */
    void QueueProtocolMessage(Packet packet);

    /**
     * @internal
     * @brief Trigger transmission of a particular packet at a defined schedule.
     * group.
     */
    void TriggerTransmission(Packet packet);

    /**
     * @internal
     * @brief Send a protocol message out on the multicast group.
     *
     * @param sockFd The socket FD that is used to send and receive to and from
     *     the life interface to which the name service message will be sent.
     * @param interfaceAddress The IPV4 address of the interface which is used
     *     as a template for constructing a subnet directed broadcast address.
     *     This address provides the network number part of the broadcast
     *     address.  Only useful if sockFd is of family QCC_AF_INET.
     * @param interfaceAddressPrefixLen The network prefix length of the
     *     networkd which is used to determine which part of <interfaceAddress>
     *     contains the network part of the address.
     * @param flags The flags (IFF_*) bits from the network interface.  Used
     *     to decide if the network is broadcast- or multicast-capable.
     * @param sockFdIsIPv4 Used to tell what address family sockFd has been
     *     initialized to and therefore what kind of multicast needs to be done
     *     or whether or not broadcast is meaningful.
     * @param interfaceIndex The index into the live interfaces array
     *     corresponding to the interface we're going to send to.  Really just
     *     used for debugging.
     */
    void SendProtocolMessage(
        qcc::SocketFd sockFd,
        qcc::IPAddress interfaceAddress,
        uint32_t interfaceAddressPrefixLen,
        uint32_t flags,
        bool sockFdIsIPv4,
        Packet packet,
        uint32_t interfaceIndex,
        const qcc::IPAddress& localAddress =  qcc::IPAddress("0.0.0.0"));


    /**
     * @internal
     * @brief Is the interface specified by the liveIndex an interface requested
     * to be opened by the transport specified by the transportIndex.
     *
     * @param transportIndex An index into the per-transport data structures
     *     derived from the transport mask of the transport under consideration.
     * @param liveIndex An index into the list of live interfaces specifying the
     *     index under consideration.
     */
    bool InterfaceRequested(uint32_t transportIndex, uint32_t liveIndex);

    /**
     * @internal
     * @brief Rewrite (set) the IPv4 and IPv6 addresses of the provided is-at message
     * respecting the version of the message as specified by msgVersion.
     *
     * @param msgVersion The version of the message that is being constructed.
     * @param isAt A pointer to the is-at message that is to be rewritten.
     * @param haveIPv4address If true, the provided ipv4Address is valid and
     *     should be used.
     * @param iPv4address The qcc::IPAddress of the IPv4 interface for which
     *      the is-at message is eventually destined.
     * @param haveIPv6address If true, the provided ipv6Address is valid and
     *      should be used.
     * @param iPv6address The qcc::IPAddress of the IPv6 interface for which
     *     the is-at message is eventually destined.
     * @param iPv4port The port that the daemon is listening to of the IPv4
     *     interface for which the is-at message is eventually destined.
     * @param iPv6port The port that the daemon is listening to of the IPv6
     *     interface for which the is-at message is eventually destined.
     * @param interface The network interface name over which the message
     *     is to be sent.
     */
    void RewriteVersionSpecific(uint32_t msgVersion, Packet packet,
                                bool haveIPv4address, qcc::IPAddress ipv4address,
                                bool haveIPv6Address, qcc::IPAddress ipv6address,
                                uint16_t unicastIpv4Port = 0, const qcc::String& interface = qcc::String(),
                                const uint16_t reliableTransportPort = 0, const uint16_t unreliableTransportPort = 0);

    /**
     * @internal
     * @brief Are the provided IP Addresses on the same network?
     *
     * Take a look at the network number part of the two IP addresses (as
     * determined by the prefix length) and compare them.  If the two networks
     * both appear to be on the same network, then return true.
     *
     * @warning This method does not take the interface index of an IP address
     * into account, so two network interfaces, both connected to a private
     * network address (192.168.x.x) may be erroneously identified as belonging
     * to the same network.  In this case, it might result in a name service
     * packet being unintentionally sent to the "duplicate" network as well as
     * the "correct" network.
     *
     * @param interfaceAddressPrefixLen The network prefix length of both
     *     networks.
     * @param addressA The qcc::IPAddress of the first of the two networks.
     * @param addressB The qcc::IPAddress of the first of the two networks.
     */
    bool SameNetwork(uint32_t interfaceAddressPrefixLen, qcc::IPAddress addressA, qcc::IPAddress addressB);

    /**
     * @internal
     * @brief Do something with a received protocol message.
     */
    void HandleProtocolMessage(uint8_t const* const buffer, uint32_t nbytes, const qcc::IPEndpoint& endpoint, const uint16_t recv_port, int32_t interfaceIndex, const qcc::IPAddress& localAddress);

    /**
     * @internal
     * @brief Do something with a received protocol question.
     */
    void HandleProtocolQuestion(WhoHas whoHas, const qcc::IPEndpoint& endpoint, int32_t interfaceIndex, const qcc::IPAddress& localAddress);

    /**
     * @internal
     * @brief Do something with a received protocol answer.
     */
    void HandleProtocolAnswer(IsAt isAt, uint32_t timer, const qcc::IPEndpoint& address, int32_t interfaceIndex);

    /**
     * @internal
     * @brief Do something with a received MDNS protocol query.
     */
    void HandleProtocolQuery(MDNSPacket packet, qcc::IPEndpoint endpoint, uint16_t recvPort);

    /**
     * @internal
     * @brief Do something with a received MDNS protocol response.
     */
    void HandleProtocolResponse(MDNSPacket mdnsPacket, qcc::IPEndpoint endpoint, uint16_t recvPort, int32_t interfaceIndex);

    /**
     * @internal
     * @brief Update the MDNSPacketTracker which is useful for keep track of burst and
     *        not responding to each packet of a burst
     */
    bool UpdateMDNSPacketTracker(qcc::String guid, qcc::IPEndpoint endpoint, uint16_t burstId);

    /**
     * One possible callback for each of the corresponding transport masks in a
     * sixteen-bit word.
     */
    Callback<void, const qcc::String&, const qcc::String&, std::vector<qcc::String>&, uint32_t>* m_callback[N_TRANSPORTS];

    Callback<void, const std::map<qcc::String, qcc::IPAddress>&>* m_networkEventCallback[N_TRANSPORTS];

    /**
     * @internal @brief A vector of list of all of the names that the various
     * transports have actively advertised.
     */
    std::set<qcc::String> m_advertised[N_TRANSPORTS];

    /**
     * @internal @brief A vector of set of all of the v2 match strings that the various
     * transports have active queries for.
     */
    std::set<qcc::String> m_v2_queries[N_TRANSPORTS];

    /**
     * @internal @brief A vector of set of all of the v1 names that the various
     * transports have active queries for.
     */
    std::set<qcc::String> m_v0_v1_queries[N_TRANSPORTS];

    /**
     * @internal @brief A vector of list of all of the names that the various
     * transports have quietly advertised.
     */
    std::set<qcc::String> m_advertised_quietly[N_TRANSPORTS];

    /**
     * @internal
     * @brief The daemon GUID string of the daemon assoicated with this instance
     * of the name service.
     */
    qcc::String m_guid;

    /**
     * @internal @brief The IPv4 address of the transports on this daemon that
     * are listening for reliable (TCP) inbound connections.  Not defined if the
     * corresponding port is zero.
     */
    qcc::String m_reliableIPv4Address[N_TRANSPORTS];

    /**
     * @internal
     * @brief Whether the ports on this daemon that are listening for reliable (TCP)
     * inbound connections over IPv4.
     */
    bool m_enabledReliableIPv4[N_TRANSPORTS];

    /**
     * @internal
     * @brief A map of network interfaces to ports on this daemon that are listening
     * for reliable (TCP) inbound connections over IPv4.
     */
    std::map<qcc::String, uint16_t> m_reliableIPv4PortMap[N_TRANSPORTS];

    std::map<qcc::String, uint16_t> m_priorReliableIPv4PortMap[N_TRANSPORTS];

    /**
     * @internal
     * @brief The IPv4 address of the transports on this daemon that are listening for
     * unreliable (UDP) inbound connections.  Not defined if the corresponding port is
     * zero.
     */
    qcc::String m_unreliableIPv4Address[N_TRANSPORTS];

    /**
     * @internal
     * @brief Whether the ports on this daemon that are listening for unreliable (UDP)
     * inbound connections over IPv4.
     */
    bool m_enabledUnreliableIPv4[N_TRANSPORTS];

    /**
     * @internal
     * @brief A map of network interfaces to ports on this daemon that are listening
     * for unreliable (UDP) inbound connections over IPv4.
     */
    std::map<qcc::String, uint16_t> m_unreliableIPv4PortMap[N_TRANSPORTS];

    std::map<qcc::String, uint16_t> m_priorUnreliableIPv4PortMap[N_TRANSPORTS];

    /**
     * @internal
     * @brief The IPv6 address of the transports on this daemon that are listening for
     * reliable (TCP) inbound connections.  Not defined if the corresponding port is
     * zero.
     */
    qcc::String m_reliableIPv6Address[N_TRANSPORTS];

    /**
     * @internal
     * @brief Whether the ports on this daemon that are listening for reliable (TCP)
     * inbound connections over IPv6.
     */
    bool m_enabledReliableIPv6[N_TRANSPORTS];

    /**
     * @internal
     * @brief The port on this daemon that are listening for reliable (TCP)
     * inbound connections over IPv6.
     */
    uint16_t m_reliableIPv6Port[N_TRANSPORTS];

    /**
     * @internal
     * @brief The IPv6 address of the transports on this daemon that are listening for
     * unreliable (UDP) inbound connections.  Not defined if the corresponding port is
     * zero.
     */
    qcc::String m_unreliableIPv6Address[N_TRANSPORTS];

    /**
     * @internal
     * @brief Whether the ports on this daemon that are listening for reliable (UDP)
     * inbound connections over IPv6.
     */
    bool m_enabledUnreliableIPv6[N_TRANSPORTS];

    /**
     * @internal
     * @brief The port on this daemon that is listening for unreliable (UDP)
     * inbound connections over IPv6.
     */
    uint16_t m_unreliableIPv6Port[N_TRANSPORTS];

    /**
     * @internal
     * @brief The time remaining before a set of advertisements must be
     * retransmitted over the multicast link.
     */
    uint32_t m_timer;

    /**
     * @internal
     * @brief Check to see if the periodic timer needs to be scheduled.
     */
    bool IsPeriodicMaintenanceTimerNeeded(void) const;

    /**
     * @internal
     * @brief Perform periodic protocol maintenance.  Called once per second
     * from the main listener loop.
     */
    void DoPeriodicMaintenance(void);

    /**
     * @internal
     * @brief Retransmit exported advertisements.
     */
    void Retransmit(uint32_t index, bool exiting, bool quietly, const qcc::IPEndpoint& destination, uint8_t type, TransportMask transportMask, std::vector<qcc::String>& wkns, const int32_t interfaceIndex = -1, const qcc::AddressFamily family = qcc::QCC_AF_UNSPEC, const qcc::IPAddress& localAddress =  qcc::IPAddress("0.0.0.0"));

    void GetResponsePackets(std::list<Packet>& packets, bool quietly = false, const qcc::IPEndpoint destination = qcc::IPEndpoint("0.0.0.0", 0), uint8_t type = TRANSMIT_V2, TransportMask transportMask = (TRANSPORT_TCP | TRANSPORT_UDP), const int32_t interfaceIndex = -1, const qcc::AddressFamily family = qcc::QCC_AF_UNSPEC);

    void GetQueryPackets(std::list<Packet>& packets, const uint8_t type = TRANSMIT_V0_V1 | TRANSMIT_V2, const int32_t interfaceIndex = -1, const qcc::AddressFamily family = qcc::QCC_AF_UNSPEC);
    /**
     * @internal
     * @brief Retry locate requests.
     */
    void Retry(void);

    uint32_t m_tDuration;
    uint32_t m_tRetransmit;
    uint32_t m_tQuestion;
    uint32_t m_modulus;
    uint32_t m_retries;

    /**
     * @internal
     * @brief Listen to our own advertisements if true.
     */
    bool m_loopback;

    /**
     * @internal
     * @brief Send name service packets via IPv4 subnet directed broadcast if
     * true.
     */
    bool m_broadcast;

    /**
     * @internal
     * @brief Advertise and listen over IPv4 if true.
     */
    bool m_enableIPv4;

    /**
     * @internal
     * @brief Advertise and listen over IPv6 if true.
     */
    bool m_enableIPv6;

    /**
     * @internal
     * @brief True if v0 and v1 versions of the protocol are enabled.
     */
    bool m_enableV1;

    /**
     * @internal
     * @brief Advertise IPv4 address assigned to this interface when multicasting
     * over IPv6 sockets in m_overrideIpv6 mode.  Used  to compensate for broken
     * Android phones that don't support IPv4 multicast.
     */
    qcc::String m_overrideInterface;

    /**
     * @internal @brief Set to true if a given transpot has indicated that it
     * wants to use all available interfaces whenever they may be up if true.
     * Think INADDR_ANY or in6addr_any.  this is typically what most transports
     * want to do.
     */
    bool m_any[N_TRANSPORTS];

    /**
     * @internal
     * @brief Event used to wake up the main name service thread and tell it
     * that a message has been queued on the outbound message list.
     */
    qcc::Event m_wakeEvent;

    /**
     * @internal
     * @brief Set to true to force a lazy update cycle if the open interfaces
     * change.
     */
    bool m_forceLazyUpdate;

    /**
     * @internal
     * @brief Set to true to force a refresh of advertisements/queries if one or
     * more new interfaces are detected.
     */
    bool m_refreshAdvertisements;

    /**
     * @internal
     * @brief A list of name service messages queued for transmission out on
     * the multicast group.
     */
    std::list<Packet> m_outbound;

#if defined(QCC_OS_GROUP_WINDOWS)
    /**
     * @internal @brief A socket to hold to keep winsock initialized
     * as long as the name service is alive.
     */
    qcc::SocketFd m_refSockFd;
#endif

    /**
     * @internal
     * @brief Tear down all live interfaces and remove them from the
     * corresponding list.
     */
    void ClearLiveInterfaces(void);

    /**
     * @internal
     * @brief Make sure that we have socket open to talk and listen to as many
     * of our desired interfaces as possible.
     */
    void LazyUpdateInterfaces(const qcc::NetworkEventSet& networkEvents = qcc::NetworkEventSet());

    /**
     * @internal
     * @brief Count the number of bits that are sent in the provided integer.
     * Methods to count the number of set bits in a data word are used to in the
     * process of calculating the Hamming Distance between two binary strings.
     * We use this "population count" function to count the numbr of set bits in
     * a TransportMask in order to enforce the one-to-one correspondence between
     * a transport and its one and only mask bit in our API.
     */
    uint32_t CountOnes(uint32_t data);

    /**
     * @internal
     * @brief Return the bit position corresponding to the guaranteed single bit
     * set in the provided data word.
     *
     * Used to convert a transport mask to an index into a table corresponding
     * to the transport mask.
     */
    uint32_t IndexFromBit(uint32_t data);

    /**
     * @internal
     * @brief Return the TransportMask implied by the index in the data word.
     *
     * Used to convert an index into a table corresponding to the transport mask
     * back into the original transport mask.
     */
    TransportMask MaskFromIndex(uint32_t data);

    /**
     * @internal
     * @brief If true, allow the name service to communicate with the outside
     * world.  If false, ensure that no packets are sent and no sockets are
     * listening for connections.  For Android Compatibility Test Suite (CTS)
     * conformance.
     */
    bool m_enabled;

    /**
     * @internal
     * @brief If true, invoke the transport network event callbacks.
     */
    bool m_processTransport[N_TRANSPORTS];

    /**
     * @internal
     * @brief If set to true, request the name service run thread to enable
     * communication with the outside world.
     */
    bool m_doEnable;

    /**
     * @internal
     * @brief If set to true, request the name service run thread to disable
     * communication with the outside world.
     */
    bool m_doDisable;

    /**
     * The BurstResponceHeader struct holds a copy of the header that will be
     * sent using the burst response.  The struct also tracks how many times the
     * header had been added to the outbound queue.
     */
    typedef struct _BurstResponseHeader {
        _BurstResponseHeader(Packet packet) : packet(packet), scheduleCount(0) { }
        ~_BurstResponseHeader() { }
        Packet packet;
        uint32_t scheduleCount;
        qcc::Timespec nextScheduleTime;
    }BurstResponseHeader;

    qcc::SocketFd m_ipv4QuietSockFd;
    qcc::SocketFd m_ipv6QuietSockFd;

    qcc::SocketFd m_ipv4UnicastSockFd;
    qcc::Event* m_unicastEvent;

    std::list<BurstResponseHeader> m_burstQueue;
    /**
     * Hash functor
     */
    struct Hash {
        inline size_t operator()(const qcc::String& s) const {
            return qcc::hash_string(s.c_str());
        }
    };

    struct Equal {
        inline bool operator()(const qcc::String& s1, const qcc::String& s2) const {
            return s1 == s2;
        }
    };

    /**
     * Hash functor for PacketTracker Key
     */
    struct HashPacketTracker {
        inline size_t operator()(const std::pair<qcc::String, qcc::IPEndpoint>& s) const {
            qcc::String str = s.first + s.second.ToString();
            return qcc::hash_string(str.c_str());
        }
    };

    struct EqualPacketTracker {
        inline bool operator()(const std::pair<qcc::String, qcc::IPEndpoint>& s1, const std::pair<qcc::String, qcc::IPEndpoint>& s2) const {
            return (s1.first == s2.first) && (s1.second == s2.second);
        }
    };
    std::unordered_map<std::pair<qcc::String, qcc::IPEndpoint>, uint16_t, HashPacketTracker, EqualPacketTracker> m_mdnsPacketTracker;

    /*
     * PeerInfo holds the information about a peer for which we know the unicast address
     * The timestamp field is used for cache refresh
     */
    struct PeerInfo {
        qcc::IPEndpoint unicastInfo;
        mutable qcc::Timespec lastQueryTimeStamp;
        mutable qcc::Timespec lastResponseTimeStamp;

        PeerInfo(const qcc::IPEndpoint& ipEndpoint) :
            unicastInfo(ipEndpoint)
        { }
        qcc::String ToString(const qcc::String& guid) const;
        bool operator<(const PeerInfo& other) const {
            return unicastInfo.ToString() < other.unicastInfo.ToString();
        }
        bool operator==(const PeerInfo& other) const {
            return unicastInfo.ToString() == other.unicastInfo.ToString();
        }
    };

    std::unordered_map<qcc::String, std::set<PeerInfo>, Hash, Equal> m_peerInfoMap;
    void PrintPeerInfoMap();

    class BurstExpiryHandler;
    BurstExpiryHandler* burstExpiryHandler;

    bool HandleSearchQuery(TransportMask transport, MDNSPacket mdnsPacket, uint16_t recvPort,
                           const qcc::String& guid, const qcc::IPEndpoint& ns4);

    bool HandleAdvertiseResponse(MDNSPacket mdnsPacket, uint16_t recvPort,
                                 const qcc::String& guid, const qcc::IPEndpoint& ns4,
                                 const qcc::IPEndpoint& r4, const qcc::IPEndpoint& r6, const qcc::IPEndpoint& u4, const qcc::IPEndpoint& u6);
    std::set<qcc::String> GetAdvertising(TransportMask transport);
    std::set<qcc::String> GetAdvertisingQuietly(TransportMask transport);
    std::list<IpNameServiceListener*> m_listeners;
    bool m_protectListeners;
    PacketScheduler m_packetScheduler;

    uint32_t m_networkChangeScheduleCount;
    bool m_doNetworkCallback[N_TRANSPORTS];
    qcc::NetworkEventSet m_networkEvents;
    qcc::Timespec m_networkChangeTimeStamp;
    bool PurgeAndUpdatePacket(MDNSPacket mdnspacket, bool updateSid);
};

} // namespace ajn

#endif // _IP_NAME_SERVICE_IMPL_H
