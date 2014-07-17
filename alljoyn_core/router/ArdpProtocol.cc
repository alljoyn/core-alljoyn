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

#define ARDP_DISCONNECT_RETRY 1  /* Not configurable, no retries */

#define ARDP_TTL_EXPIRED    0xffffffff
#define ARDP_TTL_MAX        (ARDP_TTL_EXPIRED - 1)
#define ARDP_TTL_INFINITE   0

/* Minimum Retransmit Timeout */
#define ARDP_MIN_RTO 100
/* Maximum Retransmit Timeout */
#define ARDP_MAX_RTO 64000

/* Delayed ACK timeout */
#define ARDP_ACK_TIMEOUT 100

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ABS(a) ((a) >= 0 ? (a) : -(a))

#define UDP_HEADER_SIZE 8
#define IPV4_HEADER_SIZE 20
#define IPV6_HEADER_SIZE 40

/* A simple circularly linked list node suitable for use in thin core library implementations */
typedef struct LISTNODE {
    struct LISTNODE* fwd;
    struct LISTNODE* bwd;
    uint8_t*         buf;
    uint32_t len;
} ListNode;

typedef void (*ArdpTimeoutHandler)(ArdpHandle* handle, ArdpConnRecord* conn, void* context);

typedef struct {
    ListNode list;
    ArdpTimeoutHandler handler;
    void* context;
    uint32_t delta;
    uint32_t when;
    uint32_t retry;
} ArdpTimer;

/**
 * Structure for tracking of received out-of-order segments.
 * Contains EACK bitmask to be sent to the remote side.
 */
typedef struct {
    uint32_t mask[ARDP_MAX_EACK_MASK_SZ];     /* mask in host order */
    uint32_t htnMask[ARDP_MAX_EACK_MASK_SZ];     /* mask in network order */
    uint16_t sz;
    uint16_t fixedSz;
} ArdpRcvMsk;

/**
 * Structure encapsulating the send-related quantities. The stuff we manage on
 * the local side of the connection and which we may send to THEM.
 */
typedef struct {
    uint32_t NXT;         /* The sequence number of the next segment that is to be sent */
    uint32_t UNA;         /* The sequence number of the oldest unacknowledged segment */
    uint32_t MAX;         /* The maximum number of unacknowledged segments that can be sent */
    uint32_t ISS;         /* The initial send sequence number. The number that was sent in the SYN segment */
    uint32_t LCS;         /* Sequence number of last consumed segment (we get this form them) */
} ArdpSnd;

/**
 * Structure encapsulating the information about segments on SEND side.
 */
typedef struct ARDP_SEND_BUF {
    uint8_t* data;
    uint32_t datalen;
    uint8_t* hdr;
    uint32_t ttl;
    uint32_t tStart;
    ARDP_SEND_BUF* next;
    ArdpTimer timer;
    bool inUse;
    uint16_t hdrlen;
    uint16_t fastRT;
} ArdpSndBuf;

/**
 * Structure encapsulating the receive-related quantities.  The stuff managed on
 * the remote/foreign side, copies of which we may get from THEM.
 */
typedef struct {
    uint32_t CUR;     /* The sequence number of the last segment received correctly and in sequence */
    uint32_t MAX;     /* The maximum number of segments that can be buffered for this connection */
    uint32_t IRS;     /* The initial receive sequence number.  The sequence number of the SYN that established the connection */
    uint32_t LCS;     /* The sequence number of last consumed segment */
} ArdpRcv;

/**
 * Structure encapsulating information about our send buffers.
 */
typedef struct {
    uint32_t MAX;       /* The largest possible segment that THEY can receive (our send buffer, specified by the other side during connection) */
    ArdpSndBuf* snd;    /* Array holding unacknowledged sent buffers */
    uint16_t maxDlen;   /* Maximum data payload size that can be sent without partitioning */
    uint16_t pending;   /* Number of unacknowledged sent buffers */
} ArdpSbuf;

/**
 * Structure encapsulating information about our receive buffers.
 */
typedef struct {
    uint32_t MAX;       /* The largest possible segment that WE can receive (our receive buffer, specified by our user on an open) */
    ArdpRcvBuf* rcv;    /* Array holding received buffers not consumed by the app */
    uint32_t last;      /* Sequence number of the last pending segment */
    uint16_t window;    /* Receive Window */
    uint16_t ackPending; /* Number of segments pending acknowledgement */
    uint32_t acknxt;    /* Sequence number the sender wants us to acknowledge, everything before the number has expired */
} ArdpRbuf;

/**
 * Information encapsulating the various interesting tidbits we get from the
 * other side when we receive a datagram.  Some of the names are chosen so that they
 * are simiar to the quantities found in RFC-908 when used.
 */
typedef struct {
    uint32_t SEQ;     /* The sequence number in the segment currently being processed. */
    uint32_t ACK;     /* The acknowledgement number in the segment currently being processed. */
    uint32_t MAX;     /* The maximum number of outstanding segments the receiver is willing to hold (from SYN segment). */
    uint32_t BMAX;    /* The maximum segment size accepted by the foreign host on a connection (specified in SYN). */
    uint32_t LCS;     /* The last "in-sequence" consumed segment */
    uint32_t ACKNXT;  /* The first valid SND segment, TTL accounting */
    uint32_t SOM;     /* Start sequence number for fragmented message */
    uint32_t TTL;     /* Time-to-live */
    uint16_t FCNT;    /* Number of fragments comprising a message */
    uint16_t DLEN;    /* The length of the data that came in with the current segment. */
    uint16_t DST;     /* The destination port in the header of the segment currently being processed. */
    uint16_t SRC;     /* The source port in the header of the segment currently being processed. */
    uint16_t WINDOW;  /* Receive window */
    uint8_t FLG;      /* The flags in the header of the segment currently being processed. */
    uint8_t HLEN;     /* The header length */
} ArdpSeg;

/**
 * The states through which our main state machine transitions.
 */
enum ArdpState {
    CLOSED = 1,    /* No connection exists and no connection record available */
    LISTEN,        /* Entered upon a passive open request.  Connection record is allocated and ARDP waits for a connection from remote */
    SYN_SENT,      /* Entered after rocessing an active open request.  SYN is sent and ARDP waits here for ACK of open request. */
    SYN_RCVD,      /* Reached from either LISTEN or SYN_SENT.  Generate ISN and ACK. */
    OPEN,          /* Successful echange of state information happened,.  Data may be sent and received. */
    CLOSE_WAIT     /* Entered if local close or remote RST received.  Delay for connection activity to subside. */
};

/**
 * The format of a SYN segment on the wire.
 */
typedef struct {
    uint8_t flags;      /* See Control flag definitions above */
    uint8_t hlen;       /* Length of the header in units of two octets (number of uint16_t) */
    uint16_t src;       /* Used to distinguish between multiple connections on the local side. */
    uint16_t dst;       /* Used to distinguish between multiple connections on the foreign side. */
    uint16_t dlen;      /* The length of the data in the current segment.  Does not include the header size. */
    uint32_t seq;       /* The sequence number of the current segment. */
    uint32_t ack;       /* The number of the segment that the sender of this segment last received correctly and in sequence. */
    uint32_t ttl;       /* Time-to-live */
    uint16_t window;    /* The current receive window */
    uint16_t segmax;    /* The maximum number of outstanding segments the other side can send without acknowledgement. */
    uint16_t segbmax;   /* The maximum segment size we are willing to receive.  (the RBUF.MAX specified by the user calling open). */
    uint16_t options;   /* Options for the connection.  Always Sequenced Delivery Mode (SDM). */
} ArdpSynSegment;

typedef struct {
    ArdpSynSegment ss;
    uint8_t* data;       /* Place to hold connection handshake data (SASL, HELLO, etc) */
    uint32_t dataLen;    /* Length of connection handshake data */
} ArdpSynSnd;

/**
 * A connection record describing each "connection."  This acts as a containter
 * to hold all of the interesting information about a reliable link between
 * hosts.
 */
struct ARDP_CONN_RECORD {
    ListNode list;          /* Doubly linked list node on which this connection might be */
    uint32_t id;            /* Randomly chosen connection identifier */
    ArdpState STATE;        /* The current sate of the connection */
    bool passive;           /* If true, this is a passive open (we've been connected to); if false, we did the connecting */
    ArdpSnd SND;            /* Send-side related state information */
    ArdpSbuf SBUF;          /* Information about send buffers */
    ArdpRcv RCV;            /* Receive-side related state information */
    ArdpRbuf RBUF;          /* Information about receive buffers */
    uint16_t local;         /* ARDP local port for this connection */
    uint16_t foreign;       /* ARDP foreign port for this connection */
    qcc::SocketFd sock;     /* A convenient copy of socket we use to communicate. */
    qcc::IPAddress ipAddr;  /* A convenient copy of the IP address of the foreign side of the connection. */
    uint16_t ipPort;        /* A convenient copy of the IP port of the foreign side of the connection. */
    uint16_t window;        /* Current send window, dynamic setting */
    uint16_t minSendWindow; /* Minimum send window needed to accomodate max message */
    uint16_t sndHdrLen;     /* Length of send ARDP header on this connection */
    uint16_t rcvHdrLen;     /* Length of receive ARDP header on this connection */
    ArdpRcvMsk rcvMsk;      /* Tracking of received out-of-order segments */
    uint16_t remoteMskSz;   /* Size of of EACK bitmask present in received segment */
    uint32_t lastSeen;      /* Last time we received communication on this connection. */
    ArdpSynSnd synSnd;      /* Connection establishment data */
    ListNode dataTimers;    /* List of currently scheduled retransmit timers */
    bool rttInit;           /* Flag indicating that the first RTT was measured and SRTT calculation applies */
    uint32_t rttMean;       /* Smoothed RTT value */
    uint32_t rttMeanVar;    /* RTT variance */
    uint32_t backoff;       /* Backoff factor accounting for retransmits on connection, resets to 1 when receive "good ack" */
    ArdpTimer connectTimer; /* Connect/Disconnect timer */
    ArdpTimer probeTimer;   /* Probe (link timeout) timer */
    ArdpTimer ackTimer;   /* Delayed ACK timer */
    ArdpTimer persistTimer; /* Persist (frozen window) timer */
    void* context;          /* A client-defined context pointer */
};

struct ARDP_HANDLE {
    ArdpGlobalConfig config; /* The configurable items that affect this instance of ARDP as a whole */
    ArdpCallbacks cb;        /* The callbacks to allow the protocol to talk back to the client */
    bool accepting;          /* If true the ArdpProtocol is accepting inbound connections */
    ListNode conns;          /* List of currently active connections */
    qcc::Timespec tbase;     /* Baseline time */
    uint32_t msnext;         /* To inform upper layer when to call into the protocol next time */
    void* context;           /* A client-defined context pointer */
};

/*
 * Important!!! All our numbers are within window size, so below calculation will be valid.
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
static QStatus Send(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t flags, uint32_t seq, uint32_t ack, uint32_t lcs);

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
        assert(!IsEmpty(node) && "DeList(): called on empty list");
        return;
    }
    node->bwd->fwd = node->fwd;
    node->fwd->bwd = node->bwd;
    node->fwd = node->bwd = node;
}

#ifndef NDEBUG
static void DumpBuffer(uint8_t* buf, uint16_t len)
{
    QCC_DbgPrintf(("DumpBuffer buf=%p, len=%d", buf, len));
    for (uint32_t i = 0; i < len; i += 8) {
        QCC_DbgPrintf(("\t%d\t %2x (%d), %2x (%d), %2x (%d), %2x (%d), %2x (%d), %2x (%d), %2x (%d), %2x (%d),",
                       i, buf[i], buf[i], buf[i + 1], buf[i + 1], buf[i + 2], buf[i + 2], buf[i + 3], buf[i + 3],
                       buf[i + 4], buf[i + 4], buf[i + 5], buf[i + 5], buf[i + 6], buf[i + 6], buf[i + 7], buf[i + 7]));
    }
}

static void DumpBitMask(ArdpConnRecord* conn, uint32_t* msk, uint16_t sz, bool convert)
{
    QCC_DbgPrintf(("DumpBitMask(conn=%p, msk=%p, sz=%d, convert=%s)",
                   conn, msk, sz, convert ? "true" : "false"));

    for (uint16_t i = 0; i < sz; i++) {
        uint32_t mask32;
        if (convert) {
            mask32 = ntohl(msk[i]);
        } else {
            mask32 = msk[i];
        }
        QCC_DbgPrintf(("\t %d:  %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x",
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
    QCC_DbgTrace(("SetState: conn=%p %s=>%s", conn, State2Text(conn->STATE), State2Text(state)));
    conn->STATE = state;
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

static void InitTimer(ArdpHandle* handle, ArdpConnRecord* conn, ArdpTimer* timer, ArdpTimeoutHandler handler, void*context, uint32_t timeout, uint16_t retry)
{
    QCC_DbgTrace(("InitTimer: conn=%p timer=%p handler=%p context=%p timeout=%u retry=%u",
                  conn, timer, handler, context, timeout, retry));

    timer->handler = handler;
    timer->context = context;
    timer->delta = timeout;
    timer->when = TimeNow(handle->tbase) + timeout;
    timer->retry = retry;
    /* Update "call-me-back" value */
    if ((retry != 0) && (timeout < handle->msnext)) {
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
                }
            }
        }
        return next;
    }

    /* Check probe timer */
    if (conn->probeTimer.retry != 0 && conn->probeTimer.when <= now) {
        QCC_DbgPrintf(("CheckConnTimers: Fire probe( %p ) timer %p at %u (now=%u)",
                       conn, conn->probeTimer, conn->probeTimer.when, now));
        (conn->probeTimer.handler)(handle, conn, conn->probeTimer.context);
        conn->probeTimer.when = now + conn->probeTimer.delta;
    }

    if (conn->probeTimer.when < next && conn->probeTimer.retry != 0) {
        /* Update "call-me-next-ms" value */
        next = conn->probeTimer.when;
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

        /* No pending retransmits should be present on the connection if
         * we have fired up persist timer */
        assert(IsEmpty(&conn->dataTimers));
        return next;
    }

    if (conn->persistTimer.when < next && conn->persistTimer.retry != 0) {
        /* Update "call-me-next-ms" value */
        next = conn->persistTimer.when;
    }

    ListNode* ln = &conn->dataTimers;

    if (IsEmpty(ln)) {
        return next;
    }

    for (; (ln = ln->fwd) != &conn->dataTimers;) {
        ArdpTimer* timer = (ArdpTimer*)ln;

        if ((timer->when <= now) && (timer->retry > 0)) {
            QCC_DbgPrintf(("CheckConnTimers: conn %p, fire retransmit timer %p at %u (now=%u)",
                           conn, timer, timer->when, now));
            (timer->handler)(handle, conn, timer->context);
            timer->when = now + timer->delta;
        }

        /* If we hit the retransmit limit, connection is going down.
         * No point in retransmitting the rest of the data */
        if (timer->retry == 0) {
            ln = ln->bwd;
            DeList((ListNode*)timer);
            break;
        } else if (timer->when < next) {
            /* Update "call-me-next-ms" value */
            next = timer->when;
        }
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
        if (!IsConnValid(handle, (ArdpConnRecord*) ln)) {
            ln = temp;
        }
        if (IsEmpty(&handle->conns)) {
            break;
        }
    }

    return (nextTime != ARDP_NO_TIMEOUT) ? nextTime - now : ARDP_NO_TIMEOUT;
}

static void DelConnRecord(ArdpHandle* handle, ArdpConnRecord* conn, bool forced)
{
    QCC_DbgTrace(("DelConnRecord(handle=%p conn=%p forced=%s state=%s)",
                  handle, conn, forced ? "true" : "false", State2Text(conn->STATE)));

    if (!forced && conn->STATE != CLOSED && conn->STATE != CLOSE_WAIT) {
        QCC_LogError(ER_ARDP_INVALID_STATE, ("DelConnRecord(): Delete while not CLOSED or CLOSE-WAIT conn %p state %s",
                                             conn, State2Text(conn->STATE)));
        assert((conn->STATE == CLOSED || conn->STATE == CLOSE_WAIT)  &&
               "DelConnRecord(): Delete while not CLOSED or CLOSE-WAIT");

    }

    /* Safe to check together as these buffers are always allocated together */
    if (conn->SBUF.snd != NULL && conn->SBUF.snd[0].hdr != NULL) {
        free(conn->SBUF.snd[0].hdr);
        free(conn->SBUF.snd);
    }
    if (conn->RBUF.rcv != NULL) {
        for (uint32_t i = 0; i < conn->RCV.MAX; i++) {
            if (conn->RBUF.rcv[i].data != NULL) {
                free(conn->RBUF.rcv[i].data);
            }
        }
        free(conn->RBUF.rcv);
    }

    DeList((ListNode*)conn);

    if (conn->synSnd.data != NULL) {
        free(conn->synSnd.data);
    }

    delete conn;
}

static void FlushMessage(ArdpHandle* handle, ArdpConnRecord* conn, ArdpSndBuf* snd, QStatus status)
{
    ArdpHeader* h = (ArdpHeader*) snd->hdr;
    uint16_t fcnt = ntohs(h->fcnt);
    uint32_t len = 0;
    /* Original sent data buffer */
    uint8_t*buf = snd->data;

    /* Mark all fragment SND buffers as available */
    do {
        snd->inUse = false;
        snd->fastRT = 0;
        len += ntohs(h->dlen);
        snd = snd->next;
        h = (ArdpHeader*) snd->hdr;
        conn->SBUF.pending--;
        QCC_DbgPrintf(("FlushMessage(fcnt = %d): pending = %d", fcnt, conn->SBUF.pending));
        assert((conn->SBUF.pending < conn->SND.MAX) && "Invalid number of pending segments in send queue!");
        h = (ArdpHeader*) snd->hdr;
        fcnt--;
    } while (fcnt > 0);

    QCC_DbgPrintf(("FlushMessage(): SendCb(handle=%p, conn=%p, buf=%p, len=%d, status=%d",
                   handle, conn, buf, len, status));

    /* Mark all fragment SND buffers as available */
    handle->cb.SendCb(handle, conn, buf, len, status);
}

static void FlushSendQueue(ArdpHandle* handle, ArdpConnRecord* conn, QStatus status)
{
    /*
     * SendCb() for all pending messages so that the upper layer knows
     * to release the corresponding buffers
     */
    for (uint32_t i = 0; i < conn->SND.MAX; i++) {
        ArdpHeader* h = (ArdpHeader* ) conn->SBUF.snd[i].hdr;
        if (conn->SBUF.snd[i].inUse && (h->seq == h->som)) {
            FlushMessage(handle, conn, &conn->SBUF.snd[i], status);
        }
    }
}

static bool IsRcvQueueEmpty(ArdpHandle* handle, ArdpConnRecord* conn)
{
    for (uint32_t i = 0; i < conn->RCV.MAX; i++) {
        if (conn->RBUF.rcv[i].isDelivered) {
            return false;
        }
    }
    return true;
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
        FlushSendQueue(handle, conn, ER_ARDP_DISCONNECTING);
        QCC_DbgPrintf(("DisconnectTimerHandler: DisconnectCb(handle=%p conn=%p reason = %s)", handle, conn, QCC_StatusText(reason)));
        handle->cb.DisconnectCb(handle, conn, reason);
        /*
         * Change the status to ER_ARDP_INVALID_CONNECTION. This is needed in case the DisconnectTimer is rescheduled
         * in order to wait on RCV queue to drain. */
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
    }

}

static void ConnectTimerHandler(ArdpHandle* handle, ArdpConnRecord* conn, void* context)
{
    QCC_DbgTrace(("ConnectTimerHandler: handle=%p conn=%p", handle, conn));
    QStatus status = ER_FAIL;
    ArdpTimer* timer = &conn->connectTimer;

    QCC_DbgTrace(("ConnectTimerHandler: retries left %d", timer->retry));

    if (timer->retry > 1) {
        qcc::ScatterGatherList msgSG;
        size_t sent;
        QCC_DbgPrintf(("ConnectTimerHandler: segbmax=%d", ntohs(conn->synSnd.ss.segbmax)));
        msgSG.AddBuffer(&conn->synSnd.ss, sizeof(ArdpSynSegment));
        msgSG.AddBuffer(conn->synSnd.data, conn->synSnd.dataLen);
        status = qcc::SendToSG(conn->sock, conn->ipAddr, conn->ipPort, msgSG, sent);

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
        handle->cb.ConnectCb(handle, conn, conn->passive, NULL, 0, ER_TIMEOUT);
        Send(handle, conn, ARDP_FLAG_RST | ARDP_FLAG_VER, conn->SND.UNA, 0, conn->RCV.MAX);

        /*
         * Do not delete conection record here:
         * The upper layer will detect an error status and call ARDP_ReleaseConnection()
         */
    } else {
        timer->retry--;
    }
}

static void AckTimerHandler(ArdpHandle* handle, ArdpConnRecord* conn, void* context)
{
    QStatus status = Send(handle, conn, ARDP_FLAG_ACK | ARDP_FLAG_VER, conn->SND.NXT, conn->RCV.CUR, conn->RCV.LCS);

    if (status == ER_OK) {
        /* Stop timer until there is something else to acknowledge */
        conn->ackTimer.retry = 0;
        conn->RBUF.ackPending = 0;
    }
}

static void ExpireMessageSnd(ArdpHandle* handle, ArdpConnRecord* conn, ArdpSndBuf* snd, uint32_t msElapsed)
{
    ArdpHeader* h = (ArdpHeader*) snd->hdr;;
    uint32_t som = ntohl(h->som);
    uint16_t fcnt = ntohs(h->fcnt);
    ArdpSndBuf* start = &conn->SBUF.snd[som % conn->SND.MAX];

    /* Start fragment of the expired message */
    snd = start;

    QCC_DbgPrintf(("ExpireMessageSnd message with SOM %u and fcnt %d expired", som, fcnt));
    if (SEQ32_LT(conn->SND.UNA, (som + fcnt))) {
        conn->SND.UNA = som + fcnt;
    }

    do {
        h = (ArdpHeader*) snd->hdr;
        snd->timer.retry = 0;
        snd = snd->next;
        snd->ttl = ARDP_TTL_EXPIRED;
        fcnt--;
    } while (fcnt > 0);

}

static QStatus SendMsgHeader(ArdpHandle* handle, ArdpConnRecord* conn, ArdpHeader* h)
{
    qcc::ScatterGatherList msgSG;
    size_t sent;

    QCC_DbgTrace(("SendMsgHeader(): handle=0x%p, conn=0x%p, hdr=0x%p", handle, conn, h));
    if (conn->rcvMsk.sz != 0) {
        h->flags |= ARDP_FLAG_EACK;
        QCC_DbgPrintf(("SendMsgHeader: have EACKs flags = %2x", h->flags));
    }

    msgSG.AddBuffer(h, ARDP_FIXED_HEADER_LEN);
    msgSG.AddBuffer(conn->rcvMsk.htnMask, conn->rcvMsk.fixedSz * sizeof(uint32_t));
    return qcc::SendToSG(conn->sock, conn->ipAddr, conn->ipPort, msgSG, sent);
}

static QStatus SendMsgData(ArdpHandle* handle, ArdpConnRecord* conn, ArdpSndBuf* sndBuf)
{

    ArdpHeader* h = (ArdpHeader*) sndBuf->hdr;
    qcc::ScatterGatherList msgSG;
    size_t sent;
    QStatus status;

    QCC_DbgTrace(("SendMsgData(): handle=%p, conn=%p, hdr=%p, hdrlen=%d, data=%p, datalen=%d, ttl=%u, tStart=%u",
                  handle, conn, sndBuf->hdr, sndBuf->hdrlen, sndBuf->data, sndBuf->datalen, sndBuf->ttl, sndBuf->tStart));

    msgSG.AddBuffer(sndBuf->hdr, ARDP_FIXED_HEADER_LEN);
    msgSG.AddBuffer(conn->rcvMsk.htnMask, conn->rcvMsk.fixedSz * sizeof(uint32_t));
    msgSG.AddBuffer(sndBuf->data, sndBuf->datalen);

    h->ack = htonl(conn->RCV.CUR);
    h->lcs = htonl(conn->RCV.LCS);
    h->acknxt = htonl(conn->SND.UNA);

    QCC_DbgPrintf(("SendMsgData(): seq = %u, ack=%u, lcs = %u, window = %d", ntohl(h->seq), conn->RCV.CUR, conn->RCV.LCS, conn->RBUF.window));

    if (conn->rcvMsk.sz == 0) {
        h->flags &= (~ARDP_FLAG_EACK);
    } else {
        h->flags |= ARDP_FLAG_EACK;
        QCC_DbgPrintf(("SendMsgData(): have EACKs flags = %2x", h->flags));
    }

    /*
     * the ttl found in the sndBuf has the same meaning as the ttl in the
     * AllJoyn Message that the sndBuf contains.  AllJoyn attempts to do a
     * one-way estimation of the clock offset and network delay by comparing a
     * local timestamp acquired when a message is unmarshaled with a remote
     * timestamp acquired when the message is marshaled.  AllJoyn therefore
     * has its own way of deciding when to expire a message from immediately
     * before we get it on the source side to immediately after we give it up
     * on the destination.
     *
     * Since that estimation is all tangled up with the PeerState object and
     * internal message timestamps, we have our own version of TTL which we use
     * that means more like "Time Left Until Expiration while the Message is in
     * Transit.  In-transit means while ARDP is moving it from one endpoint in
     * one Routing Node to another endpoint in another Routing Node.
     *
     */
    if (sndBuf->ttl != ARDP_TTL_INFINITE) {
        /*
         * There is a non-infinite time-to-live on this message
         */
        uint32_t msElapsed = TimeNow(handle->tbase) - sndBuf->tStart;

        /* Factor in mean RTT to account for time on the wire */
        msElapsed += (conn->rttMean >> 1);

        /*
         * If the message has never been on the wire, it is trivial to drop.
         */
        if (sndBuf->inUse == false) {

            /*
             * This is a brand new segment, we are seeing it for the first time.
             */
            QCC_DbgPrintf(("SendMsgData(): nonzero sndBuf->ttl=%d., msElapsed=%d.", sndBuf->ttl, msElapsed));

            /*
             * If the message is expired we don't send it, we return an error
             * status for ARDP_Send() to give back and that is the end of the
             * story.
             */
            if (msElapsed >= sndBuf->ttl) {
                QCC_DbgPrintf(("SendMsgData(): Dropping expired message (conn=0x%p, buf=0x%p, len=%d.)",
                               conn, sndBuf->data, sndBuf->datalen));

                return ER_ARDP_TTL_EXPIRED;
            }
        } else {
            /*
             * This is the more complicated case where we have already sent a
             * copy of the message on the wire, but it has not been ACKed yet.
             * The implication is that the retransmit timer handler fired.
             */
            if (msElapsed >= sndBuf->ttl) {
                ExpireMessageSnd(handle, conn, sndBuf, msElapsed);
                return ER_OK;
            } else {
                /*
                 * Set ttl to the number of milliseconds remaining at the "instant"
                 * before it was transmitted.
                 */
                h->ttl = htonl(sndBuf->ttl - msElapsed);
            }
        }
    }

    status = qcc::SendToSG(conn->sock, conn->ipAddr, conn->ipPort, msgSG, sent);

    if (status == ER_OK) {
        if (conn->ackTimer.retry != 0) {
            UpdateTimer(handle, conn, &conn->ackTimer, ARDP_ACK_TIMEOUT, 1);
        }
    }

    return status;
}

static QStatus Send(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t flags, uint32_t seq, uint32_t ack, uint32_t lcs)
{
    ArdpHeader h;

    QCC_DbgTrace(("Send(handle=%p, conn=%p, flags=0x%02x, seq=%u, ack=%u, lcs=%u)", handle, conn, flags, seq, ack, lcs));

    h.flags = flags;
    h.hlen = conn->sndHdrLen / 2;
    h.src = htons(conn->local);
    h.dst = htons(conn->foreign);;
    h.dlen = 0;
    h.seq = htonl(seq);
    h.ack = htonl(ack);
    h.lcs = htonl(lcs);
    h.acknxt = htonl(conn->SND.UNA);

    if (h.dst == 0) {
        QCC_DbgPrintf(("Send(): destination = 0"));
    }

    return SendMsgHeader(handle, conn, &h);
}

static QStatus Disconnect(ArdpHandle* handle, ArdpConnRecord* conn, QStatus reason)
{
    QStatus status = ER_OK;
    uint32_t timeout = 0;

    QCC_DbgTrace(("Disconnect(handle=%p, conn=%p, reason=%s)", handle, conn, QCC_StatusText(reason)));

    if (conn->STATE == CLOSE_WAIT || conn->STATE == CLOSED) {
        QCC_DbgPrintf(("Disconnect(handle=%p, conn=%p, reason=%s) Already disconnect%s",
                       handle, conn, QCC_StatusText(reason), conn->STATE == CLOSED ? "ed" : "ing"));
        return ER_OK;
    }

    if (!IsConnValid(handle, conn)) {
        return ER_ARDP_INVALID_CONNECTION;
    }

    /* Is there a nice macro that would nicely wrap integer into pointer? Just to avoid nasty surprises... */
    assert(sizeof(QStatus) <=  sizeof(void*));

    SetState(conn, CLOSE_WAIT);

    /* If the local side is the one initiating the disconnect, send RST segment to the remote */
    if (reason != ER_ARDP_REMOTE_CONNECTION_RESET) {
        status = Send(handle, conn, ARDP_FLAG_RST | ARDP_FLAG_VER, conn->SND.NXT, conn->RCV.CUR, conn->RCV.LCS);
        if (status != ER_OK) {
            QCC_LogError(status, ("Disconnect: failed to send RST to the remote"));
        }
    }

    /* If this disconnect is not a result of ARDP_Disconnect(), inform the upper layer that we are disconnecting */
    if (reason != ER_OK) {
        FlushSendQueue(handle, conn, ER_ARDP_DISCONNECTING);
        timeout = handle->config.timewait;
        QCC_DbgPrintf(("Disconnect: Call DisconnectCb() on conn %p, state %s reason %s ",
                       conn, State2Text(conn->STATE), QCC_StatusText(reason)));
        handle->cb.DisconnectCb(handle, conn, reason);
    }

    InitTimer(handle, conn, &conn->connectTimer, DisconnectTimerHandler, (void*) reason, timeout, ARDP_DISCONNECT_RETRY);

    return status;
}

/*
 *    error = measuredRTT - meanRTT
 *    new meanRTT = 7/8 * meanRTT + 1/8 * error
 *    new meanVar = 3/4 * meanVar + 1/4 * |error|
 */
static void AdjustRTT(ArdpHandle* handle, ArdpConnRecord* conn, ArdpSndBuf* snd)
{
    uint32_t now = TimeNow(handle->tbase);
    uint32_t rtt = now - snd->tStart;
    int32_t err;

    if (!conn->rttInit) {
        conn->rttMean = rtt;
        conn->rttMeanVar = rtt >> 1;
        conn->rttInit = true;
    }

    err = rtt - conn->rttMean;

    QCC_DbgPrintf(("AdjustRtt: mean = %u, var =%u, rtt = %u, now = %u, tStart= %u, error = %d",
                   conn->rttMean, conn->rttMeanVar, rtt, now, snd->tStart, err));
    conn->rttMean = (7 * conn->rttMean + rtt) >> 3;
    conn->rttMeanVar = (conn->rttMeanVar * 3 + ABS(err)) >> 2;

    conn->backoff = 0;

    QCC_DbgPrintf(("AdjustRtt: New mean = %u, var =%u", conn->rttMean, conn->rttMeanVar));
}

inline static uint32_t GetRTO(ArdpHandle* handle, ArdpConnRecord* conn)
{
    /*
     * RTO = (rttMean + (4 * rttMeanVar)) << backoff
     */
    uint32_t ms = (MAX((uint32_t)ARDP_MIN_RTO, conn->rttMean + (4 * conn->rttMeanVar))) << conn->backoff;
    return MIN(ms, (uint32_t)ARDP_MAX_RTO);
}

static void RetransmitTimerHandler(ArdpHandle* handle, ArdpConnRecord* conn, void* context)
{
    QCC_DbgTrace(("RetransmitTimerHandler: handle=%p conn=%p context=%p", handle, conn, context));
    ArdpSndBuf* snd = (ArdpSndBuf*) context;
    ArdpTimer* timer = &snd->timer;

    assert(snd->inUse && "RetransmitTimerHandler: trying to resend flushed buffer");

    if (timer->retry > 1) {
        QCC_DbgPrintf(("RetransmitTimerHandler: context=snd=%p seq=%u retries=%d", snd, ntohl(((ArdpHeader*)snd->hdr)->seq), timer->retry));
        QStatus status = SendMsgData(handle, conn, snd);

        if (status == ER_OK) {
            timer->retry--;
            conn->backoff = MAX(conn->backoff, (handle->config.dataRetries + 1) - timer->retry);
            timer->delta = GetRTO(handle, conn);
        } else if (status == ER_WOULDBLOCK) {
            timer->delta = 0; /* Try next time around */
            /* Since that won't be a legitimate retransmit,  don't update retries */
        } else {
            QCC_LogError(status, ("RetransmitTimerHandler: Write to Socket went bad. Disconnect."));
            timer->retry = 0;
            Disconnect(handle, conn, status);
        }

    } else {
        QCC_DbgHLPrintf(("RetransmitTimerHandler seq=%u retries hit the limit: %d", ntohl(((ArdpHeader*)snd->hdr)->seq), handle->config.dataRetries));
        timer->retry = 0;
        Disconnect(handle, conn, ER_TIMEOUT);
    }
}

static void PersistTimerHandler(ArdpHandle* handle, ArdpConnRecord* conn, void* context)
{
    ArdpTimer* timer = &conn->persistTimer;
    QCC_DbgTrace(("PersistTimerHandler: handle=%p conn=%p context=%p delta %u retry %u",
                  handle, conn, context, timer->delta, timer->retry));

    if (conn->window < conn->minSendWindow && IsEmpty(&conn->dataTimers)) {
        if (timer->retry > 1) {
            QCC_DbgPrintf(("PersistTimerHandler: window %u, need at least %u", conn->window, conn->minSendWindow));
            Send(handle, conn, ARDP_FLAG_ACK | ARDP_FLAG_VER | ARDP_FLAG_NUL, conn->SND.NXT, conn->RCV.CUR, conn->RCV.LCS);
            timer->retry--;
            timer->delta = handle->config.persistTimeout << (handle->config.persistRetries - timer->retry);
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
    uint32_t now = TimeNow(handle->tbase);
    uint32_t elapsed = now - conn->lastSeen;
    uint32_t delta = MAX(GetRTO(handle, conn), handle->config.probeTimeout);
    /* Connection timeout */
    uint32_t linkTimeout = delta * handle->config.probeRetries;

    /*
     * Relevant only if there are no pending retransmissions.
     * We will disconnect on retransmission attempts if we hit the limit there.
     */
    if (IsEmpty(&conn->dataTimers)) {
        QCC_DbgTrace(("ProbeTimerHandler: handle=%p conn=%p context=%p delta %u now %u lastSeen = %u elapsed %u",
                      handle, conn, context, timer->delta, now, conn->lastSeen, elapsed));
        if (elapsed >= linkTimeout) {
            QCC_DbgHLPrintf(("ProbeTimerHandler: Probe Timeout: now =%u, lastSeen = %u, elapsed=%u(vs limit of %u)", now, conn->lastSeen, elapsed, linkTimeout));
            Disconnect(handle, conn, ER_ARDP_PROBE_TIMEOUT);
        } else if (elapsed >= handle->config.probeTimeout) {
            QCC_DbgPrintf(("ProbeTimerHandler: send ping (NUL packet)"));
            Send(handle, conn, ARDP_FLAG_ACK | ARDP_FLAG_VER | ARDP_FLAG_NUL, conn->SND.NXT, conn->RCV.CUR, conn->RCV.LCS);
            timer->delta = delta;
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
    return conn->SBUF.pending;
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
       *Note: this is called ONLY from successful ConnectCb(). It should be safe
     * to return an IP port here. The above check is for catching programming error. */
    return conn->ipPort;
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

static void InitSnd(ArdpConnRecord* conn)
{
    QCC_DbgTrace(("InitSnd(conn=%p)", conn));

    conn->SND.ISS = qcc::Rand32();        /* Initial sequence number used for sending data over this connection */
    conn->SND.NXT = conn->SND.ISS + 1;    /* The sequence number of the next segment to be sent over this connection */
    conn->SND.UNA = conn->SND.ISS;        /* The oldest unacknowledged segment is the ISS */
    conn->SND.LCS = conn->SND.ISS;        /* The most recently consumed segment (we keep this in sync with the other side) */
    conn->SND.MAX = 0;                    /* The maximum number of unacknowledged segments we can send (other side will tell us this */
}

static QStatus InitRcv(ArdpConnRecord* conn, uint32_t segmax, uint32_t segbmax)
{
    QCC_DbgTrace(("InitRcv(conn=%p, segmax=%d, segbmax=%d)", conn, segmax, segbmax));
    conn->RCV.MAX = segmax;     /* The maximum number of outstanding segments that we can buffer (we will tell other side) */
    conn->RBUF.MAX = segbmax;   /* The largest buffer that can be received on this connection (our buffer size) */

    conn->RBUF.window = segmax;
    conn->RBUF.rcv = (ArdpRcvBuf*) malloc(segmax * sizeof(ArdpRcvBuf));
    if (conn->RBUF.rcv == NULL) {
        QCC_DbgPrintf(("InitRcv: Failed to allocate space for receive structure"));
        return ER_OUT_OF_MEMORY;
    }

    memset(conn->RBUF.rcv, 0, segmax * sizeof(ArdpRcvBuf));
    for (uint32_t i = 0; i < segmax; i++) {
        conn->RBUF.rcv[i].next = &conn->RBUF.rcv[((i + 1) % segmax)];
    }
    return ER_OK;
}

/* Extra Initialization after connection has been established */
static void PostInitRcv(ArdpConnRecord* conn)
{
    conn->RCV.LCS = conn->RCV.CUR;
    conn->RBUF.last = conn->RCV.CUR + 1;
    for (uint16_t i = 0; i < conn->RCV.MAX; i++) {
        conn->RBUF.rcv[i].seq = conn->RCV.IRS;
    }
}

static QStatus InitConnRecord(ArdpHandle* handle, ArdpConnRecord* conn, qcc::SocketFd sock, qcc::IPAddress ipAddr, uint16_t ipPort, uint16_t foreign)
{
    QCC_DbgTrace(("InitConnRecord(handle=%p, conn=%p, sock=%d, ipAddr=\"%s\", ipPort=%d, foreign=%d)",
                  handle, conn, sock, ipAddr.ToString().c_str(), ipPort, foreign));
    uint16_t local;
    uint32_t count = 0;

    conn->STATE = CLOSED;                        /* Starting state is always CLOSED */
    InitSnd(conn);                               /* Initialize the sender side of the connection */
    local = (qcc::Rand32() % 65534) + 1;   /* Allocate an "ephemeral" source port */
    /* Make sure this is a unique combiation of foreign/local */
    while (FindConn(handle, conn->local, foreign) != NULL) {
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

    SetEmpty(&conn->dataTimers);

    conn->rttInit = false;
    conn->rttMean = handle->config.dataTimeout;
    conn->rttMeanVar = 0;

    conn->sndHdrLen = ARDP_FIXED_HEADER_LEN;
    conn->rcvHdrLen = ARDP_FIXED_HEADER_LEN;
    conn->backoff = 0;

    return ER_OK;
}

static void ProtocolDemux(uint8_t* buf, uint16_t len, uint16_t* local, uint16_t* foreign)
{
    QCC_DbgTrace(("ProtocolDemux(buf=%p, len=%d, local*=%p, foreign*=%p)", buf, len, local, foreign));
    *local = ntohs(((ArdpHeader*)buf)->dst);
    *foreign = ntohs(((ArdpHeader*)buf)->src);
    QCC_DbgTrace(("ProtocolDemux(): local %d, foreign %d", *local, *foreign));
}

static ArdpConnRecord* FindConn(ArdpHandle* handle, uint16_t local, uint16_t foreign)
{
    QCC_DbgTrace(("FindConn(handle=%p, local=%d, foreign=%d)", handle, local, foreign));

    for (ListNode* ln = &handle->conns; (ln = ln->fwd) != &handle->conns;) {
        ArdpConnRecord* conn = (ArdpConnRecord*)ln;
        QCC_DbgPrintf(("FindConn(): check out conn->local = %d, conn->foreign = %d", conn->local, conn->foreign));
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
    uint32_t timeout;
    uint16_t fcnt;
    uint32_t lastLen;
    uint8_t* segData = buf;
    uint32_t som = htonl(conn->SND.NXT);
    uint16_t index = conn->SND.NXT % conn->SND.MAX;
    ArdpSndBuf* snd = &conn->SBUF.snd[index];
    uint32_t now = TimeNow(handle->tbase);

    /*
     * Note that a ttl of 0 means "forever" and the maximum TTL in the protocol is 65535 ms
     */

    QCC_DbgTrace(("SendData(handle=%p, conn=%p, buf=%p, len=%d., ttl=%u.)", handle, conn, buf, len, ttl));
    QCC_DbgPrintf(("SendData(): Sending %d bytes of data from src=%d to dst=%d", len, conn->local, conn->foreign));

    if (len <= conn->SBUF.maxDlen) {
        /* Data fits into one segment */
        fcnt = 1;
        lastLen = len;
    } else {
        /* Need fragmentation */
        fcnt = (len + (conn->SBUF.maxDlen - 1)) / conn->SBUF.maxDlen;
        lastLen = len % conn->SBUF.maxDlen;
        lastLen  = (lastLen != 0) ? lastLen : conn->SBUF.maxDlen;

        QCC_DbgPrintf(("SendData(): Large buffer %d, partitioning into %d segments", len, fcnt));

        if (fcnt > conn->SND.MAX) {
            QCC_DbgHLPrintf(("SendData(): number of fragments %d exceeds the window size %d", fcnt, conn->window));
            return ER_FAIL;
        }

    }

    /* Check if receiver's window is wide enough to accept FCNT number of segments */
    if (fcnt > conn->window) {
        QCC_DbgPrintf(("SendData(): number of fragments %d exceeds the window size %d", fcnt, conn->window));
        return ER_ARDP_BACKPRESSURE;
    }

    /* Check if send queue is deep enough to hold FCNT number of segments */
    if (fcnt > (conn->SND.MAX - conn->SBUF.pending)) {
        QCC_DbgPrintf(("SendData(): number of fragments %d exceeds the send queue depth %d", fcnt, conn->SND.MAX - conn->SBUF.pending));
        return ER_ARDP_BACKPRESSURE;
    }

    for (uint16_t i = 0; i < fcnt; i++) {

        ArdpHeader* h = (ArdpHeader*) snd->hdr;
        uint16_t segLen = (i == (fcnt - 1)) ? lastLen : conn->SBUF.maxDlen;
        uint16_t retries = handle->config.dataRetries + 1;

        QCC_DbgPrintf(("SendData: Segment %d, SND.NXT=%u, SND.UNA=%u, RCV.CUR=%u", i, conn->SND.NXT, conn->SND.UNA, conn->RCV.CUR));
        assert((conn->SND.NXT - conn->SND.UNA) < conn->SND.MAX);

        h->flags = ARDP_FLAG_ACK | ARDP_FLAG_VER;
        h->som = som;
        h->hlen = conn->sndHdrLen / 2;
        h->fcnt = htons(fcnt);
        h->src = htons(conn->local);
        h->dst = htons(conn->foreign);;
        h->dlen = htons(segLen);
        h->seq = htonl(conn->SND.NXT);
        snd->ttl = ttl;
        snd->tStart = now;
        snd->data = segData;
        snd->datalen = segLen;
        if (h->dst == 0) {
            QCC_DbgPrintf(("SendData(): destination = 0"));
        }

        assert(((conn->SBUF.pending) < conn->SND.MAX) && "Number of pending segments in send queue exceeds MAX!");
        QCC_DbgPrintf(("SendData(): updated send queue at index %d", index));

        status = SendMsgData(handle, conn, snd);

        if (status == ER_WOULDBLOCK) {
            QCC_DbgPrintf(("SendData(): ER_WOULDBLOCK"));
            timeout = 0; /* Schedule next time around */
            retries++;   /* Since that won't be a legitimate retransmit, increase number of retries by 1 */
            status = ER_OK;
        } else {
            timeout = GetRTO(handle, conn);
        }

        /* We change update our accounting only if the message has been sent successfully. */
        if (status == ER_OK) {
            snd->inUse = true;
            UpdateTimer(handle, conn, &snd->timer, timeout, retries);
            /* Since we scheduled a retransmit timer, cancel active persist timer */
            conn->persistTimer.retry = 0;
            EnList(conn->dataTimers.bwd, (ListNode*) &snd->timer);
            conn->SBUF.pending++;
            conn->SND.NXT++;
        } else if (status != ER_ARDP_TTL_EXPIRED) {
            /* Something irrevocably bad happened on the socket. Disconnect. */
            Disconnect(handle, conn, status);
            break;
        }

        segData += segLen;
        snd = snd->next;
    }

    return status;
}

static QStatus DoSendSyn(ArdpHandle* handle, ArdpConnRecord* conn, bool synack, uint32_t seq, uint32_t ack, uint16_t segmax, uint16_t segbmax, uint8_t* buf, uint16_t len)
{
    ArdpSynSegment* ss = &conn->synSnd.ss;
    qcc::ScatterGatherList msgSG;
    size_t sent;

    QCC_DbgTrace(("DoSendSyn(handle=%p, conn=%p, synack=%u, seq=%u, ack=%u, segmax=%d, segbmax=%d, buf=%p, len = %d)",
                  handle, conn, synack, seq, ack, segmax, segbmax, buf, len));

    ss->flags = ARDP_FLAG_SYN | ARDP_FLAG_VER;

    if (synack) {
        ss->flags |= ARDP_FLAG_ACK;
    }

    ss->hlen = sizeof(ArdpSynSegment) / 2;
    ss->src = htons(conn->local);
    ss->dst = htons(conn->foreign);
    ss->dlen = htons(len);
    ss->seq = htonl(seq);
    ss->ack = htonl(ack);
    ss->segmax = htons(segmax);
    ss->segbmax = htons(segbmax);
    ss->options = htons(ARDP_FLAG_SDM);

    if (ss->dst == 0) {
        QCC_DbgPrintf(("DoSendSyn(): destination = 0"));
    }

    conn->synSnd.data = (uint8_t*) malloc(len);
    if (conn->synSnd.data == NULL) {
        return ER_OUT_OF_MEMORY;
    }

    conn->synSnd.dataLen = len;
    memcpy(conn->synSnd.data, buf, len);

    InitTimer(handle, conn, &conn->connectTimer, ConnectTimerHandler, &conn->synSnd, handle->config.connectTimeout, handle->config.connectRetries + 1);
    QCC_DbgPrintf(("DoSendSyn(): timer=%p, retries=%u", conn->connectTimer, conn->connectTimer.retry));
    QCC_DbgPrintf(("DoSendSyn(): ss->seq=%u  data=%p (%s), len=%u", ntohl(ss->seq), conn->synSnd.data, conn->synSnd.data, conn->synSnd.dataLen));

    msgSG.AddBuffer(&conn->synSnd.ss, sizeof(ArdpSynSegment));
    msgSG.AddBuffer(conn->synSnd.data, conn->synSnd.dataLen);
    return qcc::SendToSG(conn->sock, conn->ipAddr, conn->ipPort, msgSG, sent);
}

static QStatus SendSyn(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t iss, uint16_t segmax, uint16_t segbmax, uint8_t* buf, uint16_t len)
{
    QCC_DbgTrace(("SendSyn(handle=%p, conn=%p, iss=%d, segmax=%d, segbmax=%d, buf=%p, len=%d)", handle, conn, iss, segmax, segbmax, buf, len));
    SetState(conn, SYN_SENT);
    return DoSendSyn(handle, conn, false, iss, 0, segmax, segbmax, buf, len);
}

static QStatus SendSynAck(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t seq, uint32_t ack, uint16_t recvmax, uint16_t recvbmax, uint8_t* buf, uint16_t len)
{
    QCC_DbgTrace(("SendSynAck(handle=%p, conn=%p, seq=%d, ack=%d, recvmax=%d, recvbmax=%d, buf=%p, len=%d)", handle, conn, seq, ack, recvmax, recvbmax, buf, len));
    return DoSendSyn(handle, conn, true, seq, ack, recvmax, recvbmax, buf, len);
}

static QStatus SendRst(ArdpHandle* handle, qcc::SocketFd sock, qcc::IPAddress ipAddr, uint16_t ipPort, uint16_t local, uint16_t foreign)
{
    QCC_DbgTrace(("SendRst(handle=%p, sock=%d., ipAddr=\"%s\", ipPort=%d., local=%d., foreign=%d.)",
                  handle, sock, ipAddr.ToString().c_str(), ipPort, local, foreign));

    ArdpHeader h;
    h.flags = ARDP_FLAG_RST | ARDP_FLAG_VER;
    h.hlen = ARDP_FIXED_HEADER_LEN / 2;
    h.src = htons(local);
    h.dst = htons(foreign);
    h.dlen = 0;
    h.seq = 0;
    h.ack = 0;

    QCC_DbgPrintf(("SendRst(): SendTo(sock=%d., ipAddr=\"%s\", port=%d., buf=%p, len=%d", sock, ipAddr.ToString().c_str(), ipPort, &h, ARDP_FIXED_HEADER_LEN));

    size_t sent;
    return qcc::SendTo(sock, ipAddr, ipPort, &h, ARDP_FIXED_HEADER_LEN, sent);
}

static void UpdateSndSegments(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t ack, uint32_t lcs) {
    QCC_DbgTrace(("UpdateSndSegments(): handle=%p, conn=%p, ack=%u, lcs=%u", handle, conn, ack, lcs));
    uint16_t index = ack % conn->SND.MAX;
    ArdpSndBuf* snd = &conn->SBUF.snd[index];

    /* Nothing to clean up */
    if (conn->SBUF.pending == 0) {
        conn->SND.LCS = lcs;
        return;
    }

    /*
     * Count only "good" roundrips to ajust RTT values.
     * Note, that we decrement retries with each retransmit.
     */
    if (snd->timer.retry == (handle->config.dataRetries + 1)) {
        AdjustRTT(handle, conn, snd);
    }

    assert(SEQ32_LET(lcs, conn->SND.UNA));

    /* Cycle through the buffers from SND.LCS + 1 to ACK */
    index = (conn->SND.LCS + 1) % conn->SND.MAX;
    snd = &conn->SBUF.snd[index];
    for (uint32_t i = 0; i < (ack - conn->SND.LCS); i++) {
        ArdpHeader* h = (ArdpHeader*) snd->hdr;
        uint32_t seq = ntohl(h->seq);
        uint16_t fcnt = ntohs(h->fcnt);

        assert(snd->inUse);
        assert(SEQ32_LET(seq, ack));
        if (snd->inUse) {
            /* If fragmented, wait for the last segment. Issue sendCB on the first fragment in message.*/
            QCC_DbgPrintf(("UpdateSndSegments(): fragment=%u, som=%u, fcnt=%d",
                           ntohl(h->seq), ntohl(h->som), fcnt));

            QCC_DbgPrintf(("UpdateSndSegments(): stop timer %p", &snd->timer));

            if (snd->timer.retry != 0) {
                DeList((ListNode*) &snd->timer);
                snd->timer.retry = 0;
            }

            /*
             * If the message has been consumed by the receiver,
             * check if this is the last fragment in the message and issue SendCb()
             */
            if (SEQ32_LET(seq, lcs) && seq == (ntohl(h->som) + (fcnt - 1))) {
                QCC_DbgPrintf(("UpdateSndSegments(): last fragment=%u, som=%u, fcnt=%d",
                               seq, ntohl(h->som), fcnt));

                /* First segment in message, keeps the original pointer to message buffer */
                index = ntohl(h->som) % conn->SND.MAX;
                FlushMessage(handle, conn, &conn->SBUF.snd[index], ER_OK);
            }
        }

        snd = snd->next;
    }

    /* Update the counter on our SND side */
    conn->SND.LCS = lcs;
}

static void FastRetransmit(ArdpHandle* handle, ArdpConnRecord* conn, ArdpSndBuf* snd)
{
    /* Fast retransmit to fill the gap.*/
    if (snd->fastRT == handle->config.dupackCounter) {
        QCC_DbgPrintf(("FastRetransmit(): priority re-send %u", ntohl(((ArdpHeader*)snd->hdr)->seq)));
        snd->timer.when = TimeNow(handle->tbase);
    }
    snd->fastRT++;
}

static void CancelEackedSegments(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t* bitMask) {
    QCC_DbgTrace(("CancelEackedSegments(): handle=%p, conn=%p, bitMask=%p", handle, conn, bitMask));
    uint32_t start = conn->SND.UNA;
    uint32_t index =  start % conn->SND.MAX;
    ArdpSndBuf* snd = &conn->SBUF.snd[index];

#ifndef NDEBUG
    DumpBitMask(conn, bitMask, conn->remoteMskSz, true);
#endif
    FastRetransmit(handle, conn, snd);

    /*
     * Bitmask starts at SND.UNA + 1. Cycle through the mask and cancel retransmit timers
     * on EACKed segments.
     */
    start = start + 1;
    for (uint32_t i = 0; i < conn->remoteMskSz; i++) {
        uint32_t mask32 = ntohl(bitMask[i]);
        uint32_t bitCheck = 1 << 31;

        index = (start + (i * 32)) % conn->SND.MAX;
        snd = &conn->SBUF.snd[index];
        while (mask32 != 0) {
            if (mask32 & bitCheck) {
                QCC_DbgPrintf(("CancelEackedSegments(): set retries to zero for timer %p", snd->timer));
                if (snd->timer.retry != 0) {
                    DeList((ListNode*) &snd->timer);
                    snd->timer.retry = 0;
                }
            } else if (i < 1) {
                /*
                 * Schedule fast retransmits for gaps in the first 32-segment window.
                 * Catch the others next time the EACK window moves.
                 */
                FastRetransmit(handle, conn, snd);
            }

            mask32 = mask32 << 1;
            snd = snd->next;
        }
    }
}

static void UpdateRcvMsk(ArdpConnRecord* conn, uint32_t delta)
{
    QCC_DbgPrintf(("UpdateRcvMsk: delta = %d", delta));
    if (conn->rcvMsk.sz == 0) {
        /* No EACKS */
        return;
    }

    /* First bit represents RCV.CUR + 2 */
    uint32_t skip = delta / 32;
    uint32_t lshift = delta % 32;
    uint32_t rshift = 32 - lshift;
    uint32_t saveBits = 0;
    uint16_t newSz = 0;

    /* Shift the bitmask */
    conn->rcvMsk.mask[0] = conn->rcvMsk.mask[skip] << lshift;
    if (conn->rcvMsk.mask[0] > 0) {
        newSz = 1;
    }

    for (uint32_t i = skip + 1; i < conn->rcvMsk.sz; i++) {
        if (conn->rcvMsk.mask[i] == 0) {
            continue;
        }
        saveBits = conn->rcvMsk.mask[i] >> rshift;
        conn->rcvMsk.mask[i] = conn->rcvMsk.mask[i] << lshift;
        conn->rcvMsk.mask[i - 1] =  conn->rcvMsk.mask[i - 1] | saveBits;
        if (conn->rcvMsk.mask[i] > 0) {
            newSz = i - skip;
        }
        conn->rcvMsk.htnMask[i - 1] = htonl(conn->rcvMsk.mask[i - 1]);
        conn->rcvMsk.htnMask[i] = htonl(conn->rcvMsk.mask[i]);
    }
    conn->rcvMsk.htnMask[0] = htonl(conn->rcvMsk.mask[0]);
    conn->rcvMsk.sz = newSz;
}

static void AddRcvMsk(ArdpConnRecord* conn, uint32_t delta)
{
    QCC_DbgPrintf(("AddRcvMsk: delta = %d", delta));
    /* First bit represents RCV.CUR + 2 */
    uint32_t bin32 = (delta - 1) / 32;
    uint32_t offset = (uint32_t)32 - (delta - (bin32 << 5));

    assert(bin32 < conn->rcvMsk.fixedSz);
    conn->rcvMsk.mask[bin32] = conn->rcvMsk.mask[bin32] | ((uint32_t)1 << offset);
    QCC_DbgPrintf(("AddRcvMsk: bin = %d, offset=%d, mask = 0x%f", bin32, offset, conn->rcvMsk.mask[bin32]));
    if (conn->rcvMsk.sz < bin32 + 1) {
        conn->rcvMsk.sz = bin32 + 1;
    }
    conn->rcvMsk.htnMask[bin32] = htonl(conn->rcvMsk.mask[bin32]);
}

static QStatus UpdateRcvBuffers(ArdpHandle* handle, ArdpConnRecord* conn, ArdpRcvBuf* consumed)
{
    ArdpRcvBuf* rcv = conn->RBUF.rcv;
    uint16_t index;
    uint32_t count = consumed->fcnt;

    QCC_DbgTrace(("UpdateRcvBuffers(handle=%p, conn=%p, consumed=%p)", handle, conn, consumed));

    index = consumed->seq % conn->RCV.MAX;
    if (&rcv[index] != consumed) {
        QCC_DbgHLPrintf(("UpdateRcvBuffers: released buffer %p (seq=%u) does not match rcv %p @ %u", consumed, consumed->seq, &rcv[index], index));
        assert(0 && "UpdateRcvBuffers: Buffer sequence validation failed");
        return ER_FAIL;
    }

    if (consumed->fcnt < 1) {
        QCC_DbgHLPrintf(("Invalid fragment count %d", consumed->fcnt));
        assert((consumed->fcnt >= 1) && "fcnt cannot be less than one!");
        return ER_FAIL;
    }

    /* Release the current buffers associated with the consumed message. */
    QCC_DbgPrintf(("UpdateRcvBuffers: conn->RCV.LCS=%u should be less than seq=%u", conn->RCV.LCS, consumed->seq));
    assert(SEQ32_LT(conn->RCV.LCS, consumed->seq));
    conn->RCV.LCS = consumed->seq + (count - 1);
    QCC_DbgPrintf(("UpdateRcvBuffers: conn->RCV.LCS=%u", conn->RCV.LCS));

    for (uint32_t i = 0; i < count; i++) {
        consumed->isDelivered = false;
        consumed->inUse = false;
        QCC_DbgPrintf(("UpdateRcvBuffers: released buffer %p (seq=%u)", consumed, consumed->seq));

        if (consumed->data != NULL) {
            free(consumed->data);
            consumed->data = NULL;
        }
        consumed = consumed->next;
    }

    /* Advance LCS till next valid segment */
    while (consumed->ttl == ARDP_TTL_EXPIRED && !consumed->isDelivered && SEQ32_LT(conn->RCV.LCS, conn->RCV.CUR)) {
        conn->RCV.LCS++;
        consumed = consumed->next;
        QCC_DbgPrintf(("UpdateRcvBuffers: advanced conn->RCV.LCS=%u", conn->RCV.LCS));
    }
    ;

    /* Send "unsolicited" ACK if the ACK timer is not running */
    if (conn->ackTimer.retry == 0) {
        UpdateTimer(handle, conn, &conn->ackTimer, ARDP_ACK_TIMEOUT, 1);
    }

    return ER_OK;
}

static uint32_t AdvanceRcvQueue(ArdpHandle* handle, ArdpConnRecord* conn, ArdpRcvBuf* current)
{
    uint32_t delta = 0;
    uint32_t first = current->seq;

    do {
        conn->RCV.CUR = current->seq;

        /*
         * Check if last fragment. If this is the case, re-assemble the message:
         * - Find the rcv, corresponding to SOM
         * - Make sure there are no gaps (shouldn't be the case we are in "ordered delivery" case here,
         *   assert on that), remove for release.
         * - RecvCb(SOM's rcvBuf, fcnt)
         */
        if (current->seq == current->som + (current->fcnt - 1)) {
            uint32_t tNow = TimeNow(handle->tbase);
            bool expired = false;
            ArdpRcvBuf* startFrag = &conn->RBUF.rcv[current->som % conn->RCV.MAX];
            ArdpRcvBuf* fragment = startFrag;

            /* Remove this check before release */
            for (uint32_t i = 0; i < current->fcnt; i++) {
                if (!fragment->inUse || fragment->isDelivered || fragment->som != startFrag->som || fragment->fcnt != startFrag->fcnt) {
                    QCC_LogError(ER_FAIL, ("Gap in fragmented (%d) message: start %u, this(%i): seq=%u inUse=%s, delivered = %s, som=%u, fcnt=%u",
                                           startFrag->fcnt, startFrag->som, i, fragment->seq, fragment->inUse ? "true" : "false",
                                           fragment->isDelivered ? "true" : "false", fragment->som, fragment->fcnt));
                }
                assert(fragment->inUse && "Gap in fragmented message");
                assert((fragment->som == startFrag->som && fragment->fcnt == startFrag->fcnt) && "Lost track of received fragment");
                fragment = fragment->next;
            }

            /*
             * Mark all the fragments as delivered, and while we're at it note
             * whether or not the message has expired.
             */
            fragment = startFrag;
            if ((fragment->ttl == ARDP_TTL_EXPIRED)  || (fragment->ttl != ARDP_TTL_INFINITE && (tNow - fragment->tRecv >= fragment->ttl))) {
                QCC_DbgPrintf(("ArdpRcvBuffer(): Detected expired message (conn=0x%p, seq=%u)", conn, fragment->seq));
                expired = true;
            }

            /*
             * If the message has expired, don't send it up to the upper layer,
             * just recycle it, otherwise pass it off to the transport.
             */
            if (expired) {
                startFrag->ttl = ARDP_TTL_EXPIRED;

                /* If this message is first in line to be released, flush it*/
                if ((conn->RCV.LCS + 1) == startFrag->seq) {
                    UpdateRcvBuffers(handle, conn, startFrag);
                }
            } else {
                QCC_DbgPrintf(("ArdpRcvBuffer(): RecvCb(conn=0x%p, buf=%p, seq=%u, fcnt (@ %p)=%d)", conn, startFrag, startFrag->seq, &(startFrag->fcnt), startFrag->fcnt));
                handle->cb.RecvCb(handle, conn, startFrag, ER_OK);
            }
        }

        current = current->next;
        /* Remove this check before release. Detect where the message starts */
        if (current->seq == current->som) {
            QCC_DbgPrintf(("ArdpRcvBuffer(): next message starts @ %u", current->som));
        }

        delta++;

    } while (current->seq == (first + delta));

    return delta;

}

static void FlushExpiredRcvMessages(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t acknxt)
{
    QCC_DbgTrace(("FlushExpiredRcvMessages(handle=%p, conn=%p, acknxt=%u", handle, conn, acknxt));
    uint16_t index = (conn->RCV.CUR + 1) % conn->RCV.MAX;
    ArdpRcvBuf* current = &conn->RBUF.rcv[index];
    uint32_t delta = 0;

    /* Move to the start of the message */
    ArdpRcvBuf* start = &conn->RBUF.rcv[current->som % conn->RCV.MAX];
    uint32_t fcnt = start->fcnt;
    current = start;
    index = current->seq;
    do {
        if (!current->isDelivered) {
            if (current->data != NULL) {
                free(current->data);
                current->data = NULL;
            }
            current->inUse = false;
        }
        current->ttl = ARDP_TTL_EXPIRED;
        current = current->next;
        delta++;
        index++;
    } while (SEQ32_LT(index, acknxt));

    /* in case the sender expired the entire window, nothing to do, just zero out EACK bitmask*/
    if ((acknxt - conn->RCV.CUR) >= conn->RCV.MAX) {
        memset(conn->rcvMsk.mask, 0, conn->rcvMsk.sz * sizeof(uint32_t));
        memset(conn->rcvMsk.htnMask, 0, conn->rcvMsk.sz * sizeof(uint32_t));
    } else {
        /* Update EACK bitmask */
        UpdateRcvMsk(conn, delta);

        /* If there are pending complete messages, deliver them to upper layer */
        if (current->inUse) {
            AdvanceRcvQueue(handle, conn, current);
        }
    }

    /* If nothing is pending "consumed" confirmation from upper layer, advance left edge */
    if (conn->RCV.LCS == conn->RCV.CUR) {
        conn->RCV.CUR = acknxt - 1;
    }

    /* Next segment to expect will be ACKNXT */
    conn->RCV.CUR = acknxt - 1;

}

static QStatus AddRcvBuffer(ArdpHandle* handle, ArdpConnRecord* conn, ArdpSeg* seg, uint8_t* buf, uint16_t len, bool ordered)
{
    uint16_t index = seg->SEQ % conn->RCV.MAX;
    ArdpRcvBuf* current = &conn->RBUF.rcv[index];
    uint16_t hdrlen = conn->rcvHdrLen;

    QCC_DbgTrace(("AddRcvBuffer(handle=%p, conn=%p, seg=%p, buf=%p, len=%d, ordered=%s", handle, conn, seg, buf, len, (ordered ? "true" : "false")));

    QCC_DbgPrintf(("AddRcvBuffer: seg->SEQ = %u, first=%u, last= %u", seg->SEQ, conn->RCV.LCS + 1, conn->RBUF.last));
    /* Sanity check */
    if (hdrlen != (len - seg->DLEN)) {
        QCC_DbgHLPrintf(("AddRcvBuffer: hdrlen=%d does not match (len-DLEN)=%d", hdrlen, len - seg->DLEN));
        return ER_FAIL;
    }

    if (seg->DLEN > conn->RBUF.MAX) {
        QCC_DbgHLPrintf(("AddRcvBuffer: data len %d exceeds SEGBMAX %d", seg->DLEN, conn->RBUF.MAX));
        return ER_FAIL;
    }
    assert(buf != NULL && len != 0);

    if ((current->inUse) && !(current->inUse && current->seq == seg->SEQ)) {
        QCC_DbgHLPrintf(("AddRcvBuffer: attempt to overwrite buffer that has not been released in use %u seg->seq %u",
                         current->seq, seg->SEQ));
        assert((!(current->inUse) || (current->inUse && current->seq == seg->SEQ)) &&
               "AddRcvBuffer: attempt to overwrite buffer that has not been released");
        return ER_FAIL;
    }


    if (current->seq == seg->SEQ) {
        QCC_DbgPrintf(("AddRcvBuffer: duplicate segment %u, acknowledge", seg->SEQ));
        return ER_OK;
    }

    /* Allocate holding buffer, pad up to 1K */
    current->data = (uint8_t*) malloc((((seg->DLEN + 1023) >> 10) << 10) * sizeof(uint8_t));
    if (current->data == NULL) {
        QCC_LogError(ER_OUT_OF_MEMORY, ("Failed to allocate rcv data buffer"));
        return ER_OUT_OF_MEMORY;
    }

    if (SEQ32_LT(conn->RBUF.last, seg->SEQ)) {
        assert((seg->SEQ - conn->RBUF.last) < conn->RCV.MAX);
        conn->RBUF.last = seg->SEQ;
    }

    current->seq = seg->SEQ;
    current->datalen = seg->DLEN;
    current->inUse = true;
    memcpy(current->data, buf + hdrlen, seg->DLEN);

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
        uint32_t delta = AdvanceRcvQueue(handle, conn, current);
        UpdateRcvMsk(conn, delta);

    } else {
        AddRcvMsk(conn, (seg->SEQ - (conn->RCV.CUR + 1)));
    }

#ifndef NDEBUG
    DumpBitMask(conn, conn->rcvMsk.mask, conn->rcvMsk.fixedSz, false);
#endif
    return ER_OK;
}

static QStatus InitSBUF(ArdpHandle* handle, ArdpConnRecord* conn)
{
    uint8_t* buffer;
    uint8_t hdrlen;
    /* IP header size plus UDP header size */
    uint32_t ipHeaderSize = (conn->ipAddr.IsIPv4()) ? IPV4_HEADER_SIZE : IPV6_HEADER_SIZE;
    uint32_t overhead = ipHeaderSize + UDP_HEADER_SIZE;
    uint32_t ackMaskSize = (conn->RCV.MAX + 31) >> 5;

    QCC_DbgPrintf(("InitSBUF(conn=%p)", conn));
    /* Fixed header size on send side. Should correspond to header size on remote's receive size. */
    hdrlen = ARDP_FIXED_HEADER_LEN + ackMaskSize * sizeof(uint32_t);
    conn->sndHdrLen = hdrlen;
    conn->rcvMsk.fixedSz = ackMaskSize;
    QCC_DbgPrintf(("InitSBUF(): max header len %d actual send header len %d", ARDP_MAX_HEADER_LEN, hdrlen));

    conn->SBUF.maxDlen = conn->SBUF.MAX - overhead - hdrlen;
    QCC_DbgPrintf(("InitSBUF(): actual max payload len %d", conn->SBUF.maxDlen));

    if (conn->SBUF.MAX < (overhead + hdrlen)) {
        QCC_DbgPrintf(("InitSBUF(): Provided max segment size too small %d (need at least %d)", conn->SBUF.MAX, (overhead + hdrlen)));
        return ER_FAIL;
    }

    conn->SBUF.snd = (ArdpSndBuf*) malloc(conn->SND.MAX * sizeof(ArdpSndBuf));
    if (conn->SBUF.snd == NULL) {
        QCC_DbgPrintf(("InitSBUF(): Failed to allocate SBUF"));
        return ER_OUT_OF_MEMORY;
    }
    memset(conn->SBUF.snd, 0, conn->SND.MAX * sizeof(ArdpSndBuf));

    /* Allocate contiguous space for sent data segment headers */
    buffer = (uint8_t*) malloc(conn->SND.MAX * hdrlen);

    if (buffer == NULL) {
        QCC_DbgPrintf(("InitSBUF(): Failed to allocate Send buffer"));
        free(conn->SBUF.snd);
        return ER_OUT_OF_MEMORY;
    }
    memset(buffer, 0, conn->SND.MAX * hdrlen);

    /* Array of pointers to headers of unAcked sent data buffers */
    for (uint32_t i = 0; i < conn->SND.MAX; i++) {
        InitTimer(handle, conn, &conn->SBUF.snd[i].timer, RetransmitTimerHandler, &conn->SBUF.snd[i], handle->config.dataTimeout, 0);
        conn->SBUF.snd[i].next = &conn->SBUF.snd[(i + 1) % conn->SND.MAX];
        conn->SBUF.snd[i].hdr = buffer;
        conn->SBUF.snd[i].hdrlen = hdrlen;
        buffer += hdrlen;
    }

    /* Calculate the minimum send window necessary to accomodate the largest message */
    conn->minSendWindow = (ALLJOYN_MAX_PACKET_LEN + (conn->SBUF.maxDlen - 1)) / conn->SBUF.maxDlen;
    QCC_DbgPrintf(("InitSBUF(): minSendWindow=%d", conn->minSendWindow));
    return ER_OK;
}

static void ArdpMachine(ArdpHandle* handle, ArdpConnRecord* conn, ArdpSeg* seg, uint8_t* buf, uint16_t len)
{
    QStatus status;

    QCC_DbgTrace(("ArdpMachine(handle=%p, conn=%p, seg=%p, buf=%p, len=%d)", handle, conn, seg, buf, len));

    switch (conn->STATE) {
    case CLOSED:
        {
            QCC_DbgPrintf(("ArdpMachine(): conn->STATE = CLOSED"));

            if (seg->FLG & ARDP_FLAG_RST) {
                QCC_DbgPrintf(("ArdpMachine(): CLOSED: RST on a closed connection"));
                break;
            }

            if (seg->FLG & ARDP_FLAG_ACK || seg->FLG & ARDP_FLAG_NUL) {
                QCC_DbgPrintf(("ArdpMachine(): CLOSED: Probe or ACK on a closed connection"));
                /*
                 * <SEQ=SEG>ACK + 1><RST>
                 */
                Send(handle, conn, ARDP_FLAG_RST | ARDP_FLAG_VER, 0, seg->ACK + 1, conn->RCV.MAX);
                break;
            }

            QCC_DbgPrintf(("ArdpMachine(): CLOSED: Unexpected segment on a closed connection"));
            /*
             * <SEQ=0><RST><ACK=RCV.CUR><ACK>
             */
            Send(handle, conn, ARDP_FLAG_RST | ARDP_FLAG_ACK | ARDP_FLAG_VER, 0, seg->SEQ, conn->RCV.MAX);
            break;
        }

    case LISTEN:
        {
            QCC_DbgPrintf(("ArdpMachine(): conn->STATE = LISTEN"));

            if (seg->FLG & ARDP_FLAG_RST) {
                QCC_DbgPrintf(("ArdpMachine(): LISTEN: RST on a LISTENinig connection"));
                break;
            }

            if (seg->FLG & ARDP_FLAG_ACK || seg->FLG & ARDP_FLAG_NUL) {
                QCC_DbgPrintf(("ArdpMachine(): LISTEN: Foreign host ACKing a Listening connection"));
                /*
                 * <SEQ=SEG.ACK + 1><RST>
                 */
                Send(handle, conn, ARDP_FLAG_RST | ARDP_FLAG_VER, seg->ACK + 1, 0, 0);
                Disconnect(handle, conn, ER_ARDP_INVALID_RESPONSE);
                break;
            }

            if (seg->FLG & ARDP_FLAG_SYN) {
                QCC_DbgPrintf(("ArdpMachine(): LISTEN: SYN received.  Accepting"));
                conn->RCV.CUR = seg->SEQ;
                conn->RCV.IRS = seg->SEQ;
                conn->window = conn->SND.MAX;

                /* Fixed size of EACK bitmask */
                conn->remoteMskSz = ((conn->SND.MAX + 31) >> 5);
                conn->rcvHdrLen = ARDP_FIXED_HEADER_LEN + conn->remoteMskSz * sizeof(uint32_t);
                QCC_DbgPrintf(("ArdpMachine(): LISTEN: SYN received: rcvHdrLen=%d", conn->rcvHdrLen));

                QCC_DbgPrintf(("ArdpMachine(): LISTEN: SYN received: the other side can receive max %d bytes", conn->SBUF.MAX));
                if (handle->cb.AcceptCb != NULL) {
                    uint8_t* data = buf + sizeof(ArdpSynSegment);
                    if (handle->cb.AcceptCb(handle, conn->ipAddr, conn->ipPort, conn, data, seg->DLEN, ER_OK) == false) {
                        QCC_DbgPrintf(("ArdpMachine(): LISTEN: SYN received. AcceptCb() returned \"false\""));
                        Send(handle, conn, ARDP_FLAG_RST | ARDP_FLAG_VER, seg->ACK + 1, 0, conn->RCV.MAX);
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
            QCC_DbgPrintf(("ArdpMachine(): conn->STATE = SYN_SENT"));

            if (seg->FLG & ARDP_FLAG_RST) {
                QCC_DbgPrintf(("ArdpMachine(): SYN_SENT: connection refused. state -> CLOSED"));
                Disconnect(handle, conn, ER_ARDP_REMOTE_CONNECTION_RESET);
                break;
            }

            if (seg->FLG & ARDP_FLAG_SYN) {
                ArdpSynSegment* ss = (ArdpSynSegment*) buf;
                QCC_DbgPrintf(("ArdpMachine(): SYN_SENT: SYN received"));
                conn->SND.MAX = ntohs(ss->segmax);
                conn->remoteMskSz = ((seg->MAX + 31) >> 5);
                conn->rcvHdrLen = ARDP_FIXED_HEADER_LEN + conn->remoteMskSz * sizeof(uint32_t);
                QCC_DbgPrintf(("ArdpMachine(): SYN_SENT: SYN received: rcvHdrLen=%d, remoteMskSz=%d", conn->rcvHdrLen, conn->remoteMskSz));
                conn->window = conn->SND.MAX;
                conn->foreign = seg->SRC;

                conn->RCV.IRS = seg->SEQ;
                conn->RCV.CUR = seg->SEQ;
                conn->RCV.LCS = seg->SEQ;
                conn->SBUF.MAX = ntohs(ss->segbmax);
                QCC_DbgPrintf(("ArdpMachine(): SYN_SENT: the other side can receive max %d bytes", conn->SBUF.MAX));
                status = InitSBUF(handle, conn);

                assert(status == ER_OK && "ArdpMachine():SYN_SENT: Failed to initialize Send queue");

                if (seg->FLG & ARDP_FLAG_ACK) {
                    QCC_DbgPrintf(("ArdpMachine(): SYN_SENT: SYN | ACK received. state -> OPEN"));
                    conn->SND.UNA = seg->ACK + 1;
                    PostInitRcv(conn);
                    SetState(conn, OPEN);

                    /* Stop connect retry timer */
                    conn->connectTimer.retry = 0;

                    /* Initialize and kick off link timeout timer */
                    conn->lastSeen = TimeNow(handle->tbase);
                    InitTimer(handle, conn, &conn->probeTimer, ProbeTimerHandler, NULL, handle->config.probeTimeout, handle->config.probeRetries);

                    /* Initialize persist (dead window) timer */
                    InitTimer(handle, conn, &conn->persistTimer, PersistTimerHandler, NULL, handle->config.persistTimeout, 0);

                    /* Initialize delayed ACK timer */
                    InitTimer(handle, conn, &conn->ackTimer, AckTimerHandler, NULL, ARDP_ACK_TIMEOUT, 0);

                    /*
                     * <SEQ=SND.NXT><ACK=RCV.CUR><ACK>
                     */
                    Send(handle, conn, ARDP_FLAG_ACK | ARDP_FLAG_VER, conn->SND.NXT, conn->RCV.CUR, conn->RCV.MAX);

                    if (handle->cb.ConnectCb) {
                        QCC_DbgPrintf(("ArdpMachine(): SYN_SENT->OPEN: ConnectCb(handle=%p, conn=%p", handle, conn));
                        assert(!conn->passive);
                        uint8_t* data = buf + (seg->HLEN * 2);
                        handle->cb.ConnectCb(handle, conn, false, data, seg->DLEN, ER_OK);
                        if (conn->synSnd.data != NULL) {
                            free(conn->synSnd.data);
                            conn->synSnd.data = NULL;
                        }
                    }
                } else {
                    QCC_DbgPrintf(("ArdpMachine(): SYN_SENT: SYN with no ACK implies simulateous connection attempt: state -> SYN_RCVD"));
                    uint8_t* data = buf + sizeof(ArdpSynSegment);
                    if (handle->cb.AcceptCb != NULL) {
                        handle->cb.AcceptCb(handle, conn->ipAddr, conn->ipPort, conn, data, seg->DLEN, ER_OK);
                    }
                }
                break;
            }

            if (seg->FLG & ARDP_FLAG_ACK) {
                if (seg->ACK != conn->SND.ISS) {
                    QCC_DbgPrintf(("ArdpMachine(): SYN_SENT: ACK does not ASK ISS"));
                    Disconnect(handle, conn, ER_ARDP_INVALID_RESPONSE);
                    break;
                }
            }

            break;
        }

    case SYN_RCVD:
        {
            QCC_DbgPrintf(("ArdpMachine(): conn->STATE = SYN_RCVD"));

            if (IN_RANGE(uint32_t, conn->RCV.CUR + 1, conn->RCV.MAX, seg->SEQ) == false) {
                QCC_DbgPrintf(("ArdpMachine(): SYN_RCVD: unacceptable sequence %u", seg->SEQ));
                /* <SEQ=SND.NXT><ACK=RCV.CUR><ACK> */
                Send(handle, conn, ARDP_FLAG_ACK | ARDP_FLAG_VER, conn->SND.NXT, conn->RCV.CUR, conn->RCV.MAX);
                break;
            }

            if (seg->FLG & ARDP_FLAG_RST) {
                if (conn->passive) {
                    QCC_DbgPrintf(("ArdpMachine(): SYN_RCVD: Got RST during passive open.  state -> LISTEN"));
                    SetState(conn, LISTEN);
                } else {
                    QCC_DbgPrintf(("ArdpMachine(): SYN_RCVD: Got RST during active open.  state -> CLOSED"));
                    Disconnect(handle, conn, ER_ARDP_REMOTE_CONNECTION_RESET);
                }
                break;
            }

            if (seg->FLG & ARDP_FLAG_SYN) {
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
                if (seg->ACK == conn->SND.ISS) {

                    QCC_DbgPrintf(("ArdpMachine(): SYN_RCVD: Got ACK with correct acknowledge.  state -> OPEN"));
                    PostInitRcv(conn);
                    SetState(conn, OPEN);

                    /* Stop connect retry timer */
                    conn->connectTimer.retry = 0;

                    /* Initialize and kick off link timeout timer */
                    conn->lastSeen = TimeNow(handle->tbase);
                    InitTimer(handle, conn, &conn->probeTimer, ProbeTimerHandler, NULL, handle->config.probeTimeout, handle->config.probeRetries);

                    /* Initialize persist (dead window) timer */
                    InitTimer(handle, conn, &conn->persistTimer, PersistTimerHandler, NULL, handle->config.persistTimeout, 0);


                    /* Initialize delayed ACK timer */
                    InitTimer(handle, conn, &conn->ackTimer, AckTimerHandler, NULL, ARDP_ACK_TIMEOUT, 0);

                    if (seg->FLG & ARDP_FLAG_NUL) {
                        Send(handle, conn, ARDP_FLAG_ACK | ARDP_FLAG_VER, conn->SND.NXT, conn->RCV.CUR, conn->RCV.LCS);
                    }

                    if (handle->cb.ConnectCb) {
                        QCC_DbgPrintf(("ArdpMachine(): SYN_RCVD->OPEN: ConnectCb(handle=%p, conn=%p", handle, conn));
                        assert(conn->passive);
                        uint8_t* data = buf + (seg->HLEN * 2);
                        handle->cb.ConnectCb(handle, conn, true, data, seg->DLEN, ER_OK);
                        if (conn->synSnd.data != NULL) {
                            free(conn->synSnd.data);
                            conn->synSnd.data = NULL;
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
            QCC_DbgPrintf(("ArdpMachine(): conn->STATE = OPEN"));

            if (IN_RANGE(uint32_t, conn->RCV.LCS + 1, conn->RCV.MAX, seg->SEQ) == false) {
                QCC_DbgPrintf(("ArdpMachine(): OPEN: unacceptable sequence %u, conn->RCV.CUR + 1 = %u, MAX = %d", seg->SEQ, conn->RCV.CUR + 1, conn->RCV.MAX));
                if (seg->DLEN != 0) {
#ifndef NDEBUG
                    DumpBuffer(buf, len);
#endif
                    break;
                }
            }

            if (seg->FLG & ARDP_FLAG_RST) {
                QCC_DbgPrintf(("ArdpMachine(): OPEN: got RST, disconnect"));
                Disconnect(handle, conn, ER_ARDP_REMOTE_CONNECTION_RESET);
                break;
            }

            if (seg->FLG & ARDP_FLAG_SYN) {
                QCC_DbgPrintf(("ArdpMachine(): OPEN: Got SYN, disconnect"));
                Disconnect(handle, conn, ER_ARDP_INVALID_RESPONSE);
                break;
            }

            if (SEQ32_LT(conn->RCV.CUR + 1, seg->ACKNXT)) {
                QCC_DbgPrintf(("ArdpMachine(): OPEN: FlushExpiredRcvMessages: expected %u got %u",
                               conn->RCV.CUR + 1, seg->ACKNXT));
                FlushExpiredRcvMessages(handle, conn, seg->ACKNXT);
            }

            /* If we got NUL segment, send ACK without delay */
            if (seg->FLG & ARDP_FLAG_NUL) {
                QCC_DbgPrintf(("ArdpMachine(): OPEN: got NUL, send LCS %u", conn->RCV.LCS));
                status = Send(handle, conn, ARDP_FLAG_ACK | ARDP_FLAG_VER, conn->SND.NXT, conn->RCV.CUR, conn->RCV.LCS);
                if (status == ER_OK && conn->ackTimer.retry != 0) {
                    UpdateTimer(handle, conn, &conn->ackTimer, ARDP_ACK_TIMEOUT, 1);
                    conn->RBUF.ackPending = 0;
                } else if (status != ER_OK) {
                    UpdateTimer(handle, conn, &conn->ackTimer, 0, 1);
                }
                break;
            }

            if (seg->FLG & ARDP_FLAG_ACK) {
                QCC_DbgPrintf(("ArdpMachine(): OPEN: Got ACK %u LCS %u Window %u", seg->ACK, seg->LCS, seg->WINDOW));
                if ((IN_RANGE(uint32_t, conn->SND.UNA, ((conn->SND.NXT - conn->SND.UNA) + 1), seg->ACK) == true) ||
                    (conn->SND.LCS != seg->LCS)) {
                    conn->SND.UNA = seg->ACK + 1;
                    UpdateSndSegments(handle, conn, seg->ACK, seg->LCS);
                }
            }

            if (seg->FLG & ARDP_FLAG_EACK) {
                /*
                 * Flush the segments using EACK
                 */
                QCC_DbgPrintf(("ArdpMachine(): OPEN: EACK is set"));
                CancelEackedSegments(handle, conn, (uint32_t* ) (buf + ARDP_FIXED_HEADER_LEN));
            }

            if (seg->DLEN) {
                QCC_DbgPrintf(("ArdpMachine(): OPEN: Got %d bytes of Data with SEQ %u, RCV.CUR = %u).", seg->DLEN, seg->SEQ, conn->RCV.CUR));
                status = ER_OK;
                /*
                 * Update RCV buffers if the segment is not a duplicate (accounting for a case when
                 * receiving a retransmit of a segment with sequence number between LCS and CUR).
                 */

                if (SEQ32_LT(conn->RCV.CUR, seg->SEQ)) {
                    status = AddRcvBuffer(handle, conn, seg, buf, len, seg->SEQ == (conn->RCV.CUR + 1));
                    conn->RBUF.ackPending++;
                }

                /*
                 * ACKS can be scheduled based on timeout value or number of received segments
                 * pending acknowledgement.
                 * In future, make the above parameters (i.e., timeout & pending acks) configurable.
                 */
                if (conn->ackTimer.retry == 0) {
                    UpdateTimer(handle, conn, &conn->ackTimer, ARDP_ACK_TIMEOUT, 1);
                } else if (conn->RBUF.ackPending >= ((conn->RCV.MAX >> 1) + 1)) {
                    UpdateTimer(handle, conn, &conn->ackTimer, 0, 1);
                }

            }

            if (conn->window != seg->WINDOW) {
                /* Schedule persist timer only if there are no pending retransmits */
                if (IsEmpty((ListNode*) &conn->dataTimers) && (seg->WINDOW < conn->minSendWindow) && (conn->persistTimer.retry == 0)) {
                    /* Start Persist Timer */
                    UpdateTimer(handle, conn, &conn->persistTimer, handle->config.persistTimeout, handle->config.persistRetries + 1);
                } else if ((conn->persistTimer.retry != 0) && (seg->WINDOW >= conn->minSendWindow || !IsEmpty((ListNode*) &conn->dataTimers))) {
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
            QCC_DbgPrintf(("ArdpMachine(): conn->STATE = CLOSE_WAIT"));
            /*
             * Ignore segment with set RST.
             * The transition to CLOSED is based on delay TIMWAIT only.
             */
            break;
        }

    default:
        assert(0 && "ArdpMachine(): unexpected conn->STATE %d");
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

    ArdpConnRecord* conn = NewConnRecord();
    QStatus status;

    status = InitConnRecord(handle, conn, sock, ipAddr, ipPort, 0);
    if (status != ER_OK) {
        return status;
    }

    status = InitRcv(conn, segmax, segbmax); /* Initialize the receiver side of the connection */
    if (status != ER_OK) {
        delete conn;
        return status;
    }

    conn->context = context;
    conn->passive = false;

    EnList(handle->conns.bwd, (ListNode*)conn);
    *pConn = conn;
    return SendSyn(handle, conn, conn->SND.ISS, conn->RCV.MAX, conn->RBUF.MAX, buf, len);
}

QStatus ARDP_Accept(ArdpHandle* handle, ArdpConnRecord* conn, uint16_t segmax, uint16_t segbmax, uint8_t* buf, uint16_t len)
{
    QCC_DbgTrace(("ARDP_Accept(handle=%p, conn=%p, segmax=%d, segbmax=%d, buf=%p (%s), len=%d)",
                  handle, conn, segmax, segbmax, buf, buf, len));
    QStatus status;
    if (!IsConnValid(handle, conn)) {
        return ER_ARDP_INVALID_CONNECTION;
    }

    status = InitRcv(conn, segmax, segbmax); /* Initialize the receiver side of the connection */
    if (status == ER_OK) {
        status = InitSBUF(handle, conn);
    }

    if (status != ER_OK) {
        Send(handle, conn, ARDP_FLAG_RST | ARDP_FLAG_VER, conn->SND.UNA, 0, conn->RCV.MAX);
        SetState(conn, CLOSED);
        DelConnRecord(handle, conn, false);
        return status;
    }

    SetState(conn, SYN_RCVD);
    /* <SEQ=SND.ISS><ACK=RCV.CUR><MAX=RCV.MAX><BUFMAX=RBUF.MAX><ACK><SYN> */
    SendSynAck(handle, conn, conn->SND.ISS, conn->RCV.CUR, conn->RCV.MAX, conn->RBUF.MAX, buf, len);
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
    return UpdateRcvBuffers(handle, conn, rcv);
}

QStatus ARDP_Send(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t* buf, uint32_t len, uint32_t ttl)
{
    QCC_DbgTrace(("ARDP_Send(handle=%p, conn=%p, buf=%p, len=%d., ttl=%d.)", handle, conn, buf, len, ttl));
    if (!IsConnValid(handle, conn)) {
        return ER_ARDP_INVALID_CONNECTION;
    }

    if (conn->STATE != OPEN) {
        return ER_ARDP_INVALID_STATE;
    }

    if (buf == NULL || len == 0) {
        return ER_INVALID_DATA;
    }

    QCC_DbgPrintf(("NXT=%u, UNA=%u, window=%d", conn->SND.NXT, conn->SND.UNA, conn->window));
    if ((conn->window == 0)  || (conn->SND.NXT - conn->SND.UNA) >= conn->SND.MAX) {
        QCC_DbgPrintf(("NXT - UNA=%u", conn->SND.NXT - conn->SND.UNA));
        return ER_ARDP_BACKPRESSURE;
    } else {
        return SendData(handle, conn, buf, len, ttl);
    }
}

static QStatus Receive(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t* buf, uint16_t len)
{
    QCC_DbgTrace(("Receive(handle=%p, conn=%p, buf=%p, len=%d)", handle, conn, buf, len));
    ArdpHeader* header = (ArdpHeader*)buf;
    ArdpSeg SEG;
    SEG.FLG = header->flags;        /* The flags of the current segment */
    SEG.HLEN = header->hlen;        /* The header len */
    if (!(SEG.FLG & ARDP_FLAG_SYN) && ((SEG.HLEN * 2) != conn->rcvHdrLen)) {
        QCC_DbgHLPrintf(("Receive: seg.len = %d, expected = %d", (SEG.HLEN * 2), conn->rcvHdrLen));
        return ER_ARDP_INVALID_RESPONSE;
    }
    SEG.SRC = ntohs(header->src);       /* The source ARDP port */
    SEG.DST = ntohs(header->dst);       /* The destination ARDP port */
    SEG.SEQ = ntohl(header->seq);       /* The send sequence of the current segment */
    SEG.ACK = ntohl(header->ack);       /* The cumulative acknowledgement number to our sends */
    SEG.MAX = conn->RCV.MAX;            /* The maximum number of outstanding segments the receiver (we) are willing to hold */
    SEG.BMAX = conn->SBUF.MAX;          /* The maximmum number of bytes the other side (the sender) can receive */
    SEG.DLEN = ntohs(header->dlen);     /* The amount of data in this segment */
    SEG.LCS = ntohl(header->lcs);       /* The last consumed segment on receiver side (them) */
    SEG.WINDOW = conn->SND.MAX - (conn->SND.NXT - (SEG.LCS + 1)); /* The receivers window */
    QCC_DbgPrintf(("Receive() window=%d", SEG.WINDOW));
    SEG.ACKNXT = ntohl(header->acknxt); /* The first valid segment sender wants to be acknowledged */
    SEG.TTL = ntohl(header->ttl);       /* TTL associated with this segment */
    SEG.SOM = ntohl(header->som);       /* Sequence number of the first fragment in message */
    SEG.FCNT = ntohs(header->fcnt);     /* Number of segments comprising fragmented message */

    ArdpMachine(handle, conn, &SEG, buf, len);
    return ER_OK;
}

QStatus Accept(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t* buf, uint16_t len)
{
    QCC_DbgTrace(("Accept(handle=%p, conn=%p, buf=%p, len=%d)", handle, conn, buf, len));
    assert(conn->STATE == CLOSED && "Accept(): ConnRecord in invalid state");

    ArdpSynSegment* syn = (ArdpSynSegment*)buf;
    if (syn->flags != (ARDP_FLAG_SYN | ARDP_FLAG_VER)) {
        return ER_FAIL;
    }

    ArdpSeg SEG;
    SEG.FLG =  syn->flags;                     /* The flags indicating the SYN */
    SEG.SRC =  ntohs(syn->src);                /* The source ARDP port */
    SEG.DST =  ntohs(syn->dst);                /* The destination ARDP port */
    SEG.SEQ =  ntohl(syn->seq);                /* The sequence number from the other side.  In this case the ISS */
    SEG.ACK =  ntohl(syn->ack);                /* The ack number from the other side.  Unspecified */
    SEG.MAX =  conn->SND.MAX = ntohs(syn->segmax);   /* Max number of unacknowledged packets other side can buffer */
    SEG.BMAX = conn->SBUF.MAX = ntohs(syn->segbmax); /* Max size segment the other side can handle. In conn it means how much data we can send */

    SEG.BMAX = conn->SBUF.MAX = ntohs(syn->segbmax);     /* Max size segment the other side can handle. In conn it means how much data we can send */
    QCC_DbgPrintf(("Accept:SEG.BMAX = conn->SBUF.MAX = %d", ntohs(syn->segbmax)));

    SEG.DLEN = ntohs(syn->dlen);               /* The data length included in the packet. */
    conn->STATE = LISTEN;                      /* The call to Accept() implies a jump to LISTEN */
    conn->foreign = SEG.SRC;                   /* Now that we have the SYN, we have the foreign address */
    conn->passive = true;                      /* This connection is (will be) the result of a passive open */

    ArdpMachine(handle, conn, &SEG, buf, len);

    return ER_OK;
}

QStatus ARDP_Run(ArdpHandle* handle, qcc::SocketFd sock, bool socketReady, uint32_t* ms)
{
    QCC_DbgTrace(("ARDP_Run(handle=%p, sock=%d., socketReady=%d., ms=%p)", handle, sock, socketReady, ms));
    uint8_t buf[65536];                   /* UDP packet can be up to 64K long */
    qcc::IPAddress address;               /* The IP address of the foreign side */
    uint16_t port;                        /* Will be the UDP port of the foreign side */
    size_t nbytes;                        /* The number of bytes actually received */
    QStatus status = ER_FAIL;

    *ms = handle->msnext = CheckTimers(handle);            /* When to call back (timer expiration) */
    if (socketReady) {
        status = qcc::RecvFrom(sock, address, port, buf, sizeof(buf), nbytes);
        if (status == ER_WOULDBLOCK) {
            QCC_DbgTrace(("ARDP_Run(): qcc::RecvFrom() ER_WOULDBLOCK"));
            return ER_OK;
        } else if (status != ER_OK) {
            QCC_DbgTrace(("ARDP_Run(): qcc::RecvFrom() failed: %s", QCC_StatusText(status)));
            return status;
        }

        if (nbytes > 0 && nbytes < 65536) {
            uint16_t local, foreign;
            ProtocolDemux(buf, nbytes, &local, &foreign);
            if (local == 0) {
                if (handle->accepting && handle->cb.AcceptCb) {
                    ArdpConnRecord* conn = NewConnRecord();
                    status = InitConnRecord(handle, conn, sock, address, port, foreign);
                    if (status == ER_OK) {
                        EnList(handle->conns.bwd, (ListNode*)conn);
                        status = Accept(handle, conn, buf, nbytes);
                    }
                } else {
                    status = SendRst(handle, sock, address, port, local, foreign);
                }
            } else {
                /* Is there an open connection? */
                ArdpConnRecord* conn = FindConn(handle, local, foreign);
                if (conn) {
                    conn->lastSeen = TimeNow(handle->tbase);
                    assert(conn->lastSeen != 0);
                    status = Receive(handle, conn, buf, nbytes);
                } else {
                    /* Is there a half open connection? */
                    conn = FindConn(handle, local, 0);
                    if (conn) {
                        conn->lastSeen = TimeNow(handle->tbase);
                        status = Receive(handle, conn, buf, nbytes);
                    }
                }
                /* Ignore anything else */
            }
        }
    }

    *ms = handle->msnext;
    QCC_DbgTrace(("ARDP_Run %u", *ms));

    return status;
}

} // namespace ajn
