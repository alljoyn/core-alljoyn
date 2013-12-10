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

#import "AJNKeyStoreListenerImpl.h"
#import "AJNHandle.h"

using namespace ajn;

/**
 * Constructor for the AJN key store handler implementation.
 *
 * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.     
 */    
AJNKeyStoreListenerImpl::AJNKeyStoreListenerImpl(id<AJNKeyStoreListener> aDelegate) : m_delegate(aDelegate)
{
    
}

/**
 * Virtual destructor for derivable class.
 */
AJNKeyStoreListenerImpl::~AJNKeyStoreListenerImpl()
{
    m_delegate = nil;
}

/**
 * This method is called when a key store needs to be loaded.
 * @remark The application must call <tt>#PutKeys</tt> to put the new key store data into the
 * internal key store.
 *
 * @param keyStore   Reference to the KeyStore to be loaded.
 *
 * @return  - ER_OK if the load request was satisfied
 *          - An error status otherwise
 *
 */
QStatus AJNKeyStoreListenerImpl::LoadRequest(KeyStore& keyStore)
{
    NSLog(@"AJNKeyStoreListenerImpl::LoadRequest");    
    QStatus status = ER_NONE;
    if (m_delegate) {
        status = [m_delegate load:(AJNHandle)(&keyStore)];
    }
    return status;
}

/**
 * This method is called when a key store needs to be stored.
 * @remark The application must call <tt>#GetKeys</tt> to obtain the key data to be stored.
 *
 * @param keyStore   Reference to the KeyStore to be stored.
 *
 * @return  - ER_OK if the store request was satisfied
 *          - An error status otherwise
 */
QStatus AJNKeyStoreListenerImpl::StoreRequest(KeyStore& keyStore)
{
    NSLog(@"AJNKeyStoreListenerImpl::StoreRequest");    
    QStatus status = ER_NONE;
    if (m_delegate) {
        status = [m_delegate store:(AJNHandle)(&keyStore)];
    }
    return status;    
}

/**
 * Get the current keys from the key store as an encrypted byte string.
 *
 * @param keyStore  The keyStore to get from. This is the keystore indicated in the StoreRequest call.
 * @param sink      The byte string to write the keys to.
 * @return  - ER_OK if successful
 *          - An error status otherwise
 */
QStatus AJNKeyStoreListenerImpl::GetKeys(ajn::KeyStore& keyStore, qcc::String& sink)
{
    NSLog(@"AJNKeyStoreListenerImpl::GetKeys");    
    QStatus status = ER_NONE;
    if (m_delegate) {
        status = [m_delegate getKeys:&keyStore sink:[NSString stringWithCString:sink.c_str() encoding:NSUTF8StringEncoding]];
    }
    return status;        
}

/**
 * Put keys into the key store from an encrypted byte string.
 *
 * @param keyStore  The keyStore to put to. This is the keystore indicated in the LoadRequest call.
 * @param source    The byte string containing the encrypted key store contents.
 * @param password  The password required to decrypt the key data
 *
 * @return  - ER_OK if successful
 *          - An error status otherwise
 *
 */
QStatus AJNKeyStoreListenerImpl::PutKeys(ajn::KeyStore& keyStore, const qcc::String& source, const qcc::String& password)
{    
    NSLog(@"AJNKeyStoreListenerImpl::PutKeys");    
    QStatus status = ER_NONE;
    if (m_delegate) {
        status = [m_delegate putKeys:&keyStore source:[NSString stringWithCString:source.c_str() encoding:NSUTF8StringEncoding] password:[NSString stringWithCString:password.c_str() encoding:NSUTF8StringEncoding]];
    }
    return status;        
}

