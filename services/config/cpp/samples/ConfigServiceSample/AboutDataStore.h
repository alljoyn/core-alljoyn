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
#ifndef ABOUT_DATA_STORE_H_
#define ABOUT_DATA_STORE_H_

#include <stdio.h>
#include <iostream>
#include <alljoyn/config/AboutDataStoreInterface.h>

/**
 * class AboutDataStore
 * Property store implementation
 */
class AboutDataStore : public AboutDataStoreInterface {

  public:
    /**
     * AboutDataStore - constructor
     * @param factoryConfigFile
     * @param configFile
     */
    AboutDataStore(const char* factoryConfigFile, const char* configFile);

    /**
     * SetOBCFG
     */
    void SetOBCFG();

    /**
     * FactoryReset
     */
    void FactoryReset();

    /**
     * GetConfigFileName
     * @return qcc::String&
     */
    const qcc::String& GetConfigFileName();

    /**
     * virtual Destructor
     */
    virtual ~AboutDataStore();

    /**
     * virtual method ReadAll
     * @param languageTag
     * @param filter
     * @param all
     * @return QStatus
     */
    virtual QStatus ReadAll(const char* languageTag, DataPermission::Filter filter, ajn::MsgArg& all);

    /**
     * virtual method Update
     * @param name
     * @param languageTag
     * @param value
     * @return QStatus
     */
    virtual QStatus Update(const char* name, const char* languageTag, const ajn::MsgArg* value);

    /**
     * virtual method Delete
     * @param name
     * @param languageTag
     * @return QStatus
     */
    virtual QStatus Delete(const char* name, const char* languageTag);

    /**
     * Write about data store as an xml config file
     */
    void write();

    /**
     * method Initialize
     */
    void Initialize(qcc::String deviceId = qcc::String(), qcc::String appId = qcc::String());

  private:
    bool m_IsInitialized;
    qcc::String m_configFileName;
    qcc::String m_factoryConfigFileName;

    qcc::String ToXml(AboutData* aboutData);

    QStatus IsLanguageSupported(const char* languageTag);
};

#endif /* ABOUT_DATA_STORE_H_ */