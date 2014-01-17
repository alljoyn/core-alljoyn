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
                if (rootCert.empty() || caCert.empty()) {
                    status = ER_RENDEZVOUS_SERVER_ROOT_CERTIFICATE_UNINITIALIZED;
                    break;
                }

                if (0 == port) {
                    port = 443;
                }

                SslSocket* sslSocket = new SslSocket(host, rootCert.c_str(), caCert.c_str());
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
