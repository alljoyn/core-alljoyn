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

# This script uses OpenSSL to generate the necessary certificates for Test10 and Test11 in SecurityX509Test.

# Generate EC parameters used for all keys.
openssl ecparam -outform PEM -name prime256v1 -out ecparams.pem 

#### TEST10

# Paste and format srveckey.pem into serviceecdsaPrivateKeyPEM
openssl ecparam -genkey -outform PEM -in ecparams.pem -out srveckey.pem

# Paste and format srvpubkey.pem into serviceecdsaPublicKeyPEM
openssl ec -in srveckey.pem -pubout -out srvpubkey.pem

openssl req -new -key srveckey.pem -out srvcsr.pem -sha256 -batch \
-subj '/OU=TestOrganization/CN=TestCommon'

# Paste and format srvcert.pem into serviceecdsaCertChainX509PEM
openssl x509 -req -in srvcsr.pem -out srvcert.pem -days 365 \
-signkey srveckey.pem -set_serial 0x270F \
-extfile openssl-ajekus.cnf -extensions identity

#### TEST11

# Generate CA private key; paste and format into CAPrivateKeyPEM
openssl ecparam -genkey -outform PEM -in ecparams.pem -out caeckey.pem
# Generate client private key; paste and format into clientecdsaPrivateKeyPEM
openssl ecparam -genkey -outform PEM -in ecparams.pem -out clieckey.pem

# Generate private key for intermediate CA A; not used in the code
openssl ecparam -genkey -outform PEM -in ecparams.pem -out intAeckey.pem
# Generate private key for intermediate CA B; not used in the code
openssl ecparam -genkey -outform PEM -in ecparams.pem -out intBeckey.pem

## Generate CSRs

# Root CA
openssl req -new -key caeckey.pem -out cacsr.pem \
-sha256 -batch -subj /OU=CertificateAuthorityOrg/CN=CertificateAuthorityCN

# Intermediate CA A
openssl req -new -key intAeckey.pem -out intAcsr.pem \
-sha256 -batch -subj /OU=Intermediate-A-OU/CN=Intermediate-A-CN

# Intermediate CA B
openssl req -new -key intBeckey.pem -out intBcsr.pem \
-sha256 -batch -subj /OU=Intermediate-B-OU/CN=Intermediate-B-CN

# Client
openssl req -new -key clieckey.pem -out clicsr.pem \
-sha256 -batch -subj /OU=AliceOU/CN=AliceCN

## Create and sign certificates. Paste and format all of these certificates together
## as clientecdsaCertChainX509PEM. Paste and format cacert.pem as CACertificatePEM.

# Root CA (self-signed)
openssl x509 -req -in cacsr.pem -out cacert.pem -signkey caeckey.pem \
-days 7300 -extfile openssl-ajekus.cnf -extensions unrestricted

# Intermediate A (signed by root)
openssl x509 -req -in intAcsr.pem -out intAcert.pem -CA cacert.pem \
-CAkey caeckey.pem -days 7300 -CAcreateserial \
-extfile openssl-ajekus.cnf -extensions unrestricted

# Intermediate B (signed by A)
openssl x509 -req -in intBcsr.pem -out intBcert.pem -CA intAcert.pem \
-CAkey intAeckey.pem -days 7300 -CAcreateserial \
-extfile openssl-ajekus.cnf -extensions unrestricted

# Client (signed by B)
openssl x509 -req -in clicsr.pem -out clicert.pem -CA intBcert.pem \
-CAkey intBeckey.pem -days 7300 -CAcreateserial \
-extfile openssl-ajekus.cnf -extensions identity

