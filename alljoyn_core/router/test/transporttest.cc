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

#include <nio/Proactor.h>
#include <nio/TCPTransport.h>
#include <nio/Buffer.h>
#include <nio/Endpoint.h>

#include <alljoyn/Init.h>

#include <signal.h>

#define MSG_SIZE 8


static void ReadCB(nio::TransportBase::EndpointPtr ep, std::shared_ptr<nio::Buffer> buffer, QStatus status)
{
    printf("%s: Received %u bytes on handle \n", QCC_StatusText(status), buffer->GetLength());

    if (status == ER_OK) {

        printf("Received %u bytes from %s; sending back!\n", buffer->GetLength(), ep->ToString().c_str());
        // compose a new message to go OUT
        std::shared_ptr<nio::Buffer> out_buffer = std::make_shared<nio::Buffer>(new uint8_t[MSG_SIZE], MSG_SIZE, MSG_SIZE);
        ::memcpy(out_buffer->m_buffer, buffer->m_buffer, MSG_SIZE);

        auto cb = [] (nio::TransportBase::EndpointPtr, std::shared_ptr<nio::Buffer> buffer, QStatus status) {
                      printf("%s: Send complete; %u bytes\n", QCC_StatusText(status), buffer->GetCapacity());
                  };
        status = ep->Send(buffer, cb);


        // now continue receiving messages
        std::shared_ptr<nio::Buffer> inbuffer = std::make_shared<nio::Buffer>(new uint8_t[MSG_SIZE], 0, MSG_SIZE);
        status = ep->Recv(inbuffer, &ReadCB);
        printf("ep->Recv: %s\n", QCC_StatusText(status));
    } else {
        printf("EP Disconnected: %s\n", QCC_StatusText(status));
    }
}

static bool AcceptCB(nio::TransportBase::EndpointPtr ep)
{
    printf("Accepted incoming connection from %s\n", ep->ToString().c_str());
    QStatus status;

    // send something!
/*
    uint8_t* buf = new uint8_t[6];
    memset(buf, 0, 6);
    memcpy(buf, "HELLO", 6);

    std::shared_ptr<nio::Buffer> buffer = std::make_shared<nio::Buffer>(buf, 6, 6);
    printf("Sending %p\n", buffer.get());

    auto cb = [] (nio::TransportBase::EndpointPtr, std::shared_ptr<nio::Buffer> buffer, QStatus status) {
        printf("%s: Send complete; %u bytes\n", QCC_StatusText(status), buffer->GetCapacity());
    };
    status = ep->Send(buffer, cb);
    printf("Send: %s\n", QCC_StatusText(status));
 */

    // start receiving...
    std::shared_ptr<nio::Buffer> inbuffer = std::make_shared<nio::Buffer>(new uint8_t[8], 0, 8);
    status = ep->Recv(inbuffer, &ReadCB);
    printf("ep->Recv: %s\n", QCC_StatusText(status));

    return true;
}

static nio::Proactor proactor(1);

static void CDECL_CALL SigIntHandler(int sig)
{
    QCC_UNUSED(sig);
    proactor.Stop();
}

int CDECL_CALL main(int argc, char** argv)
{
    if (AllJoynInit() != ER_OK) {
        return 1;
    }
    if (AllJoynRouterInit() != ER_OK) {
        AllJoynShutdown();
        return 1;
    }

    signal(SIGINT, SigIntHandler);

    nio::TCPTransport transport(proactor);

    std::string spec = argc > 1 ? argv[1] : "tcp:addr=127.0.0.1,port=10000";
    transport.Listen(spec, &AcceptCB);


    proactor.Run();
    printf("Finished!\n");

    AllJoynRouterShutdown();
    AllJoynShutdown();
}


