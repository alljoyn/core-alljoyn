/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/
#include <gtest/gtest.h>

#include <iostream>
#include <qcc/time.h>
#include <qcc/Thread.h>
#include <qcc/Crypto.h>
#include <qcc/CertificateECC.h>
#include <qcc/CertificateHelper.h>
#include <qcc/GUID.h>
#include <qcc/StringUtil.h>

using namespace qcc;
using namespace std;


#define AUTH_VERIFIER_LEN  Crypto_SHA256::DIGEST_SIZE

/* Test certificates and key generated using OpenSSL. */
static const char eccPrivateKeyPEMOpenSSL[] = {
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHcCAQEEICkeoQeosiS380hFJYo9zL1ziyTbea1mYqqqgHvGKZ6qoAoGCCqGSM49\n"
    "AwEHoUQDQgAE9jiMexU/7Z55ZQQU67Rn/MpXzAkYx5m6nQt2lWWUvWXYbOOLUBx0\n"
    "Tdw/Gy3Ia1WmLSY5ecyw1CUtHsZxjhrlcg==\n"
    "-----END EC PRIVATE KEY-----"
};

static const char eccSelfSignCertX509PEMOpenSSL[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBsDCCAVagAwIBAgIJAJVJ9/7bbQcWMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
    "IDZkODVjMjkyMjYxM2IzNmUyZWVlZjUyNzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1\n"
    "YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQyY2M1NjAeFw0xNTAyMjYxODAzNDlaFw0x\n"
    "NjAyMjYxODAzNDlaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy\n"
    "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n"
    "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABPY4jHsVP+2eeWUEFOu0Z/zK\n"
    "V8wJGMeZup0LdpVllL1l2Gzji1AcdE3cPxstyGtVpi0mOXnMsNQlLR7GcY4a5XKj\n"
    "DTALMAkGA1UdEwQCMAAwCgYIKoZIzj0EAwIDSAAwRQIhAKrCirrUWNNAO2gFiNTl\n"
    "/ncnbELhDiDq/N43LIpfAfX8AiAKX7h/9nXEerJlthl5gUOa4xV6UjqbZLM6+KH/\n"
    "Hk/Yvw==\n"
    "-----END CERTIFICATE-----"
};

static const char eccCertChainX509PEMOpenSSL[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBtDCCAVmgAwIBAgIJAMlyFqk69v+OMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n"
    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAyMjYyMTUxMjVaFw0x\n"
    "NjAyMjYyMTUxMjVaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy\n"
    "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n"
    "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABL50XeH1/aKcIF1+BJtlIgjL\n"
    "AW32qoQdVOTyQg2WnM/R7pgxM2Ha0jMpksUd+JS9BiVYBBArwU76Whz9m6UyJeqj\n"
    "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDSQAwRgIhAKfmglMgl67L5ALF\n"
    "Z63haubkItTMACY1k4ROC2q7cnVmAiEArvAmcVInOq/U5C1y2XrvJQnAdwSl/Ogr\n"
    "IizUeK0oI5c=\n"
    "-----END CERTIFICATE-----"
    "\n"
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBszCCAVmgAwIBAgIJAILNujb37gH2MAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n"
    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAyMjYyMTUxMjNaFw0x\n"
    "NjAyMjYyMTUxMjNaMFYxKTAnBgNVBAsMIDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBm\n"
    "NzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5\n"
    "ZGQwMjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABGEkAUATvOE4uYmt/10vkTcU\n"
    "SA0C+YqHQ+fjzRASOHWIXBvpPiKgHcINtNFQsyX92L2tMT2Kn53zu+3S6UAwy6yj\n"
    "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDSAAwRQIgKit5yeq1uxTvdFmW\n"
    "LDeoxerqC1VqBrmyEvbp4oJfamsCIQDvMTmulW/Br/gY7GOP9H/4/BIEoR7UeAYS\n"
    "4xLyu+7OEA==\n"
    "-----END CERTIFICATE-----"
};

static const char eccSelfSignCertX509OpenSSLWithAKI[] = {
    "-----BEGIN CERTIFICATE-----"
    "MIIB8jCCAZmgAwIBAgIJAOqx8nlKVPYhMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM"
    "IDZkODVjMjkyMjYxM2IzNmUyZWVlZjUyNzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1"
    "YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQyY2M1NjAeFw0xNTA0MTMxODQwMTlaFw0x"
    "NjA0MTIxODQwMTlaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy"
    "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy"
    "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABKW5toeTLEDeljEq75+gKfsV"
    "KpE85OQRVqo0du3RJ0cnL4CLr1HtP/aTWh2RhsJNmoqcpfEoQFI89kOl7BrYW+Oj"
    "UDBOMAwGA1UdEwQFMAMBAf8wHQYDVR0OBBYEFCLQgYCl00ixece0y65rCxZ/BojH"
    "MB8GA1UdIwQYMBaAFCLQgYCl00ixece0y65rCxZ/BojHMAoGCCqGSM49BAMCA0cA"
    "MEQCIAS2B9w8zUQnx8TLGKsA8+m9jw27aU6XGBE0+YHVas9GAiB1gJ37urQfquk3"
    "JIQIok1Np4BGxPZznDmFjvZbG3Hc4Q=="
    "-----END CERTIFICATE-----"
};

static const char eccUnsupportedFormatPrivateKeyPEM[] = {
    "-----BEGIN PRIVATE KEY-----\n"
    "MHcCAQEEICkeoQeosiS380hFJYo9zL1ziyTbea1mYqqqgHvGKZ6qoAoGCCqGSM49\n"
    "AwEHoUQDQgAE9jiMexU/7Z55ZQQU67Rn/MpXzAkYx5m6nQt2lWWUvWXYbOOLUBx0\n"
    "Tdw/Gy3Ia1WmLSY5ecyw1CUtHsZxjhrlcg==\n"
    "-----END PRIVATE KEY-----"
};

static const char eccBadFormatCertChainX509PEM[] = {
    "-----BEGIN CERT-----\n"
    "MIIBtDCCAVmgAwIBAgIJAMlyFqk69v+OMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n"
    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAyMjYyMTUxMjVaFw0x\n"
    "NjAyMjYyMTUxMjVaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy\n"
    "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n"
    "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABL50XeH1/aKcIF1+BJtlIgjL\n"
    "AW32qoQdVOTyQg2WnM/R7pgxM2Ha0jMpksUd+JS9BiVYBBArwU76Whz9m6UyJeqj\n"
    "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDSQAwRgIhAKfmglMgl67L5ALF\n"
    "Z63haubkItTMACY1k4ROC2q7cnVmAiEArvAmcVInOq/U5C1y2XrvJQnAdwSl/Ogr\n"
    "IizUeK0oI5c=\n"
    "-----END CERTIFICATE-----"
    "\n"
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBszCCAVmgAwIBAgIJAILNujb37gH2MAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n"
    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAyMjYyMTUxMjNaFw0x\n"
    "NjAyMjYyMTUxMjNaMFYxKTAnBgNVBAsMIDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBm\n"
    "NzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5\n"
    "ZGQwMjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABGEkAUATvOE4uYmt/10vkTcU\n"
    "SA0C+YqHQ+fjzRASOHWIXBvpPiKgHcINtNFQsyX92L2tMT2Kn53zu+3S6UAwy6yj\n"
    "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDSAAwRQIgKit5yeq1uxTvdFmW\n"
    "LDeoxerqC1VqBrmyEvbp4oJfamsCIQDvMTmulW/Br/gY7GOP9H/4/BIEoR7UeAYS\n"
    "4xLyu+7OEA==\n"
    "-----END CERTIFICATE-----"
};

static const char eccCertChainWithUnknownCACertPEM[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBtDCCAVmgAwIBAgIJAMlyFqk69v+OMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n"
    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAyMjYyMTUxMjVaFw0x\n"
    "NjAyMjYyMTUxMjVaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy\n"
    "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n"
    "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABL50XeH1/aKcIF1+BJtlIgjL\n"
    "AW32qoQdVOTyQg2WnM/R7pgxM2Ha0jMpksUd+JS9BiVYBBArwU76Whz9m6UyJeqj\n"
    "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDSQAwRgIhAKfmglMgl67L5ALF\n"
    "Z63haubkItTMACY1k4ROC2q7cnVmAiEArvAmcVInOq/U5C1y2XrvJQnAdwSl/Ogr\n"
    "IizUeK0oI5c=\n"
    "-----END CERTIFICATE-----"
    "\n"
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBszCCAVmgAwIBAgIJAMPSLBBoNwQIMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n"
    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAzMjQxNzA0MTlaFw0x\n"
    "NjAzMjMxNzA0MTlaMFYxKTAnBgNVBAsMIDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBm\n"
    "NzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5\n"
    "ZGQwMjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABOZknbv1si4H58TcDniPnlKm\n"
    "zxR2xVh1VsZ7anvgSNlxzsiF/Y7qRXeE3G+3sBFjPhrWG63DZuGn96Y+u7qTbcCj\n"
    "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDSAAwRQIgL7NAi2iY0fHaFtIC\n"
    "d58shzZcoR8IMN3uZ1r+9UFboP8CIQDca5XNPYXn+IezASVqdGfs6KodmVIFK2IO\n"
    "vAx+KmwF4Q==\n"
    "-----END CERTIFICATE-----"
};

static const char badEncodedSelfSignCertX509PEM[] = {
    "-----BEGIN CERTIFCATE-----\n"
    "MIIBsDCCAVagAwIBAgIJAJVJ9/7bbQcWMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
    "IDZkODVjMjkyMjYxM2IzNmUyZWVlZjUyNzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1\n"
    "YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQyY2M1NjAeFw0xNTAyMjYxODAzNDlaFw0x\n"
    "NjAyMjYxODAzNDlaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy\n"
    "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n"
    "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABPY4jHsVP+2eeWUEFOu0Z/zK\n"
    "V8wJGMeZup0LdpVllL1l2Gzji1AcdE3cPxstyGtVpi0mOXnMsNQlLR7GcY4a5XKj\n"
    "DTALMAkGA1UdEwQCMAAwCgYIKoZIzj0EAwIDSAAwRQIhAKrCirrUWNNAO2gFiNTl\n"
    "/ncnbELhDiDq/N43LIpfAfX8AiAKX7h/9nXEerJlthl5gUOa4xV6UjqbZLM6+KH/\n"
    "Hk/Yvw==\n"
    "-----END CERTIFICATE-----"
};

static const char eccSelfSignCertX509PEMWithExtraDNFields[] = {
    "-----BEGIN CERTIFICATE-----"
    "MIICazCCAhGgAwIBAgIJAOOAnGuLVcGyMAoGCCqGSM49BAMCMIGsMQswCQYDVQQG"
    "EwJVUzETMBEGA1UECAwKV2FzaGluZ3RvbjEQMA4GA1UEBwwHU2VhdHRsZTEaMBgG"
    "A1UECgwRU29tZSBDb21wYW55IEluYy4xFjAUBgNVBAsMDVNvbWUgRGl2aXNpb24x"
    "ETAPBgNVBAMMCFNvbWVib2R5MS8wLQYJKoZIhvcNAQkBFiBzb21lYm9keUBpbnRo"
    "ZXdob2xld2lkZXdvcmxkLm5ldDAeFw0xNTAzMjgwMTEzNDlaFw0yNTAzMjUwMTEz"
    "NDlaMIGsMQswCQYDVQQGEwJVUzETMBEGA1UECAwKV2FzaGluZ3RvbjEQMA4GA1UE"
    "BwwHU2VhdHRsZTEaMBgGA1UECgwRU29tZSBDb21wYW55IEluYy4xFjAUBgNVBAsM"
    "DVNvbWUgRGl2aXNpb24xETAPBgNVBAMMCFNvbWVib2R5MS8wLQYJKoZIhvcNAQkB"
    "FiBzb21lYm9keUBpbnRoZXdob2xld2lkZXdvcmxkLm5ldDBZMBMGByqGSM49AgEG"
    "CCqGSM49AwEHA0IABMW812QeZ0ntKD3I56m+gBab5s3CcdBGB4YdWkWaAevSY7FL"
    "U8fh9OGNMODnnTBGQemb7jCDdROtL7ef7ELlpn6jGjAYMAkGA1UdEwQCMAAwCwYD"
    "VR0PBAQDAgXgMAoGCCqGSM49BAMCA0gAMEUCIGFVfyaBn0EHd2xvHyjiiRhKqNw7"
    "yg04SMQGWZApN7J+AiEA29ziTHnZk9JKF+CS/b7LQSGWynjqBzh1XMnr0M9ZsJk="
    "-----END CERTIFICATE-----"
};

/**
 * Test certificates and key generated using Windows Crypto APIs
 * (CNG and CAPI2).
 */
static const char eccSelfSignCertX509PEMCAPI[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBUzCB+aADAgECAhDZ3jYhefXsu0VtIVMGHiOiMAoGCCqGSM49BAMCMCQxIjAg\n"
    "BgNVBAMMGUFsbEpveW5UZXN0U2VsZlNpZ25lZE5hbWUwHhcNMTUwMzMxMTg1NDQy\n"
    "WhcNMTYwMzMwMTg1NDQyWjAkMSIwIAYDVQQDDBlBbGxKb3luVGVzdFNlbGZTaWdu\n"
    "ZWROYW1lMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAESOnRO0dXA7FFv4vJJXH8\n"
    "8JBgvSvd1fg9NKosmkvAGYm5CLDBeLIhOycMVCcS2n0Q7mv+kUK+UXQbbg92exQ2\n"
    "PqMNMAswCQYDVR0TBAIwADAKBggqhkjOPQQDAgNJADBGAiEAoV1uqzHvTVkOYLYl\n"
    "QzRSg597ybtDqGoy5L6FgI7Qw5ECIQCrO+fxzcX2mMkYOX+g5gDmHurNKWKkSBnJ\n"
    "wUq30brBfQ==\n"
    "-----END CERTIFICATE-----\n"
};

static const char eccPrivateKeyPEMCAPI[] = {
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHcCAQEEIGjHhBsf1tL/qT2pVToR9SIJt6xKshX2N+svfXtDeCCooAoGCCqGSM49\n"
    "AwEHoUQDQgAESOnRO0dXA7FFv4vJJXH88JBgvSvd1fg9NKosmkvAGYm5CLDBeLIh\n"
    "OycMVCcS2n0Q7mv+kUK+UXQbbg92exQ2Pg==\n"
    "-----END EC PRIVATE KEY-----\n"
};

/* Certificate chains in AllJoyn must start with the EE cert. */
static const char eccCertChainX509PEMCAPI[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBRjCB7qADAgECAhA4NHiS/771skQq7enlPEsyMAoGCCqGSM49BAMCMB4xHDAa\n"
    "BgNVBAMME0FsbEpveW5UZXN0Um9vdE5hbWUwHhcNMTUwMzMxMjI0NjE1WhcNMTYw\n"
    "MzMwMjI0NjE1WjAcMRowGAYDVQQDDBFDZXJ0U2lnbkxpYkNsaWVudDBZMBMGByqG\n"
    "SM49AgEGCCqGSM49AwEHA0IABALDpAM6f0USoGm2vEaBBKr3dJdO9dIRukEUnTUV\n"
    "0fKWN7N0hyIx/ZdANrtVJn8ZrzWnHuEkECEnYZy6hz1QC4ejEDAOMAwGA1UdEwQF\n"
    "MAMBAf8wCgYIKoZIzj0EAwIDRwAwRAIgZT+K9SH5KnZEqvXUf/mOnJ8y0cvCaxzQ\n"
    "9L+/V/1L/o0CIFGqG58zW7QealLNE7Z4dUjZgu0brTvRJDTJKAz7QreR\n"
    "-----END CERTIFICATE-----\n"
    "\n"
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBRzCB7aADAgECAhDPaRHibuWiokAyJhlS20g+MAoGCCqGSM49BAMCMB4xHDAa\n"
    "BgNVBAMME0FsbEpveW5UZXN0Um9vdE5hbWUwHhcNMTUwMzMxMjI0NjE1WhcNMTYw\n"
    "MzMwMjI0NjE1WjAeMRwwGgYDVQQDDBNBbGxKb3luVGVzdFJvb3ROYW1lMFkwEwYH\n"
    "KoZIzj0CAQYIKoZIzj0DAQcDQgAETXyIMsSx5xmNQE+fPtUa8NjqtP3h/c+kXjpo\n"
    "XpApKcBocQ0tzXinzDWzg/GsJS9WCC+QHgJOL3BIiFFv4l1pHaMNMAswCQYDVR0T\n"
    "BAIwADAKBggqhkjOPQQDAgNJADBGAiEA4NZuQGv/Je51gfuNq1M+EnvVnUq0XocV\n"
    "C9rrhWhxSroCIQD6Sam3NVqhHis9ZsK7LwAzI9a7YOj5BqlDPW03adBdgg==\n"
    "-----END CERTIFICATE-----\n"
};

/* Bad certificate had a signature len of zero. */
static const char badCertX509PEMSignatureLenZero[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBtDCCAVmgAwIBAgIJAMlyFqk69v+OMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n"
    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAyMjYyMTUxMjVaFw0x\n"
    "NjAyMjYyMTUxMjVaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy\n"
    "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n"
    "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABL50XeH1/aKcIF1+BJtlIgjL\n"
    "AW32qoQdVOTyQg2WnM/R7pgxM2Ha0jMpksUd+JS9BiVYBBArwU76Whz9m6UyJeqj\n"
    "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDAAAwRgIhAKfmglMgl67L5ALF\n"
    "Z63haubkItTMACY1k4ROC2q7cnVmAiEArvAmcVInOq/U5C1y2XrvJQnAdwSl/Ogr\n"
    "IizUeK0oI5c=\n"
    "-----END CERTIFICATE-----"
};

class CertificateECCTest : public testing::Test {
  public:
    Crypto_ECC ecc;
    virtual void SetUp() {
        QStatus status;

        status = ecc.GenerateDSAKeyPair();
        ASSERT_EQ(ER_OK, status) << " ecc.GenerateDSAKeyPair() failed with actual status: " << QCC_StatusText(status);

        status = ecc.GenerateDHKeyPair();
        ASSERT_EQ(ER_OK, status) << " ecc.GenerateDHKeyPair() failed with actual status: " << QCC_StatusText(status);
    }

  private:


};

static QStatus CreateCert(const qcc::String& serial, const qcc::GUID128& issuer, const qcc::String& organization, const ECCPrivateKey* issuerPrivateKey, const ECCPublicKey* issuerPublicKey, const qcc::GUID128& subject, const ECCPublicKey* subjectPubKey, CertificateX509::ValidPeriod& validity, CertificateX509& x509)
{
    QStatus status = ER_CRYPTO_ERROR;

    x509.SetSerial((const uint8_t*)serial.data(), serial.size());
    qcc::String issuerName = issuer.ToString();
    x509.SetIssuerCN((const uint8_t*) issuerName.c_str(), issuerName.length());
    qcc::String subjectName = subject.ToString();
    x509.SetSubjectCN((const uint8_t*) subjectName.c_str(), subjectName.length());
    if (!organization.empty()) {
        x509.SetIssuerOU((const uint8_t*) organization.c_str(), organization.length());
        x509.SetSubjectOU((const uint8_t*) organization.c_str(), organization.length());
    }
    x509.SetSubjectPublicKey(subjectPubKey);
    x509.SetCA(true);
    x509.SetValidity(&validity);
    status = x509.SignAndGenerateAuthorityKeyId(issuerPrivateKey, issuerPublicKey);
    return status;
}

static QStatus GenKeyAndCreateCert(qcc::GUID128& issuer, const qcc::String& serial, const qcc::String& organization, ECCPrivateKey* dsaPrivateKey, ECCPublicKey* dsaPublicKey, ECCPrivateKey* subjectPrivateKey, ECCPublicKey* subjectPublicKey, bool selfSign, CertificateX509::ValidPeriod& validity, CertificateX509& x509)
{
    Crypto_ECC ecc;
    ecc.GenerateDSAKeyPair();
    *dsaPrivateKey = *ecc.GetDSAPrivateKey();
    *dsaPublicKey = *ecc.GetDSAPublicKey();
    if (!selfSign) {
        ecc.GenerateDSAKeyPair();
    }
    *subjectPrivateKey = *ecc.GetDSAPrivateKey();
    *subjectPublicKey = *ecc.GetDSAPublicKey();
    qcc::GUID128 userGuid;
    return CreateCert(serial, issuer, organization, dsaPrivateKey, dsaPublicKey, userGuid, subjectPublicKey, validity, x509);
}

static QStatus GenKeyAndCreateCert(qcc::GUID128& issuer, const qcc::String& serial, const qcc::String& organization, ECCPrivateKey* dsaPrivateKey, ECCPublicKey* dsaPublicKey, ECCPrivateKey* subjectPrivateKey, ECCPublicKey* subjectPublicKey, bool selfSign, uint32_t expiredInSeconds, CertificateX509& x509)
{
    CertificateX509::ValidPeriod validity;
    validity.validFrom = qcc::GetEpochTimestamp() / 1000;
    validity.validTo = validity.validFrom + expiredInSeconds;
    return GenKeyAndCreateCert(issuer, serial, organization, dsaPrivateKey, dsaPublicKey, subjectPrivateKey, subjectPublicKey, selfSign, validity, x509);
}

TEST_F(CertificateECCTest, EncodePrivateKey)
{
    QStatus status;

    qcc::String encoded;
    status = CertificateX509::EncodePrivateKeyPEM(ecc.GetDSAPrivateKey(), encoded);
    ASSERT_EQ(ER_OK, status) << " CertificateX509::EncodePrivateKeyPEM failed with actual status: " << QCC_StatusText(status);

    printf("The encoded private key PEM %s\n", encoded.c_str());

    ECCPrivateKey pk;
    status = CertificateX509::DecodePrivateKeyPEM(encoded, &pk);
    ASSERT_EQ(ER_OK, status) << " CertificateX509::DecodePrivateKeyPEM failed with actual status: " << QCC_StatusText(status);

    printf("Original private key %s\n", ecc.GetDSAPrivateKey()->ToString().c_str());
    printf("Decoded private key %s\n", pk.ToString().c_str());
    ASSERT_EQ(pk, *ecc.GetDSAPrivateKey()) << " decoded private key not equal to original";
}

/**
 * Test decoding a private key created externally (in PKCS#8 DER format).
 */
static void DecodePrivateKeyHelper(const char* eccPrivateKeyPEM)
{
    QStatus status;

    qcc::String encoded(eccPrivateKeyPEM);

    ECCPrivateKey pk;
    status = CertificateX509::DecodePrivateKeyPEM(encoded, &pk);
    ASSERT_EQ(ER_OK, status) << " CertificateX509::DecodePrivateKeyPEM failed with actual status: " << QCC_StatusText(status);
    printf("Decoded private key %s\n", pk.ToString().c_str());
}
TEST_F(CertificateECCTest, DecodePrivateKey)
{
    DecodePrivateKeyHelper(eccPrivateKeyPEMOpenSSL);
    DecodePrivateKeyHelper(eccPrivateKeyPEMCAPI);
}

TEST_F(CertificateECCTest, DecodeUnsupportedFormatPrivateKeyPEM)
{
    qcc::String encoded(eccUnsupportedFormatPrivateKeyPEM);
    ECCPrivateKey pk;
    ASSERT_NE(ER_OK, CertificateX509::DecodePrivateKeyPEM(encoded, &pk)) << " CertificateX509::DecodePrivateKeyPEM did not fail";
}

/**
 * Test encoding and decoding of public keys.
 */
TEST_F(CertificateECCTest, EncodePublicKey)
{
    QStatus status;

    qcc::String encoded;
    status = CertificateX509::EncodePublicKeyPEM(ecc.GetDSAPublicKey(), encoded);
    ASSERT_EQ(ER_OK, status) << " CertificateX509::EncodePublicKeyPEM failed with actual status: " << QCC_StatusText(status);

    printf("The encoded public key PEM %s\n", encoded.c_str());

    ECCPublicKey pk;
    status = CertificateX509::DecodePublicKeyPEM(encoded, &pk);
    ASSERT_EQ(ER_OK, status) << " CertificateX509::DecodePublicKeyPEM failed with actual status: " << QCC_StatusText(status);

    printf("Original public key %s\n", ecc.GetDSAPublicKey()->ToString().c_str());
    printf("Decoded public key %s\n", pk.ToString().c_str());
    ASSERT_EQ(pk, *ecc.GetDSAPublicKey()) << " decoded private key not equal to original";
}

/**
 * Use the encoded texts to put in the bbservice and bbclient files
 */
TEST_F(CertificateECCTest, GenSelfSignECCX509CertForBBservice)
{
    qcc::GUID128 issuer;
    ECCPrivateKey dsaPrivateKey;
    ECCPublicKey dsaPublicKey;
    ECCPrivateKey subjectPrivateKey;
    ECCPublicKey subjectPublicKey;
    CertificateX509 x509;

    /* cert expires in one year */
    QStatus status = GenKeyAndCreateCert(issuer, "1010101", "organization", &dsaPrivateKey, &dsaPublicKey, &subjectPrivateKey, &subjectPublicKey, true, 365 * 24 * 3600, x509);
    ASSERT_EQ(ER_OK, status) << " GenKeyAndCreateCert failed with actual status: " << QCC_StatusText(status);

    String encodedPK;
    status = CertificateX509::EncodePrivateKeyPEM(&subjectPrivateKey, encodedPK);
    ASSERT_EQ(ER_OK, status) << " CertificateX509::EncodePrivateKeyPEM failed with actual status: " << QCC_StatusText(status);

    std::cout << "The encoded subject private key PEM:" << endl << encodedPK.c_str() << endl;

    status = CertificateX509::EncodePublicKeyPEM(&subjectPublicKey, encodedPK);
    ASSERT_EQ(ER_OK, status) << " CertificateX509::EncodePublicKeyPEM failed with actual status: " << QCC_StatusText(status);

    std::cout << "The encoded subject public key PEM:" << endl << encodedPK.c_str() << endl;

    status = x509.Verify(&dsaPublicKey);
    ASSERT_EQ(ER_OK, status) << " verify cert failed with actual status: " << QCC_StatusText(status);
    String pem = x509.GetPEM();
    std::cout << "The encoded cert PEM for ECC X.509 cert:" << endl << pem.c_str() << endl;

    std::cout << "The ECC X.509 cert: " << endl << x509.ToString().c_str() << endl;

}


/**
 * Test expiry date for X509 cert
 */
TEST_F(CertificateECCTest, ExpiredX509Cert)
{
    qcc::GUID128 issuer;
    ECCPrivateKey dsaPrivateKey;
    ECCPublicKey dsaPublicKey;
    ECCPrivateKey subjectPrivateKey;
    ECCPublicKey subjectPublicKey;
    CertificateX509 x509;
    qcc::String noOrg;

    /* cert expires in two seconds */
    QStatus status = GenKeyAndCreateCert(issuer, "1010101", noOrg, &dsaPrivateKey, &dsaPublicKey, &subjectPrivateKey, &subjectPublicKey, true, 2, x509);
    ASSERT_EQ(ER_OK, status) << " GenKeyAndCreateCert failed with actual status: " << QCC_StatusText(status);

    /* verify that the cert is not yet expired */
    status = x509.VerifyValidity();
    ASSERT_EQ(ER_OK, status) << " verify validity failed with actual status: " << QCC_StatusText(status);

    /* sleep for 3 seconds to wait for the cert expires */
    qcc::Sleep(3000);
    status = x509.VerifyValidity();
    ASSERT_NE(ER_OK, status) << " verify validity did not fail with actual status: " << QCC_StatusText(status);
}

/**
 * Generate certificate with expiry date past the year 2050
 */
TEST_F(CertificateECCTest, X509CertExpiresBeyond2050)
{
    qcc::GUID128 issuer;
    ECCPrivateKey dsaPrivateKey;
    ECCPublicKey dsaPublicKey;
    ECCPrivateKey subjectPrivateKey;
    ECCPublicKey subjectPublicKey;
    CertificateX509 cert;

    /* cert expires in about 36 years */
    uint32_t expiredInSecs = 36 * 365 * 24 * 60 * 60;
    QStatus status = GenKeyAndCreateCert(issuer, "1010101", "organization", &dsaPrivateKey, &dsaPublicKey, &subjectPrivateKey, &subjectPublicKey, true, expiredInSecs, cert);
    ASSERT_EQ(ER_OK, status) << " GenKeyAndCreateCert failed with actual status: " << QCC_StatusText(status);
    status = cert.Verify(&dsaPublicKey);
    ASSERT_EQ(ER_OK, status) << " verify cert failed with actual status: " << QCC_StatusText(status);
}

/**
 * Verify X509 self signed certificates generated by an external tool.
 */
static void VerifyX509SelfSignExternalCertHelper(const char* eccSelfSignCertX509PEM)
{
    CertificateX509 cert;
    String pem(eccSelfSignCertX509PEM);
    QStatus status = cert.LoadPEM(pem);
    ASSERT_EQ(ER_OK, status) << " load external cert PEM failed with actual status: " << QCC_StatusText(status);
    status = cert.Verify();
    ASSERT_EQ(ER_OK, status) << " verify cert failed with actual status: " << QCC_StatusText(status);
    printf("cert: %s\n", cert.ToString().c_str());
}

TEST_F(CertificateECCTest, VerifyX509SelfSignExternalCert)
{
    VerifyX509SelfSignExternalCertHelper(eccSelfSignCertX509PEMOpenSSL);
    VerifyX509SelfSignExternalCertHelper(eccSelfSignCertX509PEMCAPI);
    VerifyX509SelfSignExternalCertHelper(eccSelfSignCertX509OpenSSLWithAKI);
}

/**
 * test a bad cert with zero length signature
 */
TEST_F(CertificateECCTest, BadCertDoesNotLoad)
{
    CertificateX509 cert;
    String pem(badCertX509PEMSignatureLenZero);
    ASSERT_NE(ER_OK, cert.LoadPEM(pem)) << " load external cert PEM did not fail";
}

/**
 * Verify X509 self signed certificates generated by external tools.
 * Add check that they do not verify with a different public key.
 */
static void VerifyX509SelfSignCertPlusDoNotVerifyWithOtherKeyHelper(const char* eccSelfSignCertX509PEM, const ECCPublicKey* otherPublic)
{
    CertificateX509 cert;
    String pem(eccSelfSignCertX509PEM);
    QStatus status = cert.LoadPEM(pem);
    ASSERT_EQ(ER_OK, status) << " load external cert PEM failed with actual status: " << QCC_StatusText(status);
    status = cert.Verify();
    ASSERT_EQ(ER_OK, status) << " verify cert failed with actual status: " << QCC_StatusText(status);
    /* now verify with a different public key.  It is expected to fail */
    status = cert.Verify(otherPublic);
    ASSERT_NE(ER_OK, status) << " verify cert did not fail";
}

TEST_F(CertificateECCTest, VerifyX509SelfSignCertPlusDoNotVerifyWithOtherKey)
{
    VerifyX509SelfSignCertPlusDoNotVerifyWithOtherKeyHelper(eccSelfSignCertX509PEMOpenSSL, ecc.GetDSAPublicKey());
    VerifyX509SelfSignCertPlusDoNotVerifyWithOtherKeyHelper(eccSelfSignCertX509PEMCAPI, ecc.GetDSAPublicKey());
}

/**
 * Verify X509 self signed certificates generated by external tools.
 */
static void VerifyX509ExternalCertChainHelper(const char* eccCertChainX509PEM)
{
    String pem(eccCertChainX509PEM);
    /* count how many certs in the chain */
    size_t count = 0;
    QStatus status = CertificateHelper::GetCertCount(pem, &count);
    ASSERT_EQ(ER_OK, status) << " count the number of certs in the chain failed with actual status: " << QCC_StatusText(status);
    ASSERT_EQ((size_t) 2, count) << " expecting two certs in the cert chain";

    CertificateX509 certs[2];
    status = CertificateX509::DecodeCertChainPEM(pem, certs, count);
    ASSERT_EQ(ER_OK, status) << " decode the cert chain failed with actual status: " << QCC_StatusText(status);
    for (size_t cnt = 0; cnt < count; cnt++) {
        printf("certs[%d]: %s\n", (int) cnt, certs[cnt].ToString().c_str());
    }
    status = certs[0].Verify(certs[1].GetSubjectPublicKey());
    ASSERT_EQ(ER_OK, status) << " verify leaf cert failed with actual status: " << QCC_StatusText(status);
}

TEST_F(CertificateECCTest, VerifyX509ExternalCertChain)
{
    VerifyX509ExternalCertChainHelper(eccCertChainX509PEMOpenSSL);
    VerifyX509ExternalCertChainHelper(eccCertChainX509PEMCAPI);
}

/**
 * Generate certificate with start date in the past and expiry date way in the
 * the future.  One can use the PEM file to do spot check the date with openssl.
 */
TEST_F(CertificateECCTest, X509CertWideRangeValidPeriod)
{
    qcc::GUID128 issuer;
    ECCPrivateKey dsaPrivateKey;
    ECCPublicKey dsaPublicKey;
    ECCPrivateKey subjectPrivateKey;
    ECCPublicKey subjectPublicKey;
    CertificateX509 cert;

    CertificateX509::ValidPeriod validity;
    /* cert valid from 1970 */
    validity.validFrom = 0;
    /* cert expires in the year 2035 */
    uint32_t expiredInSecs = 20 * 365 * 24 * 60 * 60;
    validity.validTo = qcc::GetEpochTimestamp() / 1000 + expiredInSecs;

    QStatus status = GenKeyAndCreateCert(issuer, "1010101", "organization", &dsaPrivateKey, &dsaPublicKey, &subjectPrivateKey, &subjectPublicKey, true, validity, cert);
    ASSERT_EQ(ER_OK, status) << " GenKeyAndCreateCert failed with actual status: " << QCC_StatusText(status);
    status = cert.Verify(&dsaPublicKey);
    ASSERT_EQ(ER_OK, status) << " verify cert failed with actual status: " << QCC_StatusText(status);
    String pem = cert.GetPEM();
    std::cout << "The encoded cert PEM for ECC X.509 cert:" << endl << pem.c_str() << endl;
    std::cout << "The cert:" << endl << cert.ToString().c_str() << endl;
}

/**
 * Test load a badly formatted PEM for a certificate chain
 */
TEST_F(CertificateECCTest, BadFormatExternalCertChain)
{
    String pem(eccBadFormatCertChainX509PEM);
    /* count how many certs in the chain */
    size_t count = 0;
    CertificateHelper::GetCertCount(pem, &count);
    ASSERT_NE((size_t) 2, count) << "Did not expect to have two certs in the cert chain";
}

/**
 * Certificate chain with an unknown CA cert.
 */
TEST_F(CertificateECCTest, FailToVerifyCertChainWithUnknownCACert)
{
    String pem(eccCertChainWithUnknownCACertPEM);
    size_t count = 0;
    ASSERT_EQ(ER_OK, CertificateHelper::GetCertCount(pem, &count)) << " count the number of certs in the chain failed";
    ASSERT_EQ((size_t) 2, count) << " expecting two certs in the cert chain";

    CertificateX509 certs[2];
    ASSERT_EQ(ER_OK, CertificateX509::DecodeCertChainPEM(pem, certs, count)) << " decode the cert chain failed";
    ASSERT_NE(ER_OK, certs[0].Verify(certs[1].GetSubjectPublicKey())) << " verify leaf cert did not fail";
}

/**
 * Badly formatted cert PEM should fail to load
 */
TEST_F(CertificateECCTest, FailToLoadBadlyEncodedCertPEM)
{
    CertificateX509 cert;
    String pem(badEncodedSelfSignCertX509PEM);
    ASSERT_NE(ER_OK, cert.LoadPEM(pem)) << "load badly encoded cert PEM did not fail";
}

/**
 * Verify a X509 self signed certificate generated by external tool.
 * This certificate hold extra fields in the distinguished name.
 */
TEST_F(CertificateECCTest, VerifyX509SelfSignExternalCertWithExtraDNFields)
{
    CertificateX509 cert;
    String pem(eccSelfSignCertX509PEMWithExtraDNFields);
    QStatus status = cert.LoadPEM(pem);
    ASSERT_EQ(ER_OK, status) << " load external cert PEM failed with actual status: " << QCC_StatusText(status);
    status = cert.Verify();
    ASSERT_EQ(ER_OK, status) << " verify cert failed with actual status: " << QCC_StatusText(status);
}

/**
 * Create a self signed cert and reload its PEM
 */
TEST_F(CertificateECCTest, GenerateAndLoadSelfSignedCert)
{
    qcc::GUID128 issuer;
    ECCPrivateKey dsaPrivateKey;
    ECCPublicKey dsaPublicKey;
    ECCPrivateKey subjectPrivateKey;
    ECCPublicKey subjectPublicKey;
    CertificateX509 cert;

    /* cert expires in one year */
    ASSERT_EQ(ER_OK, GenKeyAndCreateCert(issuer, "1010101", "organization", &dsaPrivateKey, &dsaPublicKey, &subjectPrivateKey, &subjectPublicKey, true, 365 * 24 * 3600, cert)) << " GenKeyAndCreateCert failed";

    ASSERT_EQ(ER_OK, cert.Verify(&dsaPublicKey)) << " verify cert failed";
    CertificateX509 cert2;
    ASSERT_EQ(ER_OK, cert2.LoadPEM(cert.GetPEM())) << " Error reload cert from PEM";
    ASSERT_EQ(ER_OK, cert2.Verify(&dsaPublicKey)) << " verify cert failed";
}

/**
 * Test a self signed certificate with the basic constraints field marked as
 * critical.
 */
TEST_F(CertificateECCTest, TestSelfSignedCertWithCritialBasicConstraint)
{
    static const char eccSelfSignCertX509PEM[] = {
        "-----BEGIN CERTIFICATE-----\n"
        "MIIBVDCB/KADAgECAhC+Ci4hDqaWuEWj2eDd0zrfMAoGCCqGSM49BAMCMCQxIjAgBgNVBAMMGUFsbEpveW5UZXN0U2VsZlNpZ25lZE5hbWUwHhcNMTUwMzMxMTc0MTQwWhcNMTYwMzMwMTc0MTQwWjAkMSIwIAYDVQQDDBlBbGxKb3luVGVzdFNlbGZTaWduZWROYW1lMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE5nmP2qHqZ6N67jdoVxSA64U+Y+rThK+oAwgR6DNezFKMSgVMA1Snn4qsc1Q+KbaYAMj7hWs6xDUIbz6XTOJBvaMQMA4wDAYDVR0TAQH/BAIwADAKBggqhkjOPQQDAgNHADBEAiBJpmVQof40vG9qjWgBTMkETUT0d1kGADBjQK162bUCygIgAtHmpfRztbtr5hgXYdjx4W3Kw0elmnuIfsvrY86ONZs=\n"
        "-----END CERTIFICATE-----\n"
    };

    VerifyX509SelfSignExternalCertHelper(eccSelfSignCertX509PEM);
}

/**
 * Test a certificate chain with leaf cert containing no CA field and
 * signing cert has pathlen = 0
 */
TEST_F(CertificateECCTest, TestCertChainWithNoCAFieldInBasicContraints)
{
    /* the leaf cert does not contain the CA field */
    static const char eccCertChainX509PEM[] = {
        "-----BEGIN CERTIFICATE-----\n"
        "MIIBRTCB66ADAgECAhAIrQyeRPmaj0tCzYi1kc1LMAoGCCqGSM49BAMCMB4xHDAaBgNVBAMME0FsbEpveW5UZXN0Um9vdE5hbWUwHhcNMTUwMzMxMjMyODU2WhcNMTYwMzMwMjMyODU2WjAcMRowGAYDVQQDDBFDZXJ0U2lnbkxpYkNsaWVudDBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABDrQE+EUBFzwtXq/vlG6IYYEpVxEndizIvaysExCBML5uYovNVLfWEqFmEDGLvv3rJkZ0I0xhzSyzLD+Zo4xzU+jDTALMAkGA1UdEwQCMAAwCgYIKoZIzj0EAwIDSQAwRgIhAJ++iDjgYeje0kmJ3cdYTwen1V92Ldz4m0NInbpPX3BOAiEAvUTLYd83T4uXNh6P+JL4Phj3zxVBo2mSvwnuFSyeSOg=\n"
        "-----END CERTIFICATE-----\n"
        "\n"
        "-----BEGIN CERTIFICATE-----\n"
        "MIIBTDCB86ADAgECAhDNAwko47UUmUcr+HFVMJj1MAoGCCqGSM49BAMCMB4xHDAaBgNVBAMME0FsbEpveW5UZXN0Um9vdE5hbWUwHhcNMTUwMzMxMjMyODU2WhcNMTYwMzMwMjMyODU2WjAeMRwwGgYDVQQDDBNBbGxKb3luVGVzdFJvb3ROYW1lMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEwmq2CF9Q1Lh/RfE9ejHMGb+AkgKljRgh3D2uOVCGCvxpMtH4AR+QzAPKwYOHvKewsZIBtC41N5Fb4wFbR3kaSaMTMBEwDwYDVR0TBAgwBgEB/wIBADAKBggqhkjOPQQDAgNIADBFAiAyIj1kEli20k2jRuhmSqyjHJ1rlv0oyLOXpgI5f5P0nAIhALIV4i9VG6+DiL7VgNQ1LQswZMgjEUMuPWL6UyuBDe3z\n"
        "-----END CERTIFICATE-----\n"
    };

    String pem(eccCertChainX509PEM);
    /* count how many certs in the chain */
    size_t count = 0;
    QStatus status = CertificateHelper::GetCertCount(pem, &count);
    ASSERT_EQ(ER_OK, status) << " count the number of certs in the chain failed with actual status: " << QCC_StatusText(status);
    ASSERT_EQ((size_t) 2, count) << " expecting two certs in the cert chain";

    CertificateX509 certs[2];
    status = CertificateX509::DecodeCertChainPEM(pem, certs, count);
    ASSERT_EQ(ER_OK, status) << " decode the cert chain failed with actual status: " << QCC_StatusText(status);
    status = certs[0].Verify(certs[1].GetSubjectPublicKey());
    ASSERT_EQ(ER_OK, status) << " verify leaf cert failed with actual status: " << QCC_StatusText(status);
}

/**
 * Helper to test validity period.
 */
static void TestValidityPeriod(CertificateX509::ValidPeriod& validity)
{
    qcc::GUID128 issuer;
    ECCPrivateKey dsaPrivateKey;
    ECCPublicKey dsaPublicKey;
    ECCPrivateKey subjectPrivateKey;
    ECCPublicKey subjectPublicKey;
    CertificateX509 cert;


    QStatus status = GenKeyAndCreateCert(issuer, "1010101", "organization", &dsaPrivateKey, &dsaPublicKey, &subjectPrivateKey, &subjectPublicKey, true, validity, cert);
    EXPECT_EQ(ER_OK, status) << " GenKeyAndCreateCert failed with actual status: " << QCC_StatusText(status);
    status = cert.Verify(&dsaPublicKey);
    EXPECT_EQ(ER_OK, status) << " verify cert failed with actual status: " << QCC_StatusText(status);
    String pem = cert.GetPEM();
    CertificateX509 cert2;
    EXPECT_EQ(ER_OK, cert2.LoadPEM(pem)) << " load PEM failed";
    EXPECT_EQ(validity.validFrom, cert2.GetValidity()->validFrom) << "validFrom not the same";
    EXPECT_EQ(validity.validTo, cert2.GetValidity()->validTo) << "validTo not the same";
}

/**
 * Test validity date generation
 */
TEST_F(CertificateECCTest, validityPeriodGeneration)
{
    CertificateX509::ValidPeriod validity;
    validity.validFrom = 1427404154;   /* 150326210914Z a date with day light savings */
    validity.validTo = 1427404154 + 630720000;   /* 350321210914Z */
    TestValidityPeriod(validity);

    validity.validFrom = 1423177645;   /* 150205230725Z a date with no day light savings */
    validity.validTo = validity.validFrom + 630720000;   /* 350131230725Z */
    TestValidityPeriod(validity);
}

TEST_F(CertificateECCTest, SubjectAltNameInExternalGeneratedCert)
{
    /* Cert has identity alias field. Cert was generated using openssl command line */
    IdentityCertificate cert;
    static const char CustomIdentityAliasCert[] = {
        "-----BEGIN CERTIFICATE-----\n"
        "MIIBxjCCAW2gAwIBAgIJAIfm4O/IwDYDMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM"
        "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4"
        "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTA1MTMxNjM2MDdaFw0x"
        "NjA1MTIxNjM2MDdaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy"
        "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy"
        "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABCK3CCj7zDXi3dcXjL/ECUR3"
        "9NLRN9nnNDNlyy2jkdOJvUEje0+ZTRUJR7y8HfP49PgmRHuMQgYLhG52CMxlDZyj"
        "JDAiMCAGA1UdEQQZMBegFQYKKwYBBAGC3nwBBKAHBAVhbGlhczAKBggqhkjOPQQD"
        "AgNHADBEAiBRXDdDAiYPQt7aftVWkcB6q2tWQL6mVGa+r18T6C9wfwIgJbZRDtJ0"
        "szuhC+BaE0r7AfOGUiXBys5CYXYxIMGe8zM="
        "-----END CERTIFICATE-----"
    };
    String pem(CustomIdentityAliasCert);
    EXPECT_EQ(ER_OK, cert.LoadPEM(pem)) << " load external cert PEM failed.";
    EXPECT_EQ(0, memcmp("alias", cert.GetAlias().data(), cert.GetAlias().size())) << " expect to have alias as the subject alt name";

    /* Cert has the security group id field.  Cert was generated using openssl command line */
    static const char CustomMembershipCert[] = {
        "-----BEGIN CERTIFICATE-----\n"
        "MIIB0TCCAXigAwIBAgIJAIfm4O/IwDYGMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM"
        "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4"
        "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTA1MTMxNzU1MTBaFw0x"
        "NjA1MTIxNzU1MTBaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy"
        "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy"
        "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABGTa279SkSAKGSbPTp4BtF1k"
        "CsPHR9dhwjN1moCcp8LMWQi22walAsohxsS+J5Ma1ROSxllZ+70EbFAZkQWsG/6j"
        "LzAtMCsGA1UdEQQkMCKgIAYKKwYBBAGC3nwBA6ASBBBBMTZCeXRlQXJyTm90SGV4"
        "MAoGCCqGSM49BAMCA0cAMEQCIHk/c64FSVd3ODPsa5q19nbSNv3bg0PWN5oo+Vy4"
        "BO/YAiBpoMjlAwfTYfAENq+0jsePNtT5d+au77GviCh8ZdAl/w=="
        "-----END CERTIFICATE-----"
    };
    String pem2(CustomMembershipCert);
    MembershipCertificate cert2;
    EXPECT_EQ(ER_OK, cert2.LoadPEM(pem2)) << " load external cert PEM failed.";
    EXPECT_EQ(0, memcmp("A16ByteArrNotHex", cert2.GetGuild().GetBytes(), qcc::GUID128::SIZE)) << " expect to have A16ByteArrNotHex as the subject alt name";

    /* The subject alt name has an unknown otherName OID. Cert was generated
     * using openssl command line.
     */
    static const char UnknownOtherNameOIDCert[] = {
        "-----BEGIN CERTIFICATE-----\n"
        "MIIB0jCCAXigAwIBAgIJAIfm4O/IwDYHMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM"
        "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4"
        "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTA1MTMxODA3NDJaFw0x"
        "NjA1MTIxODA3NDJaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy"
        "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy"
        "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABMAB1fmU/POy0YxEl/QJJqKy"
        "4YE4RbF97hCKuHyM2EF7x6JsQlDkM0c5rYYBFVbhgTgpTQk5mNNMZznlWxPFWjmj"
        "LzAtMCsGA1UdEQQkMCKgIAYKKwYBBAGC3nwBAKASBBBBMTZCeXRlQXJyTm90SGV4"
        "MAoGCCqGSM49BAMCA0gAMEUCIE4WgW4y4NyXGwXGGAEgd9dTewZcoQ7sp2TJYj/D"
        "H4bPAiEA4JvCcg8Kr/66qyrbKz0wWBv7L8igT9Y9KsJRcKSmYNo="
        "-----END CERTIFICATE-----"
    };
    String pem3(UnknownOtherNameOIDCert);
    CertificateX509 cert3;
    EXPECT_EQ(ER_OK, cert3.LoadPEM(pem3)) << " load external cert PEM failed.";
    EXPECT_TRUE(cert3.GetSubjectAltName().empty()) << " cert subject alt name is supposed to be empty.";
}

TEST_F(CertificateECCTest, SubjectAltNameInIdentityCert)
{
    IdentityCertificate cert;
    cert.SetAlias("alias");
    qcc::GUID128 issuer;
    ECCPrivateKey dsaPrivateKey;
    ECCPublicKey dsaPublicKey;
    ECCPrivateKey subjectPrivateKey;
    ECCPublicKey subjectPublicKey;

    EXPECT_EQ(ER_OK, GenKeyAndCreateCert(issuer, "1010101", "organization", &dsaPrivateKey, &dsaPublicKey, &subjectPrivateKey, &subjectPublicKey, true, 365 * 24 * 3600, cert)) << " GenKeyAndCreateCert failed.";

    String pem = cert.GetPEM();
    IdentityCertificate cert2;
    EXPECT_EQ(ER_OK, cert2.LoadPEM(pem)) << " load cert PEM failed.";
    EXPECT_EQ(0, memcmp("alias", cert2.GetAlias().data(), cert2.GetAlias().size())) << " expect to have alias as the subject alt name";
}

TEST_F(CertificateECCTest, SubjectAltNameInMembershipCert)
{
    MembershipCertificate cert;
    qcc::GUID128 securityGroupId;
    cert.SetGuild(securityGroupId);
    qcc::GUID128 issuer;
    ECCPrivateKey dsaPrivateKey;
    ECCPublicKey dsaPublicKey;
    ECCPrivateKey subjectPrivateKey;
    ECCPublicKey subjectPublicKey;

    EXPECT_EQ(ER_OK, GenKeyAndCreateCert(issuer, "1010101", "organization", &dsaPrivateKey, &dsaPublicKey, &subjectPrivateKey, &subjectPublicKey, true, 365 * 24 * 3600, cert)) << " GenKeyAndCreateCert failed.";

    String pem = cert.GetPEM();
    MembershipCertificate cert2;
    EXPECT_EQ(ER_OK, cert2.LoadPEM(pem)) << " load cert PEM failed.";
    EXPECT_EQ(securityGroupId, cert2.GetGuild()) << " expect to have security group Id as the subject alt name";
}

TEST_F(CertificateECCTest, TestIsDNEqual)
{
    CertificateX509 cert1, cert2;

    ASSERT_EQ(ER_OK, cert1.LoadPEM(eccSelfSignCertX509PEMOpenSSL));
    ASSERT_EQ(ER_OK, cert2.LoadPEM(eccSelfSignCertX509PEMCAPI));

    EXPECT_TRUE(cert1.IsDNEqual(cert1));
    EXPECT_TRUE(cert1.IsDNEqual(cert1.GetSubjectCN(), cert1.GetSubjectCNLength(), cert1.GetSubjectOU(), cert1.GetSubjectOULength()));

    EXPECT_FALSE(cert1.IsDNEqual(cert2));
    EXPECT_FALSE(cert1.IsDNEqual(cert2.GetSubjectCN(), cert2.GetSubjectCNLength(), cert2.GetSubjectOU(), cert2.GetSubjectOULength()));
}

TEST_F(CertificateECCTest, TestIsIssuerOf)
{
    CertificateX509 cert1, cert2;

    ASSERT_EQ(ER_OK, cert1.LoadPEM(eccSelfSignCertX509PEMOpenSSL));
    ASSERT_EQ(ER_OK, cert2.LoadPEM(eccSelfSignCertX509PEMCAPI));

    EXPECT_TRUE(cert1.IsIssuerOf(cert1));
    EXPECT_TRUE(cert2.IsIssuerOf(cert2));
    EXPECT_FALSE(cert1.IsIssuerOf(cert2));
    EXPECT_FALSE(cert2.IsIssuerOf(cert1));
}
