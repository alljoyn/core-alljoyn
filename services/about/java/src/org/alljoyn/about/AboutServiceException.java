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

package org.alljoyn.about;

/**
 * The exception is thrown when an internal error has occurred in the {@link AboutService}
 * @deprecated
 */
@Deprecated
public class AboutServiceException extends RuntimeException {
    private static final long serialVersionUID = -6407441278632848769L;

    @Deprecated
    public AboutServiceException() {
    }

    /**
     * @deprecated
     * @param message user-defined message
     */
    @Deprecated
    public AboutServiceException(String message) {
        super(message);
    }

    /**
     * @deprecated
     * @param cause the cause of this exception
     */
    @Deprecated
    public AboutServiceException(Throwable cause) {
        super(cause);
    }

    /**
     * @deprecated
     * @param message user-defined message
     * @param cause the cause of this exception
     */
    @Deprecated
    public AboutServiceException(String message, Throwable cause) {
        super(message, cause);
    }

}
