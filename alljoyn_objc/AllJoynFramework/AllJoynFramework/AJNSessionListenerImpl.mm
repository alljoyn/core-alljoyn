////////////////////////////////////////////////////////////////////////////////
// // 
//    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
//    Source Project Contributors and others.
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0

////////////////////////////////////////////////////////////////////////////////

#import "AJNSessionListenerImpl.h"

using namespace ajn;

/**
 * Constructor for the AJN session listener implementation.
 *
 * @param aBusAttachment    Objective C bus attachment wrapper object.
 * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.     
 */    
AJNSessionListenerImpl::AJNSessionListenerImpl(AJNBusAttachment *aBusAttachment, id<AJNSessionListener> aDelegate) :
    m_delegate(aDelegate), busAttachment(aBusAttachment)
{
    
}

/**
 * Virtual destructor for derivable class.
 */
AJNSessionListenerImpl::~AJNSessionListenerImpl()
{
    m_delegate = nil;
    busAttachment = nil;
}

/**
 * Called by the bus when an existing session becomes disconnected.
 *
 * @param sessionId     Id of session that was lost.
 * @param reason        The reason for the session being lost
 *
 */
void AJNSessionListenerImpl::SessionLost(SessionId sessionId, SessionLostReason reason)
{
    @autoreleasepool {
        if ([m_delegate respondsToSelector:@selector(sessionWasLost:forReason:)]) {
            __block id<AJNSessionListener> theDelegate = m_delegate;
            dispatch_queue_t queue = dispatch_get_main_queue();
            dispatch_async(queue, ^{
                [theDelegate sessionWasLost:sessionId forReason:(AJNSessionLostReason)reason];
            });
        }
    }
}

/**
 * Called by the bus when a member of a multipoint session is added.
 *
 * @param sessionId     Id of session whose member(s) changed.
 * @param uniqueName    Unique name of member who was added.
 */
void AJNSessionListenerImpl::SessionMemberAdded(SessionId sessionId, const char* uniqueName)
{
    @autoreleasepool {
        if ([m_delegate respondsToSelector:@selector(didAddMemberNamed:toSession:)]) {
            NSString *aUniqueName = [NSString stringWithCString:uniqueName encoding:NSUTF8StringEncoding];
            __block id<AJNSessionListener> theDelegate = m_delegate;                        
            dispatch_queue_t queue = dispatch_get_main_queue();
            dispatch_async(queue, ^{
                [theDelegate didAddMemberNamed:aUniqueName toSession:sessionId];
            });
        }
    }
}

/**
 * Called by the bus when a member of a multipoint session is removed.
 *
 * @param sessionId     Id of session whose member(s) changed.
 * @param uniqueName    Unique name of member who was removed.
 */
void AJNSessionListenerImpl::SessionMemberRemoved(SessionId sessionId, const char* uniqueName)
{
    @autoreleasepool {    
        if ([m_delegate respondsToSelector:@selector(didRemoveMemberNamed:fromSession:)]) {
            NSString *aUniqueName = [NSString stringWithCString:uniqueName encoding:NSUTF8StringEncoding];            
            __block id<AJNSessionListener> theDelegate = m_delegate;                        
            dispatch_queue_t queue = dispatch_get_main_queue();
            dispatch_async(queue, ^{
                [theDelegate didRemoveMemberNamed:aUniqueName fromSession:sessionId];
            });
        }
    }    
}