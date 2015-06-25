/**
 * @file
 *
 * This file defines the IP Address abstraction class.
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

#ifndef _QCC_IPADDRESS_H
#define _QCC_IPADDRESS_H


#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/Event.h>
#include <qcc/String.h>
#include <assert.h>
#include <string.h>

#include <qcc/SocketTypes.h>

#include <Status.h>

/** @internal */
#define QCC_MODULE "NETWORK"

namespace qcc {

/**
 * IP Address class for handling IPv4 and IPv6 addresses uniformly.  Even
 * supports automtic mapping of IPv4 addresses on to IPv6 address spaces.
 */
class IPAddress {
  public:

    static const size_t IPv4_SIZE = 4;  ///< Size of IPv4 address.
    static const size_t IPv6_SIZE = 16; ///< Size of IPv6 address.
    /**
     * Default constructor initializes an invalid IP address.  Generally, this
     * should not be used but is still made available if needed.
     */
    IPAddress() : addrSize(0) { ::memset(addr, 0, sizeof(addr)); }

    /**
     * Consruct IPAddress based on a string containing an IPv4 or IPv6 address.
     *
     * @param addrString    Reference to the string with the IP address.
     */
    IPAddress(const qcc::String& addrString);

    /**
     * Set or change the address that an existing IPAddress refers to.
     * Using this method instead of using the constructor allows
     * errors to be returned.
     *
     * @param addrString     IP address (V4 or V6) or hostname if allowHostnames == true.
     * @param allowHostnames If true allows addresses to be specified as host names.
     * @param timeoutMs      Timeout when resolving host names.
     * @return ER_OK if successful.
     */
    QStatus SetAddress(const qcc::String& addrString, bool allowHostnames = true, uint32_t timeoutMs = qcc::Event::WAIT_FOREVER);

    /**
     * Consruct IPAddress based on a buffer containing an IPv4 or IPv6 address.
     *
     * @param addrBuf       Pointer to the buffer with the IP address.
     * @param addrBufSize   Size of the address buffer.
     */
    IPAddress(const uint8_t* addrBuf, size_t addrBufSize);

    /**
     * Construct and IPv4 address from a 32-bit integer.
     *
     * @param ipv4Addr  32-bit integer representation of the IP address.
     */
    IPAddress(uint32_t ipv4Addr);

    /**
     * Compare equality between 2 IPAddress's.
     *
     * @param other     The other IPAddress to compare against.
     *
     * @return  'true' if they are the same, 'false' otherwise.
     */
    bool operator==(const IPAddress& other) const
    {
        return ((Size() == other.Size()) &&
                (memcmp(GetIPReference(), other.GetIPReference(), Size()) == 0));
    }

    /**
     * Compare inequality between 2 IPAddress's.
     *
     * @param other     The other IPAddress to compare against.
     *
     * @return  'true' if they are not the same, 'false' otherwise.
     */
    bool operator!=(const IPAddress& other) const { return !(*this == other); }

    /**
     * Get the size of the IP address.
     *
     * @return  The size of the IP address.
     */
    size_t Size(void) const { return addrSize; }

    /**
     * Test if IP address is an IPv4 address.
     *
     * @return  "true" if IP address is an IPv4 address.
     */
    bool IsIPv4(void) const { return addrSize == IPv4_SIZE; }

    /**
     * Test if IP address is an IPv6 address.
     *
     * @return  "true" if IP address is an IPv6 address.
     */
    bool IsIPv6(void) const { return addrSize == IPv6_SIZE; }

    /**
     * Test if IP address is a loopback address.
     *
     * @return  "true" if IP address is a loopback.
     */
    bool IsLoopback(void) const
    {
        if (addrSize == IPv4_SIZE) {
            uint32_t ipv4Addr =  GetIPv4AddressCPUOrder();
            /*
             * A Loopback address in IPv4 is 127.X.Y.Z for all X, Y, Z
             */
            if ((ipv4Addr & 0x7f000000) != (127 << 24)) {
                return false;
            }
        } else {
            /*
             * The Loopback address in IPv6 is ::1
             *
             * This is 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 01 which is
             * three uint32_t 0x00000000 followed by one 0x00000001 (in network
             * order).
             */
            for (uint32_t i = 0; i < 3; ++i) {
                if (*(uint32_t*)(&addr[i << 2])) {
                    return false;
                }
            }
            uint32_t last = *(uint32_t*)(&addr[12]);
#if (QCC_TARGET_ENDIAN == QCC_LITTLE_ENDIAN)
            if (last != 0x01000000) {
                return false;
            }
#else
            if (last != 0x00000001) {
                return false;
            }
#endif
        }
        return true;
    }

    /**
     * Convert the IP address into a human readable form in a string.  IPv4
     * addresses will use the standard "dot-quad" notation (i.e., 127.0.0.1)
     * and IPv6 addresses will use the standard notation as defined in RFC
     * 4291 (i.e., ::1).
     *
     * @return  The string representation of the IP address.
     */
    qcc::String ToString(void) const
    {
        if (addrSize == IPv4_SIZE) {
            return IPv4ToString(&addr[IPv6_SIZE - IPv4_SIZE]);
        } else if (addrSize == IPv6_SIZE) {
            return IPv6ToString(addr);
        } else {
            return qcc::String("<invalid IP address>");
        }
    }

    /**
     * Helper function to convert an IPv4 address in a buffer to a string.
     *
     * @param addrBuf   Buffer at least 4 octets long containing the IPv4 address.
     *
     * @return  The string representation of the IPv4 address.
     */
    static qcc::String AJ_CALL IPv4ToString(const uint8_t addrBuf[]);

    /**
     * Helper function to convert an IPv6 address in a buffer to a string.
     *
     * @param addrBuf   Buffer at least 16 octets long containing the IPv6 address.
     *
     * @return  The string representation of the IPv6 address.
     */
    static qcc::String AJ_CALL IPv6ToString(const uint8_t addrBuf[]);

    /**
     * Helper function to convert an IPv6 address string to its byte packed equivalent.
     *
     * @param address           IPv6 string to convert
     * @param addrBuf           Buffer to store the packed bytes from conversion
     * @param addrBufSize   Length of the addrBuf buffer. This must be 16.
     *
     * @return  ER_OK iff conversion was successful.
     */
    static QStatus AJ_CALL StringToIPv6(const qcc::String& address, uint8_t addrBuf[], size_t addrBufSize);

    /**
     * Helper function to convert an IPv6 address string to its byte packed equivalent.
     *
     * @param address           IPv4 string to convert
     * @param addrBuf           Buffer to store the packed bytes from conversion
     * @param addrBufSize   Length of the addrBuf buffer. This must be 4.
     *
     * @return  ER_OK iff conversion was successful.
     */
    static QStatus AJ_CALL StringToIPv4(const qcc::String& address, uint8_t addrBuf[], size_t addrBufSize);

    /**
     * Renders the IPv4 address in binary format into a buffer.
     *
     * @param addrBuf       An array for storing the IPv4 address.
     * @param addrBufSize   Size of the address buffer.
     *
     * @return  Status indicating success or failure.
     */
    QStatus RenderIPv4Binary(uint8_t addrBuf[], size_t addrBufSize) const;

    /**
     * Renders the IPv6 address in binary format into a buffer.
     *
     * @param addrBuf       An array for storing the IPv6 address.
     * @param addrBufSize   Size of the address buffer.
     *
     * @return  Status indicating success or failure.
     */
    QStatus RenderIPv6Binary(uint8_t addrBuf[], size_t addrBufSize) const;

    /**
     * Renders the IP address (IPv4 or IPv6) in binary format into a buffer.
     *
     * @param addrBuf       An array for storing the IP address.
     * @param addrBufSize   Size of the address buffer.
     *
     * @return  Status indicating success or failure.
     */
    QStatus RenderIPBinary(uint8_t addrBuf[], size_t addrBufSize) const;

    /**
     * Get a direct reference to the IPv6 address.
     *
     * @return  A constant pointer to the address buffer.
     */
    const uint8_t* GetIPv6Reference(void) const { return addr; }

    /**
     * Get a direct reference to the IPv4 address.
     *
     * @return  A constant pointer to the address buffer.
     */
    const uint8_t* GetIPv4Reference(void) const { return &addr[IPv6_SIZE - IPv4_SIZE]; }

    /**
     * Get a direct reference to the IP address.
     *
     * @return  A constant pointer to the address buffer.
     */
    const uint8_t* GetIPReference(void) const { return &addr[IPv6_SIZE - addrSize]; }

    /**
     * Get the IPv4 address as a 32-bit unsigned integer in CPU order.
     *
     * @return  A 32-bit unsigned integer representation of the IPv4 address.
     */
    uint32_t GetIPv4AddressCPUOrder(void) const;

    /**
     * Get the IPv4 address as a 32-bit unsigned integer in network order.
     *
     * @return  A 32-bit unsigned integer representation of the IPv4 address.
     */
    uint32_t GetIPv4AddressNetOrder(void) const;

    /**
     * Convert the IP address to an IPv4 address.
     *
     * @attention This is only useful for converting IPv6 addresses that were
     * previously converted from an IPv4 address.
     */
    void ConvertToIPv4(void) { addrSize = IPv4_SIZE; }

    /**
     * Convert the IP address to an IPv6 address.  This results in an
     * IPv4-mapped-on-IPv6 address (e.g., ::ffff:a0a:2020 for 10.10.32.32).
     */
    void ConvertToIPv6(void) { addrSize = IPv6_SIZE; }

    /**
     * Get the address family for the IPAddress.
     *
     * @return  The address family for this address.
     */
    AddressFamily GetAddressFamily(void) const { return (IsIPv4() ? QCC_AF_INET : QCC_AF_INET6); }

  private:
    uint8_t addr[IPv6_SIZE];    ///< Storage for IP address.
    uint16_t addrSize;          ///< IP address size (indirectly indicates IPv4 vs. IPv6).

};


/**
 * IpEndpoint describes an address/port endpoint for an IP-based connection.
 *
 */
class IPEndpoint {
  public:

    /**
     * Empty Constructor
     */
    IPEndpoint() : port(0) { }

    /**
     * Construct and IPEndpoint from a string and port
     *
     * @param addrString  The address
     * @param port        The port
     */
    IPEndpoint(const qcc::String& addrString, uint16_t port) : addr(addrString), port(port) { }

    /**
     * Construct and IPEndpoint from a address and port
     *
     * @param addrString  The address
     * @param port        The port
     */
    IPEndpoint(qcc::IPAddress& addr, uint16_t port) : addr(addr), port(port) { }

    /**
     * Get the port for the IPEndpoint.
     *
     * @return  The port for this endpoint.
     */
    uint16_t GetPort() const { return port; }

    /**
     * Get the IP Address for the IPEndpoint.
     *
     * @return  The IP Address for this endpoint.
     */
    qcc::IPAddress GetAddress() const { return addr; }

    /** Address */
    qcc::IPAddress addr;

    /** Port */
    uint16_t port;

    /**
     * Equality test
     * @param other  Endpoint to compare with.
     * @return true iff other equals this IPAddress.
     */
    bool operator==(const qcc::IPEndpoint& other) const { return ((addr == other.addr) && (port == other.port)); }

    /**
     * Inequality test
     * @param other  Endpoint to compare with.
     * @return true iff other does not equal this IPAddress.
     */
    bool operator!=(const qcc::IPEndpoint& other) const { return !(*this == other); }

    /**
     * Human readable version of IPEndpoint.
     */
    qcc::String ToString() const;

};

}

#undef QCC_MODULE
#endif
