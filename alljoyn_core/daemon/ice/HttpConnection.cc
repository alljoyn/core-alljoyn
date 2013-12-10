/**
 * @file
 *
 * Extremely simple HTTP/1.0 implementation.
 *
 */

/******************************************************************************
 * Copyright (c) 2009,2012 AllSeen Alliance. All rights reserved.
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

#include <map>
#include <string>
#include <vector>
#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/SocketTypes.h>
#include <qcc/SslSocket.h>
#include <qcc/String.h>
#include <qcc/Stream.h>
#include <qcc/StringUtil.h>
#include <JSON/json.h>
#include <alljoyn/Status.h>
#include "HttpConnection.h"

using namespace std;
using namespace qcc;

#define QCC_MODULE "HTTP"

static const char RendezvousTestServerRootCertificate[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIEVzCCAz+gAwIBAgIQFoFkpCjKEt+rEvGfsbk1VDANBgkqhkiG9w0BAQUFADCB\n"
    "jDELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMTAwLgYDVQQL\n"
    "EydGb3IgVGVzdCBQdXJwb3NlcyBPbmx5LiAgTm8gYXNzdXJhbmNlcy4xMjAwBgNV\n"
    "BAMTKVZlcmlTaWduIFRyaWFsIFNlY3VyZSBTZXJ2ZXIgUm9vdCBDQSAtIEcyMB4X\n"
    "DTA5MDQwMTAwMDAwMFoXDTI5MDMzMTIzNTk1OVowgYwxCzAJBgNVBAYTAlVTMRcw\n"
    "FQYDVQQKEw5WZXJpU2lnbiwgSW5jLjEwMC4GA1UECxMnRm9yIFRlc3QgUHVycG9z\n"
    "ZXMgT25seS4gIE5vIGFzc3VyYW5jZXMuMTIwMAYDVQQDEylWZXJpU2lnbiBUcmlh\n"
    "bCBTZWN1cmUgU2VydmVyIFJvb3QgQ0EgLSBHMjCCASIwDQYJKoZIhvcNAQEBBQAD\n"
    "ggEPADCCAQoCggEBAMCJggWnSVAcIomnvCFhXlCdgafCKCDxVSNQY2jhYGZXcZsq\n"
    "ToJmDQ7b9JO39VCPnXELOENP2+4FNCUQnzarLfghsJ8kQ9pxjRTfcMp0bsH+Gk/1\n"
    "qLDgvf9WuiBa5SM/jXNvroEQZwPuMZg4r2E2k0412VTq9ColODYNDZw3ziiYdSjV\n"
    "fY3VfbsLSXJIh2jaJC5kVRsUsx72s4/wgGXbb+P/XKr15nMIB0yH9A5tiCCXQ5nO\n"
    "EV7/ddZqmL3zdeAtyGmijOxjwiy+GS6xr7KACfbPEJYZYaS/P0wctIOyQy6CkNKL\n"
    "o5vDDkOZks0zjf6RAzNXZndvsXEJpQe5WO1avm8CAwEAAaOBsjCBrzAPBgNVHRMB\n"
    "Af8EBTADAQH/MA4GA1UdDwEB/wQEAwIBBjBtBggrBgEFBQcBDARhMF+hXaBbMFkw\n"
    "VzBVFglpbWFnZS9naWYwITAfMAcGBSsOAwIaBBSP5dMahqyNjmvDz4Bq1EgYLHsZ\n"
    "LjAlFiNodHRwOi8vbG9nby52ZXJpc2lnbi5jb20vdnNsb2dvLmdpZjAdBgNVHQ4E\n"
    "FgQUSBnnkm+SnTRjmcDwmcjWpYyMf2UwDQYJKoZIhvcNAQEFBQADggEBADuswa8C\n"
    "0hunHp17KJQ0WwNRQCp8f/u4L8Hz/TiGfybnaMXgn0sKI8Xe79iGE91M7vrzh0Gt\n"
    "ap0GLShkiqHGsHkIxBcVMFbEQ1VS63XhTeg36cWQ1EjOHmu+8tQe0oZuwFsYYdfs\n"
    "n4EZcpspiep9LFc/hu4FE8SsY6MiasHR2Ay97UsC9A3S7ZaoHfdwyhtcINXCu2lX\n"
    "W0Gpi3vzWRvwqgua6dm2WVKJfvPfmS1mAP0YmTcIwjdiNXiU6sSsJEoNlTR9zCoo\n"
    "4oKQ8wVoWZpbuPZb5geszhS7YsABUPIAAfF1YQCiMULtpa6HFzzm7sdf72N3HfwE\n"
    "aQNg95KnKGrrDUI=\n"
    "-----END CERTIFICATE-----"
};

static const char RendezvousStageServerRootCertificate[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIFTDCCBDSgAwIBAgIQB4vio3UvantYOivK060ZQDANBgkqhkiG9w0BAQUFADCB\n"
    "tTELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\n"
    "ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTswOQYDVQQLEzJUZXJtcyBvZiB1c2Ug\n"
    "YXQgaHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL3JwYSAoYykxMDEvMC0GA1UEAxMm\n"
    "VmVyaVNpZ24gQ2xhc3MgMyBTZWN1cmUgU2VydmVyIENBIC0gRzMwHhcNMTIwNDMw\n"
    "MDAwMDAwWhcNMTMwNTAxMjM1OTU5WjCBhDELMAkGA1UEBhMCVVMxEzARBgNVBAgT\n"
    "CkNhbGlmb3JuaWExEjAQBgNVBAcUCVNhbiBEaWVnbzEeMBwGA1UEChQVUVVBTENP\n"
    "TU0gSW5jb3Jwb3JhdGVkMQ0wCwYDVQQLFARDT1JQMR0wGwYDVQQDFBRyZHZzLXN0\n"
    "Zy5hbGxqb3luLm9yZzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBANSV\n"
    "xS3TM7dgakw2YsHEdx9T78YpozuzcZ6HUfytjSey20ii7BaavxBbBapRU046/Us1\n"
    "r+ODf7XWz4Cn7kuUPByLxZlqKATlGAg+J4eAOgP3eXbfPPMEcw3HiGdBuSJ4C3bQ\n"
    "jxokKz7XyY4jk39lhO/N4/rMqrdmFmSU421Ypw7QdPuOozCUTbsuYUBphzEO2i4S\n"
    "tvB0X2ZYueTArAIvxruH8dJBLIUg3gXzeIddKGFAgTLhwC46EKIIGZyw4OgQ75TF\n"
    "7+7jcTWvjVC6oh/e4LTiKFlTuGaBZ0nJ00ywmFXMsaUaVets83ksuLyOzv6V1j+q\n"
    "IyXw5ChCWKcytMxR1pMCAwEAAaOCAYUwggGBMB8GA1UdEQQYMBaCFHJkdnMtc3Rn\n"
    "LmFsbGpveW4ub3JnMAkGA1UdEwQCMAAwDgYDVR0PAQH/BAQDAgWgMEUGA1UdHwQ+\n"
    "MDwwOqA4oDaGNGh0dHA6Ly9TVlJTZWN1cmUtRzMtY3JsLnZlcmlzaWduLmNvbS9T\n"
    "VlJTZWN1cmVHMy5jcmwwRAYDVR0gBD0wOzA5BgtghkgBhvhFAQcXAzAqMCgGCCsG\n"
    "AQUFBwIBFhxodHRwczovL3d3dy52ZXJpc2lnbi5jb20vY3BzMB0GA1UdJQQWMBQG\n"
    "CCsGAQUFBwMBBggrBgEFBQcDAjAfBgNVHSMEGDAWgBQNRFwWU0TBgn4dIKsl9AFj\n"
    "2L55pTB2BggrBgEFBQcBAQRqMGgwJAYIKwYBBQUHMAGGGGh0dHA6Ly9vY3NwLnZl\n"
    "cmlzaWduLmNvbTBABggrBgEFBQcwAoY0aHR0cDovL1NWUlNlY3VyZS1HMy1haWEu\n"
    "dmVyaXNpZ24uY29tL1NWUlNlY3VyZUczLmNlcjANBgkqhkiG9w0BAQUFAAOCAQEA\n"
    "erROdpT8c2ETQI2vD1Vqnu4I1S3bIxHMBxYrepknmxpo0sd8/2+IjfzV5Hw2RrbZ\n"
    "AW8jQF9yAUWU5iOwRILdCHtGxdXrJBpvzKICd8dkaPnvJIOESheqM06yrx18swkX\n"
    "H+4JvS5nOwEzQt2qpwBkUmtGjG/9ACHDZ/ORQyNw/ygyR7ZhBANNAu19C4M72+dI\n"
    "rf6kTZ3PXC5AaaiBKV/XAAZmbT1SkDMVIr0p6zWNcjWwWQOxxIQ+m7Qr5PYiDx6/\n"
    "Yk9x7onFy2Qx3l8SeSOXAN14kHDcEG+5BhkWDse1/LVk+eDmFDNmnsgtCFLNVG5z\n"
    "D97Ydn9rPYTEABQUlWTUsg==\n"
    "-----END CERTIFICATE-----"
};

static const char RendezvousDeploymentServerRootCertificate[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIF7DCCBNSgAwIBAgIQbsx6pacDIAm4zrz06VLUkTANBgkqhkiG9w0BAQUFADCB\n"
    "yjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\n"
    "ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJp\n"
    "U2lnbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxW\n"
    "ZXJpU2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0\n"
    "aG9yaXR5IC0gRzUwHhcNMTAwMjA4MDAwMDAwWhcNMjAwMjA3MjM1OTU5WjCBtTEL\n"
    "MAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZW\n"
    "ZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTswOQYDVQQLEzJUZXJtcyBvZiB1c2UgYXQg\n"
    "aHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL3JwYSAoYykxMDEvMC0GA1UEAxMmVmVy\n"
    "aVNpZ24gQ2xhc3MgMyBTZWN1cmUgU2VydmVyIENBIC0gRzMwggEiMA0GCSqGSIb3\n"
    "DQEBAQUAA4IBDwAwggEKAoIBAQCxh4QfwgxF9byrJZenraI+nLr2wTm4i8rCrFbG\n"
    "5btljkRPTc5v7QlK1K9OEJxoiy6Ve4mbE8riNDTB81vzSXtig0iBdNGIeGwCU/m8\n"
    "f0MmV1gzgzszChew0E6RJK2GfWQS3HRKNKEdCuqWHQsV/KNLO85jiND4LQyUhhDK\n"
    "tpo9yus3nABINYYpUHjoRWPNGUFP9ZXse5jUxHGzUL4os4+guVOc9cosI6n9FAbo\n"
    "GLSa6Dxugf3kzTU2s1HTaewSulZub5tXxYsU5w7HnO1KVGrJTcW/EbGuHGeBy0RV\n"
    "M5l/JJs/U0V/hhrzPPptf4H1uErT9YU3HLWm0AnkGHs4TvoPAgMBAAGjggHfMIIB\n"
    "2zA0BggrBgEFBQcBAQQoMCYwJAYIKwYBBQUHMAGGGGh0dHA6Ly9vY3NwLnZlcmlz\n"
    "aWduLmNvbTASBgNVHRMBAf8ECDAGAQH/AgEAMHAGA1UdIARpMGcwZQYLYIZIAYb4\n"
    "RQEHFwMwVjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cudmVyaXNpZ24uY29tL2Nw\n"
    "czAqBggrBgEFBQcCAjAeGhxodHRwczovL3d3dy52ZXJpc2lnbi5jb20vcnBhMDQG\n"
    "A1UdHwQtMCswKaAnoCWGI2h0dHA6Ly9jcmwudmVyaXNpZ24uY29tL3BjYTMtZzUu\n"
    "Y3JsMA4GA1UdDwEB/wQEAwIBBjBtBggrBgEFBQcBDARhMF+hXaBbMFkwVzBVFglp\n"
    "bWFnZS9naWYwITAfMAcGBSsOAwIaBBSP5dMahqyNjmvDz4Bq1EgYLHsZLjAlFiNo\n"
    "dHRwOi8vbG9nby52ZXJpc2lnbi5jb20vdnNsb2dvLmdpZjAoBgNVHREEITAfpB0w\n"
    "GzEZMBcGA1UEAxMQVmVyaVNpZ25NUEtJLTItNjAdBgNVHQ4EFgQUDURcFlNEwYJ+\n"
    "HSCrJfQBY9i+eaUwHwYDVR0jBBgwFoAUf9Nlp8Ld7LvwMAnzQzn6Aq8zMTMwDQYJ\n"
    "KoZIhvcNAQEFBQADggEBAAyDJO/dwwzZWJz+NrbrioBL0aP3nfPMU++CnqOh5pfB\n"
    "WJ11bOAdG0z60cEtBcDqbrIicFXZIDNAMwfCZYP6j0M3m+oOmmxw7vacgDvZN/R6\n"
    "bezQGH1JSsqZxxkoor7YdyT3hSaGbYcFQEFn0Sc67dxIHSLNCwuLvPSxe/20majp\n"
    "dirhGi2HbnTTiN0eIsbfFrYrghQKlFzyUOyvzv9iNw2tZdMGQVPtAhTItVgooazg\n"
    "W+yzf5VK+wPIrSbb5mZ4EkrZn0L74ZjmQoObj49nJOhhGbXdzbULJgWOw27EyHW4\n"
    "Rs/iGAZeqa6ogZpHFt4MKGwlJ7net4RYxh84HqTEy2Y=\n"
    "-----END CERTIFICATE-----"
};

/*
 * That certificate is the intermediate cert (G3 issued by G5) that was used to verify
 * the cert issued to the rendezvous server and signed with G3. And this cert would be
 * the same for production, test and stage environments because the certs of all these
 * three servers are signed with the same G3 cert.
 */
static const char RendezvousServerCACertificate[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIE0zCCA7ugAwIBAgIQGNrRniZ96LtKIVjNzGs7SjANBgkqhkiG9w0BAQUFADCB\n"
    "yjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\n"
    "ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJp\n"
    "U2lnbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxW\n"
    "ZXJpU2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0\n"
    "aG9yaXR5IC0gRzUwHhcNMDYxMTA4MDAwMDAwWhcNMzYwNzE2MjM1OTU5WjCByjEL\n"
    "MAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZW\n"
    "ZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJpU2ln\n"
    "biwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxWZXJp\n"
    "U2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9y\n"
    "aXR5IC0gRzUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCvJAgIKXo1\n"
    "nmAMqudLO07cfLw8RRy7K+D+KQL5VwijZIUVJ/XxrcgxiV0i6CqqpkKzj/i5Vbex\n"
    "t0uz/o9+B1fs70PbZmIVYc9gDaTY3vjgw2IIPVQT60nKWVSFJuUrjxuf6/WhkcIz\n"
    "SdhDY2pSS9KP6HBRTdGJaXvHcPaz3BJ023tdS1bTlr8Vd6Gw9KIl8q8ckmcY5fQG\n"
    "BO+QueQA5N06tRn/Arr0PO7gi+s3i+z016zy9vA9r911kTMZHRxAy3QkGSGT2RT+\n"
    "rCpSx4/VBEnkjWNHiDxpg8v+R70rfk/Fla4OndTRQ8Bnc+MUCH7lP59zuDMKz10/\n"
    "NIeWiu5T6CUVAgMBAAGjgbIwga8wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8E\n"
    "BAMCAQYwbQYIKwYBBQUHAQwEYTBfoV2gWzBZMFcwVRYJaW1hZ2UvZ2lmMCEwHzAH\n"
    "BgUrDgMCGgQUj+XTGoasjY5rw8+AatRIGCx7GS4wJRYjaHR0cDovL2xvZ28udmVy\n"
    "aXNpZ24uY29tL3ZzbG9nby5naWYwHQYDVR0OBBYEFH/TZafC3ey78DAJ80M5+gKv\n"
    "MzEzMA0GCSqGSIb3DQEBBQUAA4IBAQCTJEowX2LP2BqYLz3q3JktvXf2pXkiOOzE\n"
    "p6B4Eq1iDkVwZMXnl2YtmAl+X6/WzChl8gGqCBpH3vn5fJJaCGkgDdk+bW48DW7Y\n"
    "5gaRQBi5+MHt39tBquCWIMnNZBU4gcmU7qKEKQsTb47bDN0lAtukixlE0kF6BWlK\n"
    "WE9gyn6CagsCqiUXObXbf+eEZSqVir2G3l6BFoMtEMze/aiCKm0oHw0LxOXnGiYZ\n"
    "4fQRbxC1lfznQgUy286dUV4otp6F01vvpX1FQHKOtw5rDgb7MzVIcbidJ4vEZV8N\n"
    "hnacRHr2lVz2XTIIM6RUthg/aFzyQkqFOFSDX9HoLPKsEdao7WNq\n"
    "-----END CERTIFICATE-----\n"
};



static String urlEncode(String str)
{
    static const String hexChars = "0123456789ABCDEF";
    static const String unsafeChars = " <>\"#%{}|\\^~[]`";
    String outStr;
    String::const_iterator it = str.begin();

    while (it != str.end()) {
        char c = *it;
        if ((c < 0x20) || (c >= 0x80) || (String::npos != unsafeChars.find(c))) {
            outStr.push_back('%');
            outStr.push_back(hexChars[c >> 4]);
            outStr.push_back(hexChars[c & 0x0F]);
        } else {
            outStr.push_back(c);
        }
        ++it;
    }
    return outStr;
}

static QStatus getLine(Source* source, String& str) {

    char c;
    QStatus status = ER_OK;
    size_t received;

    while (ER_OK == status) {
        status = source->PullBytes(&c, 1, received);
        if ((ER_OK == status) && (1 == received)) {
            if ('\r' == c) {
                continue;
            } else if ('\n' == c) {
                break;
            } else {
                str.push_back(c);
            }
        }
    }

    return status;
}

QStatus HttpResponseSource::PullBytes(void*buf, size_t reqBytes, size_t& actualBytes,  uint32_t timeout)
{
    QStatus status = ER_NONE;

    size_t rb = min(reqBytes, contentLength - bytesRead);
    status = source->PullBytes(buf, rb, actualBytes);
    if (ER_OK == status) {
        bytesRead += actualBytes;
    }
    return status;
}

void HttpResponseSource::Reset(Source& source)
{
    this->source = &source;
    contentLength = 0;
    bytesRead = 0;
}

HttpConnection::~HttpConnection()
{
    Close();
}

void HttpConnection::Clear(void)
{
    /** Clear all the HTTP state except the connection itself */
    method = METHOD_GET;
    urlPath.clear();
    query.clear();
    httpStatus = HTTP_STATUS_INVALID;
    requestBody.clear();
    isMultipartForm = false;
    isApplicationJson = false;
    requestHeaders.clear();
    responseHeaders.clear();

    /* Dump any remaining chars in response stream */
    if (stream) {
        QStatus status = ER_OK;
        size_t contentLength = httpSource.GetContentLength();
        while ((ER_OK == status) && (contentLength > httpSource.GetBytesRead())) {
            uint8_t buf[256];
            size_t reqBytes = min(sizeof(buf) / sizeof(buf[0]), contentLength - httpSource.GetBytesRead());
            size_t actual;
            status = httpSource.PullBytes(buf, reqBytes, actual);
        }
    }
}

void HttpConnection::SetUrlPath(String urlPath)
{
    this->urlPath = urlEncode(urlPath);
}

void HttpConnection::SetHost(String host)
{
    QCC_DbgPrintf(("HttpConnection::SetHost(): Setting the host to %s\n", host.c_str()));
    SetRequestHeader("Host", host);
    this->host = host;
    QCC_DbgPrintf(("HttpConnection::SetHost(): requestHeaders[Host] = %s\n", requestHeaders["Host"].c_str()));
}

QStatus HttpConnection::SetHostIPAddress(String host)
{
    QStatus status = ER_OK;
    IPAddress tempAddr;
    // Retrieve the Server IP Address from the Server Name
    status = tempAddr.SetAddress(host, true, NAME_RESOLUTION_TIMEOUT_IN_MS);

    if (status != ER_OK) {
        QCC_LogError(status, ("HttpConnection::SetHostIPAddress(): Invalid Rendezvous Server address specified"));
        return status;
    }

    this->hostIPAddress = tempAddr.ToString();

    QCC_DbgPrintf(("HttpConnection::SetHostIPAddress(): Setting the host ip address to %s\n", tempAddr.ToString().c_str()));

    return status;
}

void HttpConnection::AddQueryParameter(String name, String value)
{
    if (query.empty()) {
        query = "?" + urlEncode(name) + "=" + urlEncode(value);
    } else {
        query = query + "&" + urlEncode(name) + "=" + urlEncode(value);
    }
}

void HttpConnection::AddMultipartFormField(String name, String content, String contentType)
{
    if (requestBody.empty()) {
        requestHeaders["Content-Type"] = "multipart/form-data, boundary=AaB03x";
        requestBody.append("--AaB03x");
        isMultipartForm = true;
    }
    requestBody.append("\r\nContent-Disposition: form-data; name=\"");
    requestBody.append(name);
    requestBody.append("\"\r\n");
    if (0 < contentType.size()) {
        requestBody.append("Content-Type: ");
        requestBody.append(contentType);
        requestBody.append("\r\n");
    }
    requestBody.append("\r\n");
    requestBody.append(content);
    requestBody.append("\r\n--AaB03x");
}

void HttpConnection::AddApplicationJsonField(String content) {

    if (requestBody.empty()) {
        requestHeaders["Content-Type"] = "application/json";
        isApplicationJson = true;
    }

    requestBody.append("\r\n");
    requestBody.append(content);
    requestBody.append("\r\n");
}

QStatus HttpConnection::Connect(SocketFd sock)
{
    QStatus status = ER_OK;

    /* Connect using the passed Socket */
    if (NULL == stream) {
        switch (protocol) {
        default:
        case PROTO_HTTP:
        {
            if (0 == port) {
                port = 80;
            }
            SocketStream* sockStream = new SocketStream(sock);
            stream = sockStream;
            status = sockStream->Connect(hostIPAddress, port);

            /* Retrieve the interface details over which the OS has
             * set up the socket to talk to the Server */
            uint16_t localPort;
            qcc::GetLocalAddress(sockStream->GetSocketFd(), localIPAddress, localPort);

            httpSource.Reset(*stream);
            break;
        }

        case PROTO_HTTPS:
        {
            const char* rootCert;
            const char* caCert;

            caCert = RendezvousServerCACertificate;
            if (host == "connect-test.qualcomm.com") {
                rootCert = RendezvousTestServerRootCertificate;
            } else if (host == "connect.alljoyn.org") {
                rootCert = RendezvousDeploymentServerRootCertificate;
            } else if (host == "connect-stg.alljoyn.org") {
                rootCert = RendezvousStageServerRootCertificate;
            } else {
                status = ER_RENDEZVOUS_SERVER_ROOT_CERTIFICATE_UNINITIALIZED;
                break;
            }

            if (0 == port) {
                port = 443;
            }

            SslSocket* sslSocket = new SslSocket(host, rootCert, caCert);
            stream = sslSocket;
#if defined(QCC_OS_WINRT)
            // On WinRT, when using SslSocket, Connect() should use the host name instead of its IP address, otherwise it will fail with error "CertCN_NO_MATCH"
            status = sslSocket->Connect(host, port);
#else
            status = sslSocket->Connect(hostIPAddress, port);
#endif

            /* Retrieve the interface details over which the OS has
             * set up the socket to talk to the Server */
            uint16_t localPort;
            qcc::GetLocalAddress(sslSocket->GetSocketFd(), localIPAddress, localPort);

            httpSource.Reset(*stream);

            break;
        }
        }

    } else {
        QCC_DbgPrintf(("A connection with the Server already exists."));
    }

    return status;
}

QStatus HttpConnection::Send(void)
{
    QStatus status = ER_OK;
    String outStr;
    size_t sentBytes;

    /* Send HTTP request */

    // Multipart form data needs to have a content-length and trailing "--"
    if (isMultipartForm) {
        requestBody.push_back('-');
        requestBody.push_back('-');
        requestHeaders["Content-Length"] = U32ToString(requestBody.size());
    }

    // application/json data needs to have a content-length
    if (isApplicationJson) {
        requestHeaders["Content-Length"] = U32ToString(requestBody.size());
    }

    if (METHOD_INVALID != method) {
        outStr.append(GetHTTPMethodString(method));
    }
    outStr.append(urlPath);
    if (!query.empty()) {
        outStr.append(query);
    }
    outStr.append(" HTTP/1.1\r\n");

    std::map<String, String>::const_iterator it;

    for (it = requestHeaders.begin(); it != requestHeaders.end(); it++) {
        outStr.append(it->first.c_str());
        outStr.append(": ");
        outStr.append(it->second.c_str());
        outStr.append("\r\n");
    }
    outStr.append("\r\n");

    /* Send body if it exists */
    if (0 < requestBody.size()) {
        outStr.append(requestBody);
    }

    QCC_DbgPrintf(("Sending HTTP Request: %s size %d", outStr.c_str(), outStr.size()));

    status = stream->PushBytes((void*)outStr.c_str(), outStr.size(), sentBytes);
    if ((ER_OK == status) && (sentBytes != outStr.size())) {
        status = ER_WRITE_ERROR;
    }

    if (ER_OK != status) {
        Close();
    }

    return status;
}

QStatus HttpConnection::GetStatusCode(HttpStatus& httpStatus)
{
    QStatus status = ER_OK;
    httpStatus = this->httpStatus;
    return status;
}

void HttpConnection::Close()
{
    if (stream) {
        delete stream;
        stream = NULL;
    }
}

QStatus HttpConnection::ParseResponse(HTTPResponse& response)
{
    if (!stream) {
        return ER_FAIL;
        QCC_LogError(ER_FAIL, ("HttpConnection::ParseResponse(): steam is NULL"));
    } else {
        QStatus status = ER_OK;

        httpSource.Reset(*stream);

        /* Get HTTP response status line.*/
        String statusLine;
        status = getLine(stream, statusLine);

        if (ER_OK == status) {
            size_t pos = statusLine.find(' ');
            if (pos != String::npos) {
                uint32_t s = StringToU32(statusLine.substr(pos + 1).c_str(), 10, 0);
                status = CheckHTTPResponseStatus(httpStatus, s);
                if (ER_OK != status) {
                    QCC_LogError(status, ("HttpConnection::ParseResponse(): Unrecognized HTTP Status code received in response"));
                } else {
                    response.statusCode = httpStatus;
                    /* Get response headers */
                    while (1) {
                        String line;
                        status = getLine(stream, line);

                        if (ER_OK == status) {
                            if (line.empty()) {
                                break;
                            } else {
                                size_t pos = line.find(':');
                                if ((0 != pos) && (pos != String::npos)) {
                                    responseHeaders[Trim(line.substr(0, pos))] = Trim(line.substr(pos + 1));
                                }
                            }
                        } else {
                            break;
                        }
                    }

                    if (ER_OK == status) {
                        /* Setup response stream */
                        httpSource.SetContentLength(StringToU32(responseHeaders["Content-Length"], 10, 0));

                        /*We need to parse the payload only if we have a payload in the response*/
                        if (httpSource.GetContentLength() != 0) {
                            char* buf = (char*)malloc(httpSource.GetContentLength() + 1);
                            size_t reqBytes = httpSource.GetContentLength();
                            size_t actual;
                            status = httpSource.PullBytes(buf, reqBytes, actual);

                            if ((ER_OK == status) && (httpSource.GetContentLength() == actual)) {
                                buf[actual] = '\0';
                                string responseStr = string(buf);

                                // Parse the payload using the JSON parser only of the HTTP status code received is
                                // HTTP_STATUS_OK.
                                if (httpStatus == HTTP_STATUS_OK) {
                                    Json::Reader reader;
                                    if (!reader.parse(responseStr, response.payload)) {
                                        status = ER_FAIL;
                                        QCC_LogError(status, ("HttpConnection::ParseResponse(): JSON payload parsing failed"));
                                    } else {
                                        response.payloadPresent = true;
                                    }
                                }
                            } else {
                                status = ER_FAIL;
                                QCC_LogError(status, ("HttpConnection::ParseResponse(): Payload parsing failed"));
                            }
                            free(buf);
                        } else {
                            QCC_DbgPrintf(("HttpConnection::ParseResponse(): Received a response with no payload"));
                        }
                    }
                }
            }
        }

        /* Cleanup socket on error*/
        if (ER_OK != status) {
            Close();
        }

        return status;
    }
}

QStatus HttpConnection::CheckHTTPResponseStatus(HttpStatus& verifiedStatus, uint32_t status) {
    QCC_DbgPrintf(("HttpConnection::CheckHTTPResponseStatus(): status = %d", status));
    QStatus retStatus = ER_OK;

    switch (status) {
    case 200:
        verifiedStatus = HTTP_STATUS_OK;
        break;

    case 400:
        verifiedStatus = HTTP_STATUS_BAD_REQUEST;
        break;

    case 401:
        verifiedStatus = HTTP_UNAUTHORIZED_REQUEST;
        break;

    case 404:
        verifiedStatus = HTTP_STATUS_NOT_FOUND;
        break;

    case 405:
        verifiedStatus = HTTP_STATUS_METH_NOT_ALLOW;
        break;

    case 406:
        verifiedStatus = HTTP_STATUS_NOT_ACCEPTABLE;
        break;

    case 411:
        verifiedStatus = HTTP_STATUS_LENGTH_REQUIRED;
        break;

    case 500:
        verifiedStatus = HTTP_STATUS_INTERNAL_ERROR;
        break;

    case 501:
        verifiedStatus = HTTP_STATUS_NOT_IMPLEMENTED;
        break;

    case 503:
        verifiedStatus = HTTP_STATUS_UNAVAILABLE;
        break;

    case 505:
        verifiedStatus = HTTP_STATUS_VERSION_ERROR;
        break;

    default:
        verifiedStatus = HTTP_STATUS_INVALID;
        retStatus = ER_FAIL;
        QCC_LogError(retStatus, ("HttpConnection::CheckHTTPResponseStatus(): Unrecognized Status Code"));
    }

    return retStatus;
}
