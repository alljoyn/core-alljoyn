/**
 * @file
 * @internal
 * Data structures used for a lightweight service protocol.
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

#ifndef _NS_PROTOCOL_H
#define _NS_PROTOCOL_H

#ifndef __cplusplus
#error Only include IpNsProtocol.h in C++ code.
#endif

#include <vector>
#include <qcc/String.h>
#include <qcc/IPAddress.h>
#include <alljoyn/TransportMask.h>
#include <alljoyn/Status.h>

namespace ajn {

/**
 * @defgroup name_service_protocol Name Service Protocol
 * @{
 * <b>Introduction</b>
 *
 * One goal of AllJoyn is to allow clients of the bus to make Remote Procedure Calls
 * (RPC) or receive Signals from physically remote obejcts connected to the
 * bus as if they were local.  Collections of RCP and Signal signatures,
 * typically called interfaces.  Bus attachments are collections of interface
 * implementations and are described by so-called well-known or bus names.
 * Groups of one or more bus attachments are coordinated by AllJoyn daemon
 * processes that run on each host.  Physically or logically distributed AllJoyn
 * daemons may be merged into a single virtual bus.
 *
 * One of the fundamental issues in distributing processes across different
 * hosts is the discovering the address and port of a given service.  In the
 * case of AllJoyn, the communication endpoints of the various daemon processes
 * must be located so communication paths may be established.  This lightweight
 * name service protocol provides a definition of a protocol for such a
 * process.
 *
 * <b>Transport</b>
 *
 * Name service protocol messages are expected to be transported
 * over UDP, typically over a well-known multicast group and port.  A
 * UDP datagram carrying a name service message would appear like,
 *
 * @verbatim
 *      0                   1                   2                   3
 *      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |        Source Port            |      Destination Port         |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |           Length              |           Checksum            |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |                      Name Service Packet                      |
 *     ~                                                               ~
 *     |                                                               |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * @endverbatim
 *
 * <b>Strings</b>
 *
 * Since well-known names are strings, one of the fundamental objects in
 * the protocol is the StringData object.  Strings are encoded an octet
 * giving the length of the string, followed by some number UTF-8 characters.
 * (no terminating zero is required)  For example, the string "STRING"
 * would be encoded as follows.  The single octet length means that the
 * longest string possible is 255 characters.  This should not prove to
 * be a problem since it is the same maximum length as a domain name, on
 * which bus names are modeled.
 *
 * @verbatim
 *      0                   1                   2                   3
 *      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |       6       |       S       |       T       |      R        |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                                               |
 *     |       I       |       N       |       G       |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * @endverbatim
 *
 * <b>IS-AT Message</b>
 *
 * The IS-AT message is an "answer" message used to advertise the existence
 * of a number of bus names on a given AllJoyn daemon.  IS-AT messages can
 * be sent as part of a response to a question, or they can be sent
 * gratuitously when an AllJoyn daemon decides to export the fact that it
 * supports some number of bus names.
 *
 * <b>Version 0</b>
 *
 * Version zero of the protocol includes the following fields:
 *
 * @verbatim
 *      0                   1                   2                   3
 *      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |F S U T C G| M |     Count     |              Port             |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |            IPv4Address present if 'F' bit is set              |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |                                                               |
 *     |            IPv6Address present if 'S' bit is set              |
 *     |                                                               |
 *     |                                                               |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |                                                               |
 *     ~       Daemon GUID StringData present if 'G' bit is set        ~
 *     |                                                               |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |                                                               |
 *     ~            Variable Number of StringData Records              ~
 *     |                                                               |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * @endverbatim
 *
 * @li @c M The message type of the IS-AT message.  Defined to be '01' (1).
 * @li @c G If '1' indicates that a variable length daemon GUID string is present.
 * @li @c C If '1' indicates that the list of StringData records is a complete
 *     list of all well-known names exported by the responding daemon.
 * @li @c T If '1' indicates that the responding daemon is listening on TCP.
 * @li @c U If '1' indicates that the responding daemon is listening on UDP.
 * @li @c S If '1' indicates that the responding daemon is listening on an IPv6
 *     address and that an IPv6 address is present in the message.  If '0'
 *     indicates is no IPv6 address present.
 * @li @c F If '1' indicates that the responding daemon is listening on an IPv4
 *     address and that an IPv4 address is present in the message.  If '0'
 *     indicates is no IPv4 address present.
 * @li @c Count The number of StringData items that follow.  Each StringData item
 *     describes one well-known bus name supported by the responding daemon.
 * @li @c Port The port on which the responding daemon is listening.
 * @li @c IPv4Address The IPv4 address on which the responding daemon is listening.
 *     Present if the 'F' bit is set to '1'.
 * @li @c IPv6Address The IPv6 address on which the responding daemon is listening.
 *     Present if the 'S' bit is set to '1'.
 *
 * <b>Version 1</b>
 *
 * Version one of the protocol extends version zero to include the type of the
 * transport that is doing the advertisement and admit the possibility of
 * reliable and unreliable transports over both IPv4 and IPv6.
 *
 * The most general form of an advertisement containing this information includes
 * Four address/port pairs:
 *
 *     R4: IPv4 address, port of a TCP-based endpoint present;
 *     U4: IPv4 address, port of a UDP-based endpoint present;
 *     R6: IPv6 address, port of a TCP-based endpoint present;
 *     U6: IPv6 address, port of a UDP-based endpoint present.
 *
 * One can contemplate various optimizations which would allow us to elide
 * addresses in some case, but the complexity of managing such optimizations is
 * such that we do the simple thing and send a complete endpoint description for
 * every endpoint.  It is the most general case and if we were able to optimize
 * all four addresses out we are talking about forty bytes.  The code complexity
 * just doesn't seem worth it; and so the four cases above replace the F, S, U,
 * T bits of the version zero protocol.
 *
 * Since the port is now associated to the provided addresses, the port field in the version
 * zero is-at message becomes a TransportMask which indicates the AllJoyn transport that is making the
 * advertisement.
 *
 * @verbatim
 *      0                   1                   2                   3
 *      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |R U R U C G| M |     Count     |         TransportMask         |
 *     |4 4 6 6    |   |               |                               |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |         R4 IPv4Address present if 'R4' bit is set             |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |  R4 Port if 'R4' bit is set   |  U4 Ipv4Address present if    |
 *     |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |       'U4' bit is set         |  U4 Port if 'U4' bit is set   |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |                                                               |
 *     |         R6 IPv6Address present if 'R6' bit is set             |
 *     |                                                               |
 *     |                                                               |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |  R6 Port if 'R6' bit is set   |                               |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
 *     |                                                               |
 *     |         R6 IPv6Address present if 'R6' bit is set             |
 *     |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |                               |                               |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
 *     |                                                               |
 *     ~       Daemon GUID StringData present if 'G' bit is set        ~
 *     |                                                               |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |                                                               |
 *     ~            Variable Number of StringData Records              ~
 *     |                                                               |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * @endverbatim
 *
 * @li @c M The message type of the IS-AT message.  Defined to be '01' (1).
 * @li @c G If '1' indicates that a variable length daemon GUID string is present.
 * @li @c C If '1' indicates that the list of StringData records is a complete
 *     list of all well-known names exported by the responding daemon.
 * @li @c R4 If '1' indicates that the IPv4 endpoint of a reliable method (TCP)
 *     transport (IP address and port) is present
 * @li @c U4 If '1' indicates that the IPv4 endpoint of an unreliable method (UDP)
 *     transport (IP address and port) is present
 * @li @c R6 If '1' indicates that the IPv6 endpoint of a reliable method (TCP)
 *     transport (IP address and port) is present
 * @li @c U6 If '1' indicates that the IPv6 endpoint of an unreliable method (UDP)
 *     transport (IP address and port) is present
 * @li @c Count The number of StringData items that follow.  Each StringData item
 *     describes one well-known bus name supported by the responding daemon.
 * @li @c TransportMask The bit mask of transport identifiers that indicates which
 *     AllJoyn transport is making the advertisement.
 *
 * <b>WHO-HAS Message</b>
 *
 * The WHO-HAS message is a "question" message used to ask AllJoyn daemons if
 * they support one or more bus names.
 *
 * <b>Version 0</b>
 *
 * Version zero of the protocol includes the following fields:
 *
 * @verbatim
 *      0                   1                   2                   3
 *      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |F S U T R R| M |     Count     |                               |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
 *     |                                                               |
 *     ~              Variable Number of StringData Records            ~
 *     |                                                               |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * @endverbatim
 *
 * @li @c M The message type of the WHO-HAS message.  Defined to be '10' (2)
 * @li @c R Reserved bit.
 * @li @c T If '1' indicates that the requesting daemon wants to connect using
 *     TCP.
 * @li @c U If '1' indicates that the requesting daemon wants to connect using
 *     UDP.
 * @li @c S If '1' indicates that the responding daemon is interested in receiving
 *     information about services accessible via IPv6 addressing.
 * @li @c F If '1' indicates that the responding daemon is interested in receiving
 *     information about services accessible via IPv4 addressing.
 * @li @c Count The number of StringData items that follow.  Each StringData item
 *     describes one well-known bus name that the querying daemon is interested in.
 *
 * <b>Version 1</b>
 *
 * Version one of the protocol removes the T, U, S and F bits; converting them to
 * reserved bits.  The rationale is that passive observers of name service packets
 * could be interested in all available configurations and artificially limiting
 * responses could mislead those observers.
 *
 * Version one of the protocol includes the following fields:
 *
 * @verbatim
 *      0                   1                   2                   3
 *      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |R R R R R R| M |     Count     |                               |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
 *     |                                                               |
 *     ~              Variable Number of StringData Records            ~
 *     |                                                               |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * @endverbatim
 *
 * @li @c M The message type of the WHO-HAS message.  Defined to be '10' (2)
 * @li @c R Reserved bit.
 * @li @c Count The number of StringData items that follow.  Each StringData item
 *     describes one well-known bus name that the querying daemon is interested in.
 *
 * <b>Messages<b>
 *
 * A name service message consists of a header, followed by a variable
 * number of question (Q) messages (for example, WHO-HAS) followed by a variable
 * number of answer(A) messages (for example, IS-AT).  All messages are packed
 * to octet boundaries.
 *
 * <b>Name Service Header</b>
 *
 * @verbatim
 *      0                   1                   2                   3
 *      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |    Version    |    QCount     |    ACount     |     Timer     |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * @endverbatim
 *
 * @li @c Version The version of the protocol.
 * @li @c QCount The number of question messages that follow the header.
 * @li @c ACount The number of question messages that follow the question
 *     messages.
 * @li @c Timer A count of seconds for which any answers should be considered
 *     valid.  A zero in this field means that the sending daemon is
 *     withdrawing the advertisements.  A value of 255 in this field means
 *     "forever," or at least until withdrawn
 *
 * <b>Example</b>
 *
 * It is expected that when an AllJoyn daemon comes up, it will want to advertise
 * the fact that it is up and supports some number of bus names.  It may also
 * be the case that the AllJoyn daemon already knows that it will need to find
 * some remote bus names when it comes up.  In this case, it can send an initial
 * message that both announces its bus names, and asks for any other AllJoyn
 * daemons that support what it wants.
 *
 * Consider a daemon that is capable of dealing with IPv4 addresses, and is
 * listening on Port 9955 of IPv4 address "192.168.10.10" for incoming
 * connections.  If the daemon is interested in locating another daemons that
 * supports the bus name "org.yadda.foo" and wants to export the fact that it
 * supports the bus name, "org.yadda.bar", and will support than name forever,
 * it might send a packet that combines WHO-HAS and IS-AT messages and that
 * looks something like
 *
 * @verbatim
 *      0                   1                   2                   3
 *      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     | Version = 0   |  Q Count = 1  |  A Count = 1  | Timer = 255   |   (A)
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |   WHO-HAS     |   Count = 1   |   Count = 13  |     'o'       |   (B) (C)
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+               |
 *     |     'r'             'g'             '.'             'y'       |
 *     |                                                               |
 *     |     'a'             'd'             'd'             'a'       |
 *     |                                                               |
 *     |     '.'             'f'             'o'             'o'       |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |     IS-AT     |  Count = 1    |     192       |      168      |  (D)
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |      10              10       |    9955       |  Count = 13   |  (E)
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |      'o'            'r'             'g'             '.'       |
 *     |                                                               |
 *     |      'y'            'a'             'd'             'd'       |
 *     |                                                               |
 *     |      'a'            '.'             'b'             'a'       |
 *     |               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |      'r'      |
 *     +-+-+-+-+-+-+-+-+
 * @endverbatim
 *
 * The notation (A) indicates the name service header.  This header tells
 * us that the version is one, there is one question and one answer
 * message following, and that the timeout value of any answer messages
 * present in this message is set to be 255 (infinite).
 *
 * The (B) section shows one question, which is a WHO-HAS message.  The
 * flags present in the WHO-HAS message are not shown here. What follows
 * is the count of bus names present in this message (1).  The bus name
 * shown here is "org.yadda.foo" (C).  This name is contained in a
 * serialized SDATA (string data) message.  The length of the string is
 * thirteen bytes, and the characters of the string follow.  This ends
 * the question section of the messge since there was only one question
 * present as indicated by the header.
 *
 * Next, the (D) notation shows the single answer message described in
 * the header.  Answer messages are called IS-AT messages.  There is a
 * count of one bus name in the IS-AT messsage which, the message is
 * telling us, can be found at IPv4 address 192.168.10.10 port 9955.
 * The single SDATA record (E) with a count of 13 indicates that the
 * sending daemon supports the bus name "org.yadda.bar" at that address
 * and port.
 * @} End of "defgroup name_service_protocol"
 */

/**
 * @internal
 * @brief An abstract data type defining the operations that each element
 * of a name service protocol must implement.
 *
 * Every instance of a piece of the name service protocol must
 * have the capability of being serialized into a datagram and deserialized
 * from a datagram.  It is also useful to be able to query an existing
 * object for how much buffer space it and its children will need in order
 * to be successfully serialized.
 *
 * This ADT class "enforces" the signatures of these functions.
 *
 * @ingroup name_service_protocol
 */
class ProtocolElement {
  public:
    /**
     * Virtual destructor for derivable class.
     */
    virtual ~ProtocolElement() { }

    /**
     * @internal
     * @brief Get the size of a buffer that will allow the object and all of
     * its children to be successfully serialized
     *
     * @return The size of the buffer required to serialize the object
     */
    virtual size_t GetSerializedSize(void) const = 0;

    /**
     * @internal
     * @brief Serialize this object and all of its children to the provided
     * buffer.
     *
     * @warning The buffer should be at least as large as the size returned
     * by GetSerializedSize().
     *
     * @return The number of octets written to the buffer.
     */
    virtual size_t Serialize(uint8_t* buffer) const = 0;

    /**
     * @internal
     * @brief Deserialize this object and all of its children from the provided
     * buffer.
     *
     * An object implementing this method will attempt to read its wire
     * representation and construct an object representation.  If there are
     * insuffucient bits in the buffer to do so, it will return a count of
     * zero bytes read.  There is very little redundancy in a wire representation
     * to provide information for error checking, so if the UDP checksum of
     * the containing datagram indicates a successfully received message, a
     * protocol error will typically be reported through a zero return count
     * here.
     *
     * @param buffer The buffer to read the bytes from.
     * @param bufsize The number of bytes available in the buffer.
     *
     * @return The number of octets read from the buffer, or zero if an error
     * occurred.
     */
    virtual size_t Deserialize(uint8_t const* buffer, uint32_t bufsize) = 0;
};

/**
 * @internal
 * @brief A class representing a name service StringData object.
 *
 * Since well-known names are strings, one of the fundamental objects in
 * the protocol is the StringData object.  Strings are encoded an octet
 * giving the length of the string, followed by some number UTF-8 characters.
 * (no terminating zero is required)  For example, the string "STRING"
 * would be encoded as follows.  The single octet length means that the
 * longest string possible is 255 characters.  This should not prove to
 * be a problem since it is the same maximum length as a domain name, on
 * which bus names are modeled.
 *
 * @ingroup name_service_protocol
 */
class StringData : public ProtocolElement {
  public:
    /**
     * @internal
     * @brief Construct a StringData object.  The size of the string represented
     * by the object is initialized to 0 -- the null string.
     */
    StringData();

    /**
     * @internal
     * @brief Destroy a StringData object.
     */
    ~StringData();

    /**
     * @internal
     * @brief Set the string represented by this StringData object to the
     * provided string.
     *
     * @param string The well-known or bus name to be represented by this object.
     */
    void Set(qcc::String string);

    /**
     * @internal
     * @brief Get the string represented by this StringData object.
     *
     * @return The well-known or bus name to represented by this object.
     */
    qcc::String Get(void) const;

    /**
     * @internal
     * @brief Get the size of a buffer that will allow the object to be
     * successfully serialized
     *
     * @return The size of the buffer required to serialize the object
     */
    size_t GetSerializedSize(void) const;

    /**
     * @internal
     * @brief Serialize this string to the provided buffer.
     *
     * @warning The buffer should be at least as large as the size returned
     * by GetSerializedSize().
     *
     * @return The number of octets written to the buffer.
     */
    size_t Serialize(uint8_t* buffer) const;

    /**
     * @internal
     * @brief Deserialize this string from the provided buffer.
     *
     * @see ProtocolElement::Deserialize()
     *
     * @param buffer The buffer to read the bytes from.
     * @param bufsize The number of bytes available in the buffer.
     *
     * @return The number of octets read from the buffer, or zero if an error
     * occurred.
     */
    size_t Deserialize(uint8_t const* buffer, uint32_t bufsize);

  private:
    /**
     * @internal
     * @brief The in-memory representation of the StringData object.
     */
    qcc::String m_string;

    /**
     * @internal
     * @brief The size of the represented StringData object.
     */
    size_t m_size;
};

/**
 * @internal
 * @brief A class representing an authoritative answer in the name service
 * protocol.
 *
 * The IS-AT message is an answer message used to advertise the existence
 * of a number of bus names on a given AllJoyn daemon.  IS-AT messages can
 * be sent as part of a response to a direct question, or they can be sent
 * gratuitously when an AllJoyn daemon decides to export the fact that it
 * supports some number of bus names.
 *
 * @ingroup name_service_protocol
 */
class IsAt : public ProtocolElement {
  public:
    /**
     * @internal
     * @brief Construct an in-memory object representation of an on-the-wire
     * name service protocol answer.
     */
    IsAt();

    /**
     * @internal
     * @brief Destroy a name service protocol answer object.
     */
    ~IsAt();

    /**
     * @internal
     * @brief Set the wire protocol version that the object will use.
     *
     * The name service protocol is versioned.  For backward compatibility, we
     * need to be able to get down-version messages to "old" daemons and current
     * version messages to "new" daemons.  The first thing to observe is that,
     * like most protocols, the wire protocol is versioned, not extensible.
     * This means that in order to support multiple versions, we will need to
     * send multiple messages.  This introduces ambiguitiy in the receiver of
     * the messages since it cannot know whether or not "new" messages will
     * appear after 'old" ones.  To resolve this ambiguity, the wire protocol
     * version is constructed of two versions:  a message version and a sender
     * protocol version.
     *
     * If a new version of the protocol is defined, new senders will be modified
     * to generate the new version, and new receivers will be able to receive
     * new messages.  The message version tells a receiver what the absolute
     * format of the current message is; and the sender protocol version
     * provides a context shared between the sender and the receiver that can
     * indicate more, or better, information may be forthcoming.
     *
     * The message version tells "this" object which version of the wire
     * protocol to use when serializing the bits and which version to expect
     * when deserializing the bits.  The sender protocol version is meta-data
     * for higher level code.
     *
     * The version is found in the Header, so we will know for sure what version
     * the overall packet will take.  Objects of class IsAt support all versions
     * known at the time of compilation.  If the version of the wire protocol
     * read does not provide a value for a given field, a default value will be
     * provided.  Objects of class IsAt can support writing down-version packets
     * for compatibility.
     *
     * It is up to higher levels to understand the contextual implications, if any,
     * of writing or reading down-version packets.
     *
     * @param nsVersion The version  (0 .. 16) of the sender's latest implemented protocol.
     * @param msgVersion The version  (0 .. 16) of the actual message.
     */
    void SetVersion(uint32_t nsVersion, uint32_t msgVersion) { m_version = nsVersion << 4 | msgVersion; }

    /**
     * @internal
     * @brief Get the wire protocol version that the object will use.
     *
     * @param nsVersion Gets the version  (0 .. 16) of the sender's latest implemented protocol.
     * @param msgVersion Gets the version  (0 .. 16) of the actual message.
     *
     * @see SetVersion()
     */
    void GetVersion(uint32_t& nsVersion, uint32_t& msgVersion) { nsVersion = m_version >> 4; msgVersion = m_version & 0xf; }

    /**
     * @internal
     * @brief Set the transport mask of the transport sending the advertisement
     *
     * The IP name service protocol can be used by a number of different AllJoyn
     * transports.  These transports may need to build a wall around their own
     * advertisements so we provide the name service with a way to plumb the
     * advertisements from end-to-end according to transport.
     *
     * It is up to higher levels to understand the contextual implications, if any,
     * of peeking at other transports' advertisements or discovery requests.
     *
     * @param mask The transport mask of the object.
     */
    void SetTransportMask(TransportMask mask) { m_transportMask = mask; }

    /**
     * @internal
     * @brief Get the transport mask of the transport that is sending the message.
     *
     * @return The transport mask of the transport that is sending the message.
     *
     * @see SetVersion()
     */
    TransportMask GetTransportMask(void) { return m_transportMask; }

    /**
     * @internal
     * @brief Set the protocol flag indicating that the daemon generating
     * this answer is providing its entire well-known name list.
     *
     * @param flag True if the daemon is providing the entire well-known name
     * list.
     */
    void SetCompleteFlag(bool flag) { m_flagC = flag; }

    /**
     * @internal
     * @brief Set the protocol flag indicating that the daemon generating
     * this answer is providing its entire well-known name list.
     *
     * @return True if the daemon is providing the entire well-known name
     * list.
     */
    bool GetCompleteFlag(void) const { return m_flagC; }

    /**
     * @internal
     * @brief Set the protocol flag indicating that the daemon generating
     * this answer is listening on a TCP socket.
     *
     * @param flag True if the daemon is listening on TCP.
     *
     * @warning Useful for version zero objects only.
     */
    void SetTcpFlag(bool flag) { m_flagT = flag; }

    /**
     * @internal
     * @brief Get the protocol flag indicating that the daemon generating
     * this answer is listening on a TCP socket.
     *
     * @return True if the daemon is listening on TCP.
     *
     * @warning Useful for version zero objects only.
     */
    bool GetTcpFlag(void) const { return m_flagT; }

    /**
     * @internal
     * @brief Set the protocol flag indicating that the daemon generating
     * this answer is listening on a UDP socket.
     *
     * @param flag True if the daemon is listening on UDP.
     *
     * @warning Useful for version zero objects only.
     */
    void SetUdpFlag(bool flag) { m_flagU = flag; }

    /**
     * @internal
     * @brief Get the protocol flag indicating that the daemon generating
     * this answer is listening on a UDP socket.
     *
     * @return True if the daemon is listening on UDP.
     *
     * @warning Useful for version zero objects only.
     */
    bool GetUdpFlag(void) const { return m_flagU; }

    /**
     * @internal
     * @brief Get the protocol flag indicating that the daemon generating
     * this answer is listening on a reliable IPv4 endpoint and has provided
     * a reliable IPv4 IP address and port.
     *
     * @return True if the daemon is listening on a reliable IPv4 endpoint.
     *
     * @warning Useful for version one objects only.
     */
    bool GetReliableIPv4Flag(void) const { return m_flagR4; }

    /**
     * @internal
     * @brief Get the protocol flag indicating that the daemon generating
     * this answer is listening on an unreliable IPv4 endpoint and has provided
     * an unreliable IPv4 IP address and port.
     *
     * @return True if the daemon is listening on an unreliable IPv4 endpoint.
     *
     * @warning Useful for version one objects only.
     */
    bool GetUnreliableIPv4Flag(void) const { return m_flagU4; }

    /**
     * @internal
     * @brief Get the protocol flag indicating that the daemon generating
     * this answer is listening on a reliable IPv6 endpoint and has provided
     * a reliable IPv6 IP address and port.
     *
     * @return True if the daemon is listening on a reliable IPv6 endpoint.
     *
     * @warning Useful for version one objects only.
     */
    bool GetReliableIPv6Flag(void) const { return m_flagR6; }

    /**
     * @internal
     * @brief Get the protocol flag indicating that the daemon generating
     * this answer is listening on an unreliable IPv6 endpoint and has provided
     * an unreliable IPv6 IP address and port.
     *
     * @return True if the daemon is listening on an unreliable IPv6 endpoint.
     *
     * @warning Useful for version one objects only.
     */
    bool GetUnreliableIPv6Flag(void) const { return m_flagU6; }

    /**
     * @internal
     * @brief Get the protocol flag indicating that the daemon generating
     * this answer has provided a GUID string.
     *
     * This flag is set to true by the SetGuid() call (or deserializing
     * a message for which the flag was set) and indicates that a daemon
     * GUID is provided in the message.
     *
     * @return True if the daemon has provided a GUID.
     */
    bool GetGuidFlag(void) const { return m_flagG; }

    /**
     * @internal
     * @brief Get the protocol flag indicating that the daemon generating
     * this answer is listening on an IPv6 address.
     *
     * This flag is set to true by the SetIPv6() call (or deserializing
     * a message for which the flag was set) and indicates that an IPv6
     * address is provided in the message.
     *
     * @return True if the daemon is listening on IPv6.
     *
     * @warning Useful for version zero objects only.
     */
    bool GetIPv6Flag(void) const { return m_flagS; }

    /**
     * @internal
     * @brief Get the protocol flag indicating that the daemon generating
     * this answer is listening on an IPv4 address.
     *
     * This flag is set to true by the SetIPv4() call (or deserializing
     * a message for which the flag was set) and indicates that an IPv4
     * address is provided in the message.
     *
     * @return True if the daemon is listening on IPv4.
     *
     * @warning Useful for version zero objects only.
     */
    bool GetIPv4Flag(void) const { return m_flagF; }

    /**
     * @internal
     * @brief Set the GUID string for the responding name service.
     *
     * This method takes a string with the name service guid in "presentation" format
     * and  arranges for it to be written out in the protocol message.  Although
     * this is typically a string representation of a GUID, the string is not
     * interpreted by the protocol and can be used to carry any global (not
     * related to an individual well-known name) information related to the
     * generating daemon.
     *
     * @param guid The name service GUID string of the responding name service.
     */
    void SetGuid(const qcc::String& guid);

    /**
     * @internal
     * @brief Get the name service GUID string for the responding daemon.
     *
     * This method returns a string with the guid in "presentation" format.
     * Although this is typically a string representation of a GUID, the string
     * is not interpreted by the protocol and can be used to carry any global
     * (not related to an infividual well-known name) information related to the
     * generating daemon.
     *
     * @return The GUID string of the responding name service.
     */
    qcc::String GetGuid(void) const;

    /**
     * @internal
     * @brief Set the port on which the daemon generating this answer is
     * listening.
     *
     * @param port The port on which the daemon is listening.
     *
     * @warning Useful for version zero objects only.
     */
    void SetPort(uint16_t port);

    /**
     * @internal
     * @brief Get the port on which the daemon generating this answer is
     * listening.
     *
     * @return The port on which the daemon is listening.
     *
     * @warning Useful for version zero objects only.
     */
    uint16_t GetPort(void) const;

    /**
     * @internal
     * @brief Clear the IPv4 address.
     *
     * @warning Useful for version zero objects only.
     */
    void ClearIPv4(void);

    /**
     * @internal
     * @brief Set the IPv4 address on which the daemon generating this answer
     * is listening.
     *
     * This method takes an IPv4 address string in presentation format and
     * arranges for it to be written out in the protocol message in network
     * format (32-bits, big endian).  It also has the side-effect of setting
     * the IPv4 flag.
     *
     * @param ipv4Addr The IPv4 address on which the daemon is listening.
     *
     * @warning Useful for version zero objects only.
     */
    void SetIPv4(qcc::String ipv4Addr);

    /**
     * @internal
     * @brief Get the IPv4 address on which the daemon generating this answer
     * is listening.
     *
     * This method returns an IPv4 address string in presentation format.
     * If the IPv4 flag is not set, the results are undefined.
     *
     * @return The IPv4 address on which the daemon is listening.
     *
     * @warning Useful for version zero objects only.
     */
    qcc::String GetIPv4(void) const;

    /**
     * @internal
     * @brief Clear the IPv6 address.
     * @warning Useful for version zero objects only.
     */
    void ClearIPv6(void);

    /**
     * @internal
     * @brief Set the IPv6 address on which the daemon generating this answer
     * is listening.
     *
     * This method takes an IPv6 address string in presentation format and
     * arranges for it to be written out in the protocol message in network
     * format (128-bits, big endian).  It also has the side-effect of setting
     * the IPv6 flag.
     *
     * @param ipv6Addr The IPv6 address on which the daemon is listening.
     *
     * @warning Useful for version zero objects only.
     */
    void SetIPv6(qcc::String ipv6Addr);

    /**
     * @internal
     * @brief Get the IPv6 address on which the daemon generating this answer
     * is listening.
     *
     * This method returns an IPv6 address string in presentation format.
     * If the IPv6 flag is not set, the results are undefined.
     *
     * @return The IPv6 address on which the daemon is listening.
     *
     * @warning Useful for version zero objects only.
     */
    qcc::String GetIPv6(void) const;

    /**
     * @internal
     * @brief Clear the ReliableIPv4 address and port
     *
     * @warning Useful for version one objects only.
     */
    void ClearReliableIPv4(void);

    /**
     * @internal
     * @brief Set the reliable IPv4 address and port
     *
     * This method takes an IPv4 address string in presentation format and
     * arranges for it to be written out in the protocol message in network
     * format (32-bits, big endian).  It also takes a port number and arranges
     * for it to be written out in network format (16-bits, big endian).  This
     * method has the side-effect of setting the reliableIPv4 flag.
     *
     * @param addr The reliable method IPv4 address on which the daemon can be
     *     contacted.
     * @param port The port on which the daemon is listening for connections.
     *
     * @warning Useful for version one objects only.
     */
    void SetReliableIPv4(qcc::String addr, uint16_t port);

    /**
     * @internal
     * @brief Get the reliable IPv4 address
     *
     * This method returns an IPv4 address string in presentation format.
     * If the reliableIPv4 flag is not set, the results are undefined.
     *
     * @return The reliable IPv4 address on which the daemon is listening.
     *
     * @warning Useful for version one objects only.
     */
    qcc::String GetReliableIPv4Address(void) const;

    /**
     * @internal
     * @brief Get the reliable IPv4 port
     *
     * This method returns an IPv4 port.  If the reliableIPv4 flag is not set,
     * the results are undefined.
     *
     * @return The reliable IPv4 port on which the daemon is listening.
     *
     * @warning Useful for version one objects only.
     */
    uint16_t GetReliableIPv4Port(void) const;

    /**
     * @internal
     * @brief Clear the UnreliableIPv4 address and port
     *
     * @warning Useful for version one objects only.
     */
    void ClearUnreliableIPv4(void);

    /**
     * @internal
     * @brief Set the unreliable IPv4 address and port
     *
     * This method takes an IPv4 address string in presentation format and
     * arranges for it to be written out in the protocol message in network
     * format (32-bits, big endian).  It also takes a port number and arranges
     * for it to be written out in network format (16-bits, big endian).  This
     * method has the side-effect of setting the unreliableIPv4 flag.
     *
     * @param addr The unreliable method IPv4 address on which the daemon can be
     *     contacted.
     * @param port The port on which the daemon is listening for connections.
     *
     * @warning Useful for version one objects only.
     */
    void SetUnreliableIPv4(qcc::String addr, uint16_t port);

    /**
     * @internal
     * @brief Get the unreliable IPv4 address
     *
     * This method returns an IPv4 address string in presentation format.
     * If the unreliableIPv4 flag is not set, the results are undefined.
     *
     * @return The unreliable IPv4 address on which the daemon is listening.
     *
     * @warning Useful for version one objects only.
     */
    qcc::String GetUnreliableIPv4Address(void) const;

    /**
     * @internal
     * @brief Get the unreliable IPv4 port
     *
     * This method returns an IPv4 port.  If the unreliableIPv4 flag is not set,
     * the results are undefined.
     *
     * @return The unreliable IPv4 port on which the daemon is listening.
     *
     * @warning Useful for version one objects only.
     */
    uint16_t GetUnreliableIPv4Port(void) const;

    /**
     * @internal
     * @brief Clear the ReliableIPv6 address and port
     *
     * @warning Useful for version one objects only.
     */
    void ClearReliableIPv6(void);

    /**
     * @internal
     * @brief Set the reliable IPv6 address and port
     *
     * This method takes an IPv6 address string in presentation format and
     * arranges for it to be written out in the protocol message in network
     * format (128-bits, big endian).  It also takes a port number and arranges
     * for it to be written out in network format (16-bits, big endian).  This
     * method has the side-effect of setting the reliableIPv6 flag.
     *
     * @param addr The reliable method IPv6 address on which the daemon can be
     *     contacted.
     * @param port The port on which the daemon is listening for connections.
     *
     * @warning Useful for version one objects only.
     */
    void SetReliableIPv6(qcc::String addr, uint16_t port);

    /**
     * @internal
     * @brief Get the reliable IPv6 address
     *
     * This method returns an IPv6 address string in presentation format.
     * If the reliableIPv6 flag is not set, the results are undefined.
     *
     * @return The reliable IPv6 address on which the daemon is listening.
     *
     * @warning Useful for version one objects only.
     */
    qcc::String GetReliableIPv6Address(void) const;

    /**
     * @internal
     * @brief Get the reliable IPv6 port
     *
     * This method returns an IPv6 port.  If the reliableIPv6 flag is not set,
     * the results are undefined.
     *
     * @return The reliable IPv6 port on which the daemon is listening.
     *
     * @warning Useful for version one objects only.
     */
    uint16_t GetReliableIPv6Port(void) const;

    /**
     * @internal
     * @brief Clear the UnreliableIPv6 address and port
     *
     * @warning Useful for version one objects only.
     */
    void ClearUnreliableIPv6(void);

    /**
     * @internal
     * @brief Set the unreliable IPv6 address and port
     *
     * This method takes an IPv6 address string in presentation format and
     * arranges for it to be written out in the protocol message in network
     * format (128-bits, big endian).  It also takes a port number and arranges
     * for it to be written out in network format (16-bits, big endian).  This
     * method has the side-effect of setting the unreliableIPv6 flag.
     *
     * @param addr The unreliable method IPv6 address on which the daemon can be
     *     contacted.
     * @param port The port on which the daemon is listening for connections.
     *
     * @warning Useful for version one objects only.
     */
    void SetUnreliableIPv6(qcc::String addr, uint16_t port);

    /**
     * @internal
     * @brief Get the unreliable IPv6 address
     *
     * This method returns an IPv6 address string in presentation format.
     * If the unreliableIPv6 flag is not set, the results are undefined.
     *
     * @return The unreliable IPv6 address on which the daemon is listening.
     *
     * @warning Useful for version one objects only.
     */
    qcc::String GetUnreliableIPv6Address(void) const;

    /**
     * @internal
     * @brief Get the unreliable IPv6 port
     *
     * This method returns an IPv6 port.  If the unreliableIPv6 flag is not set,
     * the results are undefined.
     *
     * @return The unreliable IPv6 port on which the daemon is listening.
     *
     * @warning Useful for version one objects only.
     */
    uint16_t GetUnreliableIPv6Port(void) const;

    /**
     * @internal @brief Clear any objects from the list names, effectively
     * resetting the object.
     */
    void Reset(void);

    /**
     * @internal
     * @brief Add a string representing a well-known or bus name to the answer.
     *
     * This method adds a well-known or bus name and adds it to an internal
     * list of authoritative answers regarding the names supported by the
     * calling daemon.  These names will be serialized to an answer message
     *  as StringData objects.
     *
     * @param name A well-known or bus name which the daemon supports.
     */
    void AddName(qcc::String name);

    /**
     * @internal
     * Remove the well-known or bus name from the answer.
     *
     * This method removed a well-known name or bus name from the list of answers
     * regarding the names supported by the calling daemon.
     *
     * @note complexity constant time plus the number of element after the last
     * element deleted
     *
     * @param index the index value for the name to be removed
     */
    void RemoveName(uint32_t index);

    /**
     * @internal
     * @brief Get the number of well-known or bus names represented by this
     * object.
     *
     * This method returns the number of well-known bus names from the internal
     * list of authoritative answers regarding the names supported by the
     * responding daemon.  These names are typically deserialized from an
     * answer received over the network.
     *
     * @see GetName()
     *
     * @return The number of well-known names represented by this answer
     * object.
     */
    uint32_t GetNumberNames(void) const;

    /**
     * @internal
     * @brief Get a string representing a well-known or bus name.
     *
     * This method returns one of the well-known or bus names from the internal
     * list of authoritative answers regarding the names supported by the
     * responding daemon.  These names are typically deserialized from an answer
     *  received over the network.
     *
     * The number of entries in the list (used to determine legal values for the
     * index) is found by calling GetNumberNames()
     *
     * @see GetNumberNames()
     *
     * @param name The index of the name to retrieve.
     *
     * @return The well-known or bus name at the provided index.
     */
    qcc::String GetName(uint32_t index) const;

    /**
     * @internal
     * @brief Get the size of a buffer that will allow the answer object and
     * all of its children StringData objects to be successfully serialized.
     *
     * @return The size of the buffer required to serialize the object
     */
    size_t GetSerializedSize(void) const;

    /**
     * @internal
     * @brief Serialize this answer and all of its children StringData objects
     * to the provided buffer.
     *
     * @warning The buffer should be at least as large as the size returned
     * by GetSerializedSize().
     *
     * @return The number of octets written to the buffer.
     */
    size_t Serialize(uint8_t* buffer) const;

    /**
     * @internal
     * @brief Deserialize this answer from the provided buffer.
     *
     * @see ProtocolElement::Deserialize()
     *
     * @param buffer The buffer to read the bytes from.
     * @param bufsize The number of bytes available in the buffer.
     *
     * @return The number of octets read from the buffer, or zero if an error
     * occurred.
     */
    size_t Deserialize(uint8_t const* buffer, uint32_t bufsize);

  private:
    uint8_t m_version;

    TransportMask m_transportMask; /**< Version one only */

    bool m_flagG;
    bool m_flagC;

    bool m_flagT; /**< Version zero only */
    bool m_flagU; /**< Version zero only */
    bool m_flagS; /**< Version zero only */
    bool m_flagF; /**< Version zero only */

    bool m_flagR4; /**< Version one only */
    bool m_flagU4; /**< Version one only */
    bool m_flagR6; /**< Version one only */
    bool m_flagU6; /**< Version one only */

    uint16_t m_port;     /**< Version zero only */
    qcc::String m_ipv4;  /**< Version zero only */
    qcc::String m_ipv6;  /**< Version zero only */

    qcc::String m_reliableIPv4Address;    /**< Version one only */
    uint16_t m_reliableIPv4Port;          /**< Version one only */
    qcc::String m_unreliableIPv4Address;  /**< Version one only */
    uint16_t m_unreliableIPv4Port;        /**< Version one only */

    qcc::String m_reliableIPv6Address;    /**< Version one only */
    uint16_t m_reliableIPv6Port;          /**< Version one only */
    qcc::String m_unreliableIPv6Address;  /**< Version one only */
    uint16_t m_unreliableIPv6Port;        /**< Version one only */

    qcc::String m_guid;
    std::vector<qcc::String> m_names;
};

/**
 * @internal
 * @brief A class representing a question in the name service protocol.
 *
 * The WHO-HAS message is a question message used to ask AllJoyn daemons if they
 * support one or more bus names.
 *
 * @ingroup name_service_protocol
 */
class WhoHas : public ProtocolElement {
  public:

    /**
     * @internal
     * @brief Construct an in-memory object representation of an on-the-wire
     * name service protocol question.
     */
    WhoHas();

    /**
     * @internal
     * @brief Destroy a name service protocol answer object.
     */
    ~WhoHas();

    /**
     * @internal
     * @brief Set the wire protocol version that the object will use.
     *
     * The name service protocol is versioned.  For backward compatibility, we
     * need to be able to get down-version messages to "old" daemons and current
     * version messages to "new" daemons.  The first thing to observe is that,
     * like most protocols, the wire protocol is versioned, not extensible.
     * This means that in order to support multiple versions, we will need to
     * send multiple messages.  This introduces ambiguitiy in the receiver of
     * the messages since it cannot know whether or not "new" messages will
     * appear after 'old" ones.  To resolve this ambiguity, the wire protocol
     * version is constructed of two versions:  a message version and a sender
     * protocol version.
     *
     * If a new version of the protocol is defined, new senders will be modified
     * to generate the new version, and new receivers will be able to receive
     * new messages.  The message version tells a receiver what the absolute
     * format of the current message is; and the sender protocol version
     * provides a context shared between the sender and the receiver that can
     * indicate more, or better, information may be forthcoming.
     *
     * The message version tells "this" object which version of the wire
     * protocol to use when serializing the bits and which version to expect
     * when deserializing the bits.  The sender protocol version is meta-data
     * for higher level code.
     *
     * The version is found in the Header, so we will know for sure what version
     * the overall packet will take.  Objects of class IsAt support all versions
     * known at the time of compilation.  If the version of the wire protocol
     * read does not provide a value for a given field, a default value will be
     * provided.  Objects of class IsAt can support writing down-version packets
     * for compatibility.
     *
     * It is up to higher levels to understand the contextual implications, if any,
     * of writing or reading down-version packets.
     *
     * @param nsVersion The version  (0 .. 16) of the sender's latest implemented protocol.
     * @param msgVersion The version  (0 .. 16) of the actual message.
     */
    void SetVersion(uint32_t nsVersion, uint32_t msgVersion) { m_version = nsVersion << 4 | msgVersion; }

    /**
     * @internal
     * @brief Get the wire protocol version that the object will use.
     *
     * @param nsVersion Gets the version  (0 .. 16) of the sender's latest implemented protocol.
     * @param msgVersion Gets the version  (0 .. 16) of the actual message.
     *
     * @see SetVersion()
     */
    void GetVersion(uint32_t& nsVersion, uint32_t& msgVersion) { nsVersion = m_version >> 4; msgVersion = m_version & 0xf; }

    /**
     * @internal
     * @brief Set the transport mask of the transport sending the advertisement
     *
     * The IP name service protocol can be used by a number of different AllJoyn
     * transports.  These transports may need to build a wall around their own
     * advertisements so we provide the name service with a way to plumb the
     * advertisements from end-to-end according to transport.
     *
     * It is up to higher levels to understand the contextual implications, if any,
     * of peeking at other transports' advertisements or discovery requests.
     *
     * @param mask The transport mask of the object.
     *
     * @warning Due to an oversight, the transport mask is not actually sent in
     * a version one who-has message.  The transport mask is carried around internally
     * in outgoing messages, but is set to zero for incoming messages.
     */
    void SetTransportMask(TransportMask mask) { m_transportMask = mask; }

    /**
     * @internal
     * @brief Get the transport mask of the transport that is sending the message.
     *
     * @return The transport mask of the transport that is sending the message.
     *
     * @see SetTransportMask()
     *
     * @warning Due to an oversight, the transport mask is not actually sent in
     * a version one who-has message.  The transport mask is carried around internally
     * in outgoing messages, but is set to zero for incoming messages.
     */
    TransportMask GetTransportMask(void) { return m_transportMask; }

    /**
     * @internal
     * @brief Set the protocol flag indicating that the daemon generating
     * this question is interested in hearing about daemons listening on a TCP
     * socket.
     *
     * @param flag True if the daemon is interested in TCP.
     */
    void SetTcpFlag(bool flag) { m_flagT = flag; }

    /**
     * @internal
     * @brief Get the protocol flag indicating that the daemon generating
     * this question is interested in hearing about daemons listening on a TCP
     * socket.
     *
     * @return True if the daemon ways it is interested in TCP.
     */
    bool GetTcpFlag(void) const { return m_flagT; }

    /**
     * @internal
     * @brief Set the protocol flag indicating that the daemon generating
     * this question is interested in hearing about daemons listening on a UDP
     * socket.
     *
     * @param flag True if the daemon is interested in UDP.
     */
    void SetUdpFlag(bool flag) { m_flagU = flag; }

    /**
     * @internal
     * @brief Get the protocol flag indicating that the daemon generating
     * this question is interested in hearing about daemons listening on a UDP
     * socket.
     *
     * @return True if the daemon ways it is interested in UDP.
     */
    bool GetUdpFlag(void) const { return m_flagU; }

    /**
     * @internal
     * @brief Set the protocol flag indicating that the daemon generating
     * this question is interested in IPv6 addresses.
     *
     * @param flag True if the daemon is interested on IPv6.
     */
    void SetIPv6Flag(bool flag) { m_flagS = flag; }

    /**
     * @internal
     * @brief Get the protocol flag indicating that the daemon generating
     * this question is interested in IPv6 addresses.
     *
     * @return True if the daemon is interested on IPv6.
     */
    bool GetIPv6Flag(void) const { return m_flagS; }

    /**
     * @internal
     * @brief Set the protocol flag indicating that the daemon generating
     * this question is interested in IPv4 addresses.
     *
     * @param flag True if the daemon is interested on IPv4.
     */
    void SetIPv4Flag(bool flag) { m_flagF = flag; }

    /**
     * @internal
     * @brief Get the protocol flag indicating that the daemon generating
     * this question is interested in IPv4 addresses.
     *
     * @return True if the daemon is interested on IPv4.
     */
    bool GetIPv4Flag(void) const { return m_flagF; }

    /**
     * @internal @brief Clear any objects from the list names, effectively
     * resetting the object.
     */
    void Reset(void);

    /**
     * @internal
     * @brief Add a string representing a well-known or bus name to the
     * question.
     *
     * This method adds a well-known or bus name to an internal list of names
     * in which the calling daemon is interested.  These names will be
     * serialized to a question message as StringData objects.
     *
     * @param name The well-known or bus name which the daemon is interested.
     */
    void AddName(qcc::String name);

    /**
     * @internal
     * @brief Get the number of well-known or bus names represented by this
     * object.
     *
     * This method returns the number of well-known or bus names from the
     * internal list names that the questioning daemon is interested in.
     * These names are typically deserialized from a question received over the
     * network.
     *
     * @see GetName()
     *
     * @return The number of well-known interfaces represented by this question
     * object.
     */
    uint32_t GetNumberNames(void) const;

    /**
     * @internal
     * @brief Get a string representing a well-known or bus name.
     *
     * This method returns one of the well-known or bus names from the internal
     * list of names in which questioning daemon is interested.  These names
     * are typically deserialized from a question received over the network.
     *
     * The number of entries in the list (used to determine legal values for the
     * index) is found by calling GetNumberNames()
     *
     * @see GetNumberNames()
     *
     * @param name The index of the name to retrieve.
     *
     * @return The well-known or bus name at the provided index.
     */
    qcc::String GetName(uint32_t index) const;

    /**
     * @internal
     * @brief Get the size of a buffer that will allow the question object and
     * all of its children StringData objects to be successfully serialized.
     *
     * @return The size of the buffer required to serialize the object
     */
    size_t GetSerializedSize(void) const;

    /**
     * @internal
     * @brief Serialize this question and all of its children StringData objects
     * to the provided buffer.
     *
     * @warning The buffer should be at least as large as the size returned
     * by GetSerializedSize().
     *
     * @return The number of octets written to the buffer.
     */
    size_t Serialize(uint8_t* buffer) const;

    /**
     * @internal
     * @brief Deserialize a question wire-representation from the provided
     * buffer.
     *
     * @see ProtocolElement::Deserialize()
     *
     * @param buffer The buffer to read the bytes from.
     * @param bufsize The number of bytes available in the buffer.
     *
     * @return The number of octets read from the buffer, or zero if an error
     * occurred.
     */
    size_t Deserialize(uint8_t const* buffer, uint32_t bufsize);

  private:
    uint8_t m_version;
    TransportMask m_transportMask;
    bool m_flagT;
    bool m_flagU;
    bool m_flagS;
    bool m_flagF;
    std::vector<qcc::String> m_names;
};

/**
 * @internal
 * @brief A class representing a message in the name service protocol.
 *
 * A name service message consists of a header, followed by a variable
 * number of question (Q) messages (for example, WHO-HAS) followed by a variable
 * number of answer(A) messages (for example, IS-AT).  All messages are packed
 * to octet boundaries.
 *
 * @ingroup name_service_protocol
 */
class Header : public ProtocolElement {
  public:

    /**
     * @internal
     * @brief Construct an in-memory object representation of an on-the-wire
     * name service protocol header.
     */
    Header();

    /**
     * @internal
     * @brief Destroy a name service protocol header object.
     */
    ~Header();

    /**
     * @internal
     * @brief Set the optional destination address for the message corresponding
     * to this header.  This is not a perfect place for this information, but it
     * is a very convenient place.  This information is not part of the wire
     * protocol.
     *
     * @param destination The destination IP address of the corresponding message.
     */
    void SetDestination(qcc::IPEndpoint destination) { m_destination = destination; m_destinationSet = true; }

    /**
     * @internal
     * @brief Get the optional destination address for the message corresponding
     * to this header.  If the destination has not been set, the result is not
     * defined.  This is not a perfect place for this information, but it is a
     * very convenient place.  This information is not part of the wire
     * protocol.
     *
     * @return The destination IP address of the corresponding message.
     */
    qcc::IPEndpoint GetDestination(void) { return m_destination; }

    /**
     * @internal
     * @brief Set the optional destination address for the message corresponding
     * to this header.  This is not a perfect place for this information, but it
     * is a very convenient place.  This information is not part of the wire
     * protocol.
     *
     * @return The destination IP address of the corresponding message.
     */
    bool DestinationSet(void) { return m_destinationSet; }

    /**
     * @internal
     * @brief Set the optional destination address for the message corresponding
     * to this header.  This is not a perfect place for this information, but it
     * is a very convenient place.  This information is not part of the wire
     * protocol.
     */
    void ClearDestination() { m_destinationSet = false; }

    /**
     * @internal
     * @brief Set the number of times this header has been sent on the wire.
     * This is not a perfect place for this information, but it is a very
     * convenient place.  This information is not part of the wire protocol.
     *
     * @param retries The number of times the header has been sent on the wire.
     */
    void SetRetries(uint32_t retries) { m_retries = retries; }

    /**
     * @internal
     * @brief Set the number of times this header has been sent on the wire.
     * This is not a perfect place for this information, but it is a very
     * convenient place.  This information is not part of the wire protocol.
     *
     * @return The number of times the header has been sent on the wire.
     */
    uint32_t GetRetries(void) { return m_retries; }

    /**
     * @internal
     * @brief Get the tick value representing the last time this header was sent
     * on the wire.  This is not a perfect place for this information, but it
     * is a very convenient place.  This information is not part of the wire
     * protocol.
     *
     * @param tick The last time the header was sent on the wire.
     */
    void SetRetryTick(uint32_t tick) { m_tick = tick; }

    /**
     * @internal
     * @brief Set the tick value representing the last time this header was sent
     * on the wire.  This is not a perfect place for this information, but it
     * is a very convenient place.  This information is not part of the wire
     * protocol.
     *
     * @return The last time the header was been sent on the wire.
     */
    uint32_t GetRetryTick(void) { return m_tick; }

    /**
     * @internal
     * @brief Set the wire protocol version that the object will use.
     *
     * The name service protocol is versioned.  For backward compatibility, we
     * need to be able to get down-version messages to "old" daemons and current
     * version messages to "new" daemons.  The first thing to observe is that,
     * like most protocols, the wire protocol is versioned, not extensible.
     * This means that in order to support multiple versions, we will need to
     * send multiple messages.  This introduces ambiguitiy in the receiver of
     * the messages since it cannot know whether or not "new" messages will
     * appear after 'old" ones.  To resolve this ambiguity, the wire protocol
     * version is constructed of two versions:  a message version and a sender
     * protocol version.
     *
     * If a new version of the protocol is defined, new senders will be modified
     * to generate the new version, and new receivers will be able to receive
     * new messages.  The message version tells a receiver what the absolute
     * format of the current message is; and the sender protocol version
     * provides a context shared between the sender and the receiver that can
     * indicate more, or better, information may be forthcoming.
     *
     * The message version tells "this" object which version of the wire
     * protocol to use when serializing the bits and which version to expect
     * when deserializing the bits.  The sender protocol version is meta-data
     * for higher level code.
     *
     * The version is found in the Header, so we will know for sure what version
     * the overall packet will take.  Objects of class IsAt support all versions
     * known at the time of compilation.  If the version of the wire protocol
     * read does not provide a value for a given field, a default value will be
     * provided.  Objects of class IsAt can support writing down-version packets
     * for compatibility.
     *
     * It is up to higher levels to understand the contextual implications, if any,
     * of writing or reading down-version packets.
     *
     * @param nsVersion The version  (0 .. 16) of the sender's latest implemented protocol.
     * @param msgVersion The version  (0 .. 16) of the actual message.
     */
    void SetVersion(uint32_t nsVersion, uint32_t msgVersion) { m_version = nsVersion << 4 | msgVersion; }

    /**
     * @internal
     * @brief Get the wire protocol version that the object will use.
     *
     * @param nsVersion Gets the version  (0 .. 16) of the sender's latest implemented protocol.
     * @param msgVersion Gets the version  (0 .. 16) of the actual message.
     *
     * @see SetVersion()
     */
    void GetVersion(uint32_t& nsVersion, uint32_t& msgVersion) { nsVersion = m_version >> 4; msgVersion = m_version & 0xf; }

    /**
     * @internal
     * @brief Set the timer value for all answers present in the protocol
     * message.
     *
     * The timer value is typcally used to encode whether or not included
     * answer (IS-AT) messages indicate the establishment or withdrawal
     * of service advertisements.  A timer value of zero indicates that
     * the included answers are valid for zero seconds.  This implies
     * that the advertisements are no longer valid and should be withdrawn.
     *
     * A timer value of 255 indicates that the advertisements included in
     * the following IS-AT messages should be considered valid until they
     * are explicitly withdrawn.
     *
     * Other timer values indicate that the advertisements included are
     * are ephemeral and should not be considered valid for longer than
     * the number of seconds after which the message datagram containing
     * the header is received.
     *
     * @param timer The timer value (0 .. 255) for included answers.
     */
    void SetTimer(uint8_t timer);

    /**
     * @internal
     * @brief Get the timer value for all answers present in the protocol
     * message.
     *
     * @see SetTimer()
     *
     * @return  The timer value (0 .. 255) for included answers.
     */
    uint8_t GetTimer(void) const;

    /**
     * @internal
     * @brief Clear any objects from the lists of questions and answers,
     * effectively resetting the header.
     */
    void Reset(void);

    /**
     * @internal
     * @brief Add a question object to the list of questions represented by
     * this header.
     *
     * This method adds a question (WhoHas) object to an internal list of
     * questions the calling daemon is asking.  These questions will be
     * serialized to on-the-wire question objects when the header is
     * serialized.
     *
     * @see class WhoHas
     *
     * @param whoHas The question object to add to the protocol message.
     */
    void AddQuestion(WhoHas whoHas);

    /**
     * @internal
     * @brief Get the number of question objects represented by this object.
     *
     * @see GetQuestion()
     *
     * @return The number of question objects represented by this header object.
     */
    uint32_t GetNumberQuestions(void) const;

    /**
     * @internal
     * @brief Get a Question object represented by this header object.
     *
     * This method returns one of the question objects from the internal list
     * of questions asked by a questioning daemon.  These questions are
     * typically automatically deserialized from a header received over the
     * network.
     *
     * The number of entries in the list (used to determine legal values for the
     * index) is found by calling GetNumberQuestions()
     *
     * @see GetNumberQuestions()
     *
     * @param name The index of the question to retrieve.
     *
     * @return The question object at the provided index.
     */
    WhoHas GetQuestion(uint32_t index) const;

    /**
     * @internal
     * @brief Get a pointer to an answer object represented by this header object.
     *
     * This method returns a pointer.  This is typically used if one needs to
     * rewrite the contents of a question object.  We can't just return the pointer
     * since C++ doesn't consider return types in overload resolution, so we have
     * to return it indirectly.
     *
     * @see GetQuestion()
     *
     * @param name The index of the question to retrieve.
     * @param answer A pointer to the returned pointer to the question object at
     *     the provided index.
     */
    void GetQuestion(uint32_t index, WhoHas** question);

    /**
     * @internal
     * @brief Add an answer object to the list of answers represented by
     * this header.
     *
     * This method adds an answer (IsAt) object to an internal list of
     * answers the calling daemon is providing.  These answers will be
     * serialized to on-the-wire answer objects when the header is
     * serialized.
     *
     * @see class IsAt
     *
     * @param isAt The answer object to add to the protocol message.
     */
    void AddAnswer(IsAt isAt);

    /**
     * @internal
     * Remove an answer object from the list of answers represented by this header.
     *
     * This method removed an answer (IsAt) object from an internal list of answers
     * the calling daemon is providing.
     *
     * @note complexity is constant time plus the number of elements after the
     * deleted element.
     *
     * @param index the index value of the answer to be removed
     */
    void RemoveAnswer(uint32_t index);

    /**
     * @internal
     * @brief Get the number of answer objects represented by this object.
     *
     * @see GetAnswer()
     *
     * @return The number of answer objects represented by this header object.
     */
    uint32_t GetNumberAnswers(void) const;

    /**
     * @internal
     * @brief Get an answer object represented by this header object.
     *
     * This method returns one of the answer objects from the internal list
     * of answers provided by a responding daemon.  These answers are typically
     * automatically deserialized from a header received over the network.
     *
     * The number of entries in the list (used to determine legal values for the
     * index) is found by calling GetNumberAnswers()
     *
     * @see GetNumberAnswers()
     *
     * @param name The index of the answer to retrieve.
     *
     * @return The answer object at the provided index.
     */
    IsAt GetAnswer(uint32_t index) const;

    /**
     * @internal
     * @brief Get a pointer to an answer object represented by this header object.
     *
     * This method returns a pointer.  This is typically used if one needs to
     * rewrite the contents of an answer object.  We can't just return the pointer
     * since C++ doesn't consider return types in overload resolution, so we have
     * to return it indirectly.
     *
     * @see GetAnswer()
     *
     * @param name The index of the answer to retrieve.
     * @param answer A pointer to the returned pointer to the answer object at
     *     the provided index.
     */
    void GetAnswer(uint32_t index, IsAt** answer);

    /**
     * @internal
     * @brief Get the size of a buffer that will allow the header object and
     * all of its children questions and answer objects to be successfully
     * serialized.
     *
     * @return The size of the buffer required to serialize the object
     */
    size_t GetSerializedSize(void) const;

    /**
     * @internal
     * @brief Serialize this header and all of its children question and
     * answer objects to the provided buffer.
     *
     * @warning The buffer should be at least as large as the size returned
     * by GetSerializedSize().
     *
     * @return The number of octets written to the buffer.
     */
    size_t Serialize(uint8_t* buffer) const;

    /**
     * @internal
     * @brief Deserialize a header wire-representation and all of its children
     * questinos and answers from the provided buffer.
     *
     * @see ProtocolElement::Deserialize()
     *
     * @param buffer The buffer to read the bytes from.
     * @param bufsize The number of bytes available in the buffer.
     *
     * @return The number of octets read from the buffer, or zero if an error
     * occurred.
     */
    size_t Deserialize(uint8_t const* buffer, uint32_t bufsize);

  private:
    uint8_t m_version;
    uint8_t m_timer;
    qcc::IPEndpoint m_destination;
    bool m_destinationSet;
    uint32_t m_retries;
    uint32_t m_tick;
    std::vector<WhoHas> m_questions;
    std::vector<IsAt> m_answers;
};

} // namespace ajn

#endif // _NS_PROTOCOL_H
