/**
 * @file
 * A mechanism for easily specifying what transports should be instantiated by
 * a particular AllJoyn-enabled program.
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

#ifndef _ALLJOYN_TRANSPORTFACTORY_H
#define _ALLJOYN_TRANSPORTFACTORY_H

#include "Transport.h"

namespace ajn {

class TransportFactoryBase {
    qcc::String m_type;
    bool m_isDefault;

  public:
    TransportFactoryBase(const char* type, bool isDefault) : m_type(type), m_isDefault(isDefault) { }
    virtual ~TransportFactoryBase() { }

    bool IsDefault(void) { return m_isDefault; }
    qcc::String GetType(void) { return m_type; }

    virtual Transport* Create(BusAttachment& bus) = 0;
};

template <typename T>
class TransportFactory : public TransportFactoryBase {
  public:
    TransportFactory(const char* type, bool isDefault) : TransportFactoryBase(type, isDefault) { }
    virtual ~TransportFactory() { }

    virtual Transport* Create(BusAttachment& bus) { return new T(bus); }
};

class TransportFactoryContainer {
    std::vector<TransportFactoryBase*> m_factories;

  public:
    TransportFactoryContainer() { }

    virtual ~TransportFactoryContainer()
    {
        for (uint32_t i = 0; i < m_factories.size(); ++i) {
            delete m_factories[i];
        }
        m_factories.clear();
    }

    uint32_t Size(void) const { return m_factories.size(); }
    TransportFactoryBase* Get(uint32_t i) const { return m_factories[i]; }
    void Add(TransportFactoryBase* factory) { m_factories.push_back(factory); }
  private:
    TransportFactoryContainer(const TransportFactoryContainer& other);
    TransportFactoryContainer& operator=(const TransportFactoryContainer& other);
};

}  // namespace ajn

#endif // TRANSPORTFACTORY_H