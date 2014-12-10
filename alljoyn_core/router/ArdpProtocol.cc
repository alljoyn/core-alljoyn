/**
 * @file ArdpProtocol is an implementation of the Reliable Datagram Protocol
 * (RDP) adapted to AllJoyn.
 */

/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
#include <qcc/IPAddress.h>
#include <qcc/Socket.h>
#include <qcc/time.h>
#include <qcc/Util.h>

#include <alljoyn/Message.h>

#include "ScatterGatherList.h"
#include "ArdpProtocol.h"

#define QCC_MODULE "ARDP_PROTOCOL"

namespace ajn {

#define UDP_MTU 1472

#define ARDP_VERSION_BITS 0xC0   /* Bits 6-7 of FLAGS byte in ARDP segment header*/
#define ARDP_DISCONNECT_RETRY 1  /* Not configurable */
#define ARDP_DISCONNECT_RETRY_TIMEOUT 1000  /* Not configurable */

/**< Reserved TTL value to indicate that data associated with the message has expired */
#define ARDP_TTL_EXPIRED    0xffffffff
/**<  Maximum allowed TTL value */
#define ARDP_TTL_MAX        (ARDP_TTL_EXPIRED - 1)
/**< Reserved TTL value to indicate that data  associated with the message never expires */
#define ARDP_TTL_INFINITE   0

/**< Flag indicating that the buffer is occupied */
#define ARDP_BUFFER_IN_USE 0x01
/**< Flag indicating that the buffer is delivered to the upper layer */
#define ARDP_BUFFER_DELIVERED 0x02

/* Minimum Roundtrip Time */
#define ARDP_MIN_RTO 100

/* Maximum Roundtrip Time */
#define ARDP_MAX_RTO 64000

/* Minimum Delayed ACK Timeout */
#define ARDP_MIN_DELAYED_ACK_TIMEOUT 10

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ABS(a) ((a) >= 0 ? (a) : -(a))

#define UDP_HEADER_SIZE 8

/* Marshal/Unmarshal ARDP header offsets */
#define FLAGS_OFFSET   0
#define HLEN_OFFSET    1
#define SRC_OFFSET     2
#define DST_OFFSET     4
#define DLEN_OFFSET    6
#define SEQ_OFFSET     8
#define ACK_OFFSET    12
#define TTL_OFFSET    16
#define LCS_OFFSET    20
#define ACKNXT_OFFSET 24
#define SOM_OFFSET    28
#define FCNT_OFFSET   32
#define RSRV_OFFSET   34

/* Additional Marshal/Unmarshal ARDP SYN header offsets */
#define SEGMAX_OFFSET  16
#define SEGBMAX_OFFSET 18
#define DACKT_OFFSET   20
#define OPTIONS_OFFSET 24

#define ARDP_SYN_HEADER_SIZE 28

/* A simple circularly linked list node suitable for use in thin core library implementations */
typedef struct LISTNODE {
    struct LISTNODE* fwd;
    struct LISTNODE* bwd;
    uint8_t*         buf;
    uint32_t len;
} ListNode;

/* Callback for timeout handler */
typedef void (*ArdpTimeoutHandler)(ArdpHandle* handle, ArdpConnRecord* conn, void* context);

/* Structure encapsulating timer to to handle timeouts */
typedef struct ARDP_TIMER {
    ListNode list;
    ArdpConnRecord* conn;
    ArdpTimeoutHandler handler;
    void* context;
    uint32_t delta;
    uint32_t when;
    uint32_t retry;
} ArdpTimer;

/* Structure encapsulating the information about segments on SEND side */
typedef struct ARDP_SEND_BUF {
    uint8_t* data;
    uint32_t datalen;
    uint8_t* hdr;
    uint32_t ttl;
    uint32_t tStart;
    ARDP_SEND_BUF* next;
    ArdpTimer timer;
    uint16_t fastRT;
    uint16_t retransmits;
    bool inUse;
} ArdpSndBuf;

/**
 * Structure encapsulating the send-related quantities. The stuff we manage on
 * the local side of the connection and which we may send to THEM.
 */
typedef struct {
    uint32_t NXT;         /* The sequence number of the next segment that is to be sent */
    uint32_t UNA;         /* The sequence number of the oldest unacknowledged segment */
    uint32_t SEGMAX;      /* The maximum number of unacknowledged segments that can be sent */
    uint32_t SEGBMAX;     /* The largest possible segment that THEY can receive (our send buffer, specified by the other side during connection) */
    uint32_t ISS;         /* The initial send sequence number. The number that was sent in the SYN segment */
    uint32_t LCS;         /* Sequence number of last consumed segment (we get this form them) */
    uint32_t DACKT;       /* Delayed ACK timeout from the other side */
    ArdpSndBuf* buf;      /* Dynamically allocated array of unacked sent buffers */
    uint16_t maxDlen;     /* Maximum data payload size that can be sent without partitioning */
    uint16_t pending;     /* Number of unacknowledged sent buffers */
} ArdpSnd;

/**
 * Structure for tracking of received out-of-order segments.
 * Contains EACK bitmask to be sent to the remote side.
 */
typedef struct {
    uint32_t mask[ARDP_MAX_WINDOW_SIZE >> 5];     /* mask in host order */
    uint32_t htnMask[ARDP_MAX_WINDOW_SIZE >> 5];  /* mask in network order */
    uint16_t sz;
    uint16_t fixedSz;
} ArdpEack;

/**
 * Structure encapsulating the receive-related quantities. The stuff managed on
 * the remote/foreign side, copies of which we may get from THEM.
 */
typedef struct {
    uint32_t CUR;        /* The sequence number of the last segment received correctly and in sequence */
    uint32_t SEGMAX;     /* The maximum number of segments that can be buffered for this connection */
    uint32_t SEGBMAX;    /* The largest possible segment that WE can receive (our receive buffer, specified by our user on an open) */
    uint32_t IRS;        /* The initial receive sequence number.  The sequence number of the SYN that established the connection */
    uint32_t LCS;        /* The sequence number of last consumed segment */
    ArdpRcvBuf* buf;     /* Array holding received buffers not consumed by the app */
    ArdpEack eack;       /* Tracking of received out-of-order segments */
} ArdpRcv;

/**
 * Information encapsulating the various interesting tidbits we get from the
 * other side when we receive a datagram.  Some of the names are chosen so that they
 * are similar to the quantities found in RFC-908 when used.
 */
typedef struct {
    uint32_t SEQ;     /* The sequence number in the segment currently being processed. */
    uint32_t ACK;     /* The acknowledgement number in the segment currently being processed. */
    uint32_t LCS;     /* The last "in-sequence" consumed segment */
    uint32_t ACKNXT;  /* The first valid SND segment, TTL accounting */
    uint32_t SOM;     /* Start sequence number for fragmented message */
    uint32_t TTL;     /* Time-to-live */
    uint16_t FCNT;    /* Number of fragments comprising a message */
    uint16_t DLEN;    /* The length of the data that came in with the current segment. */
    uint16_t WINDOW;  /* Receive window */
    uint8_t FLG;      /* The flags in the header of the segment currently being processed. */
    uint8_t HLEN;     /* The header length */
} ArdpSeg;

/**
 * The states through which our main state machine transitions.
 */
enum ArdpState {
    CLOSED = 1,    /* No connection exists and no connection record available */
    CLOSE_WAIT,    /* Entered if local close or remote RST received.  Delay for connection activity to subside. */
    LISTEN,        /* Entered upon a passive open request.  Connection record is allocated and ARDP waits for a connection from remote */
    SYN_SENT,      /* Entered after processing an active open request.  SYN is sent and ARDP waits here for ACK of open request. */
    SYN_RCVD,      /* Reached from either LISTEN or SYN_SENT.  Generate ISN and ACK. */
    OPEN           /* Successful echange of state information happened,.  Data may be sent and received. */
};

/**
 * The format of a SYN segment on the wire.
 */
#pragma pack(push, 1)
typedef struct {
    uint8_t flags;      /* See Control flag definitions above */
    uint8_t hlen;       /* Length of the header in units of two octets (number of uint16_t) */
    uint16_t src;       /* Used to distinguish between multiple connections on the local side. */
    uint16_t dst;       /* Used to distinguish between multiple connections on the foreign side. */
    uint16_t dlen;      /* The length of the data in the current segment.  Does not include the header size. */
    uint32_t seq;       /* The sequence number of the current segment. */
    uint32_t ack;       /* The number of the segment that the sender of this segment last received correctly and in sequence. */
    uint16_t segmax;    /* The maximum number of outstanding segments the other side can send without acknowledgement. */
    uint16_t segbmax;   /* The maximum segment size we are willing to receive.  (the RBUF.MAX specified by the user calling open). */
    uint32_t dackt;     /* Receiver's delayed ACK timeout. Used in TTL estimate prior to sending a message. */
    uint16_t options;   /* Options for the connection.  Always Sequenced Delivery Mode (SDM). */
    uint16_t reserve;   /* Reserved for future use. */
} ArdpSynHeader;
#pragma pack(pop)

typedef struct {
    uint8_t* buf;       /* Place to hold connection handshake data (SASL, HELLO, etc) */
    uint32_t len;       /* Length of connection handshake data */
} ArdpSynData;

/**
 * A connection record describing each "connection."  This acts as a containter
 * to hold all of the interesting information about a reliable link between
 * hosts.
 */
struct ARDP_CONN_RECORD {
    ListNode list;          /* Doubly linked list node on which this connection might be */
    uint32_t id;            /* Randomly chosen connection identifier */
    ArdpState state;        /* The current sate of the connection */
    bool passive;           /* If true, this is a passive open (we've been connected to); if false, we did the connecting */
    ArdpSnd snd;            /* Send-side related state information */
    ArdpRcv rcv;            /* Receive-side related state information */
    uint16_t local;         /* ARDP local port for this connection */
    uint16_t foreign;       /* ARDP foreign port for this connection */
    qcc::SocketFd sock;     /* A convenient copy of socket we use to communicate. */
    qcc::IPAddress ipAddr;  /* A convenient copy of the IP address of the foreign side of the connection. */
    uint16_t ipPort;        /* A convenient copy of the IP port of the foreign side of the connection. */
    uint16_t window;        /* Current send window, dynamic setting */
    uint16_t minSendWindow; /* Minimum send window needed to accomodate max message */
    uint16_t remoteMskSz;   /* Size of of EACK bitmask present in received segment */
    uint32_t lastSeen;      /* Last time we received communication on this connection. */
    ArdpSynData synData;    /* Connection establishment data */
    bool rttInit;           /* Flag indicating that the first RTT was measured and SRTT calculation applies */
    uint32_t rttMean;       /* Smoothed RTT value */
    uint32_t rttMeanVar;    /* RTT variance */
    uint32_t backoff;       /* Backoff factor accounting for retransmits on connection, resets to 1 when receive "good ack" */
    uint32_t rttMeanUnit;   /* Smoothed RTT value per UDP MTU */
    ArdpTimer connectTimer; /* Connect/Disconnect timer */
    ArdpTimer probeTimer;   /* Probe (link timeout) timer */
    ArdpTimer ackTimer;     /* Delayed ACK timer */
    ArdpTimer persistTimer; /* Persist (frozen window) timer */
    uint32_t ackPending;
    void* context;          /* A client-defined context pointer */
};

struct ARDP_HANDLE {
    ArdpGlobalConfig config; /* The configurable items that affect this instance of ARDP as a whole */
    ArdpCallbacks cb;        /* The callbacks to allow the protocol to talk back to the client */
#if ARDP_TESTHOOKS
    ArdpTesthooks th;        /* Test hooks allowing test programs to examime or change data or behaviors */
#endif
#if ARDP_STATS
    ArdpStats stats;         /* Protocol statistics gathering */
#endif
    bool accepting;          /* If true the ArdpProtocol is accepting inbound connections */
    ListNode conns;          /* List of currently active connections */
    qcc::Timespec tbase;     /* Baseline time */
    ListNode dataTimers;     /* List of currently scheduled retransmit timers */
    uint32_t msnext;         /* To inform upper layer when to call into the protocol next time */
    bool trafficJam;         /* "Socket Write Block" indicator */
    void* context;           /* A client-defined context pointer */
};

/*
 * Important!!! All our numbers are within window size, the calculation below will hold.
 * If necessary, can add check that delta between the numbers does not exceed half-range.
 */
#define SEQ32_LT(a, b) ((int32_t)((uint32_t)(a) - (uint32_t)(b)) < 0)
#define SEQ32_LET(a, b) (((int32_t)((uint32_t)(a) - (uint32_t)(b)) < 0) || ((a) == (b)))

/**
 * Inside window calculation.
 * Returns true if p is in range [beg, beg+sz)
 * This function properly accounts for possible wrap-around in [beg, beg+sz) region.
 */
#define IN_RANGE(tp, beg, sz, p) (((static_cast<tp>((beg) + (sz)) > (beg)) && ((p) >= (beg)) && ((p) < static_cast<tp>((beg) + (sz)))) || \
                                  ((static_cast<tp>((beg) + (sz)) < (beg)) && !(((p) < (beg)) && (p) >= static_cast<tp>((beg) + (sz)))))



static ArdpConnRecord* FindConn(ArdpHandle* handle, uint16_t local, uint16_t foreign);
static QStatus DoSendSyn(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t* buf, uint16_t len);

/**************
 * End of definitions
 */

static void SetEmpty(ListNode* node)
{
    QCC_DbgTrace(("SetEmpty(node=%p)", node));
    node->fwd = node->bwd = node;
}

static bool IsEmpty(ListNode* node)
{
    return (node->fwd == node);
}

static void EnList(ListNode* after, ListNode* node)
{
    QCC_DbgTrace(("EnList(after=%p, node=%p)", after, node));
    node->fwd = after->fwd;
    node->bwd = after;
    node->fwd->bwd = node;
    after->fwd = node;
}

static void DeList(ListNode* node)
{
    QCC_DbgTrace(("DeList(node=%p)", node));

    if (IsEmpty(node)) {
        return;
    }
    node->bwd->fwd = node->fwd;
    node->fwd->bwd = node->bwd;
    node->fwd = node->bwd = node;
}

#ifndef NDEBUG
static void DumpBitMask(ArdpConnRecord* conn, uint32_t* msk, uint16_t sz, bool convert)
{
    QCC_DbgHLPrintf(("DumpBitMask(conn=%p, msk=%p, sz=%d, convert=%s)",
                     conn, msk, sz, convert ? "true" : "false"));

    for (uint16_t i = 0; i < sz; i++) {
        uint32_t mask32;
        if (convert) {
            mask32 = ntohl(msk[i]);
        } else {
            mask32 = msk[i];
        }
        QCC_DbgHLPrintf(("\t %d:  %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x",
                         i, (mask32 >> 31) & 1, (mask32 >> 30) & 1, (mask32 >> 29) & 1, (mask32 >> 28) & 1,
                         (mask32 >> 27) & 1, (mask32 >> 26) & 1, (mask32 >> 25) & 1, (mask32 >> 24) & 1,
                         (mask32 >> 23) & 1, (mask32 >> 22) & 1, (mask32 >> 21) & 1, (mask32 >> 20) & 1,
                         (mask32 >> 19) & 1, (mask32 >> 18) & 1, (mask32 >> 17) & 1, (mask32 >> 16) & 1,
                         (mask32 >> 15) & 1, (mask32 >> 14) & 1, (mask32 >> 13) & 1, (mask32 >> 12) & 1,
                         (mask32 >> 11) & 1, (mask32 >> 10) & 1, (mask32 >> 9) & 1, (mask32 >> 8) & 1,
                         (mask32 >> 7) & 1, (mask32 >> 6) & 1, (mask32 >> 5) & 1, (mask32 >> 4) & 1,
                         (mask32 >> 3) & 1, (mask32 >> 2) & 1, (mask32 >> 1) & 1, mask32 & 1));

    }
}

static const char* State2Text(ArdpState state)
{
    switch (state) {
    case CLOSED: return "CLOSED";

    case LISTEN: return "LISTEN";

    case SYN_SENT: return "SYN_SENT";

    case SYN_RCVD: return "SYN_RCVD";

    case OPEN: return "OPEN";

    case CLOSE_WAIT: return "CLOSE_WAIT";

    default: return "UNDEFINED";
    }
}
#endif // NDEBUG

static inline void SetState(ArdpConnRecord* conn, ArdpState state)
{
    QCC_DbgTrace(("SetState: conn=%p %s=>%s", conn, State2Text(conn->state), State2Text(state)));
    conn->state = state;
}

static uint32_t TimeNow(qcc::Timespec base)
{
    qcc::Timespec now;
    qcc::GetTimeNow(&now);
    return 1000 * (now.seconds - base.seconds) + (now.mseconds - base.mseconds);
}

static bool IsConnValid(ArdpHandle* handle, ArdpConnRecord* conn)
{
    if (conn == NULL) {
        return false;
    }

    if (IsEmpty(&handle->conns)) {
        return false;
    }

    for (ListNode* ln = &handle->conns; (ln = ln->fwd) != &handle->conns;) {
        if (conn == (ArdpConnRecord*)ln) {
            return true;
        }
    }
    return false;
}

static void moveAhead(ArdpHandle* handle, ArdpConnRecord* conn)
{
    ListNode* head = &handle->conns;
    ListNode* ln = (ListNode* ) conn;

    if (head->fwd != ln) {
        DeList(ln);
        EnList(head, ln);
    }
}

static void InitTimer(ArdpHandle* handle, ArdpConnRecord* conn, ArdpTimer* timer, ArdpTimeoutHandler handler, void*context, uint32_t timeout, uint16_t retry)
{
    QCC_DbgTrace(("InitTimer: conn=%p timer=%p handler=%p context=%p timeout=%u retry=%u",
                  conn, timer, handler, context, timeout, retry));

    timer->conn = conn;
    timer->handler = handler;
    timer->context = context;
    timer->delta = timeout;
    timer->when = TimeNow(handle->tbase) + timeout;
    timer->retry = retry;
    /* Update "call-me-back" value */
    if ((retry != 0) && (timeout < handle->msnext)) {
        moveAhead(handle, conn);
        handle->msnext = timeout;
    }
}

static void UpdateTimer(ArdpHandle* handle, ArdpConnRecord* conn, ArdpTimer* timer, uint32_t timeout, uint16_t retry)
{
    QCC_DbgTrace(("UpdateTimer: conn=%p timer=%p timeout=%u retry=%u", conn, timer, timeout, retry));
    timer->delta = timeout;
    timer->when = TimeNow(handle->tbase) + timeout;
    timer->retry = retry;
    if ((retry != 0) && (timeout < handle->msnext)) {
        moveAhead(handle, conn);
        handle->msnext = timeout;
    }
}

static uint32_t CheckConnTimers(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t next, uint32_t now)
{
    /*
     * Check connect/disconnect timer. This timer is alive only when the connection is being established or going away.
     * No other timers should be active on the connection.
     */
    if (conn->connectTimer.retry != 0) {
        if (conn->connectTimer.when <= now) {
            QCC_DbgPrintf(("CheckConnTimers: Fire connection( %p ) timer %p at %u (now=%u)",
                           conn, conn->connectTimer, conn->connectTimer.when, now));
            (conn->connectTimer.handler)(handle, conn, conn->connectTimer.context);
            if (IsConnValid(handle, conn)) {
                conn->connectTimer.when = now + conn->connectTimer.delta;
                if (conn->connectTimer.when < next && conn->connectTimer.retry != 0) {
                    /* Update "call-me-next-ms" value */
                    next = conn->connectTimer.when;
                    moveAhead(handle, conn);
                }
            }
        }
        return next;
    }

    /* If connection is not in OPEN state, return */
    if (conn->state != OPEN) {
        return next;
    }

    /* Check probe timer, it's always turned on */
    if (conn->probeTimer.when <= now) {
        QCC_DbgPrintf(("CheckConnTimers: Fire probe( %p ) timer %p at %u (now=%u)",
                       conn, conn->probeTimer, conn->probeTimer.when, now));
        (conn->probeTimer.handler)(handle, conn, &now);
        conn->probeTimer.when = now + conn->probeTimer.delta;
    }

    if (conn->probeTimer.when < next) {
        /* Update "call-me-next-ms" value */
        next = conn->probeTimer.when;
        moveAhead(handle, conn);
    }

    /* Check delayed ACK timer */
    if (conn->ackTimer.retry != 0 && conn->ackTimer.when <= now) {
        QCC_DbgPrintf(("CheckConnTimers (conn %p): Fire ACK timer %p at %u (now=%u)",
                       conn, conn->ackTimer, conn->ackTimer.when, now));
        (conn->ackTimer.handler)(handle, conn, conn->ackTimer.context);
    }

    if (conn->ackTimer.when < next && conn->ackTimer.retry != 0) {
        /* Update "call-me-next-ms" value */
        next = conn->ackTimer.when;
    }

    /* Check persist timer */
    if (conn->persistTimer.retry != 0 && conn->persistTimer.when <= now) {
        QCC_DbgPrintf(("CheckConnTimers: Fire persist( %p ) timer %p at %u (now=%u)",
                       conn, conn->persistTimer, conn->persistTimer.when, now));
        (conn->persistTimer.handler)(handle, conn, conn->persistTimer.context);
        conn->persistTimer.when = now + conn->persistTimer.delta;
    }

    if (conn->persistTimer.when < next && conn->persistTimer.retry != 0) {
        /* Update "call-me-next-ms" value */
        next = conn->persistTimer.when;
        moveAhead(handle, conn);
    }

    return next;
}

/*
 * Fire expired ones and return the next one
 */
static uint32_t CheckTimers(ArdpHandle* handle)
{
    uint32_t nextTime = ARDP_NO_TIMEOUT;
    uint32_t now = TimeNow(handle->tbase);
    ListNode* ln = &handle->conns;

    if (IsEmpty(ln)) {
        return nextTime;
    }

    for (; (ln = ln->fwd) != &handle->conns;) {
        ListNode* temp = ln->bwd;

        nextTime = CheckConnTimers(handle, (ArdpConnRecord* ) ln, nextTime, now);

        /* Check if connection record has been removed due to expiring connect/disconnect timers */
        if (IsEmpty(&handle->conns)) {
            break;
        } else if (!IsConnValid(handle, (ArdpConnRecord*) ln)) {
            ln = temp;
        }
    }

    ln = &handle->dataTimers;

    if (!handle->trafficJam && !IsEmpty(ln)) {
        for (; (ln = ln->fwd) != &handle->dataTimers;) {
            ArdpTimer* timer = (ArdpTimer*)ln;

            if ((timer->when <= now) && (timer->retry > 0)) {
                QCC_DbgPrintf(("CheckTimers: conn %p, fire retransmit timer %p at %u (now=%u)",
                               timer->conn, timer, timer->when, now));
                (timer->handler)(handle, timer->conn, timer->context);
                timer->when = now + timer->delta;
            }

            if (timer->retry == 0) {
                /* We either hit the retransmit limit or the message's TTL has expired. */
                ln = ln->bwd;
                DeList((ListNode*)timer);
                break;
            } else if (timer->when < nextTime) {
                /* Update "call-me-next-ms" value */
                nextTime = timer->when;
            }

            if (handle->trafficJam) {
                break;
            }
        }
    }

    return (nextTime != ARDP_NO_TIMEOUT) ? nextTime - now : ARDP_NO_TIMEOUT;
}

static void DelConnRecord(ArdpHandle* handle, ArdpConnRecord* conn, bool forced)
{
    QCC_DbgTrace(("DelConnRecord(handle=%p conn=%p forced=%s state=%s)",
                  handle, conn, forced ? "true" : "false", State2Text(conn->state)));

    if (!forced && conn->state != CLOSED && conn->state != CLOSE_WAIT) {
        QCC_LogError(ER_ARDP_INVALID_STATE, ("DelConnRecord(): Delete while not CLOSED or CLOSE-WAIT conn %p state %s",
                                             conn, State2Text(conn->state)));
        assert((conn->state == CLOSED || conn->state == CLOSE_WAIT)  &&
               "DelConnRecord(): Delete while not CLOSED or CLOSE-WAIT");

    }

    /* Safe to check together as these buffers are always allocated together */
    if (conn->snd.buf != NULL && conn->snd.buf[0].hdr != NULL) {
        free(conn->snd.buf[0].hdr);
        free(conn->snd.buf);
    }
    if (conn->rcv.buf != NULL) {
        for (uint32_t i = 0; i < conn->rcv.SEGMAX; i++) {
            if (conn->rcv.buf[i].data != NULL) {
                free(conn->rcv.buf[i].data);
            }
        }
        free(conn->rcv.buf);
    }

    DeList((ListNode*)conn);

    if (conn->synData.buf != NULL) {
        free(conn->synData.buf);
    }

    delete conn;
}

static void FlushMessage(ArdpHandle* handle, ArdpConnRecord* conn, ArdpSndBuf* sBuf, QStatus status)
{
    ArdpHeader* h = (ArdpHeader*) sBuf->hdr;
    uint16_t fcnt = ntohs(h->fcnt);
    uint32_t len = 0;
    /* Original sent data buffer */
    uint8_t* buf = sBuf->data;

    /* Mark all SND buffers associated with the message as available */
    do {
        if (sBuf->timer.retry != 0) {
            assert(conn->state != OPEN);
            DeList((ListNode*) (&sBuf->timer));
        }

        sBuf->inUse = false;
        sBuf->fastRT = 0;
        sBuf->retransmits = 0;
        len += ntohs(h->dlen);
        sBuf = sBuf->next;
        conn->snd.pending--;
        QCC_DbgPrintf(("FlushMessage(fcnt = %d): pending = %d", fcnt, conn->snd.pending));
        assert((conn->snd.pending < conn->snd.SEGMAX) && "Invalid number of pending segments in send queue!");
        h = (ArdpHeader*) sBuf->hdr;
        fcnt--;
    } while (fcnt > 0);

    QCC_DbgPrintf(("FlushMessage(): SendCb(handle=%p, conn=%p, buf=%p, len=%d, status=%d",
                   handle, conn, buf, len, status));
#if ARDP_STATS
    ++handle->stats.sendCbs;
#endif
    handle->cb.SendCb(handle, conn, buf, len, status);
}

static void FlushSendQueue(ArdpHandle* handle, ArdpConnRecord* conn, QStatus status)
{
    /*
     * SendCb() for all pending messages so that the upper layer knows
     * to release the corresponding buffers
     */
    ArdpSndBuf* sBuf = &conn->snd.buf[(conn->snd.LCS + 1) % conn->snd.SEGMAX];
    for (uint32_t i = 0; i < conn->snd.SEGMAX; i++) {
        ArdpHeader* h = (ArdpHeader* ) sBuf->hdr;
        if (sBuf->inUse && (h->seq == h->som)) {
            FlushMessage(handle, conn, sBuf, status);
        }
        sBuf = sBuf->next;
    }
}

static bool IsRcvQueueEmpty(ArdpHandle* handle, ArdpConnRecord* conn)
{
    for (uint32_t i = 0; i < conn->rcv.SEGMAX; i++) {
        if (conn->rcv.buf[i].flags & ARDP_BUFFER_DELIVERED) {
            return false;
        }
    }
    return true;
}

static void MarshalHeader(uint32_t* buf32, ArdpHeader* h)
{
    uint8_t* txbuf = reinterpret_cast<uint8_t*>(buf32);

    *(txbuf + FLAGS_OFFSET) = h->flags;
    *(txbuf + HLEN_OFFSET) = h->hlen;
    *reinterpret_cast<uint16_t*>(txbuf + SRC_OFFSET) = h->src;
    *reinterpret_cast<uint16_t*>(txbuf + DST_OFFSET) = h->dst;
    *reinterpret_cast<uint16_t*>(txbuf + DLEN_OFFSET) = h->dlen;
    *reinterpret_cast<uint32_t*>(txbuf + SEQ_OFFSET) = h->seq;
    *reinterpret_cast<uint32_t*>(txbuf + ACK_OFFSET) = h->ack;
    *reinterpret_cast<uint32_t*>(txbuf + TTL_OFFSET) = h->ttl;
    *reinterpret_cast<uint32_t*>(txbuf + LCS_OFFSET) = h->lcs;
    *reinterpret_cast<uint32_t*>(txbuf + ACKNXT_OFFSET) = h->acknxt;
    *reinterpret_cast<uint32_t*>(txbuf + SOM_OFFSET) = h->som;
    *reinterpret_cast<uint16_t*>(txbuf + FCNT_OFFSET) = h->fcnt;
    *reinterpret_cast<uint16_t*>(txbuf + RSRV_OFFSET) = 0;
}

static void MarshalSynHeader(uint32_t* buf32, ArdpSynHeader* h)
{
    uint8_t* txbuf = reinterpret_cast<uint8_t*>(buf32);

    *(txbuf + FLAGS_OFFSET) = h->flags;
    *(txbuf + HLEN_OFFSET) = h->hlen;
    *reinterpret_cast<uint16_t*>(txbuf + SRC_OFFSET) = h->src;
    *reinterpret_cast<uint16_t*>(txbuf + DST_OFFSET) = h->dst;
    *reinterpret_cast<uint16_t*>(txbuf + DLEN_OFFSET) = h->dlen;
    *reinterpret_cast<uint32_t*>(txbuf + SEQ_OFFSET) = h->seq;
    *reinterpret_cast<uint32_t*>(txbuf + ACK_OFFSET) = h->ack;
    *reinterpret_cast<uint16_t*>(txbuf + SEGMAX_OFFSET) = h->segmax;
    *reinterpret_cast<uint16_t*>(txbuf + SEGBMAX_OFFSET) = h->segbmax;
    *reinterpret_cast<uint32_t*>(txbuf + DACKT_OFFSET) = h->dackt;
    *reinterpret_cast<uint16_t*>(txbuf + OPTIONS_OFFSET) = h->options;
}

static QStatus SendMsgHeader(ArdpHandle* handle, ArdpConnRecord* conn, ArdpHeader* h)
{
    qcc::ScatterGatherList msgSG;
    size_t sent;
    QStatus status;
    uint32_t buf32[ARDP_FIXED_HEADER_LEN >> 2];
    uint32_t len;

    QCC_DbgTrace(("SendMsgHeader(): handle=0x%p, conn=0x%p, hdr=0x%p", handle, conn, h));

    msgSG.AddBuffer(&buf32[0], ARDP_FIXED_HEADER_LEN);

    if (conn->rcv.eack.sz != 0) {
        QCC_DbgPrintf(("SendMsgHeader: have EACKs"));
        h->flags |= ARDP_FLAG_EACK;
        len = ARDP_FIXED_HEADER_LEN + conn->rcv.eack.fixedSz;
        msgSG.AddBuffer(conn->rcv.eack.htnMask, conn->rcv.eack.fixedSz);
    } else {
        len = ARDP_FIXED_HEADER_LEN;
    }

    /* Safe to type cast since len < 512 */
    h->hlen = (uint8_t)(len >> 1);

    /* Marshal the header structure into a byte buffer */
    MarshalHeader(buf32, h);

#if ARDP_TESTHOOKS
    /*
     * Call the outbound testhook in case the test team needs to munge the
     * outbound data.
     */
    if (handle->th.SendToSG) {
        handle->th.SendToSG(handle, conn, SEND_MSG_HEADER, msgSG);
    }
#endif

    status = SendToSG(conn->sock, conn->ipAddr, conn->ipPort, msgSG, sent);
    if (status == ER_WOULDBLOCK) {
        QCC_DbgHLPrintf(("SendMsgHeader: ER_WOULDBLOCK"));
        handle->trafficJam = true;
    } else {
        /* Cancel ACK timer */
        conn->ackTimer.retry = 0;
        conn->ackPending = 0;
    }
    return status;
}

static QStatus Send(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t flags, uint32_t seq, uint32_t ack)
{
    ArdpHeader h;
    memset(&h, 0, sizeof (h));

    QCC_DbgTrace(("Send(handle=%p, conn=%p, flags=0x%02x, seq=%u, ack=%u)", handle, conn, flags, seq, ack));

    h.flags = flags;
    h.src = htons(conn->local);
    h.dst = htons(conn->foreign);;
    h.seq = htonl(seq);
    h.ack = htonl(ack);
    h.lcs = htonl(conn->rcv.LCS);
    h.acknxt = htonl(conn->snd.UNA);

    if (h.dst == 0) {
        QCC_DbgPrintf(("Send(): destination = 0"));
    }

    return SendMsgHeader(handle, conn, &h);
}


static void UnmarshalSynSegment(ArdpConnRecord* conn, uint8_t* buf, ArdpSeg* seg)
{
    conn->foreign = ntohs(*reinterpret_cast<uint16_t*>(buf + SRC_OFFSET)); /* The source ARDP port */
    conn->snd.SEGMAX = ntohs(*reinterpret_cast<uint16_t*>(buf + SEGMAX_OFFSET));     /* Max number of unacknowledged packets other side can buffer */
    conn->snd.SEGBMAX = ntohs(*reinterpret_cast<uint16_t*>(buf + SEGBMAX_OFFSET));   /* Max size segment the other side can handle */
    conn->snd.DACKT = ntohl(*reinterpret_cast<uint32_t*>(buf + DACKT_OFFSET));       /* Delayed ACK timeout from the other side.  */

    conn->rcv.CUR = seg->SEQ;
    conn->rcv.IRS = seg->SEQ;
    conn->rcv.LCS = seg->SEQ;
    conn->window = conn->snd.SEGMAX;

    /* Fixed size of EACK bitmask */
    conn->remoteMskSz = ((conn->snd.SEGMAX + 31) >> 5);
}

static void DisconnectTimerHandler(ArdpHandle* handle, ArdpConnRecord* conn, void* context)
{
    QCC_DbgTrace(("DisconnectTimerHandler: handle=%p conn=%p", handle, conn));

    /* Tricking the compiler */
    QStatus reason = static_cast<QStatus>(reinterpret_cast<uintptr_t>(context));
    QCC_DbgPrintf(("DisconnectTimerHandler: handle=%p conn=%p reason=%p", handle, conn, QCC_StatusText(reason)));
    SetState(conn, CLOSED);

    /*
     * In case the upper layer initiated the disconnect, we still need to send
     * DisconnectCb(). For all the other cases the DisconnectCb was issued inside Disconnect().
     */
    if (reason == ER_OK) {
        if (conn->snd.pending != 0) {
            FlushSendQueue(handle, conn, ER_ARDP_DISCONNECTING);
        }
        QCC_DbgPrintf(("DisconnectTimerHandler: DisconnectCb(handle=%p conn=%p reason = %s)", handle, conn, QCC_StatusText(reason)));
#if ARDP_STATS
        ++handle->stats.disconnectCbs;
#endif
        handle->cb.DisconnectCb(handle, conn, reason);
        /*
         * Change the status to ER_ARDP_INVALID_CONNECTION. This is needed in case the DisconnectTimer is rescheduled
         * in order to wait on RCV queue to drain.
         */
        reason = ER_ARDP_INVALID_CONNECTION;
        conn->connectTimer.context  = (void*) reason;
    }

    /* Check if there are any received buffers delivered to the upper layer that haven't been comsumed yet */
    if (IsRcvQueueEmpty(handle, conn)) {
        DelConnRecord(handle, conn, false);
    } else {
        /* Reschedule connection removal */
        QCC_DbgPrintf(("DisconnectTimerHandler: waiting for receive Q to drain handle=%p, con=%p",
                       handle, conn));
        UpdateTimer(handle, conn, &conn->connectTimer, ARDP_DISCONNECT_RETRY_TIMEOUT, ARDP_DISCONNECT_RETRY);
    }

}

static void ConnectTimerHandler(ArdpHandle* handle, ArdpConnRecord* conn, void* context)
{
    QCC_DbgTrace(("ConnectTimerHandler: handle=%p conn=%p", handle, conn));
    QStatus status = ER_FAIL;
    ArdpTimer* timer = &conn->connectTimer;

    QCC_DbgTrace(("ConnectTimerHandler: retries left %d", timer->retry));

    if (timer->retry > 1) {
        status = DoSendSyn(handle, conn, conn->synData.buf, conn->synData.len);
        if (status == ER_WOULDBLOCK) {
            /* Retry sooner */
            timer->delta = handle->config.connectTimeout >> 2;
            status = ER_OK;
        } else if (status == ER_OK) {
            timer->delta = handle->config.connectTimeout;
        }
    }

    if (status != ER_OK) {
        SetState(conn, CLOSED);
#if ARDP_STATS
        ++handle->stats.connectCbs;
#endif
        handle->cb.ConnectCb(handle, conn, conn->passive, NULL, 0, ER_TIMEOUT);
#if ARDP_STATS
        ++handle->stats.rstSends;
#endif
        Send(handle, conn, ARDP_FLAG_RST | ARDP_FLAG_VER, conn->snd.NXT, conn->rcv.CUR);

        /*
         * Do not delete conection record here:
         * The upper layer will detect an error status and call ARDP_ReleaseConnection()
         */
        timer->retry = 0;
    } else {
        timer->retry--;
    }
}

static void AckTimerHandler(ArdpHandle* handle, ArdpConnRecord* conn, void* context)
{
    QStatus status = Send(handle, conn, ARDP_FLAG_ACK | ARDP_FLAG_VER, conn->snd.NXT, conn->rcv.CUR);

    if (status == ER_WOULDBLOCK) {
        conn->ackTimer.delta = 0;
    }
}

static void ExpireMessageSnd(ArdpHandle* handle, ArdpConnRecord* conn, ArdpSndBuf* sBuf, uint32_t msElapsed)
{
    ArdpHeader* h = (ArdpHeader*) sBuf->hdr;
    uint32_t som = ntohl(h->som);
    uint16_t fcnt = ntohs(h->fcnt);
    uint16_t cnt = fcnt;
    ArdpSndBuf* start = &conn->snd.buf[som % conn->snd.SEGMAX];

    /* Start fragment of the expired message */
    sBuf = start;

    QCC_DbgPrintf(("ExpireMessageSnd: message with SOM %u and fcnt %d expired", som, fcnt));
    do {
        h = (ArdpHeader*) sBuf->hdr;
        sBuf->timer.retry = 0;
        sBuf->ttl = ARDP_TTL_EXPIRED;
        sBuf = sBuf->next;
        cnt--;
    } while (cnt != 0);

    /*
     * If the last unAcked segment is associated with the expired message,
     * advance snd.UNA counter to the start of the next message.
     */
    if (SEQ32_LET(som, conn->snd.UNA) && SEQ32_LT(conn->snd.UNA, som + fcnt)) {
        /*
         * Notice that it is possible for UNA to catch up with NXT when the expired
         * message is the last one that has been sent out.
         */
        conn->snd.UNA = som + fcnt;
        /* Schedule "unsolicited" ACK to allow the receiver to move on */
        if (conn->ackTimer.retry == 0) {
            UpdateTimer(handle, conn, &conn->ackTimer, ARDP_MIN_DELAYED_ACK_TIMEOUT, 1);
        }
    }
}

static QStatus SendMsgData(ArdpHandle* handle, ArdpConnRecord* conn, ArdpSndBuf* sBuf, uint32_t ttl)
{

    ArdpHeader* h = (ArdpHeader*) sBuf->hdr;
    qcc::ScatterGatherList msgSG;
    uint32_t buf32[ARDP_FIXED_HEADER_LEN >> 2];
    uint32_t len;
    size_t sent;
    QStatus status;

    QCC_DbgTrace(("SendMsgData(): handle=%p, conn=%p, hdr=%p, data=%p, datalen=%d, ttl=%u, tStart=%u",
                  handle, conn, sBuf->hdr, sBuf->data, sBuf->datalen, sBuf->ttl, sBuf->tStart));

    msgSG.AddBuffer(&buf32[0], ARDP_FIXED_HEADER_LEN);

    h->ack = htonl(conn->rcv.CUR);
    h->lcs = htonl(conn->rcv.LCS);
    h->acknxt = htonl(conn->snd.UNA);
    h->flags = ARDP_FLAG_ACK | ARDP_FLAG_VER;
    h->ttl = htonl(ttl);

    QCC_DbgPrintf(("SendMsgData(): seq = %u, ack=%u, lcs = %u, ttl=%u", ntohl(h->seq), conn->rcv.CUR, conn->rcv.LCS, ttl));

    if (conn->rcv.eack.sz == 0) {
        len = ARDP_FIXED_HEADER_LEN;
    } else {
        QCC_DbgPrintf(("SendMsgData(): have EACKs"));
        h->flags |= ARDP_FLAG_EACK;
        len = ARDP_FIXED_HEADER_LEN + conn->rcv.eack.fixedSz;
        msgSG.AddBuffer(conn->rcv.eack.htnMask, conn->rcv.eack.fixedSz);
    }

    /* Safe to type cast since len < 512 */
    h->hlen = (uint8_t)(len >> 1);

    /* Marshal the header structure into a byte buffer */
    MarshalHeader(buf32, h);

    /* Add data payload buffer */
    msgSG.AddBuffer(sBuf->data, sBuf->datalen);

#if ARDP_TESTHOOKS
    /*
     * Call the outbound testhook in case the test team needs to munge the
     * outbound data.
     */
    if (handle->th.SendToSG) {
        handle->th.SendToSG(handle, conn, SEND_MSG_DATA, msgSG);
    }
#endif

    status = qcc::SendToSG(conn->sock, conn->ipAddr, conn->ipPort, msgSG, sent);

    if (status == ER_OK) {
        /* Piggyback ACKs with data. Cancel ACK timer. */
        conn->ackTimer.retry = 0;
        conn->ackPending = 0;
        handle->trafficJam = false;
    } else if (status == ER_WOULDBLOCK) {
        handle->trafficJam = true;
    }

    return status;
}

static QStatus Disconnect(ArdpHandle* handle, ArdpConnRecord* conn, QStatus reason)
{
    QStatus status = ER_OK;
    uint32_t timeout = 0;

    QCC_DbgTrace(("Disconnect(handle=%p, conn=%p, reason=%s)", handle, conn, QCC_StatusText(reason)));

    if (conn->state == CLOSE_WAIT || conn->state == CLOSED) {
        QCC_DbgPrintf(("Disconnect(handle=%p, conn=%p, reason=%s) Already disconnect%s",
                       handle, conn, QCC_StatusText(reason), conn->state == CLOSED ? "ed" : "ing"));
        return ER_OK;
    }

    if (!IsConnValid(handle, conn)) {
        return ER_ARDP_INVALID_CONNECTION;
    }

    /* Is there a nice macro that would wrap integer into pointer? Just to avoid nasty surprises... */
    assert(sizeof(QStatus) <=  sizeof(void*));

    SetState(conn, CLOSE_WAIT);

    /* If the local side is the one initiating the disconnect, send RST segment to the remote */
    if (reason != ER_ARDP_REMOTE_CONNECTION_RESET) {
#if ARDP_STATS
        ++handle->stats.rstSends;
#endif
        status = Send(handle, conn, ARDP_FLAG_RST | ARDP_FLAG_VER, conn->snd.NXT, conn->rcv.CUR);
        if (status != ER_OK) {
            QCC_LogError(status, ("Disconnect: failed to send RST to the remote"));
        }
    }

    /* If this disconnect is not a result of ARDP_Disconnect(), inform the upper layer that we are disconnecting */
    if (reason != ER_OK) {
        if (conn->snd.pending != 0) {
            FlushSendQueue(handle, conn, ER_ARDP_DISCONNECTING);
        }
        timeout = handle->config.timewait;
        QCC_DbgPrintf(("Disconnect: Call DisconnectCb() on conn %p, state %s reason %s ",
                       conn, State2Text(conn->state), QCC_StatusText(reason)));
#if ARDP_STATS
        ++handle->stats.disconnectCbs;
#endif
        handle->cb.DisconnectCb(handle, conn, reason);
    }

    InitTimer(handle, conn, &conn->connectTimer, DisconnectTimerHandler, (void*) reason, timeout, ARDP_DISCONNECT_RETRY);

    return status;
}

/*
 *    error = measuredRTT - meanRTT
 *    new meanRTT = 7/8 * meanRTT + 1/8 * error
 *    if (measuredRTT >= (meanRTT - meanVAR))
 *        new meanVar = 3/4 * meanVar + 1/4 * |error|
 *    else
 *        new meanVar = 31/32 * meanVar + 1/32 * |error|
 *
 *    Since ARDP segments can have varying length, maintain additional
 *    mean RTT calculation of UDP MTU unit that will be used in message TTL estimate.
 */
static void AdjustRTT(ArdpHandle* handle, ArdpConnRecord* conn, ArdpSndBuf* sBuf)
{
    uint32_t now = TimeNow(handle->tbase);
    uint16_t units = (sBuf->datalen + UDP_MTU - 1) / UDP_MTU;
    uint32_t rtt = now - sBuf->tStart;
    uint32_t rttUnit = rtt / units;
    int32_t err;

    if (!conn->rttInit) {
        conn->rttMean = rtt;
        conn->rttMeanVar = rtt >> 1;
        conn->rttInit = true;
    }

    err = rtt - conn->rttMean;

    QCC_DbgHLPrintf(("AdjustRtt: mean = %u, var =%u, rtt = %u, now = %u, tStart= %u, error = %d",
                     conn->rttMean, conn->rttMeanVar, rtt, now, sBuf->tStart, err));
    conn->rttMean = (7 * conn->rttMean + rtt) >> 3;

    if ((rtt + conn->rttMeanVar) >= conn->rttMean) {
        conn->rttMeanVar = (conn->rttMeanVar * 3 + ABS(err)) >> 2;
    } else {
        conn->rttMeanVar = (conn->rttMeanVar * 31 + ABS(err)) >> 5;
    }

    rttUnit = (7 * conn->rttMeanUnit + rttUnit) >> 3;

    conn->backoff = 0;

    QCC_DbgHLPrintf(("AdjustRtt: New mean = %u, var =%u", conn->rttMean, conn->rttMeanVar));
}

inline static uint32_t GetRTO(ArdpHandle* handle, ArdpConnRecord* conn)
{
    /* RTO = (rttMean + (4 * rttMeanVar)) << backoff */
    uint32_t ms = (MAX((uint32_t)ARDP_MIN_RTO, conn->rttMean + (4 * conn->rttMeanVar))) << conn->backoff;
    return MIN(ms, (uint32_t)ARDP_MAX_RTO);
}

inline static uint32_t GetDataTimeout(ArdpHandle* handle, ArdpConnRecord* conn)
{
    uint32_t timeout = handle->config.totalDataRetryTimeout;

    if (conn->rttInit) {
        timeout = MAX(timeout, (conn->snd.SEGMAX * conn->snd.SEGBMAX * (conn->rttMean >> 1)) / UDP_MTU);
    }
    return timeout;
}

static void RetransmitTimerHandler(ArdpHandle* handle, ArdpConnRecord* conn, void* context)
{
    ArdpSndBuf* sBuf = (ArdpSndBuf*) context;
    ArdpTimer* timer = &sBuf->timer;
    uint32_t msElapsed = TimeNow(handle->tbase) - sBuf->tStart;
    uint32_t timeout = GetDataTimeout(handle, conn);

    QCC_DbgTrace(("RetransmitTimerHandler: handle=%p conn=%p context=%p", handle, conn, context));

    assert(sBuf->inUse && "RetransmitTimerHandler: trying to resend flushed buffer");

    sBuf->retransmits++;

    if ((msElapsed >= timeout) && (timer->retry > handle->config.minDataRetries)) {
        QCC_DbgHLPrintf(("RetransmitTimerHandler seq=%u hit the time limit %u, retries %u",
                         ntohl(((ArdpHeader*)sBuf->hdr)->seq), timeout, timer->retry));
        timer->retry = 0;
        Disconnect(handle, conn, ER_TIMEOUT);
    } else {
        QStatus status;

        QCC_DbgPrintf(("RetransmitTimerHandler: context=sBuf=%p seq=%u retries=%d",
                       sBuf, ntohl(((ArdpHeader*)sBuf->hdr)->seq), timer->retry));

        /*
         * Check TTL of the segment about to be retransmitted. If TTL expired,
         * do not retransmit and flush the whole message.
         */
        if (sBuf->ttl != ARDP_TTL_INFINITE) {
            /* Factor in mean RTT to account for time on the wire */
            if (conn->rttInit) {
                msElapsed += MIN((conn->rttMeanUnit * (sBuf->datalen + UDP_MTU - 1) / UDP_MTU) >> 1, (conn->rttMean >> 1));
            }

            if (msElapsed >= sBuf->ttl) {
#if ARDP_STATS
                ++handle->stats.outboundDrops;
                ++handle->stats.inflightDrops;
#endif
                QCC_DbgPrintf(("RetransmitTimerHandler: segment %u expired", ntohl(((ArdpHeader*)sBuf->hdr)->seq)));
                ExpireMessageSnd(handle, conn, sBuf, msElapsed);
                return;
            }
        } else {
            msElapsed = 0;
        }

        status = SendMsgData(handle, conn, sBuf, sBuf->ttl - msElapsed);

        if (status == ER_OK) {
            conn->backoff = MAX(conn->backoff, timer->retry);
            if (conn->rttInit) {
                timer->delta = GetRTO(handle, conn);
            } else {
                timer->delta = handle->config.initialDataTimeout;
            }
            timer->retry++;
        } else if (status == ER_WOULDBLOCK) {
            QCC_DbgHLPrintf(("RetransmitTimerHandler: segment %u ER_WOULDBLOCK", ntohl(((ArdpHeader*)sBuf->hdr)->seq)));
            timer->delta = 0;
        } else {
            QCC_LogError(status, ("RetransmitTimerHandler: Write to Socket went bad. Disconnect."));
            timer->retry = 0;
            Disconnect(handle, conn, status);
        }
    }
}

static inline bool IsDataRetransmitScheduled(ArdpConnRecord* conn)
{
    return (((conn->snd.UNA + 1) != conn->snd.NXT) && (conn->snd.UNA != conn->snd.NXT));
}

static void PersistTimerHandler(ArdpHandle* handle, ArdpConnRecord* conn, void* context)
{
    ArdpTimer* timer = &conn->persistTimer;
    QStatus status;

    QCC_DbgHLPrintf(("PersistTimerHandler: handle=%p conn=%p context=%p delta %u retry %u",
                     handle, conn, context, timer->delta, timer->retry));

    if (conn->window < conn->minSendWindow && !IsDataRetransmitScheduled(conn)) {
        if (timer->retry > 1) {
            QCC_DbgPrintf(("PersistTimerHandler: window %u, need at least %u", conn->window, conn->minSendWindow));
            status = Send(handle, conn, ARDP_FLAG_ACK | ARDP_FLAG_VER | ARDP_FLAG_NUL, conn->snd.NXT, conn->rcv.CUR);
            if (status == ER_OK) {
                timer->retry--;
                timer->delta = handle->config.persistInterval << ((handle->config.totalAppTimeout / handle->config.persistInterval) - timer->retry);
#if ARDP_STATS
                ++handle->stats.nulSends;
#endif
            }
        } else {
            QCC_DbgHLPrintf(("PersistTimerHandler: Persist Timeout (frozen window %d, need %d)",
                             conn->window, conn->minSendWindow));
            Disconnect(handle, conn, ER_ARDP_PERSIST_TIMEOUT);
        }
    }
}

static void ProbeTimerHandler(ArdpHandle* handle, ArdpConnRecord* conn, void* context)
{
    ArdpTimer* timer = &conn->probeTimer;
    QStatus status;
    uint32_t now = *((uint32_t* ) context);
    uint32_t elapsed = now - conn->lastSeen;

    QCC_DbgHLPrintf(("ProbeTimerHandler: handle=%p conn=%p delta %u now %u lastSeen = %u elapsed %u",
                     handle, conn, timer->delta, now, conn->lastSeen, elapsed));

    /* If there are no sent segments in flight, reset RTT calculation. */
    if (!IsDataRetransmitScheduled(conn)) {
        conn->rttInit = false;
    }

    /*
     * Checking for total link timeout in very unlikely case the socket write is blocked for
     * exceedingly long time period during which we fail to send probe NUL segment.
     */
    if ((elapsed > timer->delta) || (elapsed >= handle->config.linkTimeout)) {
        if (timer->retry == 0) {
            QCC_DbgHLPrintf(("ProbeTimerHandler: Probe Timeout: now =%u, lastSeen = %u, (limit of %u)", now, conn->lastSeen, handle->config.linkTimeout));
            Disconnect(handle, conn, ER_ARDP_PROBE_TIMEOUT);
        } else {
            QCC_DbgHLPrintf(("ProbeTimerHandler: send ping (NUL packet)"));
            status = Send(handle, conn, ARDP_FLAG_ACK | ARDP_FLAG_VER | ARDP_FLAG_NUL, conn->snd.NXT, conn->rcv.CUR);
            if (status == ER_OK) {
                timer->retry--;
#if ARDP_STATS
                ++handle->stats.nulSends;
#endif
            } else if (status != ER_WOULDBLOCK) {
                /* Socket error */
                Disconnect(handle, conn, ER_FAIL);
            }
        }
    }
}

ArdpHandle* ARDP_AllocHandle(ArdpGlobalConfig* config)
{
    QCC_DbgTrace(("ARDP_AllocHandle()"));

    srand(qcc::Rand32());

    ArdpHandle* handle = new ArdpHandle;
    memset(handle, 0, sizeof(ArdpHandle));
    SetEmpty(&handle->conns);
    SetEmpty(&handle->dataTimers);
    GetTimeNow(&handle->tbase);
    handle->msnext = ARDP_NO_TIMEOUT;
    memcpy(&handle->config, config, sizeof(ArdpGlobalConfig));
    return handle;
}

void ARDP_FreeHandle(ArdpHandle* handle)
{
    QCC_DbgTrace(("ARDP_FreeHandle(handle=0%p)", handle));
    if (!IsEmpty(&handle->conns)) {
        for (ListNode* ln = &handle->conns; (ln = ln->fwd) != &handle->conns;) {
            ListNode* tmp = ln;
            ln = ln->bwd;
            SetState((ArdpConnRecord*)tmp, CLOSED);
            DelConnRecord(handle, (ArdpConnRecord*)tmp, false);
        }
    }
    delete handle;
}

void ARDP_SetAcceptCb(ArdpHandle* handle, ARDP_ACCEPT_CB AcceptCb)
{
    QCC_DbgTrace(("ARDP_SetAcceptCb(handle=%p, AcceptCb=%p)", handle, AcceptCb));
    handle->cb.AcceptCb = AcceptCb;
}

void ARDP_SetConnectCb(ArdpHandle* handle, ARDP_CONNECT_CB ConnectCb)
{
    QCC_DbgTrace(("ARDP_SetConnectCb(handle=%p, ConnectCb=%p)", handle, ConnectCb));
    handle->cb.ConnectCb = ConnectCb;
}

void ARDP_SetDisconnectCb(ArdpHandle* handle, ARDP_DISCONNECT_CB DisconnectCb)
{
    QCC_DbgTrace(("ARDP_SetDisconnectCb(handle=%p, DisconnectCb=%p)", handle, DisconnectCb));
    handle->cb.DisconnectCb = DisconnectCb;
}

void ARDP_SetRecvCb(ArdpHandle* handle, ARDP_RECV_CB RecvCb)
{
    QCC_DbgTrace(("ARDP_SetRecvCb(handle=%p, RecvCb=%p)", handle, RecvCb));
    handle->cb.RecvCb = RecvCb;
}

void ARDP_SetSendCb(ArdpHandle* handle, ARDP_SEND_CB SendCb)
{
    QCC_DbgTrace(("ARDP_SetSendCb(handle=%p, SendCb=%p)", handle, SendCb));
    handle->cb.SendCb = SendCb;
}

void ARDP_SetSendWindowCb(ArdpHandle* handle, ARDP_SEND_WINDOW_CB SendWindowCb)
{
    QCC_DbgTrace(("ARDP_SetSendWindowCb(handle=%p, SendWindowCb=%p)", handle, SendWindowCb));
    handle->cb.SendWindowCb = SendWindowCb;
}

#if ARDP_TESTHOOKS
void ARDP_HookSendToSG(ArdpHandle* handle, ARDP_SENDTOSG_TH SendToSG)
{
    QCC_DbgTrace(("ARDP_HookSendToSG(handle=%p, RecvCb=%p)", handle, SendToSG));
    handle->th.SendToSG = SendToSG;
}

void ARDP_HookSendTo(ArdpHandle* handle, ARDP_SENDTO_TH SendTo)
{
    QCC_DbgTrace(("ARDP_HookSendTo(handle=%p, SendTo=%p)", handle, SendTo));
    handle->th.SendTo = SendTo;
}

void ARDP_HookRecvFrom(ArdpHandle* handle, ARDP_RECVFROM_TH RecvFrom)
{
    QCC_DbgTrace(("ARDP_HookRecvFrom(handle=%p, RecvFrom=%p)", handle, RecvFrom));
    handle->th.RecvFrom = RecvFrom;
}
#endif

#if ARDP_STATS
ArdpStats* ARDP_GetStats(ArdpHandle* handle)
{
    QCC_DbgTrace(("ARDP_GetStats(handle=%p)", handle));
    return &handle->stats;
}

void ARDP_ResetStats(ArdpHandle* handle)
{
    QCC_DbgTrace(("ARDP_ResetStats(handle=%p)", handle));
    memset(&handle->stats, 0, sizeof(handle->stats));

}
#endif

void ARDP_SetHandleContext(ArdpHandle* handle, void* context)
{
    QCC_DbgTrace(("ARDP_SetHandleContext(handle=%p, context=%p)", handle, context));
    handle->context = context;
}

void ARDP_ReleaseConnection(ArdpHandle* handle, ArdpConnRecord* conn)
{
    QCC_DbgTrace(("ARDP_ReleaseConnection(handle=%p, conn=%p)", handle, conn));
    if (!IsConnValid(handle, conn)) {
        QCC_LogError(ER_ARDP_INVALID_CONNECTION, ("ARDP_ReleaseConnection(handle=%p), context = %p", handle, handle->context));
        return;
    }
    DelConnRecord(handle, conn, true);
}

void* ARDP_GetHandleContext(ArdpHandle* handle)
{
    QCC_DbgTrace(("ARDP_GetHandleContext(handle=%p)", handle));
    return handle->context;
}

bool ARDP_IsConnValid(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t connId)
{
    QCC_DbgTrace(("ARDP_IsConnValid(handle=%p, conn=%p)", handle, conn));
    if (IsConnValid(handle, conn)) {
        if (conn->id == connId) {
            return true;
        }
    }
    return false;
}

QStatus ARDP_SetConnContext(ArdpHandle* handle, ArdpConnRecord* conn, void* context)
{
    QCC_DbgTrace(("ARDP_SetConnContext(handle=%p, conn=%p, context=%p)", handle, conn, context));
    if (!IsConnValid(handle, conn)) {
        QCC_LogError(ER_ARDP_INVALID_CONNECTION, ("ARDP_SetHandleContext(handle=%p), context = %p", handle, handle->context));
        return ER_ARDP_INVALID_CONNECTION;
    }
    conn->context = context;
    return ER_OK;
}

void* ARDP_GetConnContext(ArdpHandle* handle, ArdpConnRecord* conn)
{
    QCC_DbgTrace(("ARDP_GetConnContext(handle=%p, conn=%p)", handle, conn));
    if (!IsConnValid(handle, conn)) {
        QCC_LogError(ER_ARDP_INVALID_CONNECTION, ("ARDP_GetHandleContext(handle=%p), context = %p", handle, handle->context));
        return NULL;
    }
    return conn->context;
}

uint32_t ARDP_GetConnId(ArdpHandle* handle, ArdpConnRecord* conn)
{
    QCC_DbgTrace(("ARDP_GetConnId(handle=%p, conn=%p)", handle, conn));
    if (!IsConnValid(handle, conn)) {
        QCC_LogError(ER_ARDP_INVALID_CONNECTION, ("ARDP_GetConnId(handle=%p), context = %p", handle, handle->context));
        return ARDP_CONN_ID_INVALID;
    }
    return conn->id;
}

uint32_t ARDP_GetConnPending(ArdpHandle* handle, ArdpConnRecord* conn)
{
    QCC_DbgTrace(("ARDP_GetConnPending(handle=%p, conn=%p)", handle, conn));
    if (!IsConnValid(handle, conn)) {
        QCC_LogError(ER_ARDP_INVALID_CONNECTION, ("ARDP_GetConnPending(handle=%p), context = %p", handle, handle->context));
        assert(false && "Connection not found");
        return 0;
    }
    return conn->snd.pending;
}

qcc::IPAddress ARDP_GetIpAddrFromConn(ArdpHandle* handle, ArdpConnRecord* conn)
{
    QCC_DbgTrace(("ARDP_GetIpAddrFromConn(handle=%p, conn=%p)", handle, conn));
    if (!IsConnValid(handle, conn)) {
        QCC_LogError(ER_ARDP_INVALID_CONNECTION, ("ARDP_GetIpAddrFromConn(handle=%p), context = %p", handle, handle->context));
        assert(false && "Connection not found");
    }
    /*
     * Note: this is called ONLY from successful ConnectCb(). It should be safe
     * to return an address here. The above check is for catching programming error.
     */
    return conn->ipAddr;
}

uint16_t ARDP_GetIpPortFromConn(ArdpHandle* handle, ArdpConnRecord* conn)
{
    QCC_DbgTrace(("ARDP_GetIpPortFromConn(handle=%p, conn=%p)", handle, conn));
    if (!IsConnValid(handle, conn)) {
        QCC_LogError(ER_ARDP_INVALID_CONNECTION, ("ARDP_GetIpPortFromConn(handle=%p), context = %p", handle, handle->context));
        assert(false && "Connection not found");
    }
    /*
     * Note: this is called ONLY from successful ConnectCb(). It should be safe
     * to return an IP port here. The above check is for catching programming error.
     */
    return conn->ipPort;
}

/*
 * Dynamic data timeout based on connection statistics.
 * Add some headroom for data timeout as it will appear outside ARDP.
 */
uint32_t ARDP_GetDataTimeout(ArdpHandle* handle, ArdpConnRecord* conn)
{
    if (!IsConnValid(handle, conn)) {
        QCC_LogError(ER_ARDP_INVALID_CONNECTION, ("ARDP_GetIpPortFromConn(handle=%p), context = %p", handle, handle->context));
        return handle->config.totalDataRetryTimeout + 2 * handle->config.initialDataTimeout;
    }

    return (GetDataTimeout(handle, conn) + 2 * handle->config.initialDataTimeout);
}

static ArdpConnRecord* NewConnRecord(void)
{
    QCC_DbgTrace(("NewConnRecord()"));
    ArdpConnRecord* conn = new ArdpConnRecord();
    memset(conn, 0, sizeof(ArdpConnRecord));
    do {
        conn->id = qcc::Rand32();
    } while (conn->id == ARDP_CONN_ID_INVALID);
    QCC_DbgTrace(("NewConnRecord(): conn %p, id %u", conn, conn->id));
    SetEmpty(&conn->list);
    return conn;
}

static QStatus InitRcv(ArdpConnRecord* conn, uint32_t segmax, uint32_t segbmax)
{
    conn->rcv.SEGMAX = segmax;     /* The maximum number of outstanding segments that we can buffer (we will tell other side) */
    conn->rcv.SEGBMAX = segbmax;   /* The largest buffer that can be received on this connection (our buffer size) */

    conn->rcv.buf = (ArdpRcvBuf*) malloc(segmax * sizeof(ArdpRcvBuf));
    if (conn->rcv.buf == NULL) {
        QCC_DbgPrintf(("InitRcv: Failed to allocate space for receive structure"));
        return ER_OUT_OF_MEMORY;
    }

    memset(conn->rcv.buf, 0, segmax * sizeof(ArdpRcvBuf));
    for (uint32_t i = 0; i < segmax; i++) {
        conn->rcv.buf[i].next = &conn->rcv.buf[((i + 1) % segmax)];
    }
    return ER_OK;
}

/* Extra Initialization after connection has been established */
static void PostInitRcv(ArdpConnRecord* conn)
{
    conn->rcv.LCS = conn->rcv.CUR;
    for (uint16_t i = 0; i < conn->rcv.SEGMAX; i++) {
        conn->rcv.buf[i].seq = conn->rcv.IRS;
    }
}

static QStatus InitConnRecord(ArdpHandle* handle, ArdpConnRecord* conn, qcc::SocketFd sock, qcc::IPAddress ipAddr, uint16_t ipPort, uint16_t foreign)
{
    QCC_DbgTrace(("InitConnRecord(handle=%p, conn=%p, sock=%d, ipAddr=\"%s\", ipPort=%d, foreign=%d)",
                  handle, conn, sock, ipAddr.ToString().c_str(), ipPort, foreign));
    uint16_t local;
    uint32_t count = 0;

    conn->state = CLOSED;                 /* Starting state is always CLOSED */
    local = (qcc::Rand32() % 65534) + 1;  /* Allocate an "ephemeral" source port */

    /* Make sure this is a unique combiation of foreign/local */
    while (FindConn(handle, local, foreign) != NULL) {
        local++;
        count++;
        if (count == 65535) {
            /* Really? We exhausted all the connections?! */
            QCC_LogError(ER_FAIL, ("InitConnRecord: Cannot get a new connection record. Too many connections?"));
            return ER_FAIL;
        }
    }

    conn->local = local;
    conn->foreign = foreign;                     /* The ARDP port of the foreign host */

    conn->sock = sock;                           /* The socket to use when talking on this connection */
    conn->ipAddr = ipAddr;                       /* The IP address of the foreign host */
    conn->ipPort = ipPort;                       /* The UDP port of the foreign host */

    conn->lastSeen = TimeNow(handle->tbase);

    /* Initialize the sender side of the connection */
    conn->snd.ISS = qcc::Rand32();             /* Initial sequence number used for sending data over this connection */
    conn->snd.NXT = conn->snd.ISS + 1;         /* The sequence number of the next segment to be sent over this connection */
    conn->snd.UNA = conn->snd.ISS;             /* The oldest unacknowledged segment is the ISS */
    conn->snd.LCS = conn->snd.ISS;             /* The most recently consumed segment (we keep this in sync with the other side) */

    conn->rttInit = false;
    conn->rttMean = handle->config.initialDataTimeout;
    conn->rttMeanUnit = handle->config.initialDataTimeout;
    conn->rttMeanVar = 0;

    conn->backoff = 0;

    return ER_OK;
}

static void ProtocolDemux(uint8_t* buf, uint16_t len, uint16_t* local, uint16_t* foreign)
{
    QCC_DbgTrace(("ProtocolDemux(buf=%p, len=%d, local*=%p, foreign*=%p)", buf, len, local, foreign));
    *local = ntohs(*reinterpret_cast<uint16_t*>(buf + DST_OFFSET));
    *foreign = ntohs(*reinterpret_cast<uint16_t*>(buf + SRC_OFFSET));
    QCC_DbgTrace(("ProtocolDemux(): local %d, foreign %d", *local, *foreign));
}

static ArdpConnRecord* FindConn(ArdpHandle* handle, uint16_t local, uint16_t foreign)
{
    QCC_DbgTrace(("FindConn(handle=%p, local=%d, foreign=%d)", handle, local, foreign));

    for (ListNode* ln = &handle->conns; (ln = ln->fwd) != &handle->conns;) {
        ArdpConnRecord* conn = (ArdpConnRecord*)ln;
        QCC_DbgPrintf(("FindConn(): conn %p local = %d, foreign = %d", conn, conn->local, conn->foreign));
        if (conn->local == local && conn->foreign == foreign) {
            QCC_DbgPrintf(("FindConn(): Found conn %p", conn));
            return conn;
        }
    }
    return NULL;
}

static QStatus SendData(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t* buf, uint32_t len, uint32_t ttl)
{
    QStatus status = ER_OK;
    uint32_t timeout = handle->config.initialDataTimeout;
    uint16_t fcnt;
    uint32_t lastLen;
    uint8_t* segData = buf;
    uint32_t som = htonl(conn->snd.NXT);
    uint16_t index = conn->snd.NXT % conn->snd.SEGMAX;
    ArdpSndBuf* sBuf = &conn->snd.buf[index];
    uint32_t now = TimeNow(handle->tbase);
    uint32_t ttlSend = ttl;

    QCC_DbgTrace(("SendData(handle=%p, conn=%p, buf=%p, len=%u, ttl=%u)", handle, conn, buf, len, ttl));
    QCC_DbgPrintf(("SendData(): Sending %u bytes of data from src=0x%x to dst=0x%x", len, conn->local, conn->foreign));

    /* Check if message needs to be fragmented */
    if (len <= conn->snd.maxDlen) {
        /* Data fits into one segment */
        fcnt = 1;
        lastLen = len;
    } else {
        /* Need fragmentation */
        fcnt = (len + (conn->snd.maxDlen - 1)) / conn->snd.maxDlen;
        lastLen = len - conn->snd.maxDlen * (fcnt - 1);
        QCC_DbgPrintf(("SendData(): Large buffer %d, partitioning into %d segments", len, fcnt));
    }

    assert(fcnt != 0);

    /* Check if receiver's window is wide enough to accept FCNT number of segments */
    if (fcnt > conn->window) {
        QCC_DbgPrintf(("SendData(): number of fragments %u exceeds the window size %u", fcnt, conn->window));
        return ER_ARDP_BACKPRESSURE;
    }

    /* Check if send queue is deep enough to hold FCNT number of segments */
    if (fcnt > (conn->snd.SEGMAX - conn->snd.pending)) {
        QCC_DbgPrintf(("SendData(): number of fragments %u exceeds the send queue depth %u",
                       fcnt, conn->snd.SEGMAX - conn->snd.pending));
        return ER_ARDP_BACKPRESSURE;
    }

    /*
     * Check if the message's TTL is less than half RTT. If this is the case,
     * do not bother sending, return error.
     */
    if (conn->rttInit && (ttl != ARDP_TTL_INFINITE)) {
        uint32_t expireThreshold = MIN((conn->rttMeanUnit * (len + UDP_MTU - 1) / UDP_MTU) >> 1, ((conn->rttMean * fcnt) >> 1));
        if ((ttl + conn->snd.DACKT) <= expireThreshold) {

#if ARDP_STATS
            ++handle->stats.outboundDrops;
            ++handle->stats.preflightDrops;
#endif
            QCC_DbgPrintf(("SendMsgData(): Dropping expired message (conn=0x%p, buf=0x%p, len=%d, ttl=%u, dack=%u, rttMean=%u)",
                           conn, sBuf->data, sBuf->datalen, ttl, conn->snd.DACKT, conn->rttMean >> 1));

            return ER_ARDP_TTL_EXPIRED;
        }

        /* If we passed the above "expire" test only due to factoring in DACKT, do not adjust ttl */
        if (ttl > expireThreshold) {
            ttlSend = ttl - expireThreshold;
        }
    }

    for (uint16_t i = 0; i < fcnt; i++) {
        ArdpHeader* h = (ArdpHeader*) sBuf->hdr;
        uint16_t segLen = (i == (fcnt - 1)) ? lastLen : conn->snd.maxDlen;

        status = ER_OK;

        QCC_DbgPrintf(("SendData: Segment %d, snd.NXT=%u, snd.UNA=%u", i, conn->snd.NXT, conn->snd.UNA));
        assert((conn->snd.NXT - conn->snd.UNA) < conn->snd.SEGMAX);

        h->som = som;
        h->fcnt = htons(fcnt);
        h->src = htons(conn->local);
        h->dst = htons(conn->foreign);;
        h->dlen = htons(segLen);
        h->seq = htonl(conn->snd.NXT);
        sBuf->ttl = ttl;
        sBuf->tStart = now;
        sBuf->data = segData;
        sBuf->datalen = segLen;
        if (h->dst == 0) {
            QCC_DbgPrintf(("SendData(): destination = 0"));
        }

        if (!handle->trafficJam) {
            status = SendMsgData(handle, conn, sBuf, ttlSend);
            if (conn->rttInit) {
                timeout = GetRTO(handle, conn);
            } else {
                timeout = handle->config.initialDataTimeout;
            }
        }

        if (handle->trafficJam) {
            timeout = 0;
            status = ER_OK;
        }

        /*
         * We change update our accounting only if the message has been sent successfully
         * or has been put on the retransmit queue */
        if (status == ER_OK) {
            sBuf->inUse = true;
            UpdateTimer(handle, conn, &sBuf->timer, timeout, 1);
            /* Since we scheduled a retransmit timer, cancel active persist timer */
            conn->persistTimer.retry = 0;
            EnList(handle->dataTimers.bwd, (ListNode*) &sBuf->timer);
            conn->snd.pending++;
            assert(((conn->snd.pending) <= conn->snd.SEGMAX) && "Number of pending segments in send queue exceeds MAX!");
            conn->snd.NXT++;
        } else {
            /* Something irrevocably bad happened on the socket. Disconnect. */
            Disconnect(handle, conn, status);
            break;
        }

        segData += segLen;
        sBuf = sBuf->next;
    }

    return status;
}

static QStatus DoSendSyn(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t* buf, uint16_t len)
{
    ArdpSynHeader hSyn;
    qcc::ScatterGatherList msgSG;
    uint32_t buf32[(ARDP_SYN_HEADER_SIZE + 3) >> 2];
    size_t sent;

    QCC_DbgTrace(("DoSendSyn(handle=%p, conn=%p, buf=%p, len = %d)", handle, conn, buf, len));

    hSyn.flags = ARDP_FLAG_SYN | ARDP_FLAG_VER;

    if (conn->passive) {
        hSyn.flags |= ARDP_FLAG_ACK;
    }

    hSyn.hlen = ARDP_SYN_HEADER_SIZE >> 1;
    hSyn.src = htons(conn->local);
    hSyn.dst = htons(conn->foreign);
    hSyn.dlen = htons(len);
    hSyn.seq = htonl(conn->snd.ISS);
    hSyn.ack = htonl(conn->rcv.CUR);
    hSyn.segmax = htons(conn->rcv.SEGMAX);
    hSyn.segbmax = htons(conn->rcv.SEGBMAX);
    hSyn.dackt = htonl(handle->config.delayedAckTimeout);
    hSyn.options = htons(ARDP_FLAG_SDM);

    if (hSyn.dst == 0) {
        QCC_DbgPrintf(("DoSendSyn(): destination = 0"));
    }

    /* Marshal the header structure into a byte buffer */
    MarshalSynHeader(buf32, &hSyn);

    QCC_DbgPrintf(("DoSendSyn(): hSyn.seq=%u  data=%p (%s), len=%u", ntohl(hSyn.seq), conn->synData.buf, conn->synData.buf,
                   conn->synData.len));

    msgSG.AddBuffer(&buf32[0], ARDP_SYN_HEADER_SIZE);
    msgSG.AddBuffer(conn->synData.buf, conn->synData.len);

#if ARDP_TESTHOOKS
    /*
     * Call the outbound testhook in case the test team needs to munge the
     * outbound data.
     */
    if (handle->th.SendToSG) {
        handle->th.SendToSG(handle, conn, DO_SEND_SYN, msgSG);
    }
#endif

#if ARDP_STATS
    ++handle->stats.synSends;
#endif

    return qcc::SendToSG(conn->sock, conn->ipAddr, conn->ipPort, msgSG, sent);
}

static QStatus SendSyn(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t* buf, uint16_t len)
{
    QStatus status;

    QCC_DbgTrace(("SendSyn(handle=%p, conn=%p, buf=%p, len=%d)", handle, conn, buf, len));
    conn->synData.buf = (uint8_t*) malloc(len);
    if (conn->synData.buf == NULL) {
        return ER_OUT_OF_MEMORY;
    }

    conn->synData.len = len;
    memcpy(conn->synData.buf, buf, len);
    status = DoSendSyn(handle, conn, buf, len);
    if (status == ER_OK) {
        InitTimer(handle, conn, &conn->connectTimer, ConnectTimerHandler, NULL, handle->config.connectTimeout, handle->config.connectRetries + 1);
        QCC_DbgPrintf(("SendSyn(): timer=%p, retries=%u", conn->connectTimer, conn->connectTimer.retry));
    }
    return status;
}

static QStatus SendRst(ArdpHandle* handle, qcc::SocketFd sock, qcc::IPAddress ipAddr, uint16_t ipPort, uint16_t local, uint16_t foreign)
{
    QCC_DbgTrace(("SendRst(handle=%p, sock=%d., ipAddr=\"%s\", ipPort=%d., local=%d., foreign=%d.)",
                  handle, sock, ipAddr.ToString().c_str(), ipPort, local, foreign));

    ArdpHeader h;
    uint32_t buf32[ARDP_FIXED_HEADER_LEN >> 2];

    memset(&h, 0, sizeof(ArdpHeader));
    h.flags = ARDP_FLAG_RST | ARDP_FLAG_VER;
    h.hlen = ARDP_FIXED_HEADER_LEN >> 1;
    h.src = htons(local);
    h.dst = htons(foreign);

    MarshalHeader(buf32, &h);
    QCC_DbgPrintf(("SendRst(): SendTo(sock=%d., ipAddr=\"%s\", port=%d., buf=%p, len=%d", sock, ipAddr.ToString().c_str(), ipPort, &h, ARDP_FIXED_HEADER_LEN));

#if ARDP_TESTHOOKS
    /*
     * Call the outbound testhook in case the test team needs to munge the
     * outbound data.
     */
    if (handle->th.SendTo) {
        handle->th.SendTo(handle, NULL, SEND_RST, &h, ARDP_FIXED_HEADER_LEN);
    }
#endif

#if ARDP_STATS
    ++handle->stats.rstSends;
#endif

    size_t sent;
    return qcc::SendTo(sock, ipAddr, ipPort, &h, ARDP_FIXED_HEADER_LEN, sent);
}

static QStatus UpdateSndSegments(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t ack, uint32_t lcs) {
    QCC_DbgTrace(("UpdateSndSegments(): handle=%p, conn=%p, ack=%u, lcs=%u", handle, conn, ack, lcs));
    uint16_t index = ack % conn->snd.SEGMAX;
    ArdpSndBuf* sBuf = &conn->snd.buf[index];
    bool needUpdate = false;

    /* Nothing to clean up */
    if (conn->snd.pending == 0) {
        conn->snd.LCS = lcs;
        return ER_OK;
    }

    /*
     * Count only "good" roundrips to ajust RTT values.
     * Note, that we decrement retries with each retransmit.
     */
    if ((sBuf->retransmits == 0) && (sBuf->timer.retry != 0)) {
        AdjustRTT(handle, conn, sBuf);
    }


    /* Cycle through the buffers from snd.LCS + 1 to ACK */
    index = (conn->snd.LCS + 1) % conn->snd.SEGMAX;
    sBuf = &conn->snd.buf[index];

    for (uint32_t i = 0; i < (ack - conn->snd.LCS); i++) {
        ArdpHeader* h = (ArdpHeader*) sBuf->hdr;
        uint32_t seq = ntohl(h->seq);
        uint16_t fcnt = ntohs(h->fcnt);

        if (!(sBuf->inUse) || (SEQ32_LT(ack, seq))) {
            QCC_LogError(ER_ARDP_INVALID_RESPONSE, ("Bad update: %s SND %u ACK %u",
                                                    (sBuf->inUse) ? "full" : "empty", seq, ack));
            return ER_ARDP_INVALID_RESPONSE;
        }

        if (sBuf->inUse) {
            /* If fragmented, wait for the last segment. Issue sendCB on the first fragment in message.*/
            QCC_DbgPrintf(("UpdateSndSegments(): fragment=%u, som=%u, fcnt=%d",
                           ntohl(h->seq), ntohl(h->som), fcnt));
            DeList((ListNode*) &sBuf->timer);
            sBuf->timer.retry = 0;

            /*
             * If the message has been consumed by the receiver,
             * check if this is the last fragment in the message and issue SendCb()
             */
            if (SEQ32_LET(seq, lcs) && seq == (ntohl(h->som) + (fcnt - 1))) {
                QCC_DbgPrintf(("UpdateSndSegments(): last fragment=%u, som=%u, fcnt=%d",
                               seq, ntohl(h->som), fcnt));

                /* First segment in message, keeps the original pointer to message buffer */
                index = ntohl(h->som) % conn->snd.SEGMAX;
                FlushMessage(handle, conn, &conn->snd.buf[index], (sBuf->ttl != ARDP_TTL_EXPIRED) ? ER_OK : ER_ARDP_TTL_EXPIRED);
            }
        }

        sBuf = sBuf->next;
    }

    /* Update snd.UNA counter if the TTL of segments following last ACKed segment have expired */
    while (sBuf->inUse && (sBuf->ttl == ARDP_TTL_EXPIRED)) {
        ArdpHeader* h = (ArdpHeader*) sBuf->hdr;
        uint32_t seq = ntohl(h->seq);
        conn->snd.UNA = seq + 1;
        sBuf = sBuf->next;
        needUpdate = true;
    }

    /* Schedule "unsolicited" ACK */
    if (needUpdate && (conn->ackTimer.retry == 0)) {
        UpdateTimer(handle, conn, &conn->ackTimer, ARDP_MIN_DELAYED_ACK_TIMEOUT, 1);
    }

    /* Update the counter on our SND side */
    conn->snd.LCS = lcs;

    return ER_OK;
}

static void FastRetransmit(ArdpHandle* handle, ArdpConnRecord* conn, ArdpSndBuf* sBuf)
{
    /*
     * Fast retransmit to fill the gap. Schedule only for those segments that haven't been
     * tried for retransmission yet.
     */
    if ((sBuf->fastRT == handle->config.fastRetransmitAckCounter) && (sBuf->retransmits == 0)) {
        QCC_DbgPrintf(("FastRetransmit(): priority re-send %u", ntohl(((ArdpHeader*)sBuf->hdr)->seq)));
        sBuf->timer.when = TimeNow(handle->tbase);
    }
    sBuf->fastRT++;
}

static void CancelEackedSegments(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t ack, uint32_t* bitMask) {
    QCC_DbgHLPrintf(("CancelEackedSegments(): handle=%p, conn=%p, bitMask=%p, ack=%u (snd.Una %u)",
                     handle, conn, bitMask, ack, conn->snd.UNA));
    uint32_t start = ack + 1;
    uint32_t index =  start % conn->snd.SEGMAX;
    ArdpSndBuf* sBuf = &conn->snd.buf[index];
    uint32_t bitCheck = 1 << 31;

#ifndef NDEBUG
    DumpBitMask(conn, bitMask, conn->remoteMskSz, true);
#endif
    FastRetransmit(handle, conn, sBuf);

    /*
     * Bitmask starts at ACK + 1. Cycle through the mask and cancel retransmit timers
     * on EACKed segments.
     */
    start = start + 1;

    /* Iterate over 32-bit bins */
    for (uint32_t i = 0; i < conn->remoteMskSz; i++) {
        uint32_t mask32 = ntohl(bitMask[i]);

        index = (start + (i * 32)) % conn->snd.SEGMAX;
        sBuf = &conn->snd.buf[index];
        while (mask32 != 0) {
            if (mask32 & bitCheck) {
                QCC_DbgPrintf(("CancelEackedSegments(): set retries to zero for timer %p (seq %u)",
                               sBuf->timer, ntohl(((ArdpHeader*)(sBuf->hdr))->seq)));
                if (sBuf->timer.retry != 0) {
                    DeList((ListNode*) &sBuf->timer);
                    sBuf->timer.retry = 0;
                }
            } else if (i < 1) {
                /*
                 * Schedule fast retransmits for gaps in the first 32-segment window.
                 * Catch the others next time the EACK window moves.
                 */
                FastRetransmit(handle, conn, sBuf);
            }

            mask32 = mask32 << 1;
            sBuf = sBuf->next;
        }
    }
}

static void ShiftRcvMsk(ArdpConnRecord* conn)
{
    QCC_DbgPrintf(("ShiftRcvMsk"));
    if (conn->rcv.eack.sz == 0) {
        /* No EACKS */
        return;
    }

    /* First bit represents rcv.CUR + 2 */
    uint32_t saveBits = 0;
    uint16_t newSz = 0;
    uint32_t i = 1;

    /* Shift the bitmask */
    conn->rcv.eack.mask[0] = conn->rcv.eack.mask[0] << 1;
    if (conn->rcv.eack.mask[0] > 0) {
        newSz = 1;
    }

    for (; i < conn->rcv.eack.sz; i++) {
        if (conn->rcv.eack.mask[i] == 0) {
            continue;
        }
        saveBits = conn->rcv.eack.mask[i] >> 31;
        conn->rcv.eack.mask[i] = conn->rcv.eack.mask[i] << 1;
        conn->rcv.eack.mask[i - 1] =  conn->rcv.eack.mask[i - 1] | saveBits;
        if (conn->rcv.eack.mask[i] > 0) {
            newSz = i + 1;
        } else if (saveBits != 0) {
            newSz = i;
        }
        conn->rcv.eack.htnMask[i - 1] = htonl(conn->rcv.eack.mask[i - 1]);
    }
    conn->rcv.eack.htnMask[i - 1] = htonl(conn->rcv.eack.mask[i - 1]);

    conn->rcv.eack.sz = newSz;

#ifndef NDEBUG
    DumpBitMask(conn, conn->rcv.eack.mask, conn->rcv.eack.fixedSz >> 2, false);
#endif
}

static void AddRcvMsk(ArdpConnRecord* conn, uint32_t delta)
{
    QCC_DbgPrintf(("AddRcvMsk: delta = %d", delta));
    /* First bit represents rcv.CUR + 2 */
    uint32_t bin32 = (delta - 1) / 32;
    uint32_t offset = (uint32_t)32 - (delta - (bin32 << 5));

    assert(bin32 < (uint32_t) (conn->rcv.eack.fixedSz >> 2));
    conn->rcv.eack.mask[bin32] = conn->rcv.eack.mask[bin32] | ((uint32_t)1 << offset);
    QCC_DbgPrintf(("AddRcvMsk: bin = %d, offset=%d, mask = 0x%f", bin32, offset, conn->rcv.eack.mask[bin32]));
    if (conn->rcv.eack.sz < bin32 + 1) {
        conn->rcv.eack.sz = bin32 + 1;
    }
    conn->rcv.eack.htnMask[bin32] = htonl(conn->rcv.eack.mask[bin32]);

#ifndef NDEBUG
    DumpBitMask(conn, conn->rcv.eack.mask, conn->rcv.eack.fixedSz >> 2, false);
#endif

}

static void DumpRcvQueue(ArdpConnRecord* conn)
{
    uint32_t i;
    for (i = 0; i < conn->rcv.SEGMAX; i++) {
        QCC_LogError(ER_OK, ("%d: %s, %s seq %u, som %u, fcnt %d ttl %x", i,
                             (conn->rcv.buf[i].flags && ARDP_BUFFER_IN_USE) ? "in use" : "not in use",
                             (conn->rcv.buf[i].flags && ARDP_BUFFER_DELIVERED) ? "delivered" : "not delivered",
                             conn->rcv.buf[i].seq, conn->rcv.buf[i].som, conn->rcv.buf[i].fcnt, conn->rcv.buf[i].ttl));

    }
}

static QStatus ReleaseRcvBuffers(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t seq, uint16_t fcnt, QStatus reason)
{
    uint16_t index = seq % conn->rcv.SEGMAX;
    ArdpRcvBuf* consumed = &conn->rcv.buf[index];
    uint32_t count = fcnt;

    QCC_DbgTrace(("ReleaseRcvBuffers(handle=%p, conn=%p, seq=%u, fcnt=%u, reason=%s)",
                  handle, conn, seq, fcnt, QCC_StatusText(reason)));

    if (fcnt == 0) {
        QCC_DbgHLPrintf(("Invalid fragment count %u", fcnt));
        assert((fcnt > 0) && "fcnt cannot be zero");
        return ER_FAIL;
    }

    if (seq != (conn->rcv.LCS + 1)) {
        if (reason == ER_ARDP_TTL_EXPIRED) {
            QCC_DbgPrintf(("Expired segment %u is not first in rcv queue (%u)", seq, conn->rcv.LCS + 1));
            return ER_OK;
        } else {
            QCC_LogError(ER_OK, ("conn %p, Consumed message %u is not first in rcv queue (%u)", conn, seq, conn->rcv.LCS + 1));
            DumpRcvQueue(conn);
            assert(0 && "Consumed message is not first in rcv queue");
            return ER_FAIL;
        }
    }

    /* Perform additional validation if the buffer was handed to us by the transport layer */
    if (reason != ER_ARDP_TTL_EXPIRED) {
        if (conn->rcv.buf[index].seq != seq) {
            QCC_DbgHLPrintf(("ReleaseRcvBuffers: released buffer seq=%u does not match rcv %u", seq, conn->rcv.buf[index].seq));
            assert(0 && "ReleaseRcvBuffers: Buffer sequence validation failed");
            return ER_FAIL;
        }
    }

    /*
     * Release all the buffers associated with consumed message.
     * Also, release the subsequent buffers with expired TTL.
     */
    do {
        consumed->flags = 0;
        consumed->ttl = ARDP_TTL_INFINITE;
        if (consumed->data != NULL) {
            free(consumed->data);
            consumed->data = NULL;
        }
        conn->rcv.LCS++;
        if (count != 0) {
            count--;
        }

        QCC_DbgPrintf(("ReleaseRcvBuffers: released buffer %p (seq=%u)", consumed, conn->rcv.LCS));
        consumed = consumed->next;
        QCC_DbgPrintf(("ReleaseRcvBuffers: next buffer %p, ttl = 0x%x, is %s",
                       consumed, consumed->ttl, (consumed->flags & ARDP_BUFFER_DELIVERED) ? "delivered" : "not delivered"));

    } while (count != 0 || (consumed->ttl == ARDP_TTL_EXPIRED && !(consumed->flags & ARDP_BUFFER_DELIVERED)));

    /*
     * If we advanced the receive queue  based on expired messages in RCV window,
     * it's possible that RCV.CUR < RCV.LCS.
     * This can happen when a message has been only partially received
     * and it's TTL has expired before we had a chance to assemble a complete message.
     * We need to update left edge of ACK window to match LCS.
     */
    if (SEQ32_LT(conn->rcv.CUR, conn->rcv.LCS)) {
        conn->rcv.CUR = conn->rcv.LCS;
        QCC_DbgPrintf(("ReleaseRcvBuffers: new conn->rcv.CUR=%u", conn->rcv.CUR));
    }

    /* Schedule "unsolicited" ACK if the ACK timer is not running. */
    if (conn->ackTimer.retry == 0) {
        QCC_DbgHLPrintf(("ReleaseRcvBuffers: Schedule ACK timer to inform about new values of rcv.CUR %u and rcv.LCS %u",
                         conn->rcv.CUR, conn->rcv.LCS));
        UpdateTimer(handle, conn, &conn->ackTimer, handle->config.delayedAckTimeout, 1);
    }

    return ER_OK;
}

static void AdvanceRcvQueue(ArdpHandle* handle, ArdpConnRecord* conn, ArdpRcvBuf* current)
{
    uint32_t seq = current->seq;
    bool isExpiring = false;
    ArdpRcvBuf* startFrag;
    ArdpRcvBuf* fragment;
    uint32_t startSeq;
    uint16_t fcnt;
    uint16_t count;

    do {
        uint32_t tNow = TimeNow(handle->tbase);
        QCC_DbgPrintf(("AdvanceRcvQueue: segment %u (%p)", seq, current));
        assert((seq - conn->rcv.LCS) <= conn->rcv.SEGMAX);

        ShiftRcvMsk(conn);

        if (isExpiring) {
            /* Check if last fragment. If this is the case, release the associated buffers */
            if (seq == (startSeq + fcnt - 1)) {
                ReleaseRcvBuffers(handle, conn, startSeq, fcnt, ER_ARDP_TTL_EXPIRED);
                isExpiring = false;
            }

        } else if (current->ttl != ARDP_TTL_INFINITE && (tNow - current->tRecv >= current->ttl)) {
            QCC_DbgPrintf(("AdvanceRcvQueue(): Detected expired fragment %u (start %u, fcnt %u)",
                           current->seq, current->som, current->fcnt));
#if ARDP_STATS
            ++handle->stats.inboundDrops;
#endif
            startFrag = &conn->rcv.buf[current->som % conn->rcv.SEGMAX];
            startSeq = current->som;
            fcnt = current->fcnt;
            fragment = startFrag;
            count = 0;

            /* Mark all the fragments in the message as expired. */
            do {
                fragment->ttl = ARDP_TTL_EXPIRED;
                fragment = fragment->next;
                count++;
            } while (count < fcnt);

            /*
             * If this is the complete message, flush the message and release the associated buffers.
             * Else, iterate till we get to the last fragment.
             */
            if (seq == (current->som + current->fcnt - 1)) {
                ReleaseRcvBuffers(handle, conn, current->som, current->fcnt, ER_ARDP_TTL_EXPIRED);
                isExpiring = false;
            } else {
                isExpiring = true;
            }

        } else {
            isExpiring = false;

            /* Check if last fragment. If this is the case, re-assemble the message */
            if (current->seq == current->som + (current->fcnt - 1)) {
                startFrag = &conn->rcv.buf[current->som % conn->rcv.SEGMAX];
                fragment = startFrag;

                QCC_DbgPrintf(("AdvanceRcvQueue(): RecvCb(conn=0x%p, buf=%p, seq=%u, fcnt (@ %p)=%d)", conn, startFrag, startFrag->seq, &(startFrag->fcnt), startFrag->fcnt));
#if ARDP_STATS
                ++handle->stats.recvCbs;
#endif
                handle->cb.RecvCb(handle, conn, startFrag, ER_OK);

                /* Mark all the fragments as delivered, and while we're at it note */
                for (uint32_t i = 0; i < current->fcnt; i++) {
                    fragment->flags |= ARDP_BUFFER_DELIVERED;
                    fragment = fragment->next;
                }
            }
        }

        current = current->next;
        seq++;

    } while (((current->flags & ARDP_BUFFER_IN_USE) && !(current->flags & ARDP_BUFFER_DELIVERED)) || isExpiring);

    if (SEQ32_LT(conn->rcv.CUR, seq - 1)) {
        conn->rcv.CUR  = seq - 1;
    }

    if (conn->ackTimer.retry == 0) {
        QCC_DbgHLPrintf(("AdvanceReleaseQueue: Schedule ACK timer to inform about new values of rcv.CUR %u and rcv.LCS %u",
                         conn->rcv.CUR, conn->rcv.LCS));
        UpdateTimer(handle, conn, &conn->ackTimer, handle->config.delayedAckTimeout, 1);
    }

    QCC_DbgHLPrintf(("AdvanceRcvQueue: rcv.CUR = %u, rcv.LCS = %u", conn->rcv.CUR, conn->rcv.LCS));
}

static void FlushExpiredRcvMessages(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t acknxt)
{
    uint16_t index = (conn->rcv.CUR) % conn->rcv.SEGMAX;
    ArdpRcvBuf* current = &conn->rcv.buf[index];
    uint32_t startSeq;
    uint32_t seq;
    uint32_t delta;
    ArdpRcvBuf* start;
    QCC_DbgTrace(("FlushExpiredRcvMessages(handle=%p, conn=%p, acknxt=%u", handle, conn, acknxt));
    QCC_DbgPrintf(("FlushExpiredRcvMessages(acknxt=%u, lcs=%u, cur=%u", acknxt, conn->rcv.LCS, conn->rcv.CUR));

    /* Move to the start of the message */
    if (!(current->flags & ARDP_BUFFER_IN_USE) || (current->seq == (current->som + current->fcnt - 1))) {
        /* All the fragments of the last message were received or the buffer is empty (expired) */
        start = current->next;
        startSeq = conn->rcv.CUR + 1;
        delta = 0;

        /* If this is the tail of RCV queue, just update the counter */
        if ((conn->rcv.eack.sz == 0) && (conn->rcv.LCS == conn->rcv.CUR)) {
            conn->rcv.LCS = acknxt - 1;
            conn->rcv.CUR = acknxt - 1;
            if (conn->ackTimer.retry == 0) {
                QCC_DbgHLPrintf(("FlushExpiredRcvMessages: Schedule ACK timer to inform about new values of rcv.CUR %u and rcv.LCS %u",
                                 conn->rcv.CUR, conn->rcv.LCS));
                UpdateTimer(handle, conn, &conn->ackTimer, handle->config.delayedAckTimeout, 1);
            }
            return;
        }

    } else {
        /* Message was partially received. Move to the beginning of incomplete message */
        start = &conn->rcv.buf[current->som % conn->rcv.SEGMAX];
        delta = (conn->rcv.CUR + 1) - current->som;
        startSeq = current->som;
    }

    current = start;
    seq = startSeq;

    /* Mark all the segments with sequence numbers up to ACKNXT as expired */
    do {
        assert(!(current->flags & ARDP_BUFFER_DELIVERED));
        QCC_DbgPrintf(("FlushExpiredRcvMessages: seq = %u", seq));

        current->ttl = ARDP_TTL_EXPIRED;

        /* Update EACK mask */
        if (delta == 0) {
            ShiftRcvMsk(conn);
        } else {
            delta--;
        }
        seq++;
        current = current->next;
    } while (SEQ32_LT(seq, acknxt));

    /*
     * Update counter for the current ACK counter to ACKNXT - 1.
     */
    if (SEQ32_LT(conn->rcv.CUR, acknxt - 1)) {
        conn->rcv.CUR = acknxt - 1;
    }

    /* Release the buffers associated with the expired message(s) */
    ReleaseRcvBuffers(handle, conn, startSeq, seq - startSeq, ER_ARDP_TTL_EXPIRED);

    /* If there are pending complete messages, deliver them to transport layer */
    if ((current->flags & ARDP_BUFFER_IN_USE) && !(current->flags & ARDP_BUFFER_DELIVERED)) {
        QCC_DbgPrintf(("FlushExpiredRcvMessages: advance seq %u", current->seq));
        AdvanceRcvQueue(handle, conn, current);
    }

    if (conn->ackTimer.retry == 0) {
        QCC_DbgHLPrintf(("FlushExpiredRcvMessages: Schedule ACK timer to inform about new values of rcv.CUR %u and rcv.LCS %u",
                         conn->rcv.CUR, conn->rcv.LCS));
        UpdateTimer(handle, conn, &conn->ackTimer, handle->config.delayedAckTimeout, 1);
    }

    QCC_DbgPrintf(("FlushExpiredRcvMessages(UPDATE rcv.CUR = %u, rcv.LCS = %u, acknxt = %u", conn->rcv.CUR, conn->rcv.LCS, acknxt));
}

static QStatus AddRcvBuffer(ArdpHandle* handle, ArdpConnRecord* conn, ArdpSeg* seg, uint8_t* buf, uint16_t len, bool ordered)
{
    uint16_t index = seg->SEQ % conn->rcv.SEGMAX;
    ArdpRcvBuf* current = &conn->rcv.buf[index];

    QCC_DbgTrace(("AddRcvBuffer(handle=%p, conn=%p, seg=%p, buf=%p, len=%d, ordered=%s", handle, conn, seg, buf, len, (ordered ? "true" : "false")));

    QCC_DbgPrintf(("AddRcvBuffer: seg->SEQ = %u, first=%u", seg->SEQ, conn->rcv.LCS + 1));

    assert(buf != NULL && len != 0);

    if (current->seq == seg->SEQ) {
        QCC_DbgPrintf(("AddRcvBuffer: duplicate segment %u, acknowledge", seg->SEQ));
        return ER_OK;
    }

    if (current->flags & ARDP_BUFFER_IN_USE) {
        QCC_DbgHLPrintf(("AddRcvBuffer: attempt to overwrite buffer that has not been released( in use %u seg->seq %u)",
                         current->seq, seg->SEQ));
        assert(!(current->flags & ARDP_BUFFER_IN_USE) &&
               "AddRcvBuffer: attempt to overwrite buffer that has not been released");
        return ER_FAIL;
    }

    /* Allocate holding buffer, pad up to 1K */
    current->data = (uint8_t*) malloc((((seg->DLEN + 1023) >> 10) << 10) * sizeof(uint8_t));
    if (current->data == NULL) {
        QCC_LogError(ER_OUT_OF_MEMORY, ("Failed to allocate rcv data buffer"));
        return ER_OUT_OF_MEMORY;
    }

    current->seq = seg->SEQ;
    current->datalen = seg->DLEN;
    current->flags = ARDP_BUFFER_IN_USE;
    memcpy(current->data, buf + (seg->HLEN * 2), seg->DLEN);

    current->fcnt = seg->FCNT;
    current->som = seg->SOM;

    /*
     * Get the remaining time to live for this message and mark when we received
     * it.
     */
    current->ttl = seg->TTL;
    current->tRecv = TimeNow(handle->tbase);

    /* Deliver this segment (and following out-of-order segments) to the upper layer. */
    if (ordered) {
        AdvanceRcvQueue(handle, conn, current);
    } else {
        AddRcvMsk(conn, (seg->SEQ - (conn->rcv.CUR + 1)));
    }

    return ER_OK;
}

static bool CheckConfigValid(uint16_t segmax, uint16_t segbmax, uint16_t window)
{
    uint32_t ackMaskSize = (window + 31) >> 5;
    uint8_t hlen = ARDP_FIXED_HEADER_LEN + ackMaskSize * sizeof(uint32_t);
    uint32_t maxPayload = segbmax - (UDP_HEADER_SIZE + hlen);

    if (segmax > ARDP_MAX_WINDOW_SIZE) {
        QCC_LogError(ER_INVALID_CONFIG, ("SEGMAX %u exceeds ARDP maximum window size %u", segmax, ARDP_MAX_WINDOW_SIZE));
        return false;
    }

    if (segbmax <= (UDP_HEADER_SIZE + hlen)) {
        QCC_LogError(ER_FAIL, ("InitSnd(): SEGBMAX too small %u (need at least %u)", segbmax, (UDP_HEADER_SIZE + hlen)));
        return false;
    }

    if ((maxPayload * segmax) < (uint32_t) (ALLJOYN_MAX_PACKET_LEN)) {
        QCC_LogError(ER_INVALID_CONFIG, ("SEGMAX %u and SEGBMAX %u cannot fit max Alljoyn message %u", segmax, segbmax, ALLJOYN_MAX_PACKET_LEN));
        return false;
    }

    return true;
}

static QStatus InitSnd(ArdpHandle* handle, ArdpConnRecord* conn)
{
    uint8_t* buffer;
    uint32_t ackMaskSize = (conn->rcv.SEGMAX + 31) >> 5;
    uint8_t hlen = ARDP_FIXED_HEADER_LEN + ackMaskSize * sizeof(uint32_t);

    conn->rcv.eack.fixedSz = ackMaskSize * sizeof(uint32_t);
    conn->snd.maxDlen = conn->snd.SEGBMAX - (UDP_HEADER_SIZE + hlen);
    QCC_DbgPrintf(("InitSnd(): actual max payload len %d", conn->snd.maxDlen));

    if (!CheckConfigValid(conn->snd.SEGMAX, conn->snd.SEGBMAX, conn->rcv.SEGMAX)) {
        return ER_FAIL;
    }

    conn->snd.buf = (ArdpSndBuf*) malloc(conn->snd.SEGMAX * sizeof(ArdpSndBuf));
    if (conn->snd.buf == NULL) {
        QCC_DbgPrintf(("InitSnd(): Failed to allocate send buffer info"));
        return ER_OUT_OF_MEMORY;
    }
    memset(conn->snd.buf, 0, conn->snd.SEGMAX * sizeof(ArdpSndBuf));

    /* Allocate contiguous space for sent data segment headers */
    buffer = (uint8_t*) malloc(conn->snd.SEGMAX * ARDP_FIXED_HEADER_LEN);

    if (buffer == NULL) {
        QCC_DbgPrintf(("InitSnd(): Failed to allocate send buffer headers"));
        free(conn->snd.buf);
        return ER_OUT_OF_MEMORY;
    }
    memset(buffer, 0, conn->snd.SEGMAX * ARDP_FIXED_HEADER_LEN);

    /* Array of pointers to headers of unAcked sent data buffers */
    for (uint32_t i = 0; i < conn->snd.SEGMAX; i++) {
        InitTimer(handle, conn, &conn->snd.buf[i].timer, RetransmitTimerHandler, &conn->snd.buf[i], handle->config.initialDataTimeout, 0);
        conn->snd.buf[i].next = &conn->snd.buf[(i + 1) % conn->snd.SEGMAX];
        conn->snd.buf[i].hdr = buffer;
        buffer += ARDP_FIXED_HEADER_LEN;
    }

    /* Calculate the minimum send window necessary to accomodate the largest message */
    conn->minSendWindow = (ALLJOYN_MAX_PACKET_LEN + (conn->snd.maxDlen - 1)) / conn->snd.maxDlen;
    QCC_DbgPrintf(("InitSnd(): minSendWindow=%d", conn->minSendWindow));
    return ER_OK;
}

static void ArdpMachine(ArdpHandle* handle, ArdpConnRecord* conn, ArdpSeg* seg, uint8_t* buf, uint16_t len)
{
    QStatus status;

    QCC_DbgTrace(("ArdpMachine(handle=%p, conn=%p, seg=%p, buf=%p, len=%d)", handle, conn, seg, buf, len));

    switch (conn->state) {
    case CLOSED:
        {
            QCC_DbgPrintf(("ArdpMachine(): conn->state = CLOSED"));

            if (seg->FLG & ARDP_FLAG_RST) {
#if ARDP_STATS
                ++handle->stats.rstRecvs;
#endif
                QCC_DbgPrintf(("ArdpMachine(): CLOSED: RST on a closed connection"));
                break;
            }

            if (seg->FLG & ARDP_FLAG_ACK || seg->FLG & ARDP_FLAG_NUL) {
                QCC_DbgPrintf(("ArdpMachine(): CLOSED: Probe or ACK on a closed connection"));
#if ARDP_STATS
                if (seg->FLG & ARDP_FLAG_NUL) {
                    ++handle->stats.nulRecvs;
                }
#endif
                /*
                 * <SEQ=SEG>ACK + 1><RST>
                 */
#if ARDP_STATS
                ++handle->stats.rstSends;
#endif
                Send(handle, conn, ARDP_FLAG_RST | ARDP_FLAG_VER, seg->ACK + 1, 0);
                break;
            }

            QCC_DbgPrintf(("ArdpMachine(): CLOSED: Unexpected segment on a closed connection"));
            /*
             * <SEQ=0><RST><ACK=RCV.CUR><ACK>
             */
#if ARDP_STATS
            ++handle->stats.rstSends;
#endif
            Send(handle, conn, ARDP_FLAG_RST | ARDP_FLAG_ACK | ARDP_FLAG_VER, 0, seg->SEQ);
            break;
        }

    case LISTEN:
        {
            QCC_DbgPrintf(("ArdpMachine(): conn->state = LISTEN"));

            if (seg->FLG & ARDP_FLAG_RST) {
#if ARDP_STATS
                ++handle->stats.rstRecvs;
#endif
                QCC_DbgPrintf(("ArdpMachine(): LISTEN: RST on a LISTENing connection"));
                break;
            }

            if (seg->FLG & ARDP_FLAG_ACK || seg->FLG & ARDP_FLAG_NUL) {
                QCC_DbgPrintf(("ArdpMachine(): LISTEN: Foreign host ACKing a Listening connection"));
#if ARDP_STATS
                if (seg->FLG & ARDP_FLAG_NUL) {
                    ++handle->stats.nulRecvs;
                }
#endif
                /*
                 * <SEQ=SEG.ACK + 1><RST>
                 */
#if ARDP_STATS
                ++handle->stats.rstSends;
#endif
                Send(handle, conn, ARDP_FLAG_RST | ARDP_FLAG_VER, seg->ACK + 1, 0);
                Disconnect(handle, conn, ER_ARDP_INVALID_RESPONSE);
                break;
            }

            if (seg->FLG & ARDP_FLAG_SYN) {
                QCC_DbgPrintf(("ArdpMachine(): LISTEN: SYN received.  Accepting"));
#if ARDP_STATS
                ++handle->stats.synRecvs;
#endif

                UnmarshalSynSegment(conn, buf, seg);

                QCC_DbgPrintf(("ArdpMachine(): LISTEN: SYN received: the other side can receive max %d bytes", conn->snd.SEGBMAX));
                if (handle->cb.AcceptCb != NULL) {
                    uint8_t* data = buf + ARDP_SYN_HEADER_SIZE;
#if ARDP_STATS
                    ++handle->stats.acceptCbs;
#endif

                    if (handle->cb.AcceptCb(handle, conn->ipAddr, conn->ipPort, conn, data, seg->DLEN, ER_OK) == false) {
                        QCC_DbgPrintf(("ArdpMachine(): LISTEN: SYN received. AcceptCb() returned \"false\""));
#if ARDP_STATS
                        ++handle->stats.rstSends;
#endif
                        Send(handle, conn, ARDP_FLAG_RST | ARDP_FLAG_VER, seg->ACK + 1, seg->SEQ);
                        SetState(conn, CLOSED);
                        DelConnRecord(handle, conn, false);
                    }
                }
                break;
            }
            break;
        }

    case SYN_SENT:
        {
            QCC_DbgPrintf(("ArdpMachine(): conn->state = SYN_SENT"));
            status = ER_OK;

            if (seg->FLG & ARDP_FLAG_RST) {
                QCC_DbgHLPrintf(("ArdpMachine(): SYN_SENT: connection refused. state -> CLOSED"));
                /*
                 * If detected that ARDP versions do not match (and are within valid range),
                 * an educated guess would be that this was the reason for the remote side sending RST.
                 * In future, when more than one version of ARDP exists, the protocol will
                 * make an attempt to match the version indicated by the remote provided the version
                 * is locally supported.
                 */
                if ((seg->FLG  & ARDP_VERSION_BITS) != ARDP_FLAG_VER) {
                    QCC_DbgHLPrintf(("ArdpMachine(): SYN_SENT: Detected unsupported protocol version 0x%x",
                                     seg->FLG & ARDP_VERSION_BITS));
                }

#if ARDP_STATS
                ++handle->stats.rstRecvs;
#endif
                status = ER_ARDP_REMOTE_CONNECTION_RESET;
            } else if (seg->FLG & ARDP_FLAG_SYN) {
                QCC_DbgPrintf(("ArdpMachine(): SYN_SENT: SYN received"));
#if ARDP_STATS
                ++handle->stats.synRecvs;
#endif

                UnmarshalSynSegment(conn, buf, seg);

                status = InitSnd(handle, conn);

                assert(status == ER_OK && "ArdpMachine():SYN_SENT: Failed to initialize Send queue");

                if (seg->FLG & ARDP_FLAG_ACK) {
                    if ((seg->FLG  & ARDP_VERSION_BITS) != ARDP_FLAG_VER) {

                        QCC_DbgHLPrintf(("ArdpMachine(): SYN_SENT: Unsupported protocol version 0x%x",
                                         seg->FLG & ARDP_VERSION_BITS));
                        status = ER_ARDP_VERSION_NOT_SUPPORTED;

                    } else if (seg->ACK != conn->snd.ISS) {

                        QCC_DbgPrintf(("ArdpMachine(): SYN_SENT: ACK does not ASK ISS"));
                        status = ER_ARDP_INVALID_RESPONSE;

                    } else {
                        QCC_DbgPrintf(("ArdpMachine(): SYN_SENT: SYN | ACK received. state -> OPEN"));
#if ARDP_STATS
                        ++handle->stats.synackRecvs;
#endif

                        conn->snd.UNA = seg->ACK + 1;
                        PostInitRcv(conn);
                        SetState(conn, OPEN);

                        /* Stop connect retry timer */
                        conn->connectTimer.retry = 0;

                        /* Initialize and kick off link timeout timer */
                        conn->lastSeen = TimeNow(handle->tbase);
                        InitTimer(handle, conn, &conn->probeTimer, ProbeTimerHandler, &conn->lastSeen,
                                  handle->config.linkTimeout / handle->config.keepaliveRetries, handle->config.keepaliveRetries);

                        /* Initialize persist (dead window) timer */
                        InitTimer(handle, conn, &conn->persistTimer, PersistTimerHandler, NULL, handle->config.persistInterval, 0);

                        /* Initialize delayed ACK timer */
                        InitTimer(handle, conn, &conn->ackTimer, AckTimerHandler, NULL, handle->config.delayedAckTimeout, 0);

#if ARDP_STATS
                        ++handle->stats.synackackSends;
#endif
                        /*
                         * <SEQ=snd.NXT><ACK=RCV.CUR><ACK>
                         */
                        status = Send(handle, conn, ARDP_FLAG_ACK | ARDP_FLAG_VER, conn->snd.NXT, conn->rcv.CUR);

                        if (handle->cb.ConnectCb) {
                            QCC_DbgPrintf(("ArdpMachine(): SYN_SENT->OPEN: ConnectCb(handle=%p, conn=%p", handle, conn));
                            assert(!conn->passive);
                            uint8_t* data = &buf[ARDP_SYN_HEADER_SIZE];
#if ARDP_STATS
                            ++handle->stats.connectCbs;
#endif
                            handle->cb.ConnectCb(handle, conn, false, data, seg->DLEN, ER_OK);
                            if (conn->synData.buf != NULL) {
                                free(conn->synData.buf);
                                conn->synData.buf = NULL;
                            }
                        }
                    }

                } else {
                    QCC_DbgPrintf(("ArdpMachine(): SYN_SENT: SYN with no ACK implies simulateous connection attempt: state -> SYN_RCVD"));
                    uint8_t* data = &buf[ARDP_SYN_HEADER_SIZE];
                    if (handle->cb.AcceptCb != NULL) {
                        status = ER_OK;
#if ARDP_STATS
                        ++handle->stats.acceptCbs;
#endif
                        handle->cb.AcceptCb(handle, conn->ipAddr, conn->ipPort, conn, data, seg->DLEN, ER_OK);
                    } else {
                        status = ER_ARDP_INVALID_STATE;
                    }
                }
            }

            if (status != ER_OK && status != ER_WOULDBLOCK) {
                SetState(conn, CLOSED);

                /* Stop connect retry timer */
                conn->connectTimer.retry = 0;

                handle->cb.ConnectCb(handle, conn, false, NULL, 0, status);
                /*
                 * Do not delete conection record here:
                 * The upper layer will detect an error status and call ARDP_ReleaseConnection()
                 */
            }

            break;
        }

    case SYN_RCVD:
        {
            QCC_DbgPrintf(("ArdpMachine(): conn->state = SYN_RCVD"));

            if (seg->FLG & ARDP_FLAG_RST) {
#if ARDP_STATS
                ++handle->stats.rstRecvs;
#endif
                QCC_DbgPrintf(("ArdpMachine(): SYN_RCVD: Got RST during passive open"));
                Disconnect(handle, conn, ER_ARDP_REMOTE_CONNECTION_RESET);
                break;
            }

            if (IN_RANGE(uint32_t, conn->rcv.CUR + 1, conn->rcv.SEGMAX, seg->SEQ) == false) {
                QCC_DbgPrintf(("ArdpMachine(): SYN_RCVD: unacceptable sequence %u", seg->SEQ));
                /* <SEQ=snd.NXT><ACK=RCV.CUR><ACK> */
                Send(handle, conn, ARDP_FLAG_ACK | ARDP_FLAG_VER, conn->snd.NXT, conn->rcv.CUR);
                break;
            }

            if (seg->FLG & ARDP_FLAG_SYN) {
#if ARDP_STATS
                ++handle->stats.synRecvs;
#endif

                QCC_DbgPrintf(("ArdpMachine(): SYN_RCVD: Got SYN, state -> CLOSED"));
                Disconnect(handle, conn, ER_ARDP_INVALID_RESPONSE);
                break;
            }

            if (seg->FLG & ARDP_FLAG_EACK) {
                QCC_DbgPrintf(("ArdpMachine(): SYN_RCVD: Got EACK. Send RST"));
                Disconnect(handle, conn, ER_ARDP_INVALID_RESPONSE);
                break;
            }

            if (seg->FLG & ARDP_FLAG_ACK) {
                if (seg->ACK == conn->snd.ISS) {

                    QCC_DbgPrintf(("ArdpMachine(): SYN_RCVD: Got ACK with correct acknowledge.  state -> OPEN"));
#if ARDP_STATS
                    ++handle->stats.synackackRecvs;
#endif

                    conn->snd.UNA = seg->ACK + 1;
                    PostInitRcv(conn);
                    SetState(conn, OPEN);

                    /* Stop connect retry timer */
                    conn->connectTimer.retry = 0;

                    /* Initialize and kick off link timeout timer */
                    conn->lastSeen = TimeNow(handle->tbase);
                    InitTimer(handle, conn, &conn->probeTimer, ProbeTimerHandler, &conn->lastSeen,
                              handle->config.linkTimeout / handle->config.keepaliveRetries,
                              handle->config.keepaliveRetries);

                    /* Initialize persist (dead window) timer */
                    InitTimer(handle, conn, &conn->persistTimer, PersistTimerHandler, NULL, handle->config.persistInterval, 0);


                    /* Initialize delayed ACK timer */
                    InitTimer(handle, conn, &conn->ackTimer, AckTimerHandler, NULL, handle->config.delayedAckTimeout, 0);

                    if (seg->FLG & ARDP_FLAG_NUL) {
#if ARDP_STATS
                        ++handle->stats.nulRecvs;
                        ++handle->stats.nulSends;
#endif
                        Send(handle, conn, ARDP_FLAG_ACK | ARDP_FLAG_VER, conn->snd.NXT, conn->rcv.CUR);
                    }

                    if (handle->cb.ConnectCb) {
                        QCC_DbgPrintf(("ArdpMachine(): SYN_RCVD->OPEN: ConnectCb(handle=%p, conn=%p", handle, conn));
                        assert(conn->passive);
                        uint8_t* data = &buf[ARDP_SYN_HEADER_SIZE];
#if ARDP_STATS
                        ++handle->stats.connectCbs;
#endif
                        handle->cb.ConnectCb(handle, conn, true, data, seg->DLEN, ER_OK);
                        if (conn->synData.buf != NULL) {
                            free(conn->synData.buf);
                            conn->synData.buf = NULL;
                        }
                    }

                    break;

                } else {
                    /* <SEQ=SEG.ACK + 1><RST> */
                    QCC_DbgPrintf(("ArdpMachine(): SYN_RCVD: Got ACK with incorrect acknowledge %u. RST", seg->ACK));
                    Disconnect(handle, conn, ER_ARDP_INVALID_RESPONSE);
                    break;
                }
            } else {
                QCC_DbgPrintf(("ArdpMachine(): SYN_RCVD: Got datagram with no ACK"));
                break;
            }
            break;
        }

    case OPEN:
        {
            QCC_DbgPrintf(("ArdpMachine(): conn->state = OPEN"));
            uint32_t validWindow = (seg->DLEN != 0) ? conn->rcv.SEGMAX : (conn->rcv.SEGMAX + 1);
            bool isDuplicate = false;

            if (seg->FLG & ARDP_FLAG_RST) {
#if ARDP_STATS
                ++handle->stats.rstRecvs;
#endif
                QCC_DbgPrintf(("ArdpMachine(): OPEN: got RST, disconnect"));
                Disconnect(handle, conn, ER_ARDP_REMOTE_CONNECTION_RESET);
                break;
            }

            if (IN_RANGE(uint32_t, conn->rcv.LCS + 1, validWindow, seg->SEQ) == false) {
                if (seg->DLEN != 0) {

                    /* Check if data segment is a duplicate. Did the remote side miss our ACK? */
                    if ((IN_RANGE(uint32_t, conn->rcv.CUR + 1 - conn->rcv.SEGMAX, conn->rcv.SEGMAX, seg->SEQ) == true)) {
                        /* This is a duplicate data segment */
                        isDuplicate = true;
                        QCC_DbgHLPrintf(("ArdpMachine(): OPEN: duplicate data segment %u, conn->rcv.CUR + 1 = %u, conn->rcv.LCS + 1 = %u",
                                         seg->SEQ, conn->rcv.CUR + 1, conn->rcv.LCS + 1));
                    }
                }
                /* Bad bad bad remote! Disconnect. */
                if (!isDuplicate) {
                    QCC_LogError(ER_ARDP_INVALID_RESPONSE, ("ArdpMachine(): OPEN: unacceptable sequence %u, conn->rcv.CUR + 1 = %u, conn->rcv.LCS + 1 = %u, MAX = %d", seg->SEQ, conn->rcv.CUR + 1, conn->rcv.LCS + 1, conn->rcv.SEGMAX));
                    Disconnect(handle, conn, ER_ARDP_INVALID_RESPONSE);
                    break;
                }
            }

            if (seg->FLG & ARDP_FLAG_SYN) {
#if ARDP_STATS
                ++handle->stats.synRecvs;
#endif
                /* Did the remote side miss our ACK of SYN-ACK? */
                if (isDuplicate && !conn->passive && ((seg->FLG & ARDP_FLAG_ACK) && (seg->ACK == conn->snd.ISS))) {
                    QCC_DbgPrintf(("ArdpMachine(): OPEN: Got duplicate SYN-ACK, acknowledge"));
                    Send(handle, conn, ARDP_FLAG_ACK | ARDP_FLAG_VER, conn->snd.NXT, conn->rcv.CUR);
                } else {
                    QCC_DbgPrintf(("ArdpMachine(): OPEN: Got unexpected SYN, disconnect"));
                    Disconnect(handle, conn, ER_ARDP_INVALID_RESPONSE);
                }
                break;
            }

            if (SEQ32_LT(conn->rcv.CUR + 1, seg->ACKNXT)) {
                QCC_DbgPrintf(("ArdpMachine(): OPEN: FlushExpiredRcvMessages: seq = %u, expected %u got %u",
                               seg->SEQ, conn->rcv.CUR + 1, seg->ACKNXT));
                FlushExpiredRcvMessages(handle, conn, seg->ACKNXT);
            }

            if (seg->FLG & ARDP_FLAG_ACK) {
                QCC_DbgHLPrintf(("ArdpMachine(): OPEN: Got ACK %u LCS %u Window %u", seg->ACK, seg->LCS, seg->WINDOW));
                bool needUpdate = false;

                if ((IN_RANGE(uint32_t, conn->snd.UNA, ((conn->snd.NXT - conn->snd.UNA) + 1), seg->ACK) == true) ||
                    (conn->snd.LCS != seg->LCS)) {
                    conn->snd.UNA = seg->ACK + 1;
                    needUpdate = true;
                }

                if (seg->FLG & ARDP_FLAG_EACK) {
                    uint32_t* eackBuf = reinterpret_cast<uint32_t*>(buf + ARDP_FIXED_HEADER_LEN);
                    /*
                     * Flush the segments using EACK
                     */
                    QCC_DbgPrintf(("ArdpMachine(): OPEN: EACK is set"));
                    CancelEackedSegments(handle, conn, seg->ACK, eackBuf);
                }

                if (needUpdate) {
                    status = UpdateSndSegments(handle, conn, seg->ACK, seg->LCS);
                    if (status != ER_OK) {
                        Disconnect(handle, conn, status);
                        break;
                    }
                }
            }

            /* If we got NUL segment, send ACK without delay */
            if (seg->FLG & ARDP_FLAG_NUL) {
                QCC_DbgPrintf(("ArdpMachine(): OPEN: got NUL, send LCS %u", conn->rcv.LCS));
#if ARDP_STATS
                ++handle->stats.nulRecvs;
#endif
                status = Send(handle, conn, ARDP_FLAG_ACK | ARDP_FLAG_VER, conn->snd.NXT, conn->rcv.CUR);

                /* If socket was busy, re-schedule ACK timer immediately. */
                if (status == ER_WOULDBLOCK) {
                    UpdateTimer(handle, conn, &conn->ackTimer, 0, 1);
                }

            } else if (seg->DLEN) {
                isDuplicate = (SEQ32_LT(seg->SEQ, conn->rcv.CUR + 1)) ? true : isDuplicate;
                QCC_DbgHLPrintf(("ArdpMachine(): OPEN: Got %d bytes of Data with SEQ %u, rcv.CUR = %u (%s)).", seg->DLEN, seg->SEQ, conn->rcv.CUR, isDuplicate ? "duplicate" : "new"));

                status = ER_OK;

                /*
                 * Update RCV buffers if the segment is not a duplicate.
                 */
                if (!isDuplicate) {
                    status = AddRcvBuffer(handle, conn, seg, buf, len, seg->SEQ == (conn->rcv.CUR + 1));

                    if (status != ER_OK) {
                        Disconnect(handle, conn, status);
                        break;
                    }
                }

                conn->ackPending++;
                /* Schedule ACK timer if it's not running already */
                if (conn->ackTimer.retry == 0) {
                    UpdateTimer(handle, conn, &conn->ackTimer, handle->config.delayedAckTimeout, 1);
                    QCC_DbgHLPrintf(("ArdpMachine():schedule ackTimer @ %u", conn->ackTimer.when));
                } else if (conn->ackPending >= (conn->rcv.SEGMAX >> 2)) {
                    QCC_DbgHLPrintf(("ArdpMachine():accumulated %d segments, send urgent ACK", conn->ackPending));
                    AckTimerHandler(handle, conn, NULL);
                }
            }

            if (conn->window != seg->WINDOW) {
                /* Schedule persist timer only if there are no pending retransmits */
                if (!IsDataRetransmitScheduled(conn) && (seg->WINDOW < conn->minSendWindow) && (conn->persistTimer.retry == 0)) {
                    /* Start Persist Timer */
                    UpdateTimer(handle, conn, &conn->persistTimer, handle->config.persistInterval,
                                handle->config.totalAppTimeout / handle->config.persistInterval + 1);
                } else if ((conn->persistTimer.retry != 0) && ((seg->WINDOW >= conn->minSendWindow) || IsDataRetransmitScheduled(conn))) {
                    /* Cancel Persist Timer */
                    conn->persistTimer.retry = 0;
                }

                conn->window = seg->WINDOW;
                if (handle->cb.SendWindowCb != NULL) {
                    handle->cb.SendWindowCb(handle, conn, conn->window, (conn->window != 0) ? ER_OK : ER_ARDP_BACKPRESSURE);
                }
            }

            break;
        }

    case CLOSE_WAIT:
        {
            QCC_DbgPrintf(("ArdpMachine(): conn->state = CLOSE_WAIT"));
            /*
             * Ignore segment with set RST.
             * The transition to CLOSED is based on delay TIMWAIT only.
             */
            break;
        }

    default:
        assert(0 && "ArdpMachine(): unexpected conn->state %d");
        break;
    }

}

QStatus ARDP_StartPassive(ArdpHandle* handle)
{
    QCC_DbgTrace(("ARDP_StartPassive(handle=%p)", handle));
    handle->accepting = true;
    return ER_OK;
}

QStatus ARDP_Connect(ArdpHandle* handle, qcc::SocketFd sock, qcc::IPAddress ipAddr, uint16_t ipPort, uint16_t segmax, uint16_t segbmax, ArdpConnRecord**pConn, uint8_t* buf, uint16_t len, void* context)
{
    QCC_DbgTrace(("ARDP_Connect(handle=%p, sock=%d, ipAddr=\"%s\", ipPort=%d, segmax=%d, segbmax=%d, pConn=%p, buf=%p, len=%d, context=%p)",
                  handle, sock, ipAddr.ToString().c_str(), ipPort, segmax, segbmax, pConn, buf, len, context));

    ArdpConnRecord* conn;
    QStatus status;

    *pConn = NULL;

    if (!CheckConfigValid(segmax, segbmax, ARDP_MAX_WINDOW_SIZE)) {
        return ER_INVALID_CONFIG;
    }

    conn = NewConnRecord();

    status = InitConnRecord(handle, conn, sock, ipAddr, ipPort, 0);
    if (status != ER_OK) {
        delete conn;
        return status;
    }

    /* Initialize the receiver side of the connection. */
    status = InitRcv(conn, segmax, segbmax);
    if (status == ER_OK) {
        conn->context = context;
        conn->passive = false;
        EnList(handle->conns.bwd, (ListNode*)conn);
        status = SendSyn(handle, conn, buf, len);
    }

    if (status != ER_OK) {
        DelConnRecord(handle, conn, false);
    } else {
        SetState(conn, SYN_SENT);
        *pConn = conn;
    }


    return status;
}

QStatus ARDP_Accept(ArdpHandle* handle, ArdpConnRecord* conn, uint16_t segmax, uint16_t segbmax, uint8_t* buf, uint16_t len)
{
    QCC_DbgTrace(("ARDP_Accept(handle=%p, conn=%p, segmax=%d, segbmax=%d, buf=%p (%s), len=%d)",
                  handle, conn, segmax, segbmax, buf, buf, len));
    QStatus status = ER_OK;
    if (!IsConnValid(handle, conn)) {
        return ER_ARDP_INVALID_CONNECTION;
    }

    if (!CheckConfigValid(segmax, segbmax, ARDP_MAX_WINDOW_SIZE)) {
        status = ER_INVALID_CONFIG;
    }

    if (status == ER_OK) {
        /* Initialize Receive side of the connection */
        status = InitRcv(conn, segmax, segbmax);
    }

    if (status == ER_OK) {
        /* Initialize Send side of the connection */
        status = InitSnd(handle, conn);
    }

    if (status != ER_OK) {
#if ARDP_STATS
        ++handle->stats.rstSends;
#endif
        Send(handle, conn, ARDP_FLAG_RST | ARDP_FLAG_VER, conn->snd.NXT, 0);

        /*
         * Important!!! Current assumption is that ARDP_Accept() is being called from within AcceptCb()
         * in transport layer. If this is ever to change (i.e., ARDP_Accept() is moved outside AcceptCb(),
         * then connection state should be set to CLOSED and connection record is removed here.
         */

        return status;
    }

    SetState(conn, SYN_RCVD);
    /* <SEQ=snd.ISS><ACK=RCV.CUR><MAX=RCV.SEGMAX><BUFMAX=RCV.SEGBMAX><ACK><SYN> */
    SendSyn(handle, conn, buf, len);
    return ER_OK;
}

QStatus ARDP_Disconnect(ArdpHandle* handle, ArdpConnRecord* conn)
{
    QCC_DbgTrace(("ARDP_Disconnect(handle=%p, conn=%p)", handle, conn));
    if (!IsConnValid(handle, conn)) {
        return ER_ARDP_INVALID_CONNECTION;
    }
    return Disconnect(handle, conn, ER_OK);
}

QStatus ARDP_RecvReady(ArdpHandle* handle, ArdpConnRecord* conn, ArdpRcvBuf* rcv)
{
    QCC_DbgTrace(("ARDP_RecvReady(handle=%p, conn=%p, rcv=%p)", handle, conn, rcv));
    assert(rcv != NULL);
    if (!IsConnValid(handle, conn)) {
        return ER_ARDP_INVALID_CONNECTION;
    }

    if (conn->state == OPEN) {
        return ReleaseRcvBuffers(handle, conn, rcv->seq, rcv->fcnt, ER_OK);
    } else if ((conn->state == CLOSED) || (conn->state == CLOSE_WAIT)) {
        uint32_t i;
        for (i = 0; i < rcv->fcnt; i++) {
            if (rcv->data != NULL) {
                free(rcv->data);
            }
            rcv->flags = 0;
            rcv->data = NULL;
            if (!(rcv->next->flags & ARDP_BUFFER_IN_USE) || (rcv->next->som != rcv->som)) {
                break;
            }
            rcv = rcv->next;
        }
        return ER_OK;
    } else {
        return ER_ARDP_INVALID_STATE;
    }
}

QStatus ARDP_Send(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t* buf, uint32_t len, uint32_t ttl)
{
    QCC_DbgTrace(("ARDP_Send(handle=%p, conn=%p, buf=%p, len=%d., ttl=%d.)", handle, conn, buf, len, ttl));
    if (!IsConnValid(handle, conn)) {
        return ER_ARDP_INVALID_CONNECTION;
    }

    if (conn->state != OPEN) {
        return ER_ARDP_INVALID_STATE;
    }

    if (buf == NULL || len == 0) {
        return ER_INVALID_DATA;
    }

    QCC_DbgPrintf(("NXT=%u, UNA=%u", conn->snd.NXT, conn->snd.UNA));
    if ((conn->window == 0)  || (conn->snd.NXT - conn->snd.UNA) >= conn->snd.SEGMAX) {
        QCC_DbgPrintf(("NXT - UNA=%u, window=%u", conn->snd.NXT - conn->snd.UNA, conn->window));
        return ER_ARDP_BACKPRESSURE;
    } else {
        return SendData(handle, conn, buf, len, ttl);
    }
}

static QStatus Receive(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t* rxbuf, uint16_t len)
{
    QCC_DbgTrace(("Receive(handle=%p, conn=%p, rxbuf=%p, len=%d)", handle, conn, rxbuf, len));
    ArdpSeg seg;
    uint8_t hdrSz;

    seg.FLG = rxbuf[FLAGS_OFFSET];        /* The flags of the current segment */
    seg.HLEN = rxbuf[HLEN_OFFSET];      /* The header len */

    if (seg.FLG & ARDP_FLAG_RST) {

        /* This is a disconnect from the remote, no checks are needed */
        ArdpMachine(handle, conn, &seg, rxbuf, len);
        return ER_OK;
    }

    seg.DLEN = ntohs(*reinterpret_cast<uint16_t*>(rxbuf + DLEN_OFFSET)); /* The data payload size */

    hdrSz = (seg.FLG & ARDP_FLAG_SYN) ? ARDP_SYN_HEADER_SIZE : ARDP_FIXED_HEADER_LEN;

    /* Perform length validation checks */
    if (((seg.HLEN * 2) < hdrSz) || (len < hdrSz) || (seg.DLEN + (seg.HLEN * 2)) != len) {
        QCC_DbgHLPrintf(("Receive: length check failed len = %u, seg.hlen = %u, seg.dlen = %u",
                         len, (seg.HLEN * 2), seg.DLEN));
        return ER_ARDP_INVALID_RESPONSE;
    }

    seg.SEQ = ntohl(*reinterpret_cast<uint32_t*>(rxbuf + SEQ_OFFSET)); /* The send sequence of the current segment */
    seg.ACK = ntohl(*reinterpret_cast<uint32_t*>(rxbuf + ACK_OFFSET)); /* The cumulative acknowledgement number to our sends */

    if (!(seg.FLG & ARDP_FLAG_SYN)) {
        seg.LCS = ntohl(*reinterpret_cast<uint32_t*>(rxbuf + LCS_OFFSET)); /* The last consumed segment on receiver side (them) */
        seg.WINDOW = conn->snd.SEGMAX - (conn->snd.NXT - (seg.LCS + 1));   /* The receiver's window */
        seg.ACKNXT = ntohl(*reinterpret_cast<uint32_t*>(rxbuf + ACKNXT_OFFSET)); /* The first valid segment sender wants to be acknowledged */

        QCC_DbgHLPrintf(("Receive() window = %u, seq = %u, ack = %u, lcs = %u, acknxt = %u",
                         seg.WINDOW, seg.SEQ, seg.ACK, seg.LCS, seg.ACKNXT));

        seg.TTL = ntohl(*reinterpret_cast<uint32_t*>(rxbuf + TTL_OFFSET));       /* TTL associated with this segment */
        seg.SOM = ntohl(*reinterpret_cast<uint32_t*>(rxbuf + SOM_OFFSET));       /* Sequence number of the first fragment in message */
        seg.FCNT = ntohs(*reinterpret_cast<uint16_t*>(rxbuf + FCNT_OFFSET));     /* Number of segments comprising fragmented message */

        /* Perform sequence validation checks */
        if (SEQ32_LT(conn->snd.NXT, seg.ACK)) {
            QCC_DbgHLPrintf(("Receive: ack %u ahead of SND>NXT %u", seg.ACK, conn->snd.NXT));
            return ER_ARDP_INVALID_RESPONSE;
        }

        if (SEQ32_LT(seg.ACK, seg.LCS)) {
            QCC_DbgHLPrintf(("Receive: lcs %u and ack %u out of order", seg.LCS, seg.ACK));
            return ER_ARDP_INVALID_RESPONSE;
        }

        /*
         * SEQ and ACKNXT must fall within receive window. In case of segment with no payload,
         * allow one extra.
         */
        if (((seg.SEQ - seg.ACKNXT) > conn->rcv.SEGMAX) || (SEQ32_LT(seg.SEQ, seg.ACKNXT)) ||
            ((seg.DLEN != 0) && ((seg.SEQ - seg.ACKNXT) == conn->rcv.SEGMAX))) {
            QCC_DbgHLPrintf(("Receive: incorrect sequence numbers seg.seq = %u, seg.acknxt = %u",
                             seg.SEQ, seg.ACKNXT));
            return ER_ARDP_INVALID_RESPONSE;
        }

        /* Additional checks for invalid payload values */
        if (seg.DLEN != 0) {
            if ((seg.FCNT == 0) || (seg.FCNT > conn->rcv.SEGMAX) || ((seg.SEQ - seg.SOM) >= seg.FCNT)) {
                QCC_DbgHLPrintf(("Receive: incorrect data segment seq = %u, som = %u,  fcnt = %u",
                                 seg.SEQ, seg.SOM, seg.FCNT));
                return ER_ARDP_INVALID_RESPONSE;
            }
        }
    }

    ArdpMachine(handle, conn, &seg, rxbuf, len);
    return ER_OK;
}

QStatus Accept(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t* rxbuf, uint16_t len)
{
    QCC_DbgTrace(("Accept(handle=%p, conn=%p, buf=%p, len=%d)", handle, conn, rxbuf, len));
    assert(conn->state == CLOSED && "Accept(): ConnRecord in invalid state");

    if (!(rxbuf[FLAGS_OFFSET] & ARDP_FLAG_SYN) || (rxbuf[FLAGS_OFFSET] & ARDP_FLAG_RST)) {
        return ER_ARDP_INVALID_CONNECTION;
    }

    if ((rxbuf[FLAGS_OFFSET] & ARDP_VERSION_BITS) != ARDP_FLAG_VER) {
        QCC_DbgHLPrintf(("Accept(): Unsupported protocol version 0x%x",
                         rxbuf[FLAGS_OFFSET] & ARDP_VERSION_BITS));
        return ER_ARDP_VERSION_NOT_SUPPORTED;
    }

    conn->state = LISTEN;                      /* The call to Accept() implies a jump to LISTEN */
    conn->passive = true;                      /* This connection is (will be) the result of a passive open */

    return Receive(handle, conn, rxbuf, len);
}

static bool IsDuplicateConnRequest(ArdpHandle* handle, uint16_t foreign, qcc::IPAddress address)
{
    for (ListNode* ln = &handle->conns; (ln = ln->fwd) != &handle->conns;) {
        ArdpConnRecord* conn = (ArdpConnRecord*)ln;
        if ((conn->foreign == foreign) && (conn->ipAddr == address)) {
            QCC_DbgPrintf(("isDuplicateConnRequest(): Found conn %p, foreign %u", conn, foreign));
            return true;
        }
    }
    return false;
}

QStatus ARDP_Run(ArdpHandle* handle, qcc::SocketFd sock, bool sockRead, bool sockWrite, uint32_t* ms)
{
    const size_t bufferSize = 65536;      /* UDP packet can be up to 64K long */
    uint32_t buf32[bufferSize >> 2];
    uint8_t* buf = reinterpret_cast<uint8_t*>(buf32);
    qcc::IPAddress address;               /* The IP address of the foreign side */
    uint16_t port;                        /* Will be the UDP port of the foreign side */
    size_t nbytes;                        /* The number of bytes actually received */
    QStatus status = ER_OK;

    //QCC_DbgTrace(("ARDP_Run(handle=%p, sock=%d., socketRead=%d., socketWrite=%d., ms=%p)", handle, sock, sockRead, sockWrite, ms));
    if (sockWrite) {
        handle->trafficJam = false;
    }

    if (sockRead) {
        while ((status = qcc::RecvFrom(sock, address, port, buf, bufferSize, nbytes)) == ER_OK) {
#if ARDP_TESTHOOKS
            /*
             * Call the inbound testhook in case the test team needs to munge the
             * inbound data.
             */
            if (handle->th.RecvFrom) {
                handle->th.RecvFrom(handle, NULL, ARDP_RUN, buf, nbytes);
            }
#endif

            if (nbytes > 0 && nbytes < 65536) {
                uint16_t local, foreign;
                ProtocolDemux(buf, nbytes, &local, &foreign);
                if (local == 0) {
                    if (handle->accepting && handle->cb.AcceptCb) {
                        if (!IsDuplicateConnRequest(handle, foreign, address)) {
                            ArdpConnRecord* conn = NewConnRecord();
                            status = InitConnRecord(handle, conn, sock, address, port, foreign);
                            if (status == ER_OK) {
                                EnList(handle->conns.bwd, (ListNode*)conn);
                                status = Accept(handle, conn, buf, nbytes);
                            }
                            if (status != ER_OK) {
                                SetState(conn, CLOSED);
                                DelConnRecord(handle, conn, false);
                            }
                        } /*
                           * Else the remote most likely timed out waiting for our SYN_ACK.
                           * We should rely on local connection retry mechanism to kick in
                           * and eventually establish the connection.
                           */

                    } else {
                        status = ER_ARDP_INVALID_STATE;
                    }
                    if (status != ER_OK) {
                        SendRst(handle, sock, address, port, local, foreign);
                    }
                } else {
                    /* Is there an open connection? */
                    ArdpConnRecord* conn = FindConn(handle, local, foreign);
                    if (!conn) {
                        /* Is there a half open connection? */
                        conn = FindConn(handle, local, 0);
                    }

                    if (conn) {
                        if ((conn->state != CLOSED) && (conn->state != CLOSE_WAIT)) {
                            QCC_DbgHLPrintf(("ARDP_Run conn state %s", State2Text(conn->state)));
                            conn->lastSeen = TimeNow(handle->tbase);
                            conn->probeTimer.retry = handle->config.keepaliveRetries;
                            status = Receive(handle, conn, buf, nbytes);
                            if (status == ER_ARDP_INVALID_RESPONSE) {
                                Disconnect(handle, conn, status);
                            }
                        } else {
                            SendRst(handle, sock, address, port, local, foreign);
                        }
                    }
                }
            } else {
                QCC_DbgHLPrintf(("ARDP_Run(): Socket read failed (nbytes = %d)", nbytes));
                break;
            }
        }
    }

    handle->msnext = CheckTimers(handle);

    /*  Tell the higher levels when to call back next (timer expiration) */
    *ms = handle->msnext;

    //QCC_DbgHLPrintf(("ARDP_Run(): Call back in %u ms", *ms));

    if (handle->trafficJam) {
        status = (QStatus) ER_ARDP_WRITE_BLOCKED;
        //QCC_DbgPrintf(("ARDP_Run(): ER_ARDP_WRITE_BLOCKED"));
    }

    return status;
}

} // namespace ajn
