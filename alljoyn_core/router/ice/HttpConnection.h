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

#ifndef _HTTPCONNECTION_H
#define _HTTPCONNECTION_H

#include <map>
#include <string>
#include <qcc/platform.h>
#include <qcc/Socket.h>
#include <qcc/SocketStream.h>
#include <qcc/Event.h>
#include <JSON/json.h>
#include <alljoyn/Status.h>

using namespace qcc;

/**
 * HttpResponseSource is a Source wrapper that keeps track of the number of bytes read
 * from the stream. This behaviour is needed for persistent connecitons in order to
 * demark the end of one response from the beginning of the next one.
 */
class HttpResponseSource : public Source {
  public:

    /**
     * Construct an HttpResponseSource wrapper.
     *
     * @param source  Raw source of HTTP response data.
     **/
    HttpResponseSource(Source& source = Source::nullSource) : source(&source), contentLength(0), bytesRead(0) { }

    /**
     * Retrieve bytes from source.
     *
     * @param buf          Buffer to store pulled bytes
     * @param reqBytes     Number of bytes requested to be pulled from source.
     * @param actualBytes  Actual number of bytes retrieved from source.
     * @return   OI_OK if successful. ER_NONE if source is exhausted. Otherwise an error.
     */
    QStatus PullBytes(void*buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout = Event::WAIT_FOREVER);

    /**
     * Get the Event indicating that data is available when signaled.
     *
     * @return Event that is signaled when data is available.
     */
    Event&  GetSourceEvent() { return source->GetSourceEvent(); }

    /**
     * Get total length of response stream.
     * @return Number of bytes in response stream.
     */
    size_t GetContentLength(void) { return contentLength; }

    /**
     * Get the number of bytes already read from source.
     * @return Number of bytes already read from source.
     */
    size_t GetBytesRead(void) { return bytesRead; }

    /**
     * Set the content length.
     *
     * @param contentLength   Total number of content bytes in the response.
     */
    void SetContentLength(uint32_t contentLength) { this->contentLength = contentLength; }

    /**
     * Reset this response source in preparation for reuse.
     *
     * @param source   Raw source.
     */
    void Reset(Source& source);

  private:
    Source* source;     /**< Underlying HTTP(s) source */
    size_t contentLength;    /**< Number of bytes in response stream */
    size_t bytesRead;        /**< Number of bytes already read from stream */
};

/**
 * The HttpConnection class is responsible for issuing HTTP/HTTPS requests and retrieving the corresponding responses.
 */
class HttpConnection {
  public:

    typedef enum _HttpStatus {
        HTTP_STATUS_INVALID         = 0,
        /** HTTP Success Responses */
        /**< OK */
        HTTP_STATUS_OK              = 200,

        /** HTTP Client Error Responses */
        /**< Request was not understood */
        HTTP_STATUS_BAD_REQUEST     = 400,
        /**< Unauthorized request received */
        HTTP_UNAUTHORIZED_REQUEST     = 401,
        /**< Resource not found */
        HTTP_STATUS_NOT_FOUND       = 404,
        /**< Method not allowed */
        HTTP_STATUS_METH_NOT_ALLOW  = 405,
        /**< Accept header cannot be satisfied */
        HTTP_STATUS_NOT_ACCEPTABLE  = 406,
        /**< Content-Length required */
        HTTP_STATUS_LENGTH_REQUIRED = 411,

        /** HTTP Server Error Responses */
        /**< Internal server error */
        HTTP_STATUS_INTERNAL_ERROR  = 500,
        /**< Server does not support operation */
        HTTP_STATUS_NOT_IMPLEMENTED = 501,
        /**< Server unavailable */
        HTTP_STATUS_UNAVAILABLE     = 503,
        /**< HTTP version not supported */
        HTTP_STATUS_VERSION_ERROR   = 505
    } HttpStatus;

    /**
     * Internal function to verify that a received HTTP response code is one
     * of the recognized ones.
     */
    QStatus CheckHTTPResponseStatus(HttpStatus& verifiedStatus, uint32_t status);

    /** Protocol */
    typedef enum {
        PROTO_HTTP,
        PROTO_HTTPS
    } Protocol;

    /** HTTP Method */
    typedef enum {
        METHOD_INVALID,
        METHOD_POST,
        METHOD_PUT,
        METHOD_GET,
        METHOD_DELETE
    } Method;

    String GetHTTPMethodString(Method method) {
        String retStr = String("INVALID");

        switch (method) {

        case METHOD_POST:
            retStr = String("POST ");
            break;

        case METHOD_PUT:
            retStr = String("PUT ");
            break;

        case METHOD_GET:
            retStr = String("GET ");
            break;

        case METHOD_DELETE:
            retStr = String("DELETE ");
            break;

        default:
        case METHOD_INVALID:
            break;
        }

        return retStr;
    }

    /**
     * @internal
     *
     * @brief HTTP Response.
     */
    class HTTPResponse {

      public:
        /* The received HTTP status code */
        HttpConnection::HttpStatus statusCode;

        /* If set to true, valid payload is present */
        bool payloadPresent;

        /* Received payload */
        Json::Value payload;

        HTTPResponse() : payloadPresent(false) { }
    };

    /** Default Constructor */
    HttpConnection() : stream(0),
        httpSource(),
        host("127.0.0.1"),
        port(0),
        protocol(PROTO_HTTP),
        httpStatus(HTTP_STATUS_INVALID),
        isMultipartForm(false),
        isApplicationJson(false)
    {
    }

    /** Destructor */
    ~HttpConnection();

    /**
     * Set the destination host name.
     *
     * @param host      Destination hostname for HTTP connection.
     */
    void SetHost(String host);

    /**
     * Set the destination host IP address.
     *
     * @param host      Destination host (IP address or hostname) for HTTP connection.
     */
    QStatus SetHostIPAddress(String host);

    /**
     * Set to destination port.
     * Port defaults to standard port for chosen Protocol.
     * It is not valid to change the port when reusing an existing HttpConnection (see {@link #Clear}).
     *
     * @param  port    Destination port number to contact.
     */
    void SetPort(uint16_t port) { this->port = port; }

    /**
     * Set the protocol.
     * The protocol defaults to PROTO_HTTP.
     * It is not valid to change the protocol when reusing an existing HttpConnection (see {@link #Clear}).
     *
     * @param protocol    Protocol used to contact host
     */
    void SetProtocol(Protocol protocol) { this->protocol = protocol; }

    /**
     * Set the URL path.
     *
     * @param urlPath     Path portion of URL.
     */
    void SetUrlPath(String urlPath);

    /**
     * Add a query parameter to the request.
     *
     * @param name     Parameter name
     * @param value    Parameter value
     */
    void AddQueryParameter(String name, String value);

    /**
     * Set the HTTP method.
     *
     * @param method   HTTP_METHOD for this connection.
     */
    void SetMethod(Method method) { this->method = method; }

    /**
     * Get the HTTP method.
     *
     * @param method   HTTP_METHOD for this connection.
     */
    Method GetMethod(void) { return(this->method); }

    /**
     * Add or replace a request header..
     *
     * @param name     Header name.
     * @param value    Header value.
     */
    void SetRequestHeader(String name, String value) { requestHeaders[name] = value; }

    /**
     * Add a form field using multipart/form-data.
     * Must be used with Method == POST.
     *
     * @param name          Field name.
     * @param content       Field value.
     * @param contentType   Optional content type for multipart value
     */
    void AddMultipartFormField(String name, String content, String contentType = String());

    /**
     * Add a JSON field using application/json.
     *
     * @param content       Field value.
     */
    void AddApplicationJsonField(String content);

    /**
     * Connect to destination.
     * This call blocks until connection attempt is complete.
     *
     * @param sock         Socket using which a HTTP connection needs to be
     *                     established with the Server.
     * @return ER_OK if successful.
     */
    QStatus Connect(SocketFd sock);

    /**
     * Send request to destination. This call does not wait for a response.
     * The handling of responses is done in an asynchronous fashion by
     * another thread. This is done so in order to support HTTP pipelining
     *
     * @return ER_OK if successful.
     */
    QStatus Send(void);

    /**
     * Clear previous request/response state.
     * This method is called before reusing an HttpConnection in order to achieve HTTP 1.1 persistence.
     * Reusing an HttpConnection implies that the underlying TCP connection will be reused. Therefore,
     * it is not valid to change the host, port or protocol associated with a reused HttpConnection.
     */
    void Clear();

    /**
     * Get the HTTP Status Code.
     * The status code must be checked before attempting to read response data.
     * If non-blocking Connect is in use (see {@link #Connect}) and the server has not yet
     * sent the response then this call will block until the response is received. Read select()
     * on the response stream's descriptor to avoid blocking here.
     *
     * @param  status   Written with HTTP status code if return status is ER_OK.
     * @return   ER_OK if successful.
     */
    QStatus GetStatusCode(HttpStatus& status);

    /**
     * Get the response headers.
     * Response headers are only valid after calling GetStatusCode()
     *
     * @return Map of HTTP(s) response headers
     */
    std::map<String, String> GetResponseHeaders() { return responseHeaders; }

    /**
     * Get response byte stream.
     *
     * @return  Response source.
     */
    HttpResponseSource& GetResponseSource() { return httpSource; }

    /**
     * Indicate whether HttpConnection is connected to destination.
     *
     * @return true if connected.
     */
    bool IsConnected(void) { return stream != NULL; }

    /**
     * Close an HttpConnection.
     */
    void Close();


    /** Helper used to parse response */
    QStatus ParseResponse(HTTPResponse& response);

    /** Helper used to find if no payload was received in the response */
    bool IsPayloadEmpty(void) {
        return (httpSource.GetContentLength() == 0);
    }

    /** Returns the IPAddress of the local interface over which the HTTP connection exists */
    IPAddress GetLocalInterfaceAddress(void) { return localIPAddress; };

    /** Returns the IP address of the host */
    void GetHostIPAddress(qcc::String& address) { address = hostIPAddress; };

  private:
    /* Just defined to make klocwork happy. Should never be used */
    HttpConnection(const HttpConnection& other) :
        stream(0),
        httpSource(),
        host(other.host),
        port(other.port),
        protocol(other.protocol),
        httpStatus(other.httpStatus),
        isMultipartForm(other.isMultipartForm),
        isApplicationJson(other.isApplicationJson)
    {
        /* This constructor should never be invoked */
        assert(false);
    }

    /* Just defined to make klocwork happy. Should never be used */
    HttpConnection& operator=(const HttpConnection& other) {
        /* This operator should never be invoked */
        assert(false);

        if (this != &other) {
            stream = 0;
            host = other.host;
            port = other.port;
            protocol = other.protocol;
            httpStatus = other.httpStatus;
            isMultipartForm = other.isMultipartForm;
            isApplicationJson = other.isApplicationJson;
        }

        return *this;
    }

    /**
     * @internal
     *
     * @brief Name resolution timeout.
     */
    /*PPN - Review duration*/
    static const uint32_t NAME_RESOLUTION_TIMEOUT_IN_MS = 5000;

    Stream* stream;                           /**< HTTP response stream */
    HttpResponseSource httpSource;            /**< Source wrapper */
    String host;                              /**< Destination host */
    String hostIPAddress;                     /**< Destination host IP Address*/
    uint16_t port;                            /**< Destination port */
    Protocol protocol;                        /**< Protocol */
    Method method;                            /**< HTTP method */
    String urlPath;                           /**< File path portion of request URL */
    String query;                             /**< Query string portion of request URL */
    HttpStatus httpStatus;                    /**< Status returned from HTTP server */
    String requestBody;                       /**< Request body (used for POST) */
    bool isMultipartForm;                     /**< true iff request is a multipart form post */
    bool isApplicationJson;                   /**< true iff request is a application/json format */
    std::map<String, String> requestHeaders;  /**< HTTP headers sent in request */
    std::map<String, String> responseHeaders; /**< HTTP headers sent in response */
    IPAddress localIPAddress;                 /**< IP address of the local interface to be used for connection*/
};

#endif
