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

#define USE_SG_LIST 1

#if USE_SG_LIST
#include "ScatterGatherList.h"
#else
#include <errno.h>
#endif /* USE_SG_LIST */

#include "ArdpProtocol.h"

#define QCC_MODULE "ARDP_PROTOCOL"

namespace ajn {

#define ARDP_MIN_LEN 120

#define ARDP_RETRANSMIT_TIMEOUT 500
#define ARDP_URGENT_RETRANSMIT_TIMEOUT (ARDP_RETRANSMIT_TIMEOUT >> 2)
#define ARDP_RETRANSMIT_RETRY 4
#define ARDP_DISCONNECT_RETRY 1  /* Not configurable, no retries */

#define ARDP_TTL_EXPIRED    0xffffffff
#define ARDP_TTL_MAX        (ARDP_TTL_EXPIRED - 1)
#define ARDP_TTL_INFINITE   0

/**
 * A simple circularly linked list node suitable for use in thin core library implementations.
 */
typedef struct LISTNODE {
    struct LISTNODE* fwd;
    struct LISTNODE* bwd;
    uint8_t*         buf;
    uint16_t len;
} ListNode;

typedef void (*ArdpTimeoutHandler)(ArdpHandle* handle, ArdpConnRecord* conn, void* context);

enum ArdpTimerType {
    DISCONNECT_TIMER = 1,
    CONNECT_TIMER,
    RETRANSMIT_TIMER,
    WINDOW_CHECK_TIMER,
    PROBE_TIMER
};

typedef struct {
    ListNode list;
    ArdpTimeoutHandler handler;
    ArdpConnRecord* conn;
    ArdpTimerType type;
    void* context;
    uint32_t delta;
    uint32_t when;
    uint16_t retry;
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
    uint32_t NXT;         /* The sequence number of the next segment that is to be sent. */
    uint32_t UNA;         /* The sequence number of the oldest unacknowledged segment. */
    uint32_t MAX;         /* The maximum number of unacknowledged segments that can be sent. */
    uint32_t ISS;         /* The initial send sequence number.  The number that was sent in the SYN segment. */
} ArdpSnd;

/**
 * Structure encapsulating the information about segments on SEND side.
 */
typedef struct ARDP_SEND_BUF {
    uint8_t* data;
    uint32_t datalen;
    uint8_t* hdr;
    ArdpTimer* timer;
    uint32_t ttl;
    uint32_t tStart;
    bool onTheWire;
    bool inUse;
    uint16_t hdrlen;
} ArdpSndBuf;

/**
 * Structure encapsulating the receive-related quantities.  The stuff managed on
 * the remote/foreign side, copies of which we may get from THEM.
 */
typedef struct {
    uint32_t CUR;     /* The sequence number of the last segment received correctly and in sequence */
    uint32_t MAX;     /* The maximum number of segments that can be buffered for this connection */
    uint32_t IRS;     /* The initial receive sequence number.  The sequence number of the SYN that established the connection */
} ArdpRcv;

/**
 * Structure encapsulating information about our send buffers.
 */
typedef struct {
    uint32_t MAX;       /* The largest possible segment that THEY can receive (our send buffer, specified by the other side during connection) */
    ArdpSndBuf* snd;     /* Array holding unacknowledged sent buffers */
    uint16_t maxDlen;     /* Maximum data payload size that can be sent without partitioning */
    uint16_t pending;     /* Number of unacknowledged sent buffers */
} ArdpSbuf;

/**
 * Structure encapsulating information about our receive buffers.
 */
typedef struct {
    uint32_t MAX;       /* The largest possible segment that WE can receive (our receive buffer, specified by our user on an open) */
    ArdpRcvBuf* rcv;     /* Array holding received buffers not consumed by the app */
    uint32_t first;     /* Sequence number of the first pending segment */
    uint32_t last;      /* Sequence number of the last pending segment */
    uint16_t window;     /* Receive Window */
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
    uint32_t BMAX;     /* The maximum segment size accepted by the foreign host on a connection (specified in SYN). */
    uint32_t RVALID;     /* The first valid RCV segment, TTL accounting */
    uint32_t SVALID;     /* The first valid SND segment, TTL accounting */
    uint32_t SOM;     /* Start sequence number for fragmented message */
    uint16_t FCNT;     /* Number of fragments comprising a message */
    uint16_t DLEN;     /* The length of the data that came in with the current segment. */
    uint16_t DST;     /* The destination port in the header of the segment currently being processed. */
    uint16_t SRC;     /* The source port in the header of the segment currently being processed. */
    uint16_t WINDOW;     /* The current number of segments that the other side is currently able to hold. Range 0 - MAX. */
    uint32_t TTL;     /* Time-to-live */
    uint8_t FLG;     /* The flags in the header of the segment currently being processed. */
    uint8_t HLEN;     /* The header length */
} ArdpSeg;

/**
 * The states through which our main state machine transitions.
 */
enum ArdpState {
    CLOSED = 1,     /* No connection exists and no connection record available */
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
    uint16_t window;     /* The current receive window */
    uint16_t segmax;     /* The maximum number of outstanding segments the other side can send without acknowledgement. */
    uint16_t segbmax;     /* The maximum segment size we are willing to receive.  (the RBUF.MAX specified by the user calling open). */
    uint16_t options;     /* Options for the connection.  Always Sequenced Delivery Mode (SDM). */
} ArdpSynSegment;

typedef struct {
    ArdpSynSegment ss;
    uint8_t* data;       /* Place to hold connection handshake data (SASL, HELLO, etc) */
    uint32_t dataLen;     /* Length of connection handshake data */
} ArdpSynSnd;

/**
 * A connection record describing each "connection."  This acts as a containter
 * to hold all of the interesting information about a reliable link between
 * hosts.
 */
struct ARDP_CONN_RECORD {
    ListNode list;          /* Doubly linked list node on which this connection might be */
    ArdpState STATE;        /* The current sate of the connection */
    bool passive;           /* If true, this is a passive open (we've been connected to); if false, we did the connecting */
    ArdpSnd SND;            /* Send-side related state information */
    ArdpSbuf SBUF;          /* Information about send buffers */
    ArdpRcv RCV;            /* Receive-side related state information */
    ArdpRbuf RBUF;          /* Information about receive buffers */
    uint16_t local;         /* ARDP local port for this connection */
    uint16_t foreign;       /* ARDP foreign port for this connection */
    qcc::SocketFd sock;     /* A convenient copy of socket we use to communicate. */
    qcc::IPAddress ipAddr;     /* A convenient copy of the IP address of the foreign side of the connection. */
    uint16_t ipPort;        /* A convenient copy of the IP port of the foreign side of the connection. */
    uint16_t window;        /* Current send window, dynamic setting */
    uint16_t minSendWindow;     /* Minimum send window needed to accomodate max message */
    uint16_t sndHdrLen;     /* Length of send ARDP header on this connection */
    uint16_t rcvHdrLen;     /* Length of receive ARDP header on this connection */
    ArdpRcvMsk rcvMsk;      /* Tracking of received out-of-order segments */
    uint16_t remoteMskSz;     /* Size of of EACK bitmask present in received segment */
    uint32_t lastSeen;      /* Last time we received communication on this connection. */
    ListNode timers;        /* List of currently scheduled timeout callbacks */
    ArdpSynSnd synSnd;      /* Connection establishment data */
    void* context;          /* A client-defined context pointer */
};

struct ARDP_HANDLE {
    ArdpGlobalConfig config;     /* The configurable items that affect this instance of ARDP as a whole */
    ArdpCallbacks cb;            /* The callbacks to allow the protocol to talk back to the client */
    bool accepting;              /* If true the ArdpProtocol is accepting inbound connections */
    ListNode conns;              /* List of currently active connections */
    qcc::Timespec tbase;         /* Baseline time */
    void* context;               /* A client-defined context pointer */
};

/*
 * Important!!! All our numbers are within window size, so below calculation will be valid.
 * If necessary, can add check that delta between the numbers does not exceed half-range.
 */
#define SEQ32_LT(a, b) ((int32_t)((uint32_t)(a) - (uint32_t)(b)) < 0)
#define SEQ32_LET(a, b) (((int32_t)((uint32_t)(a) - (uint32_t)(b)) < 0) || (a == b))

/**
 * Inside window calculation.
 * Returns true if p is in range [beg, beg+sz)
 * This function properly accounts for possible wrap-around in [beg, beg+sz) region.
 */
#define IN_RANGE(tp, beg, sz, p) (((static_cast<tp>((beg) + (sz)) > (beg)) && ((p) >= (beg)) && ((p) < static_cast<tp>((beg) + (sz)))) || \
                                  ((static_cast<tp>((beg) + (sz)) < (beg)) && !(((p) < (beg)) && (p) >= static_cast<tp>((beg) + (sz)))))


/******************
 * Test hooks
 ******************/

#define TEST_DROP_SEGMENTS 0
#define TEST_SEQ32_WRAPAROUND 0

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
    assert(!IsEmpty(node) && "DeList(): called on empty list");
    node->bwd->fwd = node->fwd;
    node->fwd->bwd = node->bwd;
    node->fwd = node->bwd = node;
}

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
                       (mask32 >> 23) & 1, (mask32 >> 23) & 1, (mask32 >> 21) & 1, (mask32 >> 20) & 1,
                       (mask32 >> 19) & 1, (mask32 >> 18) & 1, (mask32 >> 17) & 1, (mask32 >> 16) & 1,
                       (mask32 >> 15) & 1, (mask32 >> 14) & 1, (mask32 >> 13) & 1, (mask32 >> 12) & 1,
                       (mask32 >> 11) & 1, (mask32 >> 10) & 1, (mask32 >> 9) & 1, (mask32 >> 8) & 1,
                       (mask32 >> 7) & 1, (mask32 >> 6) & 1, (mask32 >> 5) & 1, (mask32 >> 4) & 1,
                       (mask32 >> 3) & 1, (mask32 >> 2) & 1, (mask32 >> 1) & 1, mask32 & 1));

    }
}

static void DumpSndInfo(ArdpConnRecord* conn)
{
    QCC_DbgPrintf(("DumpSndInfo(conn=%p)", conn));
    QCC_DbgPrintf(("\tmaxDlen=%d, size=%d, pending=%d, free=%d", conn->SBUF.maxDlen, conn->SND.MAX, conn->SBUF.pending, (conn->SND.MAX - conn->SBUF.pending)));
    for (uint32_t i = 0; i < conn->SND.MAX; i++) {
        ArdpHeader* h = (ArdpHeader*) conn->SBUF.snd[i].hdr;
        uint32_t seq = (h != NULL) ? ntohl(h->seq) : 0;
        QCC_DbgPrintf(("\t inUse=%d, seq=%u, hdr=%p, hdrlen=%d, data=%p, datalen=%d., ttl=%d., tStart=%d, onTheWire=%d, fcnt=%d, som %u.",
                       conn->SBUF.snd[i].inUse, seq, conn->SBUF.snd[i].hdr, conn->SBUF.snd[i].hdrlen,
                       conn->SBUF.snd[i].data, conn->SBUF.snd[i].datalen,
                       conn->SBUF.snd[i].ttl, conn->SBUF.snd[i].tStart, conn->SBUF.snd[i].onTheWire,
                       ntohs(h->fcnt), ntohl(h->som)));
    }
}

static QStatus InitSBUF(ArdpConnRecord* conn)
{
    uint8_t* buffer;
    uint8_t hdrLen;
    uint32_t overhead = 20 + 8;     /* IP header size minus UDP header size */
    uint32_t ackMaskSize = (conn->RCV.MAX + 31) >> 5;

    QCC_DbgPrintf(("InitSBUF(conn=%p)", conn));
    /* Fixed header size on send side. Should correspond to header size on remote's receive size. */
    hdrLen = ARDP_FIXED_HEADER_LEN + ackMaskSize * sizeof(uint32_t);
    conn->sndHdrLen = hdrLen;
    conn->rcvMsk.fixedSz = ackMaskSize;
    QCC_DbgPrintf(("InitSBUF(): max header len %d actual send header len %d", ARDP_MAX_HEADER_LEN, hdrLen));

    conn->SBUF.maxDlen = conn->SBUF.MAX - overhead - hdrLen;
    QCC_DbgPrintf(("InitSBUF(): actual max payload len %d", conn->SBUF.maxDlen));

    if (conn->SBUF.MAX < (overhead + hdrLen)) {
        QCC_DbgPrintf(("InitSBUF(): Provided max segment size too small %d (need at least %d)", conn->SBUF.MAX, (overhead + hdrLen)));
        return ER_FAIL;
    }

    conn->SBUF.snd = (ArdpSndBuf*) malloc(conn->SND.MAX * sizeof(ArdpSndBuf));
    if (conn->SBUF.snd == NULL) {
        QCC_DbgPrintf(("InitSBUF(): Failed to allocate SBUF"));
        return ER_OUT_OF_MEMORY;
    }
    memset(conn->SBUF.snd, 0, conn->SND.MAX * sizeof(ArdpSndBuf));

    /* Allocate contiguous space for sent data segment headers */
    buffer = (uint8_t*) malloc(conn->SND.MAX * hdrLen);

    if (buffer == NULL) {
        QCC_DbgPrintf(("InitSBUF(): Failed to allocate Send buffer"));
        free(conn->SBUF.snd);
        return ER_OUT_OF_MEMORY;
    }
    memset(buffer, 0, conn->SND.MAX * hdrLen);

    /* Array of pointers to headers of unAcked sent data buffers */
    for (uint32_t i = 0; i < conn->SND.MAX; i++) {
        conn->SBUF.snd[i].hdr = buffer;
        buffer += hdrLen;
    }

    /* Calculate the minimum send window necessary to accomodate the largest message */
    conn->minSendWindow = (ALLJOYN_MAX_PACKET_LEN + (conn->SBUF.maxDlen - 1)) / conn->SBUF.maxDlen;
    QCC_DbgPrintf(("InitSBUF(): minSendWindow=%d", conn->minSendWindow));
    return ER_OK;
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

static ArdpTimer* AddTimer(ArdpHandle* handle, ArdpConnRecord* conn, ArdpTimerType type, ArdpTimeoutHandler handler, void*context, uint32_t timeout, uint16_t retry)
{
    QCC_DbgTrace(("AddTimer: conn=%p type=%d timeout=%d", conn, type, timeout));

    ArdpTimer* timer = new ArdpTimer();

    timer->handler = handler;
    timer->context = context;
    timer->conn = conn;
    timer->type = type;
    timer->delta = timeout;
    timer->when = TimeNow(handle->tbase) + timeout;
    timer->retry = retry;
    EnList(conn->timers.bwd, (ListNode*) timer);

    return timer;
}

static void DeleteTimer(ArdpTimer*timer)
{
    QCC_DbgTrace(("DeleteTimer(timer=%p)", timer));
    DeList((ListNode*)timer);
    delete timer;
}

static void CancelTimer(ArdpConnRecord* conn, ArdpTimerType type, void* context)
{
    QCC_DbgTrace(("CancelTimer(conn=%p type=%d context=%p)", conn, type, context));

    for (ListNode* ln = &conn->timers; (ln = ln->fwd) != &conn->timers;) {
        ArdpTimer* timer = (ArdpTimer*) ln;
        if (timer == NULL) {
            break;
        }
        if (timer->type == type && (context == NULL || timer->context == context)) {
            DeleteTimer(timer);
            break;
        }
    }
}

static void CancelAllTimers(ArdpConnRecord* conn)
{
    QCC_DbgTrace(("CancelAllTimers: conn=%p", conn));

    for (ListNode* ln = &conn->timers; (ln = ln->fwd) != &conn->timers;) {
        ArdpTimer* timer = (ArdpTimer*) ln;
        ln = ln->bwd;
        DeleteTimer(timer);
    }
}

static uint32_t CheckConnTimers(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t next, uint32_t now)
{
    ListNode* ln = &conn->timers;

    if (IsEmpty(ln)) {
        return next;
    }

    for (; (ln = ln->fwd) != &conn->timers;) {
        ArdpTimer* timer = (ArdpTimer*)ln;

        if (timer == NULL) {
            break;
        }
        if ((timer->when <= now) && (timer->retry > 0)) {
            QCC_DbgPrintf(("CheckConnTimers: conn %p, head %p, Fire timer %p (type=%d) at %u (now=%u)",
                           conn, &conn->timers, timer, timer->type, timer->when, now));
            if (timer->handler != NULL) {
                (timer->handler)(handle, timer->conn, timer->context);
                /* Caution: Both Connect and Disconnect timeouts may result in canceling all the
                 * outstanding timers on the connection and removing the connection record. */
                if (!IsConnValid(handle, conn)) {
                    QCC_DbgPrintf(("CheckConnTimers: disconnected conn %p", conn));
                    break;
                }
            }
            timer->when = TimeNow(handle->tbase) + timer->delta;
        }

        if (timer->retry <= 0) {
            QCC_DbgPrintf(("CheckConnTimers: conn %x delete timer %x", conn, timer));
            ln = ln->bwd;
            DeleteTimer(timer);

            if (IsEmpty(&conn->timers)) {
                break;
            }

        } else if (timer->when > next) {
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

static void DelConnRecord(ArdpHandle* handle, ArdpConnRecord* conn)
{
    QCC_DbgTrace(("DelConnRecord(handle=%p conn=%p)", handle, conn));
    assert(conn->STATE == CLOSED && "DelConnRecord(): Delete while not CLOSED");

    /* Cancel any associated pending timers */
    CancelAllTimers(conn);
    /* Safe to check together as these buffers are always allocated together */
    if (conn->SBUF.snd != NULL && conn->SBUF.snd[0].hdr != NULL) {
        free(conn->SBUF.snd[0].hdr);
        free(conn->SBUF.snd);
    }
    if (conn->RBUF.rcv != NULL && conn->RBUF.rcv[0].data != NULL) {
        free(conn->RBUF.rcv[0].data);
        free(conn->RBUF.rcv);
    }
    DeList((ListNode*)conn);

    if (conn->synSnd.data != NULL) {
        free(conn->synSnd.data);
    }

    delete conn;
}

#if USE_SG_LIST
/* Don't need GetSockAddr, used internally by Socket.cc */
#else
static QStatus GetSockAddr(qcc::IPAddress ipAddr, uint16_t port,
                           struct sockaddr_storage* addrBuf, socklen_t& addrSize)
{
    if (ipAddr.IsIPv4()) {
        struct sockaddr_in sa;
        assert((size_t)addrSize >= sizeof(sa));
        memset(&sa, 0, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
        sa.sin_addr.s_addr = ipAddr.GetIPv4AddressNetOrder();
        addrSize = sizeof(sa);
        memcpy(addrBuf, &sa, sizeof(sa));
    } else {
        struct sockaddr_in6 sa;
        assert((size_t)addrSize >= sizeof(sa));
        memset(&sa, 0, sizeof(sa));
        sa.sin6_family = AF_INET6;
        sa.sin6_port = htons(port);
        sa.sin6_flowinfo = 0;
        ipAddr.RenderIPv6Binary(sa.sin6_addr.s6_addr, sizeof(sa.sin6_addr.s6_addr));
        sa.sin6_scope_id = 0;
        addrSize = sizeof(sa);
        memcpy(addrBuf, &sa, sizeof(sa));
    }
    return ER_OK;
}

static QStatus SendMsg(ArdpHandle* handle, ArdpConnRecord* conn, struct iovec*iov, size_t iovLen)
{
    QCC_DbgTrace(("SendMsg(): handle=0x%p, conn=0x%p, iov=0x%p, iovLen=%d.", handle, conn, iov, iovLen));

    struct msghdr msg;
    struct sockaddr_storage addr;
    socklen_t addrLen = sizeof(addr);
    ssize_t ret;
    QStatus status;

    status = GetSockAddr(conn->ipAddr, conn->ipPort, &addr, addrLen);
    //status = qcc::GetSockAddr(&addr, addrLen, conn->ipAddr, conn->ipPort);
    if (status != ER_OK) {
        return status;
    }

    msg.msg_name = (void*) &addr;
    msg.msg_namelen = addrLen;
    msg.msg_iov = iov;
    msg.msg_iovlen = iovLen;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;

    ret = sendmsg(conn->sock, &msg, 0);

    if (ret == -1) {
        if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK) {
            status = ER_WOULDBLOCK;
        } else {
            status = ER_FAIL;
        }
        QCC_LogError(status, ("SendMsg (sock = %u): %d ( %s )", conn->sock, errno, strerror(errno)));
    } else {
        size_t sent = (size_t) ret;
        QCC_DbgPrintf(("SendMsg sent %d", sent));
        status = ER_OK;
    }

    return status;
}
#endif /* USE_SG_LIST */

static void DisconnectTimerHandler(ArdpHandle* handle, ArdpConnRecord* conn, void* context)
{
    QCC_DbgTrace(("DisconnectTimerHandler: handle=%p conn=%p", handle, conn));

    /* Tricking the compiler */
    QStatus reason = *reinterpret_cast<QStatus*>(&context);

    assert((handle != NULL && conn != NULL) && "Handle and connection cannot be NULL");
    SetState(conn, CLOSED);
    if (handle->cb.DisconnectCb != NULL) {
        handle->cb.DisconnectCb(handle, conn, reason);
    }
    DelConnRecord(handle, conn);

}

static void ConnectTimerHandler(ArdpHandle* handle, ArdpConnRecord* conn, void* context)
{
    QCC_DbgTrace(("ConnectTimerHandler: handle=%p conn=%p", handle, conn));
    assert((handle != NULL && conn != NULL) && "Handle and connection cannot be NULL");
    QStatus status = ER_FAIL;
    ArdpTimer* timer = (ArdpTimer*) context;
    assert(timer != NULL);

    QCC_DbgTrace(("ConnectTimerHandler: retries left %d", timer->retry));

    if (timer->retry > 1) {
#if USE_SG_LIST
        qcc::ScatterGatherList msgSG;
        size_t sent;
        QCC_DbgPrintf(("ConnectTimerHandler: segbmax=%d", ntohs(conn->synSnd.ss.segbmax)));
        msgSG.AddBuffer(&conn->synSnd.ss, sizeof(ArdpSynSegment));
        msgSG.AddBuffer(conn->synSnd.data, conn->synSnd.dataLen);
        status = qcc::SendToSG(conn->sock, conn->ipAddr, conn->ipPort, msgSG, sent);
#else

        struct iovec iov[] = { { &conn->synSnd.ss, sizeof(ArdpSynSegment) }, { conn->synSnd.data, conn->synSnd.dataLen }};
        status = SendMsg(handle, conn, iov, ArraySize(iov));
#endif /* USE_SG_LIST */

        if (status == ER_WOULDBLOCK) {
            /* Retry sooner */
            timer->delta = handle->config.connectTimeout >> 2;
            status = ER_OK;
        } else if (status == ER_OK) {
            timer->delta = handle->config.connectTimeout;
        }
    }

    if (status != ER_OK) {
        if (handle->cb.ConnectCb != NULL) {
            handle->cb.ConnectCb(handle, conn, conn->passive, NULL, 0, ER_TIMEOUT);
        }
        SetState(conn, CLOSED);
        DelConnRecord(handle, conn);
    } else {
        timer->retry--;
    }
}

static QStatus SendMsgHeader(ArdpHandle* handle, ArdpConnRecord* conn, ArdpHeader* h)
{
    QCC_DbgTrace(("SendMsgHeader(): handle=0x%p, conn=0x%p, hdr=0x%p", handle, conn, h));
    if (conn->rcvMsk.sz != 0) {
        h->flags |= ARDP_FLAG_EACK;
        QCC_DbgPrintf(("SendMsgHeader: have EACKs flags = %2x", h->flags));
    }

#if USE_SG_LIST
    qcc::ScatterGatherList msgSG;
    size_t sent;

    msgSG.AddBuffer(h, ARDP_FIXED_HEADER_LEN);
    msgSG.AddBuffer(conn->rcvMsk.htnMask, conn->rcvMsk.fixedSz * sizeof(uint32_t));
    return qcc::SendToSG(conn->sock, conn->ipAddr, conn->ipPort, msgSG, sent);
#else
    struct iovec iov[] = { { h, ARDP_FIXED_HEADER_LEN }, { conn->rcvMsk.htnMask, conn->rcvMsk.fixedSz * sizeof(uint32_t) }};
    return SendMsg(handle, conn, iov, ArraySize(iov));
#endif /* USE_SG_LIST */

}

static QStatus SendMsgData(ArdpHandle* handle, ArdpConnRecord* conn, ArdpSndBuf* sndBuf)
{
    QCC_DbgTrace(("SendMsgData(): handle=0x%p, conn=0x%p, hdr=0x%p, hdrlen=%d., data=0x%p, datalen=%d., ttl=%d., tStart=%d., onTheWire=%d.",
                  handle, conn, sndBuf->hdr, sndBuf->hdrlen, sndBuf->data, sndBuf->datalen, sndBuf->ttl, sndBuf->tStart, sndBuf->onTheWire));
#if USE_SG_LIST

    qcc::ScatterGatherList msgSG;
    size_t sent;

    msgSG.AddBuffer(sndBuf->hdr, ARDP_FIXED_HEADER_LEN);
    msgSG.AddBuffer(conn->rcvMsk.htnMask, conn->rcvMsk.fixedSz * sizeof(uint32_t));
    msgSG.AddBuffer(sndBuf->data, sndBuf->datalen);

#else

    struct iovec iov[] = { { sndBuf->hdr, ARDP_FIXED_HEADER_LEN }, { conn->rcvMsk.htnMask, conn->rcvMsk.fixedSz * sizeof(uint32_t) }, { sndBuf->data, sndBuf->datalen }};

#endif /* USE_SG_LIST */

    ArdpHeader* h = (ArdpHeader*) sndBuf->hdr;

    h->ack = htonl(conn->RCV.CUR);
    h->window = htons(conn->RBUF.window);
    QCC_DbgPrintf(("SendMsgData(): seq = %u, window = %d", ntohl(h->seq), conn->RBUF.window));

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
     * One tricky bit here is that once a segment goes out on the wire, it
     * occupies a sequence number.  We can't just silently drop a segment after
     * it has been sent because if it is dropped in transmission, the other side
     * will still expect it to be valid and will ask for retransmission.
     *
     * TODO: We have a mechanism available to send IGN sequence numbers (like
     * EACKs but the in the other direction) in order to tell the other side to
     * forget about particular segments that expire during a retransmission
     * interval; but it is not implemented yet.  Temporarily, we continue to
     * reliably transmit messages that happen to expire during a retransmit
     * interval, but mark their ttl as expired; and they are dropped first
     * thing on the other side.
     */
    if (sndBuf->ttl != ARDP_TTL_INFINITE) {
        /*
         * There is a non-infinite time-to-live on this message
         */
        uint32_t msElapsed = TimeNow(handle->tbase) - sndBuf->tStart;

        /*
         * If the message has never been on the wire, it is trivial to drop.
         */
        if (sndBuf->onTheWire == false) {

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

                /*
                 * TODO: This is not an error, but this is here so we know when a
                 * message is dropped.  Remove before release.
                 */
                QCC_LogError(ER_TIMEOUT, ("SendMsgData(): Dropping expired message (conn=0x%p, buf=0x%p, len=%d.)",
                                          conn, sndBuf->data, sndBuf->datalen));
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
                /*
                 * Set ttl to a magic constant indicating that the message has
                 * actually expired.
                 *
                 * TODO: Don't send expired messages even if they have been on
                 * the wire.  In this case, add an ignore-this-sequence-number
                 * field to the next packet telling the other side not to ask
                 * for retransmission of the resulting sequence number gap.
                 */
                h->ttl = htonl(ARDP_TTL_EXPIRED);
            } else {
                /*
                 * Set ttl to the number of milliseconds remaining at the "instant"
                 * before it was transmitted.
                 */
                h->ttl = htonl(sndBuf->ttl - msElapsed);
            }
        }
    }

    sndBuf->onTheWire = true;

#if TEST_DROP_SEGMENTS
    static int drop = 0;
    drop++;
    if (!(drop % 4) || !((drop + 1) % 5)) {
        QCC_DbgPrintf(("SendMsgData: dropping %u", (ntohl) (h->seq)));
        return ER_OK;
    } else {
#if USE_SG_LIST
        return qcc::SendToSG(conn->sock, conn->ipAddr, conn->ipPort, msgSG, sent);
#else
        return SendMsg(conn, iov, ArraySize(iov));
#endif /* USE_SG_LIST */
    }

#else

#if USE_SG_LIST
    return qcc::SendToSG(conn->sock, conn->ipAddr, conn->ipPort, msgSG, sent);
#else
    return SendMsg(handle, conn, iov, ArraySize(iov));
#endif /* USE_SG_LIST */

#endif /* TEST_DROP_SEGMENTS */
}

static QStatus Send(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t flags, uint32_t seq, uint32_t ack, uint16_t window)
{
    QCC_DbgTrace(("Send(handle=%p, conn=%p, flags=0x%02x, seq=%d, ack=%d, window=%d)", handle, conn, flags, seq, ack, window));
    ArdpHeader h;
    h.flags = flags;
    h.hlen = conn->sndHdrLen / 2;
    h.src = htons(conn->local);
    h.dst = htons(conn->foreign);;
    h.dlen = 0;
    h.seq = htonl(seq);
    h.ack = htonl(ack);
    h.window = htons(window);

    if (h.dst == 0) {
        QCC_DbgPrintf(("Send(): destination = 0"));
    }

    return SendMsgHeader(handle, conn, &h);
}

static void RetransmitTimerHandler(ArdpHandle* handle, ArdpConnRecord* conn, void* context)
{
    QCC_DbgTrace(("RetransmitTimerHandler: handle=%p conn=%p context=%p", handle, conn, context));
    ArdpSndBuf* snd = (ArdpSndBuf*) context;
    assert(snd->inUse && "RetransmitTimerHandler: trying to resend flushed buffer");
    ArdpTimer* timer = snd->timer;

    if (timer->retry > 1) {
        QCC_DbgPrintf(("RetransmitTimerHandler: context=snd=%p retries=%d", snd, timer->retry));
        QStatus status = SendMsgData(handle, conn, snd);

        if (status == ER_WOULDBLOCK) {
            timer->delta = ARDP_URGENT_RETRANSMIT_TIMEOUT;
        } else if (status == ER_OK) {
            timer->delta = ARDP_RETRANSMIT_TIMEOUT;
        } else {
            QCC_LogError(status, ("Write to Socket went bad. Disconnect?"));
        }
        timer->retry--;
    } else {
        QCC_DbgPrintf(("RetransmitTimerHandler: context=snd=%p retries=%d", snd, timer->retry));
        ArdpHeader* h = (ArdpHeader*) snd->hdr;

        uint32_t len;
        uint8_t*buf;
        uint16_t fcnt = ntohs(h->fcnt);

        timer->retry = 0;
        snd->timer = NULL;

        /* Invalidate send buffer.
         * If this is a fragment, invalidate the whole message. */
        if (fcnt > 1) {
            uint32_t som = ntohl(h->som);
            uint16_t index = som % conn->RBUF.MAX;

            QCC_DbgPrintf(("RetransmitTimerHandler:  cancel message of %d fragments with  SOM=%u", fcnt, som));

            /* Pointer to the original message buffer */
            buf = conn->SBUF.snd[index].data;

            for (uint16_t i = 0; i < fcnt; i++) {
                h = (ArdpHeader*) conn->SBUF.snd[index].hdr;
                assert(((ntohs(h->fcnt) == fcnt) && (ntohl(h->som) == som)) && "RetransmitTimerHandler: Not a valid fragment!");

                conn->SBUF.snd[index].inUse = false;
                conn->SBUF.pending--;
                index = (index + 1) % conn->SND.MAX;
                assert((conn->SBUF.pending <= conn->SND.MAX) && "RetransmitTimerHandler: Number of pending segments exceeds max!");
                /* Cancel retransmit timers if any. Notice that the reference to the timer that fired up this handler
                 * was set to NULL prior to entering the for loop. Therefore it will not be canceled from underneath
                 * the execution flow. This timer will be taken care of by CheckConnTimers() based on retry==0
                 * once we get out of RetransmitTimerHandler() call. */
                if (conn->SBUF.snd[index].timer != NULL) {
                    /* Caution: Do not delete actual timers here. Timer cleann up will happen
                     * in CheckConnTimers() based on (retry <= 0) */
                    timer->retry = 0;
                    conn->SBUF.snd[index].timer = NULL;
                }
            }

            /* Original message length */
            len =  conn->SBUF.maxDlen * (fcnt - 1) + ntohs(h->dlen);

        } else {
            len = snd->datalen;
            buf = snd->data;
        }

        QCC_DbgPrintf(("RetransmitTimerHandler(): SendCb(handle=%p, conn=%p, buf=%p, len=%d, status=%d",
                       handle, conn, buf, len, ER_FAIL));
        handle->cb.SendCb(handle, timer->conn, buf, len, ER_FAIL);
    }

}

static QStatus Disconnect(ArdpHandle* handle, ArdpConnRecord* conn, QStatus reason)
{
    QCC_DbgTrace(("Disconnect(handle=%p, conn=%p, reason=%s)", handle, conn, QCC_StatusText(reason)));
    if (!IsConnValid(handle, conn) || conn->STATE == CLOSED || conn->STATE == CLOSE_WAIT) {
        return ER_ARDP_INVALID_STATE;
    }

    /* Is there a nice macro that would nicely wrap integer into pointer? Just to avoid nasty surprises... */
    assert(sizeof(QStatus) <=  sizeof(void*));
    if (conn->STATE == OPEN) {
        AddTimer(handle, conn, DISCONNECT_TIMER, DisconnectTimerHandler, (void*) reason, handle->config.timewait, ARDP_DISCONNECT_RETRY);
        SetState(conn, CLOSE_WAIT);
        return Send(handle, conn, ARDP_FLAG_RST | ARDP_FLAG_VER, conn->SND.NXT, conn->RCV.CUR, conn->RBUF.window);
    } else {
        SetState(conn, CLOSED);
        AddTimer(handle, conn, DISCONNECT_TIMER, DisconnectTimerHandler, (void*) reason, 0, ARDP_DISCONNECT_RETRY);
    }
    return ER_OK;
}

static void WindowCheckTimerHandler(ArdpHandle* handle, ArdpConnRecord* conn, void* context)
{
    ArdpTimer* timer = (ArdpTimer*) context;

    QCC_DbgTrace(("WindowCheckTimerHandler: handle=%p conn=%p context=%p delta %u retry %u",
                  handle, conn, context, timer->delta, timer->retry));

    if (conn->window < conn->minSendWindow) {
        if (timer->retry > 1) {
            QCC_DbgPrintf(("WindowCheckTimerHandler: send ping (NUL packet)"));
            QCC_DbgPrintf(("WindowCheckTimerHandler: window %u, need at least %u", conn->window, conn->minSendWindow));
            Send(handle, conn, ARDP_FLAG_ACK | ARDP_FLAG_VER | ARDP_FLAG_NUL, conn->SND.NXT, conn->RCV.CUR, conn->RBUF.window);
            timer->retry--;
        } else {
            QCC_LogError(ER_ARDP_PERSIST_TIMEOUT, ("WindowCheckTimerHandler: Persist Timeout frozen window %d (need %d)",
                                                   conn->window, conn->minSendWindow));
            Disconnect(handle, conn, ER_ARDP_PERSIST_TIMEOUT);
        }
    } else {
        timer->retry = handle->config.persistRetries;
    }
}

static void ProbeTimerHandler(ArdpHandle* handle, ArdpConnRecord* conn, void* context)
{
    ArdpTimer* timer = (ArdpTimer*) context;
    uint32_t now = TimeNow(handle->tbase);
    uint32_t elapsed = now - conn->lastSeen;
    /* Connection timeout */
    uint32_t linkTimeout = handle->config.probeTimeout * handle->config.probeRetries;

    QCC_DbgTrace(("ProbeTimerHandler: handle=%p conn=%p context=%p delta %u now %u lastSeen = %u elapsed %u",
                  handle, conn, context, timer->delta, now, conn->lastSeen, elapsed));
    if (elapsed >= linkTimeout) {

        QCC_LogError(ER_ARDP_PROBE_TIMEOUT, ("ProbeTimerHandler: Probe Timeout: now =%u, lastSeen = %u, elapsed=%u(vs limit of %u)", now, conn->lastSeen, elapsed, linkTimeout));
        Disconnect(handle, conn, ER_ARDP_PROBE_TIMEOUT);

    } else {
        QCC_DbgPrintf(("ProbeTimerHandler: send ping (NUL packet)"));
        Send(handle, conn, ARDP_FLAG_ACK | ARDP_FLAG_VER | ARDP_FLAG_NUL, conn->SND.NXT, conn->RCV.CUR, conn->RBUF.window);
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
            DelConnRecord(handle, (ArdpConnRecord*)tmp);
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

void* ARDP_GetHandleContext(ArdpHandle* handle)
{
    QCC_DbgTrace(("ARDP_GetHandleContext(handle=%p)", handle));
    return handle->context;
}

void ARDP_SetConnContext(ArdpConnRecord* conn, void* context)
{
    QCC_DbgTrace(("ARDP_SetConnContext(conn=%p, context=%p)", conn, context));
}

void* ARDP_GetConnContext(ArdpConnRecord* conn)
{
    QCC_DbgTrace(("ARDP_GetConnContext(conn=%p)", conn));
    return conn->context;
}

qcc::IPAddress ARDP_GetIpAddrFromConn(ArdpConnRecord* conn)
{
    QCC_DbgTrace(("ARDP_GetIpAddrFromConn()"));
    return conn->ipAddr;
}

uint16_t ARDP_GetIpPortFromConn(ArdpConnRecord* conn)
{
    QCC_DbgTrace(("ARDP_GetIpPortFromConn()"));
    return conn->ipPort;
}

static ArdpConnRecord* NewConnRecord(void)
{
    QCC_DbgTrace(("NewConnRecord()"));
    ArdpConnRecord* conn = new ArdpConnRecord();
    memset(conn, 0, sizeof(ArdpConnRecord));
    SetEmpty(&conn->list);
    return conn;
}

static void InitSnd(ArdpConnRecord* conn)
{
    QCC_DbgTrace(("InitSnd(conn=%p)", conn));

#if TEST_SEQ32_WRAPAROUND
    conn->SND.ISS = ((uint32_t)0xfffffff0) + (qcc::Rand32() % 4);             /* Initial sequence number used for sending data over this connection */
#else
    conn->SND.ISS = qcc::Rand32();             /* Initial sequence number used for sending data over this connection */
#endif
    conn->SND.NXT = conn->SND.ISS + 1;     /* The sequence number of the next segment to be sent over this connection */
    conn->SND.UNA = conn->SND.ISS;        /* The oldest unacknowledged segment is the ISS */
    conn->SND.MAX = 0;                    /* The maximum number of unacknowledged segments we can send (other side will tell us this */
}

static QStatus InitRcv(ArdpConnRecord* conn, uint32_t segmax, uint32_t segbmax)
{
    QCC_DbgTrace(("InitRcv(conn=%p, segmax=%d, segbmax=%d)", conn, segmax, segbmax));
    conn->RCV.MAX = segmax;     /* The maximum number of outstanding segments that we can buffer (we will tell other side) */
    conn->RBUF.MAX = segbmax;     /* The largest buffer that can be received on this connection (our buffer size) */

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
static void  PostInitRcv(ArdpConnRecord* conn)
{
    conn->RBUF.first = conn->RCV.CUR + 1;
    conn->RBUF.last = conn->RCV.CUR + 1;
    for (uint16_t i = 0; i < conn->RCV.MAX; i++) {
        conn->RBUF.rcv[i].seq = conn->RCV.IRS;
    }
}

static void InitConnRecord(ArdpHandle* handle, ArdpConnRecord* conn, qcc::SocketFd sock, qcc::IPAddress ipAddr, uint16_t ipPort, uint16_t foreign)
{
    QCC_DbgTrace(("InitConnRecord(handle=%p, conn=%p, sock=%d, ipAddr=\"%s\", ipPort=%d, foreign=%d)",
                  handle, conn, sock, ipAddr.ToString().c_str(), ipPort, foreign));

    conn->STATE = CLOSED;                  /* Starting state is always CLOSED */
    InitSnd(conn);                         /* Initialize the sender side of the connection */
    conn->local = (qcc::Rand32() % 65534) + 1;     /* Allocate an "ephemeral" source port */
    conn->foreign = foreign;               /* The ARDP port of the foreign host */
    conn->sock = sock;                     /* The socket to use when talking on this connection */
    conn->ipAddr = ipAddr;                 /* The IP address of the foreign host */
    conn->ipPort = ipPort;                 /* The UDP port of the foreign host */

    conn->lastSeen = TimeNow(handle->tbase);

    SetEmpty(&conn->timers);

    conn->sndHdrLen = ARDP_FIXED_HEADER_LEN;
    conn->rcvHdrLen = ARDP_FIXED_HEADER_LEN;
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
    uint32_t timeout = ARDP_RETRANSMIT_TIMEOUT;
    uint16_t fcnt;
    uint32_t lastLen;
    uint8_t* segData = buf;
    uint32_t som = htonl(conn->SND.NXT);

    /*
     * Note that a ttl of 0 means "forever" and the maximum TTL in the protocol is 65535 ms
     */

    QCC_DbgTrace(("SendData(handle=%p, conn=%p, buf=%p, len=%d., ttl=%d.)", handle, conn, buf, len, ttl));
    QCC_DbgPrintf(("SendData(): Sending %d bytes of data from src=%d to dst=%d", len, conn->local, conn->foreign));
    QCC_DbgPrintf(("SendData(): SND.NXT=%u, SND.UNA=%u, RCV.CUR=%u", conn->SND.NXT, conn->SND.UNA, conn->RCV.CUR));

    if ((conn->SND.NXT - conn->SND.UNA) < conn->SND.MAX) {

        if (len <= conn->SBUF.maxDlen) {
            /* Data fits into one segment */
            fcnt = 1;
            lastLen = len;
        } else {
            /* Need fragmentation */
            fcnt = (len + (conn->SBUF.maxDlen - 1)) / conn->SBUF.maxDlen;
            lastLen = len % conn->SBUF.maxDlen;

            QCC_DbgPrintf(("SendData(): Large buffer %d, partitioning into %d segments", len, fcnt));

            if (fcnt > conn->SND.MAX) {
                QCC_LogError(status, ("SendData(): number of fragments %d exceeds the window size %d", fcnt, conn->window));
                return ER_FAIL;
            }

            /* Check if receiver's window is wide enough to accept FCNT number of segments */
            if (((conn->SND.NXT - conn->SND.UNA) + fcnt) > conn->window) {
                QCC_DbgPrintf(("SendData(): number of fragments %d exceeds the window size %d", fcnt, conn->window));
                return ER_ARDP_BACKPRESSURE;
            }
        }

        for (uint16_t i = 0; i < fcnt; i++) {
            uint16_t index = conn->SND.NXT % conn->SND.MAX;
            ArdpHeader* h = (ArdpHeader*) conn->SBUF.snd[index].hdr;
            uint16_t segLen = (i == (fcnt - 1)) ? lastLen : conn->SBUF.maxDlen;

            QCC_DbgPrintf(("SendData: Segment %d, SND.NXT=%u, SND.UNA=%u, RCV.CUR=%u", i, conn->SND.NXT, conn->SND.UNA, conn->RCV.CUR));
            assert((conn->SND.NXT - conn->SND.UNA) < conn->SND.MAX);

            h->flags = ARDP_FLAG_ACK | ARDP_FLAG_VER;
            h->som = som;
            h->fcnt = htons(fcnt);

            h->hlen = conn->sndHdrLen / 2;
            h->src = htons(conn->local);
            h->dst = htons(conn->foreign);;
            h->dlen = htons(segLen);
            h->seq = htonl(conn->SND.NXT);
            h->ttl = htonl(ttl);
            conn->SBUF.snd[index].ttl = ttl;
            conn->SBUF.snd[index].tStart = TimeNow(handle->tbase);
            conn->SBUF.snd[index].data = segData;
            conn->SBUF.snd[index].datalen = segLen;
            conn->SBUF.snd[index].hdrlen = conn->sndHdrLen;
            if (h->dst == 0) {
                QCC_DbgPrintf(("SendData(): destination = 0"));
            }

            assert(((conn->SBUF.pending) < conn->SND.MAX) && "Number of pending segments in send queue exceeds MAX!");
            QCC_DbgPrintf(("SendData(): updated send queue at index %d", index));

            status = SendMsgData(handle, conn, &conn->SBUF.snd[index]);

            if (status == ER_WOULDBLOCK) {
                timeout = ARDP_URGENT_RETRANSMIT_TIMEOUT;
                status = ER_OK;
            }

            /* We change update our accounting only if the message has been sent successfully. */
            if (status == ER_OK) {
                conn->SBUF.snd[index].timer = AddTimer(handle, conn, RETRANSMIT_TIMER, RetransmitTimerHandler, (void*) &conn->SBUF.snd[index], timeout, ARDP_RETRANSMIT_RETRY + 1);

                conn->SBUF.pending++;
                conn->SND.NXT++;
                conn->SBUF.snd[index].inUse = true;
            } else if (status != ER_ARDP_TTL_EXPIRED) {
                /* Something irrevocably bad happened on the socket. Disconnect. */
                Disconnect(handle, conn, status);
                break;
            }

            DumpSndInfo(conn);
        }
    } else {
        QCC_DbgPrintf(("SendData(): Send window full"));
        status = ER_ARDP_BACKPRESSURE;
    }

    return status;
}

/* This is a special case: ACK with handshake data.
 * Used only for connection establishment in active mode. */
static QStatus DoSendAck(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t seq, uint32_t ack, uint8_t* const buf, uint16_t len)
{
    QCC_DbgTrace(("DoSendAck(handle=%p, conn=%p, seq=%d, ack=%d, buf=%p, len = %d)", handle, conn, seq, ack, buf, len));

    assert(len < conn->RBUF.MAX);

    ArdpHeader h;
    h.flags = ARDP_FLAG_ACK | ARDP_FLAG_VER;
    h.hlen = conn->sndHdrLen / 2;
    h.src = htons(conn->local);
    h.dst = htons(conn->foreign);;
    h.dlen = htons(len);
    h.seq = htonl(seq);
    h.ack = htonl(ack);
    h.window = htons(conn->RCV.MAX);

#if USE_SG_LIST
    qcc::ScatterGatherList msgSG;
    size_t sent;

    msgSG.AddBuffer(&h, ARDP_FIXED_HEADER_LEN);
    msgSG.AddBuffer(conn->rcvMsk.htnMask, conn->rcvMsk.fixedSz * sizeof(uint32_t));
    msgSG.AddBuffer(buf, len);
    return qcc::SendToSG(conn->sock, conn->ipAddr, conn->ipPort, msgSG, sent);
#else
    struct iovec iov[] = { { &h, ARDP_FIXED_HEADER_LEN }, { conn->rcvMsk.htnMask, conn->rcvMsk.fixedSz * sizeof(uint32_t) }, { buf, len }};
    return SendMsg(handle, conn, iov, ArraySize(iov));
#endif /* USE_SG_LIST */

}

static QStatus DoSendSyn(ArdpHandle* handle, ArdpConnRecord* conn, bool synack, uint32_t seq, uint32_t ack, uint16_t segmax, uint16_t segbmax, uint8_t* buf, uint16_t len)
{
    QCC_DbgTrace(("DoSendSyn(handle=%p, conn=%p, synack=%d, seq=%d, ack=%d, segmax=%d, segbmax=%d, buf=%p, len = %d)",
                  handle, conn, synack, seq, ack, segmax, segbmax, buf, len));

    assert(len < segbmax);

    ArdpSynSegment* ss = &conn->synSnd.ss;
    ArdpTimer* timer;

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

    assert(buf != NULL && len != 0);
    conn->synSnd.data = (uint8_t*) malloc(len);
    if (conn->synSnd.data == NULL) {
        return ER_OUT_OF_MEMORY;
    }

    conn->synSnd.dataLen = len;
    memcpy(conn->synSnd.data, buf, len);

    timer = AddTimer(handle, conn, CONNECT_TIMER, ConnectTimerHandler, &conn->synSnd, handle->config.connectTimeout, handle->config.connectRetries + 1);
    timer->context = (void*) timer;
    QCC_DbgPrintf(("DoSendSyn(): timer=%p, retries=%u", timer, timer->retry));
    QCC_DbgPrintf(("DoSendSyn(): ss->seq=%u  data=%p (%s), len=%u", ntohl(ss->seq), conn->synSnd.data, conn->synSnd.data, conn->synSnd.dataLen));

#if USE_SG_LIST
    qcc::ScatterGatherList msgSG;
    size_t sent;

    msgSG.AddBuffer(&conn->synSnd.ss, sizeof(ArdpSynSegment));
    msgSG.AddBuffer(conn->synSnd.data, conn->synSnd.dataLen);
    return qcc::SendToSG(conn->sock, conn->ipAddr, conn->ipPort, msgSG, sent);
#else
    struct iovec iov[] = { { ss, sizeof(ArdpSynSegment) }, { conn->synSnd.data, conn->synSnd.dataLen }};
    return SendMsg(handle, conn, iov, ArraySize(iov));
#endif /* USE_SG_LIST */
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

static void FlushAckedSegments(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t ack) {
    QCC_DbgTrace(("FlushAckedSegments(): handle=%p, conn=%p, ack=%d", handle, conn, ack));
    uint16_t index = conn->SND.UNA % conn->SND.MAX;

    /* Cycle through the buffers in flight */
    for (uint32_t i = 0; i < (conn->SND.NXT - conn->SND.UNA); i++) {
        ArdpHeader* h = (ArdpHeader*) conn->SBUF.snd[index].hdr;
        uint32_t seq = ntohl(h->seq);
        uint16_t fcnt = ntohs(h->fcnt);

        if (SEQ32_LET(seq, ack) && (conn->SBUF.snd[index].inUse)) {

            if (conn->SBUF.snd[index].timer != NULL) {
                conn->SBUF.snd[index].timer->retry = 0;
                conn->SBUF.snd[index].timer = NULL;
            }

            /* If fragmented, wait for the last segment. Issue sendCB on the first fragment in message.*/
            if (fcnt > 1) {
                QCC_DbgPrintf(("FlushAckedSegments(): fragment=%u, som=%u, fcnt=%d",
                               ntohl(h->seq), ntohl(h->som), fcnt));

                /* Check if this is the last one in chain and send cb only for that one */
                if (ntohl(h->seq) != ntohl(h->som) + (fcnt - 1)) {
                    index = (index + 1) % conn->SND.MAX;
                    continue;
                } else {
                    QCC_DbgPrintf(("FlushAckedSegments(): last fragment=%u, som=%u, fcnt=%d",
                                   ntohl(h->seq), ntohl(h->som), fcnt));

                    /* First segment in message, keeps original pointer to message buffer */
                    uint16_t fragIndex = ntohl(h->som) % conn->SND.MAX;
                    /* Original message length */
                    uint32_t len =  conn->SBUF.maxDlen * (fcnt - 1) + ntohs(h->dlen);
                    QCC_DbgPrintf(("FlushAckedSegments(): First Fragment SendCb(handle=%p, conn=%p, buf=%p, len=%d, status=%d",
                                   handle, conn, conn->SBUF.snd[fragIndex].data, len, ER_OK));

                    /* Mark all fragment SND buffers as available */
                    do {
                        conn->SBUF.snd[fragIndex].inUse = false;
                        fragIndex = (fragIndex + 1) % conn->SND.MAX;
                        conn->SBUF.pending--;
                        QCC_DbgPrintf(("FlushAckedSegments(fcnt = %d): pending = %d", fcnt, conn->SBUF.pending));
                        assert((conn->SBUF.pending < conn->SND.MAX) && "Invalid number of pending segments in send queue!");
                        fcnt--;
                    } while (fcnt > 0);

                    handle->cb.SendCb(handle, conn, conn->SBUF.snd[fragIndex].data, len, ER_OK);
                }
            } else {
                QCC_DbgPrintf(("FlushAckedSegments(): SendCb(handle=%p, conn=%p, buf=%p, len=%d, status=%d",
                               handle, conn, conn->SBUF.snd[index].data, conn->SBUF.snd[index].datalen, ER_OK));
                conn->SBUF.snd[index].inUse = false;
                conn->SBUF.pending--;
                QCC_DbgPrintf(("FlushAckedSegments(unfragmented): pending = %d", conn->SBUF.pending));
                assert((conn->SBUF.pending < conn->SND.MAX) && "Invalid number of pending segments in send queue!");

                handle->cb.SendCb(handle, conn, conn->SBUF.snd[index].data, conn->SBUF.snd[index].datalen, ER_OK);
            }

        }

        index = (index + 1) % conn->SND.MAX;
    }

    DumpSndInfo(conn);
}

static void CancelEackedSegments(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t* bitMask) {
    QCC_DbgTrace(("CancelEackedSegments(): handle=%p, conn=%p, bitMask=%p", handle, conn, bitMask));
    uint32_t start = conn->SND.UNA;
    uint16_t index =  start % conn->SND.MAX;
    ArdpTimer* timer = conn->SBUF.snd[index].timer;

    DumpBitMask(conn, bitMask, conn->remoteMskSz, true);

    /* Schedule fast retransmit to fill the gap */
    if (timer != NULL) {
        QCC_DbgPrintf(("CancelEackedSegments(): prioritize timer %p for %u", conn->SBUF.snd[index].timer, conn->SND.UNA));
        timer->when = timer->when - timer->delta;
    }

    /* Bitmask starts at SND.UNA + 2. Cycle through the mask and cancel retransmit timers
     * on EACKed segments. */
    start = start + 1;
    for (uint16_t i = 0; i < conn->remoteMskSz; i++) {
        uint32_t mask32 = ntohl(bitMask[i]);
        uint32_t bitCheck = 1 << 31;

        index = (start + (i * 32)) % conn->SND.MAX;
        while (mask32 != 0) {
            timer = conn->SBUF.snd[index].timer;
            if (mask32 & bitCheck) {
                if (timer != NULL) {
                    QCC_DbgPrintf(("CancelEackedSegments(): set retries to zero for timer %p for index %u", timer, index));
                    conn->SBUF.snd[index].timer->retry = 0;
                    conn->SBUF.snd[index].timer = NULL;
                }
            }
            mask32 = mask32 << 1;
            index = (index + 1) % conn->SND.MAX;
        }
    }
}

static void UpdateRcvMsk(ArdpConnRecord* conn, uint32_t delta)
{
    QCC_DbgPrintf(("UpdateRcvMsk: delta = %d", delta));
    /* First bit represents RCV.CUR + 2 */
    uint32_t skip = delta / 32;
    uint32_t lshift = 32 - (delta % 32);
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
    conn->rcvMsk.sz = newSz;
}

static void AddRcvMsk(ArdpConnRecord* conn, uint32_t delta)
{
    QCC_DbgPrintf(("AddRcvMsk: delta = %d", delta));
    /* First bit represents RCV.CUR + 2 */
    uint32_t bin32 = (delta - 1) / 32;
    uint32_t offset = 32 - (delta - (bin32 << 5));

    assert(bin32 < conn->rcvMsk.fixedSz);
    conn->rcvMsk.mask[bin32] = conn->rcvMsk.mask[bin32] | (1 << offset);
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
    QCC_DbgTrace(("UpdateRcvBuffers: first=%u, this seq=%u", conn->RBUF.first, consumed->seq));

    /* Important! The contract with the upper layer is that the buffers are ALWAYS released in the same order
     * they have been delivered. */

    if (conn->RBUF.first != consumed->seq) {
        QCC_LogError(ER_FAIL, ("UpdateRcvBuffers: released buffer %p (seq=%u) is not first in line to be released: rcv %p (seq %u)",
                               consumed, consumed->seq, &rcv[conn->RBUF.first % conn->RCV.MAX], conn->RBUF.first));
    }
    assert(conn->RBUF.first == consumed->seq);

    index = consumed->seq % conn->RCV.MAX;
    if (&rcv[index] != consumed) {
        QCC_LogError(ER_FAIL, ("UpdateRcvBuffers: released buffer %p (seq=%u) does not match rcv %p @ %u", consumed, consumed->seq, &rcv[index], index));
        assert(0 && "UpdateRcvBuffers: Buffer sequence validation failed");
        return ER_FAIL;
    }

    if (consumed->fcnt < 1) {
        QCC_LogError(ER_FAIL, ("Invalid fragment count %d", consumed->fcnt));
    }

    assert((consumed->fcnt >= 1) && "fcnt cannot be less than one!");

    /* Release the current buffers associated with the consumed message.
     * At the same time check whether the next in queue message expired,
     * and if so, release the corresponding buffers as well. */
    do {
        for (uint32_t i = 0; i < count; i++) {
            assert((consumed->inUse) && "UpdateRcvBuffers: Attempt to release a buffer that is not in use");
            assert((consumed->isDelivered) && "UpdateRcvBuffers: Attempt to release a buffer that has not been delivered");

            consumed->inUse = false;
            consumed->isDelivered = false;
            QCC_DbgPrintf(("UpdateRcvBuffers: released buffer %p (seq=%u)", consumed, consumed->seq));

            assert(consumed->data != NULL);
            if (consumed->data != NULL) {
                free(consumed->data);
                consumed->data = NULL;
            }
            conn->RBUF.first++;
            consumed = consumed->next;
        }
        count = consumed->fcnt;
    } while ((consumed->isDelivered) && (consumed->ttl == ARDP_TTL_EXPIRED));

    /* Update receive window size. This will be communicated to the remote side. */
    if (SEQ32_LT(conn->RBUF.last, conn->RBUF.first)) {
        /* Receive window is completely empty */
        QCC_DbgPrintf(("UpdateRcvBuffers: window empty last %u first %u", conn->RBUF.last, conn->RBUF.first));
        conn->RBUF.window = conn->RCV.MAX;
        conn->RBUF.last = conn->RBUF.first;
    } else {
        conn->RBUF.window = conn->RCV.MAX - ((conn->RBUF.last - conn->RBUF.first) + 1);
        QCC_DbgPrintf(("UpdateRcvBuffers: window %d last %u first %u", conn->RBUF.window, conn->RBUF.last, conn->RBUF.first));
    }

    QCC_DbgPrintf(("UpdateRcvBuffers: window %d", conn->RBUF.window));
    return ER_OK;
}

static QStatus AddRcvBuffer(ArdpHandle* handle, ArdpConnRecord* conn, ArdpSeg* seg, uint8_t* buf, uint16_t len, bool ordered)
{
    uint16_t index = seg->SEQ % conn->RCV.MAX;
    ArdpRcvBuf* current = &conn->RBUF.rcv[index];
    uint16_t hdrlen = conn->rcvHdrLen;

    QCC_DbgTrace(("AddRcvBuffer(handle=%p, conn=%p, seg=%p, buf=%p, len=%d, ordered=%s", handle, conn, seg, buf, len, (ordered ? "true" : "false")));

    QCC_DbgPrintf(("AddRcvBuffer: seg->SEQ = %u, first=%u, last= %u", seg->SEQ, conn->RBUF.first, conn->RBUF.last));
    /* Sanity check */
    if (hdrlen != (len - seg->DLEN)) {
        QCC_DbgPrintf(("AddRcvBuffer: hdrlen=%d does not match (len-DLEN)=%d", hdrlen, len - seg->DLEN));
        assert(false);
    }

    /* Allow the segments that fall within fisrst in last, filling the gaps */
    if (conn->RBUF.window == 0 && !(SEQ32_LT(seg->SEQ, conn->RBUF.last))) {
        QCC_DbgPrintf(("AddRcvBuffer: Receive Window full for conn %p", conn));
        assert(false && "AddRcvBuffer: Attempt to add to a full window");
        return ER_FAIL;
    }

    if (seg->DLEN > conn->RBUF.MAX) {
        QCC_DbgPrintf(("AddRcvBuffer: data len %d exceeds SEGBMAX %d", seg->DLEN, conn->RBUF.MAX));
        return ER_FAIL;
    }
    assert(!(current->inUse) && "AddRcvBuffer: attempt to overwrite buffer that has not been released");


    current->data = (uint8_t*) malloc(seg->DLEN * sizeof(uint8_t));
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
     *
     * TODO: This does not account for time on the network -- e.g. SRTT/2
     */
    current->ttl = seg->TTL;
    current->tRecv = TimeNow(handle->tbase);

    /* Deliver this segment (and following out-of-order segments) to the upper layer. */
    if (ordered) {
        uint32_t delta = 0;

        /* Same execution flow for both fragmented and whole messages */
        do {
            conn->RCV.CUR = current->seq;

            /*
             * Check if last fragment. If this is the case, re-assemble the message:
             * - Find the rcv, corresponding to SOM
             * - Make sure there are no gaps (shouldn't be the case we are in "erdered delivery" case here,
             *   assert on that), remove for release.
             * - RecvCb(SOM's rcvBuf, fcnt)
             */
            if (current->seq == current->som + (current->fcnt - 1)) {
                index = seg->SOM % conn->RCV.MAX;
                ArdpRcvBuf* startFrag = &conn->RBUF.rcv[index];
                ArdpRcvBuf* fragment = startFrag;
                uint32_t tNow = TimeNow(handle->tbase);
                bool expired = false;

                /* Remove this check before release */
                for (uint32_t i = 0; i < current->fcnt; i++) {
                    if (!fragment->inUse || fragment->isDelivered || fragment->som != seg->SOM || fragment->fcnt != seg->FCNT) {
                        QCC_LogError(ER_FAIL, ("Gap in fragmented (%i) message: start %u, this(%d) %u",
                                               seg->FCNT, seg->SOM, i, fragment->seq));
                    }
                    assert(fragment->inUse && "Gap in fragmented message");
                    assert((fragment->som == seg->SOM && fragment->fcnt == seg->FCNT) && "Lost track of received fragment");
                    fragment = fragment->next;
                }

                /*
                 * Mark all the fragments as delivered, and while we're at it note
                 * whether or not the message has expired.
                 */
                fragment = startFrag;
                for (uint32_t i = 0; i < current->fcnt; i++) {
                    /*
                     * If any one of the fragments is marked as expired, the entire message has expired.
                     */
                    if ((fragment->ttl == ARDP_TTL_EXPIRED)  || (fragment->ttl != ARDP_TTL_INFINITE && (tNow - fragment->tRecv >= fragment->ttl))) {
                        QCC_DbgPrintf(("ArdpRcvBuffer(): Detected expired message (conn=0x%p, seq=%u)", conn, fragment->seq));
                        expired = true;
                    }


                    fragment->isDelivered = true;
                    fragment = fragment->next;
                }

                /*
                 * If the message has expired, don't send it up to the upper layer,
                 * just recycle it, otherwise pass it off to the transport.
                 */
                if (expired) {
                    /* This is not really an error, remove before the release */
                    QCC_LogError(ER_TIMEOUT, ("ArdpRcvBuffer(): Ignoring expired message (conn=0x%p, start seq =%u)", conn, startFrag->seq));
                    startFrag->ttl = ARDP_TTL_EXPIRED;

                    /* If this message is first in line to be released, flush it*/
                    if (conn->RBUF.first == startFrag->seq) {
                        UpdateRcvBuffers(handle, conn, startFrag);
                    }
                } else {
                    handle->cb.RecvCb(handle, conn, startFrag, ER_OK);
                }
            }

            current = current->next;
            delta++;
            QCC_DbgPrintf(("ArdpRcvBuffer(): current->seq = %u, (seg->SEQ + delta) = %u", current->seq, (seg->SEQ + delta)));
        } while (current->seq == (seg->SEQ + delta));

        if (delta > 1) {
            UpdateRcvMsk(conn, delta + 1);
        }
    } else {
        AddRcvMsk(conn, (seg->SEQ - (conn->RCV.CUR + 1)));
    }

    conn->RBUF.window = conn->RCV.MAX - ((conn->RBUF.last - conn->RBUF.first) + 1);
    QCC_DbgPrintf(("ArdpRcvBuffer(): window = %d", conn->RBUF.window));

    DumpBitMask(conn, conn->rcvMsk.mask, conn->rcvMsk.fixedSz, false);

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
            conn->window = conn->SND.MAX;

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
                break;
            }

            if (seg->FLG & ARDP_FLAG_SYN) {
                QCC_DbgPrintf(("ArdpMachine(): LISTEN: SYN received.  Accepting"));
                conn->RCV.CUR = seg->SEQ;
                conn->RCV.IRS = seg->SEQ;

                /* Fixed size of EACK bitmask */
                conn->remoteMskSz = ((conn->SND.MAX + 31) >> 5);
                conn->rcvHdrLen = ARDP_FIXED_HEADER_LEN + conn->remoteMskSz * sizeof(uint32_t);
                QCC_DbgPrintf(("ArdpMachine(): LISTEN: SYN received: rcvHdrLen=%d", conn->rcvHdrLen));

                QCC_DbgPrintf(("ArdpMachine(): LISTEN: SYN received: the other side can receive max %d bytes", conn->SBUF.MAX));
                if (handle->cb.AcceptCb != NULL) {
                    uint8_t* data = buf + sizeof(ArdpSynSegment);
                    if (handle->cb.AcceptCb(handle, conn->ipAddr, conn->ipPort, conn, data, seg->DLEN, ER_OK) == false) {
                        QCC_DbgPrintf(("ArdpMachine(): LISTEN: SYN received. AcceptCb() returned \"false\""));
                        DelConnRecord(handle, conn);
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
                SetState(conn, CLOSED);
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
                conn->RCV.CUR = seg->SEQ;
                conn->RCV.IRS = seg->SEQ;
                conn->SBUF.MAX = ntohs(ss->segbmax);
                QCC_DbgPrintf(("ArdpMachine(): SYN_SENT: the other side can receive max %d bytes", conn->SBUF.MAX));
                status = InitSBUF(conn);

                assert(status == ER_OK && "ArdpMachine():SYN_SENT: Failed to initialize Send queue");

                if (seg->FLG & ARDP_FLAG_ACK) {
                    ArdpTimer* timer;
                    QCC_DbgPrintf(("ArdpMachine(): SYN_SENT: SYN | ACK received. state -> OPEN"));
                    conn->SND.UNA = seg->ACK + 1;
                    PostInitRcv(conn);
                    SetState(conn, OPEN);
                    CancelTimer(conn, CONNECT_TIMER, NULL);
                    conn->lastSeen = TimeNow(handle->tbase);

                    /* Add link timeout timer */
                    timer = AddTimer(handle, conn, PROBE_TIMER, ProbeTimerHandler, NULL, handle->config.probeTimeout, handle->config.probeRetries);
                    timer->context = (void*) timer;

                    /* Add dead window timer */
                    timer = AddTimer(handle, conn, WINDOW_CHECK_TIMER, WindowCheckTimerHandler, NULL, handle->config.persistTimeout, handle->config.persistRetries);
                    timer->context = (void*) timer;

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

                    /*
                     * Do not send the ACK immediately.  Wait for the ARDP_Acknowledge to give the final
                     * bits of connection data.
                     */
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
                if ((seg->FLG & ARDP_FLAG_RST) == 0 && seg->ACK != conn->SND.ISS) {
                    QCC_DbgPrintf(("ArdpMachine(): SYN_SENT: ACK does not ASK ISS"));
                    SetState(conn, CLOSED);
                    /*
                     * <SEQ=SEG.ACK + 1><RST>
                     */
                    Send(handle, conn, ARDP_FLAG_RST | ARDP_FLAG_VER, seg->ACK + 1, 0, conn->RCV.MAX);
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
                    SetState(conn, CLOSED);
                }
                break;
            }

            if (seg->FLG & ARDP_FLAG_SYN) {
                QCC_DbgPrintf(("ArdpMachine(): SYN_RCVD: Got SYN, state -> CLOSED"));
                SetState(conn, CLOSED);

                /* <SEQ=SEG.ACK + 1><RST> */
                Send(handle, conn, ARDP_FLAG_RST | ARDP_FLAG_VER, seg->ACK + 1, 0, conn->RCV.MAX);
                break;
            }

            if (seg->FLG & ARDP_FLAG_EACK) {
                QCC_DbgPrintf(("ArdpMachine(): SYN_RCVD: Got EACK. Send RST"));
                /* <SEQ=SEG.ACK + 1><RST> */
                Send(handle, conn, ARDP_FLAG_RST | ARDP_FLAG_VER, seg->ACK + 1, 0, conn->RCV.MAX);
                break;
            }

            if (seg->FLG & ARDP_FLAG_ACK) {
                if (seg->ACK == conn->SND.ISS) {
                    ArdpTimer* timer;

                    QCC_DbgPrintf(("ArdpMachine(): SYN_RCVD: Got ACK with correct acknowledge.  state -> OPEN"));
                    PostInitRcv(conn);
                    SetState(conn, OPEN);
                    CancelTimer(conn, CONNECT_TIMER, NULL);
                    conn->lastSeen = TimeNow(handle->tbase);

                    /* Add dead Link timeout timer */
                    timer = AddTimer(handle, conn, PROBE_TIMER, ProbeTimerHandler, NULL, handle->config.probeTimeout, handle->config.probeRetries);
                    timer->context = (void*) timer;

                    /* Add dead window timer */
                    timer = AddTimer(handle, conn, WINDOW_CHECK_TIMER, WindowCheckTimerHandler, NULL, handle->config.persistTimeout, handle->config.persistRetries);
                    timer->context = (void*) timer;

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

                    if (seg->FLG & ARDP_FLAG_NUL) {
                        Send(handle, conn, ARDP_FLAG_ACK | ARDP_FLAG_VER, conn->SND.NXT, conn->RCV.CUR, conn->RBUF.window);
                    }
                    break;

                } else {
                    /* <SEQ=SEG.ACK + 1><RST> */
                    Send(handle, conn, ARDP_FLAG_RST | ARDP_FLAG_VER, seg->ACK + 1, 0, conn->RCV.MAX);
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

            if (IN_RANGE(uint32_t, conn->RCV.CUR + 1, conn->RCV.MAX, seg->SEQ) == false) {
                QCC_DbgPrintf(("ArdpMachine(): OPEN: unacceptable sequence %u, conn->RCV.CUR + 1 = %u, MAX = %d", seg->SEQ, conn->RCV.CUR + 1, conn->RCV.MAX));
                /* This check sjhould be removed in release version */
                if (SEQ32_LT(seg->SEQ, conn->RCV.CUR + 1)) {
                    QCC_DbgPrintf(("ArdpMachine(): OPEN: duplicate %u", seg->SEQ));
                } else {
                    DumpBuffer(buf, len);
                    assert(false);
                }
                /*
                 * <SEQ=SND.NXT><ACK=RCV.CUR><ACK>
                 */
                Send(handle, conn, ARDP_FLAG_ACK | ARDP_FLAG_VER, conn->SND.NXT, conn->RCV.CUR, conn->RBUF.window);
                break;
            }

            if (seg->FLG & ARDP_FLAG_RST) {
                QCC_DbgPrintf(("ArdpMachine(): OPEN: got RST.  state -> CLOSE_WAIT"));
                Disconnect(handle, conn, ER_ARDP_REMOTE_CONNECTION_RESET);
                break;
            }

            if (seg->FLG & ARDP_FLAG_SYN) {
                if (conn->passive) {
                    QCC_DbgPrintf(("ArdpMachine(): OPEN: Got SYN while passive open.  state -> LISTEN"));
                    SetState(conn, LISTEN);
                } else {
                    QCC_DbgPrintf(("ArdpMachine(): OPEN: Got SYN while active open.  state -> CLOSED"));
                    SetState(conn, CLOSED);
                }
                /*
                 * <SEQ=SEG.ACK + 1><RST>
                 */
                Send(handle, conn, ARDP_FLAG_RST | ARDP_FLAG_VER, seg->ACK + 1, 0, conn->RBUF.window);
                break;
            }

            if (seg->FLG & ARDP_FLAG_NUL) {
                QCC_DbgPrintf(("ArdpMachine(): OPEN: got NUL, send window %d", conn->RBUF.window));
                Send(handle, conn, ARDP_FLAG_ACK | ARDP_FLAG_VER, conn->SND.NXT, conn->RCV.CUR, conn->RBUF.window);
                break;
            }

            if (seg->FLG & ARDP_FLAG_ACK) {
                QCC_DbgPrintf(("ArdpMachine(): OPEN: Got ACK %u", seg->ACK));
                if (IN_RANGE(uint32_t, conn->SND.UNA, ((conn->SND.NXT - conn->SND.UNA) + 1), seg->ACK) == true) {
                    //if (conn->SND.UNA <= seg->ACK && seg->ACK < conn->SND.NXT) {
                    FlushAckedSegments(handle, conn, seg->ACK);
                    conn->SND.UNA = seg->ACK + 1;
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
                if (SEQ32_LT(conn->RCV.CUR, seg->SEQ)) {
                    status = AddRcvBuffer(handle, conn, seg, buf, len, seg->SEQ == (conn->RCV.CUR + 1));
                }

                if (status == ER_OK) {
                    Send(handle, conn, ARDP_FLAG_ACK | ARDP_FLAG_VER, conn->SND.NXT, conn->RCV.CUR, conn->RBUF.window);
                }
            }

            if ((conn->window != seg->WINDOW) && (handle->cb.SendWindowCb != NULL)) {
                conn->window = seg->WINDOW;
                handle->cb.SendWindowCb(handle, conn, conn->window, (conn->window != 0) ? ER_OK : ER_ARDP_BACKPRESSURE);
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

QStatus ARDP_Connect(ArdpHandle* handle, qcc::SocketFd sock, qcc::IPAddress ipAddr, uint16_t ipPort, uint16_t segmax, uint16_t segbmax, ArdpConnRecord** pConn, uint8_t* buf, uint16_t len, void* context)
{
    QCC_DbgTrace(("ARDP_Connect(handle=%p, sock=%d, ipAddr=\"%s\", ipPort=%d, segmax=%d, segbmax=%d, pConn=%p, buf=%p, len=%d, context=%p)",
                  handle, sock, ipAddr.ToString().c_str(), ipPort, segmax, segbmax, pConn, buf, len, context));

    ArdpConnRecord* conn = NewConnRecord();
    QStatus status;

    InitConnRecord(handle, conn, sock, ipAddr, ipPort, 0);
    status = InitRcv(conn, segmax, segbmax);     /* Initialize the receiver side of the connection */
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
        return ER_ARDP_INVALID_STATE;
    }

    status = InitRcv(conn, segmax, segbmax);     /* Initialize the receiver side of the connection */
    if (status != ER_OK) {
        DelConnRecord(handle, conn);
        return status;
    }

    status = InitSBUF(conn);
    if (status != ER_OK) {
        DelConnRecord(handle, conn);
        return status;
    }

    SetState(conn, SYN_RCVD);
    /* <SEQ=SND.ISS><ACK=RCV.CUR><MAX=RCV.MAX><BUFMAX=RBUF.MAX><ACK><SYN> */
    SendSynAck(handle, conn, conn->SND.ISS, conn->RCV.CUR, conn->RCV.MAX, conn->RBUF.MAX, buf, len);
    return ER_OK;
}

QStatus ARDP_Acknowledge(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t* buf, uint16_t len)
{
    QCC_DbgTrace(("ARDP_Acnowledge(handle=%p, conn=%p, buf=%p (%s), len=%d)", handle, conn, buf, buf, len));

    if (!IsConnValid(handle, conn)) {
        return ER_ARDP_INVALID_STATE;
    }

    /*
     * This is the function that an Active connector uses to drive the sending of the
     * final ACK in the SYN, SYN + ACK, ACK three-way handshake.
     *
     * <SEQ=SND.NXT><ACK=RCV.CUR><ACK>
     */
    DoSendAck(handle, conn, conn->SND.NXT, conn->RCV.CUR, buf, len);
    return ER_OK;
}

QStatus ARDP_Disconnect(ArdpHandle* handle, ArdpConnRecord* conn)
{
    QCC_DbgTrace(("Disconnect(handle=%p, conn=%p)", handle, conn));
    return Disconnect(handle, conn, ER_OK);
}

QStatus ARDP_RecvReady(ArdpHandle* handle, ArdpConnRecord* conn, ArdpRcvBuf* rcv)
{
    QCC_DbgTrace(("ARDP_RecvReady(handle=%p, conn=%p, rcv=%p, cnt=%d)", handle, conn, rcv));
    if (!IsConnValid(handle, conn)) {
        return ER_ARDP_INVALID_STATE;
    }
    return UpdateRcvBuffers(handle, conn, rcv);
}

QStatus ARDP_Send(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t* buf, uint32_t len, uint32_t ttl)
{
    QCC_DbgTrace(("ARDP_Send(handle=%p, conn=%p, buf=%p, len=%d., ttl=%d.)", handle, conn, buf, len, ttl));
    if (!IsConnValid(handle, conn) || conn->STATE != OPEN) {
        return ER_ARDP_INVALID_STATE;
    }

    if (buf == NULL || len == 0) {
        return ER_INVALID_DATA;
    }

    QCC_DbgPrintf(("NXT=%u, UNA=%u, window=%d", conn->SND.NXT, conn->SND.UNA, conn->window));
    if ((conn->window == 0)  || ((conn->SND.NXT - conn->SND.UNA) >= conn->window)) {
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
    SEG.HLEN = header->hlen;         /* The header len */
    if (!(SEG.FLG & ARDP_FLAG_SYN) && ((SEG.HLEN * 2) != conn->rcvHdrLen)) {
        QCC_DbgPrintf(("Receive: seg.len = %d, expected = %d", (SEG.HLEN * 2), conn->rcvHdrLen));
        assert(false);
    }
    SEG.SRC = ntohs(header->src);       /* The source ARDP port */
    SEG.DST = ntohs(header->dst);       /* The destination ARDP port */
    SEG.SEQ = ntohl(header->seq);       /* The send sequence of the current segment */
    SEG.ACK = ntohl(header->ack);       /* The cumulative acknowledgement number to our sends */
    SEG.MAX = conn->RCV.MAX;            /* The maximum number of outstanding segments the receiver (we) are willing to hold */
    SEG.BMAX = conn->SBUF.MAX;          /* The maximmum number of bytes the other side (the sender) can receive */
    SEG.DLEN = ntohs(header->dlen);     /* The amount of data in this segment */
    SEG.WINDOW = ntohs(header->window);     /* The receivers window */
    QCC_DbgPrintf(("Receive() window=%d", SEG.WINDOW));
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

    *ms = CheckTimers(handle);            /* When to call back (timer expiration) */
    QCC_DbgPrintf(("ARDP_Run %u", *ms));

    while (socketReady) {
        QStatus status = qcc::RecvFrom(sock, address, port, buf, sizeof(buf), nbytes);
        if (status == ER_WOULDBLOCK) {
            QCC_DbgPrintf(("ARDP_Run(): qcc::RecvFrom() ER_WOULDBLOCK"));
            return ER_OK;
        } else if (status != ER_OK) {
            QCC_LogError(status, ("ARDP_Run(): qcc::RecvFrom() failed"));
            return status;
        }

        if (nbytes > 0 && nbytes < 65536) {
            uint16_t local, foreign;
            ProtocolDemux(buf, nbytes, &local, &foreign);
            if (local == 0) {
                if (handle->accepting && handle->cb.AcceptCb) {
                    ArdpConnRecord* conn = NewConnRecord();
                    InitConnRecord(handle, conn, sock, address, port, foreign);
                    EnList(handle->conns.bwd, (ListNode*)conn);
                    return Accept(handle, conn, buf, nbytes);
                }
                return SendRst(handle, sock, address, port, local, foreign);
            } else {
                /* Is there an open connection? */
                ArdpConnRecord* conn = FindConn(handle, local, foreign);
                if (conn) {
                    conn->lastSeen = TimeNow(handle->tbase);
                    assert(conn->lastSeen != 0);
                    return Receive(handle, conn, buf, nbytes);
                }

                /* Is there a half open connection? */
                conn = FindConn(handle, local, 0);
                if (conn) {
                    conn->lastSeen = TimeNow(handle->tbase);
                    return Receive(handle, conn, buf, nbytes);
                }

                /* Ignore anything else */
            }
        }
    }
    return ER_FAIL;
}

} // namespace ajn
