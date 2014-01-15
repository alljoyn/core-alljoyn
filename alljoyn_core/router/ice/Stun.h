#ifndef _STUN_H
#define _STUN_H
/**
 * @file
 *
 * This file defines the STUN layer API.
 *
 * The STUN layer API is a C++ class definition intended for use by the ICE
 * layer.
 */

/******************************************************************************
 * Copyright (c) 2009,2012 AllSeen Alliance. All rights reserved.
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

#ifndef __cplusplus
#error Only include Stun.h in C++ code.
#endif

#include <queue>
#include <qcc/Mutex.h>
#include <qcc/Socket.h>
#include <qcc/SocketTypes.h>
#include <alljoyn/Status.h>
#include <qcc/IPAddress.h>
#include <qcc/Thread.h>
#include <StunMessage.h>
#include "RendezvousServerInterface.h"

using namespace qcc;

/** @internal */
#define QCC_MODULE "STUN"

namespace ajn {

// forward declaration
class Component;  // FIXME: We need to remove cross linkage to the ICE layer.

/**
 * @class Stun
 *
 * This class defines the API used by the ICE library.  Each instance of this
 * class corresponds to one Component of a Stream as defined in ICE draft IETF
 * standard.
 */
class Stun {

  public:

    /**
     * This is the constructor for the Stun class.  Because each instance
     * corresponds to one Stream Component and a Component is essentially a
     * socket connection, one Stun class must be instantiated per Stream
     * Component.
     *
     * @param type          Socket type.
     * @param component     ???
     * @param autoFraming   Handle automatic data/STUN framing for direct TCP connections.
     */
    Stun(SocketType type,
         Component* component,
         STUNServerInfo stunInfo,
         const uint8_t* key,
         size_t keyLen,
         size_t mtu,
         bool autoFraming = true
         );

    /**
     * Destructor for the Stun class.  This will close the connection if the
     * connetion is currently opened.
     *
     * @sa Stun:CloseConnection
     */
    ~Stun(void);


    /**
     * Open up a socket descriptor.
     *
     * @param af            Address family of the IP Address that will be connected to.
     *
     * @return  Indication of success or failure.
     */
    QStatus OpenSocket(AddressFamily af);


    /**
     * Establish a connection with the specified remote host.
     *
     * @param remoteAddr    Peer's IP address
     * @param remotePort    Peer's IP port number
     * @param relayData     Set to 'true' if application data should be relayed
     *                      via the TURN server specified in the constructor.
     *
     * @return Indication of success or failure.
     */
    QStatus Connect(const IPAddress& remoteAddr, uint16_t remotePort, bool relayData);


    /**
     * Bind the socket to a specific IP address and port.
     *
     * @param localAddr Local IP address to be bound.
     * @param localPort Port on local address to be bound. If zero,
     *                  a port is assigned by the OS.
     *
     * @return Indication of success or failure.
     */
    QStatus Bind(const IPAddress& localAddr, uint16_t localPort);


    /**
     * Listen for incoming conenction on a bound address/port.
     *
     * @param backlog   Number of pending incoming client connections to queue up.
     *
     * @return Indication of success or failure.
     */
    QStatus Listen(int backlog);


    /**
     * Accept and incoming connection on a bound address/port.
     *
     * @param newStun       [OUT] Pointer to a new Stun object encapsulation
     *                            the new socket descriptor.
     *
     * @return Indication of success or failure.
     */
    QStatus Accept(Stun** newStun);


    /**
     * This sets an internal flag that indicates if the connection is using a
     * TURN server or not.  It _MUST_ be called after Stun::Accept() returns
     * with ER_OK if the connection is using a TURN server before any call to
     * send or receive data.  The internal internal flag defaults to
     * indicating that a TURN server is not used.
     *
     * @param usingTurn [Optional] Indicates whether the TURN server is used
     *                  for this connection or not.  (Defaults to 'true'.)
     */
    void SetUsingTurn(bool usingTurn = true) { this->usingTurn = usingTurn; }


    /**
     * Shutdown the connection with the remote host specified in
     * Stun::Connect.
     */
    QStatus Shutdown(void);

    /**
     * Close the connection with the remote host specified in
     * Stun::Connect.
     */
    QStatus Close(void);

    /**
     * Disable STUN message processing.
     * This call disables all Stun message recv processing but leaves any
     * connected file descriptor alone.
     */
    void DisableStunProcessing();

    /**
     * Provide the caller with the local address and port number of the opened
     * socket.  This can only be called after the socket has been opened.
     *
     * @param addr  Reference to where the IP Address should be stored.
     * @param port  Reference to where the port number should be stored.
     *
     * @return  Indication of success or failure.
     */
    QStatus GetLocalAddress(IPAddress& addr, uint16_t& port);

    /**
     * Provide the caller with the STUN server specific information.
     *
     * @return STUN server information.
     *
     */
    STUNServerInfo GetSTUNServerInfo(void);


    /**
     * Send a STUN message to the specified address with the specified
     * list of message attributes.
     *
     * @param msg       STUN Message to be sent.
     * @param destAddr  Destination IP address
     * @param destPort  Destination IP port number
     * @param RelayMsg  Set to "true" if message must be relayed via the TURN server.
     * @return Indication of success or failure.
     */
    QStatus SendStunMessage(const StunMessage& msg,
                            IPAddress destAddr,
                            uint16_t destPort,
                            bool relayMsg);

    /**
     * Receive a STUN message from any sender, and return the address of the sender.
     *
     * @param msg           [OUT] Where the parsed STUN message is stored.
     * @param sourceAddr    [OUT] IP address of the sender
     * @param sourcePort    [OUT] IP port number of the sender
     * @param relayed       [OUT] Indicates if message was relay via the TURN server.
     * @param maxMs         Timeout for maximum number of milliseconds to wait
     *                      for a message.
     *
     * @return  ER_OK for success, ER_TIMEOUT if timeout expired (msg,
     *          sourceAddr, and sourcePort unchanged), ER_STOPPING_THREAD if
     *          interrupted by a stop signal while blocking an ERThread (msg,
     *          sourceAddr, and sourcePort unchanged), other statuses indicate
     *          an message error.
     */
    QStatus RecvStunMessage(StunMessage& msg,
                            IPAddress& sourceAddr,
                            uint16_t& sourcePort,
                            bool& relayed,
                            uint32_t maxMs);


    /**
     * Send application data over the STUN-socket.
     *
     * @param buf   Data to be sent.
     * @param len   Amount of data to send.
     * @param sent  [OUT] Number of bytes sent.
     *
     * @return Indication of success or failure.
     */
    QStatus AppSend(const void* buf, size_t len, size_t& sent);

    /**
     * Send application data over the STUN-socket.
     *
     * @param sg    Scatter-gather list of buffers.
     * @param sent  [OUT] Number of bytes sent.
     *
     * @return Indication of success or failure.
     */
    QStatus AppSendSG(const ScatterGatherList& sg, size_t& sent);


    /**
     * Receive application data from the STUN-socket.  NOTE: There is a
     * significant performance penalty for buffers less than 22 bytes in size.
     *
     * @param buf       Location for receive data.
     * @param len       Size of receive buffer.
     * @param received  [OUT] Number of bytes received.
     *
     * @return Indication of success or failure.
     */
    QStatus AppRecv(void*buf, size_t len, size_t& received);

    /**
     * Receive application data from the STUN-socket.  NOTE: There is a
     * significant performance penalty for buffers less than 22 bytes in size.
     *
     * @param sg        Scatter-gather list of buffers.
     * @param received  [OUT] Number of bytes received.
     *
     * @return Indication of success or failure.
     */
    QStatus AppRecvSG(ScatterGatherList& sg, size_t& received);

    /**
     * Get the underlying socket file descriptor for use in poll() or
     * select().  Use of this file descriptor for any other purpose will cause
     * strange failures.
     *
     * @return  The socket file descriptor.
     */
    SocketFd GetSocketFD(void) { return sockfd; }

    /// FIXME: This should be made more generic and return a void * to user data.
    Component* GetComponent(void) const { return component; }

    SocketType GetSocketType(void) const { return type; }

    /**
     * This releases the Stun objects control of the underlying file
     * descriptor.  Callers are expected to have called GetSocketFD() before
     * hand and are responsible for closing the socket except if close is set
     * to true, in which case the socke will be closed as well.  Once this is
     * called, this object is for all intents and purposes dead.
     */
    void ReleaseFD(bool close = false);

    /**
     * Return the Remote Host Address of the STUN connection
     */
    IPAddress GetRemoteAddr() { return remoteAddr; };

    /**
     * Return the Remote Port Number of the STUN connection
     */
    uint16_t GetRemotePort() { return remotePort; };

    /**
     * Return the Local Host Address of the STUN connection
     */
    IPAddress GetLocalAddr() { return localAddr; };

    /**
     * Return the Remote Port Number of the STUN connection
     */
    uint16_t GetLocalPort() { return localPort; };

    /**
     * Return the value of usingTurn
     */
    bool GetUsingTurn() { return usingTurn; };

    /**
     * Return the value of maxMTU
     */
    size_t GetMtu() { return maxMTU; };

    /**
     * Return the Local Host Address of the TURN connection
     */
    IPAddress GetTurnAddr() { return turnAddr; };

    /**
     * Return the Remote Port Number of the TURN connection
     */
    uint16_t GetTurnPort() { return turnPort; };

    /**
     * Return the Local Host Address of the STUN connection
     */
    IPAddress GetStunAddr() { return STUNInfo.address; };

    /**
     * Return the Remote Port Number of the STUN connection
     */
    uint16_t GetStunPort() { return STUNInfo.port; };

    /**
     * Set the address of the TURN server
     */
    void SetTurnAddr(IPAddress address) { turnAddr = address; };

    /**
     * Set the port of the TURN server
     */
    void SetTurnPort(uint16_t port) { turnPort = port; };

    /**
     * Return the TURN user name
     */
    String GetTurnUserName() { return STUNInfo.acct; };

    /**
     * Return the HMAC Key
     */
    const uint8_t* GetHMACKey(void) const { return hmacKey; }

    /**
     * Return the HMAC Key length
     */
    size_t GetHMACKeyLength(void) const { return hmacKeyLen; }

    /**
     * Set the local server reflexive candidate details
     */
    void SetLocalSrflxCandidate(IPEndpoint& srflxCandidate) {
        localSrflxCandidate.addr = srflxCandidate.addr;
        localSrflxCandidate.port = srflxCandidate.port;
    }

    /**
     * Get the local server reflexive candidate details
     */
    void GetLocalSrflxCandidate(IPAddress& addr, uint16_t& port) {
        addr = localSrflxCandidate.addr;
        port = localSrflxCandidate.port;
    }

  private:
    struct StunBuffer {
        uint8_t* storage;
        uint8_t* buf;
        size_t len;
        IPAddress addr;
        uint16_t port;
        bool relayed;
        StunBuffer(size_t len) :
            storage(new uint8_t[len]), buf(storage), len(len), addr(), port(0), relayed(false) { }
    };

    Thread*rxThread;

    static const size_t MAX_APP_RX_QUEUE = 5;
    Mutex appQueueLock;
    Mutex stunMsgQueueLock;
    Event appQueueModified;
    Event stunMsgQueueModified;
    std::queue<StunBuffer> appQueue;
    std::queue<StunBuffer> stunMsgQueue;



    /// Number of framing bytes when handling a direct TCP connection.
    static const size_t FRAMING_SIZE = sizeof(uint16_t);

    IPAddress turnAddr;     ///< TURN server host address.
    uint16_t turnPort;      ///< TURN server port port.

    IPAddress remoteAddr;   ///< Remote host address.
    uint16_t remotePort;    ///< Remote port.

    IPAddress localAddr;   ///< Local host address.
    uint16_t localPort;    ///< Local port.

    IPEndpoint localSrflxCandidate;   ///< Local Server Reflexive candidate.

    SocketFd sockfd;       ///< Socket file descriptor.
    SocketType type;     ///< Socket type.

    bool connected;         ///< Flag indicating if the socket is connected or not.
    bool opened;            ///< Flag indicating if the socket has been opened or not.
    bool usingTurn;         ///< Flag indicating if the socket will be for communication
                            //   using a TURN server.
    bool autoFraming;       ///< Flag indicating if automatic framing will be done on TCP streams.

    Mutex frameLock;        ///< Mutex for blocking ICE or the App from
    //   sending in the middle of the other's frame
    //   for TCP streams.

    size_t rxFrameRemain;   ///< Number of octets remaining in a media stream RX frame.
    size_t txFrameRemain;   ///< Number of octets remaining in a media stream TX frame.

    uint8_t* rxLeftoverBuf; ///< Leftover buffer when receiving direct TCP data.
    uint8_t* rxLeftoverPos; ///< Position in leftover buffer.
    size_t rxLeftoverLen;   ///< Amount of data leftover.

    size_t maxMTU;          ///< Maximium MTU size of all interfaces.

    Component* component;   ///< FIXME: This should a void * and be made generic.

    /// Map of STUN Transaction IDs and HMAC-Keys and the length of the keys.
    StunMessage::ExpectedResponseMap expectedResponses;


    STUNServerInfo STUNInfo;

    /**
     * Internal constructor used when accepting a connection.
     *
     * @param sockfd        Socket descriptor of the new socket connection.
     * @param type          Socket type.
     * @param remoteAddr    Remote address of the connection.
     * @param remotePort    Remote port of the connection.
     * @param autoFraming   Handle automatic data/STUN framing for direct TCP connections.
     */
    Stun(SocketFd sockfd,
         SocketType type,
         IPAddress& remoteAddr,
         uint16_t remotePort,
         bool autoFraming = false);

    void ReceiveTCP(void);
    void ReceiveUDP(void);

    static ThreadReturn STDCALL RxThread(void*arg);

    /**
     * Process leftover framed STUN message (direct TCP connection only).
     *
     * @return  Indication of success or failure.
     */
    QStatus ProcessLeftoverSTUNFrame(void);

    /**
     * Process leftover framed application data (have full data frame).
     *
     * @param appBufFill    Fill level of application's data buffers.
     * @param appBufSpace   Size of application's data buffers.
     * @param fillSG        SG List used to fill application's data buffers.
     * @param checkingFrame Flag indicating if the frame needs to be checked for
     *                      STUN message.
     * @param extraBuf      Buffer to hold overflow data.
     */
    void ProcessLeftoverAppFrame(size_t& appBufFill, size_t appBufSpace,
                                 ScatterGatherList& fillSG, bool checkingFrame,
                                 uint8_t*& extraBuf);

    /**
     * Process leftover framed application data (have full data frame).
     *
     * @param appBufFill    Fill level of application's data buffers.
     * @param appBufSpace   Size of application's data buffers.
     * @param fillSG        SG List used to fill application's data buffers.
     * @param checkSG       SG List used for checking for STUN message.
     * @param extraBuf      Buffer to hold overflow data.
     *
     * @return  Indication of success or failure.
     */
    QStatus ProcessLeftoverRXFrameData(size_t& appBufFill,
                                       size_t appBufSpace,
                                       ScatterGatherList& fillSG,
                                       ScatterGatherList& checkSG,
                                       uint8_t*& extraBuf);

    /**
     * Process unchecked received data (may contain a STUN message).
     *
     * @param appBufFill    Fill level of application's data buffers.
     * @param appBufSpace   Size of application's data buffers.
     * @param checkSG       SG List used for checking for STUN message.
     * @param extraBuf      Buffer to hold overflow data.
     *
     * @return  Indication of success or failure.
     */
    QStatus ProcessUncheckedRXFrameData(size_t& appBufFill,
                                        size_t appBufSpace,
                                        ScatterGatherList& checkSG,
                                        uint8_t*& extraBuf);

    /**
     * Receive framed data into an SG list and process it.
     *
     * @param appSG     Application data SG list.
     * @param received  [OUT] Number of bytes received.
     *
     * @return  Indication of success or failure.
     */
    QStatus ReceiveAppFramedSG(ScatterGatherList& appSG, size_t& received);

    const uint8_t* hmacKey;

    size_t hmacKeyLen;
};

} //namespace ajn

#undef QCC_MODULE
#endif
