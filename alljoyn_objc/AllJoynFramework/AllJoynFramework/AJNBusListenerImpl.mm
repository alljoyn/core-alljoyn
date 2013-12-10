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

#import "AJNBusListenerImpl.h"

/**
 * Constructor for the AJN bus listener implementation.
 *
 * @param aBusAttachment    Objective C bus attachment wrapper object.
 * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.     
 */    
AJNBusListenerImpl::AJNBusListenerImpl(AJNBusAttachment *aBusAttachment, id<AJNBusListener> aDelegate) : busAttachment(aBusAttachment), m_delegate(aDelegate)
{

}

/**
 * Virtual destructor for derivable class.
 */
AJNBusListenerImpl::~AJNBusListenerImpl()
{
    busAttachment = nil;
    m_delegate = nil;
}

/**
 * Called by the bus when the listener is registered. This give the listener implementation the
 * opportunity to save a reference to the bus.
 *
 * @param bus  The bus the listener is registered with.
 */
void AJNBusListenerImpl::ListenerRegistered(ajn::BusAttachment* bus)
{
    if ([m_delegate respondsToSelector:@selector(listenerDidRegisterWithBus:)]) {
        __block id<AJNBusListener> theDelegate = m_delegate;
        __block AJNBusAttachment *theBusAttachment = busAttachment;
        dispatch_queue_t queue = dispatch_get_main_queue();
        dispatch_async(queue, ^{
                [theDelegate listenerDidRegisterWithBus:theBusAttachment];
        });
    }        
}

/**
 * Called by the bus when the listener is unregistered.
 */
void AJNBusListenerImpl::ListenerUnregistered()
{
    if ([m_delegate respondsToSelector:@selector(listenerDidUnregisterWithBus:)]) {
        __block id<AJNBusListener> theDelegate = m_delegate;
        __block AJNBusAttachment *theBusAttachment = busAttachment;        
        dispatch_queue_t queue = dispatch_get_main_queue();
        dispatch_async(queue, ^{
                [theDelegate listenerDidUnregisterWithBus:theBusAttachment];
        });   
    }            
}

/**
 * Called by the bus when an external bus is discovered that is advertising a well-known name
 * that this attachment has registered interest in via a DBus call to org.alljoyn.Bus.FindAdvertisedName
 *
 * @param name         A well known name that the remote bus is advertising.
 * @param transport    Transport that received the advertisement.
 * @param namePrefix   The well-known name prefix used in call to FindAdvertisedName that triggered this callback.
 */
void AJNBusListenerImpl::FoundAdvertisedName(const char* name, ajn::TransportMask transport, const char* namePrefix)
{
    @autoreleasepool {
        if ([m_delegate respondsToSelector:@selector(didFindAdvertisedName:withTransportMask:namePrefix:)]) {
            NSString *advertisedName = [NSString stringWithCString:name encoding:NSUTF8StringEncoding];
            NSString *advertisedNamePrefix = [NSString stringWithCString:namePrefix encoding:NSUTF8StringEncoding];
            __block id<AJNBusListener> theDelegate = m_delegate;            
            dispatch_queue_t queue = dispatch_get_main_queue();
            dispatch_async(queue, ^{
                [theDelegate didFindAdvertisedName:advertisedName withTransportMask:(AJNTransportMask)transport namePrefix:advertisedNamePrefix];
            });        
        }            
    }
}

/**
 * Called by the bus when an advertisement previously reported through FoundName has become unavailable.
 *
 * @param name         A well known name that the remote bus is advertising that is of interest to this attachment.
 * @param transport    Transport that stopped receiving the given advertised name.
 * @param namePrefix   The well-known name prefix that was used in a call to FindAdvertisedName that triggered this callback.
 */
void AJNBusListenerImpl::LostAdvertisedName(const char* name, ajn::TransportMask transport, const char* namePrefix)
{
    @autoreleasepool {
        if ([m_delegate respondsToSelector:@selector(didLoseAdvertisedName:withTransportMask:namePrefix:)]) {
            __block id<AJNBusListener> theDelegate = m_delegate;            
            dispatch_queue_t queue = dispatch_get_main_queue();
            NSString *advertisedName = [NSString stringWithCString:name encoding:NSUTF8StringEncoding];
            NSString *advertisedNamePrefix = [NSString stringWithCString:namePrefix encoding:NSUTF8StringEncoding];
            dispatch_async(queue, ^{
                    [theDelegate didLoseAdvertisedName:advertisedName withTransportMask:(AJNTransportMask)transport namePrefix:advertisedNamePrefix];
            });
        }
    }
}

/**
 * Called by the bus when the ownership of any well-known name changes.
 *
 * @param busName        The well-known name that has changed.
 * @param previousOwner  The unique name that previously owned the name or NULL if there was no previous owner.
 * @param newOwner       The unique name that now owns the name or NULL if the there is no new owner.
 */
void AJNBusListenerImpl::NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
{
    @autoreleasepool {    
        if ([m_delegate respondsToSelector:@selector(nameOwnerChanged:to:from:)]) {

            NSString *aBusName;
            NSString *aPreviousOwner;
            NSString *aNewOwner;    
            
            if (busName) {
                aBusName = [NSString stringWithCString:busName encoding:NSUTF8StringEncoding];
            }
            
            if (previousOwner) {
                aPreviousOwner = [NSString stringWithCString:previousOwner encoding:NSUTF8StringEncoding];
            }
            
            if (newOwner) {
                aNewOwner = [NSString stringWithCString:newOwner encoding:NSUTF8StringEncoding];
            }
            __block id<AJNBusListener> theDelegate = m_delegate;            
            dispatch_queue_t queue = dispatch_get_main_queue();       
            dispatch_async(queue, ^{
                    [theDelegate nameOwnerChanged:aBusName to:aNewOwner from:aPreviousOwner];
            });        
        }
    }
}

/**
 * Called when a BusAttachment this listener is registered with is stopping.
 */
void AJNBusListenerImpl::BusStopping()
{
    if ([m_delegate respondsToSelector:@selector(busWillStop)]) {    
        __block id<AJNBusListener> theDelegate = m_delegate;        
        dispatch_queue_t queue = dispatch_get_main_queue();
        dispatch_async(queue, ^{
                [theDelegate busWillStop];
        });
    }
}

/**
 * Called when a BusAttachment this listener is registered with is has become disconnected from
 * the bus.
 */
void AJNBusListenerImpl::BusDisconnected()
{
    if ([m_delegate respondsToSelector:@selector(busDidDisconnect)]) {
        __block id<AJNBusListener> theDelegate = m_delegate;        
        dispatch_queue_t queue = dispatch_get_main_queue();
        dispatch_async(queue, ^{
            [theDelegate busDidDisconnect];
        });
    }
}
