/******************************************************************************
 *
 * Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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
#include <qcc/UARTStream.h>
#include <qcc/SLAPStream.h>
#define PACKET_SIZE             100
#define WINDOW_SIZE             4
#define BAUDRATE                115200
#define RANDOM_BYTES_MAX        5000
using namespace qcc;

TEST(UARTTest, DISABLED_uart_large_buffer_test)
{
    Timer timer0("SLAPtimer0", true, 1, false, 10);
    timer0.Start();
    Timer timer1("SLAPtimer1", true, 1, false, 10);
    timer1.Start();
    UARTFd fd0;
    UARTFd fd1;
    QStatus status = UART("/tmp/COM0", BAUDRATE, fd0);
    ASSERT_EQ(status, ER_OK);

    status = UART("/tmp/COM1", BAUDRATE, fd1);
    ASSERT_EQ(status, ER_OK);

    UARTStream* s = new UARTStream(fd0);
    UARTStream* s1 = new UARTStream(fd1);
    SLAPStream h(s, timer0, PACKET_SIZE, WINDOW_SIZE, BAUDRATE);
    SLAPStream h1(s1, timer1, PACKET_SIZE, WINDOW_SIZE, BAUDRATE);
    h.ScheduleLinkControlPacket();
    h1.ScheduleLinkControlPacket();
    IODispatch iodisp("iodisp", 4);
    iodisp.Start();

    UARTController uc(s, iodisp, &h);
    UARTController uc1(s1, iodisp, &h1);
    uc.Start();
    uc1.Start();
    uint8_t rxBuffer[400];
    memset(&rxBuffer, 'R', sizeof(rxBuffer));

    uint8_t txBuffer[400];
    memset(&txBuffer, 'T', sizeof(txBuffer));

    int blocksize = 20;
    for (int blocks = 0; blocks < 20; blocks++) {
        memset(txBuffer + (blocks * blocksize), 'A' + (uint8_t)blocks, blocksize);
    }

    size_t actual;

    h.PushBytes(txBuffer, 400, actual);
    EXPECT_EQ(actual, 400U);


    h1.PullBytes(rxBuffer, 400, actual);
    EXPECT_EQ(actual, 400U);

    for (int x = 0; x < 400; x++) {
        EXPECT_EQ(txBuffer[x], rxBuffer[x]);
    }

    timer0.Stop();
    timer1.Stop();
    uc.Stop();
    uc1.Stop();
    iodisp.Stop();

    timer0.Join();
    timer1.Join();
    uc.Join();
    uc1.Join();
    iodisp.Join();

    h.Close();
    h1.Close();
    delete s;
    delete s1;
}
TEST(UARTTest, DISABLED_uart_codisco_test)
{
    Timer timer0("SLAPtimer0", true, 1, false, 10);
    timer0.Start();
    Timer timer1("SLAPtimer1", true, 1, false, 10);
    timer1.Start();
    UARTFd fd0;
    UARTFd fd1;
    QStatus status = UART("/tmp/COM0", BAUDRATE, fd0);
    ASSERT_EQ(status, ER_OK);

    status = UART("/tmp/COM1", BAUDRATE, fd1);
    ASSERT_EQ(status, ER_OK);

    UARTStream* s = new UARTStream(fd0);
    UARTStream* s1 = new UARTStream(fd1);
    SLAPStream h(s, timer0, PACKET_SIZE, WINDOW_SIZE, BAUDRATE);
    SLAPStream h1(s1, timer1, PACKET_SIZE, WINDOW_SIZE, BAUDRATE);
    h.ScheduleLinkControlPacket();
    h1.ScheduleLinkControlPacket();
    IODispatch iodisp("iodisp", 4);
    iodisp.Start();

    UARTController uc(s, iodisp, &h);
    UARTController uc1(s1, iodisp, &h1);
    uc.Start();
    uc1.Start();
    uint8_t rxBuffer[400];
    memset(&rxBuffer, 'R', sizeof(rxBuffer));

    uint8_t txBuffer[400];
    memset(&txBuffer, 'T', sizeof(txBuffer));

    int blocksize = 20;
    for (int blocks = 0; blocks < 20; blocks++) {
        memset(txBuffer + (blocks * blocksize), 'A' + (uint8_t)blocks, blocksize);
    }

    size_t actual;
    /* Wait for link to become Active */
    qcc::Sleep(1000);
    h.Close();

    status = h1.PullBytes(rxBuffer, 400, actual);
    EXPECT_EQ(status, ER_SLAP_OTHER_END_CLOSED);

    timer0.Stop();
    timer1.Stop();
    uc.Stop();
    uc1.Stop();
    iodisp.Stop();

    timer0.Join();
    timer1.Join();
    uc.Join();
    uc1.Join();
    iodisp.Join();

    h1.Close();
    delete s;
    delete s1;
}
TEST(UARTTest, DISABLED_uart_small_buffer_test)
{
    Timer timer0("SLAPtimer0", true, 1, false, 10);
    timer0.Start();
    Timer timer1("SLAPtimer1", true, 1, false, 10);
    timer1.Start();
    UARTFd fd0;
    UARTFd fd1;
    QStatus status = UART("/tmp/COM0", BAUDRATE, fd0);
    ASSERT_EQ(status, ER_OK);

    status = UART("/tmp/COM1", BAUDRATE, fd1);
    ASSERT_EQ(status, ER_OK);

    UARTStream* s = new UARTStream(fd0);
    UARTStream* s1 = new UARTStream(fd1);
    /* Test different packet size and window size values */
    SLAPStream h(s, timer0, 1000, WINDOW_SIZE, BAUDRATE);
    SLAPStream h1(s1, timer1, PACKET_SIZE, 2, BAUDRATE);
    h.ScheduleLinkControlPacket();
    h1.ScheduleLinkControlPacket();

    IODispatch iodisp("iodisp", 4);
    iodisp.Start();

    UARTController uc(s, iodisp, &h);
    UARTController uc1(s1, iodisp, &h1);
    uc.Start();
    uc1.Start();

    char buf[16] = "AAAAA";
    char buf1[16] = "BBBBB";
    size_t x;
    char buf2[16] = "CCCCC";
    char buf3[16] = "DDDDD";
    char buf4[16] = "EEEEE";

    status = h.PushBytes(&buf, 5, x);
    EXPECT_EQ(status, ER_OK);
    EXPECT_EQ(x, 5U);
    status = h.PushBytes(&buf1, 5, x);
    EXPECT_EQ(status, ER_OK);
    EXPECT_EQ(x, 5U);

    status = h.PushBytes(&buf2, 5, x);
    EXPECT_EQ(status, ER_OK);
    EXPECT_EQ(x, 5U);

    status = h.PushBytes(&buf3, 5, x);
    EXPECT_EQ(status, ER_OK);
    EXPECT_EQ(x, 5U);

    size_t act;

    status = h1.PullBytes(&buf, 12, act);
    EXPECT_EQ(status, ER_OK);
    EXPECT_EQ(act, 12U);

    status = h1.PullBytes(&buf, 8, act);
    EXPECT_EQ(status, ER_OK);
    EXPECT_EQ(act, 8U);


    status = h.PushBytes(&buf4, 5, x);
    EXPECT_EQ(status, ER_OK);
    EXPECT_EQ(x, 5U);

    status = h.PushBytes(&buf, 5, x);
    EXPECT_EQ(status, ER_OK);
    EXPECT_EQ(x, 5U);

    status = h.PushBytes(&buf1, 5, x);
    EXPECT_EQ(status, ER_OK);
    EXPECT_EQ(x, 5U);


    status = h1.PullBytes(&buf, 15, act);
    EXPECT_EQ(status, ER_OK);
    EXPECT_EQ(act, 15U);

    timer0.Stop();
    timer1.Stop();
    uc.Stop();
    uc1.Stop();
    iodisp.Stop();

    timer0.Join();
    timer1.Join();
    uc.Join();
    uc1.Join();
    iodisp.Join();

    h.Close();
    h1.Close();
    delete s;
    delete s1;
}
/* The following two tests are written using the Gtest framework but arent really unit tests.
 * The tests are to be run side by side and both ends send and receive data.
 */
TEST(UARTTest, DISABLED_serial_testrecv) {
    Timer timer("SLAPtimer", true, 1, false, 10);
    timer.Start();
    uint8_t rxBuffer[1600];
    memset(&rxBuffer, '\0', sizeof(rxBuffer));
    uint8_t txBuffer[1600];
    memset(&txBuffer, 'T', sizeof(txBuffer));

    int blocksize = 100;
    for (int blocks = 0; blocks < 16; blocks++) {
        memset(txBuffer + (blocks * blocksize), 0x41 + (uint8_t)blocks, blocksize);
    }
    UARTFd fd0;
    QStatus status = UART("/tmp/COM0", BAUDRATE, fd0);
    ASSERT_EQ(status, ER_OK);

    UARTStream* s = new UARTStream(fd0);
    SLAPStream h(s, timer, PACKET_SIZE, WINDOW_SIZE, BAUDRATE);
    h.ScheduleLinkControlPacket();

    IODispatch iodisp("iodisp", 4);
    iodisp.Start();

    UARTController uc(s, iodisp, &h);
    uc.Start();
    size_t act;
    for (int iter = 0; iter < 10; iter++) {
        printf("iteration %d", iter);
        h.PullBytes(rxBuffer, 200, act);
        EXPECT_EQ(act, 200U);
        qcc::Sleep(500);
        printf(".");

        h.PullBytes(rxBuffer + 200, 200, act);
        EXPECT_EQ(act, 200U);
        qcc::Sleep(500);
        printf(".");

        h.PullBytes(rxBuffer + 400, 200, act);
        EXPECT_EQ(act, 200U);
        qcc::Sleep(500);
        printf(".");

        h.PullBytes(rxBuffer + 600, 200, act);
        EXPECT_EQ(act, 200U);
        qcc::Sleep(500);
        printf(".");

        h.PullBytes(rxBuffer + 800, 200, act);
        EXPECT_EQ(act, 200U);
        qcc::Sleep(500);
        printf(".");

        h.PullBytes(rxBuffer + 1000, 200, act);
        EXPECT_EQ(act, 200U);
        qcc::Sleep(500);
        printf(".");

        h.PullBytes(rxBuffer + 1200, 200, act);
        EXPECT_EQ(act, 200U);
        qcc::Sleep(500);
        printf(".");

        h.PullBytes(rxBuffer + 1400, 200, act);
        EXPECT_EQ(act, 200U);
        for (int i = 0; i < 1600; i++) {
            EXPECT_EQ(txBuffer[i], rxBuffer[i]);
        }
        h.PushBytes(txBuffer, sizeof(txBuffer), act);
        EXPECT_EQ(act, 1600U);
        printf("\n");

    }

    /* Wait for retransmission to finish */
    qcc::Sleep(4000);

    timer.Stop();
    uc.Stop();
    iodisp.Stop();

    timer.Join();
    uc.Join();
    iodisp.Join();

    h.Close();
    delete s;
}

TEST(UARTTest, DISABLED_serial_testsend) {
    Timer timer("SLAPtimer", true, 1, false, 10);
    timer.Start();
    uint8_t rxBuffer[1600];
    memset(&rxBuffer, 'R', sizeof(rxBuffer));

    uint8_t txBuffer[1600];
    memset(&txBuffer, 'T', sizeof(txBuffer));

    int blocksize = 100;
    for (int blocks = 0; blocks < 16; blocks++) {
        memset(txBuffer + (blocks * blocksize), 0x41 + (uint8_t)blocks, blocksize);
    }
    size_t x;
    UARTFd fd1;
    QStatus status = UART("/tmp/COM1", BAUDRATE, fd1);
    ASSERT_EQ(status, ER_OK);

    UARTStream* s1 = new UARTStream(fd1);
    SLAPStream h1(s1, timer, PACKET_SIZE, WINDOW_SIZE, BAUDRATE);
    h1.ScheduleLinkControlPacket();

    IODispatch iodisp("iodisp", 4);
    iodisp.Start();

    UARTController uc(s1, iodisp, &h1);
    uc.Start();

    for (int iter = 0; iter < 10; iter++) {
        printf("iteration %d", iter);
        h1.PushBytes(txBuffer, sizeof(txBuffer), x);
        EXPECT_EQ(x, 1600U);
        printf(".");
        h1.PullBytes(rxBuffer, 1600, x);
        EXPECT_EQ(x, 1600U);
        printf(".");
        for (int i = 0; i < 1600; i++) {
            EXPECT_EQ(txBuffer[i], rxBuffer[i]);
        }
        printf("\n");

    }
    /* Wait for retransmission to finish */
    qcc::Sleep(4000);

    timer.Stop();
    uc.Stop();
    iodisp.Stop();

    timer.Join();
    uc.Join();
    iodisp.Join();

    h1.Close();
    delete s1;
}

TEST(UARTTest, DISABLED_serial_testrecv_ajtcl) {

    Timer timer("SLAPtimer", true, 1, false, 10);
    timer.Start();

    uint8_t rxBuffer[1600];
    memset(&rxBuffer, '\0', sizeof(rxBuffer));

    uint8_t txBuffer[1600];
    memset(&txBuffer, 'T', sizeof(txBuffer));

    int blocksize = 100;

    for (int blocks = 0; blocks < 16; blocks++) {
        memset(txBuffer + (blocks * blocksize), 0x41 + (uint8_t)blocks, blocksize);
    }

    UARTFd fd0;
    QStatus status = UART("/tmp/COM0", BAUDRATE, fd0);
    ASSERT_EQ(status, ER_OK);

    UARTStream* s = new UARTStream(fd0);
    SLAPStream h(s, timer, PACKET_SIZE, WINDOW_SIZE, BAUDRATE);
    h.ScheduleLinkControlPacket();

    IODispatch iodisp("iodisp", 4);
    iodisp.Start();

    UARTController uc(s, iodisp, &h);
    uc.Start();


    size_t act;

    h.PullBytes(rxBuffer, 1600, act);
    EXPECT_EQ(act, 1600U);

    for (int i = 0; i < 1600; i++) {
        EXPECT_EQ(txBuffer[i], rxBuffer[i]);
    }
    printf("\n");

    timer.Stop();
    uc.Stop();
    iodisp.Stop();

    timer.Join();
    uc.Join();
    iodisp.Join();

    h.Close();
    delete s;
}

TEST(UARTTest, DISABLED_serial_testsend_ajtcl)
{

    Timer timer("SLAPtimer", true, 1, false, 10);
    timer.Start();

    uint8_t rxBuffer[1600];
    memset(&rxBuffer, 'R', sizeof(rxBuffer));

    uint8_t txBuffer[1600];
    memset(&txBuffer, 'T', sizeof(txBuffer));

    int blocksize = 100;

    for (int blocks = 0; blocks < 16; blocks++) {
        memset(txBuffer + (blocks * blocksize), 0x41 + (uint8_t)blocks, blocksize);
    }

    size_t x;

    UARTFd fd1;
    QStatus status = UART("/tmp/COM1", BAUDRATE, fd1);
    ASSERT_EQ(status, ER_OK);

    UARTStream* s1 = new UARTStream(fd1);
    SLAPStream h1(s1, timer, PACKET_SIZE, WINDOW_SIZE, BAUDRATE);
    h1.ScheduleLinkControlPacket();
    IODispatch iodisp("iodisp", 4);
    iodisp.Start();

    UARTController uc1(s1, iodisp, &h1);
    uc1.Start();


    h1.PushBytes(txBuffer, sizeof(txBuffer), x);
    EXPECT_EQ(x, 1600U);

    /* Wait for retransmission to finish */
    qcc::Sleep(4000);
    timer.Stop();
    uc1.Stop();
    iodisp.Stop();

    timer.Join();
    uc1.Join();
    iodisp.Join();

    h1.Close();
    delete s1;
}
TEST(UARTTest, DISABLED_serial_testrandomecho) {
    Timer timer("SLAPtimer", true, 1, false, 10);
    timer.Start();
    uint8_t rxBuffer[RANDOM_BYTES_MAX];
    memset(&rxBuffer, '\0', sizeof(rxBuffer));

    size_t x;
    UARTFd fd1;
    QStatus status = UART("/tmp/COM0", BAUDRATE, fd1);
    ASSERT_EQ(status, ER_OK);

    UARTStream* s1 = new UARTStream(fd1);
    SLAPStream h1(s1, timer, PACKET_SIZE, WINDOW_SIZE, BAUDRATE);
    h1.ScheduleLinkControlPacket();

    IODispatch iodisp("iodisp", 4);
    iodisp.Start();

    UARTController uc(s1, iodisp, &h1);
    uc.Start();
    int iter = 0;
    size_t actual;
    while (1) {
        printf("iteration %d\n", iter);
        status = h1.PullBytes(rxBuffer, RANDOM_BYTES_MAX, x, 5000);
        if (status == ER_TIMEOUT) {
            continue;
        }
        if (status != ER_OK) {
            printf("Failed PullBytes status = %s\n", QCC_StatusText(status));
            break;
        }
        iter++;
        /* Echo same bytes back to sender */
        h1.PushBytes(rxBuffer, x, actual);
        EXPECT_EQ(x, actual);

        printf("\n");

    }
    /* Wait for retransmission to finish */
    qcc::Sleep(4000);

    timer.Stop();
    uc.Stop();
    iodisp.Stop();

    timer.Join();
    uc.Join();
    iodisp.Join();

    h1.Close();
    delete s1;
}
TEST(UARTTest, DISABLED_serial_testsendrecv) {
    Timer timer("SLAPtimer", true, 1, false, 10);
    timer.Start();
    uint8_t rxBuffer[RANDOM_BYTES_MAX];
    memset(&rxBuffer, 'R', sizeof(rxBuffer));

    uint8_t txBuffer[RANDOM_BYTES_MAX];
    memset(&txBuffer, 'T', sizeof(txBuffer));

    int blocksize = 100;
    for (int blocks = 0; blocks < 16; blocks++) {
        memset(txBuffer + (blocks * blocksize), 0x41 + (uint8_t)blocks, blocksize);
    }
    size_t x;
    UARTFd fd1;
    QStatus status = UART("/tmp/COM1", BAUDRATE, fd1);
    ASSERT_EQ(status, ER_OK);

    UARTStream* s1 = new UARTStream(fd1);
    SLAPStream h1(s1, timer, PACKET_SIZE, WINDOW_SIZE, BAUDRATE);
    h1.ScheduleLinkControlPacket();

    IODispatch iodisp("iodisp", 4);
    iodisp.Start();

    UARTController uc(s1, iodisp, &h1);
    uc.Start();
    int iter = 0;
    size_t txlen;
    while (1) {
        printf("iteration %d\n", iter);
        iter++;
        txlen = rand() % RANDOM_BYTES_MAX;
        for (size_t i = 0; i < txlen; i++) {
            txBuffer[i] = rand() % 256;
        }
        /* Send bytes */
        h1.PushBytes(txBuffer, txlen, x);
        EXPECT_EQ(x, txlen);

        /* Read bytes back */
        status = h1.PullBytes(rxBuffer, txlen, x);
        EXPECT_EQ(x, txlen);
        EXPECT_EQ(memcmp(txBuffer, rxBuffer, txlen), 0);

    }
    /* Wait for retransmission to finish */
    qcc::Sleep(4000);

    timer.Stop();
    uc.Stop();
    iodisp.Stop();

    timer.Join();
    uc.Join();
    iodisp.Join();

    h1.Close();
    delete s1;
}

TEST(UARTTest, DISABLED_valid_parameters)
{
    const uint8_t databits[] = { 5, 6, 7, 8 };
    const qcc::String parity[] = { "none", "even", "odd", "mark", "space" };
    const uint8_t stopbits[] = { 1, 2 };

#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))
    for (size_t d = 0; d < SIZEOF(databits); ++d) {
        for (size_t p = 0; p < SIZEOF(parity); ++p) {
            for (size_t s = 0; s < SIZEOF(stopbits); ++s) {
                UARTFd fd;
                QStatus status = UART("/tmp/COM0", BAUDRATE, databits[d], parity[p], stopbits[s], fd);
                ASSERT_EQ(ER_OK, status);
                ASSERT_NE(-1, fd);
                close(fd);
            }
        }
    }
}
