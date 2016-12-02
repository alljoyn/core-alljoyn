@echo off

REM REM 
REM    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
REM    Source Project Contributors and others.
REM    
REM    All rights reserved. This program and the accompanying materials are
REM    made available under the terms of the Apache License, Version 2.0
REM    which accompanies this distribution, and is available at
REM    http://www.apache.org/licenses/LICENSE-2.0


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