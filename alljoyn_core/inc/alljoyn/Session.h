#ifndef _ALLJOYN_SESSION_H
#define _ALLJOYN_SESSION_H
/**
 * @file
 * AllJoyn session related data types.
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

#include <qcc/platform.h>
#include <qcc/String.h>
#include <alljoyn/Status.h>
#include <alljoyn/TransportMask.h>

namespace ajn {

/**
 * SessionPort identifies a per-BusAttachment receiver for incoming JoinSession requests.
 * SessionPort values are bound to a BusAttachment when the attachment calls
 * BindSessionPort.
 *
 * NOTE: Valid SessionPort values range from 1 to 0xFFFF.
 */
typedef uint16_t SessionPort;

/** Invalid SessionPort value used to indicate that BindSessionPort should choose any available port */
const SessionPort SESSION_PORT_ANY = 0;

/** SessionId uniquely identifies an AllJoyn session instance */
typedef uint32_t SessionId;

/** Invalid session id value used to indicate that a signal should be emitted on all hosted sessions */
const SessionId SESSION_ID_ALL_HOSTED = ((SessionId) - 1);

/* Forward declaration */
class MsgArg;

/**
 * SessionOpts contains a set of parameters that define a Session's characteristics.
 */
class SessionOpts {

  public:
    /** Traffic type */
    typedef enum {
        TRAFFIC_MESSAGES          = 0x01,   /**< Session carries message traffic */
        TRAFFIC_RAW_UNRELIABLE    = 0x02,   /**< Session carries an unreliable (lossy) byte stream */
        TRAFFIC_RAW_RELIABLE      = 0x04,   /**< Session carries a reliable byte stream */
    } TrafficType;
    TrafficType traffic; /**< holds the Traffic type for this SessionOpt*/

    /**
     * Multi-point session capable.
     * A session is multi-point if it can be joined multiple times to form a single
     * session with multi (greater than 2) endpoints. When false, each join attempt
     * creates a new point-to-point session.
     */
    bool isMultipoint;

    /**@name Proximity */
    // {@
    typedef uint8_t Proximity;
    static const Proximity PROXIMITY_ANY      = 0xFF;
    static const Proximity PROXIMITY_PHYSICAL = 0x01;
    static const Proximity PROXIMITY_NETWORK  = 0x02;
    Proximity proximity;
    // @}

    /** Allowed Transports  */
    TransportMask transports;

    /**
     * Construct a SessionOpts with specific parameters.
     *
     * @param traffic           Type of traffic.
     * @param isMultipoint      true iff session supports multipoint (greater than two endpoints).
     * @param proximity         Proximity constraint bitmask.
     * @param transports        Allowed transport types bitmask.
     * @param exchangeAllNames  true if all names need to be exchanged.
     *                          false if only session related names need to be exchanged.
     *                          (Default, used for optimal performance)
     *
     */
    SessionOpts(SessionOpts::TrafficType traffic, bool isMultipoint, SessionOpts::Proximity proximity, TransportMask transports, bool exchangeAllNames = false) :
        traffic(traffic),
        isMultipoint(isMultipoint),
        proximity(proximity),
        transports(transports),
        nameTransfer(exchangeAllNames ? ALL_NAMES : (isMultipoint ? MP_NAMES : P2P_NAMES))
    { }

    /**
     * Construct a default SessionOpts
     */
    SessionOpts() : traffic(TRAFFIC_MESSAGES), isMultipoint(false), proximity(PROXIMITY_ANY), transports(TRANSPORT_ANY), nameTransfer(P2P_NAMES) { }

    /**
     * Destructor
     */
    virtual ~SessionOpts() { }

    /**
     * Determine whether this SessionOpts is compatible with the SessionOpts offered by other
     *
     * Compatibility means that the SessionOpts share at least one of each
     *  - transport type
     *  - traffic type
     *  - proximity type
     *
     * Note that multipoint support is not a condition of compatibility
     *
     * @param other  Options to be compared against this one.
     * @return true iff this SessionOpts can use the option set offered by other.
     */
    bool IsCompatible(const SessionOpts& other) const;

    /**
     * Create a human readable representation of a SessionOpts
     *
     * @return string with human readable SessionOpts
     */
    qcc::String ToString() const;

    /**
     * Compare SessionOpts
     *
     * @param other the SessionOpts being compared against
     * @return true if all of the SessionOpts parameters are the same
     *
     */
    bool operator==(const SessionOpts& other) const
    {
        return (traffic == other.traffic) && (isMultipoint == other.isMultipoint) && (proximity == other.proximity) && (transports == other.transports);
    }

    /**
     * Rather arbitrary less-than operator to allow containers holding SessionOpts
     * to be sorted.
     * Traffic takes precedence when sorting SessionOpts.
     *
     * #TRAFFIC_MESSAGES \< #TRAFFIC_RAW_UNRELIABLE \< #TRAFFIC_RAW_RELIABLE
     *
     * If traffic is equal then Proximity takes next level of precedence.
     *
     * PROXIMITY_PHYSICAL \< PROXIMITY_NETWORK \< PROXIMITY_ANY
     *
     * last transports.
     *
     * #TRANSPORT_LOCAL \< #TRANSPORT_WLAN \< #TRANSPORT_WWAN \< #TRANSPORT_ANY
     *
     *
     * @param other the SessionOpts being compared against
     * @return true if this SessionOpts is designated as less than the SessionOpts
     *         being compared against.
     */
    bool operator<(const SessionOpts& other) const
    {
        if ((traffic < other.traffic) ||
            ((traffic == other.traffic) && !isMultipoint && other.isMultipoint) ||
            ((traffic == other.traffic) && (isMultipoint == other.isMultipoint) && (proximity < other.proximity)) ||
            ((traffic == other.traffic) && (isMultipoint == other.isMultipoint) && (proximity == other.proximity) && (transports < other.transports))) {
            return true;
        }
        return false;
    }

    /**
     * Set up this session to exchange all names not just session related names.
     *
     */
    void SetAllNames();

    /**
     * Set up this session to exchange only session related names not all names.
     *
     */
    void SetSessionNames();

    /**
     * Is this session set up to exchange all names
     * @return true if session is set up to exchange all names.
     */
    bool IsAllNames() const;

    /**
     * Is this session set up to exchange only session related names
     * @return true if session is set up to exchange only session related names.
     */
    bool IsSessionNames() const;


    /** Internal
     *
     * NameTransferType when a session is established list of names are exchanged.
     * The NameTransferType specifies what information is exchanged.
     */
    typedef enum {
        ALL_NAMES = 0x00,       /**< ExchangeNames and NameChanged to be forwarded to this session,
                                     AttachSessionWithNames to be converted into an ExchangeNames and
                                     sent over this session,
                                     all NameChanged to be sent, all names to be sent as a part of
                                     initial AttachSessionWithNames */
        SLS_NAMES = 0x01,      /**< No ExchangeNames and NameChanged forwarding,
                                     no NameChanged to be sent, only router names and
                                     sessionless emitter names(if host routing node)
                                     to be sent as a part of initial AttachSessionWithNames */
        MP_NAMES = 0x02,        /**< ExchangeNames and NameChanged to be forwarded only over endpoints that
                                     match the session id of the endpoint that it was received on,
                                     NameChanged to be sent to routing nodes if a session to this leaf existed,
                                     only routing node and joiner or host and existing session member names
                                     to be sent as a part of initial AttachSessionWithNames */
        P2P_NAMES = 0x03,       /**< No ExchangeNames and NameChanged forwarding,
                                     NameChanged to be sent only if a session to this leaf existed,
                                     only routing node and joiner/host names to be sent as a part of initial
                                     AttachSessionWithNames */
    } NameTransferType;

  protected:
    /**
     * Construct a SessionOpts with specific parameters.
     *
     * @param traffic       Type of traffic.
     * @param isMultipoint  true iff session supports multipoint (greater than two endpoints).
     * @param proximity     Proximity constraint bitmask.
     * @param transports    Allowed transport types bitmask.
     * @param nameType      The NameTransferType specifies what information is exchanged
     *                      values #ALL_NAMES, #SLS_NAMES
     */
    SessionOpts(SessionOpts::TrafficType traffic, bool isMultipoint, SessionOpts::Proximity proximity, TransportMask transports, NameTransferType nameType) :
        traffic(traffic),
        isMultipoint(isMultipoint),
        proximity(proximity),
        transports(transports),
        nameTransfer(nameType)
    { }

  private:
    friend class SessionlessObj;
    friend class AllJoynObj;
    friend class TCPTransport;
    friend class UDPTransport;
    friend class Features;
    friend void SetSessionOpts(const SessionOpts& opts, MsgArg& msgArg);
    friend QStatus GetSessionOpts(const MsgArg& msgArg, SessionOpts& opts);

    /**
     * NameTransferType used for specifying the name propagation during initial
     * ExchangeNames
     */
    NameTransferType nameTransfer;

};


}

#endif
