/**
 * @file
 *
 * Define the abstracted socket interface.
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
#ifndef _QCC_SOCKET_H
#define _QCC_SOCKET_H

#include <qcc/platform.h>

#include <qcc/IPAddress.h>
#include <qcc/SocketTypes.h>
#include <qcc/SocketWrapper.h>

#include <Status.h>

namespace qcc {

/**
 * Return the error that was set as a result of the last failing (sysem) operation.
 *
 * Many operating systems or system libraries may provide access to a generic
 * error number via a variable, macro or function.  This function provides
 * access to the OS-specific errors in a consistent way; but ultimately, the
 * error number recovered may be system and location specific.
 *
 * @return  The last error set by the underlying system..
 *
 * @see GetLastErrorString()
 *
 * @warning This function returns the last error encountered by the underlying
 * system, not necessarily the last error encountered by this library.
 *
 * @warning This error may valid only after a function known to set the error has
 * actually encountered an error.
 */
uint32_t GetLastError();

/**
 * Map the error number last set by the underlying system to an OS- and
 * locale-dependent error message String.
 *
 * @return A String containing an error message corresponding to the last error
 * number set by the underlying system.
 *
 * @see GetLastError()
 */
qcc::String GetLastErrorString();

/**
 * The maximum number of files descriptors that can be sent or received by this implementations.
 * See SendWithFds() and RecvWithFds().
 */
static const size_t SOCKET_MAX_FILE_DESCRIPTORS = 16;

/**
 * Open a socket.
 *
 * @param addrFamily    Address family.
 * @param type          Socket type.
 * @param[out] sockfd   Socket descriptor if successful.
 *
 * @return  Indication of success of failure.
 */
QStatus Socket(AddressFamily addrFamily, SocketType type, SocketFd& sockfd);

/**
 * Connect a socket to a remote host on a specified port and set it
 * non-blocking.
 *
 * @param sockfd        Socket descriptor.
 * @param remoteAddr    IP Address of remote host.
 * @param remotePort    IP Port on remote host.
 *
 * @return  Indication of success of failure.
 */
QStatus Connect(SocketFd sockfd, const IPAddress& remoteAddr, uint16_t remotePort);

/**
 * Connect a local socket and set it non-blocking.
 *
 * @param sockfd        Socket descriptor.
 * @param pathName      Path for the socket
 *
 * @return  Indication of success of failure.
 */
QStatus Connect(SocketFd sockfd, const char* pathName);

/**
 * Bind a socket to an address and port.
 *
 * @param sockfd        Socket descriptor.
 * @param localAddr     IP Address to bind on the local host (maybe 0.0.0.0 or ::).
 * @param localPort     IP Port to bind on the local host.
 *
 * @return  Indication of success of failure.
 */
QStatus Bind(SocketFd sockfd, const IPAddress& localAddr, uint16_t localPort);

/**
 * Bind a socket to a local path name
 *
 * @param sockfd        Socket descriptor.
 * @param pathName      Path for the socket
 *
 * @return  Indication of success of failure.
 */
QStatus Bind(SocketFd sockfd, const char* pathName);

/**
 * Listen for incoming connections on a bound socket.
 *
 * @param sockfd        Socket descriptor.
 * @param backlog       Number of pending connections to queue up.
 *
 * @return  Indication of success of failure.
 */
QStatus Listen(SocketFd sockfd, int backlog);

/**
 * Accept an incoming connection from a remote host.
 *
 * @param sockfd          Socket descriptor.
 * @param[out] remoteAddr IP Address of remote host.
 * @param[out] remotePort IP Port on remote host.
 * @param[out] newSockfd  New, non-blocking, socket descriptor for the connection.
 *
 * @return  Indication of success of failure.
 */
QStatus Accept(SocketFd sockfd, IPAddress& remoteAddr, uint16_t& remotePort, SocketFd& newSockfd);

/**
 * Accept an incoming connection from a remote host.
 *
 * @param sockfd         Socket descriptor.
 * @param[out] newSockfd New, non-blocking, socket descriptor for the connection.
 *
 * @return  Indication of success of failure.
 */
QStatus Accept(SocketFd sockfd, SocketFd& newSockfd);

/**
 * Shutdown a connection.
 *
 * @param sockfd        Socket descriptor.
 * @param how           What parts of a full-duplex connection to shutdown.
 *
 * @return  Indication of success of failure.
 */
QStatus Shutdown(SocketFd sockfd, ShutdownHow how);

/**
 * Shutdown a connection.
 *
 * @param sockfd        Socket descriptor.
 *
 * @return  Indication of success of failure.
 */
QStatus Shutdown(SocketFd sockfd);

/**
 * Close a socket descriptor.  This releases the bound port number.
 *
 * @param sockfd        Socket descriptor.
 */
void Close(SocketFd sockfd);

/**
 * Duplicate a socket descriptor.
 *
 * @param sockfd The socket descriptor to duplicate
 * @param[out] dupSock The duplicated socket descriptor.
 *
 * @return  Indication of success of failure.
 */
QStatus SocketDup(SocketFd sockfd, SocketFd& dupSock);

/**
 * Create a connected pair of (local domain) sockets.
 *
 * @param[out] sockets Array of two sockets
 *
 * @return
 * - #ER_OK creation succeeded.
 * - #ER_OS_ERROR the underlying creation failed.
 */
QStatus SocketPair(SocketFd(&sockets)[2]);

/**
 * Get the local address of the socket.
 *
 * @param sockfd        Socket descriptor.
 * @param addr          IP Address of the local host.
 * @param port          IP Port on local host.
 *
 * @return  Indication of success of failure.
 */
QStatus GetLocalAddress(SocketFd sockfd, IPAddress& addr, uint16_t& port);

/**
 * Send a buffer of data to a remote host on a socket.
 *
 * @param sockfd        Socket descriptor.
 * @param remoteAddr    IP Address of remote host.
 * @param remotePort    IP Port on remote host.
 * @param buf           Pointer to the buffer containing the data to send.
 * @param len           Number of octets in the buffer to be sent.
 * @param[out] sent     Number of octets sent.
 * @param flags         SendMsgFlags to underlying sockets call (see sendmsg() in sockets API)
 *
 * @return  Indication of success of failure.
 *
 * @warning Not all flags defined in SendMsgFlags are supported on all platforms.
 *          Platform-dependent code may modify flags.
 */
QStatus SendTo(SocketFd sockfd, IPAddress& remoteAddr, uint16_t remotePort,
               const void* buf, size_t len, size_t& sent, SendMsgFlags flags = QCC_MSG_NONE);

/**
 * Send a buffer of data to a remote host on a socket.
 *
 * @param sockfd        Socket descriptor.
 * @param remoteAddr    IP Address of remote host.
 * @param remotePort    IP Port on remote host.
 * @param scopeId       Scope ID of address.
 * @param buf           Pointer to the buffer containing the data to send.
 * @param len           Number of octets in the buffer to be sent.
 * @param[out] sent     Number of octets sent.
 * @param flags         SendMsgFlags to underlying sockets call (see sendmsg() in sockets API)
 *
 * @return  Indication of success of failure.
 *
 * @warning Not all flags defined in SendMsgFlags are supported on all platforms.
 *          Platform-dependent code may modify flags.
 */
QStatus SendTo(SocketFd sockfd, IPAddress& remoteAddr, uint16_t remotePort, uint32_t scopeId,
               const void* buf, size_t len, size_t& sent, SendMsgFlags flags = QCC_MSG_NONE);

/**
 * Receive a buffer of data from a remote host on a socket.
 * This call will block until data is available, the socket is closed.
 *
 * @param sockfd        Socket descriptor.
 * @param buf           Pointer to the buffer where received data will be stored.
 *                      This must not be NULL.
 * @param len           Size of the buffer in octets.
 * @param[out] received Number of octets received.
 *
 * @return  Indication of success of failure.
 */
QStatus Recv(SocketFd sockfd, void* buf, size_t len, size_t& received);

/**
 * Receive a buffer of data from a remote host on a socket.
 *
 * @param sockfd        Socket descriptor.
 * @param remoteAddr    IP Address of remote host.
 * @param remotePort    IP Port on remote host.
 * @param buf           Pointer to the buffer where received data will be stored.
 * @param len           Size of the buffer in octets.
 * @param[out] received Number of octets received.
 *
 * @return  Indication of success of failure.
 */
QStatus RecvFrom(SocketFd sockfd, IPAddress& remoteAddr, uint16_t& remotePort,
                 void* buf, size_t len, size_t& received);

/**
 * Receive a buffer of data and ancillary data from a remote host on a socket.
 *
 * @param sockfd              Socket descriptor.
 * @param remoteAddr          IP Address of remote host.
 * @param remotePort          IP Port on remote host.
 * @param buf                 Pointer to the buffer where received data will be stored.
 * @param len                 Size of the buffer in octets.
 * @param[out] received       Number of octets received.
 * @param[out] interfaceIndex Interface index.
 *
 * @return  Indication of success of failure.
 */
QStatus RecvWithAncillaryData(SocketFd sockfd, IPAddress& remoteAddr, uint16_t& remotePort, IPAddress& localAddr,
                              void* buf, size_t len, size_t& received, int32_t& interfaceIndex);

/**
 * Receive a buffer of data from a remote host on a socket and any file descriptors accompanying the
 * data.  This call will block until data is available, the socket is closed.
 *
 * @param sockfd         Socket descriptor.
 * @param buf            Pointer to the buffer where received data will be stored.
 * @param len            Size of the buffer in octets.
 * @param[out] received  Number of octets received.
 * @param fdList         The file descriptors received.
 * @param maxFds         The maximum number of file descriptors (size of fdList)
 * @param[out] recvdFds  The number of file descriptors received.
 *
 * @return
 * - #ER_BAD_ARG_5 fdList is NULL.
 * - #ER_BAD_ARG_6 numFds is 0.
 * - #ER_OK the receive succeeded.
 * - #ER_OS_ERROR the underlying receive failed.
 * - #ER_WOULDBLOCK sockfd is non-blocking and the underlying receive would block.
 */
QStatus RecvWithFds(SocketFd sockfd, void* buf, size_t len, size_t& received, SocketFd* fdList, size_t maxFds, size_t& recvdFds);

/**
 * Send a buffer of data with file descriptors to a socket. Depending on the transport this may may use out-of-band
 * or in-band data or some mix of the two.
 *
 * @param sockfd    Socket descriptor.
 * @param buf       Pointer to the buffer containing the data to send.
 * @param len       Number of octets in the buffer to be sent.
 * @param[out] sent Number of octets sent.
 * @param fdList    Array of file descriptors to send.
 * @param numFds    Number of files descriptors.
 * @param pid       Process ID required on some platforms.
 *
 * @return
 * - #ER_BAD_ARG_5 fdList is NULL.
 * - #ER_BAD_ARG_6 numFds is 0 or numFds is greater than #SOCKET_MAX_FILE_DESCRIPTORS.
 * - #ER_OK the send succeeded.
 * - #ER_OS_ERROR the underlying send failed.
 * - #ER_WOULDBLOCK sockfd is non-blocking and the underlying send would block.
 */
QStatus SendWithFds(SocketFd sockfd, const void* buf, size_t len, size_t& sent, SocketFd* fdList, size_t numFds, uint32_t pid);

/**
 * Set a socket to blocking or not blocking.
 *
 * @param sockfd    Socket descriptor.
 * @param blocking  If true set it to blocking, otherwise no-blocking.
 */
QStatus SetBlocking(SocketFd sockfd, bool blocking);

/**
 * Sets the maximum socket send buffer size (in bytes).
 *
 * @warning Operating systems are free to modify this value according to
 * specific platform requirements.  For example, Linux systems will double this
 * value to account for bookkeeping overhead; so a subsequent GetSndBuf() will
 * return twice the value passed in SetSndBuf().
 *
 * @param sockfd  Socket descriptor.
 * @param size    Requested maximum send buffer size.
 */
QStatus SetSndBuf(SocketFd sockfd, size_t bufSize);

/**
 * Gets the current maximum socket send buffer size (in bytes).
 *
 * @warning Operating systems are free to modify adjust buffer sizes according
 * to specific platform requirements.  For example, Linux systems will double
 * the value passed in a SetEndBuf() to account for bookkeeping overhead; so a
 * previous call to SetSndBuf() will cause a subsequent GetSndBuf() to return
 * twice the value originally set.
 *
 * @param sockfd  Socket descriptor.
 * @param size    Buffer to receive the current buffer size.
 */
QStatus GetSndBuf(SocketFd sockfd, size_t& bufSize);

/**
 * Sets the maximum socket receive buffer size (in bytes).
 *
 * @warning Operating systems are free to modify this value according to
 * specific platform requirements.  For example, Linux systems will double this
 * value to account for bookkeeping overhead; so a subsequent GetSndBuf() will
 * return twice the value passed in SetSndBuf().
 *
 * @param sockfd  Socket descriptor.
 * @param size    Requested maximum send buffer size.
 */
QStatus SetRcvBuf(SocketFd sockfd, size_t bufSize);

/**
 * Gets the current maximum socket receive buffer size (in bytes).
 *
 * @warning Operating systems are free to modify adjust buffer sizes according
 * to specific platform requirements.  For example, Linux systems will double
 * the value passed in a SetEndBuf() to account for bookkeeping overhead; so a
 * previous call to SetSndBuf() will cause a subsequent GetSndBuf() to return
 * twice the value originally set.
 *
 * @param sockfd  Socket descriptor.
 * @param size    Buffer to receive the current buffer size.
 */
QStatus GetRcvBuf(SocketFd sockfd, size_t& bufSize);

/**
 * Set TCP based socket to use or not use linger functionality
 *
 * @param sockfd  Socket descriptor.
 * @param onoff   Turn linger on if true.
 * @param linger  Time to linger if onoff is true.
 */
QStatus SetLinger(SocketFd sockfd, bool onoff, uint32_t linger);

/**
 * Set TCP based socket to use or not use Nagle algorithm (TCP_NODELAY)
 *
 * @param sockfd    Socket descriptor.
 * @param useNagle  Set to true to Nagle algorithm. Set to false to disable Nagle.
 */
QStatus SetNagle(SocketFd sockfd, bool useNagle);

/**
 * @brief Allow a service to bind to a TCP endpoint which is in the TIME_WAIT
 * state.
 *
 * Setting this option allows a service to be restarted after a crash (or
 * contrl-C) and then be restarted without having to wait for some possibly
 * significant (on the order of minutes) time.
 *
 * @warning Implementation detail: The Darwin platform uses SO_REUSEPORT while
 * Windows uses SO_EXCLUSIVEADDRUSE.  SO_REUSEADDR is used for all Linux based
 * platforms (including Android).
 *
 * @see SetReusePort()
 *
 * @param sockfd The socket descriptor identifying the resource.
 * @param reuse  Set to true to allow address and/or port reuse.
 */
QStatus SetReuseAddress(SocketFd sockfd, bool reuse);

/**
 * @brief Allow multiple services to bind to the same address and port.
 *
 * Setting this option allows a multiple services to bind to the same address
 * and port.  This is typically useful for multicast operations where more than
 * one process might want to listen on the same muticast address and port.  In
 * order to use this function successfully, ALL processes that want to listen on
 * a common port must set this option.
 *
 * @warning Implementation detail: The Darwin platform uses SO_REUSEPORT while
 * use SO_REUSEADDR is used for other platforms Windows and Linux based
 * platforms (including Android).  The original socket option SO_REUSEADDR was a
 * BSD-ism that was introduced when multicast was added there.  For posix platforms,
 * there is no functional difference between SetReusePort() and SetReuseAddress().
 *
 * @see SetReuseAddress()
 *
 * @param sockfd The socket descriptor identifying the resource.
 * @param reuse  Set to true to allow address and/or port reuse.
 */
QStatus SetReusePort(SocketFd sockfd, bool reuse);

/**
 * Ask a UDP-based socket to join the specified multicast group.
 *
 * Arrange for the system to perform an IGMP join and enable reception of
 * packets addressed to the provided multicast group.  The details of the
 * particular process used to drive the join depend on the address family of the
 * socket.  Since this call may be made on a bound or unbound socket, and there
 * is no way to discover the address family from an unbound socket, we require
 * that the address family of the socket be provided in this call.
 *
 * @param sockfd The socket file descriptor identifying the resource.
 * @param family The address family used to create the provided sockfd.
 * @param multicastGroup A string containing the desired multicast group in
 *     presentation format.
 * @param interface A string containing the interface name (e.g., "wlan0") on
 *     which to join the group.
 */
QStatus JoinMulticastGroup(SocketFd sockfd, AddressFamily family, String multicastGroup, String iface);

/**
 * Ask a UDP-based socket to join the specified multicast group.
 *
 * Arrange for the system to perform an IGMP leave and disable reception of
 * packets addressed to the provided multicast group.  The details of the
 * particular process used to drive the join depend on the address family of the
 * socket.  Since this call may be made on a bound or unbound socket, and there
 * is no way to discover the address family from an unbound socket, we require
 * that the address family of the socket be provided in this call.
 *
 * @param sockFd The socket file descriptor identifying the resource.
 * @param family The address family used to create the prvided socket.
 * @param multicastGroup A string containing the desired multicast group in
 *     presentation format.
 * @param interface A string containing the interface name (e.g., "wlan0") on
 *     which to join the group.
 */
QStatus LeaveMulticastGroup(SocketFd socketFd, AddressFamily family, String multicastGroup, String iface);

/**
 * Set the outbound interface over which multicast packets are sent using
 * the provided socket.
 *
 * Override the internal OS routing of multicast packets and specify which
 * single interface over which multicast packets sent using this socket will be
 * sent.
 *
 * @param sockfd The socket file descriptor identifying the resource.
 * @param interface A string containing the desired interface (e.g., "wlan0"),
 */
QStatus SetMulticastInterface(SocketFd socketFd, AddressFamily family, qcc::String iface);

/**
 * Set the number of hops over which multicast packets will be routed when
 * sent using the provided socket.
 *
 * @param sockfd The socket file descriptor identifying the resource.
 * @param hops The desired number of hops.
 */
QStatus SetMulticastHops(SocketFd socketFd, AddressFamily family, uint32_t hops);

/**
 * Set the broadcast option on the provided socket.
 *
 * @param sockfd The socket descriptor identifying the resource.
 * @param broadcast  Set to true to enable broadcast on the socket.
 */
QStatus SetBroadcast(SocketFd sockfd, bool broadcast);

/**
 * Set the option to receive ancillary data on the provided socket.
 *
 * @param sockfd The socket descriptor identifying the resource.
 * @param recv Set to true to enable receipt of ancillary data.
 */
QStatus SetRecvPktAncillaryData(SocketFd sockfd, AddressFamily addrFamily, bool recv);

/**
 * Set the option to receive only IPv6 packets on the provided socket.
 *
 * @param sockfd The socket descriptor identifying the resource.
 * @param recv  Set to true to enable receipt of only IPv6 packets.
 */
QStatus SetRecvIPv6Only(SocketFd sockfd, bool recv);

} // namespace qcc

#endif // _QCC_SOCKET_H
