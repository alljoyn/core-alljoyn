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

#import <alljoyn/KeystoreListener.h>
#import "AJNKeyStoreListener.h"

class AJNKeyStoreListenerImpl : public ajn::KeyStoreListener {
  protected:

    /**
     * Objective C delegate called when one of the below virtual functions
     * is called.
     */
    __weak id<AJNKeyStoreListener> m_delegate;

  public:

    /**
     * Constructor for the AJN key store handler implementation.
     *
     * @param aDelegate         Objective C delegate called when one of the below virtual functions is called.
     */
    AJNKeyStoreListenerImpl(id<AJNKeyStoreListener> aDelegate);

    /**
     * Virtual destructor for derivable class.
     */
    virtual ~AJNKeyStoreListenerImpl();

    /**
     * This method is called when a key store needs to be loaded.
     * @remark The application must call <tt>#PutKeys</tt> to put the new key store data into the
     * internal key store.
     *
     * @param keyStore   Reference to the KeyStore to be loaded.
     *
     * @return
     *      - ER_OK if the load request was satisfied
     *      - An error status otherwise
     *
     */
    QStatus LoadRequest(ajn::KeyStore& keyStore);


    /**
     * This method is called when a key store needs to be stored.
     * @remark The application must call <tt>#GetKeys</tt> to obtain the key data to be stored.
     *
     * @param keyStore   Reference to the KeyStore to be stored.
     *
     * @return
     *      - ER_OK if the store request was satisfied
     *      - An error status otherwise
     */
    QStatus StoreRequest(ajn::KeyStore& keyStore);


    /**
     * Get the current keys from the key store as an encrypted byte string.
     *
     * @param keyStore  The keyStore to get from. This is the keystore indicated in the StoreRequest call.
     * @param sink      The byte string to write the keys to.
     * @return
     *      - ER_OK if successful
     *      - An error status otherwise
     */
    QStatus GetKeys(ajn::KeyStore& keyStore, qcc::String& sink);


    /**
     * Put keys into the key store from an encrypted byte string.
     *
     * @param keyStore  The keyStore to put to. This is the keystore indicated in the LoadRequest call.
     * @param source    The byte string containing the encrypted key store contents.
     * @param password  The password required to decrypt the key data
     *
     * @return
     *      - ER_OK if successful
     *      - An error status otherwise
     *
     */
    QStatus PutKeys(ajn::KeyStore& keyStore, const qcc::String& source, const qcc::String& password);


    /**
     * Accessor for Objective-C delegate.
     *
     * return delegate         The Objective-C delegate called to handle the above event methods.
     */
    id<AJNKeyStoreListener> getDelegate();


    /**
     * Mutator for Objective-C delegate.
     *
     * @param delegate    The Objective-C delegate called to handle the above event methods.
     */
    void setDelegate(id<AJNKeyStoreListener> delegate);
};

// inline methods
//

inline id<AJNKeyStoreListener> AJNKeyStoreListenerImpl::getDelegate()
{
    return m_delegate;
}

inline void AJNKeyStoreListenerImpl::setDelegate(id<AJNKeyStoreListener> delegate)
{
    m_delegate = delegate;
}
