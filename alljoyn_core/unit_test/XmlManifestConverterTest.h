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

#include "XmlConverterTest.h"

#define VALID_RULES_ELEMENT \
    "<rules>" \
    "<node>" \
    "<interface>" \
    "<method>" \
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>" \
    "</method>" \
    "<property>" \
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>" \
    "</property>" \
    "<signal>" \
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>" \
    "</signal>" \
    "</interface>" \
    "</node>" \
    "</rules>"

static AJ_PCSTR s_validManifest =
    "<manifest>"
    "<version>1</version>"
    VALID_RULES_ELEMENT
    "<thumbprint>"
    "<oid>2.16.840.1.101.3.4.2.1</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</thumbprint>"
    "<signature>"
    "<oid>1.2.840.10045.4.3.2</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</signature>"
    "</manifest>";