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
#include <nio/TimerEvent.h>

#include <alljoyn/Init.h>

#include <signal.h>

#define MSG_SIZE 8

static nio::Proactor proactor(1);

static void CDECL_CALL SigIntHandler(int sig)
{
    QCC_UNUSED(sig);
    proactor.Stop();
}

void ReadCB(nio::TransportBase::EndpointPtr, std::shared_ptr<nio::Buffer> buf, QStatus status)
{
    if (status == ER_OK) {
        printf("Received %u bytes\n", buf->m_length);
        /*
           std::shared_ptr<nio::Buffer> buffer = std::make_shared<nio::Buffer>(new uint8_t[MSG_SIZE], MSG_SIZE, MSG_SIZE);

           auto cb = [] (nio::TransportBase::EndpointPtr, std::shared_ptr<nio::Buffer> buffer, QStatus status) {
            printf("%s: Send complete; %u bytes\n", QCC_StatusText(status), buffer->GetCapacity());
           };
           status = ep->Send(buffer, cb);
           printf("ep->Send: %s\n", QCC_StatusText(status));
         */

        // set up to receive the next message
        //std::shared_ptr<nio::Buffer> inbuffer = std::make_shared<nio::Buffer>(new uint8_t[MSG_SIZE], 0, MSG_SIZE);
        //status = ep->Recv(inbuffer, &Received);
        //printf("ep->Recv: %s\n", QCC_StatusText(status));
    } else {
        printf("Recieved: %s\n", QCC_StatusText(status));
    }
}

static std::shared_ptr<nio::TimerEvent> timer_event;

void TimerCallback(nio::TransportBase::EndpointPtr ep)
{
    // do a send and prepare to receive!

    std::shared_ptr<nio::Buffer> inbuffer = std::make_shared<nio::Buffer>(new uint8_t[8], 0, 8);
    QStatus status = ep->Recv(inbuffer, &ReadCB);
    if (status != ER_OK) {
        proactor.Cancel(timer_event);
        timer_event.reset();
        return;
    }
    printf("ep->Recv: %s\n", QCC_StatusText(status));


    std::shared_ptr<nio::Buffer> buffer = std::make_shared<nio::Buffer>(new uint8_t[MSG_SIZE], MSG_SIZE, MSG_SIZE);
    for (uint32_t i = 0; i < MSG_SIZE; ++i) {
        buffer->m_buffer[i] = i;
    }

    auto cb = [] (nio::TransportBase::EndpointPtr, std::shared_ptr<nio::Buffer> buffer, QStatus status) {
                  printf("%s: Send complete; %u bytes\n", QCC_StatusText(status), buffer->GetCapacity());
              };
    status = ep->Send(buffer, cb);
    if (status != ER_OK) {
        proactor.Cancel(timer_event);
        timer_event.reset();
        return;
    }
    printf("ep->Send: %s\n", QCC_StatusText(status));
}


void ConnectedCB(nio::TransportBase::EndpointPtr ep, QStatus status)
{
    if (status == ER_OK) {
        printf("Connected to %s\n", ep->ToString().c_str());
        auto timercb = [ep] () {
                           TimerCallback(ep);
                       };

        timer_event = std::make_shared<nio::TimerEvent>(std::chrono::milliseconds(1000), timercb, std::chrono::milliseconds(1000));
        proactor.Register(timer_event);
    } else {
        printf("ConnectedCB: %s\n", QCC_StatusText(status));
    }
}

int CDECL_CALL main(int argc, char** argv)
{
    QCC_UNUSED(argc);
    QCC_UNUSED(argv);

    if (AllJoynInit() != ER_OK) {
        return 1;
    }
    if (AllJoynRouterInit() != ER_OK) {
        AllJoynShutdown();
        return 1;
    }

    signal(SIGINT, SigIntHandler);
    nio::TCPTransport transport(proactor);
    transport.Connect("tcp:addr=127.0.0.1,port=10000", &ConnectedCB);



    proactor.Run();

    AllJoynRouterShutdown();
    AllJoynShutdown();
}
