/**
 * @file
 * Singleton for the AllJoyn Android Wi-Fi Direct (Wi-Fi P2P) Name Service
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

#ifndef _P2P_NAME_SERVICE_H
#define _P2P_NAME_SERVICE_H

#ifndef __cplusplus
#error Only include P2PNameService.h in C++ code.
#endif

#include <vector>

#include <qcc/String.h>

#include <alljoyn/TransportMask.h>

#include <alljoyn/Status.h>
#include <Callback.h>

namespace ajn {

class P2PNameServiceImpl;
class BusAttachment;

/**
 * @brief API to provide an implementation dependent P2P (Layer 2) Name Service
 * singleton for AllJoyn.
 *
 * This relatively simple implementation allows a daemon to hook into the
 * Android platform-dependent Wi-Fi Direct pre-association discovery part of its
 * framework.  Since the Android framework is written in Java and the daemon is
 * written in C++, there is no way to directly communicate.  Because of this, we
 * rely on AllJoyn itself to make RPC calls to an AllJoyn service running in a
 * process that has the Java framework available.  Because of packaging
 * constraints, this service is written in C++ under JNI, but it will be running
 * in a process forked by the Android application framework and will therefore
 * have the required Java framework pieces available through introspection.  We
 * make method calls to the AllJoyn service to make advertisement and discovery
 * requests (through an intermediate class dedicated to making that easier) and
 * we receive notifications from the Java framework through AllJoyn signals.
 *
 * The P2PNameService itself is implemented as a Meyers singleton, so a static
 * method is requred to get a reference to the sinle instance of the singleton.
 * The underlying object will be constructed the first time this method is
 * called.
 *
 * We currently expect that there will only be one transport (in the AllJoyn
 * sense) using the Android P2P name service and that is the WFD (Wi-Fi Direct)
 * transport which only runs on Android-based devices.
 *
 * We use a Meyers Singleton, and therefore we defer construction of the
 * underlying object to the time of first use, which is going to be when the
 * WFD transport is created, well after main() has started.  We want to have all
 * of the tear-down of the threads performed before main() ends, so we need to
 * have knowledge of when the singleton is no longer required.  We reference
 * count instances of transports that register with the P2PNameService to
 * accomplish this.
 *
 * Whenever a transport comes up and wants to interact with the P2PNameService it
 * calls our static Instance() method to get a reference to the underlying name
 * service object.  This accomplishes the construction on first use idiom.  This
 * is a very lightweight operation that does almost nothing.  The first thing
 * that a transport must do is to Acquire() the instance of the name service,
 * which is going to bump a reference count and do the hard work of starting the
 * P2PNameService.  The last thing a transport must do is to Release() the
 * instance of the name service.  This will do the work of stopping and joining
 * the name service threads when the last reference is released.  Since this
 * operation may block waiting for the name service thread to exit, this should
 * only be done in the transport's Join() method.
 */
class P2PNameService {
  public:

    /**
     * @brief Return a reference to the P2PNameService singleton.
     */
    static P2PNameService& Instance()
    {
        static P2PNameService p2pNameService;
        return p2pNameService;
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
     * longer be using the P2P name service.
     *
     * The last thing a transport must do is to Release() the instance of the
     * name service.  This will do the work of stopping and joining the name
     * service threads when the last reference is released.  Since this
     * operation may block waiting for the name service thread to exit, this
     * should only be done in the transport's Join() method.
     */
    void Release();

    /**
     * @brief Determine if the P2PNameService singleton has been started.
     *
     * Basically, this determines if the reference count is strictly positive.
     *
     * @return True if the singleton has been started, false otherwise.
     */
    bool Started();

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
     * @param transportMask A bitmask containing the transport handling events relating
     *     to discovery.  Used for consistency and expectation checking.
     */
    QStatus Enable(TransportMask transportMask);

    /**
     * @brief Notify the name service that that it should stop advertising over
     *     pre-association service discovery.
     *
     * Enable() communicates the fact that there will no longer be some
     * transport that will be able to receive and deal with connection attempts
     * as a result of the advertisements which may be generated as a result of
     * the call.
     *
     * @param transportMask A bitmask containing the transport handling events relating
     *     to discovery.  Used for consistency and expectation checking.
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
     * @brief Given a GUID that we have discovered and reported back, return
     * the device that was associated with that GUID when we received the
     * advdrtisement.
     *
     * @param guid The GUID reported back to the caller when we got the
     *     advertisement.
     * @param device The device we heard the advertisement from.
     */
    QStatus GetDeviceForGuid(const qcc::String& guid, qcc::String& device);

  private:
    /**
     * This is a singleton so the constructor is marked private to prevent
     * construction of a P2PNameService instance in any other sneaky way than
     * the Meyers singleton mechanism.
     */
    P2PNameService();

    /**
     * This is a singleton so the destructor is marked private to prevent
     * destruction of an P2PNameService instance in any other sneaky way than
     * the Meyers singleton mechanism.
     */
    virtual ~P2PNameService();

    /**
     * This is a singleton so the copy constructor is marked private to prevent
     * construction of an P2PNameService instance in any other sneaky way than
     * the Meyers singleton mechanism.
     */
    P2PNameService(const P2PNameService& other);

    /**
     * This is a singleton so the assignment constructor is marked private to
     * prevent construction of an P2PNameService instance in any other sneaky
     * way than the Meyers singleton mechanism.
     */
    P2PNameService& operator =(const P2PNameService& other);

    /**
     * @brief Start the P2PNameService singleton.
     *
     * Since the P2PNameService is conceivably shared among transports, the
     * responsibility for starting, stopping and joining the name service should
     * not reside with any single transport.  We provide a reference counting
     * mechanism to deal with this and so the actual Start() method is private
     * and called from the public Acquire() method.
     *
     * @return ER_OK if the start operation completed successfully, or an error code
     *     if not.
     */
    QStatus Start();

    /**
     * @brief Stop the P2PNameService singleton.
     *
     * Since the P2PNameService is conceivably shared among transports, the
     * responsibility for starting, stopping and joining the name service should
     * not reside with any single transport.  We provide a reference counting
     * mechanism to deal with this and so the actual Stop() method is private
     * and called from the public Release() method
     *
     * @return ER_OK if the stop operation completed successfully, or an error code
     *     if not.
     */
    QStatus Stop();

    /**
     * @brief Join the P2PNameService singleton.
     *
     * Since the P2PNameService is conceivably shared among transports, the
     * responsibility for starting, stopping and joining the name service should
     * not reside with any single transport.  We provide a reference counting
     * mechanism to deal with this and so the actual Join() method is private
     * and called from the public Release().
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

    bool m_constructed;           /**< State variable indicating the singleton has been constructed */
    bool m_destroyed;             /**< State variable indicating the singleton has been destroyed */
    int32_t m_refCount;           /**< The number of transports that have registered as users of the singleton */
    P2PNameServiceImpl* m_pimpl;  /**< A pointer to the private implementation of the name service */
};

} // namespace ajn

#endif // _P2P_NAME_SERVICE_H
