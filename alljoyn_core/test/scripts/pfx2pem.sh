#!/bin/sh
#
# Copyright AllSeen Alliance. All rights reserved.
#
#    Permission to use, copy, modify, and/or distribute this software for any
#    purpose with or without fee is hereby granted, provided that the above
#    copyright notice and this permission notice appear in all copies.
#
#    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
#    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
#    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
#    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
#    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
#    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
#    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

# This script takes PFX files and a local copy of OpenSSL, and extracts the certificate from
# cacert.pfx into cacert.pem. It then extracts from clicert.pfx the private key, which is saved
# as clikey.pem, and the client certificate, which is saved as clicert.pem.

openssl pkcs12 -in cacert.pfx -password pass:12345678 -nokeys -out cacert.pem

openssl pkcs12 -in clicert.pfx -password pass:12345678 -passout pass:12345678 -nocerts -out clikey-encr.pem
openssl ec -in clikey-encr.pem -passin pass:12345678 -out clikey.pem
rm clikey-encr.pem
openssl pkcs12 -in clicert.pfx -password pass:12345678 -nokeys -out clicert.pem

openssl pkcs12 -in srvcert.pfx -password pass:12345678 -passout pass:12345678 -nocerts -out srvkey-encr.pem
openssl ec -in srvkey-encr.pem -passin pass:12345678 -out srvkey.pem
rm srvkey-encr.pem
openssl pkcs12 -in srvcert.pfx -password pass:12345678 -nokeys -out srvcert.pem

