/**
 * @file
 * Wrapper to encapsulate the AboutService with a singelton.
 */
/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#ifndef ABOUT_SERVICE_API_H
#define ABOUT_SERVICE_API_H

#include <qcc/Log.h>
#include <alljoyn/version.h>
#include <alljoyn/about/AboutService.h>
#include <alljoyn/about/PropertyStore.h>

namespace ajn {
namespace services {

/**
 * AboutServiceApi  is wrapper class that encapsulates the AboutService with a Singelton.
 *
 * @deprecated The AboutServiceApi class has been deprecated please see the
 * AboutObj class. There is no direct equivalent class to the AboutServiceApi
 * since the use of a singelton has been completely removed.
 */
class AboutServiceApi : public AboutService {

  public:

    /**
     * GetInstance
     *
     * @deprecated The AboutServiceApi class has been deprecated please see the
     * AboutObj class.
     *
     * @return AboutServiceApi created only once.
     */
    QCC_DEPRECATED(static AboutServiceApi * getInstance());

    /**
     * Init with  BusAttachment and PropertyStore only once.
     * After the first Init you can call getInstance to receive a proper
     * instance of the class
     *
     * @deprecated The AboutServiceApi::Init function has been deprecated please see the
     * AboutObj class.
     *
     * @param bus
     * @param store
     */
    QCC_DEPRECATED(static void Init(ajn::BusAttachment & bus, PropertyStore & store));

    /**
     * Destroy the instance only once after finished
     *
     * @deprecated The AboutServiceApi::DestroyInstance function has been
     * deprecated please see the AboutObj class.
     */
    QCC_DEPRECATED(static void DestroyInstance());

  private:
    /**
     * Constructor
     * @param bus
     * @param store
     */
    AboutServiceApi(ajn::BusAttachment& bus, PropertyStore& store);
    /**
     * Desctructor
     */
    virtual ~AboutServiceApi();
    /**
     *  pointer to AboutServiceApi
     */
    static AboutServiceApi* m_instance;
    /**
     * pointer to BusAttachment
     */
    static BusAttachment* m_BusAttachment;
    /**
     * pointer to PropertyStore
     */
    static PropertyStore* m_PropertyStore;

};

}
}
#endif /* ABOUT_SERVICE_API_H */