/*
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
 */
#ifndef _CREDENTIALSHOST_H
#define _CREDENTIALSHOST_H

#include "BusAttachment.h"
#include "ScriptableObject.h"
#include <alljoyn/AuthListener.h>

class _CredentialsHost : public ScriptableObject {
  public:
    _CredentialsHost(Plugin& plugin, ajn::AuthListener::Credentials& credentials);
    virtual ~_CredentialsHost();

  protected:
    ajn::AuthListener::Credentials& credentials;

    bool getPassword(NPVariant* npresult);
    bool setPassword(const NPVariant* npvalue);
    bool getUserName(NPVariant* npresult);
    bool setUserName(const NPVariant* npvalue);
    bool getCertChain(NPVariant* npresult);
    bool setCertChain(const NPVariant* npvalue);
    bool getPrivateKey(NPVariant* npresult);
    bool setPrivateKey(const NPVariant* npvalue);
    bool getLogonEntry(NPVariant* npresult);
    bool setLogonEntry(const NPVariant* npvalue);
    bool getExpiration(NPVariant* npresult);
    bool setExpiration(const NPVariant* npvalue);
};

typedef qcc::ManagedObj<_CredentialsHost> CredentialsHost;

#endif // _CREDENTIALSHOST_H
