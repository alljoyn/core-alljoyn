/**
 * @file
 *
 * This file implements a UART based physical link for communication.
 */

/******************************************************************************
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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

#include <qcc/UARTStream.h>


#define QCC_MODULE "UART"

using namespace qcc;
using namespace std;
namespace qcc {
QStatus UART(const qcc::String& devName, uint32_t baud, qcc::UARTFd& fd)
{
    return UART(devName, baud, 8, "none", 1, fd);
}
QStatus UART(const qcc::String& devName, uint32_t baud, uint8_t databits, const qcc::String& parity, uint8_t stopbits, qcc::UARTFd& fd)
{
    QCC_DbgTrace(("UART(devName=%s,baud=%d,databits=%d,parity=%s,stopbits=%d)",
                  devName.c_str(), baud, databits, parity.c_str(), stopbits));
    fd = -1;

    return ER_NOT_IMPLEMENTED;
}
}
UARTStream::UARTStream(UARTFd fd) :
    fd(fd),
    sourceEvent(new Event(fd, Event::IO_READ)),
    sinkEvent(new Event(*sourceEvent, Event::IO_WRITE, false))
{

}

UARTStream::~UARTStream()
{
    delete sourceEvent;
    delete sinkEvent;
}
#define RX_BUFSIZE  640
static uint8_t RxBuffer[RX_BUFSIZE];
QStatus UARTStream::PullBytes(void* buf, size_t numBytes, size_t& actualBytes, uint32_t timeout) {
    return ER_NOT_IMPLEMENTED;
}
void UARTStream::Close() {

}
QStatus UARTStream::PushBytes(const void* buf, size_t numBytes, size_t& actualBytes) {
    return ER_NOT_IMPLEMENTED;
}

UARTController::UARTController(UARTStream* uartStream, IODispatch& iodispatch, UARTReadListener* readListener) :
    m_uartStream(uartStream), m_iodispatch(iodispatch), m_readListener(readListener), exitCount(0)
{
}

QStatus UARTController::Start()
{
    return ER_NOT_IMPLEMENTED;
}

QStatus UARTController::Stop()
{
    return ER_NOT_IMPLEMENTED;
}

QStatus UARTController::Join()
{
    return ER_NOT_IMPLEMENTED;
}

QStatus UARTController::ReadCallback(Source& source, bool isTimedOut)
{
    return ER_NOT_IMPLEMENTED;

}

void UARTController::ExitCallback()
{

}
