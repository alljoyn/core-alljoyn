/**
 * @file
 * Wrapper to encapsulate the AboutService with a singelton.
 */
/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#ifndef ABOUT_DATA_API_H
#define ABOUT_DATA_API_H

#include <qcc/Log.h>
#include <alljoyn/version.h>
#include <alljoyn/AboutObj.h>
#include <alljoyn/AboutData.h>

#if defined(QCC_OS_GROUP_WINDOWS)
/* Disabling warning C 4100. Function doesnt use all passed in parameters */
#pragma warning(push)
#pragma warning(disable: 4100)
#endif


namespace ajn {
namespace services {

/**
 *      AboutObjApi is wrapper class that encapsulates the AboutObj with a Singleton.
 */
class AboutObjApi {

  public:

    /**
     * GetInstance
     * @return AboutDataApi created only once.
     */
    static AboutObjApi* getInstance();
    /**
     * Init with  BusAttachment and AboutData only once.
     * After the first Init you can call getInstance to receive a proper instance of the class
     * @param bus
     * @param aboutData
     * @param aboutObj
     */
    static void Init(ajn::BusAttachment* bus, AboutData* aboutData, AboutObj* aboutObj);
    /**
     * Destroy the instance only once after finished
     */
    static void DestroyInstance();
    /**
     * SetPort - set the port for the announce
     * @param sessionPort
     */
    void SetPort(SessionPort sessionPort);
    /**
     * This is used to send the Announce signal.
     */
    QStatus Announce();

  private:
    /**
     * Constructor
     */
    AboutObjApi();
    /**
     * Desctructor
     */
    virtual ~AboutObjApi();
    /**
     *  pointer to AboutDataApi
     */
    static AboutObjApi* m_instance;
    /**
     * pointer to BusAttachment
     */
    static BusAttachment* m_BusAttachment;
    /**
     * pointer to AboutData
     */
    static AboutData* m_AboutData;
    /**
     * pointer to AboutObj
     */
    static AboutObj* m_AboutObj;
    /**
     * sesstion port for the about announcement
     */
    static SessionPort m_sessionPort;

};

}
}
#endif /* ABOUT_SERVICE_API_H */
