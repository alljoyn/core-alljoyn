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
#import <alljoyn/BusAttachment.h>
#import <alljoyn/InterfaceDescription.h>
#import <alljoyn/MessageReceiver.h>
#import "AJNBusAttachment.h"
#import "AJNSignalHandler.h"

class AJNSignalHandlerImpl : public ajn::MessageReceiver {
  private:
    /**
     * Filter rule associated with this signal handler.
     * We need to keep track of this to facilitate proper removal of the signal handler from core.
     */
    NSString* m_matchRule;


  protected:

    /**
     * Objective C delegate called when one of the below virtual functions
     * is called.
     */
    __weak id<AJNSignalHandler> m_delegate;

  public:

    /**
     * Constructor for the AJN signal handler implementation.
     *
     * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.
     */
    AJNSignalHandlerImpl(id<AJNSignalHandler> aDelegate);

    /**
     * Pure virtual registration function. Implement in derived classes to handle
     * registration of signal handlers.
     */
    virtual void RegisterSignalHandler(ajn::BusAttachment& bus) = 0;

    /**
     * Pure virtual unregistration function. Implement in derived classes to
     * handle unregistration of signal handlers.
     */
    virtual void UnregisterSignalHandler(ajn::BusAttachment& bus) = 0;

    /**
     * Pure virtual registration function. Implement in derived classes to handle
     * registration of signal handlers.
     */
    virtual void RegisterSignalHandlerWithRule(ajn::BusAttachment& bus, const char* matchRule) = 0;

    /**
     * Pure virtual unregistration function. Implement in derived classes to
     * handle unregistration of signal handlers.
     */
    virtual void UnregisterSignalHandlerWithRule(ajn::BusAttachment& bus, const char* matchRule) = 0;

    /**
     * Virtual destructor for derivable class.
     */
    virtual ~AJNSignalHandlerImpl();

    /**
     * Accessor for Objective-C delegate.
     *
     * return delegate         The Objective-C delegate called to handle the above event methods.
     */
    id<AJNSignalHandler> getDelegate();

    /**
     * Mutator for Objective-C delegate.
     *
     * @param delegate    The Objective-C delegate called to handle the above event methods.
     */
    void setDelegate(id<AJNSignalHandler> delegate);

    /**
     * Accessor for filter rule.
     *
     * @return matchRule   The filter rule associated with this signal handler.
     */
    NSString* getFilterRule();

    /**
     * Mutator for filter rule.
     *
     * @param matchRule   The filter rule associated with this signal handler.
     */
    void setFilterRule(NSString* matchRule);
};

inline id<AJNSignalHandler> AJNSignalHandlerImpl::getDelegate()
{
    return m_delegate;
}

inline void AJNSignalHandlerImpl::setDelegate(id<AJNSignalHandler> delegate)
{
    m_delegate = delegate;
}

inline NSString* AJNSignalHandlerImpl::getFilterRule()
{
    return m_matchRule;
}

inline void AJNSignalHandlerImpl::setFilterRule(NSString* matchRule)
{
    m_matchRule = matchRule;
}
