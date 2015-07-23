@echo off

REM This script generates a CA and client certificate and associated private keys, and then
REM exports them as PKCS#12 PFX files. Unfortunately there aren't any convenient built-in
REM utilities for working with PKCS#12 containers on Windows, and so we have to use OpenSSL
REM to extract the certificates and private key from the PFX files in the pfx2pem script.
REM OpenSSL is only used to process the PKCS#12 container; all certificate and key generation
REM is done by CAPI.

REM Please note that this method of generating certificates on Windows does not allow for setting
REM the CA flag in the basicConstraints extension to FALSE, so client and server certificates
REM generated with this method have it present and set to TRUE. This still makes them suitable for
REM use in tests, though.

REM If you have an incomplete run of this script, you may end up with old certificates still
REM in your store. Either use the "certmgr.msc" to manually delete them from the Personal store,
REM or run the "certutil -delstore" commands near the end of this script to clean up.

if exist cacert.pfx del cacert.pfx
if exist clicert.pfx del clicert.pfx
if exist srvcert.pfx del srvcert.pfx

echo.
echo.
echo Creating CA certificate and key pair.
echo.
certreq -new CertReq-CA.inf CertReq-CA.req
del CertReq-CA.req

echo.
echo Creating server certificate and key pair.
echo.
certreq -new CertReq-Server.inf CertReq-Server.req
del CertReq-Server.req

echo.
echo.

echo Creating client certificate and key pair.
echo.
echo You will first be prompted to select a certificate from a list of one. Select
echo AllJoynTestSelfSignedName.
echo It may take some time for this list to appear. Please wait.
echo.
certreq -new -cert AllJoynTestSelfSignedName CertReq-Client.inf CertReq-Client.req
del CertReq-Client.req

echo.
echo.
echo Now exporting the CA and client certificates and keys to PKCS#12 (PFX)
echo containers.
echo.
echo Exporting CA certificate and key pair. Please wait.
certutil -exportpfx -user -p "12345678" my AllJoynTestSelfSignedName cacert.pfx
echo.
echo Exporting client certificate and key pair. Please wait.
certutil -exportpfx -user -p "12345678" my AllJoynTestClientName clicert.pfx
echo.
echo Exporting server certificate and key pair. Please wait.
certutil -exportpfx -user -p "12345678" my AllJoynTestServerName srvcert.pfx

echo.

echo.
echo Cleaning up client certificate. Please wait.
certutil -delstore -user my AllJoynTestClientName
echo Cleaning up server certificate. Please wait.
certutil -delstore -user my AllJoynTestServerName
echo Cleaning up CA certificate. Please wait.
certutil -delstore -user my AllJoynTestSelfSignedName

echo.
echo.
echo Done! PFX files generated and certificate store cleaned up.
echo.
echo At this point, on a system with OpenSSL available, run the "pfx2pem.cmd" script
echo (for Windows) or the "pfx2pem.sh" script (for *NIX) in this directory with the
echo pfx files present. This will extract the CA certificate from cacert.pfx and
echo extract the certificate and private key from clicert.pfx.