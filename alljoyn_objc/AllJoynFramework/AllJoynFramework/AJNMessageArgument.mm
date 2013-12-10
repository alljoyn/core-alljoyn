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

#import <alljoyn/MsgArg.h>
#import "AJNMessageArgument.h"

using namespace ajn;

class MsgArgEx : public MsgArg
{
public:
    static QStatus Get(MsgArg* pMsgArg, const char* signature, va_list* argp)
    {
        size_t sigLen = (signature ? strlen(signature) : 0);
        if (sigLen == 0) {
            return ER_BAD_ARG_1;
        }
        QStatus status = VParseArgs(signature, sigLen, pMsgArg, 1, argp);
        return status;        
    }
    
    static QStatus Set(MsgArg* pMsgArg, const char* signature, va_list* argp)
    {
        QStatus status = ER_OK;
        
        pMsgArg->Clear();
        size_t sigLen = (signature ? strlen(signature) : 0);
        if ((sigLen < 1) || (sigLen > 255)) {
            status = ER_BUS_BAD_SIGNATURE;
        } else {
            status = VBuildArgs(signature, sigLen, pMsgArg, 1, argp);
            if ((status == ER_OK) && (*signature != 0)) {
                status = ER_BUS_NOT_A_COMPLETE_TYPE;
            }
        }
        return status;        
    }
};

@interface AJNObject(Private)

@property (nonatomic) BOOL shouldDeleteHandleOnDealloc;

@end


@interface AJNMessageArgument()

/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
@property (nonatomic, readonly) MsgArg *msgArg;

@end

@implementation AJNMessageArgument

/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
- (MsgArg *)msgArg
{
    return (MsgArg*)self.handle;
}

- (AJNType)type
{
    return (AJNType)self.msgArg->typeId;
}

- (NSString *)signature
{
    return [NSString stringWithCString:self.msgArg->Signature().c_str() encoding:NSUTF8StringEncoding];
}

- (NSString *)xml
{
    return [NSString stringWithCString:self.msgArg->ToString().c_str() encoding:NSUTF8StringEncoding];
}

- (id)init
{
    self = [super init];
    if (self) {
        self.handle = new MsgArg();
        self.shouldDeleteHandleOnDealloc = YES;
    }
    return self;
}

- (NSString *)signatureFromMessageArguments:(NSArray *)arguments
{
    NSString *result = nil;
    if (arguments.count) {
        MsgArg * pArgs = new MsgArg[arguments.count];
        for (int i = 0; i < arguments.count; i++) {
            pArgs[i] = *[[arguments objectAtIndex:i] msgArg];
        }
        result = [NSString stringWithCString:self.msgArg->Signature(pArgs, arguments.count).c_str() encoding:NSUTF8StringEncoding];
        delete [] pArgs;
    }
    return result;
}

- (NSString *)xmlFromMessageArguments:(NSArray*)arguments
{
    NSString *result = nil;
    if (arguments.count) {
        MsgArg * pArgs = new MsgArg[arguments.count];
        for (int i = 0; i < arguments.count; i++) {
            pArgs[i] = *[[arguments objectAtIndex:i] msgArg];
        }
        result = [NSString stringWithCString:self.msgArg->ToString(pArgs, arguments.count).c_str() encoding:NSUTF8StringEncoding];
        delete [] pArgs;
    }
    return result;
}

- (BOOL)conformsToSignature:(NSString *)signature
{
    return self.msgArg->HasSignature([signature UTF8String]);
}

- (QStatus)setValue:(NSString *)signature, ...
{
    va_list args;
    va_start(args, signature);
    QStatus status = MsgArgEx::Set(self.msgArg, [signature UTF8String], &args);
    va_end(args);
    return status;
}

- (QStatus)value:(NSString *)signature, ...
{
    va_list args;
    va_start(args, signature);
    QStatus status = MsgArgEx::Get(self.msgArg, [signature UTF8String], &args);
    va_end(args);
    return status;
}

- (void)clear
{
    self.msgArg->Clear();
}

- (void)stabilize
{
    self.msgArg->Stabilize();
}

- (void)setOwnershipFlags:(uint8_t)flags shouldApplyRecursively:(BOOL)deep
{
    self.msgArg->SetOwnershipFlags(flags, deep == YES);
}

- (void)dealloc
{
    if (self.shouldDeleteHandleOnDealloc) {
        MsgArg *pArg = static_cast<MsgArg*>(self.handle);
        delete pArg;
        self.handle = nil;
    }
}

@end
