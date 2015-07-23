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

