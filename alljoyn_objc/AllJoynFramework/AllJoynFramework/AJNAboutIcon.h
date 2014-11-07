////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import "AJNObject.h"
#import "AJNMessageArgument.h"
#import "AJNStatus.h"


@interface AJNAboutIcon : AJNObject

- (id)init;

/**
 * Set the content for an icon.  The content must fit into a MsgArg so the
 * largest size for the icon data is ALLJOYN_MAX_ARRAY_LEN bytes.
 *
 * Note as long as the MIME type matches it is possible to set both icon content
 * and icon URL
 *
 * If an error is returned the icon content will remain unchanged.
 *
 * @param[in] mimetype a MIME type indicating the icon image type. Typical
 *                     value will be `image/jpeg` or `image/png`
 * @param[in] data     pointer to an array of bytes that represent an icon
 * @param[in] csize    The number of bytes in data
 * @param[in] ownsData If true the AboutIcon contain gains ownership of the
 *                     data that data points to.  AboutIcon class will be
 *                     responsible for freeing the memory. If false user is
 *                     is responsible for freeing memory pointed to by data.
 * @return
 *  - ER_OK on success
 *  - ER_BUS_BAD_VALUE if the data is to large to be marshaled
 *  - other status indicating failure
 */
- (QStatus)setContentWithMimeType:(NSString *)mimeType data:(uint8_t *)data size:(size_t)csize ownsFlag:(BOOL)ownsData;

/**
 * Set a url that contain the icon for the application.
 *
 * Note as long as the MIME type matches it is possible to set both icon content
 * and icon URL
 *
 * @param[in] mimetype a MIME type indicating the icon image type. Typical
 *                     value will be `image/jpeg` or `image/png`
 * @param[in] url      A URL that contain the location of the icon hosted in
 *                     the cloud.
 */
- (QStatus)setUrlWithMimeType:(NSString *)mimetype url:(NSString *)url;

/**
 * Clear the AboutIcon.  This will change all strings to empty strings
 * set the content to NULL and the contentSize to zero.
 */
- (void)clear;

/**
 * Add the IconContent from a MsgArg. This will make a local copy of the
 * MsgArg passed in and expose the contents as the member variables content
 * and contentSize.
 *
 * @param arg the MsgArg containing the Icon
 *
 * @return
 *   - ER_OK on success
 *   - status indicating failure otherwise
 */
- (QStatus)setContentUsingMsgArg:(AJNMessageArgument *)msgArg;

/**
 * Set the content for this icon
 * @param content the array of bytes containing the contents
 */
- (void)setContent:(uint8_t *)content;

/**
 * Get the content for this icon
 * @return
 *   - the array of bytes containing the contents
 */
- (uint8_t *)getContent;

/**
 * Set the url for this icon
 * @param url the string containing the url
 */
- (void)setUrl:(NSString *)url;

/**
 * Get the url for this icon
 * @return
 *   - string containing the url for the icon
 */
- (NSString *)getUrl;

/**
 * Set the content size for this icon
 * @param contentSize the size of array of bytes containing the contents
 */
- (void)setContentSize:(size_t)contentSize;

/**
 * Get the content size for this icon
 * @return
 *   - size of the array of bytes containing the contents
 */
- (size_t)getContentSize;

/**
 * Set the mime type for this icon
 * @param mimeType string indication the mime type
 */
- (void)setMimeType:(NSString *)mimeType;

/**
 * Get the mime type for this icon
 * @return
 *   - string indication the mime type
 */

- (NSString *)getMimeType;

@end
