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
#  April 2, 2015
#
#  These are instructions for generating AllJoyn ECDSA **test** certificates
#  using the OpenSSL command line tools.  Tested with OpenSSL 1.0.1j in Cygwin,
#  and AllJoyn 15.04.  The commands were also tested in Windows with OpenSSL
#  0.9.8e 23 Feb 2007.  If you have issues, please update OpenSSL to version
#  1.0 or newer.  These instructions MUST NOT be used to create production
#  certificates.  Under Cygwin or other Unix-based environments you should be
#  able to type 'sh test_cert_generation.sh' and run these commands.  (Make
#  sure the file has the right line endings, and run dos2unix if not).  In
#  Windows rename the file to end with ".ps1" then it may be executed as a
#  PowerShell script.  If you copy/paste the commands into a Command Prompt
#  (cmd.exe), drop the trailing semincolons. 
#
#  Three files will be created:
#    ecparam.pem : A file with the elliptic curve parameters.  These are
#    required to create keys and certificates with OpenSSL, and not for AllJoyn.
#
#    eckey.pem : A file with the PEM encoding of the private key in PKCS#8 DER
#    format.
#
#    cert.pem : A self-signed ECDSA certificate corresponding to the private key
#    eckey.pem
#
#
#  Steps  
#  
#  1. Create parameters with ecparam:
    openssl ecparam -outform PEM -name prime256v1 -out ecparams.pem ;
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
#  2. Create the private key:
    openssl ecparam -genkey -outform PEM -in ecparams.pem -out eckey.pem ;
#  The file eckey.pem should have something like this:
#  -----BEGIN EC PARAMETERS-----
#  BggqhkjOPQMBBw==
#  -----END EC PARAMETERS-----
#  -----BEGIN EC PRIVATE KEY-----
#  MHcCAQEEIAczV89ctcNMUIvR6ZMtXVIUIq4oG0C0sPWOAIbf7Vl9oAoGCCqGSM49
#  AwEHoUQDQgAEhO+ffFIKoH2OIj1U4sUmr3ZUn5BGVmzcB54zeAqcN8P1lIBLHZOC
#  gomUGnIbHZWFDiT2pbHQZv3IxspDEYNKXA==
#  -----END EC PRIVATE KEY-----
#
#  3. Create a self-signed certificate for the private key:
      openssl req -x509 -newkey ec:ecparams.pem -key eckey.pem -out cert.pem \
      -days 5000 -sha256 -batch -subj /O=AllJoynTestSelfSignedName ;
#
#  4. [Optional] View the certificate with:
      openssl x509 -in cert.pem -noout -text ;
#  Alternatively, in Windows, you can rename the file to 'cert.cer' and double
#  click on the file to view it. 
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