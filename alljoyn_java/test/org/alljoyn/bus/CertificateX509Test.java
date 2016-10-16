/*
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
 */
package org.alljoyn.bus;

import junit.framework.TestCase;
import java.util.Arrays;
import java.util.UUID;
import java.nio.ByteBuffer;
import java.lang.StringBuilder;

import org.alljoyn.bus.common.ECCPrivateKey;
import org.alljoyn.bus.common.CertificateX509;
import org.alljoyn.bus.common.CryptoECC;
import org.alljoyn.bus.common.ECCPublicKey;

public class CertificateX509Test extends TestCase {

    static {
        System.loadLibrary("alljoyn_java");
    }

    /* Test certificates and key generated using OpenSSL. */
    public static final String eccPrivateKeyPEMOpenSSL =
    "-----BEGIN EC PRIVATE KEY-----\n" +
    "MHcCAQEEICkeoQeosiS380hFJYo9zL1ziyTbea1mYqqqgHvGKZ6qoAoGCCqGSM49\n" +
    "AwEHoUQDQgAE9jiMexU/7Z55ZQQU67Rn/MpXzAkYx5m6nQt2lWWUvWXYbOOLUBx0\n" +
    "Tdw/Gy3Ia1WmLSY5ecyw1CUtHsZxjhrlcg==\n" +
    "-----END EC PRIVATE KEY-----";

    public static final String eccSelfSignCertX509PEMOpenSSL =
    "-----BEGIN CERTIFICATE-----\n" +
    "MIIBsDCCAVagAwIBAgIJAJVJ9/7bbQcWMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n" +
    "IDZkODVjMjkyMjYxM2IzNmUyZWVlZjUyNzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1\n" +
    "YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQyY2M1NjAeFw0xNTAyMjYxODAzNDlaFw0x\n" +
    "NjAyMjYxODAzNDlaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy\n" +
    "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n" +
    "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABPY4jHsVP+2eeWUEFOu0Z/zK\n" +
    "V8wJGMeZup0LdpVllL1l2Gzji1AcdE3cPxstyGtVpi0mOXnMsNQlLR7GcY4a5XKj\n" +
    "DTALMAkGA1UdEwQCMAAwCgYIKoZIzj0EAwIDSAAwRQIhAKrCirrUWNNAO2gFiNTl\n" +
    "/ncnbELhDiDq/N43LIpfAfX8AiAKX7h/9nXEerJlthl5gUOa4xV6UjqbZLM6+KH/\n" +
    "Hk/Yvw==\n" +
    "-----END CERTIFICATE-----";

    public static final String eccCertChainX509PEMOpenSSL =
    "-----BEGIN CERTIFICATE-----\n" +
    "MIIBtDCCAVmgAwIBAgIJAMlyFqk69v+OMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n" +
    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n" +
    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAyMjYyMTUxMjVaFw0x\n" +
    "NjAyMjYyMTUxMjVaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy\n" +
    "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n" +
    "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABL50XeH1/aKcIF1+BJtlIgjL\n" +
    "AW32qoQdVOTyQg2WnM/R7pgxM2Ha0jMpksUd+JS9BiVYBBArwU76Whz9m6UyJeqj\n" +
    "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDSQAwRgIhAKfmglMgl67L5ALF\n" +
    "Z63haubkItTMACY1k4ROC2q7cnVmAiEArvAmcVInOq/U5C1y2XrvJQnAdwSl/Ogr\n" +
    "IizUeK0oI5c=\n" +
    "-----END CERTIFICATE-----" +
    "\n" +
    "-----BEGIN CERTIFICATE-----\n" +
    "MIIBszCCAVmgAwIBAgIJAILNujb37gH2MAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n" +
    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n" +
    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAyMjYyMTUxMjNaFw0x\n" +
    "NjAyMjYyMTUxMjNaMFYxKTAnBgNVBAsMIDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBm\n" +
    "NzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5\n" +
    "ZGQwMjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABGEkAUATvOE4uYmt/10vkTcU\n" +
    "SA0C+YqHQ+fjzRASOHWIXBvpPiKgHcINtNFQsyX92L2tMT2Kn53zu+3S6UAwy6yj\n" +
    "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDSAAwRQIgKit5yeq1uxTvdFmW\n" +
    "LDeoxerqC1VqBrmyEvbp4oJfamsCIQDvMTmulW/Br/gY7GOP9H/4/BIEoR7UeAYS\n" +
    "4xLyu+7OEA==\n" +
    "-----END CERTIFICATE-----";

    public static final String eccSelfSignCertX509OpenSSLWithAKI =
    "-----BEGIN CERTIFICATE-----" +
    "MIIB8jCCAZmgAwIBAgIJAOqx8nlKVPYhMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM" +
    "IDZkODVjMjkyMjYxM2IzNmUyZWVlZjUyNzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1" +
    "YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQyY2M1NjAeFw0xNTA0MTMxODQwMTlaFw0x" +
    "NjA0MTIxODQwMTlaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy" +
    "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy" +
    "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABKW5toeTLEDeljEq75+gKfsV" +
    "KpE85OQRVqo0du3RJ0cnL4CLr1HtP/aTWh2RhsJNmoqcpfEoQFI89kOl7BrYW+Oj" +
    "UDBOMAwGA1UdEwQFMAMBAf8wHQYDVR0OBBYEFCLQgYCl00ixece0y65rCxZ/BojH" +
    "MB8GA1UdIwQYMBaAFCLQgYCl00ixece0y65rCxZ/BojHMAoGCCqGSM49BAMCA0cA" +
    "MEQCIAS2B9w8zUQnx8TLGKsA8+m9jw27aU6XGBE0+YHVas9GAiB1gJ37urQfquk3" +
    "JIQIok1Np4BGxPZznDmFjvZbG3Hc4Q==" +
    "-----END CERTIFICATE-----";

    public static final String eccUnsupportedFormatPrivateKeyPEM =
    "-----BEGIN PRIVATE KEY-----\n" +
    "MHcCAQEEICkeoQeosiS380hFJYo9zL1ziyTbea1mYqqqgHvGKZ6qoAoGCCqGSM49\n" +
    "AwEHoUQDQgAE9jiMexU/7Z55ZQQU67Rn/MpXzAkYx5m6nQt2lWWUvWXYbOOLUBx0\n" +
    "Tdw/Gy3Ia1WmLSY5ecyw1CUtHsZxjhrlcg==\n" +
    "-----END PRIVATE KEY-----";

    public static final String eccBadFormatCertChainX509PEM =
    "-----BEGIN CERT-----\n" +
    "MIIBtDCCAVmgAwIBAgIJAMlyFqk69v+OMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n" +
    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n" +
    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAyMjYyMTUxMjVaFw0x\n" +
    "NjAyMjYyMTUxMjVaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy\n" +
    "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n" +
    "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABL50XeH1/aKcIF1+BJtlIgjL\n" +
    "AW32qoQdVOTyQg2WnM/R7pgxM2Ha0jMpksUd+JS9BiVYBBArwU76Whz9m6UyJeqj\n" +
    "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDSQAwRgIhAKfmglMgl67L5ALF\n" +
    "Z63haubkItTMACY1k4ROC2q7cnVmAiEArvAmcVInOq/U5C1y2XrvJQnAdwSl/Ogr\n" +
    "IizUeK0oI5c=\n" +
    "-----END CERTIFICATE-----" +
    "\n" +
    "-----BEGIN CERTIFICATE-----\n" +
    "MIIBszCCAVmgAwIBAgIJAILNujb37gH2MAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n" +
    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n" +
    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAyMjYyMTUxMjNaFw0x\n" +
    "NjAyMjYyMTUxMjNaMFYxKTAnBgNVBAsMIDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBm\n" +
    "NzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5\n" +
    "ZGQwMjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABGEkAUATvOE4uYmt/10vkTcU\n" +
    "SA0C+YqHQ+fjzRASOHWIXBvpPiKgHcINtNFQsyX92L2tMT2Kn53zu+3S6UAwy6yj\n" +
    "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDSAAwRQIgKit5yeq1uxTvdFmW\n" +
    "LDeoxerqC1VqBrmyEvbp4oJfamsCIQDvMTmulW/Br/gY7GOP9H/4/BIEoR7UeAYS\n" +
    "4xLyu+7OEA==\n" +
    "-----END CERTIFICATE-----";

    public static final String eccCertChainWithUnknownCACertPEM =
    "-----BEGIN CERTIFICATE-----\n" +
    "MIIBtDCCAVmgAwIBAgIJAMlyFqk69v+OMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n" +
    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n" +
    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAyMjYyMTUxMjVaFw0x\n" +
    "NjAyMjYyMTUxMjVaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy\n" +
    "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n" +
    "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABL50XeH1/aKcIF1+BJtlIgjL\n" +
    "AW32qoQdVOTyQg2WnM/R7pgxM2Ha0jMpksUd+JS9BiVYBBArwU76Whz9m6UyJeqj\n" +
    "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDSQAwRgIhAKfmglMgl67L5ALF\n" +
    "Z63haubkItTMACY1k4ROC2q7cnVmAiEArvAmcVInOq/U5C1y2XrvJQnAdwSl/Ogr\n" +
    "IizUeK0oI5c=\n" +
    "-----END CERTIFICATE-----" +
    "\n" +
    "-----BEGIN CERTIFICATE-----\n" +
    "MIIBszCCAVmgAwIBAgIJAMPSLBBoNwQIMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n" +
    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n" +
    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAzMjQxNzA0MTlaFw0x\n" +
    "NjAzMjMxNzA0MTlaMFYxKTAnBgNVBAsMIDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBm\n" +
    "NzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5\n" +
    "ZGQwMjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABOZknbv1si4H58TcDniPnlKm\n" +
    "zxR2xVh1VsZ7anvgSNlxzsiF/Y7qRXeE3G+3sBFjPhrWG63DZuGn96Y+u7qTbcCj\n" +
    "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDSAAwRQIgL7NAi2iY0fHaFtIC\n" +
    "d58shzZcoR8IMN3uZ1r+9UFboP8CIQDca5XNPYXn+IezASVqdGfs6KodmVIFK2IO\n" +
    "vAx+KmwF4Q==\n" +
    "-----END CERTIFICATE-----";

    public static final String badEncodedSelfSignCertX509PEM =
    "-----BEGIN CERTIFCATE-----\n" +
    "MIIBsDCCAVagAwIBAgIJAJVJ9/7bbQcWMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n" +
    "IDZkODVjMjkyMjYxM2IzNmUyZWVlZjUyNzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1\n" +
    "YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQyY2M1NjAeFw0xNTAyMjYxODAzNDlaFw0x\n" +
    "NjAyMjYxODAzNDlaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy\n" +
    "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n" +
    "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABPY4jHsVP+2eeWUEFOu0Z/zK\n" +
    "V8wJGMeZup0LdpVllL1l2Gzji1AcdE3cPxstyGtVpi0mOXnMsNQlLR7GcY4a5XKj\n" +
    "DTALMAkGA1UdEwQCMAAwCgYIKoZIzj0EAwIDSAAwRQIhAKrCirrUWNNAO2gFiNTl\n" +
    "/ncnbELhDiDq/N43LIpfAfX8AiAKX7h/9nXEerJlthl5gUOa4xV6UjqbZLM6+KH/\n" +
    "Hk/Yvw==\n" +
    "-----END CERTIFICATE-----";

    public static final String eccSelfSignCertX509PEMWithExtraDNFields =
    "-----BEGIN CERTIFICATE-----" +
    "MIICazCCAhGgAwIBAgIJAOOAnGuLVcGyMAoGCCqGSM49BAMCMIGsMQswCQYDVQQG" +
    "EwJVUzETMBEGA1UECAwKV2FzaGluZ3RvbjEQMA4GA1UEBwwHU2VhdHRsZTEaMBgG" +
    "A1UECgwRU29tZSBDb21wYW55IEluYy4xFjAUBgNVBAsMDVNvbWUgRGl2aXNpb24x" +
    "ETAPBgNVBAMMCFNvbWVib2R5MS8wLQYJKoZIhvcNAQkBFiBzb21lYm9keUBpbnRo" +
    "ZXdob2xld2lkZXdvcmxkLm5ldDAeFw0xNTAzMjgwMTEzNDlaFw0yNTAzMjUwMTEz" +
    "NDlaMIGsMQswCQYDVQQGEwJVUzETMBEGA1UECAwKV2FzaGluZ3RvbjEQMA4GA1UE" +
    "BwwHU2VhdHRsZTEaMBgGA1UECgwRU29tZSBDb21wYW55IEluYy4xFjAUBgNVBAsM" +
    "DVNvbWUgRGl2aXNpb24xETAPBgNVBAMMCFNvbWVib2R5MS8wLQYJKoZIhvcNAQkB" +
    "FiBzb21lYm9keUBpbnRoZXdob2xld2lkZXdvcmxkLm5ldDBZMBMGByqGSM49AgEG" +
    "CCqGSM49AwEHA0IABMW812QeZ0ntKD3I56m+gBab5s3CcdBGB4YdWkWaAevSY7FL" +
    "U8fh9OGNMODnnTBGQemb7jCDdROtL7ef7ELlpn6jGjAYMAkGA1UdEwQCMAAwCwYD" +
    "VR0PBAQDAgXgMAoGCCqGSM49BAMCA0gAMEUCIGFVfyaBn0EHd2xvHyjiiRhKqNw7" +
    "yg04SMQGWZApN7J+AiEA29ziTHnZk9JKF+CS/b7LQSGWynjqBzh1XMnr0M9ZsJk=" +
    "-----END CERTIFICATE-----";

    /**
     * Test certificates and key generated using Windows Crypto APIs
     * (CNG and CAPI2).
     */
    public static final String eccSelfSignCertX509PEMCAPI =
    "-----BEGIN CERTIFICATE-----\n" +
    "MIIBUzCB+aADAgECAhDZ3jYhefXsu0VtIVMGHiOiMAoGCCqGSM49BAMCMCQxIjAg\n" +
    "BgNVBAMMGUFsbEpveW5UZXN0U2VsZlNpZ25lZE5hbWUwHhcNMTUwMzMxMTg1NDQy\n" +
    "WhcNMTYwMzMwMTg1NDQyWjAkMSIwIAYDVQQDDBlBbGxKb3luVGVzdFNlbGZTaWdu\n" +
    "ZWROYW1lMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAESOnRO0dXA7FFv4vJJXH8\n" +
    "8JBgvSvd1fg9NKosmkvAGYm5CLDBeLIhOycMVCcS2n0Q7mv+kUK+UXQbbg92exQ2\n" +
    "PqMNMAswCQYDVR0TBAIwADAKBggqhkjOPQQDAgNJADBGAiEAoV1uqzHvTVkOYLYl\n" +
    "QzRSg597ybtDqGoy5L6FgI7Qw5ECIQCrO+fxzcX2mMkYOX+g5gDmHurNKWKkSBnJ\n" +
    "wUq30brBfQ==\n" +
    "-----END CERTIFICATE-----\n";

    public static final String eccPrivateKeyPEMCAPI =
    "-----BEGIN EC PRIVATE KEY-----\n" +
    "MHcCAQEEIGjHhBsf1tL/qT2pVToR9SIJt6xKshX2N+svfXtDeCCooAoGCCqGSM49\n" +
    "AwEHoUQDQgAESOnRO0dXA7FFv4vJJXH88JBgvSvd1fg9NKosmkvAGYm5CLDBeLIh\n" +
    "OycMVCcS2n0Q7mv+kUK+UXQbbg92exQ2Pg==\n" +
    "-----END EC PRIVATE KEY-----\n";

    /* Certificate chains in AllJoyn must start with the EE cert. */
    public static final String eccCertChainX509PEMCAPI =
    "-----BEGIN CERTIFICATE-----\n" +
    "MIIBRjCB7qADAgECAhA4NHiS/771skQq7enlPEsyMAoGCCqGSM49BAMCMB4xHDAa\n" +
    "BgNVBAMME0FsbEpveW5UZXN0Um9vdE5hbWUwHhcNMTUwMzMxMjI0NjE1WhcNMTYw\n" +
    "MzMwMjI0NjE1WjAcMRowGAYDVQQDDBFDZXJ0U2lnbkxpYkNsaWVudDBZMBMGByqG\n" +
    "SM49AgEGCCqGSM49AwEHA0IABALDpAM6f0USoGm2vEaBBKr3dJdO9dIRukEUnTUV\n" +
    "0fKWN7N0hyIx/ZdANrtVJn8ZrzWnHuEkECEnYZy6hz1QC4ejEDAOMAwGA1UdEwQF\n" +
    "MAMBAf8wCgYIKoZIzj0EAwIDRwAwRAIgZT+K9SH5KnZEqvXUf/mOnJ8y0cvCaxzQ\n" +
    "9L+/V/1L/o0CIFGqG58zW7QealLNE7Z4dUjZgu0brTvRJDTJKAz7QreR\n" +
    "-----END CERTIFICATE-----\n" +
    "\n" +
    "-----BEGIN CERTIFICATE-----\n" +
    "MIIBRzCB7aADAgECAhDPaRHibuWiokAyJhlS20g+MAoGCCqGSM49BAMCMB4xHDAa\n" +
    "BgNVBAMME0FsbEpveW5UZXN0Um9vdE5hbWUwHhcNMTUwMzMxMjI0NjE1WhcNMTYw\n" +
    "MzMwMjI0NjE1WjAeMRwwGgYDVQQDDBNBbGxKb3luVGVzdFJvb3ROYW1lMFkwEwYH\n" +
    "KoZIzj0CAQYIKoZIzj0DAQcDQgAETXyIMsSx5xmNQE+fPtUa8NjqtP3h/c+kXjpo\n" +
    "XpApKcBocQ0tzXinzDWzg/GsJS9WCC+QHgJOL3BIiFFv4l1pHaMNMAswCQYDVR0T\n" +
    "BAIwADAKBggqhkjOPQQDAgNJADBGAiEA4NZuQGv/Je51gfuNq1M+EnvVnUq0XocV\n" +
    "C9rrhWhxSroCIQD6Sam3NVqhHis9ZsK7LwAzI9a7YOj5BqlDPW03adBdgg==\n" +
    "-----END CERTIFICATE-----\n";

    /* Bad certificate had a signature len of zero. */
    public static final String badCertX509PEMSignatureLenZero =
    "-----BEGIN CERTIFICATE-----\n" +
    "MIIBtDCCAVmgAwIBAgIJAMlyFqk69v+OMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n" +
    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n" +
    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAyMjYyMTUxMjVaFw0x\n" +
    "NjAyMjYyMTUxMjVaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy\n" +
    "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n" +
    "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABL50XeH1/aKcIF1+BJtlIgjL\n" +
    "AW32qoQdVOTyQg2WnM/R7pgxM2Ha0jMpksUd+JS9BiVYBBArwU76Whz9m6UyJeqj\n" +
    "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDAAAwRgIhAKfmglMgl67L5ALF\n" +
    "Z63haubkItTMACY1k4ROC2q7cnVmAiEArvAmcVInOq/U5C1y2XrvJQnAdwSl/Ogr\n" +
    "IizUeK0oI5c=\n" +
    "-----END CERTIFICATE-----";

    public CryptoECC ecc;

    public void setUp() throws Exception {
        ecc = new CryptoECC();
        ecc.generateDSAKeyPair();
        ecc.generateDHKeyPair();
    }

    public static void createCert(String serial, UUID issuer, String organization, ECCPrivateKey issuerPrivateKey, ECCPublicKey issuerPublicKey, UUID subject, ECCPublicKey subjectPubKey, long validFrom, long validTo, CertificateX509 x509) throws BusException {

        x509.setSerial(serial.getBytes());

        ByteBuffer bbIssuer = ByteBuffer.wrap(new byte[16]);
        bbIssuer.putLong(issuer.getMostSignificantBits());
        bbIssuer.putLong(issuer.getLeastSignificantBits());
        x509.setIssuerCN(bbIssuer.array());

        ByteBuffer bbSubject = ByteBuffer.wrap(new byte[16]);
        bbSubject.putLong(subject.getMostSignificantBits());
        bbSubject.putLong(subject.getLeastSignificantBits());
        x509.setSubjectCN(bbSubject.array());

        if (organization != null && !organization.isEmpty()) {
            x509.setIssuerOU(organization.getBytes());
            x509.setSubjectOU(organization.getBytes());
        }

        x509.setSubjectPublicKey(subjectPubKey);
        x509.setCA(true);
        x509.setValidity(validFrom, validTo);

        x509.signAndGenerateAuthorityKeyId(issuerPrivateKey, issuerPublicKey);
    }

    public void genKeyAndCreateCert(UUID issuer, String serial, String organization, ECCPrivateKey dsaPrivateKey, ECCPublicKey dsaPublicKey, ECCPrivateKey subjectPrivateKey, ECCPublicKey subjectPublicKey, boolean selfSign, long validFrom, long validTo, CertificateX509 x509) throws BusException {
        ecc.generateDSAKeyPair();
        dsaPrivateKey.setD(ecc.getDSAPrivateKey().getD());
        dsaPublicKey.setX(ecc.getDSAPublicKey().getX());
        dsaPublicKey.setY(ecc.getDSAPublicKey().getY());

        if (!selfSign) {
            ecc.generateDSAKeyPair();
        }

        subjectPrivateKey.setD(ecc.getDSAPrivateKey().getD());
        subjectPublicKey.setX(ecc.getDSAPublicKey().getX());
        subjectPublicKey.setY(ecc.getDSAPublicKey().getY());
        UUID userGuid = UUID.randomUUID();

        createCert(serial, issuer, organization, dsaPrivateKey, dsaPublicKey, userGuid, subjectPublicKey, validFrom, validTo, x509);
    }

    public void genKeyAndCreateCert(UUID issuer, String serial, String organization, ECCPrivateKey dsaPrivateKey, ECCPublicKey dsaPublicKey, ECCPrivateKey subjectPrivateKey, ECCPublicKey subjectPublicKey, boolean selfSign, long expiredInSeconds, CertificateX509 x509) throws BusException {
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + expiredInSeconds;
        genKeyAndCreateCert(issuer, serial, organization, dsaPrivateKey, dsaPublicKey, subjectPrivateKey, subjectPublicKey, selfSign, validFrom, validTo, x509);
    }

    public void testEncodePrivateKey() throws BusException {
        Status status;

        String encoded = CertificateX509.encodePrivateKeyPEM(ecc.getDSAPrivateKey());

        System.out.println("The encoded private key PEM " + encoded);

        ECCPrivateKey pk = CertificateX509.decodePrivateKeyPEM(encoded);

        System.out.println("Original private key " + ecc.getDSAPrivateKey().toString());
        System.out.println("Decoded private key " + pk.toString());
        assertEquals(pk, ecc.getDSAPrivateKey());
    }

    /**
     * Test decoding a private key created externally (in PKCS#8 DER format).
     */
    public static void decodePrivateKeyHelper(String eccPrivateKeyPEM) throws BusException {
        ECCPrivateKey pk = CertificateX509.decodePrivateKeyPEM(eccPrivateKeyPEM);
        System.out.println("Decoded private key " + pk.toString());
    }

    public void testDecodePrivateKey() throws BusException {
        decodePrivateKeyHelper(eccPrivateKeyPEMOpenSSL);
        decodePrivateKeyHelper(eccPrivateKeyPEMCAPI);
    }

    public void testDecodeUnsupportedFormatPrivateKeyPEM() {
        try {
            decodePrivateKeyHelper(eccUnsupportedFormatPrivateKeyPEM);
            fail("CertificateX509.decodePrivateKeyPEM did not fail");
        } catch (BusException e) {}
    }

    /**
     * Test encoding and decoding of public keys.
     */
    public void testEncodePublicKey() throws BusException {
        String encoded = CertificateX509.encodePublicKeyPEM(ecc.getDSAPublicKey());
        System.out.println("The encoded public key PEM " + encoded);
        ECCPublicKey pk = CertificateX509.decodePublicKeyPEM(encoded);

        System.out.println("Original public key " + ecc.getDSAPublicKey().toString());
        System.out.println("Decoded public key " + pk.toString());
        assertEquals(pk, ecc.getDSAPublicKey());
    }

    /**
     * Use the encoded texts to put in the bbservice and bbclient files
     */
    public void testGenSelfSignECCX509CertForBBservice() throws BusException {
        UUID issuer = UUID.randomUUID();

        ECCPrivateKey dsaPrivateKey = new ECCPrivateKey();
        ECCPublicKey dsaPublicKey = new ECCPublicKey();
        ECCPrivateKey subjectPrivateKey = new ECCPrivateKey();
        ECCPublicKey subjectPublicKey = new ECCPublicKey();
        CertificateX509 x509 = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);

        /* cert expires in ten years */
        genKeyAndCreateCert(issuer, "1010101", "organization", dsaPrivateKey, dsaPublicKey, subjectPrivateKey, subjectPublicKey, true, 10 * 365 * 24 * 3600, x509);

        String encodedPK = CertificateX509.encodePrivateKeyPEM(subjectPrivateKey);
        System.out.println("The encoded subject private key PEM:\n" + encodedPK);

        encodedPK = CertificateX509.encodePublicKeyPEM(subjectPublicKey);
        System.out.println("The encoded subject public key PEM:\n" + encodedPK);

        x509.verify(dsaPublicKey);

        String pem = x509.getPEM();
        System.out.println("The encoded cert PEM for ECC X.509 cert:\n" + pem);

        System.out.println("The ECC X.509 cert:\n" + x509.toJavaString());
    }


    /**
     * Test expiry date for X509 cert
     */
    public void testExpiredX509Cert() throws BusException {
        UUID issuer = UUID.randomUUID();
        ECCPrivateKey dsaPrivateKey = new ECCPrivateKey();
        ECCPublicKey dsaPublicKey = new ECCPublicKey();
        ECCPrivateKey subjectPrivateKey = new ECCPrivateKey();
        ECCPublicKey subjectPublicKey = new ECCPublicKey();
        CertificateX509 x509 = new CertificateX509();
        String noOrg = null;

        /* cert expires in two seconds */
        genKeyAndCreateCert(issuer, "1010101", noOrg, dsaPrivateKey, dsaPublicKey, subjectPrivateKey, subjectPublicKey, true, 2, x509);

        /* verify that the cert is not yet expired */
        x509.verifyValidity();

        /* sleep for 3 seconds to wait for the cert expires */
        try {
            Thread.sleep(3000);
        } catch (InterruptedException e) { }

        try {
            x509.verifyValidity();
            fail();
        } catch (BusException e) {}
    }

    /**
     * Generate certificate with expiry date past the year 2050
     */
    public void testX509CertExpiresBeyond2050() throws BusException {
        UUID issuer = UUID.randomUUID();
        ECCPrivateKey dsaPrivateKey = new ECCPrivateKey();
        ECCPublicKey dsaPublicKey = new ECCPublicKey();
        ECCPrivateKey subjectPrivateKey = new ECCPrivateKey();
        ECCPublicKey subjectPublicKey = new ECCPublicKey();
        CertificateX509 cert = new CertificateX509();

        /* cert expires in about 36 years */
        long expiredInSecs = 36 * 365 * 24 * 60 * 60;
        genKeyAndCreateCert(issuer, "1010101", "organization", dsaPrivateKey, dsaPublicKey, subjectPrivateKey, subjectPublicKey, true, expiredInSecs, cert);
        cert.verify(dsaPublicKey);
    }

    /**
     * Verify X509 self signed certificates generated by an external tool.
     */
    public static void verifyX509SelfSignExternalCertHelper(String eccSelfSignCertX509PEM) throws BusException {
        CertificateX509 cert = new CertificateX509();
        cert.loadPEM(eccSelfSignCertX509PEM);
        cert.verify();
        System.out.println("cert: " + cert.toJavaString());
    }

    public void testVerifyX509SelfSignExternalCert() throws BusException {
        verifyX509SelfSignExternalCertHelper(eccSelfSignCertX509PEMOpenSSL);
        verifyX509SelfSignExternalCertHelper(eccSelfSignCertX509PEMCAPI);
        verifyX509SelfSignExternalCertHelper(eccSelfSignCertX509OpenSSLWithAKI);
    }

    /**
     * test a bad cert with zero length signature
     */
    public void testBadCertDoesNotLoad() {
        CertificateX509 cert = new CertificateX509();
        try {
            cert.loadPEM(badCertX509PEMSignatureLenZero);
            fail();
        } catch (BusException e) {}
    }

    /**
     * Verify X509 self signed certificates generated by external tools.
     * Add check that they do not verify with a different public key.
     */
    public static void verifyX509SelfSignCertPlusDoNotVerifyWithOtherKeyHelper(String eccSelfSignCertX509PEM, ECCPublicKey otherPublic) throws BusException {
        CertificateX509 cert = new CertificateX509();
        cert.loadPEM(eccSelfSignCertX509PEM);
        cert.verify();

        /* now verify with a different public key.  It is expected to fail */
        try {
            cert.verify(otherPublic);
            fail();
        } catch (BusException e) {}
    }

    public void testVerifyX509SelfSignCertPlusDoNotVerifyWithOtherKey() throws BusException {
        verifyX509SelfSignCertPlusDoNotVerifyWithOtherKeyHelper(eccSelfSignCertX509PEMOpenSSL, ecc.getDSAPublicKey());
        verifyX509SelfSignCertPlusDoNotVerifyWithOtherKeyHelper(eccSelfSignCertX509PEMCAPI, ecc.getDSAPublicKey());
    }

    /**
     * Verify X509 self signed certificates generated by external tools.
     */
    public static void verifyX509ExternalCertChainHelper(String eccCertChainX509PEM) throws BusException {

        CertificateX509[] certs = CertificateX509.decodeCertChainPEM(eccCertChainX509PEM, 2);
        for (int cnt = 0; cnt < certs.length; cnt++) {
            System.out.println("certs[" + Integer.toString(cnt) + "]: " + certs[cnt].toJavaString());
        }

        certs[0].verify(certs[1].getSubjectPublicKey());
    }

    public void testVerifyX509ExternalCertChain() throws BusException {
        verifyX509ExternalCertChainHelper(eccCertChainX509PEMOpenSSL);
        verifyX509ExternalCertChainHelper(eccCertChainX509PEMCAPI);
    }

    /**
     * Generate certificate with start date in the past and expiry date way in the
     * the future.  One can use the PEM file to do spot check the date with openssl.
     */
    public void testX509CertWideRangeValidPeriod() throws BusException {
        UUID issuer = UUID.randomUUID();
        ECCPrivateKey dsaPrivateKey = new ECCPrivateKey();
        ECCPublicKey dsaPublicKey = new ECCPublicKey();
        ECCPrivateKey subjectPrivateKey = new ECCPrivateKey();
        ECCPublicKey subjectPublicKey = new ECCPublicKey();
        CertificateX509 cert = new CertificateX509();

        /* cert valid from 1970 */
        long validFrom = 0;
        /* cert expires in the year 2035 */
        long expiredInSecs = 20 * 365 * 24 * 60 * 60;
        long validTo = System.currentTimeMillis() / 1000 + expiredInSecs;

        genKeyAndCreateCert(issuer, "1010101", "organization", dsaPrivateKey, dsaPublicKey, subjectPrivateKey, subjectPublicKey, true, validFrom, validTo, cert);
        cert.verify(dsaPublicKey);
        String pem = cert.getPEM();
        System.out.println("The encoded cert PEM for ECC X.509 cert:" + pem);
    }

    /**
     * Certificate chain with an unknown CA cert.
     */
    public void testFailToVerifyCertChainWithUnknownCACert() throws BusException {
        CertificateX509 certs[] = CertificateX509.decodeCertChainPEM(eccCertChainWithUnknownCACertPEM, 2);
        try {
            certs[0].verify(certs[1].getSubjectPublicKey());
            fail();
        } catch (BusException e) {}
    }

    /**
     * Badly formatted cert PEM should fail to load
     */
    public void testFailToLoadBadlyEncodedCertPEM() throws BusException {
        CertificateX509 cert = new CertificateX509();
        try {
            cert.loadPEM(badEncodedSelfSignCertX509PEM);
            fail();
        } catch (BusException e) {}
    }

    /**
     * Verify a X509 self signed certificate generated by external tool.
     * This certificate hold extra fields in the distinguished name.
     */
    public void testVerifyX509SelfSignExternalCertWithExtraDNFields() throws BusException {
        CertificateX509 cert = new CertificateX509();
        cert.loadPEM(eccSelfSignCertX509PEMWithExtraDNFields);
        cert.verify();
    }

    /**
     * Create a self signed cert and reload its PEM
     */
    public void testGenerateAndLoadSelfSignedCert() throws BusException {
        UUID issuer = UUID.randomUUID();
        ECCPrivateKey dsaPrivateKey = new ECCPrivateKey();
        ECCPublicKey dsaPublicKey = new ECCPublicKey();
        ECCPrivateKey subjectPrivateKey = new ECCPrivateKey();
        ECCPublicKey subjectPublicKey = new ECCPublicKey();
        CertificateX509 cert = new CertificateX509();

        /* cert expires in one year */
        genKeyAndCreateCert(issuer, "1010101", "organization", dsaPrivateKey, dsaPublicKey, subjectPrivateKey, subjectPublicKey, true, 365 * 24 * 3600, cert);

        cert.verify(dsaPublicKey);
        CertificateX509 cert2 = new CertificateX509();
        cert2.loadPEM(cert.getPEM());
        cert2.verify(dsaPublicKey);
    }

    /**
     * Test a self signed certificate with the basic constraints field marked as
     * critical.
     */
    public void testSelfSignedCertWithCritialBasicConstraint() throws BusException {
        final String eccSelfSignCertX509PEM =
            "-----BEGIN CERTIFICATE-----\n" +
                "MIIBVDCB/KADAgECAhC+Ci4hDqaWuEWj2eDd0zrfMAoGCCqGSM49BAMCMCQxIjAgBgNVBAMMGUFsbEpveW5UZXN0U2VsZlNpZ25lZE5hbWUwHhcNMTUwMzMxMTc0MTQwWhcNMTYwMzMwMTc0MTQwWjAkMSIwIAYDVQQDDBlBbGxKb3luVGVzdFNlbGZTaWduZWROYW1lMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE5nmP2qHqZ6N67jdoVxSA64U+Y+rThK+oAwgR6DNezFKMSgVMA1Snn4qsc1Q+KbaYAMj7hWs6xDUIbz6XTOJBvaMQMA4wDAYDVR0TAQH/BAIwADAKBggqhkjOPQQDAgNHADBEAiBJpmVQof40vG9qjWgBTMkETUT0d1kGADBjQK162bUCygIgAtHmpfRztbtr5hgXYdjx4W3Kw0elmnuIfsvrY86ONZs=\n" +
                "-----END CERTIFICATE-----\n";

        verifyX509SelfSignExternalCertHelper(eccSelfSignCertX509PEM);
    }

    /**
     * Test a certificate chain with leaf cert containing no CA field and
     * signing cert has pathlen = 0
     */
    public void testCertChainWithNoCAFieldInBasicContraints() throws BusException {
        /* the leaf cert does not contain the CA field */
        final String eccCertChainX509PEM =
            "-----BEGIN CERTIFICATE-----\n" +
                "MIIBRTCB66ADAgECAhAIrQyeRPmaj0tCzYi1kc1LMAoGCCqGSM49BAMCMB4xHDAaBgNVBAMME0FsbEpveW5UZXN0Um9vdE5hbWUwHhcNMTUwMzMxMjMyODU2WhcNMTYwMzMwMjMyODU2WjAcMRowGAYDVQQDDBFDZXJ0U2lnbkxpYkNsaWVudDBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABDrQE+EUBFzwtXq/vlG6IYYEpVxEndizIvaysExCBML5uYovNVLfWEqFmEDGLvv3rJkZ0I0xhzSyzLD+Zo4xzU+jDTALMAkGA1UdEwQCMAAwCgYIKoZIzj0EAwIDSQAwRgIhAJ++iDjgYeje0kmJ3cdYTwen1V92Ldz4m0NInbpPX3BOAiEAvUTLYd83T4uXNh6P+JL4Phj3zxVBo2mSvwnuFSyeSOg=\n" +
                "-----END CERTIFICATE-----\n" +
                "\n" +
                "-----BEGIN CERTIFICATE-----\n" +
                "MIIBTDCB86ADAgECAhDNAwko47UUmUcr+HFVMJj1MAoGCCqGSM49BAMCMB4xHDAaBgNVBAMME0FsbEpveW5UZXN0Um9vdE5hbWUwHhcNMTUwMzMxMjMyODU2WhcNMTYwMzMwMjMyODU2WjAeMRwwGgYDVQQDDBNBbGxKb3luVGVzdFJvb3ROYW1lMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEwmq2CF9Q1Lh/RfE9ejHMGb+AkgKljRgh3D2uOVCGCvxpMtH4AR+QzAPKwYOHvKewsZIBtC41N5Fb4wFbR3kaSaMTMBEwDwYDVR0TBAgwBgEB/wIBADAKBggqhkjOPQQDAgNIADBFAiAyIj1kEli20k2jRuhmSqyjHJ1rlv0oyLOXpgI5f5P0nAIhALIV4i9VG6+DiL7VgNQ1LQswZMgjEUMuPWL6UyuBDe3z\n" +
                "-----END CERTIFICATE-----\n";

        CertificateX509 certs[] = CertificateX509.decodeCertChainPEM(eccCertChainX509PEM, 2);
        certs[0].verify(certs[1].getSubjectPublicKey());
    }

    /**
     * Helper to test validity period.
     */
    public void helperValidityPeriod(long validFrom, long validTo) throws BusException {
        UUID issuer = UUID.randomUUID();
        ECCPrivateKey dsaPrivateKey = new ECCPrivateKey();
        ECCPublicKey dsaPublicKey = new ECCPublicKey();
        ECCPrivateKey subjectPrivateKey = new ECCPrivateKey();
        ECCPublicKey subjectPublicKey = new ECCPublicKey();
        CertificateX509 cert = new CertificateX509();

        genKeyAndCreateCert(issuer, "1010101", "organization", dsaPrivateKey, dsaPublicKey, subjectPrivateKey, subjectPublicKey, true, validFrom, validTo, cert);
        cert.verify(dsaPublicKey);
        String pem = cert.getPEM();

        CertificateX509 cert2 = new CertificateX509();
        cert2.loadPEM(pem);
        
        assertEquals(validFrom, cert2.getValidFrom());
        assertEquals(validTo, cert2.getValidTo());
    }

    /**
     * Test validity date generation
     */
    public void testvalidityPeriodGeneration() throws BusException {
        long validFrom = 1427404154;   /* 150326210914Z a date with day light savings */
        long validTo = 1427404154 + 630720000;   /* 350321210914Z */
        helperValidityPeriod(validFrom, validTo);

        validFrom = 1423177645;   /* 150205230725Z a date with no day light savings */
        validTo = validFrom + 630720000;   /* 350131230725Z */
        helperValidityPeriod(validFrom, validTo);
    }

    public void testSubjectAltNameInExternalGeneratedCert() throws BusException {
        /* Cert has identity alias field. Cert was generated using openssl command line */
        CertificateX509 cert = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);
        final String customIdentityAliasCert =
            "-----BEGIN CERTIFICATE-----\n" +
                "MIIBxjCCAW2gAwIBAgIJAIfm4O/IwDYDMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM" +
                "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4" +
                "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTA1MTMxNjM2MDdaFw0x" +
                "NjA1MTIxNjM2MDdaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy" +
                "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy" +
                "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABCK3CCj7zDXi3dcXjL/ECUR3" +
                "9NLRN9nnNDNlyy2jkdOJvUEje0+ZTRUJR7y8HfP49PgmRHuMQgYLhG52CMxlDZyj" +
                "JDAiMCAGA1UdEQQZMBegFQYKKwYBBAGC3nwBBKAHBAVhbGlhczAKBggqhkjOPQQD" +
                "AgNHADBEAiBRXDdDAiYPQt7aftVWkcB6q2tWQL6mVGa+r18T6C9wfwIgJbZRDtJ0" +
                "szuhC+BaE0r7AfOGUiXBys5CYXYxIMGe8zM=" +
                "-----END CERTIFICATE-----";

        cert.loadPEM(customIdentityAliasCert);
        cert.setSubjectAltName(("alias").getBytes());

        /* Cert has the security group id field.  Cert was generated using openssl command line */
        final String customMembershipCert =
            "-----BEGIN CERTIFICATE-----\n" +
                "MIIB0TCCAXigAwIBAgIJAIfm4O/IwDYGMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM" +
                "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4" +
                "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTA1MTMxNzU1MTBaFw0x" +
                "NjA1MTIxNzU1MTBaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy" +
                "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy" +
                "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABGTa279SkSAKGSbPTp4BtF1k" +
                "CsPHR9dhwjN1moCcp8LMWQi22walAsohxsS+J5Ma1ROSxllZ+70EbFAZkQWsG/6j" +
                "LzAtMCsGA1UdEQQkMCKgIAYKKwYBBAGC3nwBA6ASBBBBMTZCeXRlQXJyTm90SGV4" +
                "MAoGCCqGSM49BAMCA0cAMEQCIHk/c64FSVd3ODPsa5q19nbSNv3bg0PWN5oo+Vy4" +
                "BO/YAiBpoMjlAwfTYfAENq+0jsePNtT5d+au77GviCh8ZdAl/w==" +
                "-----END CERTIFICATE-----";

        CertificateX509 cert2 = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);
        cert2.loadPEM(customMembershipCert);
        cert2.setSubjectAltName(("A16ByteArrNotHex").getBytes());

        /* The subject alt name has an unknown otherName OID. Cert was generated
         * using openssl command line.
         */
        final String unknownOtherNameOIDCert =
            "-----BEGIN CERTIFICATE-----\n" +
                "MIIB0jCCAXigAwIBAgIJAIfm4O/IwDYHMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM" +
                "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4" +
                "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTA1MTMxODA3NDJaFw0x" +
                "NjA1MTIxODA3NDJaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy" +
                "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy" +
                "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABMAB1fmU/POy0YxEl/QJJqKy" +
                "4YE4RbF97hCKuHyM2EF7x6JsQlDkM0c5rYYBFVbhgTgpTQk5mNNMZznlWxPFWjmj" +
                "LzAtMCsGA1UdEQQkMCKgIAYKKwYBBAGC3nwBAKASBBBBMTZCeXRlQXJyTm90SGV4" +
                "MAoGCCqGSM49BAMCA0gAMEUCIE4WgW4y4NyXGwXGGAEgd9dTewZcoQ7sp2TJYj/D" +
                "H4bPAiEA4JvCcg8Kr/66qyrbKz0wWBv7L8igT9Y9KsJRcKSmYNo=" +
                "-----END CERTIFICATE-----";

        CertificateX509 cert3 = new CertificateX509();
        cert3.loadPEM(unknownOtherNameOIDCert);

        assertTrue(Arrays.equals("".getBytes(), cert3.getSubjectAltName()));
        assertEquals(CertificateX509.CertificateType.UNRESTRICTED_CERTIFICATE, cert3.getType());
    }

    public void testSubjectAltNameInIdentityCert() throws Exception {
        CertificateX509 cert = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);
        cert.setSubjectAltName("alias".getBytes());
        UUID issuer = UUID.randomUUID();
        ECCPrivateKey dsaPrivateKey = new ECCPrivateKey();
        ECCPublicKey dsaPublicKey = new ECCPublicKey();
        ECCPrivateKey subjectPrivateKey = new ECCPrivateKey();
        ECCPublicKey subjectPublicKey = new ECCPublicKey();

        genKeyAndCreateCert(issuer, "1010101", "organization", dsaPrivateKey, dsaPublicKey, subjectPrivateKey, subjectPublicKey, true, 365 * 24 * 3600, cert);

        String pem = cert.getPEM();

        CertificateX509 cert2 = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);
        cert2.loadPEM(pem);

        assertEquals("alias", new String(cert2.getSubjectAltName(), "UTF-8"));
    }

    public void testSubjectAltNameInMembershipCert() throws BusException {
        CertificateX509 cert = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);
        UUID securityGroupId = UUID.randomUUID();

        ByteBuffer bbsguid = ByteBuffer.wrap(new byte[16]);
        bbsguid.putLong(securityGroupId.getMostSignificantBits());
        bbsguid.putLong(securityGroupId.getLeastSignificantBits());
        cert.setSubjectAltName(bbsguid.array());

        UUID issuer = UUID.randomUUID();
        ECCPrivateKey dsaPrivateKey = new ECCPrivateKey();
        ECCPublicKey dsaPublicKey = new ECCPublicKey();
        ECCPrivateKey subjectPrivateKey = new ECCPrivateKey();
        ECCPublicKey subjectPublicKey = new ECCPublicKey();

        genKeyAndCreateCert(issuer, "1010101", "organization", dsaPrivateKey, dsaPublicKey, subjectPrivateKey, subjectPublicKey, true, 365 * 24 * 3600, cert);

        String pem = cert.getPEM();

        CertificateX509 cert2 = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);
        cert2.loadPEM(pem);
        assertTrue(Arrays.equals(bbsguid.array(), cert2.getSubjectAltName()));
    }

    public void testIsDNEqual() throws BusException {
        CertificateX509 cert1 = new CertificateX509();
        CertificateX509 cert2 = new CertificateX509();

        cert1.loadPEM(eccSelfSignCertX509PEMOpenSSL);
        cert2.loadPEM(eccSelfSignCertX509PEMCAPI);

        assertTrue(cert1.isDNEqual(cert1));
        assertTrue(cert1.isDNEqual(cert1.getSubjectCN(), cert1.getSubjectOU()));

        assertFalse(cert1.isDNEqual(cert2));
        /*
         * this call to isDNEqual fails an assertion
         * common/src/CertificateECC.cc:1351:
         * bool qcc::CertificateX509::IsDNEqual(const uint8_t*, size_t, const uint8_t*, size_t) const: Assertion `(ouLength > 0) || (nullptr == ou)'
         */
        //assertFalse(cert1.isDNEqual(cert2.getSubjectCN(), cert2.getSubjectOU()));
    }

    public void testIsIssuerOf() throws BusException {
        CertificateX509 cert1 = new CertificateX509();
        CertificateX509 cert2 = new CertificateX509();

        cert1.loadPEM(eccSelfSignCertX509PEMOpenSSL);
        cert2.loadPEM(eccSelfSignCertX509PEMCAPI);

        assertTrue(cert1.isIssuerOf(cert1));
        assertTrue(cert2.isIssuerOf(cert2));

        assertFalse(cert1.isIssuerOf(cert2));
        assertFalse(cert2.isIssuerOf(cert1));
    }

    /**
     * Test the certificate types in the chain of certs: identity, identity
     */
    public void testValidTypeInCertChain_II() throws BusException {
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 24 * 3600;

        UUID subject1 = UUID.randomUUID();
        CryptoECC ecc1 = new CryptoECC();
        ecc1.generateDSAKeyPair();
        CertificateX509 cert1 = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);

        /* self signed cert */
        createCert("1010101", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject1, ecc1.getDSAPublicKey(), validFrom, validTo, cert1);

        UUID subject0 = UUID.randomUUID();
        CryptoECC ecc0 = new CryptoECC();
        ecc0.generateDSAKeyPair();
        CertificateX509 cert0 = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);

        /* leaf cert signed by cert1 */
        createCert("2020202", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject0, ecc0.getDSAPublicKey(), validFrom, validTo, cert0);

        CertificateX509[] certs = new CertificateX509[2];
        certs[0] = cert0;
        certs[1] = cert1;

        assertTrue(CertificateX509.validateCertificateTypeInCertChain(certs));
    }

    /**
     * Test the certificate types in the chain of certs: membership, membership
     */
    public void testValidTypeInCertChain_MM() throws BusException {
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 24 * 3600;

        UUID subject1 = UUID.randomUUID();
        CryptoECC ecc1 = new CryptoECC();
        ecc1.generateDSAKeyPair();
        CertificateX509 cert1 = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);

        /* self signed cert */
        createCert("1010101", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject1, ecc1.getDSAPublicKey(), validFrom, validTo, cert1);

        UUID subject0 = UUID.randomUUID();
        CryptoECC ecc0 = new CryptoECC();
        ecc0.generateDSAKeyPair();
        CertificateX509 cert0 = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);

        /* leaf cert signed by cert1 */
        createCert("2020202", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject0, ecc0.getDSAPublicKey(), validFrom, validTo, cert0);

        CertificateX509 certs[] = new CertificateX509[2];
        certs[0] = cert0;
        certs[1] = cert1;

        assertTrue(CertificateX509.validateCertificateTypeInCertChain(certs));
    }

    /**
     * Test the certificate types in the chain of certs: membership, identity
     */
    public void testValidTypeInCertChain_MI() throws BusException {
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 24 * 3600;

        UUID subject1 = UUID.randomUUID();
        CryptoECC ecc1 = new CryptoECC();
        ecc1.generateDSAKeyPair();
        CertificateX509 cert1 = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);

        /* self signed cert */
        createCert("1010101", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject1, ecc1.getDSAPublicKey(), validFrom, validTo, cert1);

        UUID subject0 = UUID.randomUUID();
        CryptoECC ecc0 = new CryptoECC();
        ecc0.generateDSAKeyPair();
        CertificateX509 cert0 = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);

        /* leaf cert signed by cert1 */
        createCert("2020202", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject0, ecc0.getDSAPublicKey(), validFrom, validTo, cert0);

        CertificateX509 certs[] = new CertificateX509[2];
        certs[0] = cert0;
        certs[1] = cert1;

        assertFalse(CertificateX509.validateCertificateTypeInCertChain(certs));
    }

    /**
     * Test the certificate types in the chain of certs: identity, membership
     */
    public void testValidTypeInCertChain_IM() throws BusException {
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 24 * 3600;

        UUID subject1 = UUID.randomUUID();
        CryptoECC ecc1 = new CryptoECC();
        ecc1.generateDSAKeyPair();
        CertificateX509 cert1 = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);

        /* self signed cert */
        createCert("1010101", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject1, ecc1.getDSAPublicKey(), validFrom, validTo, cert1);

        UUID subject0 = UUID.randomUUID();
        CryptoECC ecc0 = new CryptoECC();
        ecc0.generateDSAKeyPair();
        CertificateX509 cert0 = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);

        /* leaf cert signed by cert1 */
        createCert("2020202", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject0, ecc0.getDSAPublicKey(), validFrom, validTo, cert0);

        CertificateX509 certs[] = new CertificateX509[2];
        certs[0] = cert0;
        certs[1] = cert1;

        assertFalse(CertificateX509.validateCertificateTypeInCertChain(certs));
    }

    /**
     * Test the certificate types in the chain of certs: unrestricted, membership
     */
    public void testValidTypeInCertChain_UM() throws BusException {
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 24 * 3600;

        UUID subject1 = UUID.randomUUID();
        CryptoECC ecc1 = new CryptoECC();
        ecc1.generateDSAKeyPair();
        CertificateX509 cert1 = new CertificateX509();

        /* self signed cert */
        createCert("1010101", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject1, ecc1.getDSAPublicKey(), validFrom, validTo, cert1);

        UUID subject0 = UUID.randomUUID();
        CryptoECC ecc0 = new CryptoECC();
        ecc0.generateDSAKeyPair();
        CertificateX509 cert0 = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);

        /* leaf cert signed by cert1 */
        createCert("2020202", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject0, ecc0.getDSAPublicKey(), validFrom, validTo, cert0);

        CertificateX509 certs[] = new CertificateX509[2];
        certs[0] = cert0;
        certs[1] = cert1;

        assertTrue(CertificateX509.validateCertificateTypeInCertChain(certs));
    }

    /**
     * Test the certificate types in the chain of certs: unrestricted, identity
     */
    public void testValidTypeInCertChain_UI() throws BusException {
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 24 * 3600;

        UUID subject1 = UUID.randomUUID();
        CryptoECC ecc1 = new CryptoECC();
        ecc1.generateDSAKeyPair();
        CertificateX509 cert1 = new CertificateX509();

        /* self signed cert */
        createCert("1010101", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject1, ecc1.getDSAPublicKey(), validFrom, validTo, cert1);

        UUID subject0 = UUID.randomUUID();
        CryptoECC ecc0 = new CryptoECC();
        ecc0.generateDSAKeyPair();
        CertificateX509 cert0 = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);

        /* leaf cert signed by cert1 */
        createCert("2020202", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject0, ecc0.getDSAPublicKey(), validFrom, validTo, cert0);

        CertificateX509 certs[] = new CertificateX509[2];
        certs[0] = cert0;
        certs[1] = cert1;

        assertTrue(CertificateX509.validateCertificateTypeInCertChain(certs));
    }

    /**
     * Test the certificate types in the chain of certs: identity, unrestricted
     */
    public void testValidTypeInCertChain_IU() throws BusException {
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 24 * 3600;

        UUID subject1 = UUID.randomUUID();
        CryptoECC ecc1 = new CryptoECC();
        ecc1.generateDSAKeyPair();
        CertificateX509 cert1 = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);

        /* self signed cert */
        createCert("1010101", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject1, ecc1.getDSAPublicKey(), validFrom, validTo, cert1);

        UUID subject0 = UUID.randomUUID();
        CryptoECC ecc0 = new CryptoECC();
        ecc0.generateDSAKeyPair();
        CertificateX509 cert0 = new CertificateX509();

        /* leaf cert signed by cert1 */
        createCert("2020202", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject0, ecc0.getDSAPublicKey(), validFrom, validTo, cert0);

        CertificateX509 certs[] = new CertificateX509[2];
        certs[0] = cert0;
        certs[1] = cert1;

        assertFalse(CertificateX509.validateCertificateTypeInCertChain(certs));
    }

    /**
     * Test the certificate types in the chain of certs: membership, unrestricted
     */
    public void testValidTypeInCertChain_MU() throws BusException {
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 24 * 3600;

        UUID subject1 = UUID.randomUUID();
        CryptoECC ecc1 = new CryptoECC();
        ecc1.generateDSAKeyPair();
        CertificateX509 cert1 = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);

        /* self signed cert */
        createCert("1010101", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject1, ecc1.getDSAPublicKey(), validFrom, validTo, cert1);

        UUID subject0 = UUID.randomUUID();
        CryptoECC ecc0 = new CryptoECC();
        ecc0.generateDSAKeyPair();
        CertificateX509 cert0 = new CertificateX509();

        /* leaf cert signed by cert1 */
        createCert("2020202", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject0, ecc0.getDSAPublicKey(), validFrom, validTo, cert0);

        CertificateX509 certs[] = new CertificateX509[2];
        certs[0] = cert0;
        certs[1] = cert1;

        assertFalse(CertificateX509.validateCertificateTypeInCertChain(certs));
    }

    /**
     * Test the certificate types in the chain of certs: unrestricted, membership, membership
     */
    public void testValidTypeInCertChain_UMM() throws BusException {
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 24 * 3600;

        UUID subject2 = UUID.randomUUID();
        CryptoECC ecc2 = new CryptoECC();
        ecc2.generateDSAKeyPair();
        CertificateX509 cert2 = new CertificateX509();

        /* self signed cert */
        createCert("serial2", subject2, "organization", ecc2.getDSAPrivateKey(), ecc2.getDSAPublicKey(), subject2, ecc2.getDSAPublicKey(), validFrom, validTo, cert2);

        UUID subject1 = UUID.randomUUID();
        CryptoECC ecc1 = new CryptoECC();
        ecc1.generateDSAKeyPair();
        CertificateX509 cert1 = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);

        /* intermediate cert signed by cert2 */
        createCert("serial1", subject2, "organization", ecc2.getDSAPrivateKey(), ecc2.getDSAPublicKey(), subject1, ecc1.getDSAPublicKey(), validFrom, validTo, cert1);

        UUID subject0 = UUID.randomUUID();
        CryptoECC ecc0 = new CryptoECC();
        ecc0.generateDSAKeyPair();
        CertificateX509 cert0 = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);

        /* leaf cert signed by cert1 */
        createCert("serial0", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject0, ecc0.getDSAPublicKey(), validFrom, validTo, cert0);

        CertificateX509 certs[] = new CertificateX509[3];
        certs[0] = cert0;
        certs[1] = cert1;
        certs[2] = cert2;

        assertTrue(CertificateX509.validateCertificateTypeInCertChain(certs));
    }

    /**
     * Test the certificate types in the chain of certs: unrestricted, identity, identity
     */
    public void testValidTypeInCertChain_UII() throws BusException {
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 24 * 3600;

        UUID subject2 = UUID.randomUUID();
        CryptoECC ecc2 = new CryptoECC();
        ecc2.generateDSAKeyPair();
        CertificateX509 cert2 = new CertificateX509();

        /* self signed cert */
        createCert("serial2", subject2, "organization", ecc2.getDSAPrivateKey(), ecc2.getDSAPublicKey(), subject2, ecc2.getDSAPublicKey(), validFrom, validTo, cert2);

        UUID subject1 = UUID.randomUUID();
        CryptoECC ecc1 = new CryptoECC();
        ecc1.generateDSAKeyPair();
        CertificateX509 cert1 = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);

        /* intermediate cert signed by cert2 */
        createCert("serial1", subject2, "organization", ecc2.getDSAPrivateKey(), ecc2.getDSAPublicKey(), subject1, ecc1.getDSAPublicKey(), validFrom, validTo, cert1);

        UUID subject0 = UUID.randomUUID();
        CryptoECC ecc0 = new CryptoECC();
        ecc0.generateDSAKeyPair();
        CertificateX509 cert0 = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);

        /* leaf cert signed by cert1 */
        createCert("serial0", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject0, ecc0.getDSAPublicKey(), validFrom, validTo, cert0);

        CertificateX509 certs[] = new CertificateX509[3];
        certs[0] = cert0;
        certs[1] = cert1;
        certs[2] = cert2;

        assertTrue(CertificateX509.validateCertificateTypeInCertChain(certs));
    }

    /**
     * Test the certificate types in the chain of certs: identity, unrestricted, identity
     */
    public void testValidTypeInCertChain_IUI() throws BusException {
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 24 * 3600;

        UUID subject2 = UUID.randomUUID();
        CryptoECC ecc2 = new CryptoECC();
        ecc2.generateDSAKeyPair();
        CertificateX509 cert2 = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);

        /* self signed cert */
        createCert("serial2", subject2, "organization", ecc2.getDSAPrivateKey(), ecc2.getDSAPublicKey(), subject2, ecc2.getDSAPublicKey(), validFrom, validTo, cert2);

        UUID subject1 = UUID.randomUUID();
        CryptoECC ecc1 = new CryptoECC();
        ecc1.generateDSAKeyPair();
        CertificateX509 cert1 = new CertificateX509();

        /* intermediate cert signed by cert2 */
        createCert("serial1", subject2, "organization", ecc2.getDSAPrivateKey(), ecc2.getDSAPublicKey(), subject1, ecc1.getDSAPublicKey(), validFrom, validTo, cert1);

        UUID subject0 = UUID.randomUUID();
        CryptoECC ecc0 = new CryptoECC();
        ecc0.generateDSAKeyPair();
        CertificateX509 cert0 = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);

        /* leaf cert signed by cert1 */
        createCert("serial0", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject0, ecc0.getDSAPublicKey(), validFrom, validTo, cert0);

        CertificateX509 certs[] = new CertificateX509[3];
        certs[0] = cert0;
        certs[1] = cert1;
        certs[2] = cert2;

        assertTrue(CertificateX509.validateCertificateTypeInCertChain(certs));
    }

    /**
     * Test the certificate types in the chain of certs: membership, unrestricted, membership
     */
    public void testValidTypeInCertChain_MUM() throws BusException {
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 24 * 3600;

        UUID subject2 = UUID.randomUUID();
        CryptoECC ecc2 = new CryptoECC();
        ecc2.generateDSAKeyPair();
        CertificateX509 cert2 = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);

        /* self signed cert */
        createCert("serial2", subject2, "organization", ecc2.getDSAPrivateKey(), ecc2.getDSAPublicKey(), subject2, ecc2.getDSAPublicKey(), validFrom, validTo, cert2);

        UUID subject1 = UUID.randomUUID();
        CryptoECC ecc1 = new CryptoECC();
        ecc1.generateDSAKeyPair();
        CertificateX509 cert1 = new CertificateX509();

        /* intermediate cert signed by cert2 */
        createCert("serial1", subject2, "organization", ecc2.getDSAPrivateKey(), ecc2.getDSAPublicKey(), subject1, ecc1.getDSAPublicKey(), validFrom, validTo, cert1);

        UUID subject0 = UUID.randomUUID();
        CryptoECC ecc0 = new CryptoECC();
        ecc0.generateDSAKeyPair();
        CertificateX509 cert0 = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);

        /* leaf cert signed by cert1 */
        createCert("serial0", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject0, ecc0.getDSAPublicKey(), validFrom, validTo, cert0);

        CertificateX509 certs[] = new CertificateX509[3];
        certs[0] = cert0;
        certs[1] = cert1;
        certs[2] = cert2;

        assertTrue(CertificateX509.validateCertificateTypeInCertChain(certs));
    }

    /**
     * Test the certificate types in the chain of certs: membership, unrestricted, identity
     */
    public void testValidTypeInCertChain_MUI() throws BusException {
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 24 * 3600;

        UUID subject2 = UUID.randomUUID();
        CryptoECC ecc2 = new CryptoECC();
        ecc2.generateDSAKeyPair();
        CertificateX509 cert2 = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);

        /* self signed cert */
        createCert("serial2", subject2, "organization", ecc2.getDSAPrivateKey(), ecc2.getDSAPublicKey(), subject2, ecc2.getDSAPublicKey(), validFrom, validTo, cert2);

        UUID subject1 = UUID.randomUUID();
        CryptoECC ecc1 = new CryptoECC();
        ecc1.generateDSAKeyPair();
        CertificateX509 cert1 = new CertificateX509();

        /* intermediate cert signed by cert2 */
        createCert("serial1", subject2, "organization", ecc2.getDSAPrivateKey(), ecc2.getDSAPublicKey(), subject1, ecc1.getDSAPublicKey(), validFrom, validTo, cert1);

        UUID subject0 = UUID.randomUUID();
        CryptoECC ecc0 = new CryptoECC();
        ecc0.generateDSAKeyPair();
        CertificateX509 cert0 = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);

        /* leaf cert signed by cert1 */
        createCert("serial0", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject0, ecc0.getDSAPublicKey(), validFrom, validTo, cert0);

        CertificateX509 certs[] = new CertificateX509[3];
        certs[0] = cert0;
        certs[1] = cert1;
        certs[2] = cert2;

        assertFalse(CertificateX509.validateCertificateTypeInCertChain(certs));
    }

    /**
     * Test the certificate types in the chain of certs: identity, unrestricted, membership
     */
    public void testValidTypeInCertChain_IUM() throws BusException {
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 24 * 3600;

        UUID subject2 = UUID.randomUUID();
        CryptoECC ecc2 = new CryptoECC();
        ecc2.generateDSAKeyPair();
        CertificateX509 cert2 = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);

        /* self signed cert */
        createCert("serial2", subject2, "organization", ecc2.getDSAPrivateKey(), ecc2.getDSAPublicKey(), subject2, ecc2.getDSAPublicKey(), validFrom, validTo, cert2);

        UUID subject1 = UUID.randomUUID();
        CryptoECC ecc1 = new CryptoECC();
        ecc1.generateDSAKeyPair();
        CertificateX509 cert1 = new CertificateX509();

        /* intermediate cert signed by cert2 */
        createCert("serial1", subject2, "organization", ecc2.getDSAPrivateKey(), ecc2.getDSAPublicKey(), subject1, ecc1.getDSAPublicKey(), validFrom, validTo, cert1);

        UUID subject0 = UUID.randomUUID();
        CryptoECC ecc0 = new CryptoECC();
        ecc0.generateDSAKeyPair();
        CertificateX509 cert0 = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);

        /* leaf cert signed by cert1 */
        createCert("serial0", subject1, "organization", ecc1.getDSAPrivateKey(), ecc1.getDSAPublicKey(), subject0, ecc0.getDSAPublicKey(), validFrom, validTo, cert0);

        CertificateX509 certs[] = new CertificateX509[3];
        certs[0] = cert0;
        certs[1] = cert1;
        certs[2] = cert2;

        assertFalse(CertificateX509.validateCertificateTypeInCertChain(certs));
    }

    /**
     * Test the certificate types in the chain of certs: identity
     */
    public void testValidTypeInCertChain_I() throws BusException {
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 24 * 3600;

        UUID subject0 = UUID.randomUUID();
        CryptoECC ecc0 = new CryptoECC();
        ecc0.generateDSAKeyPair();
        CertificateX509 cert0 = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);

        /* self signed cert */
        createCert("serial0", subject0, "organization", ecc0.getDSAPrivateKey(), ecc0.getDSAPublicKey(), subject0, ecc0.getDSAPublicKey(), validFrom, validTo, cert0);

        CertificateX509 certs[] = new CertificateX509[1];
        certs[0] = cert0;

        assertTrue(CertificateX509.validateCertificateTypeInCertChain(certs));
    }

    /**
     * Test the certificate types in the chain of certs: membership
     */
    public void testValidTypeInCertChain_M() throws BusException {
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 24 * 3600;

        UUID subject0 = UUID.randomUUID();
        CryptoECC ecc0 = new CryptoECC();
        ecc0.generateDSAKeyPair();
        CertificateX509 cert0 = new CertificateX509(CertificateX509.CertificateType.MEMBERSHIP_CERTIFICATE);

        /* self signed cert */
        createCert("serial0", subject0, "organization", ecc0.getDSAPrivateKey(), ecc0.getDSAPublicKey(), subject0, ecc0.getDSAPublicKey(), validFrom, validTo, cert0);

        CertificateX509 certs[] = new CertificateX509[1];
        certs[0] = cert0;

        assertTrue(CertificateX509.validateCertificateTypeInCertChain(certs));
    }

    /**
     * Test the certificate types in the chain of certs: unrestricted
     */
    public void testValidTypeInCertChain_U() throws BusException {
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 24 * 3600;

        UUID subject0 = UUID.randomUUID();
        CryptoECC ecc0 = new CryptoECC();
        ecc0.generateDSAKeyPair();
        CertificateX509 cert0 = new CertificateX509();

        /* self signed cert */
        createCert("serial0", subject0, "organization", ecc0.getDSAPrivateKey(), ecc0.getDSAPublicKey(), subject0, ecc0.getDSAPublicKey(), validFrom, validTo, cert0);

        CertificateX509 certs[] = new CertificateX509[1];
        certs[0] = cert0;

        assertFalse(CertificateX509.validateCertificateTypeInCertChain(certs));
    }

    /**
     * Test encoding and decoding of AKI
     */
    public void testAKIEncodingDecoding() throws BusException {
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 24 * 3600;

        UUID subject0 = UUID.randomUUID();
        CryptoECC ecc0 = new CryptoECC();
        ecc0.generateDSAKeyPair();
        CertificateX509 cert0 = new CertificateX509();
        CertificateX509 cert1 = new CertificateX509();
        byte[] der;

        /* self signed cert */
        createCert("serial0", subject0, "organization", ecc0.getDSAPrivateKey(), ecc0.getDSAPublicKey(), subject0, ecc0.getDSAPublicKey(), validFrom, validTo, cert0);
        der = cert0.encodeCertificateDER();
        cert1.decodeCertificateDER(der);
        assertTrue(Arrays.equals(cert0.getAuthorityKeyId(), cert1.getAuthorityKeyId()));
    }

    public void testCreateIdentityCertificateChain() throws BusException {
        CryptoECC key1 = new CryptoECC();
        CryptoECC key2 = new CryptoECC();
        CryptoECC key3 = new CryptoECC();
        CryptoECC key4 = new CryptoECC();
        CertificateX509 cert1 = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);
        CertificateX509 cert2 = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);
        CertificateX509 cert3 = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);
        CertificateX509 cert4 = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);
        String serial1 = "01";
        String serial2 = "02";
        String serial3 = "03";
        String serial4 = "04";
        String cn1 = "cn1";
        String cn2 = "cn2";
        String cn3 = "cn3";
        String cn4 = "cn4";

        key1.generateDSAKeyPair();
        key2.generateDSAKeyPair();
        key3.generateDSAKeyPair();
        key4.generateDSAKeyPair();

        cert1.setSerial(serial1.getBytes());
        cert2.setSerial(serial2.getBytes());
        cert3.setSerial(serial3.getBytes());
        cert4.setSerial(serial3.getBytes());
        cert1.setIssuerCN(cn1.getBytes());
        cert2.setIssuerCN(cn1.getBytes());
        cert3.setIssuerCN(cn2.getBytes());
        cert4.setIssuerCN(cn3.getBytes());
        cert1.setSubjectCN(cn1.getBytes());
        cert2.setSubjectCN(cn2.getBytes());
        cert3.setSubjectCN(cn3.getBytes());
        cert4.setSubjectCN(cn4.getBytes());
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 10000;
        cert1.setValidity(validFrom, validTo);
        cert2.setValidity(validFrom, validTo);
        cert3.setValidity(validFrom, validTo);
        cert4.setValidity(validFrom, validTo);
        cert1.setCA(true);
        cert2.setCA(true);
        cert3.setCA(true);
        cert4.setCA(false);
        cert1.setSubjectPublicKey(key1.getDSAPublicKey());
        cert2.setSubjectPublicKey(key2.getDSAPublicKey());
        cert3.setSubjectPublicKey(key3.getDSAPublicKey());
        cert4.setSubjectPublicKey(key4.getDSAPublicKey());

        cert1.sign(key1.getDSAPrivateKey());
        cert2.signAndGenerateAuthorityKeyId(key1.getDSAPrivateKey(), key1.getDSAPublicKey());
        cert3.signAndGenerateAuthorityKeyId(key2.getDSAPrivateKey(), key2.getDSAPublicKey());
        cert4.signAndGenerateAuthorityKeyId(key3.getDSAPrivateKey(), key3.getDSAPublicKey());

        cert1.verify(key1.getDSAPublicKey());
        cert2.verify(key1.getDSAPublicKey());
        cert3.verify(key2.getDSAPublicKey());
        cert4.verify(key3.getDSAPublicKey());

        /* Print out certs in end entity..root order */
        System.out.println(cert4.getPEM());
        System.out.println(cert3.getPEM());
        System.out.println(cert2.getPEM());
        System.out.println(cert1.getPEM());

        CertificateX509 cert5 = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);
        cert5.loadPEM(cert1.getPEM());
        assertEquals(cert1.getValidFrom(), cert5.getValidFrom());
        assertEquals(cert1.getValidTo(), cert5.getValidTo());
    }

    /**
     * Test Getting and setting of AKI
     */
    public void testGetSetAKI() throws BusException {
        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 24 * 3600;

        UUID subject0 = UUID.randomUUID();
        CryptoECC ecc0 = new CryptoECC();
        ecc0.generateDSAKeyPair();
        CertificateX509 cert0 = new CertificateX509();
        CertificateX509 cert1 = new CertificateX509();

        /* self signed cert */
        createCert("serial0", subject0, "organization", ecc0.getDSAPrivateKey(), ecc0.getDSAPublicKey(), subject0, ecc0.getDSAPublicKey(), validFrom, validTo, cert0);
        String aki = "abcdeef";
        cert0.setAuthorityKeyId(aki.getBytes());
        assertTrue(Arrays.equals(aki.getBytes(), cert0.getAuthorityKeyId()));

        cert0.sign(ecc0.getDSAPrivateKey());
        byte[] der = cert0.encodeCertificateDER();
        cert1.decodeCertificateDER(der);
        assertTrue(Arrays.equals(aki.getBytes(), cert1.getAuthorityKeyId()));

        //Check for for strings with 0 char
        byte charWithNull[] = { 'a', 'b', 0, 'c', 'd' };
        cert0.setAuthorityKeyId(charWithNull);
        assertEquals(charWithNull.length, cert0.getAuthorityKeyId().length);
        cert0.sign(ecc0.getDSAPrivateKey());

        byte[] derWithNull = cert0.encodeCertificateDER();
        CertificateX509 cert2 = new CertificateX509();

        cert2.decodeCertificateDER(derWithNull);
        
        assertEquals(charWithNull.length, cert2.getAuthorityKeyId().length);
        assertTrue(Arrays.equals(charWithNull, cert2.getAuthorityKeyId()));
    }

    public void testInvalidPemTests() throws BusException {
        CertificateX509 cert = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);

        /* Test that very short PEM inputs are rejected. */
        final String shortPem =
            "-----BEGIN CERTIFICATE-----\n" +
                "MIIBxjC\n";

        try {
            cert.loadPEM(shortPem);
            fail();
        } catch (BusException e) {
            assertEquals(e.getMessage(), "ER_INVALID_DATA");
        }

        /* Test that very long cert chains with valid syntax are rejected. */
        final String validPem =
            "-----BEGIN CERTIFICATE-----\n" +
                "MIIBsDCCAVagAwIBAgIJAJVJ9/7bbQcWMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n" +
                "IDZkODVjMjkyMjYxM2IzNmUyZWVlZjUyNzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1\n" +
                "YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQyY2M1NjAeFw0xNTAyMjYxODAzNDlaFw0x\n" +
                "NjAyMjYxODAzNDlaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy\n" +
                "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n" +
                "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABPY4jHsVP+2eeWUEFOu0Z/zK\n" +
                "V8wJGMeZup0LdpVllL1l2Gzji1AcdE3cPxstyGtVpi0mOXnMsNQlLR7GcY4a5XKj\n" +
                "DTALMAkGA1UdEwQCMAAwCgYIKoZIzj0EAwIDSAAwRQIhAKrCirrUWNNAO2gFiNTl\n" +
                "/ncnbELhDiDq/N43LIpfAfX8AiAKX7h/9nXEerJlthl5gUOa4xV6UjqbZLM6+KH/\n" +
                "Hk/Yvw==\n" +
                "-----END CERTIFICATE-----\n\n";

        String pemStr = "";
        for (int i = 0; i < 30; i++) {
            pemStr += validPem;
        }

        try {
            cert.loadPEM(shortPem);
            fail();
        } catch (BusException e) {
            assertEquals(e.getMessage(), "ER_INVALID_DATA");
        }

        /* Test that inputs with invalid characters are rejected */
        StringBuilder invalidPem = new StringBuilder(validPem);
        invalidPem.setCharAt(30, (char) 0x01);
        try {
            cert.loadPEM(shortPem);
            fail();
        } catch (BusException e) {
            assertEquals(e.getMessage(), "ER_INVALID_DATA");
        }
    }

    public void testInvalidASN1Test1() throws BusException {
        CertificateX509 cert = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);

        /* Test missing ASN_CONSTRUCTED_ENCODING bit from ASN.1 input corresponding to "c(i)" syntax */
        String invalidASN1 =
            "-----BEGIN CERTIFICATE-----\n" +
                "MIIBRjCB7sADAgECAhA4NHiS/771skQq7enlPEsyMAoGCCqGSM49BAMCMB4xHDAa\n" +
                "BgNVBAMME0FsbEpveW5UZXN0Um9vdE5hbWUwHhcNMTUwMzMxMjI0NjE1WhcNMTYw\n" +
                "MzMwMjI0NjE1WjAcMRowGAYDVQQDDBFDZXJ0U2lnbkxpYkNsaWVudDBZMBMGByqG\n" +
                "SM49AgEGCCqGSM49AwEHA0IABALDpAM6f0USoGm2vEaBBKr3dJdO9dIRukEUnTUV\n" +
                "0fKWN7N0hyIx/ZdANrtVJn8ZrzWnHuEkECEnYZy6hz1QC4ejEDAOMAwGA1UdEwQF\n" +
                "MAMBAf8wCgYIKoZIzj0EAwIDRwAwRAIgZT+K9SH5KnZEqvXUf/mOnJ8y0cvCaxzQ\n" +
                "9L+/V/1L/o0CIFGqG58zW7QealLNE7Z4dUjZgu0brTvRJDTJKAz7QreR\n" +
                "-----END CERTIFICATE-----\n";
        try {
            cert.loadPEM(invalidASN1);
            fail();
        } catch (BusException e) {
            assertEquals(e.getMessage(), "ER_FAIL");
        }
    }

    public void testInvalidASN1Test2() throws BusException {
        CertificateX509 cert = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);

        /* Invalid certificate that caused an invalid read of the syntax string. */
        final String invalidASN1 =
            "-----BEGIN CERTIFICATE-----\n" +
                "MIIBkzCCATq00000000000000000000000000000000000000000000000000000\n" +
                "0000000000000000000000000000000000000000000000000000000000000000\n" +
                "0000000000000000000000000000000000000000000000000000000000000000\n" +
                "0000000000000000000000000000000000000000000000000000000000000000\n" +
                "0000000000000000000000000000000000000000000000000000000000000000\n" +
                "0000000000000000000000000000000000000000000000000000000000000000\n" +
                "000000000000000000000000000000000000000000000DAKBgg00000000000NH\n" +
                "A000000000000000000000000000000000000000000000000000000000000000\n" +
                "00000000000000000000000000000000\n" +
                "-----END CERTIFICATE-----\n";
        try {
            cert.loadPEM(invalidASN1);
            fail();
        } catch (BusException e) {
            assertEquals(e.getMessage(), "ER_BAD_ARG_1");
        }
    }

    public void testInvalidASN1Test3() throws BusException {
        CertificateX509 cert = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);

        /*
         * Invalid certificate that triggers an assertion in qcc::String::assign
         * when called with a string of zero length.
         * (in the default case of Crypto_ASN1::DecodeV)
         */
        final String invalidASN1 =
            "-----BEGIN CERTIFICATE-----\n" +
                "MIIBRjCB7qADAgECAhA000000000000000000000MAoGCCqGSM49BAMCMB4xHDAa\n" +
                "Bg000000000000000000000000000000000wHhcN00000000000000000hcA0000\n" +
                "0000000000000jAc0000000000000000000000000000000000000DBZ00000000\n" +
                "0000000000000000000000000000000000000000000000000000000000000000\n" +
                "0000000000000000000000000000000000000000000000000000000000000000\n" +
                "0000000wCgYIKoZIzj0EAwIDRwA0000000000000000000000000000000000000\n" +
                "00000000000000000000000000000000000000000000000000000000\n" +
                "-----END CERTIFICATE-----\n";
        try {
            cert.loadPEM(invalidASN1);
            fail();
        } catch (BusException e) {
            assertEquals(e.getMessage(), "ER_FAIL");
        }
    }

    public void testInvalidASN1Test4() throws BusException {
        CertificateX509 cert = new CertificateX509(CertificateX509.CertificateType.IDENTITY_CERTIFICATE);

        /*
         * Invalid certificate that triggers an assertion in qcc::String::assign
         * when called with a string of zero length.
         * (in case 'b' of Crypto_ASN1::DecodeV)
         */
        final String invalidASN1 =
            "-----BEGIN CERTIFICATE-----\n" +
                "MIIBtDCCAVmgAwIBAgIJAMlyFqk69v+OMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n" +
                "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n" +
                "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAyMjYyMTUxMjVaFw0x\n" +
                "NjAyMjYyMTUxMjVaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy\n" +
                "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n" +
                "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABL50XeH1/aKcIF1+BJtlIgjL\n" +
                "AW32qoQdVOTyQg2WnM/R7pgxM2Ha0jMpksUd+JS9BiVYBBArwU76Whz9m6UyJeqj\n" +
                "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDAQAwRgIhAKfmglMgl67L5ALF\n" +
                "Z63haubkItTMACY1k4ROC2q7cnVmAiEArvAmcVInOq/U5C1y2XrvJQnAdwSl/Ogr\n" +
                "IizUeK0oI5c=\n" +
                "-----END CERTIFICATE-----\n";
        try {
            cert.loadPEM(invalidASN1);
            fail();
        } catch (BusException e) {
            assertEquals(e.getMessage(), "ER_FAIL");
        }
    }

    public void testGenerateCertAndAssignRandomSerialNumber() throws BusException {
        CryptoECC ecc0 = new CryptoECC();
        ecc0.generateDSAKeyPair();
        UUID subject0 = UUID.randomUUID();
        CertificateX509 cert0 = new CertificateX509();

        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 3600;

        createCert("serial0", subject0, "organization", ecc0.getDSAPrivateKey(), ecc0.getDSAPublicKey(), subject0, ecc0.getDSAPublicKey(), validFrom, validTo, cert0);

        cert0.generateRandomSerial();
        assertFalse(Arrays.equals("serial0".getBytes(), cert0.getSerial()));
    }

    public void testEncodeCertificateTBSSucceeds() throws BusException {
        CryptoECC ecc0 = new CryptoECC();
        ecc0.generateDSAKeyPair();
        UUID subject0 = UUID.randomUUID();
        CertificateX509 cert0 = new CertificateX509();

        long validFrom = System.currentTimeMillis() / 1000;
        long validTo = validFrom + 3600;

        createCert("serial0", subject0, "organization", ecc0.getDSAPrivateKey(), ecc0.getDSAPublicKey(), subject0, ecc0.getDSAPublicKey(), validFrom, validTo, cert0);

        byte[] tbs = cert0.encodeCertificateTBS();
        assertFalse(0 == tbs.length);
    }
}
