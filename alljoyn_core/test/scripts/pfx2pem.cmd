@echo off

REM Copyright AllSeen Alliance. All rights reserved.
REM
REM    Permission to use, copy, modify, and/or distribute this software for any
REM    purpose with or without fee is hereby granted, provided that the above
REM    copyright notice and this permission notice appear in all copies.
REM
REM    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
REM    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
REM    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
REM    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
REM    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
REM    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
REM    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

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
