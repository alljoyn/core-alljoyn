@echo off

REM    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
REM    Source Project (AJOSP) Contributors and others.
REM
REM    SPDX-License-Identifier: Apache-2.0
REM
REM    All rights reserved. This program and the accompanying materials are
REM    made available under the terms of the Apache License, Version 2.0
REM    which accompanies this distribution, and is available at
REM    http://www.apache.org/licenses/LICENSE-2.0
REM
REM    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
REM    Alliance. All rights reserved.
REM
REM    Permission to use, copy, modify, and/or distribute this software for
REM    any purpose with or without fee is hereby granted, provided that the
REM    above copyright notice and this permission notice appear in all
REM    copies.
REM
REM    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
REM    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
REM    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
REM    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
REM    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
REM    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
REM    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
REM    PERFORMANCE OF THIS SOFTWARE.

REM This script takes PFX files and a local copy of OpenSSL, and extracts the certificate from
REM cacert.pfx into cacert.pem. It then extracts from clicert.pfx the private key, which is saved
REM as clikey.pem, and the client certificate, which is saved as clicert.pem.

openssl pkcs12 -in cacert.pfx -password pass:12345678 -nokeys -out cacert.pem

openssl pkcs12 -in clicert.pfx -password pass:12345678 -passout pass:12345678 -nocerts -out clikey-encr.pem
openssl ec -in clikey-encr.pem -passin pass:12345678 -out clieckey.pem
del clikey-encr.pem
openssl pkcs12 -in clicert.pfx -password pass:12345678 -nokeys -out clicert.pem

openssl pkcs12 -in srvcert.pfx -password pass:12345678 -passout pass:12345678 -nocerts -out srvkey-encr.pem
openssl ec -in srvkey-encr.pem -passin pass:12345678 -out srveckey.pem
del srvkey-encr.pem
openssl pkcs12 -in srvcert.pfx -password pass:12345678 -nokeys -out srvcert.pem