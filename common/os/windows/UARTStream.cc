/**
 * @file
 *
 * This file implements a UART based physical link for communication.
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

#include <qcc/UARTStream.h>
#include <intsafe.h>

#define RX_BUFSIZE  640

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
    QStatus status = ER_OK;
    HANDLE hSlapCOM = INVALID_HANDLE_VALUE;
    DCB CommConfig = { 0 };
    COMMTIMEOUTS CommTimeouts = { 0 };

    QCC_DbgTrace(("UART(devName=%s,baud=%d,databits=%d,parity=%s,stopbits=%d)",
                  devName.c_str(), baud, databits, parity.c_str(), stopbits));
    fd = reinterpret_cast<qcc::UARTFd>(INVALID_HANDLE_VALUE);

    // Open the COM port in overlapped mode
    hSlapCOM = CreateFileA(devName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
    if (hSlapCOM == INVALID_HANDLE_VALUE) {
        QCC_LogError(ER_OS_ERROR, ("CreateFile() returned %u", GetLastError()));
        status = ER_OS_ERROR;
        goto Cleanup;
    }

    CommConfig.DCBlength = sizeof(CommConfig);
    if (!GetCommState(hSlapCOM, &CommConfig)) {
        QCC_LogError(ER_OS_ERROR, ("GetCommState() returned %u\n", GetLastError()));
        status = ER_OS_ERROR;
        goto Cleanup;
    }

    // Set the COM parameter
    switch (baud) {
    case 110:
        CommConfig.BaudRate = CBR_110;
        break;

    case 300:
        CommConfig.BaudRate = CBR_300;
        break;

    case 600:
        CommConfig.BaudRate = CBR_600;
        break;

    case 1200:
        CommConfig.BaudRate = CBR_1200;
        break;

    case 2400:
        CommConfig.BaudRate = CBR_2400;
        break;

    case 4800:
        CommConfig.BaudRate = CBR_4800;
        break;

    case 9600:
        CommConfig.BaudRate = CBR_9600;
        break;

    case 14400:
        CommConfig.BaudRate = CBR_14400;
        break;

    case 19200:
        CommConfig.BaudRate = CBR_19200;
        break;

    case 38400:
        CommConfig.BaudRate = CBR_38400;
        break;

    case 115200:
        CommConfig.BaudRate = CBR_115200;
        break;

    case 128000:
        CommConfig.BaudRate = CBR_128000;
        break;

    case 256000:
        CommConfig.BaudRate = CBR_256000;
        break;

    default:
        status = ER_BAD_ARG_2;
        goto Cleanup;
    }

    // Databits
    if ((databits >= 5) && (databits <= 8)) {
        CommConfig.ByteSize = databits;
    } else {
        status = ER_BAD_ARG_3;
        goto Cleanup;
    }

    // Parity
    if (strcmp(parity.c_str(), "none") == 0) {
        CommConfig.Parity = NOPARITY;
    } else if (strcmp(parity.c_str(), "even") == 0) {
        CommConfig.Parity = EVENPARITY;
    } else if (strcmp(parity.c_str(), "odd") == 0) {
        CommConfig.Parity = ODDPARITY;
    } else if (strcmp(parity.c_str(), "mark") == 0) {
        CommConfig.Parity = MARKPARITY;
    } else if (strcmp(parity.c_str(), "space") == 0) {
        CommConfig.Parity = SPACEPARITY;
    } else {
        status = ER_BAD_ARG_4;
        goto Cleanup;
    }

    // Stopbits
    switch (stopbits) {
    case 1:
        CommConfig.StopBits = ONESTOPBIT;
        break;

    case 2:
        CommConfig.StopBits = TWOSTOPBITS;
        break;

    default:
        status = ER_BAD_ARG_5;
        goto Cleanup;
    }

    CommConfig.fBinary = TRUE;

    // No software handshake
    CommConfig.fOutxCtsFlow = FALSE;
    CommConfig.fOutxDsrFlow = FALSE;
    CommConfig.fOutX = FALSE;
    CommConfig.fInX = FALSE;

    // No hardware handshake
    CommConfig.fDsrSensitivity = FALSE;
    CommConfig.fDtrControl = DTR_CONTROL_DISABLE;
    CommConfig.fRtsControl = RTS_CONTROL_DISABLE;

    // Set the COMM parameters
    if (!SetCommState(hSlapCOM, &CommConfig)) {
        QCC_LogError(ER_OS_ERROR, ("SetCommState() returned %u\n", GetLastError()));
        status = ER_OS_ERROR;
        goto Cleanup;
    }

    // Set the COMM timing parameters
    if (!GetCommTimeouts(hSlapCOM, &CommTimeouts)) {
        QCC_LogError(ER_OS_ERROR, ("GetCommTimeouts() returned %u\n", GetLastError()));
        status = ER_OS_ERROR;
        goto Cleanup;
    }
    CommTimeouts.ReadIntervalTimeout = 10;
    CommTimeouts.ReadTotalTimeoutMultiplier = 0;
    CommTimeouts.ReadTotalTimeoutConstant = 1;
    CommTimeouts.WriteTotalTimeoutMultiplier = 0;
    CommTimeouts.WriteTotalTimeoutConstant = 1;
    if (!SetCommTimeouts(hSlapCOM, &CommTimeouts)) {
        QCC_LogError(ER_OS_ERROR, ("SetCommTimeouts() returned %u\n", GetLastError()));
        status = ER_OS_ERROR;
        goto Cleanup;
    }

    // Empty the buffers to start with a clean slate
    if (!PurgeComm(hSlapCOM, PURGE_RXCLEAR | PURGE_TXCLEAR)) {
        QCC_LogError(ER_OS_ERROR, ("PurgeComm() returned %u\n", GetLastError()));
        status = ER_OS_ERROR;
        goto Cleanup;
    }

    fd = reinterpret_cast<qcc::UARTFd>(hSlapCOM);

Cleanup:
    if (status != ER_OK) {
        CloseHandle(hSlapCOM);
        hSlapCOM = INVALID_HANDLE_VALUE;
    }
    return status;
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

QStatus UARTStream::PullBytes(void* buf, size_t numBytes, size_t& actualBytes, uint32_t timeout)
{
    QStatus status = ER_OK;
    HANDLE hSlapCOM = reinterpret_cast<HANDLE>(fd);
    OVERLAPPED ov = { 0 };
    ULONG bytesToPull = 0;

    if (IntPtrToULong(numBytes, &bytesToPull) != S_OK) {
        status = ER_BAD_ARG_2;;
        QCC_LogError(ER_BAD_ARG_2, ("Parameter numBytes caused an integer overflow."));
        goto Cleanup;
    }

    // Create the completion event
    if ((ov.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr)) == nullptr) {
        status = ER_OS_ERROR;
        QCC_LogError(ER_OS_ERROR, ("CreateEvent() returned %u", GetLastError()));
        goto Cleanup;
    }

    // Schedule the read operation
    actualBytes = 0;
    if (!ReadFile(hSlapCOM, buf, bytesToPull, (LPDWORD)&actualBytes, &ov)) {
        // Let's see if we were able to schedule the read
        DWORD dwErr = GetLastError();
        if (dwErr != ERROR_IO_PENDING) {
            status = ER_OS_ERROR;
            QCC_LogError(ER_OS_ERROR, ("ReadFile() returned %u", GetLastError()));
            goto Cleanup;
        }

        // Now wait for the completion of the read but make sure to timeout if we got one
        dwErr = WaitForSingleObject(ov.hEvent, timeout ? timeout : INFINITE);
        if (dwErr == WAIT_OBJECT_0) {
            // Read complete
            actualBytes = ov.InternalHigh;
        } else if (dwErr == WAIT_TIMEOUT) {
            // No data came in the allotted time
            status = ER_TIMEOUT;
            QCC_LogError(ER_TIMEOUT, ("ReadFile() timed out after %dms", timeout));

            // Cancel the pending IO operation and wait it out
            if ((!CancelIoEx(hSlapCOM, &ov)) ||
                (!GetOverlappedResult(hSlapCOM, &ov, (LPDWORD)&actualBytes, TRUE))) {
                status = ER_OS_ERROR;
                QCC_LogError(ER_OS_ERROR, ("Cancelling the IO returned %u", GetLastError()));
            }

            goto Cleanup;
        } else {
            // The wait failed for some other reason, bail
            status = ER_OS_ERROR;
            QCC_LogError(ER_OS_ERROR, ("WaitForSingleObject() returned %u", dwErr));
            goto Cleanup;
        }
    }

    QCC_DbgPrintf(("UARTStream::PullBytes() read %d of %d bytes.", actualBytes, numBytes));

Cleanup:
    if (ov.hEvent != nullptr) {
        // Release the completion event
        QCC_VERIFY(CloseHandle(ov.hEvent));
    }
    return status;
}

void UARTStream::Close()
{
    HANDLE hSlapCOM = reinterpret_cast<HANDLE>(fd);
    QCC_DbgPrintf(("UARTStream::Close()"));

    if (hSlapCOM != INVALID_HANDLE_VALUE) {
        // Release the COM port
        QCC_VERIFY(CloseHandle(hSlapCOM));
        fd = reinterpret_cast<UARTFd>(INVALID_HANDLE_VALUE);
    }
}

QStatus UARTStream::PushBytes(const void* buf, size_t numBytes, size_t& actualBytes)
{
    QStatus status = ER_OK;
    HANDLE hSlapCOM = reinterpret_cast<HANDLE>(fd);
    OVERLAPPED ov = { 0 };
    ULONG bytesToPush = 0;

    if (IntPtrToULong(numBytes, &bytesToPush) != S_OK) {
        status = ER_BAD_ARG_2;
        QCC_LogError(ER_BAD_ARG_2, ("Parameter numBytes caused an integer overflow."));
        goto Cleanup;
    }

    // Create the completion event
    if ((ov.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr)) == nullptr) {
        status = ER_OS_ERROR;
        QCC_LogError(ER_OS_ERROR, ("CreateEvent() returned %u", GetLastError()));
        goto Cleanup;
    }

    // Schedule the write operation
    actualBytes = 0;
    if (!WriteFile(hSlapCOM, buf, bytesToPush, (LPDWORD)&actualBytes, &ov)) {
        // Let's see if we were able to schedule the write
        DWORD dwErr = GetLastError();
        if (dwErr != ERROR_IO_PENDING) {
            status = ER_OS_ERROR;
            QCC_LogError(ER_OS_ERROR, ("WriteFile() returned %u", GetLastError()));
            goto Cleanup;
        }

        // Now wait for the completion of the write, no timeout
        dwErr = WaitForSingleObject(ov.hEvent, INFINITE);
        if (dwErr == WAIT_OBJECT_0) {
            actualBytes = ov.InternalHigh;
        } else {
            // The wait failed for some other reason, bail
            status = ER_OS_ERROR;
            QCC_LogError(ER_OS_ERROR, ("WaitForSingleObject() returned %u", GetLastError()));
            goto Cleanup;
        }
    }

    QCC_DbgPrintf(("UARTStream::PushBytes() - wrote %d of %d bytes.", actualBytes, numBytes));

Cleanup:
    if (ov.hEvent != nullptr) {
        // Release the completion event
        QCC_VERIFY(CloseHandle(ov.hEvent));
    }
    return status;
}

UARTController::UARTController(UARTStream* uartStream, IODispatch& iodispatch, UARTReadListener* readListener) :
    m_uartStream(uartStream), m_iodispatch(iodispatch), m_readListener(readListener), exitCount(0)
{
}

QStatus UARTController::Start()
{
    return m_iodispatch.StartStream(m_uartStream, this, nullptr, this, true, false);
}

QStatus UARTController::Stop()
{
    return m_iodispatch.StopStream(m_uartStream);
}

QStatus UARTController::Join()
{
    while (!exitCount) {
        qcc::Sleep(100);
    }
    return ER_OK;
}

QStatus UARTController::ReadCallback(Source& source, bool isTimedOut)
{
    QCC_UNUSED(isTimedOut);
    QCC_UNUSED(source);

    QStatus status = ER_OK;
    size_t actual = 0;
    uint8_t RxBuffer[RX_BUFSIZE];

    if ((status = m_uartStream->PullBytes(RxBuffer, RX_BUFSIZE, actual)) == ER_OK) {
        m_readListener->ReadEventTriggered(RxBuffer, actual);
        m_iodispatch.EnableReadCallback(m_uartStream);
    }

    return status;
}

void UARTController::ExitCallback()
{
    m_uartStream->Close();
    exitCount = 1;
}
