/*
 * IPTransport.h
 *
 *  Created on: Jun 4, 2015
 *      Author: erongo
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
#ifndef IPTRANSPORT_H_
#define IPTRANSPORT_H_

#include "TransportBase.h"

#include <qcc/IPAddress.h>

#include <qcc/IfConfig.h>

#include <map>
#include <list>

namespace nio {

class Proactor;

class IPTransport : public TransportBase {

  public:
    IPTransport(Proactor& proactor, const std::string& name);
    virtual ~IPTransport();

  protected:

    std::string NormalizeConnectionData(const qcc::IPAddress& addr, uint16_t port);

    typedef std::list<qcc::IPEndpoint> ListenEndpoints;
    bool ParseSpec(const std::string& spec, ListenEndpoints& eps);
    bool ParseSpec(const std::string& spec, qcc::IPAddress& ip, uint16_t& port, std::string& normSpec);

    std::map<std::string, std::list<qcc::IPAddress> > ip_addresses;

    void OnNetworkEntries(const std::vector<qcc::IfConfigEntry>& entries);
    void CheckNetworks();

};

} /* namespace nio */

#endif /* IPTRANSPORT_H_ */
