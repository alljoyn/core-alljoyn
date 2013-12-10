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
#import <alljoyn/BusListener.h>
#import <alljoyn/TransportMask.h>
#import "AJNBusListener.h"
#import "AJNBusAttachment.h"

/** Internal class to bind the C++ API bus listener with the objective-c bus listener
 */
class AJNBusListenerImpl : public ajn::BusListener {
  protected:
    __weak AJNBusAttachment* busAttachment;

    /**
     * Objective C delegate called when one of the below virtual functions
     * is called.
     */
    __weak id<AJNBusListener> m_delegate;

  public:

    /**
     * Constructor for the AJN bus listener implementation.
     *
     * @param aBusAttachment    Objective C bus attachment wrapper object.
     * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.
     */
    AJNBusListenerImpl(AJNBusAttachment* aBusAttachment, id<AJNBusListener> aDelegate);

    /**
     * Virtual destructor for derivable class.
     */
    virtual ~AJNBusListenerImpl();

    /**
     * Called by the bus when the listener is registered. This give the listener implementation the
     * opportunity to save a reference to the bus.
     *
     * @param bus  The bus the listener is registered with.
     */
    virtual void ListenerRegistered(ajn::BusAttachment* bus);

    /**
     * Called by the bus when the listener is unregistered.
     */
    virtual void ListenerUnregistered();

    /**
     * Called by the bus when an external bus is discovered that is advertising a well-known name
     * that this attachment has registered interest in via a DBus call to org.alljoyn.Bus.FindAdvertisedName
     *
     * @param name         A well known name that the remote bus is advertising.
     * @param transport    Transport that received the advertisement.
     * @param namePrefix   The well-known name prefix used in call to FindAdvertisedName that triggered this callback.
     */
    virtual void FoundAdvertisedName(const char* name, ajn::TransportMask transport, const char* namePrefix);

    /**
     * Called by the bus when an advertisement previously reported through FoundName has become unavailable.
     *
     * @param name         A well known name that the remote bus is advertising that is of interest to this attachment.
     * @param transport    Transport that stopped receiving the given advertised name.
     * @param namePrefix   The well-known name prefix that was used in a call to FindAdvertisedName that triggered this callback.
     */
    virtual void LostAdvertisedName(const char* name, ajn::TransportMask transport, const char* namePrefix);

    /**
     * Called by the bus when the ownership of any well-known name changes.
     *
     * @param busName        The well-known name that has changed.
     * @param previousOwner  The unique name that previously owned the name or NULL if there was no previous owner.
     * @param newOwner       The unique name that now owns the name or NULL if the there is no new owner.
     */
    virtual void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner);

    /**
     * Called when a BusAttachment this listener is registered with is stopping.
     */
    virtual void BusStopping();

    /**
     * Called when a BusAttachment this listener is registered with is has become disconnected from
     * the bus.
     */
    virtual void BusDisconnected();

    /**
     * Accessor for Objective-C delegate.
     *
     * return delegate         The Objective-C delegate called to handle the above event methods.
     */
    id<AJNBusListener> getDelegate();

    /**
     * Mutator for Objective-C delegate.
     *
     * @param delegate    The Objective-C delegate called to handle the above event methods.
     */
    void setDelegate(id<AJNBusListener> delegate);
};

// inline methods
//

inline id<AJNBusListener> AJNBusListenerImpl::getDelegate()
{
    return m_delegate;
}

inline void AJNBusListenerImpl::setDelegate(id<AJNBusListener> delegate)
{
    m_delegate = delegate;
}
