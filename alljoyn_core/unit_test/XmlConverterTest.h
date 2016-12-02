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

#include <alljoyn/Status.h>
#include <qcc/platform.h>

struct SizeParams {
  public:
    AJ_PCSTR m_xml;
    size_t m_integer;

    SizeParams(AJ_PCSTR xml, size_t integer) :
        m_xml(xml),
        m_integer(integer)
    {
    }
};

struct StatusParams {
  public:
    AJ_PCSTR m_xml;
    QStatus m_status;

    StatusParams(AJ_PCSTR xml, QStatus status) :
        m_xml(xml),
        m_status(status)
    {
    }
};