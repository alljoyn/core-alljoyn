/**
 * @file
 * TransportList is a factory for Transports
 */

/******************************************************************************
 * Copyright (c) 2009-2012, AllSeen Alliance. All rights reserved.
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
#include <qcc/platform.h>
#include <qcc/String.h>

#include "Transport.h"
#include "TransportList.h"
#include "LocalTransport.h"

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;

namespace ajn {

TransportList::TransportList(BusAttachment& bus, TransportFactoryContainer& factories, IODispatch* m_ioDispatch, uint32_t concurrency)
    : bus(bus), localTransport(new LocalTransport(bus, concurrency)), m_factories(factories), isStarted(false), isInitialized(false), m_ioDispatch(m_ioDispatch)
{
}

TransportList::~TransportList()
{
    Stop();
    Join();

    for (size_t i = 0; i < transportList.size(); ++i) {
        delete transportList[i];
    }

    transportList.clear();

    delete localTransport;
}

QStatus TransportList::NormalizeTransportSpec(const char* inSpec, qcc::String& outSpec, map<qcc::String, qcc::String>& argMap)
{
    QStatus status;
    qcc::String inSpecStr(inSpec);

    Transport* trans = GetTransport(inSpecStr);
    if (trans) {
        status = trans->NormalizeTransportSpec(inSpec, outSpec, argMap);
    } else {
        status = ER_BUS_TRANSPORT_NOT_AVAILABLE;
    }
    return status;
}

Transport* TransportList::GetTransport(const qcc::String& transportSpec)
{
    Transport* transport = NULL;

    if (isInitialized && isStarted) {

        /* Try to get existing transport */
        size_t colonOff = transportSpec.find_first_of(':');
        for (size_t i = 0; i < transportList.size(); ++i) {
            Transport*& trans = transportList[i];
            if (0 == transportSpec.compare(0, colonOff, trans->GetTransportName())) {
                transport = trans;
                break;
            }
        }
    }

    return transport;
}

QStatus TransportList::Start(const String& transportSpecs)
{
    QCC_DbgPrintf(("TransportList::Start(specs = %s)", transportSpecs.c_str()));

    if (!isInitialized) {
        /*
         * The container of transport factories is used to do the actual
         * creation of the transports.  This is done in order to allow the
         * code at the highest level that knows if it is a daemon or a
         * client/service to specify what flavor of transport it wants.  For
         * example, a client will want the lightweight client version of the
         * TCPTransport, but a daemon will want to get the version that supports
         * inbound connections.
         *
         * The higher level code then provides a container of objects that
         * knows how to create transports.  We get a string abstractly
         * describing the kind of object here, for example "tcp" or "unix".
         * We look to the factory container for objects that can do the actual
         * creation.  The objects created when the string, "tcp" is found will
         * differ depending on the factories.  Since the factories are templated,
         * code will not be instantiated if particular specializations are not
         * used, so it is not necessary to link all flavors of transports in
         * all cases.
         */
        String specs = transportSpecs;
        size_t startPos = 0;

        /*
         * Add the default transports.  The defaults are also specified by the
         * TransportFactory settings, so different programs or samples can
         * specify different defaults.
         */
        for (uint32_t i = 0; i < m_factories.Size(); ++i) {
            TransportFactoryBase* factory = m_factories.Get(i);
            if (factory->IsDefault()) {
                transportList.push_back(factory->Create(bus));
            }
        }

        /*
         * The transport factory container provides some number of factories,
         * but only automatically creates instances of the default transports.
         * Other transports are created on-demand by specifying them in the
         * transportSpecs parameter provided to this method.
         */
        do {
            size_t endPos = specs.find_first_of(';', startPos);
            String spec = (String::npos == endPos) ? specs.substr(startPos) : specs.substr(startPos, endPos - startPos);
            size_t colonOff = spec.find_first_of(':');
            if (String::npos != colonOff) {
                String ttype = spec.substr(0, colonOff);
                Transport* exists = NULL;
                /*
                 * Check if transport has already been created
                 */
                for (size_t i = 0; i < transportList.size(); ++i) {
                    if (ttype == transportList[i]->GetTransportName()) {
                        exists = transportList[i];
                        break;
                    }
                }
                if (exists) {
                    QCC_DbgHLPrintf(("Transport %s already created", ttype.c_str()));
                } else {
                    for (uint32_t i = 0; i < m_factories.Size(); ++i) {
                        TransportFactoryBase* factory = m_factories.Get(i);
                        if (factory->GetType() == ttype && factory->IsDefault() == false) {
                            transportList.push_back(factory->Create(bus));
                        }
                    }
                }
            }
            startPos = ((String::npos == endPos) || (specs.size() <= (endPos + 1))) ? String::npos : specs.find_first_not_of(";", endPos);
        } while (startPos != String::npos);
        isInitialized = true;
    }

    /*
     * Start all of the transports we determined we selected above.
     */
    QStatus status = localTransport->Start();
    for (size_t i = 0; i < transportList.size(); ++i) {
        transportList[i]->SetListener(this);
        QStatus s = transportList[i]->Start();
        if (ER_OK == status) {
            status = s;
        }
    }

    /* Start the iodispatch */
    QStatus s = m_ioDispatch->Start();
    if (ER_OK == status) {
        status = s;
    }
    isStarted = (ER_OK == status);
    return status;
}

QStatus TransportList::Stop()
{
    QCC_DbgPrintf(("TransportList::Stop()"));
    isStarted = false;
    QStatus status = localTransport->Stop();
    for (size_t i = 0; i < transportList.size(); ++i) {
        QStatus s = transportList[i]->Stop();
        if (ER_OK == status) {
            status = s;
        }
    }
    /* Stop the iodispatch */
    QStatus s = m_ioDispatch->Stop();
    if (ER_OK == status) {
        status = s;
    }

    return status;
}

QStatus TransportList::Join()
{
    QStatus status = localTransport->Join();

    for (size_t i = 0; i < transportList.size(); ++i) {
        QStatus s = transportList[i]->Join();
        if (ER_OK == status) {
            status = s;
        }
    }
    /* Join the iodispatch */
    QStatus s = m_ioDispatch->Join();
    if (ER_OK == status) {
        status = s;
    }
    return status;
}

QStatus TransportList::RegisterListener(TransportListener* listener)
{
    listeners.push_back(listener);
    return ER_OK;
}

void TransportList::FoundNames(const qcc::String& busAddr,
                               const qcc::String& guid,
                               TransportMask transport,
                               const vector<qcc::String>* names,
                               uint32_t ttl)
{
    if (isStarted) {
        for (size_t i = 0; i < listeners.size(); ++i) {
            listeners[i]->FoundNames(busAddr, guid, transport, names, ttl);
        }
    }
}

void TransportList::BusConnectionLost(const qcc::String& busAddr)
{
    if (isStarted) {
        for (size_t i = 0; i < listeners.size(); ++i) {
            listeners[i]->BusConnectionLost(busAddr);
        }
    }
}

}
