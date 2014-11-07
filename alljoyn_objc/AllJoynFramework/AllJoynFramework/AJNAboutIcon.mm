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

#import "AJNAboutIcon.h"
#import <alljoyn/AboutIcon.h>

using namespace ajn;

@interface AJNMessageArgument(Private)
@property (nonatomic, readonly) MsgArg *msgArg;
@end

@interface AJNObject(Private)
@property (nonatomic) BOOL shouldDeleteHandleOnDealloc;
@end

@interface AJNAboutIcon()
@property (nonatomic, readonly) AboutIcon *aboutIcon;
@end

@implementation AJNAboutIcon

- (id)init
{
    self = [super init];
    if (self) {
        self.handle = new AboutIcon();
        self.shouldDeleteHandleOnDealloc = YES;
    }
    return self;
    
}


/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
- (AboutIcon*)aboutIcon
{
    return static_cast<AboutIcon*>(self.handle);
}

- (void)setContent:(uint8_t *)content
{
    self.aboutIcon->content = content;
}

- (uint8_t *)getContent
{
    return self.aboutIcon->content;
}

- (void)setUrl:(NSString *)url
{
    self.aboutIcon->url = [url UTF8String];
}

- (NSString *)getUrl
{
    return [NSString stringWithCString:(self.aboutIcon->url).c_str() encoding:NSUTF8StringEncoding ];
}

- (void)setContentSize:(size_t)contentSize
{
    self.aboutIcon->contentSize = contentSize;
}

- (size_t)getContentSize
{
    return self.aboutIcon->contentSize;
}

- (void)setMimeType:(NSString *)mimeType
{
    self.aboutIcon->mimetype = [mimeType UTF8String];
}

- (NSString *)getMimeType
{
    return [NSString stringWithCString:(self.aboutIcon->mimetype).c_str() encoding:NSUTF8StringEncoding ];
}

- (QStatus)setContentWithMimeType:(NSString *)mimeType data:(uint8_t *)data size:(size_t)csize ownsFlag:(BOOL)ownsData
{
    return self.aboutIcon->SetContent([mimeType UTF8String], data, csize, ownsData);
}

- (QStatus)setUrlWithMimeType:(NSString *)mimeType url:(NSString *)url
{
    return self.aboutIcon->SetUrl([mimeType UTF8String], [url UTF8String]);
}

- (void)clear
{
    self.aboutIcon->Clear();
}

- (QStatus)setContentUsingMsgArg:(AJNMessageArgument *)msgArg
{
    return self.aboutIcon->SetContent(*(msgArg.msgArg));
}

- (void)dealloc
{
    if (self.shouldDeleteHandleOnDealloc) {
        AboutIcon *pArg = static_cast<AboutIcon*>(self.handle);
        delete pArg;
        self.handle = nil;
    }
}


@end