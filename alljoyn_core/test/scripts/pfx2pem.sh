#!/bin/sh
#
# # 
#    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
#    Source Project Contributors and others.
#    
#    All rights reserved. This program and the accompanying materials are
#    made available under the terms of the Apache License, Version 2.0
#    which accompanies this distribution, and is available at
#    http://www.apache.org/licenses/LICENSE-2.0


# This script takes PFX files and a local copy of OpenSSL, and extracts the certificate from
# cacert.pfx into cacert.pem. It then extracts from clicert.pfx the private key, which is saved
# as clikey.pem, and the client certificate, which is saved as clicert.pem.

openssl pkcs12 -in cacert.pfx -password pass:12345678 -nokeys -out cacert.pem

openssl pkcs12 -in clicert.pfx -password pass:12345678 -passout pass:12345678 -nocerts -out clikey-encr.pem
openssl ec -in clikey-encr.pem -passin pass:12345678 -out clieckey.pem
rm clikey-encr.pem
openssl pkcs12 -in clicert.pfx -password pass:12345678 -nokeys -out clicert.pem

openssl pkcs12 -in srvcert.pfx -password pass:12345678 -passout pass:12345678 -nocerts -out srvkey-encr.pem
openssl ec -in srvkey-encr.pem -passin pass:12345678 -out srveckey.pem
rm srvkey-encr.pem
openssl pkcs12 -in srvcert.pfx -password pass:12345678 -nokeys -out srvcert.pem
