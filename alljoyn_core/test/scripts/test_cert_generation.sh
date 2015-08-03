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
#
#
#
#  Generate ECDSA Certificates with OpenSSL for use with AllJoyn
#  April 15, 2015
#
#  These are instructions for generating AllJoyn ECDSA **test** certificates
#  using the OpenSSL command line tools.  Tested with OpenSSL 1.0.1j in Cygwin,
#  and AllJoyn 15.04.  The commands were also tested in Windows with OpenSSL
#  0.9.8e 23 Feb 2007.  If you have issues, please update OpenSSL to version
#  1.0 or newer.  These instructions MUST NOT be used to create production
#  certificates.  Under Unix-based environments you should be able to type
#  './test_cert_generation.sh' and run these commands.  (Make sure the file has
#  the right line endings, and run dos2unix if not, and is executable).  In
#  Windows rename the file to end with ".ps1" then it may be executed as a
#  PowerShell script.  
#
#  Three files will be created (or overwritten if they exist):
#    ecparam.pem : A file with the elliptic curve parameters.  These are
#    required to create keys and certificates with OpenSSL, and not for AllJoyn.
#
#    caeckey.pem : A file with the PEM encoding of the private key in PKCS#8 DER
#    format.
#
#    cacsr.pem : A certificate signing request for the CA. This is not needed for AllJoyn.
#
#    cacert.pem : A self-signed ECDSA certificate corresponding to the private key
#    caeckey.pem
#
#    cacert.srl : Last serial number used for tracking CA issuance in OpenSSL. This is not
#    needed for AllJoyn.
#
#    clieckey.pem : A file with the PEM encoding of the private key for the client
#    in PKCS#8 DER format.
#
#    clicsr.pem : A certificate signing request for the client. This is not needed for AllJoyn.
#
#    clicert.pem : An ECDSA certificate corresponding to the private key clieckey.pem
#    and signed by caeckey.pem.
#
#    srveckey.pem : A file with the PEM encoding of the private key for the service
#    in PKCS#8 DER format. This is suitable for pasting into the SecurityX509Test tests that
#    take an OpenSSL-generated certificate.
#
#    srvpubkey.pem : A file with the PEM encoding of the public key for the service. This is used
#    by SecurityX509Test code for verification.
#
#    srvcsr.pem : A certificate signing request for the service. This is not needed for AllJoyn.
#
#    srvcert.pem : A self-signed ECDSA certificate corresponding to the private key srveckey.pem.
#
#
#  Steps
#
#  1. Create parameters with ecparam:

openssl ecparam -outform PEM -name prime256v1 -out ecparams.pem 

#  The file ecparams.pem should contain:
#    -----BEGIN EC PARAMETERS-----
#    BggqhkjOPQMBBw==
#    -----END EC PARAMETERS-----
#  Which encodes a "named curve", i.e., just the OID of the curve prime256v1.
#  Note that the OpenSSL name "prime256v1" is called nistp256r1 or secp256r1 in
#  other documents/software. The command:
#      openssl ecparam -list_curves
#  can be used to list available curves.
#
#  2. Create the private key for the CA:

openssl ecparam -genkey -outform PEM -in ecparams.pem -out caeckey.pem 

#  The file caeckey.pem should have something like this:
#  -----BEGIN EC PARAMETERS-----
#  BggqhkjOPQMBBw==
#  -----END EC PARAMETERS-----
#  -----BEGIN EC PRIVATE KEY-----
#  MHcCAQEEIAczV89ctcNMUIvR6ZMtXVIUIq4oG0C0sPWOAIbf7Vl9oAoGCCqGSM49
#  AwEHoUQDQgAEhO+ffFIKoH2OIj1U4sUmr3ZUn5BGVmzcB54zeAqcN8P1lIBLHZOC
#  gomUGnIbHZWFDiT2pbHQZv3IxspDEYNKXA==
#  -----END EC PRIVATE KEY-----
#
#  3. Create a certificate signing request for the CA with the private key:

openssl req -new -key caeckey.pem -out cacsr.pem \
-sha256 -batch -subj /O=AllJoynTestSelfSignedName

#  4. Self-sign the certificate with its key and add the 'unrestricted'
#     EKUs (both identity and membership)

openssl x509 -req -in cacsr.pem -out cacert.pem -signkey caeckey.pem \
-days 5000 -extfile openssl-ajekus.cnf -extensions unrestricted

#
#  5. [Optional] View the certificate with:

openssl x509 -in cacert.pem -noout -text ;

#  Alternatively, in Windows, you can rename the file to 'cacert.cer' and double
#  click on the file to view it.

#  6. Create the private key for the client:

openssl ecparam -genkey -outform PEM -in ecparams.pem -out clieckey.pem 

#  7. Create a certificate signing request for the client with the private key:

openssl req -new -key clieckey.pem -out clicsr.pem \
-sha256 -batch -subj /O=AllJoynTestClientName

#  8. Sign the certificate with the CA key and add the 'identity' EKU

openssl x509 -req -in clicsr.pem -out clicert.pem -CA cacert.pem \
-CAkey caeckey.pem -extfile openssl-ajekus.cnf -extensions identity \
-CAcreateserial

#
#  9. [Optional] View the certificate with:

openssl x509 -in clicert.pem -noout -text ;

#  10. Create the private key for the service:

openssl ecparam -genkey -outform PEM -in ecparams.pem -out srveckey.pem 

#  11. Create a certificate signing request for the service with the private key:

openssl req -new -key srveckey.pem -out srvcsr.pem \
-sha256 -batch -subj /O=AllJoynTestServiceName

#  12. Sign the certificate with the service's key and add the 'identity' EKU

openssl x509 -req -in srvcsr.pem -out srvcert.pem -signkey srveckey.pem \
-extfile openssl-ajekus.cnf -extensions identity 

#  13. Compute the public key from the private key for the service and output it to srvpubkey.pem.

openssl ec -in srveckey.pem -pubout -out srvpubkey.pem

#
#  14. [Optional] View the certificate with:

openssl x509 -in srvcert.pem -noout -text ;

#
#
#  Notes
#
#  i) The PEM format is text-based, you can open these files with any text
#  editor.  See the Perl script pem2c.pl to reformat the PEM file to a C string
#  for copying to test code.
#
#  ii) For AllJoyn private keys, we don't include the EC PARAMETERS block in
#  the .pem file, so edit them out, leaving only the EC PRIVATE KEY block in
#  the file.  (The private key format, RFC5915, includes the curve's OID.) Or if
#  you're pasting from the .pem to test code, only copy the "EC PRIVATE KEY"
#  block.  Note also that Visual Studio may auto-reformat the base64 encoded
#  values, (for example, by adding spaces before and after "+") so you may have
#  to press control-Z to undo the formating changes after a paste.
#
#  iii) It's possible to create a private key in the 'openssl req' step, but
#  there is no way to avoid having a passphrase on the private key.  The
#  'openssl ecparam -genkey' lets us create a private key without a passphrase.
#  To create a key with a passphrase drop the -key argument from the 'openssl
#  req' command.
#
#  iv) The website http://www.lapo.it/asn1js/ is handy for decoding PEM files
#  to see what they contain.
