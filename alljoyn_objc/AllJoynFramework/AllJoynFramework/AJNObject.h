////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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
#import "AJNHandle.h"

/**
 *  The base class for all AllJoyn API objects
 */
@interface AJNObject : NSObject<AJNHandle>

/** A handle to the C++ API object associated with this objective-c class */
@property (nonatomic, assign) AJNHandle handle;

/** Initialize the API object
    @param handle The handle to the C++ API object associated with this objective-c API object
 */
- (id)initWithHandle:(AJNHandle)handle;

/** Initialize the API object
   @param handle The handle to the C++ API object associated with this objective-c API object.
   @param deletionFlag A flag indicating whether or not the objective-c class should call delete on the handle when dealloc is called.
 */
- (id)initWithHandle:(AJNHandle)handle shouldDeleteHandleOnDealloc:(BOOL)deletionFlag;

@end
