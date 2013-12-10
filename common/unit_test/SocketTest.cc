/******************************************************************************
 *
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
#include <gtest/gtest.h>

#include <Status.h>
#include <qcc/Util.h>
#include <qcc/StringUtil.h>

#include <qcc/Socket.h>

using namespace qcc;

/*
 * Note: The test code shall NOT use any well-known or IANA registered ports.
 *       Hence, for test purposes, we shall restrict ourselves to
 *       'Dynamic Ports' range which have been specifically set aside for local
 *       and dynamic use.
 *
 *       Usually, a specific port number in dynamic ports range is not
 *       guaranteed to be available at all times and hence is not used as
 *       a service identifier. However, for the purposes of testing,
 *       ONLY dynamic ports are suitable.
 *
 *       For additional information see:
 *       http://tools.ietf.org/html/rfc6335#section-8.1.2
 */
static uint16_t GetRandomPrivatePortNumber(void) {
    const uint16_t priv_port_num_min = 49152;

    /*
     * The maximum private port number is 65535.
     * The range (min, max) is visualized as a sequence of partitions,
     * each of size 255 (max value of uint8_t returned by Rand8()).
     * The number of partitions would be (65535 - 49152) / 255 = 64 (approx.)
     * Two Rand8() numbers are generated to choose:
     * i.  a particular partition among the 64, and
     * ii. an offset into the partition
     * respectively.
     */

    const uint8_t size_of_each_partition = 255;

    uint8_t ith_partition = Rand8() >> 2;
    uint8_t offset = Rand8();
    return priv_port_num_min + ith_partition * size_of_each_partition + offset;
}

static void DeliverLine(AddressFamily addr_family, SocketType sock_type, String& line) {
    String debug_string;

    IPAddress this_host = (QCC_AF_INET6 == addr_family) ?
                          IPAddress("::1") : IPAddress("127.0.0.1");

    debug_string = (QCC_AF_INET6 == addr_family) ?
                   debug_string.append("Sockets on IPv6 address = ") :
                   debug_string.append("Sockets on IPv4 address = ");
    debug_string += this_host.ToString();
    debug_string = debug_string.append(". ");

    QStatus talker_status = ER_FAIL;
    SocketFd talker = INVALID_SOCKET_FD;
    uint16_t talker_mouth = GetRandomPrivatePortNumber();

    QStatus listener_status = ER_FAIL;
    SocketFd listener = INVALID_SOCKET_FD;
    uint16_t listener_ear = GetRandomPrivatePortNumber();

    debug_string = debug_string.append("Talker socket on port: ");
    debug_string += U32ToString(talker_mouth);
    debug_string = debug_string.append(", Listener socket on port: ");
    debug_string += U32ToString(listener_ear);

    talker_status =
        (ER_OK == Socket(addr_family, sock_type, talker)) &&
        (ER_OK == Bind(talker, this_host, talker_mouth)) ?
        ER_OK : talker_status;

    listener_status =
        (ER_OK == Socket(addr_family, sock_type, listener)) &&
        (ER_OK == Bind(listener, this_host, listener_ear)) ?
        ER_OK : listener_status;

    QStatus connect_status = (ER_OK == talker_status && ER_OK == listener_status) ? ER_OK : ER_FAIL;

    // For the new socketfd returned by Accept()
    SocketFd listener_earpiece = INVALID_SOCKET_FD;

    if (ER_OK == connect_status && QCC_SOCK_STREAM == sock_type) {
        const int num_backlog_connections = 1;
        connect_status = (ER_OK == Listen(listener, num_backlog_connections) &&
                          ER_OK == Connect(talker, this_host, listener_ear) &&
                          ER_OK == Accept(listener, this_host, talker_mouth, listener_earpiece)) ?
                         ER_OK : ER_FAIL;
    }

    debug_string = (QCC_SOCK_STREAM == sock_type) ?
                   debug_string.append(", Type of sockets = TCP. ") :
                   debug_string.append(", Type of sockets = UDP. ");

    if (ER_OK == connect_status) {
        QStatus said = ER_FAIL;
        QStatus heard = ER_FAIL;

        size_t amount_said = 0;
        size_t amount_heard = 0;

        const char* line_literal = line.c_str();
        const size_t line_length = line.length();
        uint8_t* scratch_pad = new uint8_t[line_length];

        said = SendTo(talker, this_host, listener_ear,
                      static_cast<const void*> (line_literal), line_length, amount_said);

        if (ER_OK == said) {
            heard = (QCC_SOCK_STREAM == sock_type) ?
                    Recv(listener_earpiece,
                         static_cast<void*> (scratch_pad), amount_said, amount_heard) :
                    RecvFrom(listener, this_host, talker_mouth,
                             static_cast<void*> (scratch_pad), amount_said, amount_heard);

            if (ER_OK == heard) {
                // Compare the number of said and heard octets
                EXPECT_EQ(amount_said, amount_heard) << debug_string.c_str() <<
                "The number of octets transmitted by the talker: " <<
                amount_said << ", was not equal to the number of "
                "octets received by the listenener: " << amount_heard;

                if (amount_said == amount_heard) {
                    debug_string = debug_string.append("Talker's message: ");
                    debug_string = debug_string.append(line_literal, amount_said);

                    for (uint8_t i = 0; i < amount_heard; i++) {
                        // Compare each said octet with respective heard octet
                        EXPECT_EQ(static_cast<uint8_t> (line_literal[i]), scratch_pad[i]) <<
                        debug_string.c_str() <<
                        " The octet sent by the talker '" << line_literal[i] <<
                        "' does not match the octet got by the listener '" <<
                        scratch_pad[i] << "' at offset " << static_cast<unsigned int> (i) <<
                        " out of " << amount_said << ".";
                    }
                }
            }
        }

        // Clean-up
        delete [] scratch_pad;
        scratch_pad = NULL;

        Close(talker);
        if (QCC_SOCK_STREAM == sock_type) {
            Close(listener_earpiece);
        }
        Close(listener);
    } else {
        // Some OS-level error occurred
        Close(talker);
        printf("\n\tATTN: Test run cancelled possibly due to OS-level errors."
               "\n\t      Talker status (socket creation & binding) was %s."
               "\n\t      Listener status (socket creation & binding) was %s.",
               QCC_StatusText(talker_status), QCC_StatusText(listener_status));
        if (QCC_SOCK_STREAM == sock_type) {
            printf("\n\t      Connect status (listen, connect, accept) was %s.",
                   QCC_StatusText(connect_status));
        }
    }
}

TEST(SocketTest, send_to_and_recv_from_test) {
    const char* wilson_lines[] = {
        "",
        "That smugness of yours really is an attractive quality.",
        "I'm still amazed you're actually in the same room with a patient.",
        "Beauty often seduces us on the road to truth.",
        "I'm not gonna date a patient's daughter.",
        "You really don't need to know everything about everybody.",
        "Be yourself: cold, uncaring, distant.",
        "Did you know your phone is dead? Do you ever recharge the batteries?",
        "Now, why do you have a season pass to The New Yankee Workshop?"
    };
    const char* house_lines[] = {
        "",
        "Thank you. It was either that or get my hair highlighted. Smugness is easier to maintain.",
        "People don't bug me until they get teeth.",
        "And triteness kicks us in the nads.",
        "Very ethical. Of course, most married men would say they don't date at all.",
        "I don't need to watch The O.C., but it makes me happy.",
        "Please, don't put me on a pedestal.",
        "They recharge? I just keep buying new phones.",
        "It's a complete moron working with power tools. How much more suspenseful can you get?"
    };

    String line;

    for (uint8_t i = 0; i < ArraySize(wilson_lines) + ArraySize(house_lines); i++) {
        line = (0 == i % 2) ? String(wilson_lines[i / 2]) : String(house_lines[i / 2]);

        AddressFamily af = (0 == Rand8() % 2) ? QCC_AF_INET6 : QCC_AF_INET;
        SocketType st = (0 == Rand8() % 2) ? QCC_SOCK_DGRAM : QCC_SOCK_STREAM;

        DeliverLine(af, st, line);
    }
}

/*
 * File descriptors are local to a machine and are not meaningful beyond
 * the machine boundaries. Hence, SendWithFds and RecvWithFds functions
 * make sense only when used with a 'local' socket.
 *
 * On POSIX systems, the 'local' socket would mean unix domain socket.
 *
 * On Windows, the 'local' sockets would be a socket on the loopback address.
 *
 * On both platforms (POSIX and Windows), the choice of transport can
 * theoretically be either 'SOCK_STREAM' or not. However, the API signatures
 *
 * SendWithFds(SocketFd sockfd, const void* buf, size_t len, size_t& sent,
 *             SocketFd* fdList, size_t numFds, uint32_t pid);
 * RecvWithFds(SocketFd sockfd, void* buf, size_t len, size_t& received,
 *             SocketFd* fdList, size_t maxFds, size_t& recvdFds);
 *
 * implicitly indicate (lack of parameters specifying
 * the other communication endpoint) that the socket is 'connected'.
 */

TEST(SocketTest, send_and_receive_with_dummy_fds) {
    String debug_string;

    SocketFd endpoint[2];

    QStatus status = SocketPair(endpoint);

    if (ER_OK == status) {
        debug_string = debug_string.append("Successfully created a SocketPair. ");

        String sender_message = String("Sending a list of some dummy fds.");
        // dummy file descriptors
        SocketFd original_list_of_fds[SOCKET_MAX_FILE_DESCRIPTORS];
        for (uint8_t i = 0; i < ArraySize(original_list_of_fds); i++) {
            original_list_of_fds[i] = INVALID_SOCKET_FD;
        }

        AddressFamily addr_family = (0 == Rand8() % 2) ? QCC_AF_INET6 : QCC_AF_INET;
        IPAddress this_host = (QCC_AF_INET6 == addr_family) ? IPAddress("::1") : IPAddress("127.0.0.1");

        // Initialize the dummy list of fds
        for (uint8_t i = 0; i < ArraySize(original_list_of_fds); i++) {
            if (ER_OK == Socket(addr_family, (0 == Rand8() % 2) ? QCC_SOCK_DGRAM : QCC_SOCK_STREAM, original_list_of_fds[i])) {
                QStatus socket_bound = ER_FAIL;
                while (ER_OK != socket_bound) {
                    socket_bound = Bind(original_list_of_fds[i], this_host, GetRandomPrivatePortNumber());
                }
            }
        }

        QStatus sent = ER_FAIL;
        QStatus received = ER_FAIL;

        size_t amount_sent = 0;
        size_t amount_received = 0;

        const char* message_literal = sender_message.c_str();
        const size_t message_length = sender_message.length();
        uint8_t* scratch_pad = new uint8_t[message_length];

        size_t num_of_total_fds = ArraySize(original_list_of_fds);
        SocketFd* stash_of_fds = new SocketFd[num_of_total_fds];
        size_t num_of_recvd_fds = 0;

        sent =
            SendWithFds(endpoint[0],
                        static_cast<const void*> (message_literal), message_length,
                        amount_sent,
                        original_list_of_fds, num_of_total_fds, GetPid());

        if (ER_OK == sent) {
            received = RecvWithFds(endpoint[1],
                                   static_cast<void*> (scratch_pad), amount_sent,
                                   amount_received,
                                   stash_of_fds, num_of_total_fds, num_of_recvd_fds);
            if (ER_OK == received) {
                // Compare the number of sent and received message octets
                EXPECT_EQ(amount_sent, amount_received) << debug_string.c_str() <<
                "The number of octets sent by the sender: " << amount_sent <<
                ", was not equal to the number of octets received by the receiver: " << amount_received;

                if (amount_sent == amount_received) {
                    debug_string = debug_string.append("Sender's message: ");
                    debug_string = debug_string.append(message_literal, amount_sent);

                    for (uint8_t i = 0; i < amount_received; i++) {
                        // Compare each sent octect with respective received octet
                        EXPECT_EQ(static_cast<uint8_t> (message_literal[i]), scratch_pad[i]) <<
                        debug_string.c_str() <<
                        " The octet sent by the sender '" << message_literal[i] <<
                        "' does not match the octet received by the receiver '" << scratch_pad[i] <<
                        "' at offset " << static_cast<unsigned int> (i) <<
                        " out of " << (amount_sent - 1) << ".";
                    }
                }

                // Compare the number of fds sent and received
                EXPECT_EQ(num_of_total_fds, num_of_recvd_fds) << debug_string.c_str() <<
                " The number of fds transmitted by the sender: " << num_of_total_fds <<
                ", was not equal to the number of fds received by the receiver: " << num_of_recvd_fds;

                if (num_of_total_fds == num_of_recvd_fds) {
                    debug_string = debug_string.append(" Sequence of port numbers (corresponding to sent fds): ");
                    for (uint8_t i = 0; i < num_of_total_fds; i++) {
                        uint16_t fd_port_number;
                        if (ER_OK == GetLocalAddress(original_list_of_fds[i], this_host, fd_port_number)) {
                            debug_string += U32ToString(fd_port_number);
                            if (num_of_total_fds - 1 != i) {
                                debug_string = debug_string.append(", ");
                            }

                        }
                    }
                    debug_string = debug_string.append(". ");

                    for (uint8_t i = 0; i < num_of_total_fds; i++) {
                        uint16_t local_addr_of_sent_fd;
                        uint16_t local_addr_of_received_fd;

                        // Compare each sent fd with respective received fd
                        if (ER_OK == GetLocalAddress(original_list_of_fds[i], this_host, local_addr_of_sent_fd) &&
                            ER_OK == GetLocalAddress(stash_of_fds[i], this_host, local_addr_of_received_fd)) {
                            EXPECT_EQ(local_addr_of_sent_fd, local_addr_of_received_fd) <<
                            debug_string.c_str() << "At index: " << static_cast<unsigned int> (i) <<
                            ", the local address (port) of fd sent by the sender: " << local_addr_of_sent_fd <<
                            " does not match the local address (port) of fd received by the receiver: " <<
                            local_addr_of_received_fd;

                        }
                    }
                }
            }
        }

        // Clean-up
        delete [] scratch_pad;
        scratch_pad = NULL;
        delete [] stash_of_fds;
        stash_of_fds = NULL;

        // Relinquish the dummy list of fds
        for (uint8_t i = 0; i < ArraySize(original_list_of_fds); i++) {
            // We only need to close those sockets that are valid. The invalid
            // ones (i.e. SocketFd 0) cannot be bound in the first place.
            if (INVALID_SOCKET_FD != original_list_of_fds[i]) {
                Close(original_list_of_fds[i]);
            }
        }

        Close(endpoint[0]);
        Close(endpoint[1]);
    } else {
        // Some OS-level error occurred
        Close(endpoint[0]);
        Close(endpoint[1]);
        printf("\n\tATTN: Test run cancelled possibly due to OS-level errors."
               "\n\t      Status (socket pair creation) was %s.", QCC_StatusText(status));
    }
}
